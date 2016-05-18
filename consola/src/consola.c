/*
 ============================================================================
 Name        : consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "consola.h"

t_log* logger;

#define PACKAGESIZE 1024 //Define cual es el tamaño maximo del paquete a enviar


/****************************************************************************************/
/*                                   FUNCION MAIN									    */
/****************************************************************************************/
int main(int argc,char *argv[]) {

	t_config_consola* configConsola = malloc(sizeof(t_config_consola));
	int source_size;
	int socket_nucleo;
	FILE *source;

	logger = log_create("consola.log", "PARSER",true, LOG_LEVEL_INFO);

	if(argc != 3){
		fprintf(stderr,"Comando invalido. Usar: consola config_path ansisop_source_path\n");
		return 1;
	}

	source = abrirSource(argv[2], &source_size);
	levantarDatosDeConfiguracion(configConsola, argv[1]);
	socket_nucleo = conectarseA(configConsola->nucleo_ip, configConsola->nucleo_puerto);
	enviar_source(socket_nucleo, source, source_size);

	return EXIT_SUCCESS;
}

/****************************************************************************************/
/*                                     FUNCIONES									    */
/****************************************************************************************/

/*
 *  FUNCION     : Envia código fuente del programa ansisop.
 *  Recibe      : socket del nucleo, puntero al archivo, tamaño del archivo
 *  Devuelve    : void
 */
void enviar_source(int nucleo_fd, FILE *source, int source_size){

	char *archivo = malloc(source_size);
	char *buffer;
	int buffer_size = source_size + 2*sizeof(int);
	int cant_leida;
	int cant_enviada = 0;
	int aux;
	int mandoArchivo = 2001;

	//prueba envio mensaje simple a nucleo
	/*
	//char* mensaje = "Hello\0";
	if (mensaje[0] != '\0'){
		aux = send(nucleo_fd, mensaje, 7, 0);
		printf("msj a enviar %s",mensaje);
		if(aux == -1){
	    	log_error_y_cerrar_logger(logger, "Error al enviar archivo.");
			exit(1);
		}

	}*/

	cant_leida = fread(archivo, sizeof(char), source_size, source);

	//leo hasta que termine completamente
	while(cant_leida < source_size){

		//Leo a partir de la posicion que venia leyendo, la cantidad maxima menos lo que leyo.
		aux = fread(archivo + cant_leida, sizeof(char), source_size - cant_leida, source);

		if (aux == -1){
		    log_error_y_cerrar_logger(logger, "Error de lectura.");
			exit(1);
		}
		cant_leida += aux;
	}//ya tengo todo el texto en *archivo.

	//Creo el paquete con toda la info: cod op, tam archivo, archivo
	buffer = malloc(source_size + 2*sizeof(int));

	memcpy(buffer, &source_size, sizeof(int));
	memcpy(buffer + sizeof(int), &mandoArchivo, sizeof(int));
	memcpy(buffer + 2*sizeof(int), archivo, source_size);

	//Ahora envio el archivo
	cant_enviada = send(nucleo_fd, buffer, buffer_size, 0);

	while(cant_enviada < buffer_size){
		aux = send(nucleo_fd, buffer + cant_enviada, buffer_size - cant_enviada, 0);
		cant_enviada += aux;
	}

	//ya tengo todo el archivo enviado
	free(archivo);
	free(buffer);

}

/*
 *  FUNCION     : Abre el archivo del programa ansisop y obtiene el tamaño en del mismo en bytes.
 *  Recibe      : path del archivo, puntero en donde se almacenará el tamaño del archivo
 *  Devuelve    : puntero a FILE
 */
FILE *abrirSource(char *path, int *source_size){

	FILE *source;
	struct stat file_info;

	source = fopen(path, "r");

	if (source == NULL){
	    log_error_y_cerrar_logger(logger, "Error al abrir el archivo fuente.");
		exit(0);
	}

	if(stat(path, &file_info) == -1){
	    log_error_y_cerrar_logger(logger, "Error con stat.");
	    exit(0);
	}

	//Me fijo el tamanio del archivo en bytes.
	*source_size = file_info.st_size;
	log_info(logger, "Tamanio archivo(bytes): %i", *source_size);

	return source;
}

