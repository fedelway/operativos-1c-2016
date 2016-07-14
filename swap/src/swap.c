/*
 ============================================================================
 Name        : swap.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "swap.h"
//TODO VER SI LOS PUEDO PONER EN EL .H
t_config* config;
t_log* logger;
char *archivoMapeado;

//TODO ver si ésto en swap cambia
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar


int main(int argc, char *argv[]){

	char message[PACKAGESIZE];
	memset(message, '\0', PACKAGESIZE);

	logger = log_create("swap.log", "SWAP", true, LOG_LEVEL_INFO);
	log_info(logger, "Proyecto para SWAP..");

	listaProcesos = list_create();
	listaEnEspera = list_create();

	crearConfiguracion(argv[1]);
	//reservo memoria para BitMap
	bitarray = (char*) malloc(CANTIDAD_PAGINAS * sizeof(int));

	crearParticionSwap();
	crearBitMap();
	archivoMapeado = cargarArchivo();

	PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	log_info(logger, "Mi puerto escucha es: %s", PUERTO_ESCUCHA);
	socket_escucha = conectarPuertoDeEscucha(PUERTO_ESCUCHA);

	printf("Aun no Se ha conectado una umc.\n");
	socket_umc = aceptarConexion(socket_escucha);
	printf("Socket umc %d\n", socket_umc);

	handshakeUMC();

	trabajarUmc();

	log_destroy(logger);

	eliminarEstructuras();

	return EXIT_SUCCESS;

}

//----------------------------CONFIGURACION -------------------------------------------//

void crearConfiguracion(char *config_path){

	config = config_create(config_path);
	PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	NOMBRE_SWAP = config_get_string_value(config, "NOMBRE_SWAP");
	CANTIDAD_PAGINAS = config_get_int_value(config, "CANTIDAD_PAGINAS");
	TAMANIO_PAGINA = config_get_int_value(config, "TAMANIO_PAGINA");
	RETARDO_COMPACTACION = config_get_int_value(config, "RETARDO_COMPACTACION");


	if(validarParametrosDeConfiguracion()){
		log_info(logger, "El archivo de configuración tiene todos los parametros requeridos.");
		return;
	}else{
		log_error(logger, "Configuración no válida");
		// TODO: VER SI ESTA OK DESTROY ACÁ
		log_destroy(logger);
		exit(EXIT_FAILURE);
	}
}

//Valida que todos los parámetros existan en el archivo de configuración
bool validarParametrosDeConfiguracion(){

	return (config_has_property(config, "PUERTO_ESCUCHA")
			&& config_has_property(config, "NOMBRE_SWAP")
			&& config_has_property(config, "CANTIDAD_PAGINAS")
			&& config_has_property(config, "TAMANIO_PAGINA")
			&& config_has_property(config, "RETARDO_COMPACTACION"));
}

//----------------------------------- CONEXIONES ---------------------------------------//

//Hacemos el handshake con la UMC
void handshakeUMC(){

	int msj_recibido;
	int soy_swap = SOY_SWAP;

	log_info(logger, "Iniciando Handshake con la UMC");

	send(socket_umc, &soy_swap, sizeof(int), 0);
	recv(socket_umc, &msj_recibido, sizeof(int), 0);

	if(msj_recibido == SOY_UMC){
		log_info(logger, "UMC validado.");
		printf("Se ha validado correctamente la UMC. \n");
	}else{
		log_error(logger, "La UMC no pudo ser validada.");
		log_destroy(logger);
		exit(0);
	}
}

//---------------------------- TRABAJAR CON LA UMC ----------------------//

// HAGO DEFINE PARA SABER CUANDO ES UN PROCESO EN ESPERA
bool esUnProcesoEnEspera(int tipoProceso){

	return tipoProceso == PROCESO_EN_ESPERA;
}

void atenderPeticiones(msj_recibido, tipoProceso){

	int  pid, tamanio, numeroPagina;
	char* contenido = malloc(TAMANIO_PAGINA);

	switch(msj_recibido){

	case RESERVA_ESPACIO :


		recv(socket_umc , &pid, sizeof(int), 0);
		recv(socket_umc , &tamanio, sizeof(int), 0);

		crearProgramaAnSISOP(pid,tamanio);

		break;

	case LEER_PAGINA:

		recv(socket_umc , &pid, sizeof(int), 0);
		recv(socket_umc , &numeroPagina, sizeof(int), 0);

		leerUnaPagina(pid, numeroPagina);

		break;

	case MODIFICAR_PAGINA:

		recv(socket_umc , &pid, sizeof(int), 0);
		recv(socket_umc , &tamanio, sizeof(int), 0);
		recv(socket_umc , &contenido, TAMANIO_PAGINA, 0);

		modificarPagina(pid, numeroPagina, contenido);

		break;

	case TERMINAR_PROGRAMA:

		recv(socket_umc , &pid, sizeof(int), 0);

		terminarProceso(pid);

		break;

	default:

		printf("Mensaje Erroneo");

	}
	free(contenido);
}


void atenderPeticionesEnspera(nodo_enEspera *nodo){

	int pid = nodo->pid;
	int tamanio= nodo->cantidad_paginas;
	int numeroPagina = nodo->numeroPagina;
	char* contenido = malloc(TAMANIO_PAGINA);
	memcpy(nodo->contenido, contenido,TAMANIO_PAGINA);
	int mensaje = nodo->mensaje;

	switch(mensaje){

	case RESERVA_ESPACIO:

		crearProgramaAnSISOP(pid,tamanio);

		break;

	case LEER_PAGINA:

		leerUnaPagina(pid, numeroPagina);

		break;

	case MODIFICAR_PAGINA:


		modificarPagina(pid, numeroPagina, contenido);

		break;

	case TERMINAR_PROGRAMA:

		terminarProceso(pid);

		break;

	default:

		printf("Mensaje Erroneo");

	}
	free(contenido);
	free(nodo);
}


void trabajarUmc(){

	printf("Trabajar con UMC \n");
	int msj_recibido;

	//Ciclo infinito
	for(;;){
		//Recibo mensajes de nucleo y hago el switch
		recv(socket_umc , &msj_recibido, sizeof(int), 0);

		//Chequeo que no haya una desconexion
		if(msj_recibido <= 0){
			printf("Desconexion de la umc. Terminando...\n");
			exit(1);
		}

		if(estaCompactando){ //ver si hay que poner = 1
			encolarProgramas(msj_recibido);
		}else{

			if(hayProgramasEnEspera == 1){  //ver como compararlo

				pthread_mutex_lock(&peticionesActuales);
				atenderProcesosEnEspera();
				pthread_mutex_unlock(&peticionesActuales);

			}else{

				pthread_mutex_lock(&enEspera);
				atenderPeticiones(msj_recibido);
				pthread_mutex_lock(&enEspera);

			}
		}
	}
}

//------------------------------ ALMACENAMIENTO EN SWAP ---------------------------//

void crearParticionSwap(){

	char comando[50];
	printf("Creando archivo Swap: \n");
	sprintf(comando, "dd if=/dev/zero bs=%d count=1 of=%s",	CANTIDAD_PAGINAS * TAMANIO_PAGINA, NOMBRE_SWAP);
	system(comando);
}

char* cargarArchivo(){

	FILE *file = fopen(NOMBRE_SWAP, "r+");// todo: falta verificar
	int fd =fileno(file);  // Esto traduce a entero el fd


	int pagesize = TAMANIO_PAGINA * CANTIDAD_PAGINAS; // TAMANIO 256 Y CANT PAGINAS 512 EN NUESTRO CASO
	char* accesoAMemoria = mmap( NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if(accesoAMemoria == (caddr_t) (-1)){
		perror("mmap");
		exit(1);
	}
	return accesoAMemoria;
}


//------------------------------ FUNCIONES SOBRE EL BITMAP ----------------------------//

//Creo BitMap usando bitarray
void crearBitMap(){

	bitMap = bitarray_create(bitarray, CANTIDAD_PAGINAS);
}

//Paso cada página del BITMAP a ocupada
void actualizarBitMap(int pid, int pagina, int cant_paginas){

	int i;
	int cant = 0;
	for (i = 1; i <= cant_paginas; i++){  // ver =0 o =1

		bitarray_set_bit(bitMap, pagina + cant);
		cant = cant + 1;
	}
}

//--------------------------- CREACION DE NODOS DE LAS LISTAS -------------------------//

nodo_proceso *crearNodoDeProceso(int pid, int cantidad_paginas,int posSwap){
	nodo_proceso *proceso = malloc(sizeof(nodo_proceso));
	proceso->pid = pid;
	proceso->cantidad_paginas = cantidad_paginas;
	proceso->posSwap = posSwap;
	return proceso;
}

nodo_enEspera *crearNodoEnEspera(int pid, int cantidad_paginas,int numeroPagina,char* contenido, int mensaje){
	nodo_enEspera *enEspera = malloc(sizeof(nodo_enEspera));
	enEspera->pid = pid;
	enEspera->cantidad_paginas = cantidad_paginas;
	enEspera->numeroPagina = numeroPagina;
	//enEspera->contenido;
	enEspera->contenido = malloc(TAMANIO_PAGINA);
	memcpy(enEspera->contenido, contenido,TAMANIO_PAGINA);
	enEspera->mensaje = mensaje;
	return enEspera;
}

void agregarNodoEnEspera(int pid, int cantidadPaginas,  int numeroPagina, char* contenido, int mensaje){

	list_add(listaEnEspera, crearNodoEnEspera(pid, cantidadPaginas, numeroPagina, contenido, mensaje));

}

void agregarNodoProceso(int pid, int cantidadPaginas, int posicionSwap){

	list_add(listaProcesos, crearNodoDeProceso(pid, cantidadPaginas , posicionSwap));

}

int cantPaginas(nodo_proceso *nodo){
	return nodo->cantidad_paginas;
}

int numeroPid(nodo_proceso *nodo){
	return nodo->pid;
}

int posicionSwap(nodo_proceso *nodo){
	return nodo->posSwap;
}

int cantPaginasEnEspera(nodo_enEspera *nodo){
	return nodo->cantidad_paginas;
}

int numeroPidEnEspera(nodo_enEspera *nodo){
	return nodo->pid;
}

//------------------------- FUNCIONES PARA DETECTAR LUGAR LIBRE ------------------//

//VERIFICA A PARTIR DE UNA PÁGINA DISPONIBLE, HAY ESPACIO CONTÍGUO
bool hayEspacioContiguo(int pagina, int tamanio){
	int i;
	//la idea es verificar el cacho de registros del vector que pueden alocar ese tamanio

	for (i = pagina; i <= pagina + tamanio; i++){
		if (bitarray_test_bit(bitMap, i) == 0 && tamanio + i  <= CANTIDAD_PAGINAS){ //== 0 libre == 1 ocupado
			return 1;
		}
	}
	return 0; // retorna F, No hay espacio disponible
}

int paginaDisponible(int tamanio){
	printf("pasó por pagina disponible \n");
	int i;
	//Recorro todo el bitMap buscando espacios contiguos
	//devuelvo la pagina desde donde tiene que reservar memoria, si no encuentra lugar devuelve -1
	for(i = 0; i <= CANTIDAD_PAGINAS; i++){
		if(bitarray_test_bit(bitMap, i) == 0){	// 1 VER, 0 FALSO
			if(hayEspacioContiguo(i, tamanio)){
				return i;

			}
		}
	}
	return -1;
}

//Recorrer la lista de procesos en swap y devolver (int) la ubicación // resuelto con list_find()!
int ubicacionEnSwap(int pid){ //FUNCIONA

	bool igualPid(nodo_proceso *nodo){
		return nodo->pid == pid;
	}

	nodo_proceso *nodo = list_find(listaProcesos,(void*)igualPid);
	if (nodo != NULL){
		int posSwap = nodo->posSwap;
		return posSwap;
	}else{
		return -1;
		puts("No se encontró el proceso solicitado\n");
	}
}
//---------------------- PETICIONES UMC --------------------------------//

//Copia el código en Swap, Agrega nodo a la lista de procesos, actualiza el BITMAP y retorna Resultado
void crearProgramaAnSISOP(int pid, int cant_paginas){

	int pagina = paginaDisponible(cant_paginas);
	if(pagina != -1){

		//ACTUALIZO LAS ESTRUCTURAS
		agregarNodoProceso(pid, cant_paginas, pagina);
		actualizarBitMap(pid, pagina, cant_paginas);
		printf("Se ha reservado espacio para el programa n°: %d", pid);

		//Aviso a umc que se pudo reservar espacio
		int mensaje = SWAP_PROGRAMA_OK;
		send(socket_umc,&mensaje,sizeof(int),0);

	} else {

		if(hayFragmentacion(cant_paginas)){

			//SEMÁFORO MIENTRAS COMPACTO
			pthread_mutex_lock(&peticionesActuales);
			pthread_mutex_lock(&enEspera);

			comenzarCompactacion();

			pthread_mutex_unlock(&peticionesActuales);
			pthread_mutex_unlock(&enEspera);

			//Ya compacte, llamo de vuelta a crearPrograma para ver si ahora si hay espacio
			crearProgramaAnSISOP(pid, cant_paginas);

		}else{

			cancelarInicializacion();
		}
	}
}

void leerUnaPagina(int pid, int pagina){

	int posSwap = ubicacionEnSwap(pid);

	if(posSwap != -1){

		char* bytes = malloc(TAMANIO_PAGINA);
		//memcpy(bytes, archivoMapeado + posSwap * tamanio_pagina,tamanio_pagina);
		memcpy(bytes,archivoMapeado +posSwap + pagina*TAMANIO_PAGINA, TAMANIO_PAGINA);
		printf("El contenido de la página es: %s\n ", bytes);

		//Enviar contenido de pagina leida
		int resultado = send(socket_umc, bytes, TAMANIO_PAGINA, 0);

		if(resultado < 0){
			log_error_y_cerrar_logger(logger, "Falló envío de mensaje de lectura a UMC  . | Nro de programa: %d | Nro de pagina: %d", pid, pagina);
			exit(EXIT_FAILURE);
		}else{
			puts("No se encontró el contenido de la página solicitada");
		}
	}
}

void modificarPagina(int pid, int pagina, char* nuevoCodigo){

	printf("entra a modificar\n");
	int posSwap = ubicacionEnSwap(pid);

	int posEscribir = (pagina - 1) * TAMANIO_PAGINA;

	if(posSwap != -1){
		memcpy(archivoMapeado + posSwap + posEscribir, nuevoCodigo, TAMANIO_PAGINA);

		//memcpy(&archivoMapeado[posSwap], &nuevoCodigo, tamanio_pagina));
		//printf("la modificacion en la pagina n° %d es: %s", posSwap, archivoMapeado);
	} else {
		printf("Error al escribir la pagina");
	}
}

//------------------------------------------------------------------------------------------------//

int espaciosLibres(int cantidad){

	int i, cantLibres = 0;
	//Recorro todo el bitMap buscando espacios separados
	for(i = 0; i <= TAMANIO_PAGINA * CANTIDAD_PAGINAS; i++){
		if(bitarray_test_bit(bitMap, i) == 0){//0 libre,  1 ocupado (no libre)
			cantLibres++;
		}
	}
	return cantLibres;
}

//No hay espacio contiguo y hay espacio separado
bool hayFragmentacion(int tamanio){

	if(!paginaDisponible(tamanio) && (espaciosLibres(tamanio) >= tamanio)){
		return 0;
	} else {
		return 1;
	}
}
/*
void encolarProgramas(int mensaje){ // ACA HAGO LOS RECV PERO NO SE SI ESTAN OK

	int pid, tamanio;

	recv(socket_umc , &pid, sizeof(int), 0);
	recv(socket_umc , &tamanio, sizeof(int), 0);


	agregarNodoEnEspera(pid, tamanio, mensaje); // guardar Mensaje
}
*/

