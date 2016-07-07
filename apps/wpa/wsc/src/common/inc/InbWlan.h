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
//  File: InbWlan.h
//  Description: This file contains CInbWlan class definition
//===========================================================================*/

#ifndef _INBWLAN_H
#define _INBWLAN_H

#include "TransportBase.h"

#pragma pack(push, 1)

#define WSC_WLAN_UDP_PORT       38000
#define WSC_WLAN_UDP_ADDR       "127.0.0.1"

#define WSC_WLAN_DATA_MAX_LENGTH         1024

#define WSC_IE_TYPE_SET_BEACON_IE     			1
#define WSC_IE_TYPE_SET_PROBE_REQUEST_IE     	2
#define WSC_IE_TYPE_SET_PROBE_RESPONSE_IE     	3
#define WSC_IE_TYPE_BEACON_IE_DATA     			4
#define WSC_IE_TYPE_PROBE_REQUEST_IE_DATA     	5
#define WSC_IE_TYPE_PROBE_RESPONSE_IE_DATA     	6
#define WSC_IE_TYPE_SEND_BEACONS_UP				7
#define WSC_IE_TYPE_SEND_PR_RESPS_UP			8
#define WSC_IE_TYPE_SEND_PROBE_REQUEST          9
#define WSC_IE_TYPE_MAX                         10

typedef uint32  u32;
typedef uint8   u8;

typedef struct wsc_ie_header {
	u8 elemId;
	u8 length;
	u8 oui[4];
} __attribute__ ((__packed__)) WSC_IE_HEADER;

typedef struct wsc_ie_command_data {
    u8 type;
    u32 length;
    u8 data[];
} __attribute__ ((__packed__)) WSC_IE_COMMAND_DATA;


// @doc

// @class This is a concrete class that defines the interface of an WLAN transport object.
// @base public | CTransportBase
class CInbWlan : public CTransportBase
{
private:
    uint32 m_recvEvent;
    uint32 m_recvThreadHandle;

    int32 m_udpFd;
    uint16 m_recvPort[2];

    bool m_apStarted;

    static void * StaticRecvThread(void *p_data);
    void * ActualRecvThread();

	uint32 SendDataDown(char * dataBuffer, uint32 dataLen);
    
// @access public
public:
    CInbWlan();
    ~CInbWlan();

    uint32 Init();
    uint32 StartMonitor();
    uint32 StopMonitor();
    uint32 WriteData(char * dataBuffer, uint32 dataLen);
    uint32 ReadData(char * dataBuffer, uint32 * dataLen);
    void CInbWlan::ClearRecvPort();
    uint32 Deinit();

    uint32 SetBeaconIE( IN uint8 *p_data, IN uint32 length );
    uint32 SetProbeReqIE( IN uint8 *p_data, IN uint32 length );
    uint32 SetProbeRespIE( IN uint8 *p_data, IN uint32 length );
	uint32 SendBeaconsUp(IN bool activate);
	uint32 SendProbeResponsesUp(IN bool activate);
    uint32 SendProbeRequest( IN char * ssid);

    void CInbWlan::ClearAPStartedFlag();
    bool CInbWlan::CheckAPStartedFlag();

}; // CInbWlan

#pragma pack(pop)

#endif // _INBWLAN_H
