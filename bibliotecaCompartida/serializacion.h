/*
 * serializacion.h
 *
 *  Created on: 3/7/2016
 *      Author: utnso
 */

#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include "parser/parser.h"
#include "parser/metadata_program.h"
#include "commons/collections/list.h"

//estructura para instrucciones del pcb
typedef struct {
	t_intructions* instrucciones;
	t_size instrucciones_size;
	t_puntero_instruccion instruccion_inicio;
} t_indice_codigo;

//estructura para etiquetas del pcb
typedef struct {
	char *etiquetas;
	t_size etiquetas_size;
} t_indice_etiquetas;

typedef struct {
	int argumentos;
	int variables;
	int dirRetorno;
	int posVariable;
} t_nodo_stack;

//estructura para el pcb
typedef struct {
	int pid;
	int PC;	//program counter
	int cant_pag;
	int idCPU;
	t_indice_codigo *indice_cod;
	t_indice_etiquetas *indice_etiquetas;
	t_list *indice_stack;
} t_pcb;

typedef struct {
	int pid;
	int tamanio;
	char *data_pcb;
} t_pcb_stream;

t_pcb_stream *serializador_PCB(t_pcb *pcb);
t_pcb *desSerializador_PCB(t_pcb_stream *pcb_stream);

#endif /* SERIALIZACION_H_ */
