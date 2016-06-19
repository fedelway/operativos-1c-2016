/*
 ============================================================================

 Name        : umc.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
*/


#include "umc.h"

#define PACKAGESIZE 1024 //Define cual es el tamaño maximo del paquete a enviar

int main(int argc,char *argv[]) {

	char *swap_ip;
	char *swap_puerto;
	int max_fd = 0;
	logger = log_create("umc.log", "UMC",true, LOG_LEVEL_INFO);

	if(argc != 2){
		fprintf(stderr,"uso: umc config_path\n");
		return 1;
	}

	crearConfiguracion(argv[1]);

	swap_ip = config_get_string_value(config,"IP_SWAP");
	swap_puerto = config_get_string_value(config,"PUERTO_SWAP");

 	swap_fd = conectarseA(swap_ip, swap_puerto);
 	handshakeSWAP();

	//conexion a cpu
	char* cpu_puerto;
	cpu_puerto = config_get_string_value(config,"PUERTO_CPU");
	printf("Mi puerto escucha es: %s\n", cpu_puerto);
	int socket_CPU = conectarPuertoDeEscucha(cpu_puerto);

	char* nucleo_puerto;
	nucleo_puerto = config_get_string_value(config,"PUERTO_NUCLEO");
	printf("Mi puerto escucha es: %s\n", nucleo_puerto);
	nucleo_fd = conectarPuertoDeEscucha(nucleo_puerto);

	if(socket_CPU > nucleo_fd){
		max_fd = socket_CPU;
	}else max_fd = nucleo_fd;

	inicializarMemoria();

	inicializarTlb();

	aceptarNucleo();

	//Ciclo principal
	recibirConexiones(socket_CPU, max_fd);

	return EXIT_SUCCESS;
}

//Esta funcion va a ser el ciclo principal. Va a estar aceptando nuevas conexiones y creando hilos con cada nueva conexion
void recibirConexiones(int cpu_fd, int max_fd){

	//Creo el fd_set principal
	fd_set listen, readyListen;
	FD_ZERO(&listen);
	FD_ZERO(&readyListen);
	FD_SET(cpu_fd, &listen);
	FD_SET(swap_fd, &listen);

	int i;

	for(;;){
		readyListen = listen;

		select( (max_fd + 1), &readyListen, NULL, NULL, NULL);

		for(i=0; i <= max_fd; i++){

			if(FD_ISSET(i, &readyListen)){

				if( i == cpu_fd){
					pthread_t thread;
					pthread_attr_t atributos;

					pthread_attr_init(&atributos);
					pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);

					pthread_create(&thread, &atributos, (void *)trabajarCpu, (void*)cpu_fd);

				}
			}
		}

	}

}

void crearConfiguracion(char* config_path){

	config = config_create(config_path);

	if(validarParametrosDeConfiguracion()){
		printf("El archivo de configuracion tiene todos los parametros requeridos.\n");
	}else{
		printf("configuracion no valida\n");
		exit(EXIT_SUCCESS);
	}
}

bool validarParametrosDeConfiguracion(){
	return 	config_has_property(config, "PUERTO_CPU")
		&& 	config_has_property(config, "IP_SWAP")
		&& 	config_has_property(config, "PUERTO_SWAP")
		&& 	config_has_property(config, "MARCOS")
		&& 	config_has_property(config, "MARCO_SIZE")
		&& 	config_has_property(config, "MARCO_X_PROC")
		&& 	config_has_property(config, "ENTRADAS_TLB")
		&& 	config_has_property(config, "RETARDO")
		&&  config_has_property(config, "PUERTO_NUCLEO")
		&& 	config_has_property(config, "TIMER_RESET");
}

void recibirMensajeCPU(char* message, int socket_CPU){

	printf("voy a recibir mensaje de %d\n", socket_CPU);
	int status = 0;
	//while(status == 0){
	status = recv(socket_CPU, message, PACKAGESIZE, 0);
		if (status != 0){
			printf("recibo mensaje de CPU %d\n", status);
			printf("mensaje recibido: %s\n", message);
			status = 0;
		}else{
			sleep(1);
			printf(".\n");
		}
	//}
}

void enviarPaqueteASwap(char* message, int socket){
	//Envio el archivo entero


	int resultSend = send(socket, message, PACKAGESIZE, 0);
	printf("resultado send %d, a socket %d \n",resultSend, socket);
	if(resultSend == -1){
		printf ("Error al enviar archivo a SWAP.\n");
		exit(1);
	}else {
		printf ("Archivo enviado a SWAP.\n");
		printf ("mensaje: %s\n", message);
	}
}

