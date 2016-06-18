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

	listaReady = list_create();
	listaCPUs = list_create();

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

	int i;

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
					agregarCpu(i, max_fd, listen, &cpu_fd_set);
				}else{
					//Recibo el mensaje
					int codMensaje = recibirMensaje(i);

					if (codMensaje <= 0) {
						// recibio 0 bytes, cierro la conexion y remuevo del set
						close(i);
						printf("Error al recibir el mensaje. Se cierra la conexion\n");
						FD_CLR(i, &readyListen);
					}
					if(FD_ISSET(i, &cpu_fd_set)){
						hacerAlgoCPU(codMensaje, i);
					}
					if(FD_ISSET(i, &cons_fd_set)){
						hacerAlgoConsola(codMensaje, i);
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

	int codMensaje = 0;
	int recvPaquete = recv(socket, &codMensaje, sizeof(int), 0);
	printf("socket %d, recvPaquete %d, msj %d \n", socket, recvPaquete, codMensaje);
	return codMensaje;
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
		t_cpu cpu_nueva;

		cpu_nueva.id = nuevaCPU;
		cpu_nueva.socket = nuevaCPU;
		cpu_nueva.libre = true;
		cpu_nueva.pcb = 0;

		max_cpu++;

		cant_cpus++;
		//Necesito un array con 1 cpu mas de espacio
		listaCpu = realloc(listaCpu, cant_cpus * sizeof(t_cpu));
		list_add(listaCPUs, listaCpu);

		listaCpu[cant_cpus - 1] = cpu_nueva;
		printf("CPUs disponibles %d\n", cant_cpus);

		if (cant_cpus > 0){
			t_cpu * nodoCPU = list_get(listaCPUs, 0);
			enviarPaqueteACPU(nodoCPU);
		}

	}else{
		printf("No se verifico la autenticidad de la cpu, cerrando la conexion...\n");
		close(nuevaCPU);
	}
}


void hacerAlgoCPU(int codigoMensaje, int fd){
	printf("hacerAlgoCPU con el msj.%d\n",codigoMensaje);

	switch(codigoMensaje){

	case ENTRADA_SALIDA:
		 //agrego el proceso en listaBloqueados, trato el sleep corresp
	break;

	case FIN_QUANTUM:
		// saco el proceso de listaEjecutado, lo muevo donde corresponda
	break;

	case FINALIZAR:
		//saco de listaEjecutado, envio msj a consola
	break;

	case OPERACION_PRIVILEGIADA:
		//proceso según la operación que se haya solicitado
	break;

	default:
		printf("mensaje Erroneo");
	}
}

void hacerAlgoConsola(int codigoMensaje, int fd){

	printf("codigoMensaje%d\n", codigoMensaje);

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
		moverDeNewA(msj_recibido,&listaReady);
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

	//1.recibe pgm de Consola
	//2.crea el PCB lo agrega a la lista de listos
	//3.envia el pgm a UMC -(si fue aceptado, agrego el PCB a la lista)


	//recibo archivo de Consola
	char *buffer, *source;
	int source_size = 0;
	int cant_recibida = 0;
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

	//Creo la estructura del PCB
	crearPCB(source_size, source);

	//solicito paginas necesarias a UMC. Si el pgm fue enviado con exito, lo agrego a la lista.
	int solicitudOK = solicitarPaginasUMC(source_size, buffer, source);
	if (solicitudOK){
		printf("solicitud de paginas ok \n");
		//list_add(&ready, paquete);
	}else{
		printf("no hay espacio, solicitud rechazada\n");
		list_remove(&new, 0);
	}

	//Libero la memoria
	free(buffer);

}


