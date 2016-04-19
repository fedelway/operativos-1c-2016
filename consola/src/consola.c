/*
 ============================================================================
 Name        : consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "commons/config.h"

t_config configuracion;

void crearConfiguracion(char* config_path);

int main(void) {
	puts("Proyecto para proceso Consola"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}

void crearConfiguracion(char* config_path){

	configuracion = *create_config(config_path);

	if(config_has_property(configuracion, "PROGRAMA_ANSISOP"))
		return;
	else{
		puts("configuracion no valida");
		exit(EXIT_SUCCESS);
	}
}
