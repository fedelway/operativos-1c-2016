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
#include "generales.c"
#include "protocolo.h"
#include "commons/bitarray.h"
#include <sys/mman.h> //para trabajar con el mmap()
#include <pthread.h>
#include <fcntl.h>


//------------------------------- VARIABLES GLOBALES-------------------------------------//

//parámetros de configuración
    char* PUERTO_ESCUCHA;
	char* NOMBRE_SWAP;
	int CANTIDAD_PAGINAS;
	int TAMANIO_PAGINA;
	int RETARDO_COMPACTACION;

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
		 int numeroPagina;
		 char* contenido;
		 int mensaje;
}nodo_enEspera;

//listas

t_list *listaProcesos;
t_list *listaEnEspera;

int estaCompactando = 0;
#define PROCESO_EN_ESPERA 5100;
// #define PROCESO_NUEVO 5200;

//SEMÁFOROS
pthread_mutex_t peticionesActuales = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t enEspera = PTHREAD_MUTEX_INITIALIZER;



int socket_escucha,socket_umc,tamanioBitMap;
int socket_swap;
FILE *file;
int pagesize;



void handshakeUMC();
int aceptarConexion(int puerto);
void crearConfiguracion(char* config_path); //levanta el archivo de configuracion y lo asigna a una estructura t_config
bool validarParametrosDeConfiguracion(); //Valida que el archivo de configuracion tenga todos los parametros requeridos
void recibirMensajeUMC(char* message, int socket_umc);
nodo_proceso *crearNodoDeProceso(int pid, int cantidad_paginas, int posSwap);
nodo_enEspera *crearNodoEnEspera(int pid, int cantidad_paginas,int numeropPagina, char* contenido, int mensaje);
void agregarNodoEnEspera(int pid, int cantidadPaginas, int numeroPagina, char* contenido, int mensaje);
void agregarNodoProceso(int pid, int cantidadPaginas, int posicionSwap);
int cantPaginas(nodo_proceso *nodo);
int pid(nodo_proceso *nodo);
int posicionSwap(nodo_proceso *nodo);


//---------------------- INICIALIZACIÓN -------------------------------------------//

void inicializarBitMap();
char* cargarArchivo();
void crearBitMap();
void crearParticionSwap();

//------------------- PETICIONES UMC -------------------------------------------------//

bool hayEspacioContiguo(int pagina, int tamanio);
int paginaDisponible(int tamanio);
int  ubicacionEnSwap(int pid);
void crearProgramaAnSISOP(int pid, int tamanio);
void crearProgramaAnSISO(int pid, int tamanio, char* codigo_prog);
void leerUnaPagina(int pid,int pag);
void modificarPagina(int pid, int pagina, char* nuevoCodigo);
void trabajarUmc();
void atenderProcesosEnEspera();
int hayProgramasEnEspera();

//------------------------------- COMPACTACION -----------------------------------------------//

int espaciosLibres(int cantidad);
bool hayFragmentacion(int tamanio);
void prepararCompactacion(int tamanio);
nodo_proceso *obtenerPrograma(int posicionEnSwap);
void actualizoListaDeProcesos(int posicioAIntercambiar, int j);
bool posicionVacia(int posicion);
void intercambioEnBitmap(int posicionAnterior, int posicionActual);
void modificarArchivoSwap(int posicionNueva, int posicionAnterior);
void modificarLista(int posicionNueva, int posicionAnterior);
void comenzarCompactacion();
void informarAUmc();
void cancelarInicializacion();
void encolarProgramas(int mensaje);

//--------------------------- TERMINAR PROGRAMA	 --------------------------------------------//

void liberarEstructuras(nodo_proceso *nodo);
void borrarDeArchivoSwap(nodo_proceso *nodo);
void borrarDeListaDeProcesos(nodo_proceso *nodo);
void liberarPosicion(nodo_proceso *nodo);
void terminarProceso(int pid);

//--------------------------- ELIMINAR ESTRUCTURAS -----------------------------------------//

void eliminarArchivoMapeado();
void cerrarArchivo();
void eliminarEstructuras();

#endif /* SWAP_H_ */
