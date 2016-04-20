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

t_config* configuracion;

int main(int argc,char *argv[]) {
	puts("Proyecto para Nucleo"); /* prints !!!Hello World!!! */

	crearConfiguracion(argv[1]);

	return EXIT_SUCCESS;
}

void crearConfiguracion(char *config_path){

	configuracion = config_create(config_path);

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

	return (	config_has_property(configuracion, "PUERTO_PROG")
			&& 	config_has_property(configuracion, "PUERTO_CPU")
			&& 	config_has_property(configuracion, "QUANTUM")
			&& 	config_has_property(configuracion, "QUANTUM_SLEEP")
			&& 	config_has_property(configuracion, "SEM_IDS")
			&& 	config_has_property(configuracion, "SEM_INIT")
			&& 	config_has_property(configuracion, "IO_IDS")
			&& 	config_has_property(configuracion, "IO_SLEEP")
			&& 	config_has_property(configuracion, "SHARED_VARS"));
}
