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
int tamanio_pagina, quantum;
int socket_umc, socket_nucleo;

/****************************************************************************************/
/*                                   FUNCION MAIN									    */
/****************************************************************************************/
int main(int argc,char *argv[]) {

	t_configuracion_cpu* configCPU = malloc(sizeof(t_configuracion_cpu));

	logger = log_create("cpu.log", "CPU",true, LOG_LEVEL_INFO);

	//valida los parametros de entrada del main
	if (argc != 2) {
	    log_error_y_cerrar_logger(logger, "Uso: cpu config_path (faltan parametros de entrada)\n");
		return EXIT_FAILURE;
	}

    log_info(logger, "Inciando proceso CPU..");
	levantarDatosDeConfiguracion(configCPU, argv[1]);

	//Conexión al nucleo
	conectarAlNucleo(configCPU);

	//Conexión al umc
	conectarAUMC(configCPU);

	char message[PACKAGESIZE];
	memset (message,'\0',PACKAGESIZE);
	//recibirPCB(message);
	printf("recibido del nucleo: %s\n", message);

	//Reenviar el msj recibido del nucleo a la UMC
//	enviarPaqueteAUMC(message);

	//TODO - Ejecutar siguiente instrucción a partir del program counter del pcb
	//Momentaneamente, obtengo metadata_desde_literal un programa de ejemplo y analizo cada instruccion.
	//TODO - Después, eliminar el metadata_desde_literal de la cpu. Tiene que obtener el pcb del nucleo.

	//Levanto un archivo ansisop de prueba
	char* programa_ansisop;

	programa_ansisop = "begin\nvariables i,b\ni = 1\n:ttttt\n:hola\ni = i + 1\nprint i\nb = i - 10\njnz b hola\ni = 1\ni = i + 1\nprint i\nb = i - 10\njnz b ttttt\n#fuera del for\nend\n";

	programa_ansisop = "begin\nvariables a, b\na = 3\nb = 5\na = b + 12\nend";

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

	/*
	while(enviar && status !=0){
	 	fgets(message, PACKAGESIZE, stdin);
		if (!strcmp(message,"exit\n")) enviar = 0;
		if (enviar) send(socket_umc, message, strlen(message) + 1, 0);

		//Recibo mensajes del servidor
		memset (message,'\0',PACKAGESIZE);
		status = recv(socket_umc, (void*) message, PACKAGESIZE, 0);
		if (status != 0) printf("%s", message);
	}*/

	//Doy de baja la CPU, cierro las conexiones.
	log_info(logger, "Cierro conexiones.",socket_nucleo);
	cerrarConexionSocket(socket_nucleo);
	cerrarConexionSocket(socket_umc);

	log_destroy(logger);

	return EXIT_SUCCESS;
}

/****************************************************************************************/
/*                            CONFIGURACION Y CONEXIONES								*/
/****************************************************************************************/


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

		int cpu_id = config_get_int_value(config,"CPU_ID");
		configuracion->cpu_id = malloc(sizeof configuracion->cpu_id);
		memcpy(configuracion->cpu_id, &cpu_id, sizeof(int));

		config_destroy(config);
	}else{
	    log_error_y_cerrar_logger(logger, "Configuracion invalida.");
	    config_destroy(config);
		exit(EXIT_FAILURE);
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
			&& 	config_has_property(config, "UMC_PUERTO")
			&& 	config_has_property(config, "CPU_ID"));
}

void conectarAlNucleo(t_configuracion_cpu* configCPU){

	//Conexión al nucleo
	log_info(logger, "Conectando al nucleo. IP y puerto del núcleo: %s - %s", configCPU->nucleo_ip, configCPU->nucleo_puerto);
	socket_nucleo = conectarseA(configCPU->nucleo_ip, configCPU->nucleo_puerto);
	log_info(logger, "Conexion establecida con el nucleo. (Socket: %d)",socket_nucleo);
	log_info(logger, "Realizando handshake con el nucleo.");
	handshakeNucleo();
}

