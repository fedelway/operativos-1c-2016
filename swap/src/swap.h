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

#include "socketServidor.h"


void crearConfiguracion(char* config_path); //levanta el archivo de configuracion y lo asigna a una estructura t_config
bool validarParametrosDeConfiguracion(); //Valida que el archivo de configuracion tenga todos los parametros requeridos



#endif /* SWAP_H_ */
