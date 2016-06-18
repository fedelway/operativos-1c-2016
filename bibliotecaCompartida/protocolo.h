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
#define NUCLEO_OK 1010
#define INICIALIZAR_PROGRAMA 1020
#define FINALIZAR_PROGRAMA 1030
#define ENVIO_PCB 1040
#define ENTRADA_SALIDA 1050
#define FIN_QUANTUM 1060
#define FINALIZAR 1070
#define OPERACION_PRIVILEGIADA 1080

//Mensajes de Consola
#define SOY_CONSOLA 2000
#define CONSOLA_OK 2010
#define ENVIO_FUENTE 2020

//Mensajes de Cpu
#define SOY_CPU 3000
#define CPU_OK 3010
#define ANSISOP_IMPRIMIR 3020
#define ANSISOP_IMPRIMIR_TEXTO 3030
#define LEER 3050 					//(int pag, int offset, int size, int pid)
#define ESCRIBIR 3060 				//(int pag, int offset, int size, int *char, int pid) Envio ademas lo que quiero escribir
#define ANSISOP_WAIT 3070
#define ANSISOP_SIGNAL 3080
#define ANSISOP_ENTRADA_SALIDA 3090
#define ANSISOP_OBTENER_VALOR_COMPARTIDO 3100
#define ANSISOP_ASIGNAR_VALOR_COMPARTIDO 3110


//Mensajes de Umc
#define SOY_UMC 4000
#define UMC_OK 4010
#define RECHAZO_PROGRAMA 4020
#define ACEPTO_PROGRAMA 4021
#define GUARDA_PAGINA 4030 		//(int pag, int pid, char* contenido) Pido a swap que guarde la pagina indicada
#define SOLICITUD_PAGINA 4031 	//(int pag, int pid) Pido a swap que me de la pagina indicada

//Mensajes de Swap
#define SOY_SWAP 5000
#define SWAP_OK 5010

#endif /* PROTOCOLO_H_ */
