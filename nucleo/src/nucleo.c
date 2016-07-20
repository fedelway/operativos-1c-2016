/*
 ============================================================================
 Name        : nucleo.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "nucleo.h"

#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar


int main(int argc, char *argv[]) {

	char *puerto_cons, *puerto_cpu;
	char *ip_UMC, *puerto_UMC;
	fd_set listen;

	ready = queue_create();
	blocked = queue_create();
	finished = queue_create();
	new = list_create();
	SEM = list_create();
	IO = list_create();
	SHARED = list_create();

	listaCpu = list_create();
	listaConsola = list_create();
	//listaCPUs = list_create();
	//listaConsolas = list_create();
	//listaListos = list_create();
	//listaBloqueados = list_create();
	//listaEjecutar = list_create();
	//listaFinalizados = list_create();


	//valida los parametros de entrada del main
	if (argc != 2) {
		fprintf(stderr, "Uso: nucleo config_path\n");
		return 1;
	}

	FD_ZERO(&listen);

	crearConfiguracion(argv[1]);

	//Conexion consolas
	cant_consolas = 0;
	puerto_cons = config_get_string_value(config, "PUERTO_PROG");
	cons_fd = conectarPuertoDeEscucha(puerto_cons);
	maximoFileDescriptor(cons_fd,&max_fd);
	FD_SET(cons_fd, &listen);
	printf("socket escucha consola: %d \n", cons_fd);
	printf("Esperando conexiones de consolas.. (PUERTO: %s)\n", puerto_cons);


	stack_size = config_get_int_value(config, "STACK_SIZE");
	quantum = config_get_int_value(config, "QUANTUM");


	//Conexion CPUs
	cant_cpus = 0;
	max_cpu = 0;
	puerto_cpu = config_get_string_value(config, "PUERTO_CPU");
	cpu_fd = conectarPuertoDeEscucha(puerto_cpu);
	maximoFileDescriptor(cpu_fd,&max_fd);
	FD_SET(cpu_fd, &listen);
	printf("socket escucha cpu: %d \n", cpu_fd);
	printf("Esperando conexiones de CPUs.. (PUERTO: %s)\n", puerto_cpu);


	//Conexion UMC
	puerto_UMC = config_get_string_value(config, "PUERTO_UMC");
	ip_UMC = config_get_string_value(config, "IP_UMC");

	socket_umc = conectarseA(ip_UMC, puerto_UMC);
	validacionUMC(socket_umc);

	//escucha actividad de sockets - nuevas CPUs, consolas, recepción msjs -
 	trabajarConexionesSockets(&listen, &max_fd, cpu_fd, cons_fd);

	return EXIT_SUCCESS;
}


void maximoFileDescriptor(int socket_escucha,int *max_fd){

	if (socket_escucha > *max_fd) {
		*max_fd = socket_escucha;
	}
}

void crearConfiguracion(char *config_path) {

	config = config_create(config_path);

	if (validarParametrosDeConfiguracion()) {
		printf("Tiene todos los parametros necesarios\n");
	} else {
		printf("Configuracion no valida\n");
		exit(EXIT_FAILURE);
	}

	//Creo los semaforos y los dispositivos IO
	crearSemaforos();
	crearIO();
	crearShared();

	inotify_fd = inotify_init();
	create_wd = inotify_add_watch(inotify_fd,config_path,IN_CREATE);
	delete_wd = inotify_add_watch(inotify_fd,config_path,IN_DELETE);
	modify_wd = inotify_add_watch(inotify_fd,config_path,IN_MODIFY);

	global_path = config_path;
}

void cambiarConfig()
{
	struct inotify_event* evento = malloc(sizeof(struct inotify_event));

	if( read(inotify_fd, evento, sizeof(struct inotify_event)) <= 0)
	{
		perror("Error al leer el inotify");
	}

	if(evento->mask == IN_MODIFY || evento->mask == IN_CREATE || evento->mask == IN_DELETE)
	{
		t_config *nuevaConfig = config_create(global_path);

		if(nuevaConfig == NULL)
		{
			printf("Inotify detecto cambios pero no se puede crear la config.\n");
			return;
		}

		config = nuevaConfig;

		quantum = config_get_int_value(config, "QUANTUM");
		printf("Valor del quantum cambiado a: %d.\n",quantum);
	}

	free(evento);
}

void crearSemaforos()
{
	char **semArray = config_get_array_value(config, "SEM_IDS");
	char **semInit = config_get_array_value(config, "SEM_INIT");


	int i;
	for(i=0; semArray[i]!=NULL; i++)
	{
		t_SEM *nuevoSem= malloc(sizeof(t_SEM));

		nuevoSem->identificador = semArray[i];
		nuevoSem->valor = atoi(semInit[i]);

		printf("id:%s,valor:%d.\n",nuevoSem->identificador,nuevoSem->valor);

		list_add(SEM, nuevoSem);
	}
}

void crearIO()
{
	char **ioArray = config_get_array_value(config, "IO_IDS");
	char **ioSleep = config_get_array_value(config, "IO_SLEEP");


	int i;
	for(i=0; ioArray[i]!=NULL; i++)
	{
		t_IO *nuevoIO = malloc(sizeof(t_IO));

		nuevoIO->identificador = ioArray[i];
		nuevoIO->sleep = atoi(ioSleep[i]);
		nuevoIO->pid_usando = -1;

		printf("id:%s,valor:%d.\n",nuevoIO->identificador,nuevoIO->sleep);

		list_add(IO, nuevoIO);
	}

}

void crearShared()
{
	char **sharedArray = config_get_array_value(config, "SHARED_VARS");

	int i;
	for(i=0; sharedArray[i]!=NULL; i++)
	{
		t_SEM *nuevaShared = malloc(sizeof(t_SEM));//La estructura para semaforos y variables compartidas es la misma

		nuevaShared->identificador = sharedArray[i];
		nuevaShared->valor = 0;

		printf("sharedID:%s.\n",sharedArray[i]);

		list_add(SHARED, nuevaShared);
	}
}

//Validar que todos los parámetros existan en el archivo de configuracion
bool validarParametrosDeConfiguracion() {

	return (config_has_property(config, "PUERTO_PROG")
			&& config_has_property(config, "PUERTO_CPU")
			&& config_has_property(config, "IP_UMC")
			&& config_has_property(config, "PUERTO_UMC")
			&& config_has_property(config, "QUANTUM")
			&& config_has_property(config, "QUANTUM_SLEEP")
			&& config_has_property(config, "SEM_IDS")
			&& config_has_property(config, "SEM_INIT")
			&& config_has_property(config, "IO_IDS")
			&& config_has_property(config, "IO_SLEEP")
			&& config_has_property(config, "SHARED_VARS")
			&& config_has_property(config, "STACK_SIZE"));
}


void validacionUMC(int socket_umc){
	//Hago la validacion con UMC

	int mensaje;
	int buffer[2];

	buffer[0] = SOY_NUCLEO;
	buffer[1] = stack_size;


	recv(socket_umc, &mensaje, sizeof(int), 0);
	printf("mensaje recibido %d\n", mensaje);

	send(socket_umc, &buffer, 2*sizeof(int), 0);//Envio codMensaje y stackSize


	if(mensaje == SOY_UMC){

		printf("Conectado a UMC.\n");

		//Recibo el tamaño de pagina
		recv(socket_umc, &mensaje, sizeof(int), 0);
		pag_size = mensaje;
		printf("Page_size = %d\n",pag_size);

	}else{
		printf("mensaje: %d\n", mensaje);
		printf("Error de autentificacion con UMC.\n");
		close(socket_umc);
		exit(1);
	}
}

//TODO: Todas las validaciones de errores
void trabajarConexionesSockets(fd_set *listen, int *max_fd, int cpu_fd, int cons_fd){

	int i, codMensaje;

	//creo un set gral de sockets
	fd_set readyListen;
	//Creo estos sets donde van los descriptores que estan listos para lectura/escritura
	fd_set cpu_fd_set;
	fd_set cons_fd_set;

	struct timeval tiempo;
	tiempo.tv_sec = 0;
	tiempo.tv_usec = 0;

	//limpio los sets
	FD_ZERO(&readyListen);
	FD_ZERO(&cpu_fd_set);
	FD_ZERO(&cons_fd_set);

	//agrego el socket servidor al set
	FD_SET(cons_fd, &readyListen);
	FD_SET(cpu_fd, &readyListen);

	//Agrego el fd del inotify
	FD_SET(inotify_fd, listen);

	// main loop
	for (;;) {
		readyListen = *listen;

		if (select((*max_fd + 1), &readyListen, NULL, NULL, &tiempo) == -1) {
			perror("Error en el Select");
		}


		// Recorro buscando el socket que tiene datos
		for (i = 0; i <= *max_fd; i++) {
			if (FD_ISSET(i, &readyListen)) {
				if(i == inotify_fd)
				{
					cambiarConfig();
				}else if (i == cons_fd) {
					//Agrego la nueva conexion de Consola
					agregarConsola(i, max_fd, listen, &cons_fd_set);

				}else if (i == cpu_fd) {
					//Agrego la nueva conexion de CPU
					agregarCpu(i, max_fd, listen, &cpu_fd_set);
				}else{
					//Recibo el mensaje
					//int codMensaje = recibirMensaje(i, &ret_recv);

					if ( recv(i,&codMensaje,sizeof(int),0) <= 0) {
						// recibio 0 bytes, cierro la conexion y remuevo del set
						perror("");
						terminarConexion(i);
						printf("Error al recibir el mensaje. Se cierra la conexion\n");
						close(i);
						FD_CLR(i, listen);
					}else{
						if(FD_ISSET(i, &cpu_fd_set)){
							procesarMensajeCPU(codMensaje, i);
						}
						if(FD_ISSET(i, &cons_fd_set)){
							procesarMensajeConsola(codMensaje, i);
						}
					}
				}
			}
		}//Termine el for de los fd

		planificar();
	}
	//Limpio los pcb en finished
	limpiarTerminados();
}

void terminarConexion(int fd)
{
	//Tengo que buscar si la conexion es una cpu o una consola y eliminar las estructuras
	int i;
	for(i=0; i<list_size(listaCpu) ;i++)
	{
		if( getListaCpu(i)->socket == fd)
		{//Remuevo y elimino
			list_remove_and_destroy_element(listaCpu,i, free);
		}
	}

	for(i=0; i<list_size(listaConsola); i++)
	{
		if( getListaConsola(i)->socketConsola == fd )
		{
			list_remove_and_destroy_element(listaConsola,i, free);
		}
	}
}

int recibirMensaje(int socket, int *ret_recv){

	int codMensaje = 0;
	*ret_recv = recv(socket, &codMensaje, sizeof(int), 0);
	printf("socket %d, recvPaquete %d, msj %d \n", socket, *ret_recv, codMensaje);
	return codMensaje;
}


void agregarConsola(int fd, int *max_fd, fd_set *listen, fd_set *consolas){

	//1.acepto nueva conexion + handshake
	//2.creo el hilo para procesar lo que envia la nueva consola

	int soy_nucleo = SOY_NUCLEO;
	int nuevaConsola;
	int msj_recibido;
	struct sockaddr_in addr; // Para recibir nuevas conexiones
	socklen_t addrlen = sizeof(addr);

	nuevaConsola = accept(fd, (struct sockaddr *) &addr, &addrlen);

	printf("Se ha conectado un nueva consola %d.\n", nuevaConsola);

	send(nuevaConsola, &soy_nucleo, sizeof(int), 0);
	recv(nuevaConsola, &msj_recibido, sizeof(int), 0);

	if(msj_recibido == SOY_CONSOLA){

		//Actualiza el máximo
		if (nuevaConsola > *max_fd) {
			*max_fd = nuevaConsola;
		}

		//Agrega a la consola al set
		FD_SET(nuevaConsola, listen);
		FD_SET(nuevaConsola, consolas);


	}else{
		printf("No se verifico la autenticidad de la consola, cerrando la conexion. \n");
		close(nuevaConsola);
	}
}

void agregarCpu(int fd, int *max_fd, fd_set *listen, fd_set *cpus){

	int nuevaCPU;
	int msj_recibido;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	nuevaCPU = accept(fd, (struct sockaddr *) &addr, &addrlen);

	printf("Se ha conectado una nueva CPU.\n");

	int buffer[2];

	buffer[0] = SOY_NUCLEO;
	buffer[1] = quantum;

	send(nuevaCPU, &buffer, 2*sizeof(int),0);
	recv(nuevaCPU, &msj_recibido, sizeof(int),0);

	if(msj_recibido == SOY_CPU){

		if(nuevaCPU > *max_fd){
			*max_fd = nuevaCPU;
		}

		FD_SET(nuevaCPU,listen);
		FD_SET(nuevaCPU,cpus);

		//Agrego la cpu a la lista de cpus
		t_cpu *cpu_nueva = malloc(sizeof(t_cpu));

		cpu_nueva->id = nuevaCPU;
		cpu_nueva->socket = nuevaCPU;
		cpu_nueva->libre = true;
		cpu_nueva->pid = 0;
		max_cpu++;

		list_add(listaCpu,cpu_nueva);


		printf("CPUs disponibles %d\n", list_size(listaCpu));


	/*	if (cant_cpus > 0){

			t_cpu * nodoCPU = list_get(listaCPUs, 0);
			pthread_t hiloCPU;
			int thread1;
			thread1 = pthread_create(&hiloCPU, NULL, enviarPaqueteACPU, (void*) nodoCPU);

		}*/

	}else{
		printf("No se verifico la autenticidad de la cpu, cerrando la conexion...\n");
		close(nuevaCPU);
	}
}


