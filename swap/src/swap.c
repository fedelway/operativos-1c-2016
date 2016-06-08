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
#include <string.h>


t_config* config;
t_log* logger;
char *archivoMapeado;

#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar
int conectarPuertoDeEscucha2(char* puerto);
void crearParticionSwap();


int main(int argc,char *argv[]) {

	char message[PACKAGESIZE];
	memset (message,'\0',PACKAGESIZE);

	logger = log_create("swap.log", "SWAP",true, LOG_LEVEL_INFO);
	log_info(logger, "Proyecto para SWAP..");


	listaProcesos = list_create();
	listaEnEspera = list_create();

	crearConfiguracion(argv[1]);
	crearParticionSwap();
	crearBitMap();
	archivoMapeado = cargarArchivo();
	inicializarArchivo(archivoMapeado);
/*
	//prueba para crear 1° programa
	int disponible1 = paginaDisponible(1, 14);
	printf("la pagina disponible para el primer programa es: %d \n", disponible1);

	char* resultadoCreacion1; // lo hago así pero dsp cambia, porque unificocon compactacion y frag exter
	char* codigo_prog = "variables a,b";
	char* resultadoDeCreacionPrograma1 = crearProgramaAnSISOP(1,14,resultadoCreacion1,codigo_prog,archivoMapeado); //para probar que devuelve
	printf("El resultado de la creación del programa %d es: %s \n",1, resultadoDeCreacionPrograma1); // probado cuando no hay espacio tambien y funciona ok



	 //prueba para crear 2° programa
	int disponible2 = paginaDisponible(2,11);
	printf("la pagina disponible para el primer programa es: %d \n", disponible2);

	char* resultadoCreacion2; // lo hago así pero dsp cambia, porque unificocon compactacion y frag exter
	char* codigo_prog2 = "int c,d,f;";
	char* resultadoDeCreacionPrograma2 = crearProgramaAnSISOP(2,11,resultadoCreacion2,codigo_prog2,archivoMapeado); //para probar que devuelve
	printf("El resultado de la creación del programa %d es: %s \n",2, resultadoDeCreacionPrograma2); // probado cuando no hay espacio tambien y funciona ok

	 //prueba para crear 3° programa
	int disponible3 = paginaDisponible(3, 8);
	printf("la pagina disponible para el primer programa es: %d \n", disponible3);

	char* resultadoCreacion3; // lo hago así pero dsp cambia, porque unificocon compactacion y frag exter
	char* codigo_prog3 = "char r;";
	char* resultadoDeCreacionPrograma3 = crearProgramaAnSISOP(3,7,resultadoCreacion3,codigo_prog3,archivoMapeado); //para probar que devuelve
	printf("El resultado de la creación del programa %d es: %s \n",3, resultadoDeCreacionPrograma3); // probado cuando no hay espacio tambien y funciona ok

*/
	//modificarPagina(1, "a + c", archivoMapeado);

	//leerUnaPagina(1,1,archivoMapeado); AUN NO FUNCIONA

	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	log_info(logger, "Mi puerto escucha es: %s", puerto_escucha);
	conectarPuertoDeEscucha2(puerto_escucha);

	//conexion a umc
	char* umc_puerto;
	umc_puerto = config_get_string_value(config,"PUERTO");
	printf("Mi puerto escucha es: %s", umc_puerto);
	int socket_umc = conectarPuertoDeEscucha(umc_puerto);
	recibirMensajeUMC(message,socket_umc);


	log_destroy(logger);

	return EXIT_SUCCESS;

}


void recibirMensajeUMC(char* message, int socket_umc){

	printf("voy a recibir mensaje de %d\n", socket_umc);
	int status = 0;
	//while(status == 0){
	status = recv(socket_umc, message, PACKAGESIZE, 0);
	if (status != 0){
		printf("recibo mensaje de UMC %d\n", status);
		printf("recibi este mensaje: %s\n", message);
		status = 0;
	}else{
		sleep(1);
		printf(".\n");
	}

	//}
}

