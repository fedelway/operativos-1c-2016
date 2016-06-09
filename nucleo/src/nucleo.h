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
#include "socketCliente.h"
#include "socketServidor.h"
#include "protocolo.h"
#include "parser/parser.h"
#include "parser/metadata_program.h"


typedef struct{
	int byte_inicio[10];
	int long_instruccion[10];
}t_indice_codigo;

typedef struct{
	char *etiquetas;
	t_size etiquetas_size;
}t_indice_etiquetas;

typedef struct{
	int argumentos;
	int variables;
	int dirRetorno;
	int posVariable;
}t_indice_stack;

typedef struct{
	int pid;
	int PC;			//program counter
	int cant_pag;
	t_indice_codigo indice_cod;
	t_indice_etiquetas indice_etiquetas;
	t_indice_stack indice_stack;
}t_pcb;


typedef struct{
	int num;
	int fd;
	bool libre;
}t_cpu;

typedef struct{
	int pcb;
	int socketConsola;
}t_consola;

//Var globales
t_config* config;
int umc_fd;
int pag_size;
int max_pid = 0;
int max_fd = 0;	 //El maximo valor de file descriptor
int stack_size;
int max_cpu;
int quantum;
int cant_cpus;
t_cpu *listaCpu;


//agregamos sockets provisorios para cpu y consola
int socket_consola = 0;
int socket_CPU = 0;
int socket_umc;
int socket_escucha;
int cpu_fd, cons_fd;

//Las colas con los dif estados de los pcb
t_queue ready, running, blocked, finished;
t_list new;


void crearConfiguracion(); //creo la configuracion y checkeo que sea valida
bool validarParametrosDeConfiguracion();
void maximoFileDescriptor(int socket_escucha,int *max_fd);
void trabajarConexiones(fd_set *listen, int *max_fd, int cpu_fd, int prog_fd);
void trabajarConexionesSockets(fd_set *listen, int *max_fd, int cpu_fd, int cons_fd);
void procesoMensajeRecibidoConsola(char* paquete, int socket);
void hacerAlgoCPU(int codigoMensaje, int fd);
void hacerAlgoConsola(int codigoMensaje, int fd);
void agregarConexion(int fd, int *max_fd, fd_set *listen, fd_set *particular, int msj);
void agregarConsola(int fd, int *max_fd, fd_set *listen, fd_set *consolas);
int recibirMensaje(int socket);

void enviarPaqueteACPU(char* package, int socket);
void iniciarNuevaConsola(int fd);
void validacionUMC();
void hacerAlgoUmc(int codigoMensaje);
void agregarConexion(int fd, int *max_fd, fd_set *listen, fd_set *particular, int msj);
void agregarConsola(int fd, int *max_fd, fd_set *listen, fd_set *consolas);
void agregarCpu(int fd, int *max_fd, fd_set *listen, fd_set *cpus);
void enviarPaqueteACPU(char* package, int socket);
void limpiarTerminados();
void planificar();
void moverDeNewA(int pid, t_queue *destino);

void crearPCB(int source_size,char *source);
void solicitarPaginasUMC(int source_size, char *buffer, char *source);

#endif /* NUCLEO_H_ */