void procesarMensajeCPU(int codigoMensaje, int fd){
	printf("procesarMensajeCPU con el msj.%d\n",codigoMensaje);

	switch(codigoMensaje){

	case ENTRADA_SALIDA:
		printf("mensaje recibido CPU Entrada-Salida");
		 //agrego el proceso en listaBloqueados, trato el sleep corresp
	break;

	case FIN_QUANTUM:
		printf("mensaje recibido CPU Fin Quantum");
		// saco el proceso de listaEjecutado, lo muevo donde corresponda
	break;

	case FINALIZAR:
		printf("mensaje recibido CPU Finalizar");
		//saco de listaEjecutado, envio msj a consola
	break;

	case OPERACION_PRIVILEGIADA:
		printf("mensaje recibido CPU Operacion privilegiada");
		//proceso según la operación que se haya solicitado
	break;

	case ANSISOP_IMPRIMIR:
		printf("mensaje recibido CPU imprimir\n");
		//busco la consola del proceso y le reenvio el mensaje a imprimir

		int mensajeAimprimir;

		if (recv(fd, &mensajeAimprimir, sizeof(int), 0) <= 0){
			perror("Error mensaje recibido");

		}else{
			printf("mensaje recibido %d\n", mensajeAimprimir);
			imprimirMsjConsola(fd, mensajeAimprimir);
		}
	break;

	case ANSISOP_IMPRIMIR_TEXTO:
		printf("mensaje recibido CPU imprimir texto\n");
		//busco la consola del proceso y le reenvio el mensaje a imprimir

		int sizeTexto;

		int sizeMensaje = recv(fd, &sizeTexto, sizeof(int), 0);

		if (sizeMensaje <= 0){
			printf("Error size recibido %d\n", sizeTexto);

		}else{
			printf("size recibido %d\n", sizeTexto);
			enviarTextoAConsola(fd, sizeTexto);
		}
	break;

	default:
		printf("mensaje recibido CPU Erroneo");
	}
}