void crearPCB(int source_size,char *source){

	char *paquete;

	t_indice_codigo * indiceCodigo;
	t_indice_etiquetas * indiceEtiquetas;
	t_indice_stack * indiceStack;

	pcb = malloc(sizeof(t_pcb));
	indiceCodigo = malloc(sizeof(t_indice_codigo));
	indiceEtiquetas = malloc(sizeof(t_indice_etiquetas));
	indiceStack = malloc(sizeof(t_indice_stack));

	// pcb + indiceCodigo + indiceEtiquetas (ver estructura de Etiquetas que tiene el tamaño segun el size_etiqueta) + indiceStack
	paquete = malloc(sizeof(t_pcb) + sizeof(t_indice_codigo) +  sizeof(t_indice_etiquetas) + sizeof(t_indice_stack));

	pcb->pid = ++max_pid;
	pcb->PC = 0;
	pcb->cant_pag = source_size;
	pcb->idCPU = 0;

	t_metadata_program* metadata;
	metadata = metadata_desde_literal(source);

	indiceCodigo->instrucciones_size = metadata->instrucciones_size;
	indiceCodigo->instrucciones = metadata->instrucciones_serializado;
	indiceCodigo->instruccion_inicio = metadata->instruccion_inicio;

	/*	int i;
	for(i=0; i < metadata->instrucciones_size; i++){
		t_intructions instr = metadata->instrucciones_serializado[i];
		indiceCodigo->byte_inicio[i] = instr.start;
		indiceCodigo->long_instruccion[i] = instr.offset;
	}*/

	indiceEtiquetas->etiquetas_size =  metadata->etiquetas_size;
	indiceEtiquetas->etiquetas = metadata->etiquetas;

	indiceStack->argumentos = 0;
	indiceStack->dirRetorno = 0;
	indiceStack->posVariable = 0;
	indiceStack->variables = 0;

	pcb->indice_cod = indiceCodigo;
	pcb->indice_etiquetas = indiceEtiquetas;
	pcb->indice_stack = indiceStack;

	//Armo el paquete
	//memcpy(paquete, &mensaje, sizeof(int));
	memcpy(paquete, &pcb, sizeof(t_pcb));
	//memcpy(paquete + sizeof(int) + sizeof(t_pcb), &indiceCodigo, sizeof(t_indice_codigo));
	//memcpy(paquete + sizeof(int) + sizeof(t_pcb) + sizeof(t_indice_codigo), &indiceEtiquetas, sizeof(t_indice_etiquetas));
	//memcpy(paquete + sizeof(int) + sizeof(t_pcb) + sizeof(t_indice_codigo) + sizeof(t_indice_etiquetas), &indiceStack, sizeof(indiceStack));

	list_add(listaReady, pcb);
	printf("elementos lista: %d\n", listaReady->elements_count);

}


int solicitarPaginasUMC(int source_size, char *buffer, char *source){

	int mensaje;
	int cant_enviada = 0;
	int aux;

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
		aux = send(socket_umc, buffer + cant_enviada, buffer_size - cant_enviada, 0);

		if(aux == -1){
			printf("Error al enviar el archivo a UMC.\n");
			return -1;
		}

		cant_enviada += aux;
	}
	printf("Archivo enviado satisfactoriamente a UMC. %s\n", source);
	return cant_enviada;

}


void enviarPaqueteACPU(t_cpu * nodoCPU){

	t_pcb * nodoPCB =  list_get(listaReady, 0);
	nodoPCB->idCPU= nodoCPU->id;

	int mensaje = ENVIO_PCB;
	char *buffer;
	//Armo el paquete a enviar
	int buffer_size = sizeof(int) + sizeof(t_pcb);
	buffer = malloc(buffer_size);

	memcpy(buffer, &mensaje, sizeof(int));
	memcpy(buffer + sizeof(int), nodoPCB, sizeof(t_pcb));

	int envioPCB = send(nodoCPU->socket,buffer,sizeof(buffer),0);

	if(envioPCB <= 0){
		printf("Error en el envio del PCB");
		exit(1);
	}else {
		//envio correcto:  cambio estado cpu y asigno id pcb. muevo a la lista correspondiente
		nodoCPU->pcb = nodoPCB->pid;
		nodoCPU->libre = false;
		//list_remove(listaDeListo, 0);
		//list_add(listaDeEjecutado,nodoPCB);
		printf ("PCB enviado a CPU.%d\n", nodoPCB->pid);

	}
}

void limpiarTerminados(){

	t_consola *pcb_terminado;
	int mensaje = FIN_PROGRAMA;

	while(!queue_is_empty(&finished)){

		pcb_terminado = queue_pop(&finished);

		//Le aviso a consola que termino la ejecucion del programa
		send(pcb_terminado->socketConsola,&mensaje,sizeof(int),0);

		close( pcb_terminado->socketConsola );

		free(pcb_terminado);
	}
}

void planificar(){

	int i;

	//Miro que no este vacia la lista de ready
	if(queue_is_empty(listaReady)){

		for(i=0; i<cant_cpus; i++){

			if(listaCpu[i].libre){
				//Cpu libre: le asigno el proceso que esta hace mas tiempo en la cola(queue_pop)
			}
		}
	}
}
