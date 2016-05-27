/*
 * cpu.h
 *
 *  Created on: 29/4/2016
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"
#include "socketCliente.h"
#include "primitivasAnsisop.h"
#include "parser/parser.h"
#include "parser/metadata_program.h"

#define PACKAGESIZE 1024

void crearConfiguracion(); //creo la configuracion y checkeo que sea valida
bool validarParametrosDeConfiguracion();
void recibirMensajeNucleo(char* message, int socket_nucleo);
void validarNucleo(int nucleo_fd);
void enviarPaqueteAUMC(char* package, int socket);
void ejecutoInstruccion(char* programa_ansisop, t_metadata_program* metadata, int numeroDeInstruccion);

AnSISOP_funciones funciones = {
		.AnSISOP_definirVariable		= socketes_definirVariable,
		.AnSISOP_obtenerPosicionVariable= socketes_obtenerPosicionVariable,
		.AnSISOP_dereferenciar			= socketes_dereferenciar,
		.AnSISOP_asignar				= socketes_asignar,
		.AnSISOP_obtenerValorCompartida	= socketes_obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = socketes_asignarValorCompartida,
		.AnSISOP_irAlLabel				= socketes_irAlLabel,
		//.AnSISOP_llamarSinRetorno		= socketes_asignar,
		.AnSISOP_llamarConRetorno		= socketes_llamarConRetorno,
		//.AnSISOP_finalizar				= socketes_asignar,
		.AnSISOP_imprimir				= socketes_imprimir,
		.AnSISOP_imprimirTexto				= socketes_imprimirTexto,
		.AnSISOP_entradaSalida			= socketes_entradaSalida
};

AnSISOP_kernel funciones_kernel = {
		.AnSISOP_wait					= socketes_wait,
		.AnSISOP_signal					= socketes_signal
};

#endif /* CPU_H_ */
