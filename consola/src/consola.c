/*
 ============================================================================
 Name        : consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // for close conection
#include "commons/config.h"

t_config* config;

#define PACKAGESIZE 1024 //Define cual es el tamaño maximo del paquete a enviar

void crearConfiguracion(char* config_path);
bool validarParametrosDeConfiguracion();
int conectarseA(char* ip, char* puerto);
void validarNucleo(int nucleo_fd);

int main(int argc,char *argv[]) {

	char* nucleo_ip;
	char* nucleo_puerto;

	if(argc != 2){
		fprintf(stderr,"uso: consola config_path\n");
		return 1;
	}

	crearConfiguracion(argv[1]);

	nucleo_ip = config_get_string_value(config,"NUCLEO_IP");
	nucleo_puerto = config_get_string_value(config,"NUCLEO_PUERTO");

	conectarseA(nucleo_ip, nucleo_puerto);

	return EXIT_SUCCESS;
}

void crearConfiguracion(char* config_path){

	config = config_create(config_path);

	if(validarParametrosDeConfiguracion()){
		printf("El archivo de configuracion tiene todos los parametros requeridos.\n\n");
	}else{
		printf("Configuracion no valida");
		exit(EXIT_SUCCESS);
	}
}

bool validarParametrosDeConfiguracion(){
	return 	config_has_property(config, "PROGRAMA_ANSISOP")
		&& 	config_has_property(config, "NUCLEO_IP")
		&&	config_has_property(config, "NUCLEO_PUERTO");
}

int conectarseA(char* ip, char* puerto){

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

	validarNucleo(socket_conexion);

	int enviar = 1;
	char message[PACKAGESIZE];

	printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");

	while(enviar){
		fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
		if (!strcmp(message,"exit\n")) enviar = 0;			// Chequeo que el usuario no quiera salir
		if (enviar) send(socket_conexion, message, strlen(message) + 1, 0); 	// Solo envio si el usuario no quiere salir.
	}

	close(socket_conexion);

	return 0;
}

void validarNucleo(int nucleo_fd){

	int msj_recibido;
	int soy_consola = 2000;

	recv(nucleo_fd, &msj_recibido, 1, 0);

	if(msj_recibido == 1000){
		printf("Nucleo validado.\n");
		send(nucleo_fd, &soy_consola, 1, 0);
	}else{
		printf("El nucleo no pudo ser validado");
		exit(0);
	}

}
