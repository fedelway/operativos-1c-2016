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
		quantum_sleep = config_get_int_value(config,"QUANTUM_SLEEP");
		printf("Valor del quantum sleep cambiado a: %d.\n",quantum_sleep);

		//Cambio el flag de cambio quantum sleep
		int i;
		for(i=0;i<list_size(listaCpu);i++)
		{
			getListaCpu(i)->cambioQuantumSleep = true;
		}
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
		nuevoSem->procesos_esperando = queue_create();

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
		nuevoIO->sleep = ((double)atoi(ioSleep[i])) / 1000;
		nuevoIO->procesos_esperando = queue_create();

		printf("id:%s,sleep:%f.\n",nuevoIO->identificador,nuevoIO->sleep);

		list_add(IO, nuevoIO);
	}

}

void crearShared()
{
	char **sharedArray = config_get_array_value(config, "SHARED_VARS");

	int i;
	for(i=0; sharedArray[i]!=NULL; i++)
	{
		t_SHARED *nuevaShared = malloc(sizeof(t_SHARED));

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
							procesarMensajeCPU(codMensaje, i, listen);
						}
						if(FD_ISSET(i, &cons_fd_set)){
							procesarMensajeConsola(codMensaje, i);
						}
					}
				}
			}
		}//Termine el for de los fd
		planificar();
		checkearEntradaSalida();

		//Limpio los pcb en finished
		limpiarTerminados(listen);
	}
}

void checkearEntradaSalida()
{
	t_IO *io;
	t_proceso_esperando *proceso;

	int i;
	for(i=0;i<list_size(IO);i++)
	{
		io = list_get(IO,i);

		if(!queue_is_empty(io->procesos_esperando) )
		{
			proceso = queue_peek(io->procesos_esperando);

			if( ((double)time(NULL) - proceso->tiempo_inicio) >= ( ((double)proceso->tiempo_espera) * io->sleep) )
			{//Ya paso el tiempo: agrego el pcb a ready
				printf("Se desbloqueo un proceso.\n");

				queue_push(ready,proceso->pcb);

				//Elimino el proceso actual de la cola de espera
				queue_pop(io->procesos_esperando);
				free(proceso);

				//Setteo el tiempo al siguiente proceso
				proceso = queue_peek(io->procesos_esperando);

				if(proceso != NULL)
					proceso->tiempo_inicio = time(NULL);
			}
		}
	}
}

void terminarConexion(int fd)
{
	printf("Terminando conexion.\n");

	//Tengo que buscar si la conexion es una cpu o una consola y eliminar las estructuras
	int i;
	for(i=0; i<list_size(listaCpu) ;i++)
	{
		if( getListaCpu(i)->socket == fd)
		{
			printf("La conexion es una cpu.\n");

			t_cpu *cpu = list_remove(listaCpu,i);

			//Paso el pcb que estaba asignado a la cpu a ready
			if(cpu->pcb != NULL)
				queue_push(ready,cpu->pcb);

			free(cpu);
		}
	}

	for(i=0; i<list_size(listaConsola); i++)
	{
		if( getListaConsola(i)->socketConsola == fd )
		{
			t_consola *consola = list_remove(listaConsola,i);

			finalizarPrograma(consola->pid);

			free(consola);
		}
	}
}

