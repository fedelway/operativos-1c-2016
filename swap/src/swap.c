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

#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar
int conectarPuertoDeEscucha2(char* puerto);
void crearParticionSwap();

int main(int argc,char *argv[]) {

	char message[PACKAGESIZE];
	memset (message,'\0',PACKAGESIZE);

	logger = log_create("swap.log", "SWAP",true, LOG_LEVEL_INFO);
	log_info(logger, "Proyecto para SWAP..");

	crearConfiguracion(argv[1]);
	crearParticionSwap();
	crearBitMap();
	listaProcesos = list_create();
	listaEnEspera = list_create();

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

	 //COMO CREAR ARCHIVO CON dd
	 //FILE *archivoSwap;
	 // dd if=/dev/zero of=nombre_swap count=cantidad_paginas bs=tamanio_paginas;

	 FILE *archivoSwap;
	 archivoSwap = fopen(nombre_swap, "wb+");
	 int i;

	 for(i = 0; i < cantidad_paginas*tamanio_pagina; i++){
		 putc('\0', archivoSwap);
	 }
	 fclose(archivoSwap);
 }
 //Creo BitMap usando bitarray
void crearBitMap(){
	bitMap = bitarray_create(bitarray, sizeof(bitarray));
}

      /*
 void inicializarBitMap(){
      bitMap_create(char *bitarray, size_t 2048);
      char bitMap[tamanio_pagina*cantidad_paginas];
      int i;
      for(i = 1; i < tamanio_pagina*cantidad_paginas; i++){
    	  bitMap[i] = '\0';
      }
      */


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

 bool hayEspacioContiguo(int pagina, int tamanio){
	 int i;
	 //la idea es verificar el cacho de registros del vector que pueden alocar ese tamanio
	 for(i = pagina; i < pagina+tamanio ;i++){
		 if(bitarray_test_bit(bitMap, i) == 1){//== 0 libre == 1 ocupado
			 return 0;
		 }
	 }
	 return 1; // retorna v, hay espacio disponible
 }

 int paginaDisponible(int pid,int tamanio){
	 int i;
	 //Recorro todo el bitMap buscando espacios contiguos y devuelvo la pagina desde donde tiene que reservar memoria, si no encuentra lugar devuelve -1

	 for(i = 0; i < tamanio_pagina*cantidad_paginas; i++ ){
		 if(bitarray_test_bit(bitMap, i) == 1){// VER 0 FALSO
			 if(hayEspacioContiguo(i,tamanio)){
				 return i;
			 }
		 }
	 }
	  return -1;
 }

 //Recorrer la lista de procesos en swap y devolver (int) la ubicación
 int  ubicacionEnArchivo(int pid, int tamanio){
 	  return 3;//return pagina; TODO
 }


 void leerArchivo(int pid,int tamanio){
	 int ubicacion = ubicacionEnArchivo(pid,tamanio);

    char cadenaLeida[tamanio];
	FILE* archivoSwap = fopen("archivoSwap.bin","r+");
    if(fseek(archivoSwap, tamanio, ubicacion) == 0){
    fread(cadenaLeida, tamanio,1,archivoSwap); //TODO validación
    }
    fclose(archivoSwap);
}


void escribirArchivo(int pid,int tamanio,char* contenido){
        int ubicacion = ubicacionEnArchivo(pid,tamanio);
        FILE* archivoSwap = fopen("archivoSwap.bin","r+");
	    if(fseek(archivoSwap, tamanio, ubicacion) == 0){
	    	fwrite(contenido, tamanio,1,archivoSwap);// TODO validacion
	    }
	    fclose(archivoSwap);

}

void crearProgramaAnSISOP(int pid,int tamanio,char* resultadoCreacion){
    int pagina = paginaDisponible(pid,tamanio);

    if(pagina != 0){
//VER COMO RESERVO MEMORIA - no va a ir a parar a lista de procesos y si al bit map
	    //reservarEspacioParaPrograma(pid, tamanio, );
	    list_add(listaProcesos, crearNodoDeProceso(pid, tamanio, pagina));
		resultadoCreacion = "Se ha creado correctamente el programa";
	}else{ //En este caso no tenemos paginas disponibles para crear el programa
		resultadoCreacion = "Inicializacion Cancelada";
	}

}

