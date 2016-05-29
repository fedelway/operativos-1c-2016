#ifndef SWAP_H_
#define SWAP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // for close conections
#include "commons/config.h"
#include "commons/log.h"
#include "commons/collections/list.h"
#include "socketServidor.h"
#include "commons/bitarray.h"
//para trabajar con el mmap()
#include <sys/mman.h>


void crearConfiguracion(char* config_path); //levanta el archivo de configuracion y lo asigna a una estructura t_config
bool validarParametrosDeConfiguracion(); //Valida que el archivo de configuracion tenga todos los parametros requeridos
void recibirMensajeUMC(char* message, int socket_umc);
void inicializarBitMap();
void crearNodo(int pid, int cantidad_paginas, int posSwap);


#endif /* SWAP_H_ */

//parámetros de configuración
    char* puerto_escucha;
	char* nombre_swap;
	int cantidad_paginas;
	int tamanio_pagina;
	int retardo_compactacion;

//bitMap
	//Configuro el tamaño del BITMAP con los valores de configuración convertidos a bit

	char bitarray[512];
	t_bitarray *bitMap;


//nodo lista de procesos
	 typedef struct {
		 int pid;
		 int cantidad_paginas;
		 int posSwap;
}nodo_proceso;

//nodo lista de procesos en espera
	 typedef struct {
		 int pid;
		 int cantidad_paginas;
}nodo_enEspera;


//listas
t_list *listaProcesos;
t_list *listaEnEspera;