void handshakeSWAP(){

	int mensaje;
	int soy_umc;

	log_info(logger, "Iniciando Handshake con la SWAP");
	soy_umc = SOY_UMC;

	//Tengo que recibir el ID del swap
	recv(swap_fd, &mensaje, sizeof(int), 0);
	printf("mensaje recibido %d\n", mensaje);

	send(swap_fd, &soy_umc, sizeof(int), 0);

	if(mensaje == SOY_SWAP){
		log_info(logger, "SWAP validado.");
		printf("Se ha validado correctamente la SWAP");
	}else{
		log_error(logger, "La SWAP no pudo ser validada.");
	    log_destroy(logger);
		exit(0);
	}
}

void inicializarMemoria(){
	cant_frames = config_get_int_value(config, "MARCOS");
	frame_size = config_get_int_value(config, "MARCO_SIZE");
	fpp = config_get_int_value(config, "MARCO_X_PROC");

	memoria = malloc(cant_frames * frame_size);

	if(memoria == NULL){
		printf("Error de malloc, no hay memoria.\n");
		exit(1);
	}

	memset(memoria,0b11111111,cant_frames*frame_size);

	//inicializo listas
	programas = list_create();

	//Creo el array de frames
	frames = malloc(cant_frames * sizeof(t_frame));
	int i;
	for(i = 0; i < cant_frames; i++){
		frames[i].libre = true;
		frames[i].posicion = i;
	}

	printf("Memoria inicializada.\n");
}

int framesDisponibles(){

	int cant_disp;
	int i;
	for(i=0 ; i < cant_frames; i++){
		if(frames[i].libre)
			cant_disp++;
	}

	return cant_disp;
}

void aceptarNucleo(){

	int msj_recibido;
	struct sockaddr_in addr; // Para recibir nuevas conexiones
	socklen_t addrlen = sizeof(addr);

	printf("Esperando conexion del nucleo.\n");

	nucleo_fd = accept(nucleo_fd, (struct sockaddr *) &addr, &addrlen);

	printf("Se ha conectado el nucleo.\n");

	//Armo paquete
	int buffer[2];
	buffer[0] = SOY_UMC;
	buffer[1] = frame_size;

	send(nucleo_fd, &buffer, 2*sizeof(int),0); //Envio codMensaje y tamaño de pagina

	recv(nucleo_fd, &msj_recibido, sizeof(int), 0);
	//Verifico que se haya conectado el nucleo
	if (msj_recibido == SOY_NUCLEO){
		printf("Se verifico la autenticidad del nucleo.\n");

		//Recibo Tamaño del stack
		recv(nucleo_fd, &stack_size,sizeof(int),0);

		//Lanzo el hilo que maneja el nucleo
		pthread_t thread;
		pthread_attr_t atributos;

		pthread_attr_init(&atributos);
		pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);

		pthread_create(&thread, &atributos, (void *)trabajarNucleo, NULL);

		//trabajarNucleo();
		return;
	}else{
		printf("No se pudo verificar la autenticidad del nucleo.\nCerrando...\n");
		close(nucleo_fd);
		exit(1);
	}
}


void trabajarNucleo(){
	printf("TrabajarNucleo\n");
	int msj_recibido;

	//Ciclo infinito
	for(;;){
		//Recibo mensajes de nucleo y hago el switch
		recv(nucleo_fd, &msj_recibido, sizeof(int), 0);

		//Chequeo que no haya una desconexion
		if(msj_recibido <= 0){
			printf("Desconexion del nucleo. Terminando...\n");
			exit(1);
		}

		switch(msj_recibido){

		case INICIALIZAR_PROGRAMA :
			inicializarPrograma();
		}
	}

}

