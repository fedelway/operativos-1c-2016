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

void crearConfiguracion(); //creo la configuracion y checkeo que sea valida
bool validarParametrosDeConfiguracion();

t_config* config;

int main(int argc,char *argv[]) {
	puts("Proyecto para Nucleo"); /* prints !!!Hello World!!! */

	crearConfiguracion(argv[1]);

	return EXIT_SUCCESS;
}

void crearConfiguracion(char *config_path){

	config = config_create(config_path);

	if (validarParametrosDeConfiguracion()){
	 printf("Tiene todos los parametros necesarios");
	 return;
	}else{
		printf("configuracion no valida");
		exit(EXIT_FAILURE);
	}
}

//Validar que todos los parámetros existan en el archivo de configuracion

bool validarParametrosDeConfiguracion(){

	return (	config_has_property(config, "PUERTO_PROG")
			&& 	config_has_property(config, "PUERTO_CPU")
			&& 	config_has_property(config, "QUANTUM")
			&& 	config_has_property(config, "QUANTUM_SLEEP")
			&& 	config_has_property(config, "SEM_IDS")
			&& 	config_has_property(config, "SEM_INIT")
			&& 	config_has_property(config, "IO_IDS")
			&& 	config_has_property(config, "IO_SLEEP")
			&& 	config_has_property(config, "SHARED_VARS"));
}