//OPCION 2: ENCOLO LAS PETICIONES EN LA LISTA DE ESPERA (GUARDO TODOS LOS RECVS)
void encolarProgramas(int mensaje){ // ACA HAGO LOS RECV PERO NO SE SI ESTAN OK

	int pid, tamanio,numeroPagina;
	char* contenido = malloc(TAMANIO_PAGINA);

	recv(socket_umc , &pid, sizeof(int), 0);

	switch(mensaje){

	case RESERVA_ESPACIO:

		recv(socket_umc , &tamanio, sizeof(int), 0);

		agregarNodoEnEspera(pid, tamanio, -1,'\0', mensaje);  //tamanio = -1 inicializo

		break;

	case LEER_PAGINA:

		recv(socket_umc , &numeroPagina, sizeof(int), 0);

		agregarNodoEnEspera(pid, -1, numeroPagina,'\0', mensaje); //tamanio = -1 y contenido  \0

		break;

	case MODIFICAR_PAGINA:

		recv(socket_umc , &numeroPagina, sizeof(int), 0);
		recv(socket_umc , &contenido, TAMANIO_PAGINA, 0);

		agregarNodoEnEspera(pid, -1, numeroPagina,contenido, mensaje); // tamanio = -1

		break;

	case TERMINAR_PROGRAMA:


		agregarNodoEnEspera(pid, 0, 0, NULL, mensaje); //numero de pagina y contenido  \0

		break;

	default:

		printf("Mensaje Erroneo");

	}
}


