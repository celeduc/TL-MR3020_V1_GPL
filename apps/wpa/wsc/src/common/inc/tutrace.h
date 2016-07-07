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
//  File: tutrace.h
//  Description: This file provides debug message printing support
//
//  Example Usage:
//  1) TUTRACE((TUTRACE_INFO, "Successfully created TimerThread"
//                   " Handle %X\n", timerThreadHandle));
//  2) TUTRACE((TUTRACE_ERR, "OpenEvent failed.\n"));
//
//===========================================================================*/

#ifndef _TUTRACE_H
#define _TUTRACE_H

#ifdef __cplusplus
extern "C" {
#endif


// include this preprocessor directive in your project/make file
#ifdef _TUDEBUGTRACE


// trace levels
#define TUERR  	0x0001
#define TUINFO  0x0002
#define TUPROTO 0x0004
#define TUUPNP  0x0008
#define TUMC    0x0010
#define TUDBG   0x0100      /* move to higher bit if more dbg level needed */
#define TUMSG   0x0200      /* move to higher bit if more dbg level needed */

#define TUTRACE_ERR        TUERR, __FILE__, __LINE__
#define TUTRACE_INFO       TUINFO, __FILE__, __LINE__
#define TUTRACE_PROTO      TUPROTO, __FILE__, __LINE__
#define TUTRACE_UPNP       TUUPNP, __FILE__, __LINE__
#define TUTRACE_MC         TUMC, __FILE__, __LINE__
#define TUTRACE_DBG         TUDBG, __FILE__, __LINE__
#define TUTRACE_MSG         TUMSG, __FILE__, __LINE__

// Set Debug Trace level here, strip UPnP
#define TUTRACELEVEL    (TUPROTO | TUERR | TUINFO | TUDBG | TUMC | TUMSG)
//#define TUTRACELEVEL    (TUERR)
//#define TUTRACELEVEL    (0)

#define TUTRACE(VARGLST)   PrintTraceMsg VARGLST

void PrintTraceMsg(int level, char *lpszFile,
                   int nLine, char *lpszFormat, ...);

#else //_TUDEBUGTRACE

#define TUTRACE(VARGLST)    ((void)0)

#endif //_TUDEBUGTRACE

#ifdef __cplusplus
}
#endif

void PrintBuffer(char *name, unsigned char *buf, int len );
void PrintBufferLevelSet( int level );
void PrintTraceLevelSet( int level );
#endif // _TUTRACE_H
