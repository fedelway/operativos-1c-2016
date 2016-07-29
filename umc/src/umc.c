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

	//Inicio listas
	programas = list_create();

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

	lanzarTerminal();

	//Ciclo principal
	recibirConexiones(socket_CPU, max_fd);

	return EXIT_SUCCESS;
}

void lanzarTerminal()
{
	pthread_t thread_terminal;
	pthread_attr_t atributos;

	pthread_attr_init(&atributos);
	pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);

	pthread_create(&thread_terminal, &atributos, (void*)terminal, NULL);

	printf("Terminal inicializada correctamente.\n");

	return;
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

					aceptarCpu(i);

				}
			}
		}

	}

}

void crearConfiguracion(char* config_path){

	config = config_create(config_path);

	if(validarParametrosDeConfiguracion()){
		printf("El archivo de configuracion tiene todos los parametros requeridos.\n");

		//Pongo en retardo el retardo
		retardo = config_get_int_value(config, "RETARDO");
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
		&& 	config_has_property(config, "TIMER_RESET")
		&&  config_has_property(config, "ALGORITMO_REEMPLAZO");
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

	memset(memoria,'\0',cant_frames*frame_size);

	//Creo el array de frames
	frames = malloc(cant_frames * sizeof(t_frame));
	int i;
	for(i = 0; i < cant_frames; i++){
		frames[i].libre = true;
		frames[i].posicion = i * frame_size;
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
		if(recv(nucleo_fd, &msj_recibido, sizeof(int), 0) <= 0)
		{//Chequeo desconexion
			printf("Desconexion del nucleo. Terminando...\n");
			close(nucleo_fd);
			exit(1);
		}

		printf("Recibi un mensaje de nucleo.\n");

		switch(msj_recibido){

		case INICIALIZAR_PROGRAMA :
			pthread_mutex_lock(&mutex_total);
			//usleep(config_get_int_value(config,"RETARDO") * 1000);
			inicializarPrograma();
			pthread_mutex_unlock(&mutex_total);
			break;

		case FINALIZAR_PROGRAMA:
			pthread_mutex_lock(&mutex_total);
			//usleep(config_get_int_value(config,"RETARDO") * 1000);
			finalizarPrograma();
			pthread_mutex_unlock(&mutex_total);
			break;
		}
	}

}

void finalizarPrograma()
{
	//Recibo el pid
	int pid;
	recv(nucleo_fd, &pid, sizeof(int),0);

	printf("Termino la ejecucion del programa pid: %d.\nLimpiando estructuras\n",pid);

	t_prog *programa = buscarPrograma(pid);

	//pthread_mutex_lock(&mutex_listaProgramas);
	//Funcion auxiliar para remover de la lista
	bool encontrarPrograma(void *elemento){
		if( ((t_prog*)elemento)->pid == pid )
			return true;
		else return false;
	}

	programa = list_remove_by_condition(programas, (void*)encontrarPrograma);
	//pthread_mutex_unlock(&mutex_listaProgramas);

	if(programa == NULL)
		return;//No existe el pid.

	//Marco todos los frames como libres
	int i;
	for(i=0;i<programa->cant_total_pag;i++)
	{
		if(programa->paginas[i].presencia)
		{
			frames[programa->paginas[i].frame].libre = true;
		}
	}

	//Libero la memoria pedida
	free(programa->paginas);
	free(programa->pag_en_memoria);
	free(programa);

	//Aviso a swap que finalice el programa
	int buffer[2];
	buffer[0] = UMC_FINALIZAR_PROGRAMA;
	buffer[1] = pid;
	send(swap_fd, &buffer, 2*sizeof(int),0);

	return;
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
	printf("Archivo recibido satisfactoriamente.%s\n",buffer);
	source = buffer; //Me guardo el archivo en el puntero source

	//Checkeo que tenga marcos disponibles para alojar el proceso.
/*	if(list_size(programas) * fpp >= cant_frames)
	{
		printf("No alcanzan los marcos para recibir este proceso.\n");
		terminarPrograma(pid);
		free(buffer);
		return;
	}*/
	if(frameLibre() == -1)
	{
		printf("No alcanzan los marcos para recibir este proceso.\n");
		terminarPrograma(pid);
		free(buffer);
		return;
	}


	//Creo las paginas que a las que va a poder acceder el programa(tabla de paginas)
	t_pag *paginas;

	paginas = malloc(sizeof(t_pag) * (stack_size + cant_paginas_cod) );
	for(aux=0; aux < stack_size + cant_paginas_cod; aux++){
		paginas[aux].nro_pag = aux;
		paginas[aux].presencia = false;
		paginas[aux].modificado = false;
		paginas[aux].frame = -1; //No tiene cargado ningun frame.
	}

	//Creo la lista de paginas ocupadas
	int *paginas_ocupadas;

	paginas_ocupadas = malloc(sizeof(int) * fpp);
	for(aux=0; aux < fpp; aux++){
		paginas_ocupadas[aux] = -1;
	}

	t_prog programa2 = { .pid=pid,.paginas=paginas,.cant_total_pag = stack_size + cant_paginas_cod,.timer=0,.pag_en_memoria=paginas_ocupadas,.puntero=0};

	t_prog *programa;

	programa = malloc(sizeof(t_prog));

	memcpy(programa,&programa2,sizeof(t_prog));

	/*programa->pid= pid;
	programa->paginas = paginas;
	programa->cant_total_pag = stack_size + cant_paginas_cod;
	programa->timer = 0;
	programa->pag_en_memoria = paginas_ocupadas;
	programa->puntero = 0;*/

	printf("cosas programa:\npid %d cantpaginas %d timer %d.\n",programa->pid,programa->cant_total_pag,programa->timer);

	//pthread_mutex_lock(&mutex_listaProgramas);
	list_add(programas, programa);
	//pthread_mutex_unlock(&mutex_listaProgramas);

	//Le mando a swap el archivo para que lo guarde
	if( enviarCodigoASwap(source,source_size, pid) == -1 ){
		//Error en envio de archivo, aviso a nucleo que termine el programa.
		terminarPrograma(pid);

		free(programa);
		free(buffer);

		return;
	}

	int msj[2];
	msj[0] = ACEPTO_PROGRAMA;
	msj[1] = pid;

	printf("%d %d.\n",msj[0],msj[1]);

	send(nucleo_fd, msj, 2*sizeof(int), 0);

	//Libero la memoria
	//free(programa);
	free(buffer);
	return;
}

int enviarCodigoASwap(char *source, int source_size, int pid){

	int mensaje[3];

	int cant_paginas_cod = (source_size / frame_size) + 1;
	if(source_size%frame_size == 0)
		cant_paginas_cod--;

	int cant_paginas = cant_paginas_cod + stack_size;

	printf("cant_paginas: %d pag_cod: %d, pag_stack: %d", cant_paginas,cant_paginas_cod,stack_size);

	mensaje[0] = RESERVA_ESPACIO;
	mensaje[1] = pid;
	mensaje[2] = cant_paginas;

	send(swap_fd,&mensaje,3*sizeof(int),0);

	//Espero el ok de swap que pudo reservar el espacio para el programa.
	int respuesta;
	recv(swap_fd,&respuesta,sizeof(int),0);

	if(respuesta == SWAP_PROGRAMA_RECHAZADO)
	{
		//Swap no tiene espacio para guardar esto en memoria
		return -1;
	}else if(respuesta == SWAP_PROGRAMA_OK)
	{//Envio el programa pagina a pagina

		//Armo el buffer
		char *buffer = malloc(frame_size + 3*sizeof(int) );

		mensaje[0] = GUARDA_PAGINA;
		mensaje[1] = pid;

		//Esto es igual en todos los mensajes
		memcpy(buffer, &mensaje, 2*sizeof(int));

		printf("source size: %d, frame size: %d.\n", source_size,frame_size);

		int cant_enviada = 0;
		int cant_a_copiar;
		mensaje[2] = 0;

		int cont = 0;
		int i;
		for(i=0;i<cant_paginas_cod;i++)
		{
			//Copio la nueva pagina
			memcpy(buffer + 2*sizeof(int), &mensaje[2], sizeof(int));

			cont++;
			cant_a_copiar = min(source_size - cant_enviada, frame_size);
			printf("Cant faltante: %d, frame_size:%d .\n",source_size-cant_enviada,frame_size);
			printf("Cant a copiar: %d.\n\n",cant_a_copiar);

			memcpy(buffer + 3*sizeof(int), source + i*frame_size, cant_a_copiar);

			if(cant_a_copiar < frame_size)
			{
				memset(buffer + 3*sizeof(int) + cant_a_copiar,'\0',frame_size - cant_a_copiar);
			}

			printf("Impresion del buffer:\n");
			printf("Guarda_pagina:%d, pid:%d, pagina:%d.\n\n",*buffer,*(buffer + sizeof(int)),*(buffer+2*sizeof(int)));
			fwrite(buffer + 3*sizeof(int), sizeof(char), cant_a_copiar, stdout);
			printf("\n\n");


			if( sendAll(swap_fd,buffer,frame_size+3*sizeof(int),0) <= 0)
			{
				perror("Error en el envio del codigo a swap");
				return -1;
			}

			cant_enviada += cant_a_copiar;
			mensaje[2] = mensaje[2] +1;
		}
		/*while(cant_enviada < source_size)
		{
			cont++;
			//Termino de armar el buffer
			cant_a_copiar = min(source_size - cant_enviada, frame_size);//Hago esto para no pasarme con lo que copio
			memcpy(buffer + 3*sizeof(int), source + cant_enviada, cant_a_copiar);

			//Si lo que tengo que escribir es menor a la pagina, setteo el resto de la memoria con \0
			if(cant_a_copiar < frame_size)
			{
				memset(buffer + 3*sizeof(int),'\0',frame_size - cant_a_copiar);
			}

			printf("Impresion del buffer:\n");
			fwrite(buffer,sizeof(int),3,stdout);
			fwrite(buffer + 3*sizeof(int), sizeof(char), cant_a_copiar, stdout);
			printf("\n");

			if (sendAll(swap_fd,buffer,frame_size + 3*sizeof(int),0) == -1)
			{
				perror("Error en el envio del codigo a swap");
				return -1;
			}

			cant_enviada += cant_a_copiar;
			mensaje[2]++;
		}*/

		printf("Programa enviado a swap exitosamente.%d\n",cont);
		return 0;
	}else{
		printf("Quilombo con los recv, recibi un mensaje de otro thread...\n");
		return -1;
	}

}

int traerPaginaDeSwap(int pag, t_prog *programa){

	//Funciones auxiliares
//	t_pag pag_apuntada()
//	{
//		return programa->paginas[programa->pag_en_memoria[programa->puntero]];
//	}

	printf("Traer pagina de swap.\n");

	void avanzarPuntero()
	{
		programa->puntero = (programa->puntero + 1) % fpp;
	}

	int cant_paginas_ocupadas = 0;
	int i;

	//Cuento la cantidad de paginas en memoria
	for(i=0; i < fpp; i++){
		if(programa->pag_en_memoria[i] != -1){
			cant_paginas_ocupadas++;
		}
	}

	/* Si la cantidad de paginas en memoria es menor a fpp, significa que aun no se cargaron
	 * todas las paginas que se pueden. Entonces la cargo directamente.*/
	if(cant_paginas_ocupadas < fpp){
		printf("Cargo directamente.\n");

		if(frameLibre() == -1){
			return reemplazarDirectamente(pag,programa);
		}
		else
		{
			programa->pag_en_memoria[cant_paginas_ocupadas] = pag;

			programa->paginas[pag].frame = recibirPagina(pag, programa->pid);
			programa->paginas[pag].presencia = true;
			programa->paginas[pag].modificado = false;

			return 0;
		}
	}

	printf("Aplico algoritmo de reemplazo de paginas: ");

	//Todas las paginas estan ocupadas, tengo que reemplazar una. Aplico el algoritmo clock
	if(!strcmp(config_get_string_value(config,"ALGORITMO_REEMPLAZO"), "CLOCK-M") )
	{//CLOCK MODIFICADO
		printf("CLOCK MODIFICADO.\n");
		for(i=0; i<fpp; i++){

			if(!pag_apuntada.referenciado)
			{
				if(!pag_apuntada.modificado)
				{//No fue referenciada, ni modificada, se sustituye
					printf("Reemplazo la pagina nro: %d.\n",programa->pag_en_memoria[programa->puntero]);

					//pongo el bit de presencia en falso y libero el frame
					pag_apuntada.presencia = false;
					frames[pag_apuntada.frame].libre = true;
					eliminarEntradaTlb(programa->pag_en_memoria[programa->puntero],programa->pid);

					//Actualizo pag en memoria
					programa->pag_en_memoria[programa->puntero] = pag;
					pag_apuntada.presencia = true;
					pag_apuntada.modificado = false;

					//recibo la pagina
					pag_apuntada.frame = recibirPagina(pag, programa->pid);

					//avanzo el puntero y salgo del ciclo
					avanzarPuntero();
					return 0;
				}
			}
		}
		//Sali del ciclo, por lo tanto todas las paginas estaban referenciadas o modificadas
		//Ahora busco paginas que esten con bit de referencia en falso, pero si modificadas
		for(i=0;i<fpp;i++)
		{
			if(!pag_apuntada.referenciado)
			{
				if(pag_apuntada.modificado)
				{//Encontre una pagina que no fue referencia, la reemplazo
					printf("Reemplazo la pagina nro: %d.\n",programa->pag_en_memoria[programa->puntero]);

					//Presencia en falso y libero el frame
					pag_apuntada.presencia = false;
					frames[pag_apuntada.frame].libre = true;
					eliminarEntradaTlb(programa->pag_en_memoria[programa->puntero],programa->pid);

					//Como fue modificada, la envio a swap
					int pag_a_enviar = programa->pag_en_memoria[programa->puntero];
					int pos_a_enviar = frames[pag_apuntada.frame].posicion;

					enviarPagina(pag_a_enviar, programa->pid, pos_a_enviar);

					//Actualizo la lista de pag en memoria
					programa->pag_en_memoria[programa->puntero] = pag;
					pag_apuntada.presencia = true;
					pag_apuntada.modificado = false;

					//Recibo la pagina
					pag_apuntada.frame = recibirPagina(pag, programa->pid);

					//Avanzo el puntero y salgo
					avanzarPuntero();
					return 0;
				}
			}else{//Pagina referenciada, cambio el bit a false
				pag_apuntada.referenciado = false;
				avanzarPuntero();
			}
		}

	}else if(!strcmp(config_get_string_value(config,"ALGORITMO_REEMPLAZO"),"CLOCK") )
	{//CLOCK COMUN
		printf("CLOCK.\n");
		for(i=0;i<fpp;i++)
		{
			if(!pag_apuntada.referenciado)
			{//No esta refenciada, por lo tanto reemplazo esta pagina
				printf("Reemplazo la pagina nro: %d.\n",programa->pag_en_memoria[programa->puntero]);

				pag_apuntada.presencia = false;
				frames[pag_apuntada.frame].libre = true;
				eliminarEntradaTlb(programa->pag_en_memoria[programa->puntero],programa->pid);

				if(pag_apuntada.modificado)
				{//Si fue modificada debo enviarla a swap
					printf("La pagina fue modificada.\n");
					int pag_a_enviar = pag_apuntada.nro_pag;
					int pos_a_copiar = frames[pag_apuntada.frame].posicion;

					enviarPagina(pag_a_enviar, programa->pid, pos_a_copiar);
				}

				//Actualizo pag en memoria
				programa->pag_en_memoria[programa->puntero] = pag;
				pag_apuntada.presencia = true;
				pag_apuntada.modificado = false;

				//Recibo la pagina aca
				pag_apuntada.frame = recibirPagina(pag,programa->pid);

				printf("Retorno.\n");
				return 0;
			}else
			{
				pag_apuntada.referenciado = false;
			}

			avanzarPuntero();
		}
	}

	//Sali del ciclo, por lo tanto todas las paginas tenian el bit de referencia activado.
	//repito la operacion
	printf("Debo repetir el algoritmo.\n");
	return traerPaginaDeSwap(pag, programa);

}

int reemplazarDirectamente(int pag, t_prog *programa)
{
	//Como no tengo paginas suficientes, reemplazo la primera pagina que este en memoria.

	printf("No hay paginas suficientes...Reemplazo directamente.\n");

	int i;
	for(i=0;i<programa->cant_total_pag;i++)
	{
		if(programa->paginas[i].presencia)
		{//Si no fue referenciada y esta en memoria, reemplazo.
			printf("Reemplazo Pag nro: %d presencia: %d\n",i,programa->paginas[i].presencia);

			programa->paginas[i].presencia = false;
			programa->paginas[i].referenciado = false;
			frames[programa->paginas[i].frame].libre = true;
			eliminarEntradaTlb(i,programa->pid);

			//Si la pagina fue modificada debo enviarla a swap
			if(programa->paginas[i].modificado)
				enviarPagina(i,programa->pid,frames[programa->paginas[i].frame].posicion);

			//Recibo la pagina
			programa->paginas[pag].frame = recibirPagina(pag, programa->pid);//Esta vez no deberia fallar
			programa->paginas[pag].presencia = true;
			programa->paginas[pag].modificado = false;

			return 0;
		}
	}

	if(frameLibre() == -1)
	{
		printf("No quedan marcos disponibles. Debo terminar la ejecucion.\n");
		return -1;
	}

	printf("No deberia haber pasado por aca....\n");
	return -1;
}

void enviarPagina(int pag, int pid, int pos_a_enviar){

	printf("Envio pagina a swap\npid: %d, pag:%d.\n",pid,pag);
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

	//El frame ya no esta libre
	frames[frame_a_escribir].libre = false;

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
	printf("No hay frames disponibles.\n");
	return -1;
}

void terminarPrograma(int pid){
	//Le aviso al nucleo que termine con la ejecucion del programa
	printf("Terminando la ejecucion del programa pid: %d.\n",pid);
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

	while(cant_escrita < size){
		//Aplico el mutex para que no haya quilombo con los threads
		//pthread_mutex_lock(&mutex_memoria);

		//Aplico el algoritmo clock
		algoritmoClock(programa);

		pos_a_escribir = buscarEnTlb(pag, programa->pid);

		if(pos_a_escribir != -1){
			printf("TLB_HIT.\n");
			pos_a_escribir+=offset;
		}else{
			printf("TLB_MISS.\n");
			usleep(retardo * 1000);

			//Verifico que la pagina esta en memoria
			if(!programa->paginas[pag].presencia){
				//La pagina no esta en memoria, la traigo de swap
				if( traerPaginaDeSwap(pag,programa) ){
					//Debo finalizar la ejecucion.
					return -1;
				}
			}

			//Obtengo la posicion en memoria donde debo escribir
			pos_a_escribir = frames[programa->paginas[pag].frame].posicion;
			pos_a_escribir += offset;

			actualizarTlb(programa->pid, pag, frames[programa->paginas[pag].frame].posicion);
		}

		//Para no pasarme de la pagina y escribir en otro frame que no me pertenece
		cant_a_escribir = min(size - cant_escrita, frame_size - offset);

		usleep(retardo * 1000);
		memcpy(memoria + pos_a_escribir, src, cant_a_escribir);

		//Actualizo la cant_escrita
		cant_escrita += cant_a_escribir;

		//Marco la pagina como modificada
		programa->paginas[pag].modificado = true;

		//Para que en la proxima vuelta escriba la siguente pagina desde el inicio
		pag++;
		offset = 0;

		//pthread_mutex_unlock(&mutex_memoria);
	}

	//Copiado satisfactoriamente
	return 0;
}

int leerEnMemoria(char *resultado, int pag, int offset, int size, t_prog *programa){

	printf("Leer en memoria.\n");

	int cant_leida = 0;
	int pos_a_leer;
	int cant_a_leer;

	if(offset > frame_size){
		printf("Error: offset mayor que tamaño de marco.\n");
		return -1;
	}

/*	if(size/frame_size > fpp){
		printf("La cantidad de marcos por programa no alcanza para leer esta cantidad de info.\n");
		return -1;
	}*/

	while(cant_leida < size){
		//Mutex
		//pthread_mutex_lock(&mutex_memoria);

		algoritmoClock(programa);

		pos_a_leer = buscarEnTlb(pag, programa->pid);

		if(pos_a_leer != -1){
			//TLB_HIT pos a leer tiene la posicion a leer
			printf("TLB_HIT.\n");
			pos_a_leer += offset;
		}else{
			//TLB_MISS
			printf("TLB_MISS.\n");
			usleep(retardo * 1000);

			if(!programa->paginas[pag].presencia){
				//La pagina no esta en memoria, la traigo de swap
				if( traerPaginaDeSwap(pag, programa) ){
					return -1;
				}
			}

			//Obtengo posicion de memoria
			pos_a_leer = frames[programa->paginas[pag].frame].posicion;
			pos_a_leer += offset;

			printf("offset: %d.\n", offset);
			printf("posicion frame: %d, pos_a_leer: %d.\n", frames[programa->paginas[pag].frame].posicion,pos_a_leer);
			printf("Nro frame: %d, presencia: %d.\n",programa->paginas[pag].frame,programa->paginas[pag].presencia);

			actualizarTlb(programa->pid,pag,frames[programa->paginas[pag].frame].posicion);
		}

		//Para no pasarme de largo y leer otros frames
		cant_a_leer = min(size - cant_leida, frame_size - offset);

		usleep(retardo * 1000);
		memcpy(resultado + cant_leida, memoria + pos_a_leer, cant_a_leer);

		//Actualizo el bit de referencia
		programa->paginas[pag].referenciado = true;

		cant_leida += cant_a_leer;

		//En la proxima iteracion leo la prox pagina, desde el byte 0
		pag++;
		offset = 0;

		//pthread_mutex_unlock(&mutex_memoria);
	}

	return 0;
}

void trabajarCpu(int cpu_fd){

	printf("trabajarCpu.\n");
	int cpu_num;

	//Recibo el numero de cpu
	recv(cpu_fd,&cpu_num,sizeof(int),0);
	printf("Cpu n°: %d.\n",cpu_num);

	//Ciclo infinito para recibir mensajes
	int msj_recibido;

	for(;;){

		if(recv(cpu_fd, &msj_recibido,sizeof(int),0) <= 0)
		{
			printf("Desconexion de la cpu.\n");
			close(cpu_fd);
			return;
		}

		switch(msj_recibido)
		{
			case LEER:
				pthread_mutex_lock(&mutex_total);
				//usleep(config_get_int_value(config,"RETARDO") * 1000);
				leerParaCpu(cpu_fd);
				pthread_mutex_unlock(&mutex_total);
				break;

			case ESCRIBIR:
				pthread_mutex_lock(&mutex_total);
				//usleep(config_get_int_value(config,"RETARDO") * 1000);
				escribirParaCpu(cpu_fd);
				pthread_mutex_unlock(&mutex_total);
				break;

			case DESCONEXION_CPU:
				printf("Desconexion de una cpu.\n");
				close(cpu_fd);
				return; //Con el return finalizo el thread
		}
	}

}

void leerParaCpu(int cpu_fd){

	printf("leer cpu.\n");

	int pag, offset, size, pid;

	//recibo todos los datos
	recv(cpu_fd, &pag, sizeof(int),0);
	recv(cpu_fd, &offset, sizeof(int),0);
	recv(cpu_fd, &size, sizeof(int),0);
	recv(cpu_fd, &pid, sizeof(int),0);

	//busco el programa segun el pid
	t_prog *programa = buscarPrograma(pid);

	if(programa == NULL)
		printf("No hay programa.\n");

	printf("Solicitud de lectura:\npag: %d, offset: %d, size: %d, pid:%d.\ncant pag: %d\n",pag,offset,size,pid,programa->cant_total_pag);

	//Me fijo si hay stack overflow
	int mensaje;
	if(pag >= programa->cant_total_pag || pag < 0)
	{
		printf("STACK OVERFLOW. Cant pag: %d, pag solicitada: %d\n",programa->cant_total_pag,pag);
		mensaje = OVERFLOW;
		send(cpu_fd,&mensaje,sizeof(int),0);

		return;
	}

	char *resultado = malloc(size);
	if( leerEnMemoria(resultado, pag, offset, size, programa) == -1 )
	{
		printf("\n-----------------------\nERROR AL LEER\n-----------------------\n\nDebo finalizar.\n");
		int mensaje = OVERFLOW;
		send(cpu_fd,&mensaje,sizeof(int),0);
		return;
	}

	mensaje = PEDIDO_OK;
	send(cpu_fd,&mensaje,sizeof(int),0);

	printf("Resultado:\n\n");
	fwrite(resultado,sizeof(char),size,stdout);
	printf("\n\n");

	//Ya tengo lo que se necesitaba leer en resultado, lo envio.
	if( sendAll(cpu_fd, resultado, size, 0) == -1){
		perror("Error en el envio de la lectura solicitada");
	}

	free(resultado);

	return;
}

void escribirParaCpu(int cpu_fd){

	printf("Escribir para cpu.\n");

	int pag, offset, size, pid;
	char *buffer;

	recv(cpu_fd, &pag, sizeof(int),0);
	recv(cpu_fd, &offset, sizeof(int),0);
	recv(cpu_fd, &size, sizeof(int),0);
	recv(cpu_fd, &pid, sizeof(int),0);

	buffer = malloc(size);

	if( recvAll(cpu_fd, buffer, size, 0) == -1){
		printf("Error al recibir la solicitud.\n");
	}

	printf("contenido a escribir:\n\n");
	fwrite(buffer,sizeof(char),size,stdout);
	printf("\n\n");

	t_prog *programa = buscarPrograma(pid);

	//Me fijo si hay stack overflow
	int mensaje;
	if(pag >= programa->cant_total_pag || pag < 0)
	{
		printf("STACK OVERFLOW. Cant pag: %d, pag solicitada: %d\n",programa->cant_total_pag,pag);
		mensaje = OVERFLOW;
		send(cpu_fd,&mensaje,sizeof(int),0);

		return;
	}

	if( escribirEnMemoria(buffer, pag, offset, size, programa) == -1){
		printf("\n-----------------------\nERROR AL ESCRIBIR\n-----------------------\n\nDebo finalizar.\n");
		int mensaje = RECHAZO_PROGRAMA;
		send(cpu_fd,&mensaje,sizeof(int),0);
		return;
	}

	mensaje = PEDIDO_OK;
	send(cpu_fd,&mensaje,sizeof(int),0);

	free(buffer);

	return;
}

void aceptarCpu(int cpu_listen_fd)
{
	int buffer[2];
	int cpu_fd;
	int msj_recibido;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	cpu_fd = accept(cpu_listen_fd, (struct sockaddr *) &addr, &addrlen);

	printf("Se ha conectado una nueva cpu.\n");

	buffer[0] = SOY_UMC;
	buffer[1] = frame_size;

	//Hago el handshake
	send(cpu_fd, &buffer, 2*sizeof(int),0); //Envio codMensaje y tamaño de pagina
	recv(cpu_fd,&msj_recibido,sizeof(int),0);

	if(msj_recibido == SOY_CPU){
		printf("Se ha verificado la autenticidad de una cpu.\n");

		//Lanzo el thread
		pthread_t thread;
		pthread_attr_t atributos;

		pthread_attr_init(&atributos);
		pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);

		pthread_create(&thread, &atributos, (void *)trabajarCpu, (void*)cpu_fd);

	}else{
		printf("No se ha podido verificar la autenticidad de la cpu.\n");
		return;
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

	if(cache_tlb.cant_entradas == 0)
		return -1;

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

	if(cache_tlb.cant_entradas == 0)
		return;

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

			printf("Tlb actualizada:\npid:%d, pag: %d, traduccion: %d.\n",pid,pag,traduccion);
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

	printf("Tlb actualizada:\npid:%d, pag: %d, traduccion: %d.\n",pid,pag,traduccion);
	return;
}

void eliminarEntradaTlb(int pag, int pid)
{
	int i;
	for(i=0; i<cache_tlb.cant_entradas; i++)
	{
		if(cache_tlb.entradas[i].nro_pag == pag && cache_tlb.entradas[i].pid == pid)
		{
			cache_tlb.entradas[i].nro_pag = -1;
			cache_tlb.entradas[i].pid = -1;
		}
	}
}

void flushTlb(){

	int i;

	//pthread_mutex_lock(&mutex_tlb);
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
	//pthread_mutex_unlock(&mutex_tlb);

}

void flushPid(int pid){

	int i;
	//pthread_mutex_lock(&mutex_tlb);
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
	//pthread_mutex_unlock(&mutex_tlb);

}

void algoritmoClock(t_prog *programa){

	programa->timer++;

	if(programa->timer > config_get_int_value(config,"TIMER_RESET")){
		printf("CLOCK: TIMER RESET.\n");
		int i;
		for(i=0; i<programa->cant_total_pag ;i++){
			programa->paginas[i].referenciado = false;
		}

		programa->timer = 0;
	}
}

t_prog *buscarPrograma(int pid){

	bool igualPid(void *elemento){
		return ((t_prog*)elemento)->pid == pid;
	}

	//pthread_mutex_lock(&mutex_listaProgramas);
	t_prog *programa_a_buscar = list_find(programas,igualPid);
	//pthread_mutex_unlock(&mutex_listaProgramas);

	return programa_a_buscar;
}

int min(int a, int b){
	if(a<=b){
		return a;
	}else return b;
}

#define programa(i) ((t_prog*)list_get(programas,i))
void terminal(){

	//variables
	const int buffer_size = 20;
	char buffer[buffer_size + 1];//1 mas para el \0
	char comando[buffer_size/2];
	char parametro[buffer_size/2];
	int cant_parametros;

	//Vars auxiliares
	int i,j,k, pid;
	t_prog *programa;

	for(;;){
		//Limpio los buffers
		fflush(stdin);
		//while ((i = getchar()) != '\n' && c != EOF);
		memset(buffer,'\0',buffer_size);
		memset(comando,'\0',buffer_size/2);
		memset(parametro,'\0',buffer_size/2);

		//obtengo los datos de stdin
		fgets(buffer, buffer_size, stdin);
		//Formateo los datos
		cant_parametros = sscanf(buffer, "%s %s", comando, parametro);

		pthread_mutex_lock(&mutex_total);
		//El switch
		if(!strcmp(comando, "flushTlb") )
		{
			if(cant_parametros == 1){
				//No hay segundo parametro
				flushTlb();

				printf("flush exitoso.\n");
			}
			else{
				pid = atoi(parametro);

				if(pidValido(pid) ){
					flushPid(pid);

					printf("flush exitoso.\n");
				}else{
					printf("pid incorrecto.\n");
				}
			}

		}
		else if(!strcmp(comando, "flushMemory") )
		{
			printf("Iniciando flush.\n");

			//pthread_mutex_lock(&mutex_listaProgramas);

			for(i=0;i<list_size(programas);i++){

				programa = list_get(programas, i);

				for(j=0;j<programa->cant_total_pag;j++){
					programa->paginas[j].modificado = true;
				}
			}

			//pthread_mutex_unlock(&mutex_listaProgramas);

			printf("flush exitoso.\n");
		}
		else if(!strcmp(comando, "retardo") )
		{
			if(cant_parametros == 1){
				printf("Mal uso de comando. Uso: retardo valor.\n");
			}
			else{
				int ret = atoi(parametro);

				retardo = ret;

				printf("\nRetardo cambiado exitosamente.\n");
			}
		}
		else if(!strcmp(comando, "dump") )
		{
			printf("\nComenzando con el dump.\n");

			//Primero necesito abrir el archivo
			FILE *dump_log = fopen("../resource/dump_log", "a");

			//Checkeo de errores
			if(dump_log == NULL){
				printf("Error al abrir el archivo.\n");
			}

			//Para el archivo seria interesante agregarle el dia y hora del dump.
			time_t rawtime;
			struct tm *timeinfo;

			time(&rawtime);
			timeinfo = localtime(&rawtime);

			fprintf(dump_log, "Dump del dia %s. \n", asctime(timeinfo) );//asctime me pasa estas estructuras de tiempo a un string leible

			if(cant_parametros == 1){
				//No se introdujo parametro. Se imprimen todas las tablas.
				//pthread_mutex_lock(&mutex_listaProgramas);
				//pthread_mutex_lock(&mutex_memoria);

				for(i=0;i<list_size(programas);i++)
				{//Itero sobre todos los programas
					programa = list_get(programas, i);

					pid = programa->pid;
					printf("Impresion de las tablas de paginas del proceso pid: %d. \n", pid);

					fprintf(dump_log, "Impresion de las tablas de paginas del proceso pid: %d. \n", pid);

					printf("fpp: %d cant_total_pag: %d.\n",fpp,programa->cant_total_pag);

					for(j=0;j<programa->cant_total_pag;j++)
					{
						printf("Pagina nro: %d presencia: %d modificado: %d\nreferenciado: %d frame: %d.\n",
								j,programa->paginas[j].presencia,programa->paginas[j].modificado,
								programa->paginas[j].referenciado, programa->paginas[j].frame);

						fprintf(dump_log,"Pagina nro: %d presencia: %d modificado: %d\nreferenciado: %d frame: %d.\n",
								j,programa->paginas[j].presencia,programa->paginas[j].modificado,
								programa->paginas[j].referenciado, programa->paginas[j].frame);
					}

				}
				//pthread_mutex_unlock(&mutex_listaProgramas);
				//pthread_mutex_unlock(&mutex_memoria);
			}
			else
			{//Hay algo en parametro
				pid = atoi(parametro);

				if(pidValido(pid))
				{
					printf("Impresion de las tablas de paginas del proceso pid: %d. \n", pid);

					//pthread_mutex_lock(&mutex_memoria);

					programa = buscarPrograma(pid);

					for(j=0;j<programa->cant_total_pag;j++)
					{
						printf("Pagina nro: %d presencia: %d modificado: %d\nreferenciado: %d frame: %d.\n\n",
								j,programa->paginas[j].presencia,programa->paginas[j].modificado,
								programa->paginas[j].referenciado, programa->paginas[j].frame);

						fprintf(dump_log,"Pagina nro: %d presencia: %d modificado: %d\nreferenciado: %d frame: %d.\n\n",
								j,programa->paginas[j].presencia,programa->paginas[j].modificado,
								programa->paginas[j].referenciado, programa->paginas[j].frame);
					}
				}else{
					printf("Pid invalido.\n");
				}
			}

			printf("\nDump finalizado exitosamente.\n");

			fclose(dump_log);
		}
		else if(!strcmp(comando, "actualizarSwap"))
		{//Envio todas las paginas que fueron modificadas a swap
			int i;
			for(i=0;i<list_size(programas);i++)
			{
				int j;
				for(j=0;j<programa(i)->cant_total_pag;j++)
				{
					if(programa(i)->paginas[j].modificado)
					{
						enviarPagina(j,programa(i)->pid,frames[programa(i)->paginas[j].frame].posicion);
					}
				}
			}

			printf("Actualizacion realizada con exito.\n");
		}
		else if(!strcmp(comando, "dumpFrames"))
		{//Imprime todos los frames
			//Primero necesito abrir el archivo
			FILE *dump_log = fopen("../resource/dump_log", "a");

			//Checkeo de errores
			if(dump_log == NULL){
				printf("Error al abrir el archivo.\n");
			}

			//Para el archivo seria interesante agregarle el dia y hora del dump.
			time_t rawtime;
			struct tm *timeinfo;

			time(&rawtime);
			timeinfo = localtime(&rawtime);

			fprintf(dump_log, "Dump del dia %s. \n", asctime(timeinfo) );//asctime me pasa estas estructuras de tiempo a un string leible

			printf("\nIniciando dump de frames\n\n");
			int i;
			for(i=0;i<cant_frames;i++)
			{
				printf("Frame nro: %d",i);
				fprintf(dump_log,"Frame nro: %d",i);
				if(frames[i].libre){
					printf("libre.\n");
					fprintf(dump_log,"libre.\n");
				}
				else
				{
					printf("\n");
					fwrite(memoria + frames[i].posicion,1,frame_size,stdout);
					printf("\n\n");

					fprintf(dump_log,"\n");
					fwrite(memoria + frames[i].posicion,1,frame_size,dump_log);
					fprintf(dump_log,"\n\n");
				}
			}

			fclose(dump_log);
		}
		else if(!strcmp(comando, "exit") )
		{
			printf("Terminando ejecucion del sistema...\n");

			exit(0);
		}
		else
		{
			printf("\nComando erroneo.\nComandos disponibles: flushTlb [pid]\nflushMemory\ndump [pid]\ndumpFrames\nretardo [ret en milisegundos]\nexit\n");
		}

		pthread_mutex_unlock(&mutex_total);
	}

}

bool pidValido(int pid)
{
	int i;

	for(i=0;i<cache_tlb.cant_entradas;i++)
	{
		if(cache_tlb.entradas[i].pid == pid)
			return true;
	}

	return false;
}
