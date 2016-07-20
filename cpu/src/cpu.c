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

	signal(SIGUSR1, handler_seniales);

	int header;

	printf("Me quedo escuchando mensajes del nucleo (socket:%d)\n", socket_nucleo);

	while(1){

		if( recv(socket_nucleo, &header, sizeof(int), 0) <= 0)
		{
			perror("Desconexion del nucleo\n");
			close(socket_nucleo);
			exit(0);
		}

		switch (header) {
			case EJECUTA:
				ejecutar();
				//recibirPCB();
				//ejecutoInstrucciones();
				//devuelvoPcbActualizadoAlNucleo();
				//liberarEspacioDelPCB();
				break;
			default:
				printf("obtuve otro id %d\n", header);
				sleep(3);
				break;
		}
	}

	//Reenviar el msj recibido del nucleo a la UMC
	//enviarPaqueteAUMC(message);

	//TODO - Ejecutar siguiente instrucción a partir del program counter del pcb
	//Momentaneamente, obtengo metadata_desde_literal un programa de ejemplo y analizo cada instruccion.
	//TODO - Después, eliminar el metadata_desde_literal de la cpu. Tiene que obtener el pcb del nucleo.
/*
	//Levanto un archivo ansisop de prueba
	char* programa_ansisop;

	programa_ansisop = "begin\nvariables i,b\ni = 1\n:ttttt\n:hola\ni = i + 1\nprint i\nb = i - 10\njnz b hola\ni = 1\ni = i + 1\nprint i\nb = i - 10\njnz b ttttt\n#fuera del for\nend\n";

	programa_ansisop = "begin\nvariables a, b\na = 3\nb = 5\na = b + 12\nend";
	programa_ansisop = "begin\n:etiqueta\nwait c\nprint !colas\nsignal b\n#Ciclar indefinidamente\ngoto etiqueta\nend";
	programa_ansisop = "#!/usr/bin/ansisop\n#Alliance - S4\nbegin\nvariables f, i, t\n#`f`: Hasta donde contar\nf=20\n:inicio\n#`i`: Iterador\ni=i+1\n#Imprimir el contador\nprint i\n#`t`: Comparador entre `i` y `f`\nt=f-i\n#De no ser iguales, salta a inicio\n#esperar\nio HDD1 3\njnz t inicio\nend";


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
*/
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

	finalizarCpu();
	log_destroy(logger);


	return EXIT_SUCCESS;
}

void ejecutar()
{
	int quantum;
	char *instruccion;

	//pcb_actual = recibirPcb(socket_nucleo, false, &quantum);
	pcb_actual = recibirPcb(socket_nucleo, false, &quantum);

	printf("quantum: %d.\n", quantum);

	int i;
	for(i=0;i<quantum;i++)
	{
		printf("for.\n");
		if(estado != TODO_OK)
			break;

		t_intructions instruction = pcb_actual.indice_cod.instrucciones[pcb_actual.PC];

		instruccion = solicitarInstruccion(instruction);

		printf("voy a entrar a analizador linea.\n");
		analizadorLinea(instruccion, &funciones, &funciones_kernel);
		printf("Ya pase por analizador linea.\n");

		free(instruccion);
	}

	if(estado == TODO_OK)
	{//Termino el quantum. Envio el pcb con las modificaciones que tuvo
		enviarPcb(pcb_actual, socket_nucleo, -1);
	}
	else if(estado == ENTRADA_SALIDA)
	{
		return;
	}
	else if(estado == WAIT)
	{
		return;
	}
	else if(estado == FIN_PROGRAMA)
	{//Notifico el fin del programa
		return;
	}
	else{
		printf("Estado erroneo.\n");
	}

}

