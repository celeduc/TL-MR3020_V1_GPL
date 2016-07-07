/*============================================================================
//
// Copyright(c) 2006 Intel Corporation. All rights reserved.
//   All rights reserved.
// 
//   Redistribution and use in source and binary forms, with or without 
//   modification, are permitted provided that the following conditions 
//   are met:
// 
//     * Redistributions of source code must retain the above copyright 
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright 
//       notice, this list of conditions and the following disclaimer in 
//       the documentation and/or other materials provided with the 
//       distribution.
//     * Neither the name of Intel Corporation nor the names of its 
//       contributors may be used to endorse or promote products derived 
//       from this software without specific prior written permission.
// 
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
//   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
//   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
//   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//  File Name : tutrace.cpp
//  Description: This file implements debug message printing function
//
//===========================================================================*/

#ifndef __linux__
#include <windows.h>
#endif
#ifdef __linux__
#include <stdarg.h>
#endif
#include <stdio.h>
#include <time.h>
#include "tutrace.h"

static int PrintBufferDbgLevel = -1;
static int PrintTraceDbgLevel = TUTRACELEVEL;

#ifdef _TUDEBUGTRACE

void PrintTraceMsg(int level, char *lpszFile, 
                   int nLine, char *lpszFormat, ...)
{
    char     szTraceMsg[2000];
    char *   lpszPfx;
    int      cbMsg;
    va_list  lpArgv;
	/*
    char     datetimebuffer[256];
    time_t   ltime;
	*/
#ifdef _WIN32_WCE
    TCHAR    szMsgW[2000];
#endif

    if ( ! (PrintTraceDbgLevel & level))
    {
        return;
    }

    /*
    lpszPfx = "[%08X] %s %s(%d) : ";

    time( &ltime );
    sprintf(datetimebuffer, "%s", ctime(&ltime));
    datetimebuffer[strlen(datetimebuffer) - 1] = 0;

    // Format trace msg prefix
    cbMsg = sprintf(szTraceMsg, 
                    lpszPfx, 
                    GetCurrentThreadId(),
                    datetimebuffer,
                    lpszFile, 
                    nLine);
    */
   
    lpszPfx = "%s(%d):";

    // Format trace msg prefix
    cbMsg = sprintf(szTraceMsg, 
                    lpszPfx, 
                    lpszFile, 
                    nLine);
    // Append trace msg to prefix.
    va_start(lpArgv, lpszFormat);
    cbMsg = vsprintf(szTraceMsg + cbMsg, lpszFormat, lpArgv);
    va_end(lpArgv);        

#ifndef _WIN32_WCE
#ifdef WIN32
    OutputDebugString(szTraceMsg);
#else // Linux
    fprintf(stderr, szTraceMsg);
#endif // WIN32
#else // _WIN32_WCE
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTraceMsg, -1, szMsgW, strlen(szTraceMsg)+1);
    RETAILMSG(1, (szMsgW));
#endif // _WIN32_WCE
}

#endif //_TUDEBUGTRACE

void PrintBuffer(char *name, unsigned char *buf, int len) 
{
#ifdef __linux__
	int i;
	
	if ( PrintBufferDbgLevel <= 0 || !name || !buf || len <= 0 ) return;

	printf("%s = ", name );	
	for ( i = 0; i < len; i++ ) {
		/* print 16 char per line */
		if ( (i & 0xf ) == 0 )
			printf("\n");
		printf("%2x ", *buf++);
	}
	printf("\n\n");
#endif
}

void PrintBufferLevelSet( int level )
{
	PrintBufferDbgLevel = level;
}

void PrintTraceLevelSet( int level )
{
	PrintTraceDbgLevel = level;
}
