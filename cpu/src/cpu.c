/*
 ============================================================================
 Name        : cpu.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h> //Para la estructura timeval del select
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // for close conections
#include "commons/config.h"

void crearConfiguracion(); //creo la configuracion y checkeo que sea valida
bool validarParametrosDeConfiguracion();

t_config* config;

int main(int argc,char *argv[]) {

	char* nucleo_ip;
	char* nucleo_puerto;
	char* umc_ip;
	char* umc_puerto;

	//Puertos propios de la CPU desde donde se conecta a los demás procesos
	char* puerto_conexion_nucleo;
	char* puerto_conexion_umc;

	printf("Proyecto para CPU..\n");

	crearConfiguracion(argv[1]);

	nucleo_ip = config_get_string_value(config, "NUCLEO_IP");
	nucleo_puerto = config_get_string_value(config, "NUCLEO_PUERTO");
	umc_ip = config_get_string_value(config, "UMC_IP");
	umc_puerto = config_get_string_value(config, "UMC_PUERTO");
	puerto_conexion_nucleo = config_get_string_value(config, "PUERTO_PARA_NUCLEO");
	puerto_conexion_umc = config_get_string_value(config, "PUERTO_PARA_UMC");

	printf("Me conecto al nucleo a través del puerto: %s\n", puerto_conexion_nucleo);
	printf("Me conecto a la umc a través del puerto: %s\n", puerto_conexion_umc);
	printf("IP y puerto del núcleo: %s - %s\n", nucleo_ip, nucleo_puerto);
	printf("IP y puerto de la umc: %s - %s\n", umc_ip, umc_puerto);

	return EXIT_SUCCESS;
}


void crearConfiguracion(char* config_path){

	config = config_create(config_path);

	if (validarParametrosDeConfiguracion()){
	 printf("El archivo de configuración tiene todos los parametros requeridos.\n");
	 return;
	}else{
		printf("Configuración no valida\n");
		exit(EXIT_FAILURE);
	}
}

//Validar que todos los parámetros existan en el archivo de configuracion
bool validarParametrosDeConfiguracion(){
	return (	config_has_property(config, "NUCLEO_IP")
			&&  config_has_property(config, "NUCLEO_PUERTO")
			&& 	config_has_property(config, "UMC_IP")
			&& 	config_has_property(config, "UMC_PUERTO")
			&& 	config_has_property(config, "PUERTO_PARA_NUCLEO")
			&& 	config_has_property(config, "PUERTO_PARA_UMC"));
}
