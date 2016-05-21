/*
 * nucleo.h
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */

#ifndef NUCLEO_H_
#define NUCLEO_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h> //Para la estructura timeval del select
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // for close conections
#include "commons/config.h"
#include "commons/collections/queue.h"
#include "commons/collections/list.h"

void crearConfiguracion(); //creo la configuracion y checkeo que sea valida
bool validarParametrosDeConfiguracion();
int conectarPuertoEscucha(char *puerto, fd_set *setEscucha, int *max_fd);
void trabajarConexiones(fd_set *listen, int *max_fd, int cpu_fd, int prog_fd);
void procesoMensajeRecibidoConsola(char* paquete, int socket);
void hacerAlgoCPU(int codigoMensaje, int fd);
void hacerAlgoProg(int codigoMensaje, int fd);
void hacerAlgoUmc(int codigoMensaje);
void agregarConexion(int fd, int *max_fd, fd_set *listen, fd_set *particular, int msj);
void agregarConsola(int fd, int *max_fd, fd_set *listen, fd_set *consolas);
void agregarCpu(int fd, int *max_fd, fd_set *listen, fd_set *cpus);
void enviarPaqueteACPU(char* package, int socket);
void iniciarNuevaConsola(int fd);
void conectarUmc();
void limpiarTerminados();
void planificar();
void moverDeNewA(int pid, t_queue *destino);

#endif /* NUCLEO_H_ */
