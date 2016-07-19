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
			sizeof(t_size) + sizeof(t_puntero_instruccion) + pcb.indice_cod.instrucciones_size * sizeof(t_intructions) +
			//Tamaño de indice etiquetas
			sizeof(t_size) + pcb.indice_etiquetas.etiquetas_size * sizeof(char) +
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
	serializar(&pcb.indice_cod.instrucciones_size)

	size = sizeof(t_puntero_instruccion);
	serializar(&pcb.indice_cod.instruccion_inicio)

	size = pcb.indice_cod.instrucciones_size * sizeof(t_intructions);
	serializar(pcb.indice_cod.instrucciones)

	//Serializo indice etiquetas
	size = sizeof(t_size);
	serializar(&pcb.indice_etiquetas.etiquetas_size)

	size = sizeof(char) * pcb.indice_etiquetas.etiquetas_size;
	serializar(pcb.indice_etiquetas.etiquetas)

	//Serializo stack
	size = sizeof(int);
	serializar(&pcb.stack.cant_entradas)

	size = sizeof(t_entrada_stack) * pcb.stack.cant_entradas;
	serializar(pcb.stack.entradas)

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
	deserializar(&pcb.indice_cod.instrucciones_size)

	size = sizeof(t_puntero_instruccion);
	deserializar(&pcb.indice_cod.instruccion_inicio)

	size = pcb.indice_cod.instrucciones_size * sizeof(t_intructions);
	pcb.indice_cod.instrucciones = malloc(size);
	deserializar(pcb.indice_cod.instrucciones)

	//Serializo indice etiquetas
	size = sizeof(t_size);
	deserializar(&pcb.indice_etiquetas.etiquetas_size)

	size = sizeof(char) * pcb.indice_etiquetas.etiquetas_size;
	pcb.indice_etiquetas.etiquetas = malloc(size);
	deserializar(pcb.indice_etiquetas.etiquetas)

	//Serializo stack
	size = sizeof(int);
	deserializar(&pcb.stack.cant_entradas)

	size = sizeof(t_entrada_stack) * pcb.stack.cant_entradas;
	pcb.stack.entradas = malloc(size);
	deserializar(pcb.stack.entradas)

	printf("%d, %d, %d", pcb.tamanio,pcb.pid,pcb.PC);
	return pcb;
}

int enviarPcb(t_pcb pcb, int fd, int quantum)
{
	t_pcb_stream stream = serializarPcb(pcb);

	if(quantum < 0)
	{//Es una cpu
		int buffer[2] = {FIN_QUANTUM,stream.tamanio};

		//Envio mensaje y tamaño
		if( send(fd, &buffer, 2*sizeof(int), 0) <= 0 )
		{
			perror("Error al enviar pcb (1).\n");
			return -1;
		}

		if( send(fd,stream.data_pcb,stream.tamanio,0) <= 0)
		{
			perror("Error al enviar pcb (2).\n");
			return -1;
		}

		//all ok
		return 0;
	}else{//Es el nucleo

		int buffer[3] = {EJECUTA, quantum, stream.tamanio};

		if( send(fd, &buffer, 3*sizeof(int), 0) <= 0)
		{
			perror("Error al enviar pcb (1).\n");
			return -1;
		}

		if( send(fd, stream.data_pcb, stream.tamanio, 0) <= 0)
		{
			perror("Error al enviar pcb (2).\n");
			return -1;
		}

		//all ok
		return 0;
	}

}

t_pcb recibirPcb(int fd, bool nucleo)
{
	t_pcb_stream stream;

	int tamanio;

	if(nucleo)
	{//Nucleo recibe solo el tamaño

		recv(fd, &tamanio, sizeof(int),0);
	}else{
		int quantum;

		recv(fd, &quantum, sizeof(int), 0);
		recv(fd, &tamanio, sizeof(int), 0);
	}
	//A partir de aca es igual tanto para cpu como para nucleo

	char *buffer = malloc(tamanio);

	if( recvAll(fd, buffer, tamanio, 0) <= 0)
	{
		perror("Error al recibir Pcb");
	}

	stream.data_pcb = buffer;

	t_pcb pcb = deSerializarPcb(stream);

	//Libero memoria
	free(stream.data_pcb);

	return pcb;
}
