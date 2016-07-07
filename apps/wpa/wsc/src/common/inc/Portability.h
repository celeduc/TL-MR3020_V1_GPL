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
//  File Name: Portability.h
//  Description: Portability layer header file
//
****************************************************************************/

#ifndef _WSC_PORTAB_
#define _WSC_PORTAB_

#include "WscTypes.h"

#ifdef __cplusplus
extern "C" {
#endif


/*Synchronization functions*/
uint32 WscSyncCreate(uint32 **handle);
uint32 WscSyncDestroy(uint32 *handle);
uint32 WscLock(uint32 *handle);
uint32 WscUnlock(uint32 *handle);

/*Thread functions*/
uint32 WscCreateThread(uint32 *handle, 
                        void *(*threadFunc)(void *),
                        void *arg);
void WscDestroyThread(uint32 handle);

/*Event functions*/
uint32 WscCreateEvent(uint32 *handle);
uint32 WscDestroyEvent(uint32 handle);
uint32 WscSetEvent(uint32 handle);
uint32 WscResetEvent(uint32 handle);
uint32 WscSingleWait(uint32 handle, uint32 *lock, uint32 timeout);

/*Byte swapping functions*/
uint32 WscHtonl(uint32 intlong);
uint16 WscHtons(uint16 intshort);
uint32 WscNtohl(uint32 intlong);
uint16 WscNtohs(uint16 intshort);

/*Sleep function*/
void WscSleep(uint32 seconds);

#ifdef __cplusplus
}
#endif

#endif /*_WSC_PORTAB_*/