void conectarAUMC(t_configuracion_cpu* configCPU){

	log_info(logger, "Conectando a la umc. IP y puerto del núcleo: %s - %s", configCPU->umc_ip, configCPU->umc_puerto);
	socket_umc = conectarseA(configCPU->umc_ip, configCPU->umc_puerto);
	log_info(logger, "Conexion establecida con la umc. (Socket: %d)",socket_umc);
	handshakeUMC(*configCPU->cpu_id);
}

void handshakeNucleo(){

	int msj_recibido;
	int soy_cpu = 3000;


	log_info(logger, "Iniciando Handshake con el Nucleo");
	//Tengo que recibir el ID del nucleo = 1000
	recv(socket_nucleo, &msj_recibido, sizeof(int), 0);

	if(msj_recibido == 1000){
		recv(socket_nucleo, &quantum, sizeof(int), 0);
		log_info(logger, "Nucleo validado. Quantum recibido: %d",quantum);
		send(socket_nucleo, &soy_cpu, sizeof(int), 0); //Envio ID de la CPU
	}else{
	    log_error_y_cerrar_logger(logger, "El nucleo no pudo ser validado.");
		exit(EXIT_FAILURE);
	}
}


void handshakeUMC(int cpu_id){

	int mensaje;
	int buffer[2];

	log_info(logger, "Iniciando Handshake con la UMC");
	buffer[0] = SOY_CPU;
	buffer[1] = cpu_id;

	//Tengo que recibir el ID del nucleo = 1000
	recv(socket_umc, &mensaje, sizeof(int), 0);

	printf("mensaje recibido %d\n", mensaje);

	send(socket_umc, &buffer, 2*sizeof(int), 0);

	if(mensaje == SOY_UMC){
		//Envio ID de la CPU según el protocolo
		recv(socket_umc, &tamanio_pagina, sizeof(int), 0);
		log_info(logger, "UMC validado. Tamaño de página recibido: %d",tamanio_pagina);
		printf("Tamaño de página recibido %d\n", tamanio_pagina);
	}else{
	    log_error_y_cerrar_logger(logger, "La UMC no pudo ser validada.");
		exit(EXIT_FAILURE);
	}
}

/****************************************************************************************/
/*                                   FUNCIONES CPU									    */
/****************************************************************************************/
void recibirPCB(char* message){

	//Recibo mensaje del nucleo.
	//Primeros 4 bytes del id de la función (sacar de la definicion de los protocolos)

	int header;

	printf("voy a recibir mensaje de %d\n", socket_nucleo);
	int status = 0;
	while(status == 0){
		status = recv(socket_nucleo, &header, sizeof(int), 0);

		switch (header) {
			case ENVIO_PCB:
				printf("obtuve el pcb\n");
				break;
			default:
				printf("obtuve otro id %d\n", header);
				break;
		}
	}
}

void enviarPaqueteAUMC(char* message){
	//Envio el archivo entero

	int resultSend = send(socket_umc, message, PACKAGESIZE, 0);
	printf("resultado send %d, a socket %d \n",resultSend, socket_umc);
	if(resultSend == -1){
		printf ("Error al enviar archivo a UMC.\n");
		exit(EXIT_FAILURE);
	}else {
		printf ("Archivo enviado a UMC.\n");
		printf ("mensaje: %s\n", message);
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
	//Estructura para crear el header + paiload

}


/****************************************************************************************/
/*                                PRIMITIVAS ANSISOP								    */
/****************************************************************************************/
static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;


/*
 * DEFINIR VARIABLE:
 */
//t_posicion socketes_definirVariable(t_nombre_variable identificador_variable);
t_puntero socketes_definirVariable(t_nombre_variable variable) {

	/*
	 * Reserva en el Contexto de Ejecución Actual el espacio necesario para una variable llamada identificador_variable y la registra en el Stack, retornando la posición del valor de esta nueva variable del stack.
El valor de la variable queda indefinido: no deberá inicializarlo con ningún valor default.
Esta función se invoca una vez por variable, a pesar de que este varias veces en una línea. Por ejemplo, evaluar variables a, b, c llamará tres veces a esta función con los parámetros a, b y c.
*/

	printf("INICIO DEFINIR_VARIABLE %c -------\n", variable);
	printf("FIN DEFINIR_VARIABLE %c -------\n", variable);
	return POSICION_MEMORIA;
}

//TODO: Cambiar t_puntero por t_posicion

//t_posicion socketes_obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_puntero socketes_obtenerPosicionVariable(t_nombre_variable variable) {
	printf("Obtener posicion de %c\n", variable);
	return POSICION_MEMORIA;
}

