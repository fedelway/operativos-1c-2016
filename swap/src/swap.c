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


int main(int argc,char *argv[]) {

	char* puerto_escucha;

	char message[PACKAGESIZE];
	memset (message,'\0',PACKAGESIZE);

	logger = log_create("swap.log", "SWAP",true, LOG_LEVEL_INFO);
	log_info(logger, "Proyecto para SWAP..");

	crearConfiguracion(argv[1]);

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


//Acá implementamos el handshake del lado del servidor
void handshakeServidor(int socketCliente){

		//Estructura para crear el header + piload
		struct t_header{
			int id;
			int tamanio;
		};

		struct package{
		  struct t_header header;
		  char* paiload;
		};

		int socketClienteHandshake = accept(listeningSocket, (struct sockaddr *) &addr, &addrlen);

		//seteo el mensaje que le envío a la umc para que ésta reciba el header.id=2 y haga desde su proceso el handshake
		struct package mensaje;
		mensaje.header.id = 2;
		mensaje.header.tamanio = 1;
		mensaje.paiload = "\0";

		send(socketClienteHandshake, mensaje.paiload, (mensaje.header.tamanio), 0);

        // Acá hay que ver si cambiamos el nombre del mensaje por package, tengo la duda de si toma el mensaje seteado del send() anterior
		switch(mensaje.header.id){

		 case 1:
			 // Procesa ok
			 printf("OK\n");
		 break;

		 case 2:
			 //Handshake cliente
			 printf("No hace nada porque está reservado para el cliente\n");
		 break;

		 case 3:
			 //Respuesta de Handshake servidor
			    memset(mensaje.paiload,'\0', mensaje.header.tamanio); //Lleno de '\0' el package, para que no me muestre basura
			    recv(socketCliente, mensaje.paiload, 8, 0);

			    //Hasta ahora solo imprimo por pantalla si acepté o no la conexión, faltaría implementar lo que le enviamos si rechazo o acepto....
			    if( !(strcmp(mensaje.paiload , "UMC"))){
			    	printf("Admito conexión con la UMC");
			    }else
			    	printf("No se admite la conexión con éste socket");
	     break;
		}
	}
}