void finalizarPrograma(int pid)
{//Busco en todas las listas si esta el pcb y lo elimino.

	eliminarDeCola(ready,pid);

	int i;
	for(i=0;i<list_size(SEM);i++)
	{
		eliminarDeCola( ((t_SEM*)list_get(SEM,i))->procesos_esperando, pid);
	}

	for(i=0;i<list_size(IO);i++)
	{
		eliminarDeIO( ((t_IO*)list_get(IO,i))->procesos_esperando, pid);
	}

	//Busco en la lista de cpus
	for(i=0;i<list_size(listaCpu);i++)
	{
		if(getListaCpu(i)->pcb != NULL)
		{
			if(getListaCpu(i)->pcb->pid == pid)
			{
				getListaCpu(i)->finForzoso = true;
			}
		}
	}

	//Lo agrego a la lista de finished(Esto lo deberia hacer en lo de fin forzoso)
/*	int *puntero_pid = malloc(sizeof(int));
	*puntero_pid = pid;

	queue_push(finished,puntero_pid);*/

	printf("Salgo de finalizar programa.\n");
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

	int nuevo_fd;
	int msj_recibido;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	nuevo_fd = accept(fd, (struct sockaddr *) &addr, &addrlen);

	printf("Se ha conectado una nueva CPU.\n");

	int buffer[3];

	buffer[0] = SOY_NUCLEO;
	buffer[1] = quantum;
	buffer[2] = quantum_sleep;

	send(nuevo_fd, &buffer, 3*sizeof(int),0);
	recv(nuevo_fd, &msj_recibido, sizeof(int),0);

	if(msj_recibido == SOY_CPU){

		if(nuevo_fd > *max_fd){
			*max_fd = nuevo_fd;
		}

		FD_SET(nuevo_fd,listen);
		FD_SET(nuevo_fd,cpus);

		//Agrego la cpu a la lista de cpus
		max_cpu++;
		t_cpu *cpu_nueva = malloc(sizeof(t_cpu));

		cpu_nueva->id = max_cpu;
		cpu_nueva->socket = nuevo_fd;
		cpu_nueva->libre = true;
		cpu_nueva->pid = 0;
		cpu_nueva->finForzoso = false;
		cpu_nueva->cambioQuantumSleep = false;
		cpu_nueva->pcb = NULL;

		list_add(listaCpu,cpu_nueva);

		printf("CPUs disponibles %d\n", list_size(listaCpu));

	}else{
		printf("No se verifico la autenticidad de la cpu, cerrando la conexion...\n");
		close(nuevo_fd);
	}
}


void procesarMensajeCPU(int codigoMensaje, int fd, fd_set *listen){
	//printf("procesarMensajeCPU con el msj.%d\n",codigoMensaje);
	int *puntero_inutil;

	t_pcb *pcb_recibido;

	switch(codigoMensaje){

	case FIN_QUANTUM:
			printf("mensaje recibido CPU Fin Quantum.\n");

			procesarFinQuantum(fd);

		break;

	case ANSISOP_ENTRADA_SALIDA:
		printf("mensaje recibido CPU Entrada-Salida.\n");
		procesarEntradaSalida(fd);

	break;

	case ANSISOP_OBTENER_VALOR_COMPARTIDO:
		printf("Mensaje recibido CPU OBTENER VALOR COMPARTIDO.\n");
		obtenerValorCompartido(fd);
		break;

	case ANSISOP_ASIGNAR_VALOR_COMPARTIDO:
		printf("Mensaje recibido CPU ASIGNAR VALOR COMPARTIDO.\n");
		asignarValorCompartido(fd);
		break;

	case ANSISOP_WAIT:
		printf("Mensaje recibido CPU WAIT.\n");
		ansisopWait(fd);
		break;

	case ANSISOP_SIGNAL:
		printf("Mensaje recibido CPU SIGNAL.\n");
		ansisopSignal(fd);
		break;

	case ANSISOP_IMPRIMIR:
		printf("mensaje recibido CPU imprimir\n");
		imprimirValor(fd);
		break;

	case ANSISOP_IMPRIMIR_TEXTO:
		printf("mensaje recibido CPU imprimir texto\n");
		imprimirTexto(fd);
		break;

	case ANSISOP_FIN_PROGRAMA:
		printf("mensaje recibido CPU Finalizar.\n");
		//Muevo a finished el proceso

		int *pid = malloc(sizeof(int));
		recv(fd,pid,sizeof(int),0);

		queue_push(finished,pid);
		printf("Ya puse el pid en finished.\n");

		//Libero la cpu
		liberarCpu(fd);

		break;

	case DESCONEXION_CPU:
		printf("-------------------------------------\nSe ha desconectado una cpu.\n\n");

		//Pregunto a ver si debo recibir el pcb
		int mensaje;
		recv(fd,&mensaje,sizeof(int),0);

		if(mensaje != BLOQUEADO)
		{//Tengo que recibir el pcb
			printf("Debo recibir el pcb.\n");
			//Recibo el FIN_QUANTUM que se envia siempre
			recv(fd,&mensaje,sizeof(int),0);

			pcb_recibido = pasarAPuntero( recibirPcb(fd,true,puntero_inutil) );
			queue_push(ready,pcb_recibido);
		}

		//Elimino la cpu de las listas correspondientes
		bool igualFd(void *elemento){
			if(((t_cpu*)elemento)->socket == fd)
				return true;
			else return false;
		}
		t_cpu *cpu_a_eliminar = list_remove_by_condition(listaCpu,igualFd);
		free(cpu_a_eliminar->pcb);
		free(cpu_a_eliminar);
		close(fd);
		FD_CLR(fd,listen);

		break;
	default:
		printf("mensaje recibido CPU Erroneo cod: %d",codigoMensaje);
	}
}

