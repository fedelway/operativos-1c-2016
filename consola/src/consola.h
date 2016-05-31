/*
 * consola.h
 *
 *  Created on: 7/5/2016
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h> //Para manejar archivos con fd, no se si lo vayamos a usar, pero lo use y lo cambie
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // Para cerrar conexiones
#include "commons/log.h"
#include "commons/config.h"
#include "generales.h"

typedef struct {
	char* nucleo_ip;
	char* nucleo_puerto;
}t_config_consola;

void levantarDatosDeConfiguracion(t_config_consola* configConsola, char* config_path);
bool validarParametrosDeConfiguracion();
int conectarseA(char* ip, char* puerto);
void handshakeConNucleo(int nucleo_fd);
FILE *abrirSource(char *path, int *source_size);
void enviar_source(int nucleo_fd, FILE *source, int source_size);

#endif /* CONSOLA_H_ */