int obtenerSocketConsola(int socketCPU){

/*	int socketBuscado;

	bool buscarCPUporSocket(t_cpu * nodoCPU) {
			return (nodoCPU->socket == socketCPU);
	}

	t_cpu * nodoDeCPU = NULL;
	//busca hasta que lo encuentra
	while (nodoDeCPU == NULL) {
		//nodoDeCPU = list_find(listaCPUs,(void*)buscarCPUporSocket);
	}


	bool buscarConsolaporPCB(t_consola * nodoConsola) {
			return (nodoConsola->pcb == nodoDeCPU->pcb);
	}

	t_consola* nodoDeConsola=NULL;
	//busca hasta que lo encuentra
	while (nodoDeConsola == NULL) {
		//nodoDeConsola = list_find(listaConsolas,(void*)buscarConsolaporPCB);
	}

	socketBuscado = nodoDeConsola->socketConsola;
	return socketBuscado;*/

	int i;
	for(i=0; i<list_size(listaCpu); i++)
	{
		if( getListaCpu(i)->socket == socketCPU)
		{
			int pid = getListaCpu(i)->pid;
			
			int j;
			for(j=0; j<cant_consolas; j++)
			{
				if(getListaConsola(j)->pid == pid)
				{
					return getListaConsola(j)->socketConsola;
				}
			}
		}
	}

	//No se encontro cpu
	printf("No se encontro la cpu con fd: %d\n", socketCPU);
	return -1;
}

