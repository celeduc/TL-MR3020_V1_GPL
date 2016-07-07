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
//  File Name : transport.h
//  Description : This file contains CTransport class definition
//===========================================================================*/

#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include "TransportBase.h"

// @module Simple Config Transport Manager |
//
// This module declares the object that manages underlying Transport objects.    
// It provides an interface between the upper MasterControl and StateMachine
// layers and the underlying transport-specific objects. 

typedef enum
{
    CLIENT_TYPE_MASTER_CONTROL = 1,
    CLIENT_TYPE_STATE_MACHINE = 2,
} CLIENT_TYPE;

typedef unsigned short sTRANSPORT_TYPE;

typedef enum
{
    TRANSPORT_TYPE_UFD = 1,
    TRANSPORT_TYPE_EAP,
    TRANSPORT_TYPE_WLAN,
    TRANSPORT_TYPE_NFC,
    TRANSPORT_TYPE_BT,
    TRANSPORT_TYPE_IP,
    TRANSPORT_TYPE_UPNP_CP,
    TRANSPORT_TYPE_UPNP_DEV,
    TRANSPORT_TYPE_MAX // insert new transport types before TRANSPORT_TYPE_MAX
} TRANSPORT_TYPE;

// This reference implementation only supports a single instance of a given 
// type.  If support for multiple instances is needed, then TRANSPORT_CONTEXT
// should be used instead of a simple TRANSPORT_TYPE.    
//
struct TRANSPORT_CONTEXT
{
    TRANSPORT_CONTEXT( TRANSPORT_TYPE t ) : trType(t) , trInstance(0) { };
    TRANSPORT_TYPE trType;
    unsigned short trInstance; 
};

class COobUfd;
class COobNfc;
class CInbEap;
class CInbWlan;
class CInbIp;

class CTransport
{
private:
    // store callback params for MasterControl
    S_CALLBACK_INFO        m_mcCallbackInfo;
    // store callback params for either RegistrarSM or EnrolleeSM
    // only 1 can be registered at any given time
    S_CALLBACK_INFO        m_smCallbackInfo;
    
    // callback data structures
    CWscQueue *     mp_cbQ;
    uint32          m_cbThreadHandle;

    bool m_cbDead;
    
    // Implementation limitation here... only one transport object per type
    // is supported in this implementation.  If multiple instances per type
    // are desired, then this will need to be changed.
    CTransportBase * m_TransportObjects[TRANSPORT_TYPE_MAX - 1];

    // callback thread
    static void * StaticCBThreadProc(IN void *p_data);
    void * ActualCBThreadProc();
    void KillCallbackThread() const;
    
    // local functions
    uint32 ProcessMessage(void * p_message);

    // GetTransportObject must be changed if multiple instances per 
    // transport type need to be supported.
    CTransportBase * GetTransportObject(TRANSPORT_TYPE trType) {
        if (trType > 0 && trType < TRANSPORT_TYPE_MAX) {
            return m_TransportObjects[trType - 1];
        }
        else {
            return NULL;
        }
    };

public:
    CTransport();
    ~CTransport();

    // callback function for others to call in
    static void StaticCallbackProc(IN void * data, IN void * cookie);
    uint32 SetMCCallback(IN CALLBACK_FN p_mcCallbackFn, IN void* cookie);
    
    uint32 SetSMCallback(IN CALLBACK_FN p_smCallbackFn, IN void* cookie);
    void* GetSMCallback(IN void);

    uint32 SetBeaconIE( IN uint8 *p_data, IN uint32 length );
    uint32 SetProbeReqIE( IN uint8 *p_data, IN uint32 length );
    uint32 SetProbeRespIE( IN uint8 *p_data, IN uint32 length );
    uint32 SendBeaconsUp(IN bool activate);
    uint32 SendProbeResponsesUp(IN bool activate);
    uint32 SendProbeRequest( IN char * ssid);

    uint32 EnableWalkTimer(bool enable);
    uint32 SendStartMessage(TRANSPORT_TYPE trType);
    uint32 SendSetSelectedRegistrar(char * dataBuffer, uint32 dataLen);

    uint32 Init();
    uint32 StartMonitor(TRANSPORT_TYPE trType);
    uint32 StartMonitor(TRANSPORT_TYPE trType, bool serverFlag);
    uint32 StopMonitor(TRANSPORT_TYPE trType);
    uint32 TrRead(TRANSPORT_TYPE trType, char * dataBuffer, uint32 * dataLen);
    uint32 TrWrite(TRANSPORT_TYPE trType, char * dataBuffer, uint32 dataLen);
    uint32 TrReadOobData(TRANSPORT_TYPE trType, EOobDataType type, 
			uint8 *macAddr, BufferObj &buff);
    uint32 TrWriteOobData(TRANSPORT_TYPE trType, EOobDataType type, 
			uint8 *macAddr, BufferObj &buff);
    uint32 Deinit();
    bool CheckAPStartedFlag();
    void ClearAPStartedFlag();
    uint32 ReSendEapStart();
    void CTransport::ClearHostapdCtrlPort();
    void CTransport::ClearStaMacAddr();
    void CTransport::GetStaMacAddr(char *macAddr);
}; // CTransport


#endif // _TRANSPORT_H
