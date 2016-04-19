/*
 ============================================================================
 Name        : swap.c
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

void crearConfiguracion(char* config_path)

int main(char* config_path) {
	puts("Proyecto para Swap"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}

void crearConfiguracion(char* config_path){

	configuracion = *create_config(config_path);

	if(		config_has_property(configuracion, "PUERTO_ESCUCHA")
		&& 	config_has_property(configuracion, "NOMBRE_SWAP")
		&& 	config_has_property(configuracion, "CANTIDAD_PAGINAS")
		&& 	config_has_property(configuracion, "TAMAÑO_PAGINA")
		&& 	config_has_property(configuracion, "RETARDO_COMPACTACION")
	)return;
	else{
		puts("configuracion no valida");
		exit(EXIT_SUCCESS);
	}
}
