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
//  File Name : OobUfd.h
//  Description : This file contains COobUfd class definition
//===========================================================================*/

#ifndef _OOBUFD_H
#define _OOBUFD_H

#include "TransportBase.h"

// @doc

#ifdef __linux__
#define MAX_MOUNTDIR_LENGTH         128
#define MAX_FILEPATH_LENGTH         256
#endif // __linux__

typedef enum 
{
    UFD_ZERO = 0,
    UFD_INSERTED = 1,
    UFD_REMOVED = 2
} EUfdStatusCode;

// @class This is a concrete class that defines the interface of an UFD transport object.
// @base public | CTransportBase
class COobUfd : public CTransportBase
{
private:
    // store callback params for Transport
    uint32 m_usbStatus;
#ifdef WIN32
    HANDLE m_monitorEvent;
    HANDLE m_monitorThreadHandle;
    char m_usbDrive[10];
#endif // WIN32
#ifdef __linux__
    uint32 m_monitorEvent;
    uint32 m_monitorThreadHandle;
    uint32 m_recvEvent;
    uint32 m_recvThreadHandle ;
    char m_usbDir[MAX_MOUNTDIR_LENGTH];
    char m_mountPath[MAX_MOUNTDIR_LENGTH];
    bool m_ufdFound;
	bool m_killRecvTh;
	bool m_killMoniTh;
#endif // __linux__

#ifdef WIN32
    static DWORD WINAPI StaticRecvThread(void *p_data);
    static DWORD WINAPI StaticMonitorThread(void *p_data);
#endif // WIN32
#ifdef __linux__
    static void * StaticRecvThread(void *p_data);
    static void * StaticMonitorThread(void *p_data);
    void * ActualRecvThread();
#endif // __linux__

    void * ActualMonitorThread();
    
	uint32 ReadDataAndSendItUp(void);
	uint32 GetUFDFileName( char * name, int maxFileName, EOobDataType oobdType, uint8 *macAddr );

// @access public
public:
    COobUfd();
    ~COobUfd();

    uint32 Init();
    uint32 StartMonitor();
    uint32 StopMonitor();
    uint32 WriteData(char * dataBuffer, uint32 dataLen);
	uint32 WriteOobData(EOobDataType oobdType, uint8 *p_mac, BufferObj &buff);
    uint32 ReadData(char * dataBuffer, uint32 * dataLen);
	uint32 ReadOobData(EOobDataType oobdType, uint8 *p_mac, BufferObj &buff);
    uint32 UfdDeleteFile(char * fileName);
    uint32 Deinit();
    uint32 SendNotification(EUfdStatusCode status);
	void SetUFDDrive(char *drive);

}; // COobUfd

#endif // _OOBUFD_H
