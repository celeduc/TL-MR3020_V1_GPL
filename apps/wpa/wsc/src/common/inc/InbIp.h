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
//  File Name: InbIp.h
//  Description: This file contains CInbIp class definition
//===========================================================================*/

#ifndef _INBIP_H
#define _INBIP_H

#include "TransportBase.h"

#pragma pack(push, 1)

// @doc

#define WSC_INBIP_IP_PORT     27645
//#define WSC_INBIP_IP_ADDR     "192.168.1.1"
#define WSC_INBIP_IP_ADDR     "127.0.0.1"

#define WSC_INBIP_RECV_BUFSIZE      2048

// @class This is a concrete class that defines the interface of an EAP transport object.
// @base public | CTransportBaseclass CInbIp
class CInbIp : public CTransportBase
{
private:
    bool m_server;
    bool m_monitoring;
    bool m_initialized;
#ifdef WIN32
    SOCKET m_listenSock;
    SOCKET m_connectSock;
    SOCKET m_sendSock;
    SOCKET m_readSock;
    WSAEVENT m_shutdownEvent;
#endif // WIN32
#ifdef __linux__
    int m_listenSock;
    int m_connectSock;
    int m_activeSock;
    int m_socketPipe[2];
#endif // __linux__
    uint32 m_connectThreadHandle;
    uint32 m_listenThreadHandle;

    static void * StaticListenThread(void *p_data);
    void * ActualListenThread();
    static void * StaticConnectThread(void *p_data);
    void * ActualConnectThread();
    static void * StaticReadThread(void *p_data);
    void * ActualReadThread();

    void InvokeCallback(char * buf, uint32 len);
    void SendConnectedNotification(void);
#ifdef __linux__
    bool WaitFunction(uint32 threadHandle);
    bool WaitForShortTime();
#endif // __linux__

// @access public
public:
    CInbIp();
    ~CInbIp();

    uint32 Init();
    uint32 Init(bool serverFlag);
    uint32 StartMonitor();
    uint32 StopMonitor();
    uint32 WriteData(char * dataBuffer, uint32 dataLen);
    uint32 ReadData(char * dataBuffer, uint32 * dataLen);
    uint32 Deinit();

}; // CInbIp

#pragma pack(pop)

#endif // _INBIP_H
