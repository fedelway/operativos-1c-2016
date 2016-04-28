#ifndef SOCKETSERVIDOR_H_
#define SOCKETSERVIDOR_H_


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


int conectarPuertoDeEscucha(char* puerto);



#endif /* SOCKETSERVIDOR_H_ */
