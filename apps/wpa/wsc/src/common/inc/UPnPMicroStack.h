/*============================================================================
//
// Copyright(c) 2006 Intel Corporation. All rights reserved.
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
// $Workfile: UPnPMicroStack.h
// $Revision: #1.0.2126.26696
// $Author:   Intel Corporation, Intel Device Builder
// $Date:     Tuesday, January 17, 2006
*/

#ifndef __UPnPMicrostack__
#define __UPnPMicrostack__


#include "ILibAsyncSocket.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \file UPnPMicroStack.h 
	\brief MicroStack APIs for Device Implementation
*/

/*! \defgroup MicroStack MicroStack Module
	\{
*/

struct UPnPDataObject;
struct packetheader;

typedef void* UPnPMicroStackToken;
typedef void* UPnPSessionToken;


/* Complex Type Parsers */


/* Complex Type Serializers */



/* UPnP Stack Management */
UPnPMicroStackToken UPnPCreateMicroStack(void *Chain, const char* FriendlyName,const char* UDN, const char* SerialNumber, const int NotifyCycleSeconds, const unsigned short PortNum);


void UPnPIPAddressListChanged(UPnPMicroStackToken MicroStackToken);
int UPnPGetLocalPortNumber(UPnPSessionToken token);
int   UPnPGetLocalInterfaceToHost(const UPnPSessionToken UPnPToken);
void* UPnPGetWebServerToken(const UPnPMicroStackToken MicroStackToken);
void UPnPSetTag(const UPnPMicroStackToken token, void *UserToken);
void *UPnPGetTag(const UPnPMicroStackToken token);
UPnPMicroStackToken UPnPGetMicroStackTokenFromSessionToken(const UPnPSessionToken token);

typedef void(*UPnP_ActionHandler_WFAWLANConfig_DelAPSettings) (void* upnptoken,unsigned char* NewAPSettings,int _NewAPSettingsLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_DelSTASettings) (void* upnptoken,unsigned char* NewSTASettings,int _NewSTASettingsLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_GetAPSettings) (void* upnptoken,unsigned char* NewMessage,int _NewMessageLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_GetDeviceInfo) (void* upnptoken);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_GetSTASettings) (void* upnptoken,unsigned char* NewMessage,int _NewMessageLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_PutMessage) (void* upnptoken,unsigned char* NewInMessage,int _NewInMessageLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_PutWLANResponse) (void* upnptoken,unsigned char* NewMessage,int _NewMessageLength,unsigned char NewWLANEventType,char* NewWLANEventMAC);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_RebootAP) (void* upnptoken,unsigned char* NewAPSettings,int _NewAPSettingsLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_RebootSTA) (void* upnptoken,unsigned char* NewSTASettings,int _NewSTASettingsLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_ResetAP) (void* upnptoken,unsigned char* NewMessage,int _NewMessageLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_ResetSTA) (void* upnptoken,unsigned char* NewMessage,int _NewMessageLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_SetAPSettings) (void* upnptoken,unsigned char* APSettings,int _APSettingsLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_SetSelectedRegistrar) (void* upnptoken,unsigned char* NewMessage,int _NewMessageLength);
typedef void(*UPnP_ActionHandler_WFAWLANConfig_SetSTASettings) (void* upnptoken);
/* UPnP Set Function Pointers Methods */
extern void (*UPnPFP_PresentationPage) (void* upnptoken,struct packetheader *packet);
extern UPnP_ActionHandler_WFAWLANConfig_DelAPSettings UPnPFP_WFAWLANConfig_DelAPSettings;
extern UPnP_ActionHandler_WFAWLANConfig_DelSTASettings UPnPFP_WFAWLANConfig_DelSTASettings;
extern UPnP_ActionHandler_WFAWLANConfig_GetAPSettings UPnPFP_WFAWLANConfig_GetAPSettings;
extern UPnP_ActionHandler_WFAWLANConfig_GetDeviceInfo UPnPFP_WFAWLANConfig_GetDeviceInfo;
extern UPnP_ActionHandler_WFAWLANConfig_GetSTASettings UPnPFP_WFAWLANConfig_GetSTASettings;
extern UPnP_ActionHandler_WFAWLANConfig_PutMessage UPnPFP_WFAWLANConfig_PutMessage;
extern UPnP_ActionHandler_WFAWLANConfig_PutWLANResponse UPnPFP_WFAWLANConfig_PutWLANResponse;
extern UPnP_ActionHandler_WFAWLANConfig_RebootAP UPnPFP_WFAWLANConfig_RebootAP;
extern UPnP_ActionHandler_WFAWLANConfig_RebootSTA UPnPFP_WFAWLANConfig_RebootSTA;
extern UPnP_ActionHandler_WFAWLANConfig_ResetAP UPnPFP_WFAWLANConfig_ResetAP;
extern UPnP_ActionHandler_WFAWLANConfig_ResetSTA UPnPFP_WFAWLANConfig_ResetSTA;
extern UPnP_ActionHandler_WFAWLANConfig_SetAPSettings UPnPFP_WFAWLANConfig_SetAPSettings;
extern UPnP_ActionHandler_WFAWLANConfig_SetSelectedRegistrar UPnPFP_WFAWLANConfig_SetSelectedRegistrar;
extern UPnP_ActionHandler_WFAWLANConfig_SetSTASettings UPnPFP_WFAWLANConfig_SetSTASettings;