void imprimirMsjConsola(int socketCPU, t_valor_variable mensaje){

	//obtener relacion socketCPU -idProceso - SocketConsola
	//enviar el mensaje al Socket consola obtenido

	int socketConsola = obtenerSocketConsola(socketCPU);

	int buffer[2];

	buffer[0] = ANSISOP_IMPRIMIR;
	buffer[1] = mensaje;

	if( send(socketConsola, &buffer, 2*sizeof(int), 0) == -1){
		perror("Error al enviar el mensaje a la consola");
	}else{
		printf("Mensaje enviado a la consola\n");
	}
}


char * recibirTexto(int socketCPU, int source_size){
	//recibo archivo de CPU
	char *buffer, *source;
	int cant_recibida = 0;
	int aux;

	recv(socketCPU, &source_size, sizeof(int), 0);

	buffer = malloc(source_size + 1);

	while(cant_recibida < source_size){

		aux = recv(socketCPU, buffer, source_size, 0);
		if (aux == -1){
			printf("Error al recibir texto");
			exit(1);
		}

		cant_recibida += aux;
	}
	//Ya tengo el archivo recibido en buffer. Le agrego un \0 al final.
	buffer[source_size] = '\0';
	source = buffer;

	source_size++;//Porque le agregue el \0
	printf("%s\n", source);
	return source;
}

