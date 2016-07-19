/*
 * sockets.c
 *
 *  Created on: 18/7/2016
 *      Author: utnso
 */

#include "sockets.h"

int sendAll(int fd, char *cosa, int size, int flags){

	int cant_enviada = 0;
	int aux;

	while(cant_enviada < size){

		aux = send(fd, cosa + cant_enviada, size - cant_enviada, flags);

		if(aux == -1){
			return -1;
		}
		cant_enviada += aux;
	}

	//Ya envie correctamente
	return cant_enviada;
}

int recvAll(int fd, char *buffer, int size, int flags){

	int cant_recibida = 0;
	int aux;

	while(cant_recibida < size){

		aux = recv(fd, buffer + cant_recibida, size - cant_recibida, flags);

		if(aux == -1){
			return -1;
		}
		cant_recibida += aux;
	}

	return cant_recibida;
}