void procesarFinQuantum(int fd)
{
	t_pcb *pcb_recibido;
	int *puntero_inutil;
	int mensaje;

	//Termino el quantum, recibo el pcb y lo pongo al final de la cola de listos
	pcb_recibido = pasarAPuntero( recibirPcb(fd,true,puntero_inutil) );

	//checkeo que no haya cambios importantes
	bool mismoFd(void *elemento){
		if( ((t_cpu*)elemento)->socket == fd)
			return true;
		else return false;
	}
	t_cpu *cpu = list_find(listaCpu,mismoFd);

	if(cpu->finForzoso){
		//Paso el pid a finished
		int *pid = malloc(sizeof(int));
		*pid = pcb_recibido->pid;
		queue_push(finished,pid);

		//Elimino el pcb y no lo paso a ready
		cpu->libre = true;
		freePcb(pcb_recibido);
		cpu->finForzoso = false;
		return;
	}

	if(cpu->cambioQuantumSleep){
		mensaje = CAMBIO_QUANTUM_SLEEP;
		send(fd,&mensaje,sizeof(int),0);
		send(fd,&quantum_sleep,sizeof(int),0);
		cpu->cambioQuantumSleep = false;
	}

	queue_push(ready,pcb_recibido);

	//Tengo que poner la cpu como libre
	liberarCpu(fd);
}

void procesarEntradaSalida(int fd)
{
	//Agrego el pcb a la cola de la entrada salida que me piden.
	int tiempo;
	int tamanio_cadena;
	char *identificador;
	t_pcb *pcb_actualizado;

	recv(fd,&tiempo,sizeof(int),0);
	recv(fd,&tamanio_cadena,sizeof(int),0);

	identificador = malloc(tamanio_cadena);
	recvAll(fd,identificador,tamanio_cadena,0);

	//Busco la entradaSalida y agrego el nodo
	bool mismoId(void *elemento)
	{
		if(!strcmp(identificador, ((t_IO*)elemento)->identificador)){
			return true;
		}else return false;
	}
	t_IO *io = list_find(IO,mismoId);

	int mensaje;
	if(io == NULL)
	{
		printf("No existe la entrada salida: ");
		fwrite(identificador,sizeof(char),tamanio_cadena,stdout);
		printf("\n");

		free(identificador);

		mensaje = KILL_PROGRAMA;
		send(fd,&mensaje,sizeof(int),0);

		return;
	}

	//El identificador existe
	mensaje = OP_OK;
	send(fd,&mensaje,sizeof(int),0);

	//Ya tengo todos los datos, ahora tengo que recibir el pcb.
	//Recibo el mensaje inutil
	int msj_inutil;
	recv(fd,&msj_inutil,sizeof(int),0);
	if(msj_inutil == FIN_QUANTUM)printf("Recibi mensaje inutil.\n");
	else printf("QUILOMBOOOOOOOOO.\n");

	//recibo el pcb
	int *ptr_inutil;
	pcb_actualizado = pasarAPuntero( recibirPcb(fd,true,ptr_inutil) );

	//Creo nodo de la lista de entradaSalida
	t_proceso_esperando *proceso = malloc(sizeof(t_proceso_esperando));
	proceso->pcb = pcb_actualizado;
	proceso->tiempo_espera = tiempo;
	proceso->tiempo_inicio = time(NULL);

	queue_push(io->procesos_esperando,proceso);

	//Libero la cpu
	liberarCpu(fd);

	free(identificador);
}