void inicializarPrograma(){
	int pid;
	int source_size;
	int cant_recibida = 0;
	int cant_paginas_cod;
	int aux;
	char *buffer, *source;

	//Recibo los datos
	recv(nucleo_fd, &pid, sizeof(int), 0);
	recv(nucleo_fd, &cant_paginas_cod, sizeof(int), 0);
	recv(nucleo_fd, &source_size, sizeof(int), 0);

	//Recibo el archivo
	buffer = malloc(source_size);
	if(buffer == NULL){
		printf("Error al asignar memoria.\n");
		//aviso a nucleo que termine programa.
		terminarPrograma(pid);
	}

	while(cant_recibida < source_size){
		aux = recv(nucleo_fd, buffer + cant_recibida, source_size - cant_recibida, 0);

		if(aux == -1){
			printf("Error al recibir el archivo source.\n");
		}

		cant_recibida += aux;
	}
	printf("Archivo recibido satisfactoriamente.\n");
	source = buffer; //Me guardo el archivo en el puntero source

	//Creo las paginas que a las que va a poder acceder el programa(tabla de paginas)
	t_pag *paginas;

	paginas = malloc(sizeof(t_pag) * (stack_size + cant_paginas_cod) );
	for(aux=0; aux < stack_size + cant_paginas_cod; aux++){
		paginas[aux].nro_pag = aux;
		paginas[aux].presencia = false;
		paginas[aux].modificado = false;
	}

	//Creo la lista de paginas ocupadas
	int *paginas_ocupadas;

	paginas_ocupadas = malloc(sizeof(int) * fpp);
	for(aux=0; aux < fpp; aux++){
		paginas_ocupadas[aux] = -1;
	}

	t_prog *programa;

	programa = malloc(sizeof(t_prog));
	programa->pid= pid;
	programa->paginas = paginas;
	programa->cant_total_pag = stack_size + cant_paginas_cod;
	programa->timer = 0;
	programa->pag_en_memoria = paginas_ocupadas;

	pthread_mutex_lock(&mutex_listaProgramas);
	list_add(programas, programa);
	pthread_mutex_unlock(&mutex_listaProgramas);

	//Le mando a swap el archivo para que lo guarde
	if( enviarCodigoASwap(source,source_size) == -1 ){
		//Error en envio de archivo, aviso a nucleo que termine el programa.
		terminarPrograma(pid);

		free(programa);
		free(buffer);
		free(source);

		return;
	}

	//Archivo enviado exitosamente, le doy el ok a nucleo
	int msj[2];

	msj[0] = ACEPTO_PROGRAMA;
	msj[1] = pid;

	send(nucleo_fd, &msj, 2*sizeof(int),0);

	//Libero la memoria
	free(programa);
	free(buffer);
	free(source);
	return;
}

int enviarCodigoASwap(char *source, int source_size){

	int cant_enviada_total = 0;
	int cant_enviada_parcial;
	int mensaje = GUARDA_PAGINA;
	int aux;
	int pag = 0;//El codigo va siempre en las primeras n paginas.
	char *buffer;

	buffer = malloc(2*sizeof(int) + frame_size);

	if(buffer == NULL){
		printf("Error de malloc.\n");
	}

	//Armo el paquete
	memcpy(buffer,&mensaje,sizeof(int));

	while(cant_enviada_total < source_size){
		cant_enviada_parcial = 0;

		memcpy(buffer + sizeof(int),&pag,sizeof(int));
		memcpy(buffer+2*sizeof(int),source + cant_enviada_total,frame_size);

		//Ya tengo el paquete armado, lo envio
		while(cant_enviada_parcial < frame_size){
			aux = send(swap_fd,buffer,2*sizeof(int) + frame_size,0);

			if(aux <= 0){
				printf("Error en el envio del archivo");
				return -1;//Error de swap, le digo a nucleo que mande el proceso a finished
			}

			cant_enviada_parcial += aux;
		}
		//Ya envie una pagina, la cant enviada total es la anterior + una pagina
		cant_enviada_total += frame_size;

		pag++;//Para que swap sepa que es otra pagina
	}

	//Codigo enviado correctamente, devuelvo 0
	return 0;
}

