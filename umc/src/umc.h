#ifndef UMC_H_
#define UMC_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "commons/log.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // for close conection
#include "commons/config.h"
#include "commons/collections/list.h"
#include <pthread.h>
#include "socketCliente.h"
#include "socketServidor.h"
#include "protocolo.h"

//Definicion de estructuras
typedef struct{
	int nro_pag;
	int frame;
	bool presencia;
	bool modificado;
	bool referenciado;
}t_pag;

typedef struct{
	int pid;
	int cant_total_pag;
	t_pag *paginas;
	int *pag_en_memoria;
	int puntero; //Para el clock, indica a que pagina estoy apuntando
	int timer; //Para algoritmo clock. Cada vez que hay una lectura/escritura se incrementa
}t_prog;

typedef struct{
	int posicion;
	bool libre;
}t_frame;

typedef struct{
	int pid;
	int nro_pag;
	int traduccion;
	int tiempo_accedido;
	bool modificado;
}t_entrada_tlb;

typedef struct{
	t_entrada_tlb *entradas;
	int cant_entradas;
	int timer; //Para LRU
}t_tlb;

//Variables globales
t_log* logger;
t_config *config;
int swap_fd; //Lo hago global porque vamos a laburar con hilos. Esto no se sincroniza porque es solo lectura.
int nucleo_fd;
t_frame *frames;//El array con todos los frames en memoria
int cant_frames;
int frame_size;
int fpp; //Frames por programa
int stack_size;
int timer_reset_mem;
t_tlb cache_tlb;


t_list *programas;

//Esta seria el area de memoria.
char *memoria;

//Mutexes
pthread_mutex_t mutex_memoria = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listaProgramas = PTHREAD_MUTEX_INITIALIZER;

void crearConfiguracion(char* config_path);
bool validarParametrosDeConfiguracion();
void handshakeSWAP();
void recibirMensajeCPU(char* message, int socket_CPU);
void enviarPaqueteASwap(char* message, int socket);
int framesDisponibles();
void recibirConexiones(int cpu_fd, int max_fd);
void aceptarNucleo();
void trabajarNucleo();
void trabajarCpu();
int aceptarCpu(int cpu_fd_listen, int *cpu_num);
void inicializarMemoria();
void inicializarPrograma();
void inicializarTlb();
int buscarEnTlb(int pag, int pid);
void actualizarTlb(int pag, int pid, int traduccion);
void reemplazarEntradaTlb(int pag, int pid, int traduccion);
int escribirEnMemoria(char *src, int pag, int offset, int size, t_prog *programa);
int leerEnMemoria(char *resultado, int pag, int offset, int size, t_prog *programa);
void terminarPrograma(int pid);
int enviarCodigoASwap(char *source, int source_size);
void traerPaginaDeSwap(int pag, t_prog *programa);
void enviarPagina(int pag, int pid, int pos_a_enviar);
int recibirPagina(int pag, int pid); //Devuelve el frame en donde escribio la pagina
void algoritmoClock(t_prog *programa);
void flushTlb();
void flushTlb(int pid);
int frameLibre();
int min(int a, int b);
void leerParaCpu(int cpu_fd);
void escribirParaCpu(int cpu_fd);
t_prog *buscarPrograma(int pid);
int sendAll(int fd, void *cosa, int size, int flags);

#endif /* UMC_H_ */