void obtenerValorCompartido(int fd)
{
	int pid;
	int tamanio_cadena;

	recv(fd,&pid,sizeof(int),0);
	recv(fd,&tamanio_cadena,sizeof(int),0);

	printf("Datos obtener valor compartido:\npid: %d tamaño: %d.\n",pid,tamanio_cadena);

	char *identificador = malloc(tamanio_cadena);

	recv(fd,identificador,tamanio_cadena,0);

	bool mismoId(void *elemento)
	{
		if(!strcmp(identificador, ((t_SHARED*)elemento)->identificador)){
			return true;
		}else return false;
	}
	t_SHARED *shared = list_find(SHARED,mismoId);

	printf("SHARED ID: %s.\n",identificador);

	int mensaje;
	if(shared == NULL)
	{
		printf("No existe la variable compartida: ");
		fwrite(identificador,sizeof(char),tamanio_cadena,stdout);
		printf(".\n");

		free(identificador);

		mensaje = KILL_PROGRAMA;
		send(fd,&mensaje,sizeof(int),0);

		return;
	}

	mensaje = OP_OK;
	send(fd,&mensaje,sizeof(int),0);

	int valor = shared->valor;
	send(fd,&valor,sizeof(int),0);

	free(identificador);
}

void asignarValorCompartido(int fd)
{
	int pid;
	int tamanio_cadena;
	int valor;

	recv(fd,&pid,sizeof(int),0);
	recv(fd,&valor,sizeof(int),0);
	recv(fd,&tamanio_cadena,sizeof(int),0);

	char *identificador = malloc(tamanio_cadena);

	recv(fd,identificador,tamanio_cadena,0);

	bool mismoId(void *elemento)
	{
		if(!strcmp(identificador, ((t_SHARED*)elemento)->identificador)){
			return true;
		}else return false;
	}
	t_SHARED *shared = list_find(SHARED,mismoId);

	int mensaje;
	if(shared == NULL)
	{
		printf("No existe la variable compartida: ");
		fwrite(identificador,sizeof(char),tamanio_cadena,stdout);
		printf("\n");

		free(identificador);

		mensaje = KILL_PROGRAMA;
		send(fd,&mensaje,sizeof(int),0);

		return;
	}

	mensaje = OP_OK;
	send(fd,&mensaje,sizeof(int),0);

	shared->valor = valor;

	free(identificador);
}

