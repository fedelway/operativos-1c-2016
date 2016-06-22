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

t_config* config;
t_log* logger;
char *archivoMapeado;

#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar


int main(int argc, char *argv[]){

	char message[PACKAGESIZE];
	memset(message, '\0', PACKAGESIZE);

	logger = log_create("swap.log", "SWAP", true, LOG_LEVEL_INFO);
	log_info(logger, "Proyecto para SWAP..");

	//Inicializo semáforos
	pthread_mutex_init(&mutexListaEnEspera,NULL);


	listaProcesos = list_create();
	listaEnEspera = list_create();

	crearConfiguracion(argv[1]);
	//reservo memoria para BitMap
	bitarray = (char*) malloc(CANTIDAD_PAGINAS * sizeof(int));

	crearParticionSwap();
	crearBitMap();
	archivoMapeado = cargarArchivo();

	//prueba para crear 1° programa
	int disponible1 = paginaDisponible(5);
	printf("la pagina disponible para el primer programa es: %d \n",
			disponible1);

	char* resultadoCreacion1; // lo hago así pero dsp cambia, porque unificocon compactacion y frag exter
	char* codigo_prog = "HOLA";

	crearProgramaAnSISOP(1, 5,	resultadoCreacion1, codigo_prog); //para probar que devuelve
	//printf("El resultado de la creación del programa %d es: %s \n", 1,
			//resultadoDeCreacionPrograma1); // probado cuando no hay espacio tambien y funciona ok

	int ubicacion = ubicacionEnSwap(1);
	printf("La ubicacion dl programa 1 en swap es: %d \n", ubicacion);

	//prueba para crear 2° programa
	int disponible2 = paginaDisponible(4);
	printf("la pagina disponible para el segundo programa es: %d \n",disponible2);

	char* resultadoCreacion2; // lo hago así pero dsp cambia, porque unificocon compactacion y frag exter
	char* codigo_prog2 = "QUE";

	crearProgramaAnSISOP(2, 3,resultadoCreacion2, codigo_prog2); //para probar que devuelve
	//printf("El resultado de la creación del programa %d es: %s \n", 2,
		//	 		resultadoDeCreacionPrograma2); // probado cuando no hay espacio tambien y funciona ok

	int ubicacion2 = ubicacionEnSwap(2);
	printf("La ubicacion en swap del segundo programa es: %d \n", ubicacion2);



	/*
	 //prueba para crear 3° programa
	 int disponible3 = paginaDisponible(2);
	 printf("la pagina disponible para el tercer programa es: %d \n", disponible3);

	 char* resultadoCreacion3; // lo hago así pero dsp cambia, porque unifico con compactacion y frag exter
	 char* codigo_prog3 = "ES";
	 char* resultadoDeCreacionPrograma3 = crearProgramaAnSISOP(3,2,resultadoCreacion3,codigo_prog3,archivoMapeado); //para probar que devuelve
	 printf("El resultado de la creación del programa %d es: %s \n",3, resultadoDeCreacionPrograma3); // probado cuando no hay espacio tambien y funciona ok
	 */

	leerUnaPagina(1,1);
	//modificarPagina(2,1, "XXXXXXXXXX");
	//leerUnaPagina(1, 1);

	PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	log_info(logger, "Mi puerto escucha es: %s", PUERTO_ESCUCHA);
	socket_escucha = conectarPuertoDeEscucha(PUERTO_ESCUCHA);
	socket_umc = aceptarConexion(socket_escucha);
	handshakeUMC();

	recibirMensajeUMC(message, socket_umc);

	log_destroy(logger);

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
		log_info(logger,
				"El archivo de configuración tiene todos los parametros requeridos.");
		return;
	}else{
		log_error(logger, "Configuración no válida");
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

//Acá implementamos el handshake del lado del servidor
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

void recibirMensajeUMC(char* message, int socket_umc){

	printf("voy a recibir mensaje de %d\n", socket_umc);
	int status = 0;
	//while(status == 0){
	status = recv(socket_umc, message, PACKAGESIZE, 0);
	if(status != 0){
		printf("recibo mensaje de UMC %d\n", status);
		printf("recibi este mensaje: %s\n", message);
		status = 0;
	}else{
		sleep(1);
		printf(".\n");
	}
}

//------------------------------ ALMACENAMIENTO EN SWAP ---------------------------//

void crearParticionSwap(){

	char comando[50];
	printf("Creando archivo Swap: \n");
	sprintf(comando, "dd if=/dev/zero bs=%d count=1 of=%s",
			CANTIDAD_PAGINAS * TAMANIO_PAGINA, NOMBRE_SWAP);
	system(comando);
}

char* cargarArchivo(){

	FILE *file = fopen(NOMBRE_SWAP, "r+");// falta verificar
	int fd =fileno(file);  // Esto traduce a entero el fd


	int pagesize = TAMANIO_PAGINA * CANTIDAD_PAGINAS; // 512 CANT PAGINAS
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
	printf("Paginas ocupadas:  %d \n", cant);
}

//--------------------------- CREACION DE NODOS DE LAS LISTAS -------------------------//

static nodo_proceso *crearNodoDeProceso(int pid, int cantidad_paginas,int posSwap){
	nodo_proceso *proceso = malloc(sizeof(nodo_proceso));
	proceso->pid = pid;
	proceso->cantidad_paginas = cantidad_paginas;
	proceso->posSwap = posSwap;
	return proceso;
}

static nodo_enEspera *crearNodoEnEspera(int pid, int cantidad_paginas){
	nodo_enEspera *enEspera = malloc(sizeof(nodo_enEspera));
	enEspera->pid = pid;
	enEspera->cantidad_paginas = cantidad_paginas;
	return enEspera;
}

void agregarNodoEnEspera(int pid, int cantidadPaginas){

	list_add(listaEnEspera, crearNodoEnEspera(pid, cantidadPaginas));

}

void adregarNodoProceso(int pid, int cantidadPaginas, int posicionSwap){

	list_add(listaProcesos, crearNodoDeProceso(pid, cantidadPaginas , posicionSwap));

}
int cantPaginas(nodo_proceso *nodo){
	return nodo->cantidad_paginas;
}

int pid(nodo_proceso *nodo){
	return nodo->pid;
}

int posicionSwap(nodo_proceso *nodo){
	return nodo->posSwap;
}
//VERIFICA A PARTIR DE UNA PÁGINA DISPONIBLE, HAY ESPACIO CONTÍGUO
bool hayEspacioContiguo(int pagina, int tamanio){
	int i;
	//la idea es verificar el cacho de registros del vector que pueden alocar ese tamanio

	for (i = pagina; i <= pagina + tamanio; i++){
		if (bitarray_test_bit(bitMap, i) == 0
				&& i + tamanio <= CANTIDAD_PAGINAS){ //== 0 libre == 1 ocupado
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

	/*
	int i;
	nodo_proceso *nodo;
	int cantidadNodos = listaProcesos->elements_count;
	for (i = 0; i < cantidadNodos; i++) {
		nodo = list_get(listaProcesos, i);
		if(nodo->pid == pid){
			return nodo->posSwap;
		}
	}
	return -1;
	*/

//Copia el código en Swap, Agrega nodo a la lista de procesos, actualiza el BITMAP y retorna Resultado
void crearProgramaAnSISOP(int pid, int cant_paginas, char* resultadoCreacion,char* codigo_prog){

	int pagina = paginaDisponible(cant_paginas);
	if(pagina != -1){
		//copia codigo_prog a archivoMapeado a partir de pagina*tamanio_pagina
		memcpy(archivoMapeado + pagina * TAMANIO_PAGINA, codigo_prog,cant_paginas*TAMANIO_PAGINA);
		adregarNodoProceso(pid, cant_paginas, pagina);

		actualizarBitMap(pid, pagina, cant_paginas);
		//resultadoCreacion = "Se ha creado correctamente el programa \n";
		printf("Codigo copiado al archivo Swap:  %s \n",archivoMapeado + pagina * TAMANIO_PAGINA);

	} else {
		//En este caso no tenemos paginas disponibles para crear el programa
		resultadoCreacion = "Inicializacion Cancelada";
	}
	//return resultadoCreacion;
}

void leerUnaPagina(int pid, int pagina){

	int posSwap = ubicacionEnSwap(pid);

	if(posSwap != -1){

		char* bytes = malloc(TAMANIO_PAGINA);
		//memcpy(bytes, archivoMapeado + posSwap * tamanio_pagina,tamanio_pagina);
		memcpy(bytes,archivoMapeado +posSwap + pagina*TAMANIO_PAGINA, TAMANIO_PAGINA);
		printf("El contenido de la página es: %s\n ", bytes);
	} else {

		puts("No se encontró el contenido de la página solicitada");
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

//COMPACTACION AL INGRESAR UN PROGRAMA
void prepararLugar(int pid, int tamanio){

	if(hayFragmentacion(tamanio)){
		//SEMÁFORO MIENTRAS COMPACTO
		pthread_mutex_lock(&mutexListaEnEspera);
		agregarNodoEnEspera(pid, tamanio);
		comenzarCompactacion();
		pthread_mutex_unlock(&mutexListaEnEspera);
	}
}

//-------------------------- FUNCIONES PARA COMPACTACION --------------------------//

void modificarArchivoSwap(int posicionAnterior, int posicionActual){

	 memcpy( archivoMapeado + posicionActual,archivoMapeado + posicionAnterior ,TAMANIO_PAGINA);
	 memset(archivoMapeado+ posicionAnterior, '\0', TAMANIO_PAGINA);// DEJO ESPACIO EN \0

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


//------------------- PROCESOS EN ESPERA -----------------------------//
//terminar
/*void procesarProcesosEnEspera(){

	int i;
	int cantidadNodos = listaEnEspera->elements_count;

	for (i = 0; i < cantidadNodos; i++) {
		nodo_proceso *nodo = list_get(listaProcesos, i);
	}
}
*/

//-----------------------------Eliminar Proceso -----------------------//

void borrarDeArchivoSwap(nodo_proceso *nodo){

	int posicion = posicionSwap(nodo);
	int totalABorrar = TAMANIO_PAGINA*cantPaginas(nodo);

	memset(archivoMapeado + posicion, '\0', totalABorrar);
}

void borrarDeListaDeProcesos(nodo_proceso *nodo){

	bool igualPid(nodo_proceso *nodoProceso){  // ver como funciona ésto que lo hice con dos nodos
			return nodoProceso->pid == pid(nodo);
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

void terminarProceso( nodo_proceso *nodo){

	int posicion = ubicacionEnSwap(pid(nodo));

	if(posicion != -1){
		liberarEstructuras(nodo);
	}else{
		puts("No se ha encontrado el programa");
	}
}

//---------------------------- TERMINAR ------------------------------//
void cerrarArchivo(){

	fclose(file);
}
