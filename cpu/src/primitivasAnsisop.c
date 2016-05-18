/*
 * primitivasAnsisop.c
 *
 *  Created on: 16/5/2016
 *      Author: utnso
 */

#include "primitivasAnsisop.h"

static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;

//t_posicion socketes_definirVariable(t_nombre_variable identificador_variable);
t_puntero socketes_definirVariable(t_nombre_variable variable) {
	printf("Definir la variable %c\n", variable);
	return POSICION_MEMORIA;
}

//TODO: Cambiar t_puntero por t_posicion

//t_posicion socketes_obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_puntero socketes_obtenerPosicionVariable(t_nombre_variable variable) {
	printf("Obtener posicion de %c\n", variable);
	return POSICION_MEMORIA;
}

//t_valor_variable socketes_dereferenciar(t_posicion direccion_variable);
t_valor_variable socketes_dereferenciar(t_puntero puntero) {
	printf("Dereferenciar %d y su valor es: %d\n", puntero, CONTENIDO_VARIABLE);
	return CONTENIDO_VARIABLE;
}
//void socketes_asignar(t_posicion direccion_variable, t_valor_variable valor)
void socketes_asignar(t_puntero puntero, t_valor_variable variable) {
	printf("Asignando en %d el valor %d\n", puntero, variable);
}

//TODO: Cambiar parámetros que recibe y devuelve!

//t_valor_variable socketes_obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable socketes_obtenerValorCompartida(t_nombre_compartida variable){
	t_valor_variable valor_variable;
	printf("Obtener Valor Compartida\n");
	return valor_variable;
}

//t_valor_variable socketes_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor)
t_valor_variable socketes_adignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	t_valor_variable valor_variable;
	printf("Asignar Valor Compartida\n");
	return valor_variable;
}
//t_puntero_instruccion socketes_irAlLabel(t_nombre_etiqueta etiqueta);
t_puntero_instruccion socketes_irAlLabel(t_nombre_etiqueta etiqueta){
	t_puntero_instruccion puntero_instruccion;
	printf("Ir al Label\n");
	return puntero_instruccion;
}
//void socketes_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
void socketes_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	printf("Llamar con retorno:\n");
}
//t_puntero_instruccion socketes_retornar(t_valor_variable retorno);
t_puntero_instruccion socketes_retornar(t_valor_variable retorno){
	t_puntero_instruccion puntero_instruccion;
	printf("Retornar\n");
	return puntero_instruccion;
}

//int socketes_imprimir(t_valor_variable valor_mostrar);
int socketes_imprimir(t_valor_variable valor_mostrar) {
	int nro_entero = 17;
	printf("Imprimir %d\n", valor_mostrar);
	return nro_entero;
}

//int socketes_imprimirTexto(char* texto);
int socketes_imprimirTexto(char* texto) {
	int nro_entero = 17;
	printf("ImprimirTexto: %s", texto);
	return nro_entero;
}

//TODO: Modificar funciones!!

//int socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
int socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){
	int nro_entero = 17;
	printf("Entrada Salida\n");
	return nro_entero;
}

//int socketes_wait(t_nombre_semaforo identificador_semaforo);
int socketes_wait(t_nombre_semaforo identificador_semaforo){
	int nro_entero = 17;
	printf("Wait \n");
	return nro_entero;
}

//int socketes_signal(t_nombre_semaforo identificador_semaforo);
int socketes_signal(t_nombre_semaforo identificador_semaforo){
	int nro_entero = 17;
	printf("Signal \n");

	return nro_entero;
}

