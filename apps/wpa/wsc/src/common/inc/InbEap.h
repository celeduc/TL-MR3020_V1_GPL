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
//
//  File: InbEap.h
//  Description: This file contains CInbEap class definition
//  
//==========================================================================*/

#ifndef _INBEAP_H
#define _INBEAP_H

#include "TransportBase.h"

#pragma pack(push, 1)

// @doc

#define WSC_EAP_DATA_MAX_LENGTH         2048

#define WSC_NOTIFY_TYPE_BUILDREQ              1
#define WSC_NOTIFY_TYPE_BUILDREQ_RESULT       2
#define WSC_NOTIFY_TYPE_PROCESS_REQ           3
#define WSC_NOTIFY_TYPE_PROCESS_RESP          4
#define WSC_NOTIFY_TYPE_PROCESS_RESULT        5

#define WSC_NOTIFY_RESULT_SUCCESS			  0x00
#define WSC_NOTIFY_RESULT_FAILURE			  0xFF

typedef uint32  u32;
typedef uint8   u8;

typedef struct wsc_notify_buildreq_tag {
    u32    id;
    u32 state;
} __attribute__ ((__packed__)) WSC_NOTIFY_BUILDREQ;

typedef struct wsc_notify_process_buildreq_result_tag {
	u8 result;
} __attribute__ ((__packed__)) WSC_NOTIFY_BUILDREQ_RESULT;

typedef struct wsc_notify_process_tag {
	u32 state;
} __attribute__ ((__packed__)) WSC_NOTIFY_PROCESS;

typedef struct wsc_notify_process_result_tag {
	u8 result;
	u8 done;
} __attribute__ ((__packed__)) WSC_NOTIFY_PROCESS_RESULT;

typedef struct wsc_notify_data_tag {
    u8 type;
    union {
        WSC_NOTIFY_BUILDREQ bldReq;
        WSC_NOTIFY_BUILDREQ_RESULT bldReqResult;
        WSC_NOTIFY_PROCESS process;
        WSC_NOTIFY_PROCESS_RESULT processResult;
    } u;
    u8 sta_mac_addr[6];
    u32 length; // length of the data that follows
} __attribute__ ((__packed__)) WSC_NOTIFY_DATA;

#define WSC_EAP_CODE_REQUEST    1
#define WSC_EAP_CODE_RESPONSE   2
#define WSC_EAP_CODE_SUCCESS    3
#define WSC_EAP_CODE_FAILURE    4 

#define WSC_EAP_TYPE            254
#define WSC_VENDORID1           0x00
#define WSC_VENDORID2           0x37
#define WSC_VENDORID3           0x2A
#define WSC_VENDORTYPE          0x00000001

//WSC Message types
#define WSC_Start 0x01
#define WSC_ACK   0x02
#define WSC_NACK  0x03
#define WSC_MSG   0x04
#define WSC_Done  0x05

typedef struct wsc_eap_header_tag {
    uint8 code;
    uint8 id;
    uint16 length;
    uint8 type;
    uint8 vendorId[3];
    uint32 vendorType;
    uint8 opcode;
    uint8 flags;
} __attribute__ ((__packed__)) WSC_EAP_HEADER;

typedef struct {
    WSC_EAP_HEADER header;
    uint16       messageLength;
} __attribute__ ((__packed__)) S_WSC_FRAGMENT_HEADER;

// @class This is a concrete class that defines the interface of an EAP transport object.
// @base public | CTransportBaseclass CInbEap
class CInbEap : public CTransportBase
{
private:
    uint32 m_recvEvent;
    uint32 m_recvThreadHandle;

    int32 m_udpFdEap;
    uint16 m_recvPort;

    bool m_readDataTimeoutEnable; 
    unsigned long m_readDataStartTime; 

    uint8 m_recentType;
    uint8 m_recentId;
    uint32 m_recentState;
    uint8 m_sta_mac_addr[SIZE_MAC_ADDR];
    bool m_gotReq;
    char m_storeBuf[WSC_EAP_DATA_MAX_LENGTH];
    uint32 m_storeLen;

    static void * StaticRecvThread(void *p_data);
    void * ActualRecvThread();

    void ProcessData(char * buf, uint32 len);
    void InvokeCallback(char * buf, uint32 len);
    uint32 AttachHeaderAndSend(char * dataBuffer, uint32 dataLen, 
    uint8 wscNotifyCode, uint8 eapCode);
    uint32 SendDataDown(char * dataBuffer, uint32 dataLen);
    uint32 SendQuit();
    bool CInbEap::CheckEapTimeout();

// @access public
public:
    CInbEap();
    ~CInbEap();

    uint32 Init();
    uint32 StartMonitor();
    uint32 StopMonitor();
    uint32 WriteData(char * dataBuffer, uint32 dataLen);
    uint32 ReadData(char * dataBuffer, uint32 * dataLen);
    uint32 ReadDataTimeoutEnable(bool enable);
    uint32 Deinit();

    void CInbEap::ClearStaMacAddr(void);
    void CInbEap::GetStaMacAddr(char *macAddr);
    uint32 SendStartMessage(void);

}; // CInbEap

#pragma pack(pop)

#endif // _INBEAP_H
