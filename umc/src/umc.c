/*
 ============================================================================
 Name        : umc.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "commons/config.h"

void crearConfiguracion(char* config_path);

t_config configuracion;

int main(char* config_path) {
	puts("Proyecto para UMC"); /* prints !!!Hello World!!! */

	crearConfiguracion(config_path);

	return EXIT_SUCCESS;
}

void crearConfiguracion(char* config_path){

	configuracion = *create_config(config_path);

	if(		config_has_property(configuracion, "PUERTO")
		&& 	config_has_property(configuracion, "IP_SWAP")
		&& 	config_has_property(configuracion, "PUERTO_SWAP")
		&& 	config_has_property(configuracion, "MARCOS")
		&& 	config_has_property(configuracion, "MARCOS_SIZE")
		&& 	config_has_property(configuracion, "MARCO_X_PROC")
		&& 	config_has_property(configuracion, "ENTRADAS_TLB")
		&& 	config_has_property(configuracion, "RETARDO")
	)return;
	else{
		puts("configuracion no valida");
		exit(EXIT_SUCCESS);
	}
}
