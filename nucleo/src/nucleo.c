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

	//valida los parametros de entrada del main
	if (argc != 2) {
		fprintf(stderr, "Uso: nucleo config_path\n");
		return 1;
	}

	FD_ZERO(&listen);


	crearConfiguracion(argv[1]);

	//Conexion consolas
	puerto_cons = config_get_string_value(config, "PUERTO_PROG");
	cons_fd = conectarPuertoDeEscucha(puerto_cons);
	maximoFileDescriptor(cons_fd,&max_fd);
	FD_SET(cons_fd, &listen);
	printf("socket escucha consola: %d \n", cons_fd);
	printf("Esperando conexiones de consolas.. (PUERTO: %s)\n", puerto_cons);
	cant_cpus = 0;
	max_cpu = 0;

	stack_size = config_get_int_value(config, "STACK_SIZE");

	quantum = config_get_int_value(config, "QUANTUM");


	//Conexion CPUs
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
		return;
	} else {
		printf("Configuracion no valida\n");
		exit(EXIT_FAILURE);
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

	send(umc_fd, &buffer, 2*sizeof(int), 0);//Envio codMensaje y stackSize

	recv(umc_fd, &mensaje, sizeof(int), 0);

	if(mensaje == SOY_UMC){

		printf("Conectado a UMC.\n");

		//Recibo el tamaño de pagina
		recv(umc_fd, &mensaje, sizeof(int), 0);
		pag_size = mensaje;
		printf("Page_size = %d\n",pag_size);

	}else{
		printf("mensaje: %d\n", mensaje);
		printf("Error de autentificacion con UMC.\n");
		close(umc_fd);
		exit(1);
	}
}

//TODO: Todas las validaciones de errores
void trabajarConexionesSockets(fd_set *listen, int *max_fd, int cpu_fd, int cons_fd){

	int i;
	int codigoMensaje;
	int recvPaquete;

	//creo un set gral de sockets
	fd_set readyListen;
	//Creo estos sets donde van los descriptores que estan listos para lectura/escritura
	fd_set cpu_fd_set;
	fd_set cons_fd_set;


	//limpio los sets
	FD_ZERO(&readyListen);
	FD_ZERO(&cpu_fd_set);
	FD_ZERO(&cons_fd_set);

	//agrego el socket servidor al set
	FD_SET(cons_fd, &readyListen);
	FD_SET(cpu_fd, &readyListen);


	// main loop
	for (;;) {
		readyListen = *listen;

		if (select((*max_fd + 1), &readyListen, NULL, NULL, NULL) == -1) {
			perror("Error en el Select");
		}


		// Recorro buscando el socket que tiene datos
		for (i = 0; i <= *max_fd; i++) {
			if (FD_ISSET(i, &readyListen)) {
				if (i == cons_fd) {
					//Agrego la nueva conexion de Consola
					agregarConsola(i, max_fd, listen, &cons_fd_set);

				}else if (i == cpu_fd) {
					//Agrego la nueva conexion de CPU
					agregarConexion(i, max_fd, listen, &cpu_fd_set, 3000);
					agregarCpu(i, max_fd, listen, &cpu_fd_set);
				}else{
					//Recibo el mensaje
					char package[1024];
					int recvPaquete = recv(i,(void*) package, 5, 0);
					printf("socket %d, recvPaquete %d, msj %s \n", i, recvPaquete, package);

					//recvPaquete = recibirMensaje(i);
					if (recvPaquete <= 0) {
						// recibio 0 bytes, cierro la conexion y remuevo del set
						close(i);
						printf("Error al recibir el mensaje. Se cierra la conexion\n");
						FD_CLR(i, &readyListen);
					}
					if(FD_ISSET(i, &cpu_fd_set)){
						hacerAlgoCPU(package, i);
					}
					if(FD_ISSET(i, &cons_fd_set)){
						hacerAlgoConsola(package, i);
					}
				}
			}
		}
	}
	//Limpio los pcb en finished
	limpiarTerminados();
	planificar();
}