void crearConfiguracion(char *config_path){

	config = config_create(config_path);
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	nombre_swap = config_get_string_value(config, "NOMBRE_SWAP");
	cantidad_paginas = config_get_int_value(config, "CANTIDAD_PAGINAS");
	tamanio_pagina = config_get_int_value(config, "TAMANIO_PAGINA");
	retardo_compactacion = config_get_int_value(config, "RETARDO_COMPACTACION");



	if (validarParametrosDeConfiguracion()){
		log_info(logger, "El archivo de configuración tiene todos los parametros requeridos.");
		return;
	}else{
		log_error(logger, "Configuración no válida");
		log_destroy(logger);
		exit(EXIT_FAILURE);
	}
}

//Valida que todos los parámetros existan en el archivo de configuración
bool validarParametrosDeConfiguracion(){

	return (	config_has_property(config, "PUERTO_ESCUCHA")
			&& 	config_has_property(config, "NOMBRE_SWAP")
			&& 	config_has_property(config, "CANTIDAD_PAGINAS")
			&& 	config_has_property(config, "TAMANIO_PAGINA")
			&& 	config_has_property(config, "RETARDO_COMPACTACION"));
}


int conectarPuertoDeEscucha2(char* puerto){

	struct addrinfo hints, *serverInfo;
	int result_getaddrinfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; 		// AF_INET or AF_INET6 to force version
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(NULL, puerto, &hints, &serverInfo);

	if ((result_getaddrinfo = getaddrinfo(NULL, puerto, &hints, &serverInfo)) != 0) {
		fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(result_getaddrinfo));
		return 2;
	}

	//Obtengo un socket para que escuche las conexiones entrantes
	int listeningSocket;
	listeningSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	printf("Asocio al listennigSocket al puerto: %s.\n", puerto);

	//Asocio al socket con el puerto para escuchar las conexiones en dicho puerto
	bind(listeningSocket,serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo); // Libero serverInfo

	printf("Listening....\n");

	//Le digo al socket que se quede escuchando
	//TODO: Ver de donde sacamos el Backlog
	listen(listeningSocket, BACKLOG);		// IMPORTANTE: listen() es una syscall BLOQUEANTE.

	//Si hay una conexion entrante, la acepta y crea un nuevo socket mediante el cual nos podamos comunicar con el cliente
	//El listennigSocket lo seguimos usando para escuchar las conexiones entrantes.
	//Nota TODO: Para que el listenningSocket vuelva a esperar conexiones, necesitariamos volver a decirle que escuche, con listen();

	struct sockaddr_in addr; // addr contien los datos de la conexion del cliente.
	socklen_t addrlen = sizeof(addr);

	int socketCliente = accept(listeningSocket, (struct sockaddr *) &addr, &addrlen);

	//Ya estamos listos para recibir paquetes de nuestro cliente...
	//Vamos a ESPERAR (ergo, funcion bloqueante) que nos manden los paquetes, y los imprimiremos por pantalla.

	char package[PACKAGESIZE];
	int status = 1;		// Estructura que maneja el status de los receive.
	int enviar=1;

	printf("Cliente conectado. Esperando mensajes:\n");

	//Cuando el cliente cierra la conexion, recv() devolvera 0.
	while (status != 0 && enviar !=0){
		memset (package,'\0',PACKAGESIZE); //Lleno de '\0' el package, para que no me muestre basura
		status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
		if (status != 0) printf("%s", package);

		//Envío un mensaje al cliente que se conectó
		fgets(package, PACKAGESIZE, stdin);
		if (!strcmp(package,"exit\n")) enviar= 0;	// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
		if (enviar) send(socketCliente, package, strlen(package) + 1, 0); 	// Solo envio si el usuario no quiere salir.

	}

	printf("Cliente se ha desconectado. Finalizo todas las conexiones.\n");

	//Al terminar el intercambio de paquetes, cerramos todas las conexiones.
	close(socketCliente);
	close(listeningSocket);

	return 0;
}
///////// ver funcion handshake, parámetros addr y listeningSocket, ahora tiene una resolucion errónea//////////////////

