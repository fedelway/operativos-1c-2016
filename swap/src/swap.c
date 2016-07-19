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

int main(int argc, char *argv[]){

	//logger = log_create("swap.log", "SWAP", true, LOG_LEVEL_INFO);
	//log_info(logger, "Proyecto para SWAP..");

	listaProcesos = list_create();
	listaEnEspera = list_create();

	crearConfiguracion(argv[1]);
	//reservo memoria para BitMap
	bitarray = (char*) malloc(CANTIDAD_PAGINAS * sizeof(int));

	//crearParticionSwap();
	crearBitMap();
	archivoMapeado = cargarArchivo();
	crearBitmap();

	PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	//log_info(logger, "Mi puerto escucha es: %s", PUERTO_ESCUCHA);
	socket_escucha = conectarPuertoDeEscucha(PUERTO_ESCUCHA);

	printf("Aun no Se ha conectado una umc.\n");
	socket_umc = aceptarConexion(socket_escucha);
	printf("Socket umc %d\n", socket_umc);

	handshakeUMC();

	trabajarUmc();

	//log_destroy(logger);

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
		//log_info(logger, "El archivo de configuración tiene todos los parametros requeridos.");
		return;
	}else{
		//log_error(logger, "Configuración no válida");
		// TODO: VER SI ESTA OK DESTROY ACÁ
		//log_destroy(logger);
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

	//log_info(logger, "Iniciando Handshake con la UMC");

	send(socket_umc, &soy_swap, sizeof(int), 0);
	recv(socket_umc, &msj_recibido, sizeof(int), 0);

	if(msj_recibido == SOY_UMC){
		//log_info(logger, "UMC validado.");
		printf("Se ha validado correctamente la UMC. \n");
	}else{
		//log_error(logger, "La UMC no pudo ser validada.");
		//log_destroy(logger);
		printf("La UMC no pudo ser validada.\n");
		exit(0);
	}
}

//---------------------------- TRABAJAR CON LA UMC ----------------------//

// HAGO DEFINE PARA SABER CUANDO ES UN PROCESO EN ESPERA
bool esUnProcesoEnEspera(int tipoProceso){

	return tipoProceso == PROCESO_EN_ESPERA;
}

void atenderPeticiones(int msj_recibido){

	int  pid, tamanio, numeroPagina;
	char *contenido = malloc(TAMANIO_PAGINA);

	if(contenido == NULL)
	{
		printf("Error de malloc.\n");
	}

	switch(msj_recibido){

	case RESERVA_ESPACIO :


		recv(socket_umc , &pid, sizeof(int), 0);
		recv(socket_umc , &tamanio, sizeof(int), 0);

		crearProgramaAnSISOP(pid,tamanio);

		break;

	case SOLICITUD_PAGINA:

		recv(socket_umc , &pid, sizeof(int), 0);
		recv(socket_umc , &numeroPagina, sizeof(int), 0);

		leerUnaPagina(pid, numeroPagina);

		break;

	case GUARDA_PAGINA:

		recv(socket_umc , &pid, sizeof(int), 0);
		recv(socket_umc , &numeroPagina, sizeof(int), 0);
		if( recvAll(socket_umc , contenido, TAMANIO_PAGINA, MSG_WAITALL) <= 0)
		{
			perror("Error al recibir el contenido de la pagina");
		}

		modificarPagina(pid, numeroPagina, contenido);

		break;

	case UMC_FINALIZAR_PROGRAMA:

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

	case SOLICITUD_PAGINA:

		leerUnaPagina(pid, numeroPagina);

		break;

	case GUARDA_PAGINA:


		modificarPagina(pid, numeroPagina, contenido);

		break;

	case UMC_FINALIZAR_PROGRAMA:

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
		if (recv(socket_umc , &msj_recibido, sizeof(int), 0) <= 0)
		{
			printf("Desconexion de la umc. Terminando...\n");
			perror("");
			exit(1);
		}

		if(estaCompactando){ //ver si hay que poner = 1
			encolarProgramas(msj_recibido);
		}else{

			/*if(hayProgramasEnEspera == 1){  //ver como compararlo

				pthread_mutex_lock(&peticionesActuales);
				atenderProcesosEnEspera();
				pthread_mutex_unlock(&peticionesActuales);

			}else{*/

				atenderPeticiones(msj_recibido);

		}
	}
}

//------------------------------ ALMACENAMIENTO EN SWAP ---------------------------//