int hayProgramasEnEspera(){

	//return listaEnEspera->elements_count > 0;
	return list_is_empty(listaEnEspera);
}

//-------------------------- FUNCIONES PARA COMPACTACION --------------------------//

void modificarArchivoSwap(int posicionAnterior, int posicionActual){

	memcpy( archivoMapeado + posicionActual,archivoMapeado + posicionAnterior ,TAMANIO_PAGINA);
	memset(archivoMapeado + posicionAnterior, '\0', TAMANIO_PAGINA);// DEJO ESPACIO EN \0

}

nodo_proceso *obtenerPrograma(int posicionEnSwap){

	bool igualPosSwap(nodo_proceso *nodo){
		return nodo->posSwap == posicionEnSwap;
	}

	nodo_proceso *nodo = list_find(listaProcesos,(void*)igualPosSwap);
	return nodo;
}

void modificarLista(int posicionNueva, int posicionAnterior){

	nodo_proceso *nodoProceso = obtenerPrograma(posicionAnterior);
	nodoProceso->posSwap = posicionNueva;
}

void intercambioEnBitmap(int posicionVacia, int posicionOcupada){

	bitarray_set_bit(bitMap, posicionVacia);
	bitarray_set_bit(bitMap, posicionOcupada);
}

bool posicionVacia(int posicionBitMap){

	return (bitarray_test_bit(bitMap, posicionBitMap) == 0);
}

