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
bool validarParametrosDeConfiguracion();

int main(int argc,char *argv[]) {
	puts("Proyecto para proceso Consola"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}

void crearConfiguracion(char* config_path){

	configuracion = *create_config(config_path);

	if(validarParametrosDeConfiguracion()){
		printf("El archivo de configuracion tiene todos los parametros requeridos.\n");
	}else{
		printf("configuracion no valida");
		exit(EXIT_SUCCESS);
	}
}

bool validarParametrosDeConfiguracion(){
	return config_has_property(configuracion, "PROGRAMA_ANSISOP");
}
