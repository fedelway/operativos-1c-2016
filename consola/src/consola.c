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

t_log* logger, *logger_sin_imprimir;

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
	logger_sin_imprimir = log_create("consola.log", "PARSER",false, LOG_LEVEL_INFO);

	if(argc != 3){
		fprintf(stderr,"Comando invalido. Usar: consola config_path ansisop_source_path\n");
		return 1;
	}

	source = abrirSource(argv[2], &source_size);
	levantarDatosDeConfiguracion(configConsola, argv[1]);
	socket_nucleo = conectarseA(configConsola->nucleo_ip, configConsola->nucleo_puerto);

	handshakeConNucleo(socket_nucleo);
	log_info(logger, "Conectando al NUCLEO. IP y puerto del núcleo: %s - %s", configConsola->nucleo_ip, configConsola->nucleo_puerto);

	enviar_source(socket_nucleo, source, source_size);

	int status = 0;
	int header_mensaje;
	while(1){

		status = recv(socket_nucleo, &header_mensaje, sizeof(int), 0);

		//Detecto desconexion
		if(status <= 0)
		{
			printf("Desconexion del nucleo. Terminando...\n");
			close(socket_nucleo);
			exit(0);
		}

		switch (header_mensaje) {

			case IMPRIMIR_VALOR:
				imprimirValor(socket_nucleo);
				break;

			case IMPRIMIR_CADENA:
				imprimirCadena(socket_nucleo);
				break;

			case FINALIZAR:
				printf("Fin programa.\n");
				exit(0);


			default:
				printf("obtuve otro id %d\n", header_mensaje);
				break;
		}

	}

	return EXIT_SUCCESS;
}

/****************************************************************************************/
/*                            CONFIGURACION Y CONEXIONES								*/
/****************************************************************************************/

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
 *  FUNCION     : Realiza el handshake con el nucleo
 *  Recibe      : socket del nucleo
 *  Devuelve    : void
 */
void handshakeConNucleo(int nucleo_fd){

	int msj_recibido;
	int mensaje = SOY_CONSOLA;

	recv(nucleo_fd, &msj_recibido, sizeof(int), 0);


	if(msj_recibido == SOY_NUCLEO){
		log_info(logger, "Nucleo validado.");
		send(nucleo_fd, &mensaje, sizeof(int), 0);
		printf("send socket: %d, mensaje %d\n", nucleo_fd, mensaje);
	}else{
		printf("socket: %d, mensaje %d\n", nucleo_fd,msj_recibido);
	    log_error_y_cerrar_logger(logger, "El nucleo no pudo ser validado.");
		exit(0);
	}
}


/****************************************************************************************/
/*                                 FUNCIONES CONSOLA								    */
/****************************************************************************************/

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
	int mandoArchivo = ENVIO_FUENTE;
	//int mandoArchivo = ENVIO_FUENTE;

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

	memcpy(buffer, &mandoArchivo, sizeof(int));
	memcpy(buffer + sizeof(int), &source_size, sizeof(int));
	memcpy(buffer + 2*sizeof(int), archivo, source_size);

	//Ahora envio el archivo
	cant_enviada = send(nucleo_fd, buffer, buffer_size, 0);

	while(cant_enviada < buffer_size){
		aux = send(nucleo_fd, buffer + cant_enviada, buffer_size - cant_enviada, 0);
		cant_enviada += aux;
	}

	printf("Archivo enviado satisfactoriamente.\n");

	//ya tengo todo el archivo enviado
	free(archivo);
	free(buffer);

}

/*
 *  FUNCION     : Recibe un valor del nucleo para mostrar por pantalla
 *  Recibe      : void
 *  Devuelve    : void
 */
void imprimirValor(int socket_nucleo){

	int valor;

	recv(socket_nucleo,&valor,sizeof(int),0);

	printf("%d.\n",valor);
}

/*
 *  FUNCION     : Recibe una cadena del nucleo para mostrar por pantalla
 *  Recibe      : void
 *  Devuelve    : void
 */
void imprimirCadena(int socket_nucleo){

	int tamanio_cadena;
	char *cadena;

	recv(socket_nucleo,&tamanio_cadena,sizeof(int),0);

	cadena = malloc(tamanio_cadena + 1);

	recv(socket_nucleo,cadena,tamanio_cadena,0);

	cadena[tamanio_cadena] = '\0';

	//Saco el \n de la cadena, creo que es accidental esto...
	if(cadena[tamanio_cadena - 1] == '\n')
		cadena[tamanio_cadena - 1] = '\0';

	printf("%s.\n",cadena);

	free(cadena);
}