void crearParticionSwap(){

	char comando[50];
	printf("Creando archivo Swap: \n");
	sprintf(comando, "dd if=/dev/zero bs=%d count=1 of=%s",	CANTIDAD_PAGINAS * TAMANIO_PAGINA, NOMBRE_SWAP);
	printf("dd if=/dev/zero bs=%d count=1 of=%s \n",	CANTIDAD_PAGINAS * TAMANIO_PAGINA, NOMBRE_SWAP);
	if( system(comando) == -1)
	{
		printf("Error al crear archivoSwap.\n");
		exit(-1);
	}
}

void crearParticionSwap2(char *path)
{
	char comando[250];
	printf("Creando archivo Swap: \n");
	sprintf(comando, "dd if=%s bs=%d count=1 of=%s", path, CANTIDAD_PAGINAS * TAMANIO_PAGINA, path);
	printf("dd if=%s bs=%d count=1 of=%s \n", path, CANTIDAD_PAGINAS * TAMANIO_PAGINA, path);
	if( system(comando) == -1)
	{
		printf("Error al crear archivoSwap.\n");
		exit(-1);
	}
}

char* cargarArchivo(){

	/*//Obtengo el directorio para ubicar el archivo de swap en la carpeta resource.
	const int size = 100 * sizeof(char);//100 caracteres alcanzan
	char *directorio = malloc(size);

	//getcwd(directorio,size);

	//string_append(&directorio, "/../resource/");
	//string_append(&directorio,"/");
	string_append(&directorio, NOMBRE_SWAP);*/

	char *directorio = string_from_format("../resource/");
	string_append(&directorio,NOMBRE_SWAP);

	printf("directorio: %s.\n",directorio);
	//crearParticionSwap2(directorio);

	directorio = "/home/utnso/proyecto/tp-2016-1c-Socketes/swap/resource/SWAP.DATA";

	FILE *file = fopen(directorio, "w+");
	if(file == NULL)
	{
		perror("Error al abrir el archivo de swap.\n");
		exit(-1);
	}
	//Necesito el fd
	data_fd =fileno(file);

	ftruncate(data_fd, CANTIDAD_PAGINAS * TAMANIO_PAGINA);

	size_t pagesize = (size_t) sysconf(_SC_PAGESIZE);
	printf("pagesize: %d.\n",pagesize);
	pagesize = CANTIDAD_PAGINAS * TAMANIO_PAGINA;//Para mappear el archivo ENTERO
	char* accesoAMemoria = (char*)mmap( NULL, pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, data_fd, 0);

	if(accesoAMemoria == MAP_FAILED){
		perror("mmap");
		exit(1);
	}
	return accesoAMemoria;
}


//------------------------------ FUNCIONES SOBRE EL BITMAP ----------------------------//

void crearBitmap()
{
	bitmap = malloc(sizeof(char) * CANTIDAD_PAGINAS);

	//Inicializo el bitmap con todas las paginas libres
	int i;
	for(i=0; i<CANTIDAD_PAGINAS; i++)
	{
		bitmap[i] = 'l';
	}
}

//Creo BitMap usando bitarray
void crearBitMap(){

	bitMap = bitarray_create(bitarray, CANTIDAD_PAGINAS);

}