//t_valor_variable socketes_dereferenciar(t_posicion direccion_variable);
t_valor_variable socketes_dereferenciar(t_puntero puntero) {
	printf("Dereferenciar %d y su valor es: %d\n", puntero, CONTENIDO_VARIABLE);
	return CONTENIDO_VARIABLE;
}

/*
 *  FUNCION     : Copia un valor en la variable ubicada en direccion_variable.
 *  Recibe      : t_posicion direccion_variable, t_valor_variable valor
 *  Devuelve    : void
 */
void socketes_asignar(t_puntero direccion_variable, t_valor_variable valor) {
	//Le solicito la página a la umc.
	//A partir de la direccion_variable, obtengo el número de página y el offset
	//Hago un memcpy del valor desde el offset de la página hasta un size de 4 bytes.
	printf("ANSISOP ------- Ejecuto asignar: en la direccion %d, el valor %d ----\n", direccion_variable, valor);

	t_solicitud_escritura *solicitud_escritura = (t_solicitud_escritura *)malloc(sizeof(t_solicitud_escritura));

	//Calculo la página y el offset a partir de dirección_variable
	solicitud_escritura->nroPagina = direccion_variable / tamanio_pagina;
	solicitud_escritura->offset = direccion_variable - solicitud_escritura->nroPagina * tamanio_pagina;
	solicitud_escritura->size = sizeof(int);
	solicitud_escritura->pid = 123; //TODO Obtener PID del PCB
	solicitud_escritura->valor = valor;

	printf(	"Nro de pagina: %d | Offset: %d | Direccion %d.\n", solicitud_escritura->nroPagina,
			solicitud_escritura->offset, direccion_variable);

	//Armo el paquete para escribir una página
	int mensaje_tamanio = sizeof(int)+sizeof(t_solicitud_escritura);
	char *mensaje = malloc(mensaje_tamanio);
	memset(mensaje,'\0',mensaje_tamanio);
	int header = ESCRIBIR;
	memcpy(mensaje, &header, sizeof(int));
	memcpy(mensaje + sizeof(int), &solicitud_escritura->nroPagina, sizeof(int));
	memcpy(mensaje + 2*sizeof(int), &solicitud_escritura->offset, sizeof(int));
	memcpy(mensaje + 3*sizeof(int), &solicitud_escritura->size, sizeof(int));
	memcpy(mensaje + 4*sizeof(int), &solicitud_escritura->pid, sizeof(int));
	memcpy(mensaje + 5*sizeof(int), &solicitud_escritura->valor, sizeof(int));
/*
	int resultado = send(socket_umc, mensaje, mensaje_tamanio, 0);

	if(resultado > 0){
		log_info(logger, "Envio mensaje a UMC para escritura. | Nro de pagina: %d | Offset: %d | Valor: %d",
				 solicitud_escritura.nroPagina, solicitud_escritura.offset, valor);
	}else{
		log_error_y_cerrar_logger(logger, "Falló envío de mensaje a UMC para escritura. | Nro de pagina: %d | Offset: %d | Valor: %d",
				 solicitud_escritura.nroPagina, solicitud_escritura.offset, valor);
		exit(EXIT_FAILURE);
	}
*/
	free(solicitud_escritura);
	free(mensaje);
}

