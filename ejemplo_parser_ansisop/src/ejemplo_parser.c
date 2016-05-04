/*
 * ejemplo_parser.c
 *
 *  Created on: 1/5/2016
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"
#include "parser/parser.h"
#include "parser/metadata_program.h"

#define PACKAGESIZE 1024

t_log* logger;

//Ejemplo para el parser

static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;

t_puntero dummy_definirVariable(t_nombre_variable variable) {
	printf("definir la variable %c\n", variable);
	return POSICION_MEMORIA;
}

t_puntero dummy_obtenerPosicionVariable(t_nombre_variable variable) {
	printf("Obtener posicion de %c\n", variable);
	return POSICION_MEMORIA;
}

t_valor_variable dummy_dereferenciar(t_puntero puntero) {
	printf("Dereferenciar %d y su valor es: %d\n", puntero, CONTENIDO_VARIABLE);
	return CONTENIDO_VARIABLE;
}

void dummy_asignar(t_puntero puntero, t_valor_variable variable) {
	printf("Asignando en %d el valor %d\n", puntero, variable);
}

void dummy_imprimir(t_valor_variable valor) {
	printf("Imprimir %d\n", valor);
}

void dummy_imprimirTexto(char* texto) {
	printf("ImprimirTexto: %s", texto);
}

AnSISOP_funciones funciones = {
		.AnSISOP_definirVariable		= dummy_definirVariable,
		.AnSISOP_obtenerPosicionVariable= dummy_obtenerPosicionVariable,
		.AnSISOP_dereferenciar			= dummy_dereferenciar,
		.AnSISOP_asignar				= dummy_asignar,
		.AnSISOP_imprimir				= dummy_imprimir,
		.AnSISOP_imprimirTexto			= dummy_imprimirTexto,

};

AnSISOP_kernel funciones_kernel = { };


int main(int argc,char *argv[]) {

	logger = log_create("parser.log", "PARSER",true, LOG_LEVEL_INFO);

	log_info(logger, "Inciando proceso ejemplo de parser..");
	//Primero pruebo levantar y analizar el codigo de un archivo

	//Levanto un archivo ansisop de prueba
	char* programa_ansisop;

	programa_ansisop = "begin\nvariables a, b\na = 3\nb = 5\na = b + 12\nend\n";


	printf("Ejecutando AnalizadorLinea\n");
	printf("================\n");
	printf("El programa de ejemplo es \n%s\n", programa_ansisop);
	printf("================\n");

	t_metadata_program* metadata;
	metadata = metadata_desde_literal(programa_ansisop);

	t_puntero_instruccion inicio;
	int offset;

int i;
	for(i=0; i < metadata->instrucciones_size; i++){
		t_intructions instr = metadata->instrucciones_serializado[i];
		inicio = instr.start;
		offset = instr.offset;
		char* instruccion = malloc(offset+1);
		//instruccion = obtener_cadena(programa_ansisop, inicio, offset);
		memset(instruccion, '\0', offset);
		strncpy(instruccion, &(programa_ansisop[inicio]), offset);
		analizadorLinea(instruccion, &funciones, &funciones_kernel);
		free(instruccion);
	}

   printf("================\n");
	log_destroy(logger);

	return EXIT_SUCCESS;
}

/*
char* obtener_cadena(char* programa_ansisop, t_puntero_instruccion inicio, int offset){
	char* instruccion;
	int i;

	for (i = 0; i < offset; ++i) {
		printf("\nvalue: %c", programa_ansisop[inicio+i]);
	}

	return instruccion;

}*/