//COMPACTACION
void comenzarCompactacion(){

	int i, j;
	int posicionAIntercambiar = -1;
	int cantidad = 0;

	puts("Compactación en curso");
	usleep(RETARDO_COMPACTACION * 1000); //ver cantidad - 60 segundos
	estaCompactando = 1;

	for(i = 0; i <= CANTIDAD_PAGINAS; i++) {

		if(posicionVacia(i)){
			posicionAIntercambiar = i;
			//ver condicion para cerrar el segundo for
			for(j = i + 1; j <= CANTIDAD_PAGINAS && posicionAIntercambiar == i; j++){

				if(!posicionVacia(j)){
					//INTERCAMBIO PAGINAS EN BITMAP
					intercambioEnBitmap(posicionAIntercambiar, j);
					//ACTUALIZO PAGINAS EN ARCHIVO
					modificarArchivoSwap(j, posicionAIntercambiar);

					cantidad ++;
					int cantidadPaginas = cantPaginas(obtenerPrograma(j));

					if(cantidad == cantidadPaginas){
						//ACTUALIZO LISTA DE PROCESOS
						modificarLista(j, posicionAIntercambiar);
					}
					posicionAIntercambiar++;
				}
			}
		}
	}
}

void cancelarInicializacion(){

	printf("Se ha cancelado la incicializacion \n");
	log_error(logger, "Se ha cancelado la incicializacion");
	informarAUmc();

}

