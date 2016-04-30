/*
 ============================================================================
 Name        : cpu.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "cpu.h"

t_config* config;
t_log* logger;

int main(int argc,char *argv[]) {

	char* nucleo_ip;
	char* nucleo_puerto;
	char* umc_ip;
	char* umc_puerto;

	int socket_nucleo, socket_umc;

	logger = log_create("cpu.log", "CPU",true, LOG_LEVEL_INFO);

    log_info(logger, "Inciando proceso CPU..");
	crearConfiguracion(argv[1]);

	nucleo_ip = config_get_string_value(config, "NUCLEO_IP");
	nucleo_puerto = config_get_string_value(config, "NUCLEO_PUERTO");
	umc_ip = config_get_string_value(config, "UMC_IP");
	umc_puerto = config_get_string_value(config, "UMC_PUERTO");

	log_info(logger, "IP y puerto del núcleo: %s - %s", nucleo_ip, nucleo_puerto);
	log_info(logger, "IP y puerto de la umc: %s - %s", umc_ip, umc_puerto);

	//Conexión al nucleo
	log_info(logger, "Conectando al nucleo. IP y puerto del núcleo: %s - %s", nucleo_ip, nucleo_puerto);
	socket_nucleo = conectarseA(nucleo_ip, nucleo_puerto);
	log_info(logger, "Conexion establecida con el nucleo. (Socket: %d)",socket_nucleo);

	//Conexión al umc

	log_info(logger, "Conectando a la umc. IP y puerto del núcleo: %s - %s", umc_ip, umc_puerto);
	socket_umc = conectarseA(umc_ip, umc_puerto);
	log_info(logger, "Conexion establecida con la umc. (Socket: %d)",socket_umc);

	int enviar = 1;
	int status = 1;
	char message[PACKAGESIZE];

	while(enviar && status !=0){
	 	fgets(message, PACKAGESIZE, stdin);
		if (!strcmp(message,"exit\n")) enviar = 0;
		if (enviar) send(socket_umc, message, strlen(message) + 1, 0);

		//Recibo mensajes del servidor
		memset (message,'\0',PACKAGESIZE);
		status = recv(socket_umc, (void*) message, PACKAGESIZE, 0);
		if (status != 0) printf("%s", message);
	}

	log_info(logger, "Cierro conexiones.",socket_nucleo);
	cerrarConexionSocket(socket_nucleo);
	cerrarConexionSocket(socket_umc);

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
			&& 	config_has_property(config, "UMC_PUERTO"));
}
