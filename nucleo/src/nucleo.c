/*
 ============================================================================
 Name        : nucleo.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "commons/config.h"

void crearConfiguracion(char* config); //creo la configuracion y checkeo que sea valida

t_config configuracion;

int main(char* config_path) {
	puts("Proyecto para Nucleo"); /* prints !!!Hello World!!! */

	crearConfiguracion(config_path);

	return EXIT_SUCCESS;
}

void crearConfiguracion(char* config){

	configuracion = *config_create(config);

	if(		config_has_property(configuracion, "PUERTO_PROG")
		&& 	config_has_property(configuracion, "PUERTO_CPU")
		&& 	config_has_property(configuracion, "QUANTUM")
		&& 	config_has_property(configuracion, "QUANTUM_SLEEP")
		&& 	config_has_property(configuracion, "SEM_IDS")
		&& 	config_has_property(configuracion, "SEM_INIT")
		&& 	config_has_property(configuracion, "IO_IDS")
		&& 	config_has_property(configuracion, "IO_SLEEP")
		&& 	config_has_property(configuracion, "SHARED_VARS")
	)return;
	else{
		puts("configuracion no valida");
		exit(EXIT_SUCCESS);
	}
}
