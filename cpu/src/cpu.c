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

t_log* logger;
int socket_umc;

int main(int argc,char *argv[]) {

	t_configuracion_cpu* configCPU = malloc(sizeof(t_configuracion_cpu));

	int socket_nucleo, socket_umc;

	logger = log_create("cpu.log", "CPU",true, LOG_LEVEL_INFO);

    log_info(logger, "Inciando proceso CPU..");
	levantarDatosDeConfiguracion(configCPU, argv[1]);
//	crearConfiguracion(argv[1]);

	//Conexión al nucleo
	log_info(logger, "Conectando al nucleo. IP y puerto del núcleo: %s - %s", configCPU->nucleo_ip, configCPU->nucleo_puerto);
	socket_nucleo = conectarseA(configCPU->nucleo_ip, configCPU->nucleo_puerto);
	log_info(logger, "Conexion establecida con el nucleo. (Socket: %d)",socket_nucleo);
	log_info(logger, "Realizando handshake con el nucleo.");
	validarNucleo(socket_nucleo);

	//Conexión al umc
	log_info(logger, "Conectando a la umc. IP y puerto del núcleo: %s - %s", configCPU->umc_ip, configCPU->umc_puerto);
	socket_umc = conectarseA(configCPU->umc_ip, configCPU->umc_puerto);
	log_info(logger, "Conexion establecida con la umc. (Socket: %d)",socket_umc);

	char message[PACKAGESIZE];
	memset (message,'\0',PACKAGESIZE);
	recibirMensajeNucleo(&message, socket_nucleo);
	printf("recibido del nucleo: %s\n", message);

	//Reenviar el msj recibido del nucleo a la UMC
	enviarPaqueteAUMC(message, socket_umc);

	//TODO - Ejecutar siguiente instrucción a partir del program counter del pcb
//Momentaneamente, obtengo metadata_desde_literal un programa de ejemplo y analizo cada instruccion.
	//TODO - Después, eliminar el metadata_desde_literal de la cpu. Tiene que obtener el pcb del nucleo.


	//Levanto un archivo ansisop de prueba
	char* programa_ansisop;

	programa_ansisop = "begin\nvariables a, b\na = 3\nb = 5\na = b + 12\nend\n";

	printf("Ejecutando AnalizadorLinea\n");
	printf("================\n");
	printf("El programa de ejemplo es \n%s\n", programa_ansisop);
	printf("================\n\n");
	printf("Ejecutando metadata_desde_literal\n");

	t_metadata_program* metadata;
	metadata = metadata_desde_literal(programa_ansisop);

	int i;
	for(i=0; i < metadata->instrucciones_size; i++){
		ejecutoInstruccion(programa_ansisop, metadata, i);
	}
	printf("================\n\n");
	printf("Terminé de ejecutar todas las instrucciones\n");

	int enviar = 1;
	int status = 1;

	/*while(enviar && status !=0){
	 	fgets(message, PACKAGESIZE, stdin);
		if (!strcmp(message,"exit\n")) enviar = 0;
		if (enviar) send(socket_umc, message, strlen(message) + 1, 0);

		//Recibo mensajes del servidor
		memset (message,'\0',PACKAGESIZE);
		status = recv(socket_umc, (void*) message, PACKAGESIZE, 0);
		if (status != 0) printf("%s", message);
	}*/

	log_info(logger, "Cierro conexiones.",socket_nucleo);
	cerrarConexionSocket(socket_nucleo);
	cerrarConexionSocket(socket_umc);

	log_destroy(logger);

	return EXIT_SUCCESS;
}

void recibirMensajeNucleo(char* message, int socket_nucleo){

	printf("voy a recibir mensaje de %d\n", socket_nucleo);
	int status = 0;
	//while(status == 0){
	status = recv(socket_nucleo, message, PACKAGESIZE, 0);
		if (status != 0){
			printf("recibo mensaje de nucleo %d\n", status);
			printf("recibi este mensaje: %s\n", message);
			status = 0;
		}else{
			sleep(1);
			printf(".\n");
		}

	//}
}

