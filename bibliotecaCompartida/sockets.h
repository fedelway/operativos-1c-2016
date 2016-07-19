/*
 * sockets.h
 *
 *  Created on: 18/7/2016
 *      Author: utnso
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

int sendAll(int fd, char *cosa, int size, int flags);
int recvAll(int fd, char *buffer, int size, int flags);

#endif /* SOCKETS_H_ */
