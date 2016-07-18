/*
 * pcb.c
 *
 *  Created on: 18/7/2016
 *      Author: utnso
 */

#include "pcb.h"

#define serializar(algo) memcpy(data + offset, algo, size);\
	offset+=size;
#define deserializar(algo) memcpy(algo, data + offset, size);\
	offset+=size;

int tamanioPcb(t_pcb pcb)
{
	return 5*sizeof(int) +
			//Tamanio indice codigo
			sizeof(t_size) + sizeof(t_puntero_instruccion) + pcb.indice_cod->instrucciones_size * sizeof(t_intructions) +
			//Tamaño de indice etiquetas
			sizeof(t_size) + pcb.indice_etiquetas->etiquetas_size * sizeof(char) +
			//Tamaño de indice stack
			sizeof(int) + pcb.stack.cant_entradas * sizeof(t_entrada_stack);
}

t_pcb_stream serializarPcb(t_pcb pcb)
{
	t_pcb_stream stream;

	stream.tamanio = tamanioPcb(pcb);

	char *data = malloc(stream.tamanio);

	//Copio todos los datos
	int offset = 0;
	int size;

	size = sizeof(int);
	serializar(&pcb.tamanio)

	serializar(&pcb.pid)

	serializar(&pcb.PC)

	serializar(&pcb.cant_pag)

	serializar(&pcb.idCPU)

	//Serializo indice de codigo
	size = sizeof(t_size);
	serializar(&pcb.indice_cod->instrucciones_size)

	size = sizeof(t_puntero_instruccion);
	serializar(&pcb.indice_cod->instruccion_inicio)

	size = pcb.indice_cod->instrucciones_size * sizeof(t_intructions);
	serializar(&pcb.indice_cod->instrucciones)

	//Serializo indice etiquetas
	size = sizeof(t_size);
	serializar(&pcb.indice_etiquetas->etiquetas_size)

	size = sizeof(char) * pcb.indice_etiquetas->etiquetas_size;
	serializar(&pcb.indice_etiquetas->etiquetas)

	//Serializo stack
	size = sizeof(int);
	serializar(&pcb.stack.cant_entradas)

	size = sizeof(t_entrada_stack) * pcb.stack.cant_entradas;
	serializar(&pcb.stack.entradas)

	stream.data_pcb = data;

	return stream;
}

t_pcb deSerializarPcb(t_pcb_stream stream)
{
	t_pcb pcb;
	char *data = stream.data_pcb;

	//datos basicos
	int size = sizeof(int);
	int offset = 0;

	deserializar(&pcb.tamanio)

	deserializar(&pcb.pid)

	deserializar(&pcb.PC)

	deserializar(&pcb.cant_pag)

	deserializar(&pcb.idCPU)

	//Serializo indice de codigo
	size = sizeof(t_size);
	deserializar(&pcb.indice_cod->instrucciones_size)

	size = sizeof(t_puntero_instruccion);
	deserializar(&pcb.indice_cod->instruccion_inicio)

	size = pcb.indice_cod->instrucciones_size * sizeof(t_intructions);
	deserializar(&pcb.indice_cod->instrucciones)

	//Serializo indice etiquetas
	size = sizeof(t_size);
	deserializar(&pcb.indice_etiquetas->etiquetas_size)

	size = sizeof(char) * pcb.indice_etiquetas->etiquetas_size;
	deserializar(&pcb.indice_etiquetas->etiquetas)

	//Serializo stack
	size = sizeof(int);
	deserializar(&pcb.stack.cant_entradas)

	size = sizeof(t_entrada_stack) * pcb.stack.cant_entradas;
	deserializar(&pcb.stack.entradas)

	printf("%d, %d, %d", pcb.tamanio,pcb.pid,pcb.PC);
	return pcb;
}