char *solicitarInstruccion(t_intructions instruccion)
{
	int mensaje[5];
	mensaje[0] = LEER;
	mensaje[1] = instruccion.start / tamanio_pagina;//Pagina en la que esta el codigo
	mensaje[2] = instruccion.start % tamanio_pagina;//Offset de la pagina
	mensaje[3] = instruccion.offset;				//Size de la instruccion
	mensaje[4] = pcb_actual.pid;					//pid

	printf("Tamaño pagina: %d.\n",tamanio_pagina);
	printf("Start instruccion: %d, offset: %d",instruccion.start,instruccion.offset);
	printf("msj %d, pag %d, offset %d, size %d, pid %d.\n", mensaje[0],mensaje[1],mensaje[2],mensaje[3],mensaje[4]);

	send(socket_umc, &mensaje, 5*sizeof(int), 0);

	char *resultado = malloc(instruccion.offset);

	printf("Espero el resultado...\n");
	if( recvAll(socket_umc,resultado,instruccion.offset,MSG_WAITALL) <= 0)
	{
		perror("Error al recibir instruccion");
	}

	printf("Pude recibir la instruccion:\n");
	fwrite(resultado,sizeof(char),instruccion.offset,stdout);
	printf("\n\n");

	return resultado;
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

		configuracion->nucleo_ip = config_get_string_value(config,"NUCLEO_IP");

		/*char* nucleo_ip = config_get_string_value(config,"NUCLEO_IP");
		configuracion->nucleo_ip = malloc(strlen(nucleo_ip));
		memcpy(configuracion->nucleo_ip, nucleo_ip, strlen(nucleo_ip));
		configuracion->nucleo_ip[strlen(nucleo_ip)] = '\0';*/

		configuracion->nucleo_puerto = config_get_string_value(config, "NUCLEO_PUERTO");

		/*char* nucleo_puerto = config_get_string_value(config,"NUCLEO_PUERTO");
		configuracion->nucleo_puerto = malloc(strlen(nucleo_puerto)+1);
		memcpy(configuracion->nucleo_puerto, nucleo_puerto, strlen(nucleo_puerto));
		configuracion->nucleo_puerto[strlen(nucleo_puerto)] = '\0';*/

		configuracion->umc_ip = config_get_string_value(config,"UMC_IP");

//		char* umc_ip = config_get_string_value(config,"UMC_IP");
//		configuracion->umc_ip = malloc(strlen(umc_ip));
//		memcpy(configuracion->umc_ip, umc_ip, strlen(umc_ip));
//		configuracion->umc_ip[strlen(umc_ip)] = '\0';

		configuracion->umc_puerto = config_get_string_value(config,"UMC_PUERTO");

//		char* umc_puerto = config_get_string_value(config,"UMC_PUERTO");
//		configuracion->umc_puerto = malloc(strlen(umc_puerto)+1);
//		memcpy(configuracion->umc_puerto, umc_puerto, strlen(umc_puerto));
//		configuracion->umc_puerto[strlen(umc_puerto)] = '\0';

		configuracion->cpu_id = config_get_int_value(config, "CPU_ID");

//		int cpu_id = config_get_int_value(config,"CPU_ID");
//		configuracion->cpu_id = malloc(sizeof configuracion->cpu_id);
//		memcpy(configuracion->cpu_id, &cpu_id, sizeof(int));

		//config_destroy(config);
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
	handshakeUMC(configCPU->cpu_id);
}