void UPnPSetDisconnectFlag(UPnPSessionToken token,void *flag);

/* Invocation Response Methods */
void UPnPResponse_Error(const UPnPSessionToken UPnPToken, const int ErrorCode, const char* ErrorMsg);
void UPnPResponseGeneric(const UPnPSessionToken UPnPToken,const char* ServiceURI,const char* MethodName,const char* Params);
void UPnPResponse_WFAWLANConfig_DelAPSettings(const UPnPSessionToken UPnPToken);
void UPnPResponse_WFAWLANConfig_DelSTASettings(const UPnPSessionToken UPnPToken);
void UPnPResponse_WFAWLANConfig_GetAPSettings(const UPnPSessionToken UPnPToken, const unsigned char* NewAPSettings, const int _NewAPSettingsLength);
void UPnPResponse_WFAWLANConfig_GetDeviceInfo(const UPnPSessionToken UPnPToken, const unsigned char* NewDeviceInfo, const int _NewDeviceInfoLength);
void UPnPResponse_WFAWLANConfig_GetSTASettings(const UPnPSessionToken UPnPToken, const unsigned char* NewSTASettings, const int _NewSTASettingsLength);
void UPnPResponse_WFAWLANConfig_PutMessage(const UPnPSessionToken UPnPToken, const unsigned char* NewOutMessage, const int _NewOutMessageLength);
void UPnPResponse_WFAWLANConfig_PutWLANResponse(const UPnPSessionToken UPnPToken);
void UPnPResponse_WFAWLANConfig_RebootAP(const UPnPSessionToken UPnPToken);
void UPnPResponse_WFAWLANConfig_RebootSTA(const UPnPSessionToken UPnPToken);
void UPnPResponse_WFAWLANConfig_ResetAP(const UPnPSessionToken UPnPToken);
void UPnPResponse_WFAWLANConfig_ResetSTA(const UPnPSessionToken UPnPToken);
void UPnPResponse_WFAWLANConfig_SetAPSettings(const UPnPSessionToken UPnPToken);
void UPnPResponse_WFAWLANConfig_SetSelectedRegistrar(const UPnPSessionToken UPnPToken);
void UPnPResponse_WFAWLANConfig_SetSTASettings(const UPnPSessionToken UPnPToken, const unsigned char* NewSTASettings, const int _NewSTASettingsLength);

/* State Variable Eventing Methods */
void UPnPSetState_WFAWLANConfig_STAStatus(UPnPMicroStackToken microstack,unsigned char val);
void UPnPSetState_WFAWLANConfig_APStatus(UPnPMicroStackToken microstack,unsigned char val);
void UPnPSetState_WFAWLANConfig_WLANEvent(UPnPMicroStackToken microstack,unsigned char* val,int _WLANEventLength);

/* Extra functions */
void UPnP_MS_ILibWebServer_AddRef(void * upnptoken);
void UPnP_MS_ILibWebServer_Release(void * upnptoken);


extern int g_TotalUPnPEventSubscribers;

/*! \} */

#ifdef __cplusplus
}
#endif

#endif
