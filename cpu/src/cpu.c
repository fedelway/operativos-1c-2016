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
	crearLog();

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
			return 0;
		}

		switch (header) {
			case EJECUTA:
				ejecutar();
				break;

			case CAMBIO_QUANTUM_SLEEP:
				recv(socket_nucleo,&quantum_sleep,sizeof(int),0);
				break;

			default:
				printf("obtuve otro id %d\n", header);
				sleep(3);
				break;
		}
	}

	finalizarCpu();
	log_destroy(logger);


	return EXIT_SUCCESS;
}

void crearLog()
{
	log = fopen("../resource/log", "a");

	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	fprintf(log, "log del dia %s. \n\n", asctime(timeinfo) );
}

void ejecutar()
{
	int mensaje, retRecv;
	int quantum;
	char *instruccion;

	pcb_actual = recibirPcb(socket_nucleo, false, &quantum);

	printf("Ejecuto el programa pid: %d",pcb_actual.pid);
	printf("quantum: %d.\n", quantum);

	//Vuelvo el estado al original
	estado = TODO_OK;

	int i;
	for(i=0;i<quantum;i++)
	{
		usleep(quantum_sleep * 1000);//Sleep enunciado

		if(estado != TODO_OK)
			break;

		if(senial_activada)
			break;

		t_intructions instruction = pcb_actual.indice_cod.instrucciones[pcb_actual.PC];

		instruccion = solicitarInstruccion(instruction);

		if(estado == ERROR)
			break;

		analizadorLinea(instruccion, &funciones, &funciones_kernel);

		//Avanzo el PC
		pcb_actual.PC++;

		free(instruccion);
	}

	if( senial_activada )
	{
		printf("Terminando ejecucion por SIGUSR1.\n");
		apagarse();
	}

	if(estado == TODO_OK)
	{//Termino el quantum. Envio el pcb con las modificaciones que tuvo
		enviarPcb(pcb_actual, socket_nucleo, -1);
		printf("Se me termino el quantum.\n");
	}
	else if( estado == ENTRADA_SALIDA || estado == WAIT )
	{
		printf("Estoy bloqueado...\n");
		//Envio el pcb
		enviarPcb(pcb_actual,socket_nucleo,-1);

		return;
	}else if( estado == FIN_PROGRAMA)
	{
		printf("Termino el programa.\n");
		return;
	}else if( estado == ERROR )
	{
		socketes_finalizar();
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

	printf("Start instruccion: %d, offset: %d.\n",instruccion.start,instruccion.offset);
	printf("msj %d, pag %d, offset %d, size %d, pid %d.\n", mensaje[0],mensaje[1],mensaje[2],mensaje[3],mensaje[4]);

	send(socket_umc, &mensaje, 5*sizeof(int), 0);

	//Recibo la respuesta si hay stack_overflow
	recv(socket_umc,&mensaje[0],sizeof(int),0);
	if(mensaje[0] == OVERFLOW)
	{
		printf("Error al solicitar instruccion: STACK_OVERFLOW");
		estado = ERROR;
		char *puntero;
		return puntero;
	}

	char *resultado = malloc(instruccion.offset + 1);

	printf("Espero el resultado...\n");
	if( recvAll(socket_umc,resultado,instruccion.offset,MSG_WAITALL) <= 0)
	{
		perror("Error al recibir instruccion");
	}

	//Le agrego un \0 al resultado
	resultado[instruccion.offset] = '\0';

	printf("Pude recibir la instruccion:\n\n");
	fwrite(resultado,sizeof(char),instruccion.offset,stdout);
	printf("\n\n");

	return resultado;
}

void apagarse()
{
	//Le envio un mensaje al nucleo avisando que me voy a desconectar
	int mensaje = DESCONEXION_CPU;
	send(socket_nucleo,&mensaje,sizeof(int),0);

	if(estado == TODO_OK)
	{
		//Envio el pcb y cierro el socket
		printf("Envio el pcb.\n");
		//Necesito mandarle un mensaje a nucleo diciendo que le voy a enviar el pcb
		mensaje = FIN_QUANTUM;
		send(socket_nucleo,&mensaje,sizeof(int),0);

		enviarPcb(pcb_actual, socket_nucleo, -1);
	}else
	{//No le tengo que enviar el pcb
		mensaje = BLOQUEADO;
		send(socket_nucleo,&mensaje,sizeof(int),0);
	}
	close(socket_nucleo);

	//Lo mismo con umc
	send(socket_umc,&mensaje,sizeof(int),0);
	close(socket_umc);

	//Libero recursos
	fclose(log);

	exit(0);
}

int buscarEtiqueta(char *etiqueta)
{
	char *etiquetas = pcb_actual.indice_etiquetas.etiquetas;
	int size = pcb_actual.indice_etiquetas.etiquetas_size;

	int cant_leida = 0;
	while(cant_leida < size)
	{
		if( !strncmp(etiqueta,etiquetas + cant_leida,strlen(etiqueta)) )
		{
			return (int) *(etiquetas + cant_leida + strlen(etiqueta));
		}else{
			cant_leida+=strlen(etiqueta);
			cant_leida+=4;
		}
	}

	return -1;
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
		recv(socket_nucleo, &quantum_sleep,sizeof(int),0);

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
			senial_activada = true;
			sleep(3);
			break;
		default:
			printf("Obtuve otra senial inesperada\n");
			sleep(3);
	}

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

#define funcionActual (pcb_actual.stack.cant_entradas) - 1
#define funcionAnterior pcb_actual.stack.cant_entradas - 2
#define varActual pcb_actual.stack.entradas[funcionActual].variables[pcb_actual.stack.entradas[funcionActual].cant_var - 1]
#define varAnterior pcb_actual.stack.entradas[funcionActual].variables[pcb_actual.stack.entradas[funcionActual].cant_var - 2]

/*
 *  FUNCION     : Reserva en el Contexto de Ejecución Actual el espacio necesario para una variable
 *  			  llamada identificador_variable y la registra en el Stack, retornando la posición
 *  			  del valor de esta nueva variable del stack.
 *  Recibe      : t_nombre_variable identificador_variable
 *  Devuelve    : t_puntero posicion_memoria (En caso de error, retorna -1.)
 */
t_puntero socketes_definirVariable(t_nombre_variable identificador_variable) {

	//Calculo de pag y offset es EL MISMO en las dos partes del if.
	t_var calcularVariable(t_pcb pcb)
	{
		t_var var, anterior;
		if(pcb_actual.stack.entradas[funcionActual].cant_var > 1)
		{//La anterior esta en el mismo contexto
			anterior = varAnterior;

			//Calculo pag y offset
			if(anterior.offset + sizeof(int) >= tamanio_pagina)
			{//Se pasa de la pagina => es la siguiente
				var.pag = anterior.pag + 1;
				var.offset = (anterior.offset + sizeof(int) ) % tamanio_pagina;
			}else
			{//No se pasa de la pagina, calculo mas facil :D
				var.pag = anterior.pag;
				var.offset = anterior.offset + sizeof(int);
			}

			return var;
		}else
		{//La variable esta en otro contexto

			//Si es el main => la posicion es la cant_pag_cod 0
			if(pcb_actual.stack.cant_entradas == 1)
			{
				var.pag = pcb_actual.cant_pag_cod;
				var.offset = 0;
				return var;
			}else
			{//La variable esta en la funcion anterior
				anterior = pcb_actual.stack.entradas[funcionAnterior].variables[pcb_actual.stack.entradas[funcionAnterior].cant_var - 1];

				//Calculo pag y offset
				if(anterior.offset + sizeof(int) >= tamanio_pagina)
				{//Se pasa de la pagina => es la siguiente
					var.pag = anterior.pag + 1;
					var.offset = (anterior.offset + sizeof(int) ) % tamanio_pagina;
				}else
				{//No se pasa de la pagina, calculo mas facil :D
					var.pag = anterior.pag;
					var.offset = anterior.offset + sizeof(int);
				}

				return var;
			}
		}
	}

	printf("ANSISOP ------- Ejecuto Definir Variable. Variable: %c ----\n", identificador_variable);
	fprintf(log,"ANSISOP ------- Ejecuto Definir Variable. Variable: %c ----\n", identificador_variable);
	/*Agrego una variable a la entrada de stack correspondiente*/

	if(pcb_actual.stack.entradas[funcionActual].variables == NULL)
	{
		printf("\nNULL.\n");
	}


	pcb_actual.stack.entradas[funcionActual].cant_var++;
	pcb_actual.stack.entradas[funcionActual].variables = realloc(pcb_actual.stack.entradas[funcionActual].variables,pcb_actual.stack.entradas[funcionActual].cant_var * sizeof(t_var));

	varActual = calcularVariable(pcb_actual);
	varActual.identificador = identificador_variable;

	t_puntero puntero = varActual.pag * tamanio_pagina + varActual.offset;

	printf("pag: %d, offset: %d.\n",varActual.pag,varActual.offset);

	return puntero;

}

/*
 *  FUNCION     : Devuelve el desplazamiento respecto al inicio del segmento Stack en que se encuentra
 *  			  el valor de la variable identificador_variable del contexto actual.
 *  Recibe      : t_nombre_variable variable
 *  Devuelve    : t_puntero (En caso de error, retorna -1.)
 */
t_puntero socketes_obtenerPosicionVariable(t_nombre_variable identificador_variable) {

	printf("ANSISOP ------- Ejecuto ObtenerPosicionVariable. Variable: %c ----\n", identificador_variable);
	fprintf(log,"ANSISOP ------- Ejecuto ObtenerPosicionVariable. Variable: %c ----\n", identificador_variable);

	//Busco en el stack, una variable del contexto actual que tenga el mismo identificador
	int i;
	for(i=0; i<pcb_actual.stack.entradas[funcionActual].cant_var; i++)
	{
		if(pcb_actual.stack.entradas[funcionActual].variables[i].identificador == identificador_variable)
		{
			return pcb_actual.stack.entradas[funcionActual].variables[i].pag * tamanio_pagina +
				   pcb_actual.stack.entradas[funcionActual].variables[i].offset;
		}
	}
	//Busco en la lista de argumentos
	for(i=0; i<pcb_actual.stack.entradas[funcionActual].cant_arg; i++)
	{
		if(pcb_actual.stack.entradas[funcionActual].argumentos[i].identificador == identificador_variable)
		{
			return pcb_actual.stack.entradas[funcionActual].argumentos[i].pag * tamanio_pagina +
				   pcb_actual.stack.entradas[funcionActual].argumentos[i].offset;
		}
	}

	//Pase por estos 2 for y no encontro => hay error
	return -1;

}

/*
 *  FUNCION     : Obtiene el valor resultante de leer a partir de direccion_variable,
 *  			  sin importar cual fuera el contexto actual.
 *  Recibe      : t_puntero puntero (Lugar donde buscar)
 *  Devuelve    : t_valor_variable (Valor que se encuentra en esa posicion)
 */
t_valor_variable socketes_dereferenciar(t_puntero direccion_variable) {

	//t_valor_variable valor_variable;
	printf("ANSISOP ------- Ejecuto dereferenciar. Direccion_variable: %d ----\n", direccion_variable);
	fprintf(log,"ANSISOP ------- Ejecuto dereferenciar. Direccion_variable: %d ----\n", direccion_variable);

	//Pido a umc el valor de la variable
	int mensaje[5];
	mensaje[0] = LEER;
	mensaje[1] = direccion_variable / tamanio_pagina;//Pagina en la que esta la variable
	mensaje[2] = direccion_variable % tamanio_pagina;//Offset de la pagina
	mensaje[3] = sizeof(int);		 				 //Size de la instruccion
	mensaje[4] = pcb_actual.pid;					 //pid

	send(socket_umc, &mensaje, 5*sizeof(int), 0);

	recv(socket_umc,&mensaje,sizeof(int),0);
	if(mensaje[0] == OVERFLOW)
	{
		printf("Error al solicitar instruccion: STACK_OVERFLOW");
		estado = ERROR;
		return 0;
	}

	int resultado;

	printf("Espero el resultado...\n");
	if( recv(socket_umc,&resultado,sizeof(int),0) <= 0)
	{
		perror("Error al recibir instruccion");
	}

	printf("\nValor de la variable: %d \n",resultado);

	return resultado;

}

/*
 *  FUNCION     : Copia un valor en la variable ubicada en direccion_variable.
 *  Recibe      : t_posicion direccion_variable, t_valor_variable valor
 *  Devuelve    : void
 */
void socketes_asignar(t_puntero direccion_variable, t_valor_variable valor) {

	printf("ANSISOP ------- Ejecuto asignar: en la direccion %d, el valor %d ----\n", direccion_variable, valor);
	fprintf(log,"ANSISOP ------- Ejecuto asignar: en la direccion %d, el valor %d ----\n", direccion_variable, valor);

	int mensaje[6];
	mensaje[0] = ESCRIBIR;
	mensaje[1] = direccion_variable / tamanio_pagina;//Pagina en la que esta la variable
	mensaje[2] = direccion_variable % tamanio_pagina;//Offset de la pagina
	mensaje[3] = sizeof(int);		 				 //Size de la escritura
	mensaje[4] = pcb_actual.pid;					 //pid
	mensaje[5] = valor;								 //Valor a enviar

	printf("Pag: %d, offset: %d, size: %d, pid: %d, valor: %d.\n",mensaje[1],mensaje[2],
			mensaje[3],mensaje[4],mensaje[5]);

	if( sendAll(socket_umc, &mensaje, 6*sizeof(int), 0) <= 0)
	{
		perror("Error al asignar variable");
	}

	//Recibo la respuesta si el pedido fue correcto o no
	recv(socket_umc,&mensaje,sizeof(int),0);
	if(mensaje[0] == OVERFLOW)
	{
		printf("Pedido incorrecto: STACK_OVERFLOW.\n");
		estado = ERROR;
	}

	return;

}


/*
 *  FUNCION     : Solicita al Núcleo el valor de una variable compartida
 *  Recibe      : t_nombre_compartida variable
 *  Devuelve    : t_valor_variable
 */
t_valor_variable socketes_obtenerValorCompartida(t_nombre_compartida variable){

	printf("ANSISOP ------- Ejecuto ObtenerValorCompartida. Variable: %s ----\n", variable);
	fprintf(log,"ANSISOP ------- Ejecuto ObtenerValorCompartida. Variable: %s ----\n", variable);

	printf("tamaño cadena: %d.\n",strlen(variable));

	if(variable[strlen(variable) - 1] == '\n'){
		variable[strlen(variable) - 1] = '\0';
		printf("Saco el barra n de la cadena.\n");
	}

	printf("Nuevo tamaño cadena: %d.\n",strlen(variable));

	printf("Nueva cadena: %s.\n",variable);

	int mensaje = ANSISOP_OBTENER_VALOR_COMPARTIDO;
	int pid = pcb_actual.pid;
	int tamanio_cadena = strlen(variable) + 1;

	char *buffer = malloc(3*sizeof(int) + tamanio_cadena);

	memcpy(buffer,&mensaje,sizeof(int));
	memcpy(buffer + sizeof(int),&pid,sizeof(int));
	memcpy(buffer + 2*sizeof(int),&tamanio_cadena,sizeof(int));
	memcpy(buffer + 3*sizeof(int),variable,tamanio_cadena);

	buffer[4*sizeof(int) + tamanio_cadena - 1] = '\0';

	if( sendAll(socket_nucleo,buffer,3*sizeof(int) + tamanio_cadena,0) <= 0 )
	{
		perror("Error al enviar mensaje para obtener compartida");
	}
	free(buffer);

	recv(socket_nucleo,&mensaje,sizeof(int),0);
	if( mensaje == KILL_PROGRAMA )
	{
		estado = ERROR;
		return 0;
	}

	//Espero la vuelta del mensaje
	int valor_compartida;

	recv(socket_nucleo,&valor_compartida,sizeof(int),0);

	printf("El valor de la variable compartida: %s es: %d",variable,valor_compartida);

	return valor_compartida;

}


/*
 *  FUNCION     : Solicita al Núcleo asignar el valor a la variable compartida. Devuelve el valor asignado.
 *  Recibe      : t_nombre_compartida variable, t_valor_variable valor_asignado
 *  Devuelve    : t_valor_variable valor_asignado
 */
t_valor_variable socketes_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor_asignado){

	printf("ANSISOP ------- Ejecuto asignarValorCompartida. Variable: %s. | Valor asignado: %d. ----\n", variable, valor_asignado);
	fprintf(log,"ANSISOP ------- Ejecuto asignarValorCompartida. Variable: %s. | Valor asignado: %d. ----\n", variable, valor_asignado);

	printf("tamaño cadena: %d.\n",strlen(variable));

	if(variable[strlen(variable) - 1] == '\n'){
		variable[strlen(variable) - 1] = '\0';
		printf("Saco el barra n de la cadena.\n");
	}

	printf("Nuevo tamaño cadena: %d.\n",strlen(variable));

	printf("Nueva cadena: %s.\n",variable);

	int mensaje = ANSISOP_ASIGNAR_VALOR_COMPARTIDO;
	int pid = pcb_actual.pid;
	int tamanio_cadena = strlen(variable) + 1;

	char *buffer = malloc(4*sizeof(int) + tamanio_cadena);

	memcpy(buffer,&mensaje,sizeof(int));
	memcpy(buffer + sizeof(int),&pid,sizeof(int));
	memcpy(buffer + 2*sizeof(int),&valor_asignado,sizeof(int));
	memcpy(buffer + 3*sizeof(int),&tamanio_cadena,sizeof(int));
	memcpy(buffer + 4*sizeof(int),variable,tamanio_cadena);

	buffer[4*sizeof(int) + tamanio_cadena - 1] = '\0';

	if( sendAll(socket_nucleo,buffer,4*sizeof(int) + tamanio_cadena, 0) <= 0 )
	{
		perror("Error al enviar mensaje para asignar compartida");
	}
	free(buffer);

	recv(socket_nucleo,&mensaje,sizeof(int),0);
	if( mensaje == KILL_PROGRAMA )
	{
		estado = ERROR;
		return 0;
	}

	return valor_asignado;

}

