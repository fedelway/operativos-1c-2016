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
#include <pthread.h>

#define getListaCpu(i) ((t_cpu*)list_get(listaCpu,i))
#define getListaConsola(i) ((t_consola*)list_get(listaConsola,i))

typedef struct{
	char *identificador;
	int sleep;
	int pid_usando;
}t_IO;

typedef struct{
	char *identificador;
	int valor;
}t_SEM;

typedef struct{
	t_intructions*	instrucciones;
	t_size			instrucciones_size;
	t_puntero_instruccion instruccion_inicio;
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
}t_entrada_stack;

typedef struct t_indice_stack{
	int cant_entradas;
	t_entrada_stack *entradas;
}t_indice_stack;

typedef struct{
	int pid;
	int PC;			//program counter
	int cant_pag;
	int idCPU;
	t_indice_codigo *indice_cod;
	t_indice_etiquetas *indice_etiquetas;
	t_indice_stack stack;
}t_pcb;


typedef struct{
	int id;
	int socket;
	bool libre;
	int pid;
}t_cpu;

typedef struct{
	int pid;
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
int cant_consolas;
t_list *listaCpu;
t_list *listaConsola;
t_pcb *pcb;

//Lista de IO y semaforos
t_list *IO, *SEM;

//agregamos sockets provisorios para cpu y consola
int socket_consola = 0;
int socket_CPU = 0;
int socket_umc;
int socket_escucha;
int cpu_fd, cons_fd;

//Las colas con los dif estados de los pcb
t_queue *ready, *blocked, *finished;
t_list* new;


//t_list * listaCPUs;
//t_list * listaConsolas;
//t_list * listaListos;
//t_list * listaBloqueados;
//t_list * listaEjecutar;
//t_list * listaFinalizados;





void crearConfiguracion(); //creo la configuracion y checkeo que sea valida
void crearSemaforos();
void crearIO();
void mostrarArrays();
bool validarParametrosDeConfiguracion();
void maximoFileDescriptor(int socket_escucha,int *max_fd);
void trabajarConexiones(fd_set *listen, int *max_fd, int cpu_fd, int prog_fd);
void trabajarConexionesSockets(fd_set *listen, int *max_fd, int cpu_fd, int cons_fd);
void procesarMensajeCPU(int codigoMensaje, int fd);
void procesarMensajeConsola(int codigoMensaje, int fd);
void agregarConsola(int fd, int *max_fd, fd_set *listen, fd_set *consolas);
int recibirMensaje(int socket, int *ret_recv);
void terminarConexion(int fd);

void iniciarNuevaConsola (int fd);
void validacionUMC();
void agregarConsola(int fd, int *max_fd, fd_set *listen, fd_set *consolas);
void agregarCpu(int fd, int *max_fd, fd_set *listen, fd_set *cpus);
void* enviarPaqueteACPU(void *nodoCPU);
void limpiarTerminados();
void planificar();
void moverDeNewAList(int pid, t_list *destino);
void moverDeNewAQueue(int pid, t_queue *destino);

int crearPCB(int source_size,char *source);
int solicitarPaginasUMC(int source_size, char *buffer, char *source);
void imprimirMsjConsola(int socketCPU, t_valor_variable mensaje);
void imprimirTextoConsola(int socketCPU, int sizeTexto);
void enviarTextoAConsola(int socketCPU, int sizeTexto);
void finalizarEjecucionProceso(int socket);

#endif /* NUCLEO_H_ */