void handshakeNucleo(){

	int msj_recibido;
	int soy_cpu = SOY_CPU;

	log_info(logger, "Iniciando Handshake con el Nucleo");
	//Tengo que recibir el ID del nucleo = 1000
	recv(socket_nucleo, &msj_recibido, sizeof(int), 0);

	if(msj_recibido == SOY_NUCLEO){
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

void finalizarCpuPorError(){
	finalizarCpu();
	log_destroy(logger);
}

/****************************************************************************************/
/*                                   FUNCIONES CPU									    */
/****************************************************************************************/
void recibirPCB(){

	/*int status;
	t_pcb_stream *pcb_stream;

	status = recv(socket_nucleo, &pcb_stream->tamanio, sizeof(int), 0);

	if(status <= 0){
		log_error(logger, "Error al recibir el tamaño del PCB. Bytes recibidos: %d",
				status);
		finalizarCpuPorError();
	}

	pcb_stream->data_pcb = malloc(pcb_stream->tamanio);
	status = recv(socket_nucleo, pcb_stream->data_pcb, pcb_stream->tamanio, 0);
	pcb_actual = desSerializador_PCB(pcb_stream);
	free(pcb_stream->data_pcb);

	if(status <= 0){
		log_error(logger, "Error al recibir el PCB. Bytes recibidos: %d",
				status);
		finalizarCpuPorError();
	}*/
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

int ejecutoInstrucciones(){
	/*printf("Ejecuto instrucciones después de recibir PCB\n");
	int nroInstr;
	for (nroInstr = 0; nroInstr < quantum; ++nroInstr) {
		int nroInstruccionAEjecutar = pcb_actual->PC;
		int start = pcb_actual->indice_cod->instrucciones[nroInstruccionAEjecutar].start;
		int offset = pcb_actual->indice_cod->instrucciones[nroInstruccionAEjecutar].offset;
		char* instruccion = solicitoInstruccionAUMC(start, offset);
		analizadorLinea(instruccion, &funciones, &funciones_kernel);
		pcb_actual->PC++;
	}
	return nroInstr;*/
}

void devuelvoPcbActualizadoAlNucleo(){

	/*log_info(logger, "Devuelvo PCB Actualizado al nucleo");

	t_pcb_stream *stream_pcb = serializador_PCB(pcb_actual);
	int header = ENVIO_PCB_ACTUALIZADO;

	char *instruccion;

	int mensaje_tamanio = sizeof(int)*2+stream_pcb->tamanio;
	char *mensaje = malloc(mensaje_tamanio);
	memset(mensaje, '\0', mensaje_tamanio);

	memcpy(mensaje, &header, sizeof(int));
	memcpy(mensaje + sizeof(int), &stream_pcb->tamanio, sizeof(int));
	memcpy(mensaje + 2*sizeof(int), stream_pcb->data_pcb, stream_pcb->tamanio);

	int resultado = send(socket_umc, mensaje, mensaje_tamanio, 0);

	if(resultado < 0){
		log_error_y_cerrar_logger(logger, "Falló envío de PCB Actualizado al nucleo.");
		exit(EXIT_FAILURE);
	}

	free(mensaje);
	free(stream_pcb->data_pcb);

	log_info(logger, "Se envió exitosamente el PCB actualizado al Núcleo.");*/
}

void liberarEspacioDelPCB(){
	log_info(logger, "Libero espacio del PCB.");
	//free(pcb_actual); //TODO chequear si con esto alcanza
}

//TODO: Terminar Implementacion
void handler_seniales(int senial) {
	switch (senial) {
		case SIGUSR1:
			printf("Obtuve una señal de SIGUSR1\n");
			sleep(3);
			break;
		default:
			printf("Obtuve otra senial inesperada\n");
			sleep(3);
	}
	exit(EXIT_FAILURE);
}


void finalizarCpu(){
	//Doy de baja la CPU, cierro las conexiones.
	log_info(logger, "Cierro conexiones.",socket_nucleo);
	cerrarConexionSocket(socket_nucleo);
	cerrarConexionSocket(socket_umc);

}

char* solicitoInstruccionAUMC(int start, int offset){

	log_info(logger, "Solicito Instruccion a la UMC. Start: %d -  Offset: %d", start, offset);

	int header = SOLICITUD_INSTRUCCION;
	char *instruccion;

	int mensaje_tamanio = sizeof(int)*3;
	char *mensaje = malloc(mensaje_tamanio);
	memset(mensaje, '\0', mensaje_tamanio);

	memcpy(mensaje, &header, sizeof(int));
	memcpy(mensaje + sizeof(int), &start, sizeof(int));
	memcpy(mensaje + 2*sizeof(int), &offset, sizeof(int));

	int resultado = send(socket_umc, mensaje, mensaje_tamanio, 0);

	if(resultado < 0){
		log_error_y_cerrar_logger(logger, "Falló envío de mensaje a UMC para solicitar la instruccion");
		exit(EXIT_FAILURE);
	}

	free(mensaje);

	log_info(logger, "Se envió exitosamente la solicitud de instruccion a la UMC");

	int header_recibido;
	int resultado_recv = recv(socket_nucleo, &header_recibido, sizeof(int), 0);

	if(resultado_recv < 0){
		printf("resultado_recv menor que 0: %d\n", resultado_recv);
		log_error_y_cerrar_logger(logger, "Fallo en la respuesta de la solicitud de instruccion. Resultado: %d", resultado_recv);
		exit(EXIT_FAILURE);
	}

	if(header_recibido == SOLICITUD_INSTRUCCION){
		int tamanio_instruccion = offset - start;
		instruccion = malloc(tamanio_instruccion);
		memset(instruccion, '\0', tamanio_instruccion);

		resultado_recv = recv(socket_umc, instruccion, tamanio_instruccion, 0);

		if(resultado_recv < 0){
			printf("resultado_recv menor que 0: %d\n", resultado_recv);
			log_error_y_cerrar_logger(logger, "Fallo al obtener instruccion de la UMC. Resultado recv: %d\n", resultado_recv);
			exit(EXIT_FAILURE);
		}
		log_info(logger, "Instruccion obtenida: %s\n",instruccion);

	}else{
		printf("header_recibido es DISTINTO a SOLICITUD_INSTRUCCION. header_recibido: %d | SOLICITUD_INSTRUCCION: %d\n", header_recibido, SOLICITUD_INSTRUCCION);
	}

	return instruccion;

}


/****************************************************************************************/
/*                                PRIMITIVAS ANSISOP								    */
/****************************************************************************************/
static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;


/*
 *  FUNCION     : Reserva en el Contexto de Ejecución Actual el espacio necesario para una variable
 *  			  llamada identificador_variable y la registra en el Stack, retornando la posición
 *  			  del valor de esta nueva variable del stack.
 *  Recibe      : t_nombre_variable identificador_variable
 *  Devuelve    : t_puntero posicion_memoria (En caso de error, retorna -1.)
 */
t_puntero socketes_definirVariable(t_nombre_variable identificador_variable) {

	printf("ANSISOP ------- Ejecuto Definir Variable. Variable: %c ----\n", identificador_variable);

	//Acceder al stack.
	//Crear un nuevo nodo para el stack. Busco un espacio libre de 4 bytes.
	//Guardo el valor en el campo vars, creando una nueva entrada /nodo con:
	//Identificador_variable - Pagina - Offset - Size (siempre 4 bytes.)
	//El valor de la variable queda indefinido -> No inicializarlo

//	int pagina = 0; //TODO cambiar offset y pagina
//	int offset = 8;
//
//	int posicion_memoria = pagina*offset;
//
//	return posicion_memoria;
}

/*
 *  FUNCION     : Devuelve el desplazamiento respecto al inicio del segmento Stack en que se encuentra
 *  			  el valor de la variable identificador_variable del contexto actual.
 *  Recibe      : t_nombre_variable variable
 *  Devuelve    : t_puntero (En caso de error, retorna -1.)
 */
t_puntero socketes_obtenerPosicionVariable(t_nombre_variable identificador_variable) {

	printf("ANSISOP ------- Ejecuto ObtenerPosicionVariable. Variable: %c ----\n", identificador_variable);

	//Buscar en el índice de stack, la entrada/nodo que pertenece a identificador_variable
	//Devolver la posición -> Supongo que como es un t_puntero,
	//debería devolver => 	nroPagina*tamanioPagina+offset. (el nroPagina y el offset lo obtengo del
	//						vars del indice stack)

//	int pagina = 0; //TODO cambiar offset y pagina
//	int offset = 8;
//
//	int posicion_memoria = pagina*offset;
//
//	return posicion_memoria;
}

/*
 *  FUNCION     : Obtiene el valor resultante de leer a partir de direccion_variable,
 *  			  sin importar cual fuera el contexto actual.
 *  Recibe      : t_puntero puntero (Lugar donde buscar)
 *  Devuelve    : t_valor_variable (Valor que se encuentra en esa posicion)
 */
t_valor_variable socketes_dereferenciar(t_puntero direccion_variable) {

	t_valor_variable valor_variable;
	printf("ANSISOP ------- Ejecuto dereferenciar. Direccion_variable: %d ----\n", direccion_variable);

	//A partir de la puntero, obtengo el número de página y el offset de la variable que quiero leer.
	//Pido a la umc, que lea el 4 bytes a partir del número de página y offset.

//	t_solicitud_lectura *solicitud_lectura = (t_solicitud_lectura *)malloc(sizeof(t_solicitud_lectura));
//
//	//Calculo la página y el offset a partir de puntero
//	solicitud_lectura->nroPagina = direccion_variable / tamanio_pagina;
//	solicitud_lectura->offset = direccion_variable - solicitud_lectura->nroPagina * tamanio_pagina;
//	solicitud_lectura->size = sizeof(int);
//	solicitud_lectura->pid = 123; //TODO Obtener PID del PCB
//
//	printf(	"Nro de pagina: %d | Offset: %d | Direccion %d.\n", solicitud_lectura->nroPagina,
//			solicitud_lectura->offset, direccion_variable);
//
//	//Armo el paquete solicitar una lectura a la UMC
//	int mensaje_tamanio = sizeof(int)+sizeof(t_solicitud_lectura);
//	char *mensaje = malloc(mensaje_tamanio);
//	memset(mensaje,'\0',mensaje_tamanio);
//	int header = LEER;
//	memcpy(mensaje, &header, sizeof(int));
//	memcpy(mensaje + sizeof(int), &solicitud_lectura->nroPagina, sizeof(int));
//	memcpy(mensaje + 2*sizeof(int), &solicitud_lectura->offset, sizeof(int));
//	memcpy(mensaje + 3*sizeof(int), &solicitud_lectura->size, sizeof(int));
//	memcpy(mensaje + 4*sizeof(int), &solicitud_lectura->pid, sizeof(int));
//
//	int resultado = send(socket_umc, mensaje, mensaje_tamanio, 0);
//
//	if(resultado < 0){
//		log_error_y_cerrar_logger(logger, "Falló envío de mensaje a UMC para escritura. | Nro de pagina: %d | Offset: %d | Direccion_variable: %d",
//			solicitud_lectura->nroPagina, solicitud_lectura->offset, direccion_variable);
//			exit(EXIT_FAILURE);
//	}
//
//	free(solicitud_lectura);
//	free(mensaje);
//
//	log_info(logger, "Envio mensaje a UMC para lectura. | Nro de pagina: %d | Offset: %d | Direccion_variable: %d",
//			 solicitud_lectura->nroPagina, solicitud_lectura->offset, direccion_variable);
//
//	int header_recibido;
//	int resultado_recv = recv(socket_nucleo, &header_recibido, sizeof(int), 0);
//
//	if(resultado_recv < 0){
//		printf("resultado_recv menor que 0: %d\n", resultado_recv);
//		log_error_y_cerrar_logger(logger, "Falló en respuesta del pedido de "
//										  "lectura de la posicion %d",direccion_variable);
//		exit(EXIT_FAILURE);
//	}
//
//	if(header_recibido == LEER){
//		resultado_recv = recv(socket_nucleo, &valor_variable, sizeof(t_valor_variable), 0);
//
//		if(resultado_recv < 0){
//			printf("resultado_recv menor que 0: %d\n", resultado_recv);
//			log_error_y_cerrar_logger(logger, "Falló en obtener el valor de la variable del pedido de "
//											  "lectura de la posicion %d",direccion_variable);
//			exit(EXIT_FAILURE);
//		}
//
//		printf("Resultado pedido de lectura. Direccion: %d. | Header: %d. | valor_variable: %d\n",
//				direccion_variable, header_recibido, valor_variable);
//	}else{
//		printf("header_recibido es DISTINTO a LEER. header_recibido: %d | LEER: %d\n", header_recibido, LEER);
//	}
//
//	printf("Dereferenciar %d y su valor es: %d\n", direccion_variable, valor_variable);
//
//	return valor_variable;
}

/*
 *  FUNCION     : Copia un valor en la variable ubicada en direccion_variable.
 *  Recibe      : t_posicion direccion_variable, t_valor_variable valor
 *  Devuelve    : void
 */
void socketes_asignar(t_puntero direccion_variable, t_valor_variable valor) {

	printf("ANSISOP ------- Ejecuto asignar: en la direccion %d, el valor %d ----\n", direccion_variable, valor);

	//A partir de la direccion_variable, obtengo el número de página y el offset,
	//para hacer la solicitud de escritura a la UMC.
//	t_solicitud_escritura *solicitud_escritura = (t_solicitud_escritura *)malloc(sizeof(t_solicitud_escritura));
//
//	//Calculo la página y el offset a partir de dirección_variable
//	solicitud_escritura->nroPagina = direccion_variable / tamanio_pagina;
//	solicitud_escritura->offset = direccion_variable - solicitud_escritura->nroPagina * tamanio_pagina;
//	solicitud_escritura->size = sizeof(int);
//	solicitud_escritura->pid = 123; //TODO Obtener PID del PCB
//	solicitud_escritura->valor = valor;
//
//	printf(	"Nro de pagina: %d | Offset: %d | Direccion %d.\n", solicitud_escritura->nroPagina,
//			solicitud_escritura->offset, direccion_variable);
//
//	//Armo el paquete para escribir una página y envio el mensaje a la UMC
//	int mensaje_tamanio = sizeof(int)+sizeof(t_solicitud_escritura);
//	char *mensaje = malloc(mensaje_tamanio);
//	memset(mensaje,'\0',mensaje_tamanio);
//	int header = ESCRIBIR;
//	memcpy(mensaje, &header, sizeof(int));
//	memcpy(mensaje + sizeof(int), &solicitud_escritura->nroPagina, sizeof(int));
//	memcpy(mensaje + 2*sizeof(int), &solicitud_escritura->offset, sizeof(int));
//	memcpy(mensaje + 3*sizeof(int), &solicitud_escritura->size, sizeof(int));
//	memcpy(mensaje + 4*sizeof(int), &solicitud_escritura->pid, sizeof(int));
//	memcpy(mensaje + 5*sizeof(int), &solicitud_escritura->valor, sizeof(int));
//
//	int resultado = send(socket_umc, mensaje, mensaje_tamanio, 0);
//
//	if(resultado > 0){
//		log_info(logger, "Envio mensaje a UMC para escritura. | Nro de pagina: %d | Offset: %d | Valor: %d",
//				 solicitud_escritura->nroPagina, solicitud_escritura->offset, valor);
//
//	}else{
//		log_error_y_cerrar_logger(logger, "Falló envío de mensaje a UMC para escritura. | Nro de pagina: %d | Offset: %d | Valor: %d",
//		 solicitud_escritura->nroPagina, solicitud_escritura->offset, valor);
//		exit(EXIT_FAILURE);
//	}
//
//	free(solicitud_escritura);
//	free(mensaje);
}


/*
 *  FUNCION     : Solicita al Núcleo el valor de una variable compartida
 *  Recibe      : t_nombre_compartida variable
 *  Devuelve    : t_valor_variable
 */
t_valor_variable socketes_obtenerValorCompartida(t_nombre_compartida variable){

	t_valor_variable valor_variableCompartida;

	printf("ANSISOP ------- Ejecuto ObtenerValorCompartida. Variable: %s ----\n", variable);

	int header_mensaje = ANSISOP_OBTENER_VALOR_COMPARTIDO;

	char *mensaje = (char *)malloc(sizeof(int));
	memcpy(mensaje, &header_mensaje, sizeof(int));

	printf("Envio mensaje al Núcleo para obtener el valor de la variable compartida.\n");

	send(socket_nucleo, &mensaje, sizeof(int), 0);

	//TODO esperar la respuesta: el valor de la variable compartida

	int status = 0;

	int header_recibido;
	while(status == 0){
		//Espero respuesta del nucleo...
		//TODO: Averiguar si el recv es bloqueante...
		status = recv(socket_umc, &header_recibido, sizeof(int), 0);
		sleep(2); //TODO: Sacar
		if (status != 0) printf("Header recibido: %d.\n", header_recibido);
	}

	//TODO: Cambiar si usa un id diferente para la respuesta de la solicitud de lectura.
	if(header_recibido == ANSISOP_OBTENER_VALOR_COMPARTIDO){
		//Recibo valor de la variable compartida
		int resultado = recv(socket_umc, &valor_variableCompartida, sizeof(int), 0);
		if(resultado <= 0){
			printf("Resultado del recv de la variable compartida %s dio: %d\n", variable,resultado);
		}
	}

	return valor_variableCompartida;
}


/*
 *  FUNCION     : Solicita al Núcleo asignar el valor a la variable compartida. Devuelve el valor asignado.
 *  Recibe      : t_nombre_compartida variable, t_valor_variable valor_asignado
 *  Devuelve    : t_valor_variable valor_asignado
 */
t_valor_variable socketes_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor_asignado){

	t_valor_variable valor_variableCompartida;

	printf("ANSISOP ------- Ejecuto asignarValorCompartida. Variable: %s. | Valor asignado: %d. ----\n", variable, valor_asignado);

	int header_mensaje = ANSISOP_ASIGNAR_VALOR_COMPARTIDO;

	int tamanio_mensaje = sizeof(int)+sizeof(t_nombre_compartida)+sizeof(t_valor_variable);
	char *mensaje = (char *)malloc(tamanio_mensaje);
	memcpy(mensaje, &header_mensaje, sizeof(int));
	memcpy(mensaje, &variable, sizeof(t_nombre_compartida));
	memcpy(mensaje, &valor_asignado, sizeof(t_valor_variable));

	printf("Envio mensaje al Núcleo para asignar el valor a la variable compartida.\n");

	send(socket_nucleo, &mensaje, tamanio_mensaje, 0);

	//TODO: Tengo que esperar un ok del nucleo por cada vez que le asigno algo a una variable?

	return valor_asignado;
}

/*
 *  FUNCION     : Cambia la linea de ejecucion a la correspondiente de la etiqueta buscada.
 *  Recibe      : t_nombre_etiqueta etiqueta
 *  Devuelve    : void
 */
void socketes_irAlLabel(t_nombre_etiqueta etiqueta){
	//Del pcb voy a recurrir al campo etiquetas.
	//Ahí aplico  metadata_buscar_etiqueta.
	//Obtengo la posición de la etiqueta en el código y....
	//actualizo el campo ret pos para saber a donde tiene que volver al terminar de ejecutar
	//lo que corresponde a esa etiqueta
	t_puntero_instruccion puntero_instruccion;
	printf("Ir al Label\n");
}

void socketes_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	printf("Llamar con retorno:\n");
}

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

	int mensaje_recibido;
	int resultado_recv = recv(socket_nucleo, &mensaje_recibido, sizeof(int), 0);

	if(resultado_recv > 0){
		printf("resultado_recv mayor que 0: %d. | mensaje_recibido: %d\n", resultado_recv, mensaje_recibido);
	}else{
		printf("resultado_recv menor que 0: %d\n", resultado_recv);
	}
}