int recibirMensaje(int socket){
	char package[1024];
	int recvPaquete = recv(socket,(void*) package, 5, 0);
	printf("socket %d, recvPaquete %d, msj %s \n", socket, recvPaquete, package);

	return recvPaquete;

}


void agregarConsola(int fd, int *max_fd, fd_set *listen, fd_set *consolas){

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

	int nuevo_fd;
	int msj_recibido;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	nuevo_fd = accept(fd, (struct sockaddr *) &addr, &addrlen);

	printf("Se ha conectado una nueva CPU.\n");

	int buffer[2];

	buffer[0] = SOY_NUCLEO;
	buffer[1] = quantum;

	send(nuevo_fd, &buffer, 2*sizeof(int),0);
	recv(nuevo_fd, &msj_recibido, sizeof(int),0);

	if(msj_recibido == SOY_CPU){

		if(nuevo_fd > *max_fd){
			*max_fd = nuevo_fd;
		}

		FD_SET(nuevo_fd,listen);
		FD_SET(nuevo_fd,cpus);

		//Agrego la cpu a la lista de cpus
		t_cpu cpu_nueva;

		cpu_nueva.fd = nuevo_fd;
		cpu_nueva.libre = true;
		cpu_nueva.num = max_cpu;
		max_cpu++;

		cant_cpus++;
		//Necesito un array con 1 cpu mas de espacio
		listaCpu = realloc(listaCpu, cant_cpus * sizeof(t_cpu));

		listaCpu[cant_cpus - 1] = cpu_nueva;
	}else{
		printf("No se verifico la autenticidad de la cpu, cerrando la conexion...\n");
		close(nuevo_fd);
	}
}

void agregarConexion(int fd, int *max_fd, fd_set *listen, fd_set *particular, int msj){
	//msj Es el mensaje de autentificacion.

	int soy_nucleo = SOY_NUCLEO;
	int nuevo_fd;
	int msj_recibido;
	struct sockaddr_in addr; // Para recibir nuevas conexiones
	socklen_t addrlen = sizeof(addr);

	nuevo_fd = accept(fd, (struct sockaddr *) &addr, &addrlen);

	printf("Se ha conectado un nuevo usuario.\n");

	send(nuevo_fd, &soy_nucleo, sizeof(int), 0);
	recv(nuevo_fd, &msj_recibido, sizeof(int), 0);
	printf("socket: %d, mensaje %d", nuevo_fd,msj_recibido);

	if(msj_recibido == msj){

		//Actualiza el máximo
		if (nuevo_fd > *max_fd) {
			*max_fd = nuevo_fd;
		}

		if(msj == SOY_CONSOLA){
			socket_consola = nuevo_fd;
			printf("El nuevo usuario es una consola, socket %d\n", socket_consola);


		}else if(msj == SOY_CPU){
			socket_CPU = nuevo_fd;
			printf("El nuevo usuario es una cpu, socket %d\n", socket_CPU);
		}

		FD_SET(nuevo_fd, listen);
		FD_SET(nuevo_fd, particular);

	}else{
		printf("No se verifico la autenticidad del usuario, cerrando la conexion. \n");
		close(nuevo_fd);
	}

}

void hacerAlgoCPU(int codigoMensaje, int fd){
	printf("hacerAlgoCPU.%d\n",codigoMensaje);
}

void hacerAlgoConsola(int codigoMensaje, int fd){

	switch(codigoMensaje){

	case ENVIO_FUENTE:
		iniciarNuevaConsola(fd);
	}
}

void hacerAlgoUmc(int codigoMensaje){

	int msj_recibido;

	switch(codigoMensaje){

	case RECHAZO_PROGRAMA:
		recv(umc_fd,&msj_recibido,sizeof(int),0);
		moverDeNewA(msj_recibido,&finished);
		break;

	case ACEPTO_PROGRAMA:
		recv(umc_fd,&msj_recibido,sizeof(int),0);
		moverDeNewA(msj_recibido,&ready);
		break;

	}
}

