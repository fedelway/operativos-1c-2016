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
#include <signal.h>
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
#include "serializacion.h"
#include "parser/parser.h"
#include "parser/metadata_program.h"


/****************************************************************************************/
/*                           DEFINICION DE ESTRUCTURAS								    */
/****************************************************************************************/
#define PACKAGESIZE 1024

//estructura de configuración de la cpu
typedef struct {
	char* nucleo_ip;
	char* nucleo_puerto;
	char* umc_ip;
	char* umc_puerto;
	int *cpu_id;
}t_configuracion_cpu;

//estructura para solicitar escritura a la umc
typedef struct{
	int nroPagina;
	int offset;
	int size;
	int pid;
	int valor;
}__attribute__ ((__packed__)) t_solicitud_escritura;

//estructura de solicitud de lectura a la umc
typedef struct{
	int nroPagina;
	int offset;
	int size;
	int pid;
}__attribute__ ((__packed__)) t_solicitud_lectura;


/****************************************************************************************/
/*                            CONFIGURACION Y CONEXIONES								*/
/****************************************************************************************/
void levantarDatosDeConfiguracion(t_configuracion_cpu* configuracion, char* config_path);
bool validarParametrosDeConfiguracion(t_config*);
void conectarAlNucleo(t_configuracion_cpu* configCPU);
void conectarAUMC(t_configuracion_cpu* configCPU);
void handshakeNucleo(void);
void handshakeUMC(int cpu_id);
void finalizarCpu(void);
void finalizarCpuPorError(void);


/****************************************************************************************/
/*                                   FUNCIONES CPU								        */
/****************************************************************************************/
void recibirPCB(void);
void enviarPaqueteAUMC(char* package);
void ejecutoInstruccion(char* programa_ansisop, t_metadata_program* metadata, int numeroDeInstruccion);
void handler_seniales(int senial);
int ejecutoInstrucciones();
void devuelvoPcbActualizadoAlNucleo();
void liberarEspacioDelPCB();
char* solicitoInstruccionAUMC(int start, int offset);

/****************************************************************************************/
/*                                PRIMITIVAS ANSISOP								    */
/****************************************************************************************/
t_puntero socketes_definirVariable(t_nombre_variable variable);
t_puntero socketes_obtenerPosicionVariable(t_nombre_variable variable);
t_valor_variable socketes_dereferenciar(t_puntero puntero);
void socketes_asignar(t_puntero puntero, t_valor_variable variable);
t_valor_variable socketes_obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable socketes_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
void socketes_irAlLabel(t_nombre_etiqueta etiqueta);
void socketes_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
t_puntero socketes_retornar(t_valor_variable retorno);
void socketes_imprimir(t_valor_variable valor_mostrar);
void socketes_imprimirTexto(char* texto);
void socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void socketes_wait(t_nombre_semaforo identificador_semaforo);
void socketes_signal(t_nombre_semaforo identificador_semaforo);
void socketes_finalizar();


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
		.AnSISOP_llamarConRetorno		= socketes_llamarConRetorno,
		.AnSISOP_finalizar				= socketes_finalizar,
		.AnSISOP_imprimir				= socketes_imprimir,
		.AnSISOP_imprimirTexto			= socketes_imprimirTexto,
		.AnSISOP_entradaSalida			= socketes_entradaSalida
};

AnSISOP_kernel funciones_kernel = {
		.AnSISOP_wait					= socketes_wait,
		.AnSISOP_signal					= socketes_signal
};

#endif /* CPU_H_ */
