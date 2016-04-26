/*
 ============================================================================
 Name        : umc.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#include "umc.h"

t_config *config;

#define PACKAGESIZE 1024 //Define cual es el tamaño maximo del paquete a enviar


int main(int argc,char *argv[]) {

	char* swap_ip;
	char* swap_puerto;

	if(argc != 2){
		fprintf(stderr,"uso: umc config_path\n");
		return 1;
	}

	crearConfiguracion(argv[1]);

	swap_ip = config_get_string_value(config,"IP_SWAP");
	swap_puerto = config_get_string_value(config,"PUERTO_SWAP");

	conectarseA(swap_ip, swap_puerto);

	return EXIT_SUCCESS;
}

void crearConfiguracion(char* config_path){

	config = config_create(config_path);

	if(validarParametrosDeConfiguracion()){
		printf("El archivo de configuracion tiene todos los parametros requeridos.\n");
	}else{
		printf("configuracion no valida");
		exit(EXIT_SUCCESS);
	}
}

bool validarParametrosDeConfiguracion(){
	return 	config_has_property(config, "PUERTO")
		&& 	config_has_property(config, "IP_SWAP")
		&& 	config_has_property(config, "PUERTO_SWAP")
		&& 	config_has_property(config, "MARCOS")
		&& 	config_has_property(config, "MARCO_SIZE")
		&& 	config_has_property(config, "MARCO_X_PROC")
		&& 	config_has_property(config, "ENTRADAS_TLB")
		&& 	config_has_property(config, "RETARDO");
}