/*
 *  FUNCION     : Cambia la linea de ejecucion a la correspondiente de la etiqueta buscada.
 *  Recibe      : t_nombre_etiqueta etiqueta
 *  Devuelve    : void
 */
void socketes_irAlLabel(t_nombre_etiqueta etiqueta){

	printf("Longitud etiqueta: %d.\n",strlen(etiqueta));
	etiqueta[strlen(etiqueta) - 1] = '\0';
	etiqueta[strlen(etiqueta)] = '\0';

	printf("ANSISOP_IR_A_LABEL: %s.\n",etiqueta);
	fprintf(log,"ANSISOP_IR_A_LABEL: %s.\n",etiqueta);
	//Ya esta hecho :D
	pcb_actual.PC = metadata_buscar_etiqueta(etiqueta,
						pcb_actual.indice_etiquetas.etiquetas,
						pcb_actual.indice_etiquetas.etiquetas_size);

	printf("\n ETIQUETAS SIZE: %d\n",pcb_actual.indice_etiquetas.etiquetas_size);

	printf("INSTRUCCION DEL IR A LABEL: %d.\n", pcb_actual.PC);
	fprintf(log,"INSTRUCCION DEL IR A LABEL: %d.\n", pcb_actual.PC);

	if(pcb_actual.PC == -1){
		printf("ERROR INDICE DE ETIQUETAS DEVOLVIO -1.\n");
		fprintf(log,"ERORR INDICE DE ETIQUETAS DEVOLVIO -1.\n");
	}

	printf("INFORMACION INDICE ETIQUETAS:\n\n");
	fwrite(pcb_actual.indice_etiquetas.etiquetas,sizeof(char),pcb_actual.indice_etiquetas.etiquetas_size,stdout);
	printf("\n\n");

	fprintf(log,"INFORMACION INDICE ETIQUETAS:\n\n");
	fwrite(pcb_actual.indice_etiquetas.etiquetas,sizeof(char),pcb_actual.indice_etiquetas.etiquetas_size,log);
	fprintf(log,"\n\n");

	//printf("Mi propio buscar etiqueta devuelve: %d", buscarEtiqueta(etiqueta) );
	//pcb_actual.PC = buscarEtiqueta(etiqueta);
	pcb_actual.PC--;
}

