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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "commons/config.h"

t_config config;

void crearConfiguracion(char* config_path);
bool validarParametrosDeConfiguracion();
int conectarseA(int ip, int puerto);

int main(int argc,char *argv[]) {

	if(argc != 2){
		fprintf(stderr,"uso: consola config_path\n");
		return 1;
	}

	crearConfiguracion(argv[1]);

	conectarseA(config_get_int_value(config,"NUCLEO_IP"), config_get_int_value(config,"NUCLEO_PUERTO"));

	return EXIT_SUCCESS;
}

void crearConfiguracion(char* config_path){

	config = *create_config(config_path);

	if(validarParametrosDeConfiguracion()){
		printf("El archivo de configuracion tiene todos los parametros requeridos.\n\n");
	}else{
		printf("configuracion no valida");
		exit(EXIT_SUCCESS);
	}
}

bool validarParametrosDeConfiguracion(){
	return 	config_has_property(config, "PROGRAMA_ANSISOP")
		&& 	config_has_property(config, "NUCLEO_IP")
		&&	config_has_property(config, "PUERTO_NUCLEO");
}

int conectarseA(int ip, int puerto){
	struct addrinfo hints, *serverInfo;
	int result_getaddrinfo;
	int socket_conexion;

	memset(hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	result_getaddrinfo = getaddrinfo(ip, puerto, &hints, &serverInfo);

	if(result_getaddrinfo != 0){
		fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(result_getaddrinfo));
		return 2;
	}

	socket_conexion = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if( connect(socket_conexion, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1){
		printf("Fallo al conectar/n");
		exit(1);
	}

	return socket_conexion;
}
