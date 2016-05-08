#ifndef SOCKETCLIENTE_H_
#define SOCKETCLIENTE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <commons/config.h>


t_config *config;

int conectarseA(char* ip, char* puerto);
int socketRecibirMensaje(int socket_conexion, char * mensaje, int longitud_mensaje);
int socketEnviarMensaje(int socket_conexion, char * mensaje, int longitud_mensaje);
int cerrarConexionSocket(int socket);

#endif /* SOCKETCLIENTE_H_ */
