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

	if(argc != 2){
		fprintf(stderr,"uso: umc config_path\n");
		return 1;
	}

	crearConfiguracion(argv[1]);

	swap_ip = config_get_string_value(config,"IP_SWAP");
	swap_puerto = config_get_string_value(config,"PUERTO_SWAP");

 	//swap_fd = conectarseA(swap_ip, swap_puerto);

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

	aceptarNucleo();

	//Ciclo principal
	recibirConexiones(socket_CPU, max_fd);

/*	char message[PACKAGESIZE];
	recibirMensajeCPU(&message, socket_CPU);

	enviarPaqueteASwap(message, swap_fd);
*/

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

					pthread_create(&thread, &atributos, (void *)trabajarCpu, NULL);

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
		&&  config_has_property(config, "PUERTO_NUCLEO");
}

/*int conectarseA(char* ip, char* puerto){

    struct addrinfo hints, *serverInfo;
	int result_getaddrinfo;
	int socket_conexion;

	memset(&hints, '\0', sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	result_getaddrinfo = getaddrinfo(ip, puerto, &hints, &serverInfo);

	if(result_getaddrinfo != 0){
		fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(result_getaddrinfo));
		return 2;
	}

	socket_conexion = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if( connect(socket_conexion, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1){
		printf("Fallo al conectar\n");
		exit(1);
	}

	int status =1;
	int enviar = 1;
	char message[PACKAGESIZE];

	printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");

	while(enviar && status !=0){
	 	fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
		if (!strcmp(message,"exit\n")) enviar = 0;			// Chequeo que el usuario no quiera salir
		if (enviar) send(socket_conexion, message, strlen(message) + 1, 0); 	// Solo envio si el usuario no quiere salir.

		//Recibo mensajes del servidor
		memset (message,'\0',PACKAGESIZE); //Lleno de '\0' el package, para que no me muestre basura
		status = recv(socket_conexion, (void*) message, PACKAGESIZE, 0);
		if (status != 0) printf("%s", message);

	}
	close(socket_conexion);

	return 0;
}*/

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

void handshakeServidor(int socket_swap){

		//Estructura para crear el header + piload
		typedef struct{
			int id;
			int tamanio;
		}t_header;

		typedef struct{
		  t_header header;
		  char* paiload;
		}package;


		//seteo el mensaje que le envío a la swap para que ésta reciba el header.id=2 y haga desde su proceso el handshake
		package mensajeAEnviar;
		mensajeAEnviar.header.id = 5020 ;
		mensajeAEnviar.header.tamanio = 0;
		mensajeAEnviar.paiload = "\0";

		package mensajeARecibir;
		mensajeARecibir.header.id = 4020;
		mensajeARecibir.header.tamanio = 0;
		mensajeARecibir.paiload = "\0";

		recv(socket_swap, &mensajeARecibir, sizeof(mensajeARecibir) , 0);

		switch(mensajeARecibir.header.id){

		 case 1000:
			 // Procesa ok
			 printf("OK\n");
		 break;

		 case 4020:
			 //Handshake servidor
			 printf("No hace nada porque está reservado para el servidor\n");
	     break;

		 case 5020:
			 //Envio de Handshake Cliente
			printf("Conectado a swap.\n");
			memset(mensajeAEnviar.paiload,'\0', mensajeAEnviar.header.tamanio); //Lleno de '\0' el package, para que no me muestre basura
			recv(socket_swap, mensajeARecibir.paiload, mensajeARecibir.header.tamanio , 0);

			send(socket_swap, &mensajeAEnviar, sizeof(mensajeAEnviar), 0);
		 break;

		 default:
			 printf("No se admite la conexión con éste socket");
		 break;
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

	int soy_umc = 4000;
	int msj_recibido;
	struct sockaddr_in addr; // Para recibir nuevas conexiones
	socklen_t addrlen = sizeof(addr);

	printf("Esperando conexion del nucleo.\n");

	nucleo_fd = accept(nucleo_fd, (struct sockaddr *) &addr, &addrlen);

	printf("Se ha conectado el nucleo.\n");

	//Armo paquete
	int buffer[2];
	buffer[0] = soy_umc;
	buffer[1] = frame_size;

	send(nucleo_fd, &buffer, 2*sizeof(int),0); //Envio codMensaje y tamaño de pagina

	recv(nucleo_fd, &msj_recibido, sizeof(int), 0);
	//Verifico que se haya conectado el nucleo
	if (msj_recibido == 1000){
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

		case 1010:
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

	if(cant_paginas_cod > fpp){
		//Rechazo la solicitud. El codigo ocupa mas lugar que el limite para cada programa
		terminarPrograma(pid);

		//Libero el archivo
		free(buffer);
		free(source);
		return;
	}

	//Creo las paginas que a las que va a poder acceder el programa(tabla de paginas)
	t_pag *paginas;

	paginas = malloc(sizeof(t_pag) * fpp);
	for(aux=0; aux < fpp; aux++){
		paginas[aux].presencia = false;
		paginas[aux].modificado = false;
	}

	t_prog *programa;

	programa = malloc(sizeof(t_prog));
	programa->pid= pid;
	programa->paginas = paginas;
	programa->pos = 0;

	list_add(programas, programa);

	//Le mando a swap el archivo para que lo guarde
	if( enviarCodigoASwap(source,source_size) == -1 ){
		//Error en envio de archivo, aviso a nucleo que termine el programa.
		terminarPrograma(pid);
	}

	//Libero la memoria
	free(programa);
	free(buffer);
	free(source);
}

int enviarCodigoASwap(char *source, int source_size){

	int cant_enviada_total = 0;
	int cant_enviada_parcial;
	int mensaje = 4030;
	int aux;
	char *buffer;

	buffer = malloc(sizeof(int) + frame_size);

	if(buffer == NULL){
		printf("Error de malloc.\n");
	}

	//Armo el paquete
	memcpy(buffer,&mensaje,sizeof(int));

	while(cant_enviada_total < source_size){
		cant_enviada_parcial = 0;

		memcpy(buffer+sizeof(int),source,frame_size);

		//Ya tengo el paquete armado, lo envio
		while(cant_enviada_parcial < frame_size){
			aux = send(swap_fd,buffer,sizeof(int) + frame_size,0);

			if(aux <= 0){
				printf("Error en el envio del archivo");
				return -1;//Error de swap, le digo a nucleo que mande el proceso a finished
			}

			cant_enviada_parcial += aux;
		}
		//Ya envie una pagina, la cant enviada total es la anterior + una pagina
		cant_enviada_total += frame_size;
	}

	//Codigo enviado correctamente, devuelvo 0
	return 0;
}

void terminarPrograma(int pid){
	int bufferInt[2];

	bufferInt[0] = 4010;
	bufferInt[1] = pid;
	send(nucleo_fd, &bufferInt,2*sizeof(int),0);
}

void escribirEnMemoria(char* src, int size, t_prog programa){

	int cant_escrita = 0;

	while(cant_escrita < size){

	}
}

void trabajarCpu(){

}
