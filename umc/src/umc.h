#ifndef UMC_H_
#define UMC_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // for close conection
#include "commons/config.h"
#include "commons/collections/list.h"
#include <pthread.h>

#include "socketCliente.h"
#include "socketServidor.h"

//Definicion de estructuras
typedef struct{
	int frame;
	bool presencia;
	bool modificado;
}t_pag;

typedef struct{
	int pid;
	int pos; //Para saber cuanto ya hay escrito
	t_pag *paginas;
}t_prog;

typedef struct{
	int posicion;
	bool libre;
}t_frame;

//Variables globales
t_config *config;
int swap_fd; //Lo hago global porque vamos a laburar con hilos. Esto no se sincroniza porque es solo lectura.
int nucleo_fd;
t_frame *frames;//El array con todos los frames en memoria
int cant_frames;
int frame_size;
int fpp; //Frames por programa
int stack_size;

t_list *programas;

char *memoria; //Esta seria el area de memoria.



void crearConfiguracion(char* config_path);
bool validarParametrosDeConfiguracion();
void recibirMensajeCPU(char* message, int socket_CPU);
void enviarPaqueteASwap(char* message, int socket);
int framesDisponibles();
void recibirConexiones(int cpu_fd, int max_fd);
void aceptarNucleo();
void trabajarNucleo();
void trabajarCpu();
void inicializarMemoria();
void inicializarPrograma();
void escribirEnMemoria(char *src, int size, t_prog programa);
void terminarPrograma(int pid);
int enviarCodigoASwap(char *source, int source_size);


#endif /* UMC_H_ */
