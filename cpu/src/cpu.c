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
#include "commons/log.h"
#include "commons/string.h"

void crearConfiguracion(); //creo la configuracion y checkeo que sea valida
bool validarParametrosDeConfiguracion();

t_config* config;
t_log* logger;

int main(int argc,char *argv[]) {

	char* nucleo_ip;
	char* nucleo_puerto;
	char* umc_ip;
	char* umc_puerto;

	//Puertos propios de la CPU desde donde se conecta a los demás procesos
	char* puerto_conexion_nucleo;
	char* puerto_conexion_umc;

	logger = log_create("cpu.log", "CPU",true, LOG_LEVEL_INFO);

    log_info(logger, "Proyecto para CPU..");
	crearConfiguracion(argv[1]);

	nucleo_ip = config_get_string_value(config, "NUCLEO_IP");
	nucleo_puerto = config_get_string_value(config, "NUCLEO_PUERTO");
	umc_ip = config_get_string_value(config, "UMC_IP");
	umc_puerto = config_get_string_value(config, "UMC_PUERTO");
	puerto_conexion_nucleo = config_get_string_value(config, "PUERTO_PARA_NUCLEO");
	puerto_conexion_umc = config_get_string_value(config, "PUERTO_PARA_UMC");

	log_info(logger, "Me conecto al nucleo a través del puerto: %s", puerto_conexion_nucleo);
	log_info(logger, "Me conecto a la umc a través del puerto: %s", puerto_conexion_umc);
	log_info(logger, "IP y puerto del núcleo: %s - %s", nucleo_ip, nucleo_puerto);
	log_info(logger, "IP y puerto de la umc: %s - %s", umc_ip, umc_puerto);

	log_destroy(logger);

	return EXIT_SUCCESS;
}


void crearConfiguracion(char* config_path){

	config = config_create(config_path);

	if (validarParametrosDeConfiguracion()){
	 log_info(logger, "El archivo de configuración tiene todos los parametros requeridos.");
	 return;
	}else{
		log_warning(logger, "LOG A NIVEL %s de prueba", "WARNING");
	    log_error(logger, "Configuración no valida");
	    log_destroy(logger);
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
