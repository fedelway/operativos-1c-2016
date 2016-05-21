#ifndef UMC_H_
#define UMC_H_

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

#include "socketCliente.h"
#include "socketServidor.h"



void crearConfiguracion(char* config_path);
bool validarParametrosDeConfiguracion();
void recibirMensajeCPU(char* message, int socket_CPU);
void enviarPaqueteASwap(char* message, int socket);
int framesDisponibles();
void recibirConexiones(int cpu_fd, int max_fd);
void aceptarNucleo();
void trabajarNucleo();
void trabajarCpu();
void inicializarMemoria();
void inicializarPrograma();
void escribirEnMemoria(char *src, int size, t_prog programa);


#endif /* UMC_H_ */
