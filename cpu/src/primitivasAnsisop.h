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
void socketes_obtenerValorCompartida();
void socketes_adignarValorCompartida();
void socketes_irAlLabel();
void socketes_llamarFuncion();
void socketes_retornar();
void socketes_imprimir(t_valor_variable valor);
void socketes_imprimirTexto(char* texto);
void socketes_entradaSalida();
void socketes_wait();
void socketes_signal();

#endif /* PRIMITIVASANSISOP_H_ */