/*
 *  FUNCION     : Informa al Núcleo que el Programa actual pretende utilizar el dispositivo
 *  			  durante tiempo unidades de tiempo.
 *  Recibe      : t_nombre_dispositivo dispositivo, int tiempo
 *  Devuelve    : void
 */
void socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){

	printf("ANSISOP ------- Ejecuto Entrada-Salida. Dispositivo :%s | Unidades de tiempo: %d ----\n", dispositivo, tiempo);

	int header_mensaje = ANSISOP_ENTRADA_SALIDA;

	char *mensaje = (char *)malloc(sizeof(int)*2+sizeof(t_nombre_semaforo));

	memcpy(mensaje, &header_mensaje, sizeof(int));
	memcpy(mensaje + sizeof(int), &dispositivo, sizeof(t_nombre_dispositivo));
	memcpy(mensaje + sizeof(int) + sizeof(t_nombre_dispositivo), &tiempo, sizeof(int));

	//TODO Mandar la estructura del pcb después del tiempo.
	//Proceso va a la cola de bloqueados. CPU no haria espera activa.
	//La liberaría para que pueda ocuparse de otro proceso.

	printf("Envio mensaje de entrada y salida al Núcleo con el identificador del semaforo y la cantidad de unidades de tiempo\n");

	send(socket_nucleo, &mensaje, 2*sizeof(int), 0);

}