void enviarPaqueteAUMC(char* message, int socket){
	//Envio el archivo entero


	int resultSend = send(socket, message, PACKAGESIZE, 0);
	printf("resultado send %d, a socket %d \n",resultSend, socket);
	if(resultSend == -1){
		printf ("Error al enviar archivo a UMC.\n");
		exit(1);
	}else {
		printf ("Archivo enviado a UMC.\n");
		printf ("mensaje: %s\n", message);
	}
}

/*
 *  FUNCION     : Valida parámetros de configuración necesarios para el proceso CPU
 *  Recibe      : puntero a estructura de configuración
 *  Devuelve    : bool
 */
bool validarParametrosDeConfiguracion(t_config* config){
	return (	config_has_property(config, "NUCLEO_IP")
			&&  config_has_property(config, "NUCLEO_PUERTO")
			&& 	config_has_property(config, "UMC_IP")
			&& 	config_has_property(config, "UMC_PUERTO"));
}

void validarNucleo(int nucleo_fd){

	int msj_recibido;
	int soy_cpu = 3000;

	//Tengo que recibir el ID del nucleo = 1000
	recv(nucleo_fd, &msj_recibido, sizeof(int), 0);

	if(msj_recibido == 1000){
		log_info(logger, "Nucleo validado.");
		send(nucleo_fd, &soy_cpu, sizeof(int), 0); //Envio ID de la CPU
	}else{
		log_error(logger, "El nucleo no pudo ser validado.");
	    log_destroy(logger);
		exit(0);
	}
}

void ejecutoInstruccion(char* programa_ansisop, t_metadata_program* metadata, int numeroDeInstruccion){
	t_puntero_instruccion inicio;
	int offset;

	t_intructions instr = metadata->instrucciones_serializado[numeroDeInstruccion];
	inicio = instr.start;
	offset = instr.offset;
	char* instruccion = malloc(offset+1);
	//instruccion = obtener_cadena(programa_ansisop, inicio, offset);
	memset(instruccion, '\0', offset);
	strncpy(instruccion, &(programa_ansisop[inicio]), offset);
	analizadorLinea(instruccion, &funciones, &funciones_kernel);
	free(instruccion);
}

void reciboPcbDeNucleo(){

}

void reciboMensaje(){
	//Estructura para crear el header + piload
	typedef struct{
		int id;
		int tamanio;
	}t_header;

	typedef struct{
	  t_header header;
	  char* paiload;
	}t_package;

}


/*
 *  FUNCION     : Carga los datos de configuración de la cpu. Valida parametros obtenidos.
 *  Recibe      : estructura de configuracion para la cpu, path del archivo de configuración
 *  Devuelve    : void
 */
void levantarDatosDeConfiguracion(t_configuracion_cpu* configuracion, char* config_path){

	t_config* config = config_create(config_path);

	if(validarParametrosDeConfiguracion(config)){

		log_info(logger, "El archivo de configuración tiene todos los parametros requeridos.");

		char* nucleo_ip = config_get_string_value(config,"NUCLEO_IP");
		configuracion->nucleo_ip = malloc(strlen(nucleo_ip));
		memcpy(configuracion->nucleo_ip, nucleo_ip, strlen(nucleo_ip));
		configuracion->nucleo_ip[strlen(nucleo_ip)] = '\0';

		char* nucleo_puerto = config_get_string_value(config,"NUCLEO_PUERTO");
		configuracion->nucleo_puerto = malloc(strlen(nucleo_puerto)+1);
		memcpy(configuracion->nucleo_puerto, nucleo_puerto, strlen(nucleo_puerto));
		configuracion->nucleo_puerto[strlen(nucleo_puerto)] = '\0';

		char* umc_ip = config_get_string_value(config,"UMC_IP");
		configuracion->umc_ip = malloc(strlen(umc_ip));
		memcpy(configuracion->umc_ip, umc_ip, strlen(umc_ip));
		configuracion->umc_ip[strlen(umc_ip)] = '\0';

		char* umc_puerto = config_get_string_value(config,"UMC_PUERTO");
		configuracion->umc_puerto = malloc(strlen(umc_puerto)+1);
		memcpy(configuracion->umc_puerto, umc_puerto, strlen(umc_puerto));
		configuracion->umc_puerto[strlen(umc_puerto)] = '\0';

		config_destroy(config);
	}else{
	    log_error_y_cerrar_logger(logger, "Configuracion invalida.");
	    config_destroy(config);
		exit(EXIT_FAILURE);
	}
}
