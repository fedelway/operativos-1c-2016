/*
 * protocolo.h
 *
 *  Created on: 23/5/2016
 *      Author: utnso
 */

#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

//Mensajes de nucleo
#define SOY_NUCLEO 1000
#define INICIALIZAR_PROGRAMA 1010
#define FINALIZAR_PROGRAMA 1020
#define ENVIO_PCB 1030

//Mensajes de Consola
#define SOY_CONSOLA 2000
#define ENVIO_FUENTE 2010

//Mensajes de Cpu
#define SOY_CPU 3000
#define ANSISOP_IMPRIMIR 3010
#define ANSISOP_IMPRIMIR_TEXTO 3020
#define LEER 3040 					//(int pag, int offset, int size, int pid)
#define ESCRIBIR 3050 				//(int pag, int offset, int size, int *char, int pid) Envio ademas lo que quiero escribir
#define ANSISOP_WAIT 3060
#define ANSISOP_SIGNAL 3070

//Mensajes de Umc
#define SOY_UMC 4000
#define RECHAZO_PROGRAMA 4010
#define ACEPTO_PROGRAMA 4011
#define GUARDA_PAGINA 4020 		//(int pag, int pid, char* contenido) Pido a swap que guarde la pagina indicada
#define SOLICITUD_PAGINA 4021 	//(int pag, int pid) Pido a swap que me de la pagina indicada

//Mensajes de Swap
#define SOY_SWAP 5000

#endif /* PROTOCOLO_H_ */