void informarAUmc(){
	int mensaje = SWAP_PROGRAMA_RECHAZADO;

	int enviado = send(socket_umc, &mensaje, sizeof(int), 0);

	if(enviado < 0){
		log_error(logger, "Falló envío de mensaje de rechazo de programa");
		exit(EXIT_FAILURE);
	}else{
		printf("Se informa a la Umc rechazo de programa \n");
	}
}


//------------------- PROCESOS EN ESPERA -----------------------------//

// ver orden me parece que falta
void atenderProcesosEnEspera(){

	int i;
	int cantidadNodos = listaEnEspera->elements_count;

	for (i = 0; i < cantidadNodos; i++){

		nodo_enEspera *nodo = list_get(listaEnEspera,i);

		atenderPeticionesEnspera(nodo);

		//atenderPeticiones(nodo_EnEspera, tipoProceso);
	}
}


//-----------------------------Eliminar Proceso -----------------------//

void borrarDeArchivoSwap(nodo_proceso *nodo){

	int posicion = posicionSwap(nodo);
	int totalABorrar = TAMANIO_PAGINA*cantPaginas(nodo);

	memset(archivoMapeado + posicion, '\0', totalABorrar);
}

void borrarDeListaDeProcesos(nodo_proceso *nodo){

	bool igualPid(nodo_proceso *nodoProceso){  // ver como funciona ésto que lo hice con dos nodos
		return nodoProceso->pid == numeroPid(nodo);
	}
	list_remove_by_condition(listaProcesos, (void*)igualPid);
}

void liberarPosicion(nodo_proceso *nodo){

	int i;

	for(i = posicionSwap(nodo); i <= cantPaginas(nodo); i++){
		bitarray_set_bit(bitMap, i);
	}
}


void liberarEstructuras(nodo_proceso *nodo){

	borrarDeArchivoSwap(nodo);
	borrarDeListaDeProcesos(nodo);
	liberarPosicion(nodo);

}

void terminarProceso( int pid){

	int posicion = ubicacionEnSwap(pid);

	if(posicion != -1){
		nodo_proceso *nodo = obtenerPrograma(posicion);
		liberarEstructuras(nodo);
	}else{
		puts("No se ha encontrado el programa");
	}
}

//---------------------------- ELIMINAR ESTRUCTURAS ------------------------------//

void eliminarBitmap(){

	free(bitarray); // todo: ver si el destroy no hace internamente el free
	bitarray_destroy(bitMap);

}

void eliminarArchivoMapeado(){
	munmap(archivoMapeado, pagesize);
}

void cerrarArchivo(){
	fclose(file);
}


void eliminarEstructuras(){

	eliminarArchivoMapeado();
	eliminarBitmap();
	cerrarArchivo();
}

