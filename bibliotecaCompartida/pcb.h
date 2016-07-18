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
	int tamanio;
	int pid;
	int PC;			//program counter
	int cant_pag;
	int idCPU;
	t_indice_codigo *indice_cod;
	t_indice_etiquetas *indice_etiquetas;
	t_indice_stack stack;
}t_pcb;

typedef struct {
	int tamanio;
	char *data_pcb;
} t_pcb_stream;

t_pcb_stream serializarPcb(t_pcb);
t_pcb deSerializarPcb(t_pcb_stream);
int tamanioPcb(t_pcb);

#endif /* PCB_H_ */