void traerPaginaDeSwap(int pag, t_prog *programa){

	int cant_paginas_ocupadas = 0;
	int i;
	int pos_a_escribir;

	//Cuento la cantidad de paginas en memoria
	for(i=0; i < fpp; i++){
		if(programa->pag_en_memoria[i] != -1){
			cant_paginas_ocupadas++;
		}
	}

	/* Si la cantidad de paginas en memoria es menor a fpp, significa que aun no se cargaron
	 * todas las paginas que se pueden. Entonces la cargo directamente.*/
	if(cant_paginas_ocupadas < fpp){
		programa->pag_en_memoria[cant_paginas_ocupadas] = pag;

		pos_a_escribir = frames[programa->paginas[pag].frame].posicion;

		recibirPagina(pag, pos_a_escribir);

		return;
	}

	//Todas las paginas estan ocupadas, tengo que reemplazar una.
	for(i=0; i<fpp; i++){

		//Si no esta referenciada, sustituyo esa pagina
		if(!programa->paginas[ programa->pag_en_memoria[programa->puntero] ].referenciado){

			//Le cambio el bit de presencia
			programa->paginas[ programa->pag_en_memoria[programa->puntero] ].presencia = false;

			//Me fijo el bit de modificado
			if( programa->paginas[ programa->pag_en_memoria[programa->puntero] ].modificado ){
				//Fue modificada, la envio a swap

				int pag_a_enviar = programa->pag_en_memoria[programa->puntero];
				int pos_a_copiar = frames[programa->paginas[programa->pag_en_memoria[programa->puntero]].frame].posicion;

				enviarPagina(pag_a_enviar, programa->pid, pos_a_copiar);
			}

			//Recibo la pagina y salgo del ciclo
			pos_a_escribir = frames[programa->paginas[ programa->pag_en_memoria[programa->puntero] ].frame].posicion;

			programa->paginas[programa->pag_en_memoria[programa->puntero] ].frame = recibirPagina(pag, programa->pid);

			//Avanzo el puntero
			programa->puntero = (programa->puntero +1) & fpp;

			return;
		}

		//Pongo el bit de refencia en false
		programa->paginas[ programa->pag_en_memoria[programa->puntero]].referenciado = false;

		//incremento el puntero
		programa->puntero = (programa->puntero + 1) % fpp;
	}

	//Sali del ciclo, por lo tanto todas las paginas tenian el bit de referencia activado.
	//repito la operacion

	traerPaginaDeSwap(pag, programa);

}

void enviarPagina(int pag, int pid, int pos_a_enviar){

	int mensaje[3];
	char *buffer;

	//Armo el mensaje
	mensaje[0] = GUARDA_PAGINA;
	mensaje[1] = pid;
	mensaje[2] = pag;

	buffer = malloc(frame_size + 3*sizeof(int));

	memcpy(buffer, &mensaje, 3*sizeof(int));
	memcpy(buffer + 3*sizeof(int), memoria + pos_a_enviar, frame_size);

	//Envio la orden de guardado
	send(swap_fd, buffer, frame_size + 3*sizeof(int),0);

	//Ya envie el mensaje para que guarde la pagina, libero la memoria reservada
	free(buffer);
}

int recibirPagina(int pag, int pid){

	int mensaje[3];
	char *buffer;

	mensaje[0] = SOLICITUD_PAGINA;
	mensaje[1] = pid;
	mensaje[2] = pag;

	buffer = malloc(frame_size);

	//Envio la solicitud
	send(swap_fd,&mensaje,3*sizeof(int),0);

	//Recibo la respuesta
	recv(swap_fd,buffer, frame_size, 0);

	int frame_a_escribir = frameLibre();

	if(frame_a_escribir == -1){
		//No hay frames libres
		return -1;
	}

	int pos_a_escribir = frames[frame_a_escribir].posicion;

	memcpy(memoria + pos_a_escribir, buffer, frame_size);

	//Ya tengo la pagina en memoria
	free(buffer);
	return frame_a_escribir;
}

int frameLibre(){

	int i;
	for(i=0; i < cant_frames; i++){

		if(frames[i].libre){
			return i;
		}
	}

	//Sali del for, por lo tanto no hay frames libres
	return -1;
}

void terminarPrograma(int pid){
	//Le aviso al nucleo que termine con la ejecucion del programa
	int bufferInt[2];
	bufferInt[0] = RECHAZO_PROGRAMA;
	bufferInt[1] = pid;
	send(nucleo_fd, &bufferInt,2*sizeof(int),0);
}