void socketes_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	printf("Llamar con retorno:\n");
	fprintf(log,"Llamar con retorno:\n");

	//Cambio el PC
	socketes_irAlLabel(etiqueta);

	//Creo un nuevo contexto(entrada en el stack)
	pcb_actual.stack.cant_entradas++;
	pcb_actual.stack.entradas = realloc(pcb_actual.stack.entradas,pcb_actual.stack.cant_entradas * sizeof(t_entrada_stack));

	pcb_actual.stack.entradas[funcionActual].cant_arg = 0;
	pcb_actual.stack.entradas[funcionActual].cant_var = 0;
	pcb_actual.stack.entradas[funcionActual].argumentos = NULL;
	pcb_actual.stack.entradas[funcionActual].variables = NULL;
	pcb_actual.stack.entradas[funcionActual].dirRetorno = metadata_buscar_etiqueta(etiqueta,
																pcb_actual.indice_etiquetas.etiquetas,
																pcb_actual.indice_etiquetas.etiquetas_size);
	pcb_actual.stack.entradas[funcionActual].pagRet = donde_retornar / tamanio_pagina;
	pcb_actual.stack.entradas[funcionActual].offsetRet = donde_retornar % tamanio_pagina;
}

t_puntero_instruccion socketes_retornar(t_valor_variable retorno){

	//Busco en el stack la posicion donde retornar, asigno el valor en esa posicion y listo.
	printf("Primitiva Retornar ---- valor: %d.\n", retorno);
	fprintf(log,"Primitiva Retornar ---- valor: %d.\n", retorno);

	//Extraigo los valores dobre donde retornar
	int pag = pcb_actual.stack.entradas[funcionActual].pagRet;
	int offset = pcb_actual.stack.entradas[funcionActual].offsetRet;
	int instruccion_retorno = pcb_actual.stack.entradas[funcionActual].dirRetorno;

	t_puntero direccion = pag * tamanio_pagina + offset;

	socketes_asignar(direccion,retorno);

	//Cambio el contexto al anterior
	free(pcb_actual.stack.entradas[funcionActual].argumentos);
	free(pcb_actual.stack.entradas[funcionActual].variables);

	pcb_actual.stack.cant_entradas--;
	pcb_actual.stack.entradas = realloc(pcb_actual.stack.entradas,pcb_actual.stack.cant_entradas);

	printf("Termino de ejecutar retornar.\n");

	return instruccion_retorno;
}

