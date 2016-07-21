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
	const int tamanioVar = sizeof(char) + 2*sizeof(int);

	int tamanio =  5*sizeof(int) +
			//Tamanio indice codigo
			sizeof(t_size) + sizeof(t_puntero_instruccion) + pcb.indice_cod.instrucciones_size * sizeof(t_intructions) +
			//Tamaño de indice etiquetas
			sizeof(t_size) + pcb.indice_etiquetas.etiquetas_size * sizeof(char);

	//Tamaño de indice stack
	tamanio += sizeof(pcb.stack.cant_entradas);

	int i;//Tamaño de cada entrada
	for(i=0; i<pcb.stack.cant_entradas; i++)
	{
		tamanio += 5*sizeof(int);								//tamaño de los ints
		tamanio += pcb.stack.entradas[i].cant_arg * tamanioVar; //tamaño de los argumentos
		tamanio += pcb.stack.entradas[i].cant_var * tamanioVar; //tamaño de los argumentos
	}

	printf("tamanio pcb: %d.\n", tamanio);

	return tamanio;
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

	serializar(&pcb.cant_pag_cod)

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

	printf("cant_entradas_stack: %d.\n",pcb.stack.cant_entradas);

	int i;//Serializo cada entrada del stack
	for(i=0; i<pcb.stack.cant_entradas; i++)
	{
		size = sizeof(int);
		serializar(&pcb.stack.entradas[i].cant_arg)

		serializar(&pcb.stack.entradas[i].cant_var)

		serializar(&pcb.stack.entradas[i].dirRetorno)

		serializar(&pcb.stack.entradas[i].pagRet)

		serializar(&pcb.stack.entradas[i].offsetRet)

		int j;//Serializo argumentos
		for(j=0; j<pcb.stack.entradas[i].cant_arg; j++)
		{
			size = sizeof(char);
			serializar(&pcb.stack.entradas[i].argumentos[j].identificador)

			size = sizeof(int);
			serializar(&pcb.stack.entradas[i].argumentos[j].pag)

			serializar(&pcb.stack.entradas[i].argumentos[j].offset)
		}

		//Serializo variables
		for(j=0; j<pcb.stack.entradas[i].cant_var; j++)
		{
			size = sizeof(char);
			serializar(&pcb.stack.entradas[i].variables[j].identificador)

			size = sizeof(int);
			serializar(&pcb.stack.entradas[i].variables[j].pag)

			serializar(&pcb.stack.entradas[i].variables[j].offset)
		}
	}//Termino de serializar stack

	stream.data_pcb = data;

	return stream;
}

t_pcb deSerializarPcb(t_pcb_stream stream)
{
	printf("DeserializarPcb.\n");
	t_pcb pcb;
	char *data = stream.data_pcb;

	//datos basicos
	int size = sizeof(int);
	int offset = 0;

	deserializar(&(pcb.tamanio))

	deserializar(&pcb.pid)

	deserializar(&pcb.PC)

	deserializar(&pcb.cant_pag_cod)

	deserializar(&pcb.idCPU)

	//Deserializo indice de codigo
	size = sizeof(t_size);
	deserializar(&pcb.indice_cod.instrucciones_size)

	size = sizeof(t_puntero_instruccion);
	deserializar(&pcb.indice_cod.instruccion_inicio)

	size = pcb.indice_cod.instrucciones_size * sizeof(t_intructions);
	pcb.indice_cod.instrucciones = malloc(size);
	deserializar(pcb.indice_cod.instrucciones)

	//Deserializo indice etiquetas
	size = sizeof(t_size);
	deserializar(&pcb.indice_etiquetas.etiquetas_size)

	size = sizeof(char) * pcb.indice_etiquetas.etiquetas_size;
	pcb.indice_etiquetas.etiquetas = malloc(size);
	deserializar(pcb.indice_etiquetas.etiquetas)

	//Deserializo stack
	size = sizeof(int);
	deserializar(&pcb.stack.cant_entradas)

	printf("cant entradas stack: %d.\n",pcb.stack.cant_entradas);

	pcb.stack.entradas = malloc(sizeof(t_entrada_stack) * pcb.stack.cant_entradas);

	int i;//Deserializo cada entrada del stack
	for(i=0; i<pcb.stack.cant_entradas; i++)
	{
		printf("Deserializo entrada n°: %d.\n",i);

		size = sizeof(int);
		deserializar(&pcb.stack.entradas[i].cant_arg)
		printf("cant_arg: %d.\n",pcb.stack.entradas[i].cant_arg);
		deserializar(&pcb.stack.entradas[i].cant_var)
		printf("cant_var: %d.\n",pcb.stack.entradas[i].cant_var);
		deserializar(&pcb.stack.entradas[i].dirRetorno)

		deserializar(&pcb.stack.entradas[i].pagRet)

		deserializar(&pcb.stack.entradas[i].offsetRet)

		pcb.stack.entradas[i].argumentos = malloc(sizeof(t_var) * pcb.stack.entradas[i].cant_arg);
		int j;//Deserializo argumentos
		for(j=0; j<pcb.stack.entradas[i].cant_arg; j++)
		{
			size = sizeof(char);
			deserializar(&pcb.stack.entradas[i].argumentos[j].identificador)

			size = sizeof(int);
			deserializar(&pcb.stack.entradas[i].argumentos[j].pag)

			deserializar(&pcb.stack.entradas[i].argumentos[j].offset)
		}

		pcb.stack.entradas[i].variables = malloc(sizeof(t_var) * pcb.stack.entradas[i].cant_var);
		//Deserializo variables
		for(j=0; j<pcb.stack.entradas[i].cant_var; j++)
		{
			size = sizeof(char);
			deserializar(&pcb.stack.entradas[i].variables[j].identificador)

			size = sizeof(int);
			deserializar(&pcb.stack.entradas[i].variables[j].pag)

			deserializar(&pcb.stack.entradas[i].variables[j].offset)
		}
	}//Termino de Deserializar stack

	printf("tamanio pcb:%d, pid: %d, program counter: %d \n\n", pcb.tamanio,pcb.pid,pcb.PC);
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
		printf("Pcb enviado correctamente.\n");
		return 0;
	}

}

t_pcb recibirPcb(int fd, bool nucleo, int *quantum)
{
	t_pcb_stream stream;

	int tamanio;

	if(nucleo)
	{//Nucleo recibe solo el tamaño

		recv(fd, &tamanio, sizeof(int),0);
	}else{
		//Cpu recibe ademas el quantum
		recv(fd, quantum, sizeof(int), 0);
		recv(fd, &tamanio, sizeof(int), 0);

		printf("quantum: %d Tamaño: %d.\n", *quantum, tamanio);
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

	printf("Recibi el pcb correctamente.\n");

	return pcb;
}

void freePcb(t_pcb *pcb)
{
	free(pcb->indice_cod.instrucciones);
	free(pcb->indice_etiquetas.etiquetas);

	int i;
	for(i=0;i<pcb->stack.cant_entradas;i++)
	{//Libero var y argumentos
		free(pcb->stack.entradas[i].argumentos);
		free(pcb->stack.entradas[i].variables);
	}
	free(pcb->stack.entradas);
	free(pcb->stack);

	free(pcb);
}