//Acá implementamos el handshake del lado del servidor
void handshakeServidor(int socket_umc){
	/*
		//Estructura para crear el header + piload
		typedef struct{
			int id;
			int tamanio;
		}t_header;

		typedef struct{
		  t_header header;
		  char* paiload;
		}package;

	   socket_umc= accept(listeningSocket, (struct sockaddr *) &addr, &addrlen);

		//seteo el mensaje que le envío a la umc para que ésta reciba el header.id=2 y haga desde su proceso el handshake
		package mensajeAEnviar;
		mensajeAEnviar.header.id = 5020 ;
		mensajeAEnviar.header.tamanio = 0;
		mensajeAEnviar.paiload = "\0";

		package mensajeARecibir;
		mensajeARecibir.header.id = 4020;
		mensajeARecibir.header.tamanio = 0;
		mensajeARecibir.paiload = "\0";
        //Falta revisar si el segundo parámetro coincide con el tipo de la funcion send()
		send(socket_umc, &mensajeAEnviar, sizeof(mensajeAEnviar), 0);
		recv(socket_umc, &mensajeARecibir, sizeof(mensajeARecibir), 0);

        // Acá hay que ver si cambiamos el nombre del mensaje por package, tengo la duda de si toma el mensaje seteado del send() anterior
		switch(mensajeARecibir.header.id){

		 case 1000:
			 // Procesa ok
			 printf("OK\n");
		 break;

		 case 4020:
			 //Respuesta de Handshake servidor
			 	printf("Conectado a UMC.\n");
			 	memset(mensajeARecibir.paiload,'\0', mensajeARecibir.header.tamanio); //Lleno de '\0' el package, para que no me muestre basura
			    recv(socketCliente, mensajeARecibir.paiload, mensajeARecibir.header.tamanio , 0);
	     break;

		 case 5020:
			 //Handshake cliente
			 printf("No hace nada porque está reservado para el cliente\n");
		 break;

		 default:
			 printf("No se admite la conexión con éste socket");
		 break;
		}
	 */
}

// CREAR ALMACENAMIENTO SWAP
void crearParticionSwap(){

	char comando[50];
	printf("Creando archivo Swap: \n");
	sprintf(comando, "dd if=/dev/zero bs=%d count=1 of=%s",cantidad_paginas*tamanio_pagina, nombre_swap);
	system(comando);
}

