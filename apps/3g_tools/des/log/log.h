/******************************************************************************
 *
 * Copyright (c) 2009 TP-LINK Technologies CO.,LTD.
 * All rights reserved.
 *
 * FILE NAME  :   log.h
 * VERSION    :   1.0
 * DESCRIPTION:   log all process output into file or stdout..
 *
 * AUTHOR     :   Huanglifu <Huanglifu@tp-link.net>
 * CREATE DATE:   09/21/2011
 *
 * HISTORY    :
 * 01   09/21/2011  Huanglifu     Create.
 *
 ******************************************************************************/
#ifndef __LOG2_H
#define __LOG2_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>

/*log type*/
typedef enum {
	ERROR,
	WARN,
	INFO,
	DEBUG,
	ALL,
}LogLevel;

void logout(FILE *fp, LogLevel level, char *format, va_list ap);
void logStdout(LogLevel level, char *format, ...);
void myLog(LogLevel level, char *format, ...);
void setFp(FILE *fp);
void setFpByFileName(char *filename);
void setLogLevel(LogLevel level);

#endif