/*
 *  FUNCION     : Informa al Núcleo que ejecute la función wait para el semáforo indicado.
 *  Recibe      : t_nombre_semaforo identificador_semaforo
 *  Devuelve    : void
 */
void socketes_wait(t_nombre_semaforo identificador_semaforo){

	printf("ANSISOP ------- Ejecuto WAIT: %s ----\n", identificador_semaforo);

	int header_mensaje = ANSISOP_WAIT;

	printf("Size of identificador: %d ----\n", sizeof(t_nombre_semaforo));

	char *mensaje = (char *)malloc(sizeof(int)+sizeof(t_nombre_semaforo));

	memcpy(mensaje, &header_mensaje, sizeof(int));
	memcpy(mensaje + sizeof(int), &identificador_semaforo, sizeof(t_nombre_semaforo));

	printf("Envio mensaje wait al Núcleo con el identificador del semaforo\n");

	send(socket_nucleo, &mensaje, 2*sizeof(int), 0);

	//TODO esperar la respuesta.
	//(?)Qué es lo que tiene que devolver? Se tiene que quedar esperando hasta que el wait de 0?
	//Tb tengo que usar wait/signal en estas funciones, para simular el wait/signal de las primitivas?
	//Si me manda que se tiene que bloquear, devuelvo el pcb actualizado no?
}


/*
 *  FUNCION     : Informa al Núcleo que ejecute la función signal para el semáforo indicado.
 *  Recibe      : t_nombre_semaforo identificador_semaforo
 *  Devuelve    : void
 */
void socketes_signal(t_nombre_semaforo identificador_semaforo){

	printf("ANSISOP ------- Ejecuto SIGNAL: %s ----\n", identificador_semaforo);

	int header_mensaje = ANSISOP_SIGNAL;

	printf("Size of identificador: %d ----\n", sizeof(t_nombre_semaforo));

	char *mensaje = (char *)malloc(sizeof(int)+sizeof(t_nombre_semaforo));

	memcpy(mensaje, &header_mensaje, sizeof(int));
	memcpy(mensaje + sizeof(int), &identificador_semaforo, sizeof(t_nombre_semaforo));

	printf("Envio mensaje SIGNAL al Núcleo con el identificador del semaforo\n");

	send(socket_nucleo, &mensaje, 2*sizeof(int), 0);

	//TODO esperar la respuesta.
	//(?)Qué es lo que tiene que devolver? Yo tengo que almacenar el valor del semaforo?
}


void socketes_finalizar(){
	printf("ANSISOP ------- Finalizar Programa. ----\n");
}
