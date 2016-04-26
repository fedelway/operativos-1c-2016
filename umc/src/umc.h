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

#include "socketCliente.h"



void crearConfiguracion(char* config_path);
bool validarParametrosDeConfiguracion();



#endif /* UMC_H_ */