int escribirEnMemoria(char* src, int pag, int offset, int size, t_prog *programa){

	int cant_escrita = 0;
	int pos_a_escribir;
	int cant_a_escribir;

	if(offset > frame_size){
		printf("Error: offset mayor que tamaño de marco.\n");
		return -1;
	}

	if(size/frame_size > fpp - pag){
		printf("No hay espacio para escribir esto en memoria.\n");//Habria que pasar cosas a swap.
		return 1;
	}

	while(cant_escrita < size){
		//Aplico el mutex para que no haya quilombo con los threads
		pthread_mutex_lock(&mutex_memoria);

		//Aplico el algoritmo clock
		algoritmoClock(programa);

		pos_a_escribir = buscarEnTlb(pag, programa->pid);

		if(pos_a_escribir != -1){
			//TLB_HIT
			pos_a_escribir++;
		}else{
			//TLB_MISS
			//Verifico que la pagina esta en memoria
			if(!programa->paginas[pag].presencia){
				//La pagina no esta en memoria, la traigo de swap
				traerPaginaDeSwap(pag,programa);
			}

			//Obtengo la posicion en memoria donde debo escribir
			pos_a_escribir = frames[programa->paginas[pag].frame].posicion;
			pos_a_escribir += offset;

			actualizarTlb(programa->pid, pag, frames[programa->paginas[pag].frame].posicion);
		}

		//Para no pasarme de la pagina y escribir en otro frame que no me pertenece
		cant_a_escribir = min(size - cant_escrita, frame_size - offset);

		memcpy(memoria + pos_a_escribir, src, cant_a_escribir);

		//Actualizo la cant_escrita
		cant_escrita += cant_a_escribir;

		//Marco la pagina como modificada
		programa->paginas[pag].modificado = true;

		//Para que en la proxima vuelta escriba la siguente pagina desde el inicio
		pag++;
		offset = 0;

		pthread_mutex_unlock(&mutex_memoria);
	}

	//Copiado satisfactoriamente
	return 0;
}

int leerEnMemoria(char *resultado, int pag, int offset, int size, t_prog *programa){

	int cant_leida = 0;
	int pos_a_leer;
	int cant_a_leer;

	if(offset > frame_size){
		printf("Error: offset mayor que tamaño de marco.\n");
		return -1;
	}

	if(size/frame_size > fpp){
		printf("La cantidad de marcos por programa no alcanza para leer esta cantidad de info.\n");
		return -1;
	}

	resultado = malloc(size);

	while(cant_leida < size){
		//Mutex
		pthread_mutex_lock(&mutex_memoria);

		algoritmoClock(programa);

		pos_a_leer = buscarEnTlb(pag, programa->pid);

		if(pos_a_leer != -1){
			//TLB_HIT pos a leer tiene la posicion a leer
			pos_a_leer += offset;
		}else{
			//TLB_MISS

			if(!programa->paginas[pag].presencia){
				//La pagina no esta en memoria, la traigo de swap
				traerPaginaDeSwap(pag, programa);
			}

			//Obtengo posicion de memoria
			pos_a_leer = frames[programa->paginas[pag].frame].posicion;
			pos_a_leer += offset;

			actualizarTlb(programa->pid, pag, frames[programa->paginas[pag].frame].posicion);
		}

		//Para no pasarme de largo y leer otros frames
		cant_a_leer = min(size - cant_leida, frame_size - offset);

		memcpy(resultado, memoria + pos_a_leer, cant_a_leer);

		cant_leida += cant_a_leer;

		//En la proxima iteracion leo la prox pagina, desde el byte 0
		pag++;
		offset = 0;

		pthread_mutex_unlock(&mutex_memoria);
	}

	return 0;
}

void trabajarCpu(int cpu_listen_fd){

	int cpu_fd;
	int cpu_num;
	cpu_fd = aceptarCpu(cpu_listen_fd, &cpu_num);

	if(cpu_fd == -1){
		return;
	}

	//Ciclo infinito para recibir mensajes
	int msj_recibido;

	for(;;){
		recv(cpu_fd, &msj_recibido,sizeof(int),0);

		if(msj_recibido <= 0){
			printf("Desconexion de la cpu");
			close(cpu_fd);
			return;
		}

		switch(msj_recibido){

			case LEER:
				leerParaCpu(cpu_fd);
				break;

			case ESCRIBIR:
				escribirParaCpu(cpu_fd);
				break;
		}
	}

}

void leerParaCpu(int cpu_fd){

	int pag, offset, size, pid;

	//recibo todos los datos
	recv(cpu_fd, &pag, sizeof(int),0);
	recv(cpu_fd, &offset, sizeof(int),0);
	recv(cpu_fd, &size, sizeof(int),0);
	recv(cpu_fd, &pid, sizeof(int),0);

	//busco el programa segun el pid
	t_prog *programa = buscarPrograma(pid);

	char *resultado;
	leerEnMemoria(resultado, pag, offset, size, programa);
	//TODO:terminar el programa si esto da -1

	//Ya tengo lo que se necesitaba leer en resultado, lo envio.
	if( sendAll(cpu_fd, resultado, size, 0) == -1){
		printf("Error en el envio de la lectura solicitada\n");
	}

	return;
}