void ansisopWait(int fd)
{
	int pid;
	int tamanio_cadena;

	recv(fd,&pid,sizeof(int),0);
	recv(fd,&tamanio_cadena,sizeof(int),0);

	char *identificador = malloc(tamanio_cadena);

	recv(fd,identificador,tamanio_cadena,0);

	printf("Datos de la solucitud wait:\npid: %d tamaño: %d identificador: %s.\n",pid,tamanio_cadena,identificador);

	bool mismoId(void *elemento)
	{
		if(!strcmp(identificador, ((t_SEM*)elemento)->identificador)){
			return true;
		}else return false;
	}
	t_SEM *sem = list_find(SEM,mismoId);

	int mensaje;
	if(sem == NULL)
	{
		printf("No existe el semaforo: ");
		fwrite(identificador,sizeof(char),tamanio_cadena,stdout);
		printf(".\n");

		printf("tamaño cadena: %d.\n",tamanio_cadena);

		free(identificador);

		mensaje = KILL_PROGRAMA;
		send(fd,&mensaje,sizeof(int),0);

		return;
	}

	printf("identificador OK.\n");
	mensaje = OP_OK;
	send(fd,&mensaje,sizeof(int),0);

	//Envio mensaje de si puede seguir ejecutando o no
	if(sem->valor > 0)
		mensaje = ANSISOP_PODES_SEGUIR;
	else mensaje = ANSISOP_BLOQUEADO;

	send(fd,&mensaje,sizeof(int),0);

	if(sem->valor <= 0)
	{//Agrego el proceso a la cola del semaforo
		printf("Bloqueado. Recibo pcb.\n");

		//Recibo el FIN_QUANTUM
		recv(fd,&pid,sizeof(int),0);

		int *puntero_inutil;
		t_pcb *pcb = pasarAPuntero( recibirPcb(fd,true,puntero_inutil) );

		queue_push(sem->procesos_esperando,pcb);

		liberarCpu(fd);
	}else printf("Puede seguir ejecutando.\n");

	sem->valor--;

	printf("Valor semaforo: %d.\n",sem->valor);

}

void ansisopSignal(int fd)
{
	int pid;
	int tamanio_cadena;

	recv(fd,&pid,sizeof(int),0);
	recv(fd,&tamanio_cadena,sizeof(int),0);

	char *identificador = malloc(tamanio_cadena);

	recv(fd,identificador,tamanio_cadena,0);

	bool mismoId(void *elemento)
	{
		if(!strcmp(identificador, ((t_SEM*)elemento)->identificador)){
			return true;
		}else return false;
	}
	t_SEM *sem = list_find(SEM,mismoId);

	int mensaje;
	if(sem == NULL)
	{
		printf("No existe el semaforo: ");
		fwrite(identificador,sizeof(char),tamanio_cadena,stdout);
		printf("\n");

		free(identificador);

		mensaje = KILL_PROGRAMA;
		send(fd,&mensaje,sizeof(int),0);

		return;
	}

	mensaje = OP_OK;
	send(fd,&mensaje,sizeof(int),0);

	sem->valor++;
	printf("Incremento el semaforo.\n");
	printf("Nuevo valor: %d.\n",sem->valor);

/*	if(sem->valor >= 0)
	{//Paso el primer pcb a ready
		printf("Libero un proceso.\n");

		if(!queue_is_empty(sem->procesos_esperando))
		{
			queue_push(ready, queue_pop(sem->procesos_esperando));
		}
	}*/
	//Libero siempre un proceso.
	printf("Libero un proceso.\n");

	if(!queue_is_empty(sem->procesos_esperando))
	{
		queue_push(ready, queue_pop(sem->procesos_esperando));
	}
}

void imprimirValor(int fd)
{
	//Recibo pid y el valor en si.
	int pid;
	int valor;

	recv(fd,&pid,sizeof(int),0);
	recv(fd,&valor,sizeof(int),0);

	int consolaFd = buscarConsolaFd(pid);

	int mensaje[2] = {IMPRIMIR_VALOR,valor};

	if(consolaFd != -1)
		send(consolaFd,&mensaje,2*sizeof(int),0);
	else printf("La consola ya se cerro.\n");

	printf("Valor a imprimir: %d.\n",valor);
}

