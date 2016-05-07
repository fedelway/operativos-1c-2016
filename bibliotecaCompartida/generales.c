/*
 * generales.c
 *
 *  Created on: 7/5/2016
 *      Author: utnso
 */

#include "generales.h"

void log_error_y_cerrar_logger(t_log* logger, const char* message, ...){
	va_list arguments;
	va_start(arguments, message);
	log_error(logger, message, arguments);
    log_destroy(logger);
    printf("hago el log destroy!\n");
}
