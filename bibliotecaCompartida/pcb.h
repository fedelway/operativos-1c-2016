/*
 * pcb.h
 *
 *  Created on: 18/7/2016
 *      Author: utnso
 */

#ifndef PCB_H_
#define PCB_H_

#include "parser/parser.h"
#include "parser/metadata_program.h"
#include "protocolo.h"
#include "sockets.h"
#include <sys/types.h>
#include <sys/socket.h>

typedef struct{
	t_size			instrucciones_size;
	t_puntero_instruccion instruccion_inicio;
	t_intructions*	instrucciones;
}t_indice_codigo;

typedef struct{
	t_size etiquetas_size;
	char *etiquetas;
}t_indice_etiquetas;

typedef struct{
	char identificador;
	int pag;
	int offset;
}t_var;

typedef struct{
	int cant_arg;
	int cant_var;
	int dirRetorno;
	int pagRet;
	int offsetRet;
	t_var *argumentos;
	t_var *variables;
}t_entrada_stack;

typedef struct t_indice_stack{
	int cant_entradas;
	t_entrada_stack *entradas;
}t_indice_stack;

typedef struct{
	int tamanio;
	int pid;
	int PC;			//program counter
	int cant_pag_cod;
	int idCPU;
	t_indice_codigo indice_cod;
	t_indice_etiquetas indice_etiquetas;
	t_indice_stack stack;
}t_pcb;

typedef struct {
	int tamanio;
	char *data_pcb;
} t_pcb_stream;

t_pcb_stream serializarPcb(t_pcb);
t_pcb deSerializarPcb(t_pcb_stream);
int tamanioPcb(t_pcb);
int enviarPcb(t_pcb pcb, int fd, int quantum);
t_pcb recibirPcb(int fd, bool nucleo, int *quantum);
t_pcb *pasarAPuntero(t_pcb);

void freePcb(t_pcb*);
void liberarPcb(t_pcb); //Libera los punteros de un pcb.

#endif /* PCB_H_ */
