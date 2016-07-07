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
//  File: InbUPnPCp.h
//  Description: This file contains CInbUPnPCp class definition
//  
//==========================================================================*/

#ifndef _INBUPNPCP_H
#define _INBUPNPCP_H

#include "TransportBase.h"

#pragma pack(push, 1)

// @doc

#define WSC_EAP_DATA_MAX_LENGTH         2048
#define WSC_MAC_ADDRESS_STRING_LENGTH   18

// @class This is a concrete class that defines the interface of a UPnP Control Point transport object.
// @base public | CTransportBaseclass CInbUPnPCp
class CInbUPnPCp : public CTransportBase
{
private:
    uint32 m_UPnPThreadHandle;
    void * m_microStackChain;
    void * m_UPnPCp;
	struct UPnPDevice * m_device;
	struct UPnPService * m_service;
    char   m_macAddr[WSC_MAC_ADDRESS_STRING_LENGTH];
    static void * StaticUPnPThread(void *p_data);
    void * ActualUPnPThread();

// @access public
public:
    CInbUPnPCp();
    ~CInbUPnPCp();

    uint32 Init();
    uint32 StartMonitor();
    uint32 StopMonitor();
    uint32 WriteData(char * dataBuffer, uint32 dataLen);
    uint32 ReadData(char * dataBuffer, uint32 * dataLen);
    uint32 Deinit();

    uint32 SendStartMessage(void);
    uint32 SendSetSelectedRegistrar(char * dataBuffer, uint32 dataLen);

    void SetDeviceAndService(struct UPnPService* Service, struct UPnPDevice * Device);
    void InvokeCallback(char * buf, uint32 len);

    bool m_waitForWlanEventResp;

}; // CInbUPnPCp

#pragma pack(pop)

#endif // _INBUPNPCP_H