/*
 *  FUNCION     : Envia al Núcleo el contenido de valor_mostrar, para que se muestre en la consola correspondiente.
 *  Recibe      : el valor del variable a mostrar
 *  Devuelve    : void / int (TODO chequear)
 */
void socketes_imprimir(t_valor_variable valor_mostrar) {

	printf("ANSISOP ------- Ejecuto Imprimir: %d ----\n", valor_mostrar);
	fprintf(log,"ANSISOP ------- Ejecuto Imprimir: %d ----\n", valor_mostrar);

	int buffer[3];

	buffer[0] = ANSISOP_IMPRIMIR;
	buffer[1] = pcb_actual.pid;
	buffer[2] = valor_mostrar;

	printf("Envio mensaje imprimir al Núcleo con el valor: %d (id del mensaje: %d)\n", buffer[1], buffer[0]);

	send(socket_nucleo, &buffer, 3*sizeof(int), 0);
}

/*
 *  FUNCION     : Envia al Núcleo una cadena de texto, para que se muestre en la consola correspondiente.
 *  Recibe      : cadena a mostrar por consola del programa
 *  Devuelve    : void / int (TODO chequear)
 */
void socketes_imprimirTexto(char* texto) {

	printf("ANSISOP ------- Ejecuto ImprimirTexto: %s ----\n", texto);
	fprintf(log,"ANSISOP ------- Ejecuto ImprimirTexto: %s ----\n", texto);

	int id_mensaje = ANSISOP_IMPRIMIR_TEXTO;
	int tamanio_texto = strlen(texto) + 1;
	int pid = pcb_actual.pid;

	char *buffer = malloc(3*sizeof(int) +tamanio_texto);

	memcpy(buffer, &id_mensaje, sizeof(int));
	memcpy(buffer + sizeof(int),&pid,sizeof(int));
	memcpy(buffer + 2*sizeof(int), &tamanio_texto, sizeof(int));
	memcpy(buffer + 3*sizeof(int), texto, tamanio_texto);

	printf("Envio mensaje imprimir texto al Núcleo con el valor: %s\n", texto);

	if( sendAll(socket_nucleo, &buffer, 3*sizeof(int) + tamanio_texto, 0) <= 0 )
	{
		perror("Error al enviar texto a nucleo");
	}
	free(buffer);
}

