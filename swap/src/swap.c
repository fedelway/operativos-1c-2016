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

t_config* config;

void crearConfiguracion(char* config_path); //levanta el archivo de configuracion y lo asigna a una estructura t_config
bool validarParametrosDeConfiguracion(); //Valida que el archivo de configuracion tenga todos los parametros requeridos

int main(int argc,char *argv[]) {

	int cfg_puerto_escucha, cfg_cantidad_paginas, cfg_tamanio_pagina, cfg_retardo_compactacion;
	char* cfg_nombre_swap;

	printf("Proyecto para Swap\n");

	crearConfiguracion(argv[1]);

	//Ejemplos para obtener los valores de configuración (Solo por ahora)
	cfg_puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
	printf("El puerto de escucha es el %d\n", cfg_puerto_escucha);

	cfg_nombre_swap = config_get_string_value(config, "NOMBRE_SWAP");
	printf("El nombre del swap es %s\n", cfg_nombre_swap);

	cfg_cantidad_paginas = config_get_int_value(config, "CANTIDAD_PAGINAS");
	printf("La cantidad de paginas es de %d\n", cfg_cantidad_paginas);

	cfg_tamanio_pagina = config_get_int_value(config, "TAMANIO_PAGINA");
	printf("El tamanio de una paginas es de %d\n", cfg_tamanio_pagina);

	cfg_retardo_compactacion = config_get_int_value(config, "RETARDO_COMPACTACION");
	printf("El retardo de compactacion es de %d\n", cfg_retardo_compactacion);

	return EXIT_SUCCESS;
}

//levanta los datos de configuracion en una estructura t_struct
void crearConfiguracion(char* config_path){

	config = config_create(config_path);

	if(validarParametrosDeConfiguracion()){
		printf("El archivo de configuracion tiene todos los parametros requeridos.\n");
		return;
	}else{
		printf("Archivo de configuracion no valido.");
		exit(EXIT_FAILURE);
	}
}

//Valida que todos los parámetros existan en el archivo de configuración
bool validarParametrosDeConfiguracion(){

	return (	config_has_property(config, "PUERTO_ESCUCHA")
			&& 	config_has_property(config, "NOMBRE_SWAP")
			&& 	config_has_property(config, "CANTIDAD_PAGINAS")
			&& 	config_has_property(config, "TAMANIO_PAGINA")
			&& 	config_has_property(config, "RETARDO_COMPACTACION"));
}
