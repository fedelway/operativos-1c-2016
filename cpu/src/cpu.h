/*
 * cpu.h
 *
 *  Created on: 29/4/2016
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"
#include "socketCliente.h"
#include "primitivasAnsisop.h"

#define PACKAGESIZE 1024

void crearConfiguracion(); //creo la configuracion y checkeo que sea valida
bool validarParametrosDeConfiguracion();
void recibirMensajeNucleo(char* message, int socket_nucleo);
void validarNucleo(int nucleo_fd);
void enviarPaqueteAUMC(char* package, int socket);

#endif /* CPU_H_ */
