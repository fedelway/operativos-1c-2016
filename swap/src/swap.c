/*
 ============================================================================
 Name        : swap.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#include "swap.h"


t_config* config;

#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar


int main(int argc,char *argv[]) {

	char* puerto_escucha;

	printf("Proyecto para Swap\n");

	crearConfiguracion(argv[1]);

	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	conectarPuertoDeEscucha(puerto_escucha);
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

//Valida que todos los parámetros existan en el archivo de configuración
bool validarParametrosDeConfiguracion(){

	return (	config_has_property(config, "PUERTO_ESCUCHA")
			&& 	config_has_property(config, "NOMBRE_SWAP")
			&& 	config_has_property(config, "CANTIDAD_PAGINAS")
			&& 	config_has_property(config, "TAMANIO_PAGINA")
			&& 	config_has_property(config, "RETARDO_COMPACTACION"));
}