//Utilizando mmap(), falta probar..
char* cargarArchivo(){
	int fd ;

	if ((fd = open ("SWAP.DATA", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1) {
		perror("open");
		exit(1);
	}

	int pagesize =tamanio_pagina*cantidad_paginas; // 512 CANT PAGINAS
	char* accesoAMemoria = mmap( NULL, pagesize, PROT_READ| PROT_WRITE, MAP_SHARED, fd, 0);

	if (accesoAMemoria == (caddr_t)(-1)) {
		perror("mmap");
		exit(1);
	}
	return accesoAMemoria;
}

//ACÁ INICIALIZO CON TODOS CARACTERES DEL ARCHIVO EN MEMORIA CON '\0'
void inicializarArchivo(char* accesoAMemoria){

	int tamanio = strlen(accesoAMemoria);

	memset(&(*accesoAMemoria),'\0', tamanio + 1);

}

//Creo BitMap usando bitarray
void crearBitMap(){

	bitMap = bitarray_create(bitarray, sizeof(bitarray));
}

//Paso cada página del BITMAP a ocupada
void actualizarBitMap(int pid, int pagina, int cant_paginas){

	int i;
	int  cant = 0;
	for(i =1; i<= cant_paginas; i++){

		bitarray_set_bit(bitMap,pagina);
		cant = cant +1;
	}
	printf("Paginas ocupadas:  %d \n",cant );
}

//CREO LOS NODOS DE LAS LISTAS
static nodo_proceso *crearNodoDeProceso(int pid, int cantidad_paginas, int posSwap){
	nodo_proceso *proceso=malloc(sizeof(nodo_proceso));
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

//VERIFICA A PARTIR DE UNA PÁGINA DISPONIBLE, HAY ESPACIO CONTÍGUO
bool hayEspacioContiguo(int pagina, int tamanio){
	int i;
	//la idea es verificar el cacho de registros del vector que pueden alocar ese tamanio
	for(i = pagina; i <= pagina+tamanio ;i++){
		if(bitarray_test_bit(bitMap, i) == 0){//== 0 libre == 1 ocupado
			return 1;
		}
	}
	return 0; // retorna F, No hay espacio disponible
}

int paginaDisponible(int pid,int tamanio){
	printf("pasó por pagina disponible \n");
	int i;
	//Recorro todo el bitMap buscando espacios contiguos
	//devuelvo la pagina desde donde tiene que reservar memoria, si no encuentra lugar devuelve -1

	for(i = 0; i <= tamanio_pagina*cantidad_paginas; i++ ){
		if(bitarray_test_bit(bitMap, i) == 0){// 1 VER, 0 FALSO
			if(hayEspacioContiguo(i,tamanio)){
				return i;

			}
		}
	}
	return -1;
}

//Recorrer la lista de procesos en swap y devolver (int) la ubicación // resuelto con list_find()!
int  ubicacionEnSwap(int pid){ //FUNCIONA

	bool ubicarPaginasPrograma(nodo_proceso nodoDeProceso){
		return(nodoDeProceso.pid == pid);
	}

	nodo_proceso *nodoDeProceso = list_find(listaProcesos, (void*) ubicarPaginasPrograma);
	if (nodoDeProceso != NULL){
		int posSwap = nodoDeProceso->posSwap;
		return posSwap;
	}else{
		return -1;
		printf("No se encontró el proceso\n");
	}
}

/* version anterior s/list_find
	int i;
	nodo_proceso *nodo;
	int cantidadNodos = listaProcesos->elements_count;
	for(i = 0; i < cantidadNodos; i++ ){
		nodo = list_get(listaProcesos, i);
		if(nodo->pid == pid){
			return nodo->posSwap;
		}
	}
	return -1;
}
*/

//Copia el código en Swap, Agrega nodo a la lista de procesos, actualiza el BITMAP y retorna Resultado
char* crearProgramaAnSISOP(int pid,int cant_paginas,char* resultadoCreacion, char* codigo_prog, char* archivoMapeado){

	int pagina = paginaDisponible(pid,cant_paginas);
	if(pagina != -1){
		//copia codigo_prog a archivoMapeado a partir de pagina*tamanio_pagina
		memcpy(archivoMapeado + pagina*tamanio_pagina, codigo_prog , cant_paginas);
		//memcpy(archivoMapeado + pagina*cant_paginas, &codigo_prog , cant_paginas);
		list_add(listaProcesos, crearNodoDeProceso(pid, cant_paginas, pagina));

		actualizarBitMap(pid, pagina, cant_paginas);
		resultadoCreacion = "Se ha creado correctamente el programa\n";
		printf("Codigo copiado al archivo Swap:  %s \n",archivoMapeado + pagina*tamanio_pagina);

	}else{
		//En este caso no tenemos paginas disponibles para crear el programa
		resultadoCreacion = "Inicializacion Cancelada";
	}
	return resultadoCreacion;
}

//todo:corregir porque no mapea lo revuelto
// FALTA archivoMapeado ponerlo COMO VAR GLOBAL( lo paso así para que funcione).
void leerUnaPagina(int pid, int pagina,char* archivoMapeado){
	printf("quiere leer pagina\n");


	if (ubicacionEnSwap(pid) == -1){
		char *bytes = malloc(sizeof(char));
		memcpy(&bytes, &archivoMapeado[ubicacionEnSwap(pid)], sizeof(char));
		printf("El contenido de la página");//: %d es %c. \n",pagina, *bytes);
	}else{

	printf("No se encontro el contenido");
	}
}

void modificarPagina(int pid, char* nuevoCodigo, char* archivoMapeado){

	printf("entra a modificar\n");
	int posSwap = ubicacionEnSwap(pid);

	archivoMapeado = malloc(posSwap + strlen(nuevoCodigo));

	if (ubicacionEnSwap(pid) != -1){
		memcpy(&archivoMapeado[posSwap], &nuevoCodigo, strlen(nuevoCodigo));
		printf("la modificacion en la pagina n° %d es: %c", posSwap, *archivoMapeado);
	}else{
		printf("PAGINA NO ENCONTRADA EN SWAP");
	}
}
/*
bool hayFragmentacion(char* archivoMapeado){
	int libre;
	if(paginaDisponible(int pid,int tamanio) == -1){
		libre == libre +1;
		printf("sumo los espacios libres \n");
		}
//compactación al ingresar un programa, falta hacerlo dsp sin el pid y tamanio
void verificarEspacio(char* archivoMapeado, char* bitMap, int pid, int tamanio){
	//No hay espacio contiguo y hay espacio separado
	if (paginaDisponible(pid, tamanio) == -1 && hayFragmentacionExterna(pid, tamanio)){
		comenzarCompactacion();
		}
	}else{

	}
}


*/