/*
 *  FUNCION     : Carga los datos de configuración de la consola. Valida parametros obtenidos.
 *  Recibe      : estructura de configuracion para la consola, path del archivo de configuración
 *  Devuelve    : void
 */
void levantarDatosDeConfiguracion(t_config_consola* configConsola, char* config_path){

	t_config* config = config_create(config_path);

	if(validarParametrosDeConfiguracion(config)){

		log_info(logger, "El archivo de configuración tiene todos los parametros requeridos.");

		char* nucleo_ip = config_get_string_value(config,"NUCLEO_IP");
		configConsola->nucleo_ip = malloc(strlen(nucleo_ip));
		memcpy(configConsola->nucleo_ip, nucleo_ip, strlen(nucleo_ip));

		char* nucleo_puerto = config_get_string_value(config,"NUCLEO_PUERTO");
		configConsola->nucleo_puerto = malloc(strlen(nucleo_puerto)+1);
		memcpy(configConsola->nucleo_puerto, nucleo_puerto, strlen(nucleo_puerto));
		configConsola->nucleo_puerto[strlen(nucleo_puerto)] = '\0';

		config_destroy(config);
	}else{
	    log_error_y_cerrar_logger(logger, "Configuracion invalida.");
	    config_destroy(config);
		exit(EXIT_FAILURE);
	}
}

/*
 *  FUNCION     : Valida parámetros de configuración necesarios para el proceso Consola
 *  Recibe      : puntero a estructura de configuración
 *  Devuelve    : bool
 */
bool validarParametrosDeConfiguracion(t_config* config){
	return 	config_has_property(config, "NUCLEO_IP")
		&&	config_has_property(config, "NUCLEO_PUERTO");
}

/*
 *  FUNCION     : Se conecta al ip y puerto ingresado por parámetro
 *  Recibe      : puntero a ip y puerto
 *  Devuelve    : int
 */
int conectarseA(char* ip, char* puerto){

    struct addrinfo hints, *serverInfo;
	int resultado_getaddrinfo;
	int socket_conexion;

	memset(&hints, '\0', sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	resultado_getaddrinfo = getaddrinfo(ip, puerto, &hints, &serverInfo);

	if(resultado_getaddrinfo != 0){
		fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(resultado_getaddrinfo));
		return 2;
	}

	socket_conexion = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if( connect(socket_conexion, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1){
	    log_error_y_cerrar_logger(logger, "Fallo al conectar.");
		exit(1);
	}

	handshakeConNucleo(socket_conexion);

	log_info(logger, "Conectando al NUCLEO. IP y puerto del núcleo: %s - %s", ip, puerto);

// Comento esto, ya no creo que lo vayamos a usar mas que para probar.
//	int enviar = 1;
//	char message[PACKAGESIZE];

//printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");

//	while(enviar){
//		fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
//		if (!strcmp(message,"exit\n")) enviar = 0;			// Chequeo que el usuario no quiera salir
//		if (enviar) send(socket_conexion, message, strlen(message) + 1, 0); 	// Solo envio si el usuario no quiere salir.
//	}
//	close(socket_conexion);

	return socket_conexion;
}

/*
 *  FUNCION     : Realiza el handshake con el nucleo
 *  Recibe      : socket del nucleo
 *  Devuelve    : void
 */
void handshakeConNucleo(int nucleo_fd){

	int msj_recibido;
	int soy_consola = 2000;

	recv(nucleo_fd, &msj_recibido, sizeof(int), 0);
	printf("socket: %d, mensaje %d\n", nucleo_fd,msj_recibido);

	if(msj_recibido == 1000){
		log_info(logger, "Nucleo validado.");
		send(nucleo_fd, &soy_consola, sizeof(int), 0);
		printf("send socket: %d, mensaje %d\n", nucleo_fd,soy_consola);
	}else{
	    log_error_y_cerrar_logger(logger, "El nucleo no pudo ser validado.");
		exit(0);
	}
}
