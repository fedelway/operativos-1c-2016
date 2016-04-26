#include "socketServidor.h"


#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar



int conectarPuertoDeEscucha(char* puerto){

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
	int status = 1;		// Estructura que maneja el status de los recieve.

	printf("Cliente conectado. Esperando mensajes:\n");

	//Cuando el cliente cierra la conexion, recv() devolvera 0.
	while (status != 0){
	    memset (package,'\0',PACKAGESIZE); //Lleno de '\0' el package, para que no me muestre basura
		status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
		if (status != 0) printf("%s", package);
	}

	printf("Cliente se ha desconectado. Finalizo todas las conexiones.\n");

	//Al terminar el intercambio de paquetes, cerramos todas las conexiones.
	close(socketCliente);
	close(listeningSocket);

	return 0;
}
