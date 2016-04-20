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
bool validarParametrosDeConfiguracion();

t_config config;

int main(int argc,char *argv[]) {
	puts("Proyecto para UMC"); /* prints !!!Hello World!!! */

	crearConfiguracion(argv[1]);

	return EXIT_SUCCESS;
}

void crearConfiguracion(char* config_path){

	config = *create_config(config_path);

	if(validarParametrosDeConfiguracion()){
		printf("El archivo de configuracion tiene todos los parametros requeridos.\n");
	}else{
		puts("configuracion no valida");
		exit(EXIT_SUCCESS);
	}
}

bool validarParametrosDeConfiguracion(){
	return 	config_has_property(config, "PUERTO")
		&& 	config_has_property(config, "IP_SWAP")
		&& 	config_has_property(config, "PUERTO_SWAP")
		&& 	config_has_property(config, "MARCOS")
		&& 	config_has_property(config, "MARCOS_SIZE")
		&& 	config_has_property(config, "MARCO_X_PROC")
		&& 	config_has_property(config, "ENTRADAS_TLB")
		&& 	config_has_property(config, "RETARDO");
}