/*
 *  FUNCION     : Informa al Núcleo que el Programa actual pretende utilizar el dispositivo
 *  			  durante tiempo unidades de tiempo.
 *  Recibe      : t_nombre_dispositivo dispositivo, int tiempo
 *  Devuelve    : void
 */
void socketes_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){

	printf("ANSISOP ------- Ejecuto Entrada-Salida. Dispositivo :%s | Unidades de tiempo: %d ----\n", dispositivo, tiempo);
	fprintf(log,"ANSISOP ------- Ejecuto Entrada-Salida. Dispositivo :%s | Unidades de tiempo: %d ----\n", dispositivo, tiempo);

	int mensaje = ANSISOP_ENTRADA_SALIDA;
	int tamanio_texto = strlen(dispositivo) + 1;

	char *buffer = malloc(3*sizeof(int) + tamanio_texto);

	memcpy(buffer,&mensaje,sizeof(int));
	memcpy(buffer + sizeof(int),&tiempo,sizeof(int));
	memcpy(buffer + 2*sizeof(int),&tamanio_texto,sizeof(int));
	memcpy(buffer + 3*sizeof(int),dispositivo,tamanio_texto);

	if( sendAll(socket_nucleo,buffer,3*sizeof(int) + tamanio_texto,0) <= 0)
	{
		perror("Error al enviar mensaje solicitud Entrada/Salida");
	}
	free(buffer);

	recv(socket_nucleo,&mensaje,sizeof(int),0);

	if(mensaje == KILL_PROGRAMA)
	{
		estado = ERROR;
		return;
	}

	//Ya envie el mensaje, ahora envio el pcb
	estado = ENTRADA_SALIDA;
	//enviarPcb(pcb_actual,socket_nucleo,-1);//Tener en cuenta que esto envia FIN_QUANTUM
}

