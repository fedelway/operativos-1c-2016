/*
 * generales.h
 *
 *  Created on: 7/5/2016
 *      Author: utnso
 */

#ifndef GENERALES_H_
#define GENERALES_H_

#include <stdarg.h>
#include "commons/log.h"

void log_error_y_cerrar_logger(t_log* logger, const char* message, ...);

#endif /* GENERALES_H_ */