void enviarTextoAConsola(int socketCPU, int sizeTexto){

	int mensaje;
	int aux;
	char *textoAimprimir;

	int socketConsola = obtenerSocketConsola(socketCPU);

	textoAimprimir = recibirTexto(socketCPU, sizeTexto);

	mensaje = ANSISOP_IMPRIMIR_TEXTO;

	char *buffer = malloc(sizeof(int)*2+sizeTexto);

	memcpy(buffer, &mensaje, sizeof(int));
	memcpy(buffer + sizeof(int), &sizeTexto, sizeof(int));
	memcpy(buffer + 2*sizeof(int), textoAimprimir, sizeTexto);

	printf("Envio mensaje imprimir texto a consola con el valor: %s\n", textoAimprimir);

	aux = send(socketConsola, &buffer, sizeof(buffer)+1, 0);

	if(aux == -1){
		printf("Error al enviar el texto a Consola.\n");
	}else{
		printf("Texto enviado a Consola: %s\n", textoAimprimir);
	}
}


void procesarMensajeConsola(int codigoMensaje, int fd){

	printf("codigoMensaje%d\n", codigoMensaje);

	switch(codigoMensaje){

	case ENVIO_FUENTE:
		iniciarNuevaConsola(fd);
		break;

	case FINALIZAR_EJECUCION:
		finalizarEjecucionProceso(fd);
		break;
	}
}


void moverDeNewAList(int pid, t_list *destino){

	bool igualPid(t_pcb *elemento){
		return elemento->pid == pid;
	}

	t_pcb *pcb_a_mover = list_find(new, (void*)igualPid);

	list_add(destino, pcb_a_mover);

	list_remove_by_condition(new, (void*)igualPid);
}

void moverDeNewAQueue(int pid, t_queue *destino)
{
	bool igualPid(t_pcb *elemento){
		return elemento->pid == pid;
	}

	t_pcb *pcb_a_mover = list_find(new, (void*)igualPid);

	queue_push(destino, pcb_a_mover);

	list_remove_by_condition(new, (void*)igualPid);
}

