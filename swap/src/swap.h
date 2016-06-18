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
#include "protocolo.h"
#include "commons/bitarray.h"
#include <sys/mman.h> //para trabajar con el mmap()
#include <fcntl.h>


//------------------------------- VARIABLES GLOBALES--------------------------------//

//parámetros de configuración
    char* puerto_escucha;
	char* nombre_swap;
	int cantidad_paginas;
	int tamanio_pagina;
	int retardo_compactacion;

//bitMap usando bitarray

	char *bitarray;
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

int socket_escucha,socket_umc,tamanioBitMap;
int socket_swap;



void handshakeUMC();
int aceptarConexion(int puerto);
void crearConfiguracion(char* config_path); //levanta el archivo de configuracion y lo asigna a una estructura t_config
bool validarParametrosDeConfiguracion(); //Valida que el archivo de configuracion tenga todos los parametros requeridos
void recibirMensajeUMC(char* message, int socket_umc);
static nodo_proceso *crearNodoDeProceso(int pid, int cantidad_paginas, int posSwap);
static nodo_enEspera *crearNodoEnEspera(int pid, int cantidad_paginas);
void inicializarBitMap();
char* cargarArchivo();
void inicializarArchivo(char* archivoAMemoria);
void crearBitMap();
bool hayEspacioContiguo(int pagina, int tamanio);
int paginaDisponible(int tamanio);
int  ubicacionEnSwap(int pid);
char* crearProgramaAnSISOP(int pid,int tamanio,char* resultadoCreacion,char* codigo_prog,char* archivoMapeado);
void leerUnaPagina(int pid,int pag);
void modificarPagina(int pid, int pagina, char* nuevoCodigo);
int espaciosLibres(int cantidad);
bool hayFragmentacion(int tamanio);
void prepararLugar( int pid, int tamanio);
void intercambioEnBitmap(int posicionVacia, int posicionOcupada);
void obtenerPrograma(int posicionEnSwap);
//nodo_proceso obtenerPrograma(int posicionEnSwap);
void actualizoListaDeProcesos(void posicioAIntercambiar, int j);
bool posicionVacia(int posicion);
void presenteEnLIsta(int posicion);
void intercambioEnBitmap(int posicionAnterior, int posicionActual);
void actualizarEstructuras(int posicionNueva, int posicionAnterior);
void modificarArchivoSwap(nodo_proceso * nodoPrograma);
void modificarPosicionEnSwap(int posicionNueva, int posicionAnterior);
void comenzarCompactacion();
#endif /* SWAP_H_ */