void escribirParaCpu(int cpu_fd){

	int pag, offset, size, pid;
	char *buffer;

	recv(cpu_fd, &pag, sizeof(int),0);
	recv(cpu_fd, &offset, sizeof(int),0);
	recv(cpu_fd, &size, sizeof(int),0);
	recv(cpu_fd, &pid, sizeof(int),0);

	if( recvAll(cpu_fd, buffer, size, 0) == -1){
		printf("Error al recibir la solicitud.\n");
	}

	t_prog *programa = buscarPrograma(pid);

	if( escribirEnMemoria(buffer, pag, offset, size, programa) == -1){
		printf("Error en la escritura en memoria.\n");
	}

	//TODO: enviar la confirmacion de que se escribio ok
	return;
}

int aceptarCpu(int cpu_listen_fd, int *cpu_num){

	int buffer[2];
	int cpu_fd;
	int msj_recibido;
	struct sockaddr_in addr; // Para recibir nuevas conexiones
	socklen_t addrlen = sizeof(addr);

	cpu_fd = accept(cpu_listen_fd, (struct sockaddr *) &addr, &addrlen);

	printf("Se ha conectado una nueva cpu.\n");

	buffer[0] = SOY_UMC;
	buffer[1] = frame_size;

	//Hago el handshake
	send(cpu_fd, &buffer, 2*sizeof(int),0); //Envio codMensaje y tamaño de pagina
	recv(cpu_fd,&msj_recibido,sizeof(int),0);
	recv(cpu_fd, cpu_num,sizeof(int),0);

	if(msj_recibido == SOY_CPU){
		printf("Se ha verificado la autenticidad de la cpu n° %d.\n",*cpu_num);
		return cpu_fd;
	}else{
		printf("No se ha podido verificar la autenticidad de la cpu.\n");
		return -1;
	}
}

void inicializarTlb(){

	int cant_entradas_tlb;

	cant_entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");

	cache_tlb.entradas = malloc(cant_entradas_tlb * sizeof(t_entrada_tlb));

	cache_tlb.cant_entradas = cant_entradas_tlb;

	cache_tlb.timer = 0;

	int i;
	for(i=0; i < cant_entradas_tlb; i++){
		cache_tlb.entradas[i].nro_pag = -1;
		cache_tlb.entradas[i].pid = -1;
		cache_tlb.entradas[i].modificado = false;
	}
	return;
}

int buscarEnTlb(int pag, int pid){

	//incremento timer
	cache_tlb.timer++;

	int i;
	for(i=0; i<cache_tlb.cant_entradas; i++){
		//Me fijo que coincidan nro pag y pid
		if(cache_tlb.entradas[i].pid == pid && cache_tlb.entradas[i].nro_pag == pag){
			//Actualizo el tiempo de acceso de la entrada
			cache_tlb.entradas[i].tiempo_accedido = cache_tlb.timer;
			return cache_tlb.entradas[i].traduccion;
		}
	}

	//Sali del ciclo => hubo tlb_miss
	return -1;
}

void actualizarTlb(int pid, int pag, int traduccion){

	int i;
	//Checkeo si hubo un mal pedido
	for(i=0; i<cache_tlb.cant_entradas; i++){

		if(cache_tlb.entradas[i].pid == pid && cache_tlb.entradas[i].nro_pag == pag){
			printf("Error: Se pidio actualizar tlb, pero la pagina ya estaba.\n");
		}
	}

	//busco una entrada libre
	for(i=0; i<cache_tlb.cant_entradas; i++){

		if(cache_tlb.entradas[i].pid == -1 && cache_tlb.entradas[i].nro_pag == -1){
			//Esta entrada esta libre
			cache_tlb.entradas[i].pid = pid;
			cache_tlb.entradas[i].nro_pag = pag;
			cache_tlb.entradas[i].traduccion = traduccion;
			cache_tlb.entradas[i].tiempo_accedido = cache_tlb.timer;
			return;
		}
	}

	//Llegue aca. Significa que no hay entradas libres, hay que reemplazar una.
	reemplazarEntradaTlb(pid, pag, traduccion);
}