void imprimirTexto(int fd)
{
	int pid;
	int tamanio_cadena;

	//Recibo el string de cpu
	recv(fd,&pid,sizeof(int),0);
	recv(fd,&tamanio_cadena,sizeof(int),0);

	char *cadena = malloc(tamanio_cadena);
	recv(fd,cadena,tamanio_cadena,0);

	//Envio a consola la order de impresion

	int mensaje = IMPRIMIR_CADENA;
	char *buffer = malloc(tamanio_cadena + 2*sizeof(int));

	memcpy(buffer,&mensaje,sizeof(int));
	memcpy(buffer + sizeof(int),&tamanio_cadena,sizeof(int));
	memcpy(buffer + 2*sizeof(int),cadena,tamanio_cadena);

	int consolaFd = buscarConsolaFd(pid);

	if(consolaFd != -1)
		send(consolaFd, buffer, tamanio_cadena + 2*sizeof(int), 0);
	else printf("La consola ya se cerro.\n");

	free(buffer);
	free(cadena);
}

int buscarConsolaFd(int pid)
{
	bool igualPid(void *elemento){
		if( ((t_consola*)elemento)->pid == pid)
			return true;
		else return false;
	}

	t_consola *consola = list_find(listaConsola,igualPid);

	if(consola == NULL)
		return -1;
	else return consola->socketConsola;
}

int obtenerSocketConsola(int socketCPU){

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

void eliminarPcb(int pid)
{
	bool igualPid(void *elemento){
		return ((t_pcb*)elemento)->pid == pid;
	}

	t_pcb *pcb_a_eliminar = list_find(new,igualPid);

	freePcb(pcb_a_eliminar);
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
	if( solicitarPaginasUMC(source_size, source, pid) == -1 )
	{//Hubo error le aviso a la consola que finalice
		int mensaje = FINALIZAR;
		send(socket,&mensaje,sizeof(int),0);

		//Elimino la consola
		bool igualFd(void *elemento){
			if( ((t_consola*)elemento)->socketConsola == socket)
				return true;
			else return false;
		}
		list_remove_and_destroy_by_condition(listaConsola,igualFd,free);
	}

	//Libero la memoria
	free(buffer);
}


int crearPCB(int source_size,char *source){

	t_pcb *pcb = malloc(sizeof(t_pcb));
	t_indice_codigo indiceCodigo;
	t_indice_etiquetas indiceEtiquetas;

	int cant_paginas = (source_size / pag_size) + 1;
	if(source_size % pag_size == 0)
		cant_paginas--;

	//Cargo camposPCB
	max_pid++;
	pcb->pid = max_pid;
	pcb->cant_pag_cod = cant_paginas;
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

	printf("Imprimo el indice de etiquetas:\n\n");
	fwrite(indiceEtiquetas.etiquetas,sizeof(char),indiceEtiquetas.etiquetas_size,stdout);
	printf("\n\n");

	//Creo la entrada del stack del main
	pcb->stack.cant_entradas = 1;
	pcb->stack.entradas = malloc(sizeof(t_entrada_stack));
	pcb->stack.entradas->cant_arg = 0;
	pcb->stack.entradas->cant_var = 0;
	pcb->stack.entradas->dirRetorno = -1;
	pcb->stack.entradas->offsetRet = -1;
	pcb->stack.entradas->pagRet = -1;
	pcb->stack.entradas->argumentos = NULL;
	pcb->stack.entradas->variables = NULL;

	//Cargo indices en PCB
	pcb->indice_cod = indiceCodigo;
	pcb->indice_etiquetas = indiceEtiquetas;

	pcb->tamanio = tamanioPcb(*pcb);

	//Cargo en el pcb la primera instruccion
	pcb->PC = metadata->instruccion_inicio;

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
	if(source_size % pag_size == 0)
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
		//Saco el pcb de new
		int i;
		for(i=0;i<list_size(new);i++){
			if(((t_pcb*)list_get(new,i))->pid == pid)
				list_remove_and_destroy_element(new,i,freePcb);
		}

		return -1;
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

void limpiarTerminados(fd_set *listen){

	int *pid;

	while(!queue_is_empty(finished))
	{
		printf("Hay algo que limpiar.\n");

		pid = queue_pop(finished);

		int mensaje = FINALIZAR;

		//Aviso a consola que termino la ejecucion
		bool igualPid(void *elemento)
		{
			return ((t_consola*)elemento)->pid == *pid;
		}
		t_consola *consola = list_find(listaConsola,igualPid);

		if(consola == NULL)
			printf("Consola ya eliminada.\n");
		else{
			send(consola->socketConsola,&mensaje,sizeof(int),0);

			//Elimino la consola y cierro el socket
			list_remove_by_condition(listaConsola,(void*)igualPid);
			FD_CLR(consola->socketConsola,listen);
			close(consola->socketConsola);
			free(consola);
		}

		//Aviso a umc que termino la ejecucion
		int msjLargo[2] = {FINALIZAR_PROGRAMA,*pid};

		send(socket_umc,&msjLargo,2*sizeof(int),0);

		//libero la memoria del pid
		free(pid);

		printf("Termine de limpiar.\n");
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

				if(pcb_a_ejecutar == NULL)
					break;

				getListaCpu(i)->libre = false;
				getListaCpu(i)->pid = pcb_a_ejecutar->pid;

				enviarPcb(*pcb_a_ejecutar, getListaCpu(i)->socket, quantum);

				getListaCpu(i)->pcb = pcb_a_ejecutar;
			}
		}
	}
}

