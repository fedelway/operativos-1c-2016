/*
 * serializacion.c
 *
 *  Created on: 3/7/2016
 *      Author: utnso
 */


#include "serializacion.h"



//Ver si tiene que recibir el tamanio del pcb o calcularlo.
t_pcb_stream *serializador_PCB(t_pcb *pcb){

	int tamanio_data_pcb = 	4*sizeof(int)  //pid, pc, cant_pag, idCPU
							+ sizeof(t_size) //indice_cod->instrucciones_size
							+ pcb->indice_cod->instrucciones_size*sizeof(t_intructions) //como metadata->instrucciones serializado es un array, multiplico el tamaño de cada posición por el total
							+ sizeof(t_puntero_instruccion) //indice_cod->instruccion_inicio
							+ sizeof(t_size) //etiquetas_size
							+ pcb->indice_etiquetas->etiquetas_size //*etiquetas
							+ sizeof(int); //cantidad de nodos del indice stack
	//						+ list_size(pcb->indice_stack)*sizeof(t_nodo_stack); //nodo_stack * numero de nodos

	char *data_pcb = malloc(tamanio_data_pcb);

	int offset = 0, tmp_size = 0, i;

	memcpy(data_pcb, &pcb->pid, tmp_size = sizeof(int));

	offset = tmp_size;
	memcpy(data_pcb + offset, &pcb->PC, tmp_size = sizeof(int));

	offset += tmp_size;
	memcpy(data_pcb + offset, &pcb->cant_pag, tmp_size = sizeof(int));

	offset += tmp_size;
	memcpy(data_pcb + offset, &pcb->indice_cod->instrucciones_size, tmp_size = sizeof(t_size));

	for(i=0; i < pcb->indice_cod->instrucciones_size; i++){
		t_intructions instr = pcb->indice_cod->instrucciones[i];

		offset += tmp_size;
		memcpy(data_pcb + offset, &instr.start, tmp_size = sizeof(t_puntero_instruccion));

		offset += tmp_size;
		memcpy(data_pcb + offset, &instr.offset, tmp_size = sizeof(t_size));
	}

	offset += tmp_size;
	memcpy(data_pcb + offset, &pcb->indice_cod->instruccion_inicio, tmp_size = sizeof(t_puntero_instruccion));

	offset += tmp_size;
	memcpy(data_pcb + offset, &pcb->indice_etiquetas->etiquetas_size, tmp_size = sizeof(t_size));

	offset += tmp_size;
	memcpy(data_pcb + offset, &pcb->indice_etiquetas->etiquetas, tmp_size = pcb->indice_etiquetas->etiquetas_size);

	//int cantidad_nodos_stack = list_size(pcb->indice_stack);
	offset += tmp_size;
	//memcpy(data_pcb + offset, &cantidad_nodos_stack, tmp_size = sizeof(int));

	void _copiar_nodos_stack(t_nodo_stack *ns) {

		//if(*ns != null){

			offset += tmp_size;
			memcpy(data_pcb + offset, &ns->argumentos, tmp_size = sizeof(int));

			offset += tmp_size;
			memcpy(data_pcb + offset, &ns->variables, tmp_size = sizeof(int));

			offset += tmp_size;
			memcpy(data_pcb + offset, &ns->dirRetorno, tmp_size = sizeof(int));

			offset += tmp_size;
			memcpy(data_pcb + offset, &ns->posVariable, tmp_size = sizeof(int));
		//}
	}

	//list_iterate(pcb->indice_stack, (void*) _copiar_nodos_stack);

	t_pcb_stream *stream_pcb = malloc(sizeof(t_pcb_stream));
	stream_pcb->tamanio = tamanio_data_pcb;
	stream_pcb->data_pcb = data_pcb;

	return stream_pcb;
}

t_pcb *desSerializador_PCB(t_pcb_stream *pcb_stream){

	t_pcb *pcb = malloc(sizeof(t_pcb));
	t_indice_codigo *indiceCodigo = malloc(sizeof(t_indice_codigo));
	t_indice_etiquetas *indiceEtiquetas = malloc(sizeof(t_indice_etiquetas));
	t_list *indiceStack;

	int offset = 0, tmp_size = 0;

	//pid, pc, cant_pag, idCPU
	memcpy(&pcb->pid, pcb_stream->data_pcb, tmp_size = sizeof(int));

	offset += tmp_size;
	memcpy(&pcb->PC, pcb_stream->data_pcb + offset, tmp_size = sizeof(int));

	offset += tmp_size;
	memcpy(&pcb->cant_pag, pcb_stream->data_pcb + offset, tmp_size = sizeof(int));

	offset += tmp_size;
	memcpy(&pcb->idCPU, pcb_stream->data_pcb + offset, tmp_size = sizeof(int));

	offset += tmp_size;
	memcpy(&pcb->idCPU, pcb_stream->data_pcb + offset, tmp_size = sizeof(t_size));

	//IndiceCodigo
	offset += tmp_size;
	memcpy(&indiceCodigo->instrucciones_size, pcb_stream->data_pcb + offset, tmp_size = sizeof(t_size));

	indiceCodigo->instrucciones = malloc(indiceCodigo->instrucciones_size * sizeof(t_intructions));
	t_intructions array_instrucciones[indiceCodigo->instrucciones_size];

	int i;
	for(i=0; i < indiceCodigo->instrucciones_size; i++){
		offset += tmp_size;
		memcpy(&array_instrucciones[i].start, pcb_stream->data_pcb + offset, tmp_size = sizeof(t_puntero_instruccion));

		offset += tmp_size;
		memcpy(&array_instrucciones[i].offset, pcb_stream->data_pcb + offset, tmp_size = sizeof(t_size));
	}

	memcpy(indiceCodigo->instrucciones, array_instrucciones, sizeof(t_intructions) * indiceCodigo->instrucciones_size);

	offset += tmp_size;
	memcpy(&indiceCodigo->instruccion_inicio, pcb_stream->data_pcb + offset, tmp_size = sizeof(t_puntero_instruccion));

	//Indice etiquetas
	offset += tmp_size;
	memcpy(&indiceEtiquetas->etiquetas_size, pcb_stream->data_pcb + offset, tmp_size = sizeof(t_size));

	offset += tmp_size;
	memcpy(indiceEtiquetas->etiquetas, pcb_stream->data_pcb + offset, tmp_size = indiceEtiquetas->etiquetas_size);

	offset += tmp_size;
	int cantidad_nodos_stack;
	memcpy(&cantidad_nodos_stack, pcb_stream->data_pcb + offset, tmp_size = sizeof(int));

	indiceStack = list_create();

	for (i = 0; i < cantidad_nodos_stack; ++i) {
		t_nodo_stack *nodo = malloc(sizeof(t_nodo_stack));

		offset += tmp_size;
		memcpy(&nodo->argumentos, pcb_stream->data_pcb + offset, tmp_size = sizeof(int));

		offset += tmp_size;
		memcpy(&nodo->variables, pcb_stream->data_pcb + offset, tmp_size = sizeof(int));

		offset += tmp_size;
		memcpy(&nodo->dirRetorno, pcb_stream->data_pcb + offset, tmp_size = sizeof(int));

		offset += tmp_size;
		memcpy(&nodo->posVariable, pcb_stream->data_pcb + offset, tmp_size = sizeof(int));

		list_add(indiceStack, nodo);
	}

	//Cargo los índices al pcb
	pcb->indice_cod = indiceCodigo;
	pcb->indice_etiquetas = indiceEtiquetas;
	//pcb->indice_stack = indiceStack;

	return pcb;

}