void moverDeNewA(int pid, t_queue *destino){

	bool igualPid(t_pcb *elemento){
		return elemento->pid == pid;
	}

	t_pcb *pcb_a_mover = list_find(&new, (void*)igualPid);

	queue_push(destino, pcb_a_mover);

	list_remove_and_destroy_by_condition(&new, (void*)igualPid, (void*)free);
}

void iniciarNuevaConsola (int fd){
	int cant_recibida = 0;
	int cant_enviada = 0;
	int source_size;
	char *buffer, *source;
	int mensaje;
	int aux;

	recv(fd, &source_size, sizeof(int), 0);

	buffer = malloc(source_size + 1);

	while(cant_recibida < source_size){

		aux = recv(fd, buffer, source_size, 0);
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

	//Creo el PCB
	t_pcb *pcb;

	pcb = malloc(sizeof(pcb));

	pcb->PC = 0;
	pcb->consola_fd = fd;
	pcb->pid = max_pid + 1;
	max_pid++;

	//Pido paginas para almacenar el codigo y el stack
	mensaje = INICIALIZAR_PROGRAMA;
	//Armo el paquete a enviar
	int buffer_size = source_size + 4*sizeof(int);
	buffer = malloc(source_size + 4*sizeof(int) );

	int cant_paginas_requeridas = source_size / pag_size;

	if(source_size % pag_size > 0)
		cant_paginas_requeridas++;

	memcpy(buffer, &mensaje, sizeof(int));
	memcpy(buffer + sizeof(int), &max_pid, sizeof(int));
	memcpy(buffer + 2*sizeof(int), &cant_paginas_requeridas, sizeof(int));
	memcpy(buffer + 3*sizeof(int), &source_size, sizeof(int));
	memcpy(buffer + 4*sizeof(int), source, source_size);

	while(cant_enviada < buffer_size){
		aux = send(umc_fd, buffer + cant_enviada, buffer_size - cant_enviada, 0);

		if(aux == -1){
			printf("Error al enviar el archivo a UMC.\n");
			exit(1);
		}

		cant_enviada += aux;
	}
	printf("Archivo enviado satisfactoriamente.\n");

	//Ya hice los pedidos necesarios a la UMC, agrego el proceso a la lista de nuevos.
	list_add(&new, pcb);

	//Libero la memoria
	free(buffer);
	free(source);
}

void procesoMensajeRecibidoConsola(char *package, int socket){
	printf("Mensaje recibido Consola: %s\n",package);
	//send a CPU
	if (strlen(package) >0){

		if (socket_CPU != 0){
			enviarPaqueteACPU(package, socket_CPU);
		}else{
			printf("no hay CPU conectada");
		}
	}
}

void enviarPaqueteACPU(char* package, int socket){
	//Envio el archivo entero

	int resultSend = send(socket, package, PACKAGESIZE, 0);
	printf("resultado send %d, a socket %d \n",resultSend, socket);
	if(resultSend == -1){
		printf ("Error al enviar archivo.\n");
		exit(1);
	}else {
		printf ("Archivo enviado a CPU.\n");
		printf ("mensaje enviado a CPU: %s\n", package);
	}
}

void limpiarTerminados(){

	t_pcb *pcb_terminado;
	int mensaje = FIN_PROGRAMA;

	while(!queue_is_empty(&finished)){

		pcb_terminado = queue_pop(&finished);

		//Le aviso a consola que termino la ejecucion del programa
		send(pcb_terminado->consola_fd,&mensaje,sizeof(int),0);

		close( pcb_terminado->consola_fd );

		free(pcb_terminado);
	}
}

void planificar(){

	int i;

	//Miro que no este vacia la lista de ready
	if(queue_is_empty(&ready)){

		for(i=0; i<cant_cpus; i++){

			if(listaCpu[i].libre){
				//Cpu libre: le asigno el proceso que esta hace mas tiempo en la cola(queue_pop)
			}
		}
	}
}