int cantCpuLibres()
{
	if(list_is_empty(listaCpu))
		return 0;

	int cant = 0;
	int i;
	for(i=0; i<list_size(listaCpu); i++)
	{
		if(getListaCpu(i)->libre)
			cant++;
	}

	return cant;
}

void liberarCpu(int fd)
{
	bool mismoFd(void *elemento){
		if(((t_cpu*)elemento)->socket == fd)
			return true;
		else return false;
	}
	t_cpu *cpu = list_find(listaCpu,mismoFd);

	cpu->libre = true;
	freePcb(cpu->pcb);
	cpu->pcb = NULL;
}

void eliminarDeCola(t_queue *cola, int pid)
{
	if(queue_size(cola) == 0)
		return;

	if(queue_size(cola) == 1)
	{
		t_pcb *pcb = queue_peek(cola);
		if(pcb->pid == pid)
		{
			queue_pop(cola);
			freePcb(pcb);
		}
		return;
	}

	//La cola tiene mas de 1 elemento
	t_queue *nueva_cola = queue_create();
	t_list *lista = list_create();

	while(queue_size(cola) > 0)
	{//Paso lo que esta en cola a la lista
		t_pcb *pcb = queue_pop(cola);

		if(pcb->pid != pid)
			list_add(lista,pcb);
	}
	queue_destroy(cola);//Ya no tiene nada esta cola

	//Reinserto los elementos en la cola
	int i;
	for(i=list_size(lista)-1;i>=0;i--)
	{
		queue_push(nueva_cola,list_remove(lista,i));
	}

	cola = nueva_cola;
	list_destroy(lista);
}

void eliminarDeIO(t_queue *cola, int pid)
{
	if(queue_size(cola) == 0)
		return;

	if(queue_size(cola) == 1)
	{
		t_proceso_esperando *proceso = queue_peek(cola);
		if(proceso->pcb->pid == pid)
		{
			queue_pop(cola);
			free(pcb);
		}
	}

	//La cola tiene mas de 1 elemento
	t_queue *nueva_cola = queue_create();
	t_list *lista = list_create();

	while(queue_size(cola) > 0)
	{//Paso lo que esta en cola a la lista
		t_proceso_esperando *proceso = queue_pop(cola);

		if(proceso->pcb->pid == pid)
			list_add(lista,queue_pop(cola));
	}
	queue_destroy(cola);///Ya no tiene nada esta cola

	//Reinserto los elementos en la cola
	int i;
	for(i=list_size(lista)-1;i>=0;i--)
	{
		queue_push(nueva_cola,list_remove(lista,i));
	}

	cola = nueva_cola;
	list_destroy(lista);
}
