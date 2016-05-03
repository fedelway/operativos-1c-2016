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
#include <sys/stat.h>
#include <fcntl.h> //Para manejar archivos con fd, no se si lo vayamos a usar, pero lo use y lo cambie
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
FILE *abrirSource(char *path, int *source_size);
void enviar_source(int nucleo_fd, FILE *source, int source_size);

int main(int argc,char *argv[]) {

	char* nucleo_ip;
	char* nucleo_puerto;
	int source_size;
	int nucleo_fd;
	FILE *source;

	if(argc != 3){
		fprintf(stderr,"uso: consola config_path ansisop_source_path\n");
		return 1;
	}

	source = abrirSource(argv[2], &source_size);

	crearConfiguracion(argv[1]);

	nucleo_ip = config_get_string_value(config,"NUCLEO_IP");
	nucleo_puerto = config_get_string_value(config,"NUCLEO_PUERTO");

	nucleo_fd = conectarseA(nucleo_ip, nucleo_puerto);

	enviar_source(nucleo_fd, source, source_size);

	return EXIT_SUCCESS;
}

void enviar_source(int nucleo_fd, FILE *source, int source_size){

	char *archivo = malloc(source_size + 1);
	int cant_leida;

	cant_leida = fread(archivo, sizeof(char), source_size, source);

	while(cant_leida < source_size){

		fread(archivo + cant_leida, sizeof(char), source_size - cant_leida, source);
	}
}

FILE *abrirSource(char *path, int *source_size){

	FILE *source;
	struct stat file_info;

	source = fopen(path, "r");

	if (source == NULL){
		printf("Error al abrir el archivo fuente.\n");
		exit(0);
	}

	if( stat(path, &file_info) == -1){
		printf("Error con stat.\n");
	}

	//Me fijo el tamanio del archivo en bytes.
	*source_size = file_info.st_size;
	printf("Tamanio archivo(bytes): %i\n", *source_size);

	return source;
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

// Comento esto, ya no creo que lo vayamos a usar mas que para probar.
//	int enviar = 1;
//	char message[PACKAGESIZE];

	printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");

//	while(enviar){
//		fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
//		if (!strcmp(message,"exit\n")) enviar = 0;			// Chequeo que el usuario no quiera salir
//		if (enviar) send(socket_conexion, message, strlen(message) + 1, 0); 	// Solo envio si el usuario no quiere salir.
//	}
//	close(socket_conexion);

	return socket_conexion;
}

void validarNucleo(int nucleo_fd){

	int msj_recibido;
	int soy_consola = 2000;

	recv(nucleo_fd, &msj_recibido, sizeof(int), 0);

	if(msj_recibido == 1000){
		printf("Nucleo validado.\n");
		send(nucleo_fd, &soy_consola, sizeof(int), 0);
	}else{
		printf("El nucleo no pudo ser validado.\n");
		exit(0);
	}

}