void reemplazarEntradaTlb(int pid, int pag, int traduccion){

	int posicion_reemplazo;
	int menor_tiempo_acceso = -1;

	int i;
	for(i=0; i<cache_tlb.cant_entradas; i++){

		if(cache_tlb.entradas[i].tiempo_accedido > menor_tiempo_acceso){
			posicion_reemplazo = i;
			menor_tiempo_acceso = cache_tlb.entradas[i].tiempo_accedido;
		}
	}

	//en posicion reemplazo ya tengo a quien tengo que reemplazar.
	cache_tlb.entradas[posicion_reemplazo].pid = pid;
	cache_tlb.entradas[posicion_reemplazo].nro_pag = pag;
	cache_tlb.entradas[posicion_reemplazo].traduccion = traduccion;
	cache_tlb.entradas[posicion_reemplazo].tiempo_accedido = cache_tlb.timer;

	if(cache_tlb.entradas[posicion_reemplazo].modificado){
		//Cambiar el modificado en tabla de paginas

		/*		bool igualPid(t_prog *elemento){
			return elemento->pid == pid;
		}

		t_prog *programa_a_modificar = list_find(programas, igualPid);

		programa_a_modificar->paginas[pag].modificado = true;*/

		t_prog *programa_a_modificar = buscarPrograma(pid);

		programa_a_modificar->paginas[pag].modificado = true;
	}
	return;
}

void flushTlb(){

	int i;

	pthread_mutex_lock(&mutex_tlb);
	for(i=0;i<cache_tlb.cant_entradas;i++){

		//Actualizo bits de modificado en tabla de paginas
		if(cache_tlb.entradas[i].modificado){
			int nro_pag = cache_tlb.entradas[i].nro_pag;
			int pid = cache_tlb.entradas[i].pid;

			t_prog *programa_modificado = buscarPrograma(pid);

			programa_modificado->paginas[nro_pag].modificado = true;
		}

		//Limpio la entrada
		cache_tlb.entradas[i].pid = -1;
	}
	pthread_mutex_unlock(&mutex_tlb);

}

void flushPid(int pid){

	int i;
	for(i=0;i<cache_tlb.cant_entradas;i++){

		//Solo reseteo aquellos que coincidan con el pid dato
		if(cache_tlb.entradas[i].pid == pid){

			//Actualizo bits de modificado en tabla de paginas
			if(cache_tlb.entradas[i].modificado){
				int nro_pag = cache_tlb.entradas[i].nro_pag;
				int pid = cache_tlb.entradas[i].pid;

				t_prog *programa_modificado = buscarPrograma(pid);

				programa_modificado->paginas[nro_pag].modificado = true;
			}

			//Limpio la entrada
			cache_tlb.entradas[i].pid = -1;
		}
	}


}

void algoritmoClock(t_prog *programa){

	programa->timer++;

	if(programa->timer < timer_reset_mem){
		int i;
		for(i=0;i<cant_frames;i++){
			programa->paginas[i].referenciado = false;
		}
	}
}

t_prog *buscarPrograma(int pid){

	bool igualPid(t_prog *elemento){
		return elemento->pid == pid;
	}

	pthread_mutex_lock(&mutex_listaProgramas);
	t_prog *programa_a_buscar = list_find(programas,(void*)igualPid);
	pthread_mutex_unlock(&mutex_listaProgramas);

	return programa_a_buscar;
}

int min(int a, int b){
	if(a>=b){
		return a;
	}else return b;
}

int sendAll(int fd, void *cosa, int size, int flags){

	int cant_enviada = 0;
	int aux;

	while(cant_enviada < size){

		aux = send(fd, cosa + cant_enviada, size - cant_enviada, flags);

		if(aux == -1){
			return -1;
		}
		cant_enviada += aux;
	}

	//Ya envie correctamente
	return 0;
}

int recvAll(int fd, char *buffer, int size, int flags){

	buffer = malloc(size);

	int cant_recibida = 0;
	int aux;

	while(cant_recibida < size){

		aux = recv(fd, buffer + cant_recibida, size - cant_recibida, flags);

		if(aux == -1){
			return -1;
		}
		cant_recibida += aux;
	}

	return 0;
}

