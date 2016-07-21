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
#define FINALIZAR_PROGRAMA 1030		//A umc le deberia enviar el pid.
#define EJECUTA 1040
#define ENTRADA_SALIDA 1050
#define FIN_QUANTUM 1060
#define FINALIZAR 1070
#define OPERACION_PRIVILEGIADA 1080
#define ANSISOP_PODES_SEGUIR 1100	//Despues de un wait avisa que puede seguir ejecutando
#define ANSISOP_BLOQUEADO	 1101	//Despues de un wait avisa que no puede seguir ejecutando
#define IMPRIMIR_VALOR 1200			//(int valor)
#define IMPRIMIR_CADENA 1201		//(int tamanio_cadena, char* cadena)

//Mensajes de Consola
#define SOY_CONSOLA 2000
#define CONSOLA_OK 2010
#define ENVIO_FUENTE 2020
#define FINALIZAR_EJECUCION 2030

//Mensajes de Cpu
#define SOY_CPU 3000
#define CPU_OK 3010
#define ANSISOP_IMPRIMIR 3020		//(int pid, int valor)
#define ANSISOP_IMPRIMIR_TEXTO 3030 //(int pid, int tamañoCadena, char *cadena)
#define LEER 3050 					//(int pag, int offset, int size, int pid)
#define ESCRIBIR 3060 				//(int pag, int offset, int size, int pid, char *) Envio ademas lo que quiero escribir
#define ANSISOP_ENTRADA_SALIDA 3090			  //(int tiempo, int tamañoCadena, char* cadena, pcb)
#define ANSISOP_OBTENER_VALOR_COMPARTIDO 3100 //(int pid, int tamañoCadena, char *id)
#define ANSISOP_ASIGNAR_VALOR_COMPARTIDO 3110 //(int pid, int nuevo_valor, int tamañoCadena, char *id)
#define ANSISOP_FIN_PROGRAMA 3200			  //(int pid)
#define DESCONEXION_CPU 3999 		//Avisa que se va a desconectar(exit feliz :D )
#define ANSISOP_WAIT 3070			//(int pid, int tamañoCadena, char *identificador)
#define ANSISOP_SIGNAL 3080			//(int pid, int tamañoCadena, char *identificador)
//Defines faltantes
#define ENVIO_PCB_ACTUALIZADO 3500
#define SOLICITUD_INSTRUCCION 3510

//Mensajes de Umc
#define SOY_UMC 4000
#define UMC_OK 4010
#define RECHAZO_PROGRAMA 4020
#define ACEPTO_PROGRAMA 4021
#define GUARDA_PAGINA 4030 		//(int pid, int pag, char* contenido) Pido a swap que guarde la pagina indicada
#define SOLICITUD_PAGINA 4031 	//(int pid, int pag) Pido a swap que me de la pagina indicada
#define UMC_FINALIZAR_PROGRAMA 4090 //(int pid) Le aviso a swap que tiene que borrar el programa y liberar sus recursos
#define RESERVA_ESPACIO	4050	//(int pid, int cant_pag)Umc pide a swap reservar espacio para el programa pid.

//Mensajes de Swap
#define SOY_SWAP 5000
#define SWAP_OK 5010
#define SWAP_PROGRAMA_OK 5020			//El ok al pedido de reservar memoria
#define SWAP_PROGRAMA_RECHAZADO 5021
#define CARGAR_PROGRAMA 5030
#define LEER_PAGINA 5040
#define MODIFICAR_PAGINA 5050
#define TERMINAR_PROGRAMA 5080

#endif /* PROTOCOLO_H_ */