/*
 *  FUNCION     : Informa al Núcleo que ejecute la función wait para el semáforo indicado.
 *  Recibe      : t_nombre_semaforo identificador_semaforo
 *  Devuelve    : void
 */
void socketes_wait(t_nombre_semaforo identificador_semaforo){

	printf("ANSISOP ------- Ejecuto WAIT: %s ----\n", identificador_semaforo);
	fprintf(log,"ANSISOP ------- Ejecuto WAIT: %s ----\n", identificador_semaforo);

	printf("tamaño cadena: %d.\n",strlen(identificador_semaforo));

	identificador_semaforo[strlen(identificador_semaforo) - 1] = '\0';
	identificador_semaforo[strlen(identificador_semaforo)] = '\0';

	printf("Nuevo tamaño cadena: %d.\n",strlen(identificador_semaforo));

	int mensaje = ANSISOP_WAIT;
	int pid = pcb_actual.pid;
	int tamanio_cadena = strlen(identificador_semaforo) + 1;

	char *buffer = malloc(3*sizeof(int) + tamanio_cadena);

	memcpy(buffer,&mensaje,sizeof(int));
	memcpy(buffer + sizeof(int),&pid,sizeof(int));
	memcpy(buffer + 2*sizeof(int),&tamanio_cadena,sizeof(int));
	memcpy(buffer + 3*sizeof(int),identificador_semaforo,tamanio_cadena);

	if( sendAll(socket_nucleo,buffer,3*sizeof(int)+tamanio_cadena,0) <= 0)
	{
		perror("Error al mandar wait");
	}
	free(buffer);

	recv(socket_nucleo,&mensaje,sizeof(int),0);
	if( mensaje == KILL_PROGRAMA )
	{
		estado = ERROR;
		return;
	}

	//Espero a que nucleo me responda si puedo seguir ejecutando o no.
	recv(socket_nucleo,&mensaje,sizeof(int),0);

	if(mensaje == ANSISOP_PODES_SEGUIR)
	{
		printf("Puedo seguir ejecutando.\n");
		return;
	}
	else if(mensaje == ANSISOP_BLOQUEADO)
	{
		printf("Me bloquee.\n");

		estado = WAIT;

		return;
	}else
	{
		printf("Recibi un mensaje erroneo de nucleo al enviar wait: %d", mensaje);
	}
/*
	int header_mensaje = ANSISOP_WAIT;

	printf("Size of identificador: %d ----\n", sizeof(t_nombre_semaforo));

	char *mensaje = (char *)malloc(sizeof(int)+sizeof(t_nombre_semaforo));

	memcpy(mensaje, &header_mensaje, sizeof(int));
	memcpy(mensaje + sizeof(int), &identificador_semaforo, sizeof(t_nombre_semaforo));

	printf("Envio mensaje wait al Núcleo con el identificador del semaforo\n");

	send(socket_nucleo, &mensaje, 2*sizeof(int), 0);
*/

}


