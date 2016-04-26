#ifndef SOCKETCLIENTE_H_
#define SOCKETCLIENTE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <commons/config.h>


int conectarseA(char* ip, char* puerto);


#endif /* SOCKETCLIENTE_H_ */
