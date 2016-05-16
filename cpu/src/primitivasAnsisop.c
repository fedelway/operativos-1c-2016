/*
 * primitivasAnsisop.c
 *
 *  Created on: 16/5/2016
 *      Author: utnso
 */

#include "primitivasAnsisop.h"

static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;

t_puntero socketes_definirVariable(t_nombre_variable variable) {
	printf("Definir la variable %c\n", variable);
	return POSICION_MEMORIA;
}

t_puntero socketes_obtenerPosicionVariable(t_nombre_variable variable) {
	printf("Obtener posicion de %c\n", variable);
	return POSICION_MEMORIA;
}

t_valor_variable socketes_dereferenciar(t_puntero puntero) {
	printf("Dereferenciar %d y su valor es: %d\n", puntero, CONTENIDO_VARIABLE);
	return CONTENIDO_VARIABLE;
}

void socketes_asignar(t_puntero puntero, t_valor_variable variable) {
	printf("Asignando en %d el valor %d\n", puntero, variable);
}

//TODO: Cambiar parámetros que recibe y devuelve!

void socketes_obtenerValorCompartida(){
	printf("Obtener Valor Compartida\n");
}

void socketes_adignarValorCompartida(){
	printf("Asignar Valor Compartida\n");
}

void socketes_irAlLabel(){
	printf("Ir al Label\n");
}

void socketes_llamarFuncion(){
	printf("Llamar función:\n");
}

void socketes_retornar(){
	printf("Retornar\n");
}

void socketes_imprimir(t_valor_variable valor) {
	printf("Imprimir %d\n", valor);
}

void socketes_imprimirTexto(char* texto) {
	printf("ImprimirTexto: %s", texto);
}

//TODO: Modificar funciones!!
void socketes_entradaSalida(){
	printf("Entrada Salida\n");
}

void socketes_wait(){
	printf("Wait \n");
}

void socketes_signal(){
	printf("Signal \n");
}