/*
 *  FUNCION     : Informa al Núcleo que ejecute la función signal para el semáforo indicado.
 *  Recibe      : t_nombre_semaforo identificador_semaforo
 *  Devuelve    : void
 */
void socketes_signal(t_nombre_semaforo identificador_semaforo){

	printf("ANSISOP ------- Ejecuto SIGNAL: %s ----\n", identificador_semaforo);
	fprintf(log,"ANSISOP ------- Ejecuto SIGNAL: %s ----\n", identificador_semaforo);

	printf("tamaño cadena: %d.\n",strlen(identificador_semaforo));

	identificador_semaforo[strlen(identificador_semaforo) - 1] = '\0';
	identificador_semaforo[strlen(identificador_semaforo)] = '\0';

	printf("Nuevo tamaño cadena: %d.\n",strlen(identificador_semaforo));

	int mensaje = ANSISOP_SIGNAL;
	int pid = pcb_actual.pid;
	int tamanio_cadena = strlen(identificador_semaforo) + 1;

	char *buffer = malloc(3*sizeof(int) + tamanio_cadena);

	memcpy(buffer,&mensaje,sizeof(int));
	memcpy(buffer + sizeof(int),&pid,sizeof(int));
	memcpy(buffer + 2*sizeof(int),&tamanio_cadena,sizeof(int));
	memcpy(buffer + 3*sizeof(int),identificador_semaforo,tamanio_cadena);

	if( sendAll(socket_nucleo,buffer,3*sizeof(int)+tamanio_cadena,0) <= 0)
	{
		perror("Error al mandar signal");
	}
	free(buffer);

	recv(socket_nucleo,&mensaje,sizeof(int),0);
	if( mensaje == KILL_PROGRAMA )
	{
		estado = ERROR;
		return;
	}

}


void socketes_finalizar(){
	printf("ANSISOP ------- Finalizar Programa. ----\n");
	fprintf(log,"ANSISOP ------- Finalizar Programa. ----\n");

	int mensaje[2] = {ANSISOP_FIN_PROGRAMA, pcb_actual.pid};

	send(socket_nucleo,&mensaje,2*sizeof(int),0);

	estado = FIN_PROGRAMA;
}
