/*
 * primitivasAnsisop.h
 *
 *  Created on: 16/5/2016
 *      Author: utnso
 */

#ifndef PRIMITIVASANSISOP_H_
#define PRIMITIVASANSISOP_H_

#include "parser/parser.h"

t_puntero socketes_definirVariable(t_nombre_variable variable);
t_puntero socketes_obtenerPosicionVariable(t_nombre_variable variable);
t_valor_variable socketes_dereferenciar(t_puntero puntero);
void socketes_asignar(t_puntero puntero, t_valor_variable variable);
t_valor_variable socketes_obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable socketes_adignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
t_puntero_instruccion socketes_irAlLabel(t_nombre_etiqueta etiqueta);
void socketes_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
t_puntero socketes_retornar(t_valor_variable retorno);
int socketes_imprimir(t_valor_variable valor_mostrar);
int socketes_imprimirTexto(char* texto);
int socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
int socketes_wait(t_nombre_semaforo identificador_semaforo);
int socketes_signal(t_nombre_semaforo identificador_semaforo);


#endif /* PRIMITIVASANSISOP_H_ */