//Paso cada página del BITMAP a ocupada
void actualizarBitMap(int pagina, int cant_paginas){

	int i;
	for(i=0; i<cant_paginas; i++)
	{
		bitmap[pagina+i] = 'o';
	}

	/*int i;
	int cant = 0;
	for (i = 1; i <= cant_paginas; i++){  // ver =0 o =1

		bitarray_set_bit(bitMap, pagina + cant);
		cant = cant + 1;
	}*/
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

void agregarNodoProceso(int pid, int cantidadPaginas, int pagina){

	nodo_proceso *proceso = malloc(sizeof(nodo_proceso));
	proceso->pid = pid;
	proceso->cantidad_paginas = cantidadPaginas;
	proceso->posSwap = pagina * TAMANIO_PAGINA;

	list_add(listaProcesos, proceso);
	//list_add(listaProcesos, crearNodoDeProceso(pid, cantidadPaginas , posicionSwap));

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
bool hayEspacioContiguo(int pag, int cant_paginas){

	int i;
	for(i=0;i<cant_paginas;i++)
	{
		if(bitmap[pag+i] == 'o')
		{
			return false;
		}
	}
	//No retorne, por lo tanto, hay espacio continuo
	return true;

/*
	int i;
	//la idea es verificar el cacho de registros del vector que pueden alocar ese tamanio

	printf("pagina: %d, cant paginas: %d.\n",pagina,tamanio);

	if( (pagina + tamanio) >= CANTIDAD_PAGINAS)
	{
		printf("Pide mas paginas de las disponibles.\n");
	}

	int booleano = 1;
	for (i = pagina; i <= pagina + tamanio; i++){
		//if (bitarray_test_bit(bitMap, i) == 0 && tamanio + i  <= CANTIDAD_PAGINAS){ //== 0 libre == 1 ocupado
		//	return 1;
		//}

		if(bitarray_test_bit(bitMap,i) != 0)
		{
			booleano = 0;
		}
	}
	return booleano;
	//return 0; // retorna F, No hay espacio disponible
*/
}

int paginaDisponible(int cant_paginas){
	printf("pasó por pagina disponible \n");
/*	int i;
	//Recorro todo el bitMap buscando espacios contiguos
	//devuelvo la pagina desde donde tiene que reservar memoria, si no encuentra lugar devuelve -1
	for(i = 0; i <= CANTIDAD_PAGINAS; i++){
		if(bitarray_test_bit(bitMap, i) == 0){	// 1 VER, 0 FALSO
			if(hayEspacioContiguo(i, cant_paginas)){
				return i;

			}
		}
	}
	return -1;*/

	int i;
	for(i=0; i<CANTIDAD_PAGINAS; i++)
	{
		if(bitmap[i] == 'l')
		{//Si la pagina esta libre miro que las siguientes tambien lo esten
			if(hayEspacioContiguo(i,cant_paginas))
			{
				return i;
			}
		}
	}
	//Sali del ciclo, no hay pagina
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
		actualizarBitMap(pagina, cant_paginas);
		printf("Se ha reservado espacio para el programa n°: %d.\n", pid);
		printf("Pagina reservada: %d. Cant Paginas: %d.\n", pagina,cant_paginas);

		//Aviso a umc que se pudo reservar espacio
		int mensaje = SWAP_PROGRAMA_OK;
		send(socket_umc,&mensaje,sizeof(int),0);

	} else {

		if(hayFragmentacion(cant_paginas)){

			comenzarCompactacion();

			//Ya compacte, llamo de vuelta a crearPrograma para ver si ahora si hay espacio
			crearProgramaAnSISOP(pid, cant_paginas);

		}else{

			cancelarInicializacion();
		}
	}
}

void leerUnaPagina(int pid, int pagina){

	int posSwap = ubicacionEnSwap(pid);

	if(posSwap == -1)
	{
		printf("Error ubicacionSwap.\n");
	}

	char *pos_a_leer = archivoMapeado + posSwap + pagina * TAMANIO_PAGINA;

	printf("El contenido de la página es: ");
	fwrite(pos_a_leer,sizeof(char),TAMANIO_PAGINA,stdout);
	printf("\n");

	//Enviar contenido de pagina leida
	if( sendAll(socket_umc, pos_a_leer, TAMANIO_PAGINA, 0) <= 0 )
	{
		perror("Error al enviar el contenido de la pagina.\n");
	}

}

void modificarPagina(int pid, int pagina, char *contenido){

	printf("ModificarPagina(pid:%d, numeroPagina:%d)\n", pid, pagina);

	int posSwap = ubicacionEnSwap(pid);
	printf("posSwap: %d.\n\n", posSwap);

	int posEscribir = pagina * TAMANIO_PAGINA;

	fwrite(contenido,sizeof(char),TAMANIO_PAGINA,stdout);
	printf("\n\n");

	if(posSwap != -1){
		memcpy(archivoMapeado + posSwap + posEscribir, contenido, TAMANIO_PAGINA);

		if( msync(archivoMapeado, TAMANIO_PAGINA * CANTIDAD_PAGINAS, MS_SYNC) == -1)
		{
			perror("Error en el sync");
		}

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

	case SOLICITUD_PAGINA:

		recv(socket_umc , &numeroPagina, sizeof(int), 0);

		agregarNodoEnEspera(pid, -1, numeroPagina,'\0', mensaje); //tamanio = -1 y contenido  \0

		break;

	case GUARDA_PAGINA:

		recv(socket_umc , &numeroPagina, sizeof(int), 0);
		recv(socket_umc , &contenido, TAMANIO_PAGINA, 0);

		agregarNodoEnEspera(pid, -1, numeroPagina,contenido, mensaje); // tamanio = -1

		break;

	case UMC_FINALIZAR_PROGRAMA:


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

void compactar()
{

}

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

	puts("Compactación en curso.\n");
	usleep(RETARDO_COMPACTACION); //ver cantidad - 60 segundos
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
	//log_error(logger, "Se ha cancelado la incicializacion");
	informarAUmc();

}

void informarAUmc(){
	int mensaje = SWAP_PROGRAMA_RECHAZADO;

	int enviado = send(socket_umc, &mensaje, sizeof(int), 0);

	if(enviado < 0){
		//log_error(logger, "Falló envío de mensaje de rechazo de programa");
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