void iniciarNuevaConsola (int socket){

	//1.recibe pgm de Consola
	//2.crea el PCB lo agrega a la lista de listos
	//3.envia el pgm a UMC -(si fue aceptado, agrego el PCB a la lista)

	//recibo archivo de Consola
	char *buffer, *source;
	int source_size = 0;
	int cant_recibida = 0;
	int aux;

	recv(socket, &source_size, sizeof(int), 0);

	buffer = malloc(source_size + 1);

	while(cant_recibida < source_size){

		aux = recv(socket, buffer, source_size, 0);
		if (aux == -1){
			printf("Error al recibir archivo");
			exit(1);
		}

		cant_recibida += aux;
	}
	//Ya tengo el archivo recibido en buffer. Le agrego un \0 al final.
	buffer[source_size] = '\0';
	source = buffer;

	source_size++;//Porque le agregue el \0
	printf("%s\n", source);

	//Creo la estructura del PCB
	int pid = crearPCB(source_size, source);

	//Agrego la consola a la lista de consolas
	t_consola *nueva_consola = malloc(sizeof(t_consola));

	nueva_consola->pid = pid;
	nueva_consola->socketConsola = socket;

	list_add(listaConsola,nueva_consola);

	//solicito paginas necesarias a UMC. Si el pgm fue enviado con exito, lo agrego a la lista.
	if( solicitarPaginasUMC(source_size, source, pid) == -1)
	{//Hubo error tengo que eliminar las consolas y eso...

	}

	//Libero la memoria
	free(buffer);
}


int crearPCB(int source_size,char *source){

	t_pcb *pcb = malloc(sizeof(t_pcb));
	t_indice_codigo indiceCodigo;
	t_indice_etiquetas indiceEtiquetas;

	//Cargo camposPCB
	max_pid++;
	pcb->pid = max_pid;
	pcb->PC = 0;
	pcb->cant_pag = source_size;
	pcb->idCPU = 0;

	t_metadata_program* metadata;
	metadata = metadata_desde_literal(source);

	//Cargo indiceCodigo
	indiceCodigo.instrucciones_size = metadata->instrucciones_size;
	indiceCodigo.instrucciones = metadata->instrucciones_serializado;
	indiceCodigo.instruccion_inicio = metadata->instruccion_inicio;

	//Cargo indiceEtiquetas
	indiceEtiquetas.etiquetas_size =  metadata->etiquetas_size;
	indiceEtiquetas.etiquetas = metadata->etiquetas;

	//Cargo indices en PCB
	pcb->indice_cod = indiceCodigo;
	pcb->indice_etiquetas = indiceEtiquetas;
	pcb->stack.cant_entradas = 0;

	pcb->tamanio = tamanioPcb(*pcb);

	list_add(new, pcb);

	return pcb->pid;

}

int solicitarPaginasUMC(int source_size, char *source, int pid){

	int mensaje;
	int cant_enviada = 0;
	int aux;

	//Pido paginas para almacenar el codigo y el stack
	mensaje = INICIALIZAR_PROGRAMA;
	//Armo el paquete a enviar
	int buffer_size = source_size + 4*sizeof(int);
	char *buffer = malloc(source_size + 4*sizeof(int) );

	int cant_paginas_requeridas = (source_size / pag_size) + 1;

	if(source_size % pag_size > 0)
		cant_paginas_requeridas--;

	memcpy(buffer, &mensaje, sizeof(int));
	memcpy(buffer + sizeof(int), &pid, sizeof(int));
	memcpy(buffer + 2*sizeof(int), &cant_paginas_requeridas, sizeof(int));
	memcpy(buffer + 3*sizeof(int), &source_size, sizeof(int));
	memcpy(buffer + 4*sizeof(int), source, source_size);

	while(cant_enviada < buffer_size){
		aux = send(socket_umc, buffer + cant_enviada, buffer_size - cant_enviada, 0);

		if(aux == -1){
			printf("Error al enviar el archivo a UMC.\n");
			return -1;
		}

		cant_enviada += aux;
	}
	printf("Archivo enviado satisfactoriamente a UMC. %s\n", source);

	//Recibo la respuesta de UMC, si fue posible o no iniciar el programa
	int *msj = malloc(2*sizeof(int));

	recv(socket_umc,msj,2*sizeof(int),MSG_WAITALL);

	if(msj[0] == ACEPTO_PROGRAMA)
	{
		printf("ACEPTO_PROGRAMA\n");
		//Lo paso a ready
		moverDeNewAQueue(pid,ready);

	}else if(msj[0] == RECHAZO_PROGRAMA)
	{
		printf("RECHAZO_PROGRAMA\n");
		//Lo paso a finished
		moverDeNewAQueue(pid,finished);
	}else{
		printf("Recibi un mensaje incorrecto de umc.\n");
		printf("%d %d \n", msj[0],msj[1]);
		perror("");
	}

	return 0;

}