//TODO: Cambiar parámetros que recibe y devuelve!

//t_valor_variable socketes_obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable socketes_obtenerValorCompartida(t_nombre_compartida variable){
	t_valor_variable valor_variable;
	printf("Obtener Valor Compartida\n");
	return valor_variable;
}

//t_valor_variable socketes_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor)
t_valor_variable socketes_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	t_valor_variable valor_variable;
	printf("Asignar Valor Compartida\n");
	return valor_variable;
}

/*
 * Devuelve el número de la primer instrucción ejecutable de etiqueta y -1 en caso de error.
 * t_puntero_instruccion irAlLabel(t_nombre_etiqueta etiqueta)
*/

//t_puntero_instruccion socketes_irAlLabel(t_nombre_etiqueta etiqueta);
t_puntero_instruccion socketes_irAlLabel(t_nombre_etiqueta etiqueta){
	t_puntero_instruccion puntero_instruccion;
	printf("Ir al Label\n");
	return puntero_instruccion;
}

void socketes_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	printf("Llamar con retorno:\n");
}

//t_puntero_instruccion socketes_retornar(t_valor_variable retorno);
t_puntero_instruccion socketes_retornar(t_valor_variable retorno){
	t_puntero_instruccion puntero_instruccion;
	printf("Retornar\n");
	return puntero_instruccion;
}

/*
 *  FUNCION     : Envia al Núcleo el contenido de valor_mostrar, para que se muestre en la consola correspondiente.
 *  Recibe      : el valor del variable a mostrar
 *  Devuelve    : void / int (TODO chequear)
 */
void socketes_imprimir(t_valor_variable valor_mostrar) {

	printf("ANSISOP ------- Ejecuto Imprimir: %d ----\n", valor_mostrar);

	int buffer[2];

	buffer[0] = ANSISOP_IMPRIMIR;
	buffer[1] = valor_mostrar;

	printf("Envio mensaje imprimir al Núcleo con el valor: %d (id del mensaje: %d)\n", buffer[1], buffer[0]);

	send(socket_nucleo, &buffer, 2*sizeof(int), 0);
}

/*
 *  FUNCION     : Envia al Núcleo una cadena de texto, para que se muestre en la consola correspondiente.
 *  Recibe      : cadena a mostrar por consola del programa
 *  Devuelve    : void / int (TODO chequear)
 */
void socketes_imprimirTexto(char* texto) {

	printf("ANSISOP ------- Ejecuto ImprimirTexto: %s ----\n", texto);

	printf("Envio mensaje imprimir al Núcleo\n");
	int id_mensaje = ANSISOP_IMPRIMIR_TEXTO;
	int tamanio_texto = strlen(texto);

	char *buffer = malloc(sizeof(int)*2+tamanio_texto);

	memcpy(buffer, &id_mensaje, sizeof(int));
	memcpy(buffer + sizeof(int), &tamanio_texto, sizeof(int));
	memcpy(buffer + 2*sizeof(int), texto, tamanio_texto);

	printf("Envio mensaje imprimir texto al Núcleo con el valor: %s\n", texto);

	send(socket_nucleo, &buffer, sizeof(buffer)+1, 0);

}

//TODO: Modificar funciones!!

//int socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){
	int nro_entero = 17;
	printf("Entrada Salida\n");
	//return nro_entero;
}

//int socketes_wait(t_nombre_semaforo identificador_semaforo);
void socketes_wait(t_nombre_semaforo identificador_semaforo){
	int nro_entero = 17;
	printf("Wait \n");
	//return nro_entero;
}

//int socketes_signal(t_nombre_semaforo identificador_semaforo);
void socketes_signal(t_nombre_semaforo identificador_semaforo){
	int nro_entero = 17;
	printf("Signal \n");

	//return nro_entero;
}


void socketes_finalizar(){
	printf("ANSISOP ------- Finalizar Programa. ----\n");
}
