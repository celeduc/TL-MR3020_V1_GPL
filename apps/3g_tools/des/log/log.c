/******************************************************************************
 *
 * Copyright (c) 2009 TP-LINK Technologies CO.,LTD.
 * All rights reserved.
 *
 * FILE NAME  :   log.c
 * VERSION    :   1.0
 * DESCRIPTION:   log all process output into file or stdout.
 *
 * AUTHOR     :   Huanglifu <Huanglifu@tp-link.net>
 * CREATE DATE:   09/21/2011
 *
 * HISTORY    :
 * 01   09/21/2011  Huanglifu     Create.
 *
 ******************************************************************************/
 #include "log.h"

/*default log level*/
LogLevel logLevel = DEBUG;
 
/*log file's pointer*/
FILE *logFp = NULL;

/*mutex for muti pthread runing called */
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

/*log level corresponding string.*/
char *logHintStr[10]={
	"ERROR",
	"WARN",
	"INFO",
	"DEBUG",
	"ALL",
};
/******************************************************************************
 * FUNCTION      :   setFp()
 * DESCRIPTION   :   set log file pointer. 

 ******************************************************************************/
void setFp(FILE *fp)
{
	logFp = fp;
}


/******************************************************************************
 * FUNCTION      :   setFpByFileName()
 * DESCRIPTION   :   set output log filename. 
 ******************************************************************************/
void setFpByFileName(char *filename)
{
	FILE *fp;
	
	fp = fopen(filename,"wb+");
	if (NULL == fp)
	{
		printf("ERROR:open log file fail.@%s",filename);
		return;
	}	
	setFp(fp);
}

/******************************************************************************
 * FUNCTION      :   setLogLevel()
 * DESCRIPTION   :   set log level. 
 ******************************************************************************/
void setLogLevel(LogLevel level)
{
	logLevel = level;
}


/******************************************************************************
 * FUNCTION      :   logout()
 * DESCRIPTION   :   log output params with format into file. 
 ******************************************************************************/
void logout(FILE *fp, LogLevel level, char *format, va_list ap)
{
	pthread_mutex_lock(&logMutex);
	static char  buf[2048];
	vsprintf(buf, format, ap);	
	fprintf(fp, "[%s]:%s\n", logHintStr[level], buf);	
	pthread_mutex_unlock(&logMutex);
}

/******************************************************************************
 * FUNCTION      :   logStdout()
 * DESCRIPTION   :   log output to stdout. 
 ******************************************************************************/
void logStdout(LogLevel level,char *format, ...)
{
	
	if (level > logLevel) 
	{
		return;
	}
	
	va_list ap;
	va_start(ap,format);
	logout(stdout, level, format,ap);
	va_end(ap);

}

/******************************************************************************
 * FUNCTION      :   myLog()
 * DESCRIPTION   :   log output to file. 
 ******************************************************************************/
void myLog(LogLevel level,char *format, ...)
{
	
	if (level > logLevel) 
	{
		return;
	}
	
	
	va_list ap;
	va_start(ap,format);
	if (logFp == NULL)
 	{
		logout(stdout, level, format, ap);
	}else
	{
		logout(logFp, level, format, ap);
	}
	va_end(ap);

}
