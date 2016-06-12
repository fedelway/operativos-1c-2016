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
#include "generales.h"
#include "protocolo.h"
#include "parser/parser.h"
#include "parser/metadata_program.h"


/****************************************************************************************/
/*                           DEFINICION DE ESTRUCTURAS								    */
/****************************************************************************************/
#define PACKAGESIZE 1024

typedef struct {
	char* nucleo_ip;
	char* nucleo_puerto;
	char* umc_ip;
	char* umc_puerto;
	int *cpu_id;

}t_configuracion_cpu;

/****************************************************************************************/
/*                            CONFIGURACION Y CONEXIONES								*/
/****************************************************************************************/
void levantarDatosDeConfiguracion(t_configuracion_cpu* configuracion, char* config_path);
bool validarParametrosDeConfiguracion(t_config*);
void conectarAlNucleo(t_configuracion_cpu* configCPU);
void conectarAUMC(t_configuracion_cpu* configCPU);
void handshakeNucleo(void);
void handshakeUMC(int cpu_id);


/****************************************************************************************/
/*                                   FUNCIONES CPU								        */
/****************************************************************************************/
void recibirPCB(char* message);
void enviarPaqueteAUMC(char* package);
void ejecutoInstruccion(char* programa_ansisop, t_metadata_program* metadata, int numeroDeInstruccion);

/****************************************************************************************/
/*                                PRIMITIVAS ANSISOP								    */
/****************************************************************************************/
t_puntero socketes_definirVariable(t_nombre_variable variable);
t_puntero socketes_obtenerPosicionVariable(t_nombre_variable variable);
t_valor_variable socketes_dereferenciar(t_puntero puntero);
void socketes_asignar(t_puntero puntero, t_valor_variable variable);
t_valor_variable socketes_obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable socketes_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
t_puntero_instruccion socketes_irAlLabel(t_nombre_etiqueta etiqueta);
void socketes_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
t_puntero socketes_retornar(t_valor_variable retorno);
void socketes_imprimir(t_valor_variable valor_mostrar);
//int socketes_imprimirTexto(char* texto); TODO: ver definiciones
void socketes_imprimirTexto(char* texto);
//int socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo); TODO: ver definiciones
void socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
//int socketes_wait(t_nombre_semaforo identificador_semaforo); TODO: ver definiciones
void socketes_wait(t_nombre_semaforo identificador_semaforo);
//int socketes_signal(t_nombre_semaforo identificador_semaforo); TODO: ver definiciones
void socketes_signal(t_nombre_semaforo identificador_semaforo);


/****************************************************************************************/
/*                      DEFINICION DE ESTRUCTURAS ANSISOP							    */
/****************************************************************************************/

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
