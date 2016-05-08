#include "socketCliente.h"


#define PACKAGESIZE 1024 //Define cual es el tamaño maximo del paquete a enviar


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

	if (socket_conexion ==-1){
			printf("Error al crear el socket \n");
		}else{
			if(connect(socket_conexion, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1){
				printf("Fallo al conectar\n");
				exit(1);
			}else{
				printf("Socket cliente conectado\n");
			}
		}
		freeaddrinfo(serverInfo);
		return socket_conexion;

}

int socketEnviarMensaje(int socket_conexion, char * mensaje, int longitud_mensaje){
	int cantbytes;
	if ((cantbytes = send(socket_conexion,mensaje,longitud_mensaje, 0)) <= 0){
		printf("Error al enviar mensaje");

		if (cantbytes == 0) {
			printf("Socket servidor desconectado\n");
		} else {
			printf("Error envio socket servidor");
		}
	}
	return cantbytes;
}

int socketRecibirMensaje(int socket_conexion, char * mensaje, int longitud_mensaje){
	int cantbytes;
	if ((cantbytes = recv(socket_conexion , mensaje, longitud_mensaje, 0)) <= 0){

		printf("Error al recibir mensaje");
		if (cantbytes == 0) {
			printf("Socket servidor desconectado\n");
		} else {
			printf("Error recepcion socket servidor");
		}
	}
	return cantbytes;
}

int cerrarConexionSocket(int socket){
	if (close(socket) ==-1){
			printf("Error al cerrar el socket servidor");
	}
	return 0;
}