void* enviarPaqueteACPU(void *nodo){

	t_cpu * nodoCPU = nodo;

	while(1){
	//	t_pcb * nodoPCB =  list_get(listaListos, 0);
		//nodoPCB->idCPU= nodoCPU->id;

		//int mensaje = ENVIO_PCB;
		int mensaje = 0;
		//int sizePCB = sizeof(nodoPCB);
		int sizePCB = 1024;
		char *buffer;
		//Armo el paquete a enviar
		int buffer_size = sizeof(int) + sizeof(int) + sizeof(t_pcb);
		buffer = malloc(buffer_size);

		memcpy(buffer, &mensaje, sizeof(int));
		memcpy(buffer + sizeof(int), &sizePCB, sizeof(int));
		//memcpy(buffer +2*sizeof(int), nodoPCB, sizeof(t_pcb));

		printf("mensaje a enviar: %d\n", mensaje);
		printf("size pcb a enviar: %d\n", sizePCB);
		printf("id pid a enviar%d\n", pcb->pid);

		//int envioPCB = send(nodoCPU->socket,buffer,sizeof(buffer),0);

		int cant_enviada = 0;

		while(cant_enviada < buffer_size){

			int envioPCB = send(nodoCPU->socket, buffer + cant_enviada, buffer_size - cant_enviada, 0);

			if(envioPCB <= 0){
		//		printf("Error en el envio del PCB %d/n", nodoPCB->pid);
				exit(1);
			}else {
				//envio correcto:  cambio estado cpu y asigno id pcb. muevo a la lista correspondiente
			//	nodoCPU->pcb = nodoPCB->pid;
				nodoCPU->libre = false;
				//list_remove(listaListos, 0);
				//list_add(listaEjecutar,nodoPCB);
				//printf ("PCB enviado a CPU.%d\n", nodoPCB->pid);
			}
			cant_enviada += envioPCB;
		}
	}
}

void finalizarEjecucionProceso(int socket){

	//buscar pid del proceso correspondiente al socket
	//Si está en la lista de ejecutando, dejar que termine la ráfaga. Si esta en cualquier otro estado debe liberar recursos y frenar la ejec

	printf("finalizar ejecucion proceso\n");

}

void limpiarTerminados(){

	t_pcb *pcb_terminado;

	while(!queue_is_empty(finished))
	{
		pcb_terminado = queue_pop(finished);

		//Le aviso a consola que termino la ejecucion del programa
		int mensaje = FINALIZAR;

		//Funcion para buscar consola
		bool igualPid(t_consola *elemento)
		{
			return elemento->pid == pcb_terminado->pid;
		}
		t_consola *consola = list_find(listaConsola,(void*)igualPid);

		send(consola->socketConsola,&mensaje,sizeof(int),0);

		//Elimino la consola y cierro el socket
		list_remove_by_condition(listaConsola,(void*)igualPid);
		close(consola->socketConsola);
		free(consola);

		//Elimino el pcb
		freePcb(pcb_terminado);
	}

}

void planificar(){

	int i;

	//Miro que no este vacia la lista de ready
	while(!queue_is_empty(ready)){

		if(cantCpuLibres() == 0)
			break;

		for(i=0; i<list_size(listaCpu); i++){

			if(getListaCpu(i)->libre){
				printf("CPU LIBRE.\n");
				t_pcb *pcb_a_ejecutar = queue_pop(ready);

				getListaCpu(i)->libre = false;
				getListaCpu(i)->pid = pcb_a_ejecutar->pid;

				enviarPcb(*pcb_a_ejecutar, getListaCpu(i)->socket, quantum);

				freePcb(pcb_a_ejecutar);
			}
		}
	}
}

int cantCpuLibres()
{
	int cant = 0;
	int i;
	for(i=0; i<list_size(listaCpu); i++)
	{
		if(getListaCpu(i)->libre)
			cant++;
	}

	return cant;
}