void terminal(){

	//variables
	const int buffer_size = 20;
	char buffer[buffer_size + 1];//1 mas para el \0
	char comando[buffer_size/2];
	char parametro[buffer_size/2];

	//Vars auxiliares
	int i,j,k, pid;
	t_prog *programa;

	for(;;){
		//Limpio el buffer
		fflush(stdin);

		//obtengo los datos de stdin
		fgets(buffer, buffer_size, stdin);
		//Formateo los datos
		sscanf(buffer, "%s %s", comando, parametro);

		//El switch
		if(strcmp(comando, "flushTlb") )
		{
			if(parametro == NULL){
				//No hay segundo parametro
				flushTlb();
			}
			else{
				pid = atoi(parametro);

				flushPid(pid);
			}

			printf("flush exitoso.\n");
		}
		else if(strcmp(comando, "flushMemory") )
		{
			pthread_mutex_lock(&mutex_listaProgramas);

			for(i=0;i<list_size(programas);i++){

				programa = list_get(programas, i);

				for(j=0;j<fpp;j++){
					programa->paginas[j].modificado = true;
				}
			}

			pthread_mutex_unlock(&mutex_listaProgramas);

			printf("flush exitoso.\n");
		}
		else if(strcmp(comando, "retardo") )
		{
			if(parametro == NULL){
				printf("Mal uso de comando. Uso: retardo valor.\n");
			}
			else{
				int ret = atoi(parametro);

				retardo = ret;

				printf("Retardo cambiado exitosamente.\n");
			}
		}
		else if(strcmp(comando, "dump") )
		{
			//Primero necesito abrir el archivo
			FILE *dump_log = fopen("../resource/dump_log", "a");

			//Checkeo de errores
			if(dump_log == NULL){
				printf("Error al abrir el archivo.\n");
			}

			if(parametro == NULL){
				//No se introdujo parametro. Se imprimen todas las tablas.
				pthread_mutex_lock(&mutex_listaProgramas);
				pthread_mutex_lock(&mutex_memoria);
				for(i=0;i<list_size(programas);i++)
				{//Itero sobre todos los programas
					programa = list_get(programas, i);

					pid = programa->pid;
					printf("Impresion de las tablas de paginas del proceso pid: %d. \n", pid);

					//Para el archivo seria interesante agregarle el dia y hora del dump.
					time_t rawtime;
					struct tm *timeinfo;

					time(&rawtime);
					timeinfo = localtime(&rawtime);

					fprintf(dump_log, "Dump del dia %s. \n", asctime(timeinfo) );//asctime me pasa estas estructuras de tiempo a un string leible
					fprintf(dump_log, "Impresion de las tablas de paginas del proceso pid: %d. \n", pid);

					for(j=0;j<fpp;j++)
					{//Itero sobre cada pagina de la tabla
						printf("Pagina nro %d: ", programa->paginas[j].nro_pag);
						fprintf(dump_log, "Pagina nro %d: ", programa->paginas[j].nro_pag);

						if(frames[programa->paginas[j].frame].libre){
							printf("Pagina libre.\n");
							fprintf(dump_log, "Pagina libre.\n");
						}else{
							int pos_en_memoria = frames[programa->paginas[j].frame].posicion;
							for(k=0;k<frame_size;k++)
							{//Escribo el contenido de cada caracter en pantalla
								fputc(memoria[pos_en_memoria + k], stdout);
								fputc(memoria[pos_en_memoria + k], dump_log);
							}
						}

						printf("\n"); //Para lograr un salto de linea despues del texto sin formato.
						fprintf(dump_log, "\n");
					}
				}
				pthread_mutex_unlock(&mutex_listaProgramas);
				pthread_mutex_unlock(&mutex_memoria);
			}
			else
			{//Hay algo en parametro
				pid = atoi(parametro);

				printf("Impresion de las tablas de paginas del proceso pid: %d. \n", pid);

				pthread_mutex_lock(&mutex_memoria);

				programa = buscarPrograma(pid);

				for(j=0;j<fpp;j++)
				{//Itero sobre cada pagina de la tabla
					printf("Pagina nro %d: ", programa->paginas[j].nro_pag);

					if(frames[programa->paginas[j].frame].libre){
						printf("Pagina libre.\n");
					}else{
						int pos_en_memoria = frames[programa->paginas[j].frame].posicion;
						for(k=0;k<frame_size;k++)
						{//Escribo el contenido de cada caracter en pantalla
							fputc(memoria[pos_en_memoria + k], stdout);
						}
					}

					printf("\n"); //Para lograr un salto de linea despues del texto sin formato.
				}
			}
			pthread_mutex_unlock(&mutex_memoria);

			fclose(dump_log);
		}

	}

}
