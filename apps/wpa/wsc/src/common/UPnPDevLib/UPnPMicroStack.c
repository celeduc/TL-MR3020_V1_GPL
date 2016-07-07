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
// $Workfile: UPnPMicroStack.c
// $Revision: #1.0.2126.26696
// $Author:   Intel Corporation, Intel Device Builder
// $Date:     Tuesday, January 17, 2006
*/


#if defined(WIN32) || defined(_WIN32_WCE)
#	ifndef MICROSTACK_NO_STDAFX
#		include "stdafx.h"
#	endif
static char* UPnPPLATFORM = "WINDOWS";
#else
static char* UPnPPLATFORM = "POSIX";
#endif

/*
#if defined(WIN32)
#define _CRTDBG_MAP_ALLOC
#endif
*/

#if defined(WINSOCK2)
#	include <winsock2.h>
#	include <ws2tcpip.h>
#elif defined(WINSOCK1)
#	include <winsock.h>
#	include <wininet.h>
#endif

#include "tutrace.h"

#include "ILibParsers.h"
#include "UPnPMicroStack.h"
#include "ILibWebServer.h"
#include "ILibWebClient.h"
#include "ILibAsyncSocket.h"
#include "ILibAsyncUDPSocket.h"

#if defined(WIN32) && !defined(_WIN32_WCE)
#include <crtdbg.h>
#endif

#define UPNP_SSDP_TTL 4
#define UPNP_HTTP_MAXSOCKETS 5
#define UPNP_MAX_SSDP_HEADER_SIZE 4096
#define UPNP_PORT 1900
#define UPNP_GROUP "239.255.255.250"
#define UPnP_MAX_SUBSCRIPTION_TIMEOUT 7200
#define UPnPMIN(a,b) (((a)<(b))?(a):(b))

#define LVL3DEBUG(x)

//{{{ObjectDefintions}}}
UPnPMicroStackToken UPnPCreateMicroStack(void *Chain, const char* FriendlyName,const char* UDN, const char* SerialNumber, const int NotifyCycleSeconds, const unsigned short PortNum);
/* UPnP Set Function Pointers Methods */
void (*UPnPFP_PresentationPage) (void* upnptoken,struct packetheader *packet);
/*! \var UPnPFP_WFAWLANConfig_DelAPSettings
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> DelAPSettings
*/
UPnP_ActionHandler_WFAWLANConfig_DelAPSettings UPnPFP_WFAWLANConfig_DelAPSettings;
/*! \var UPnPFP_WFAWLANConfig_DelSTASettings
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> DelSTASettings
*/
UPnP_ActionHandler_WFAWLANConfig_DelSTASettings UPnPFP_WFAWLANConfig_DelSTASettings;
/*! \var UPnPFP_WFAWLANConfig_GetAPSettings
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> GetAPSettings
*/
UPnP_ActionHandler_WFAWLANConfig_GetAPSettings UPnPFP_WFAWLANConfig_GetAPSettings;
/*! \var UPnPFP_WFAWLANConfig_GetDeviceInfo
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> GetDeviceInfo
*/
UPnP_ActionHandler_WFAWLANConfig_GetDeviceInfo UPnPFP_WFAWLANConfig_GetDeviceInfo;
/*! \var UPnPFP_WFAWLANConfig_GetSTASettings
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> GetSTASettings
*/
UPnP_ActionHandler_WFAWLANConfig_GetSTASettings UPnPFP_WFAWLANConfig_GetSTASettings;
/*! \var UPnPFP_WFAWLANConfig_PutMessage
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> PutMessage
*/
UPnP_ActionHandler_WFAWLANConfig_PutMessage UPnPFP_WFAWLANConfig_PutMessage;
/*! \var UPnPFP_WFAWLANConfig_PutWLANResponse
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> PutWLANResponse
*/
UPnP_ActionHandler_WFAWLANConfig_PutWLANResponse UPnPFP_WFAWLANConfig_PutWLANResponse;
/*! \var UPnPFP_WFAWLANConfig_RebootAP
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> RebootAP
*/
UPnP_ActionHandler_WFAWLANConfig_RebootAP UPnPFP_WFAWLANConfig_RebootAP;
/*! \var UPnPFP_WFAWLANConfig_RebootSTA
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> RebootSTA
*/
UPnP_ActionHandler_WFAWLANConfig_RebootSTA UPnPFP_WFAWLANConfig_RebootSTA;
/*! \var UPnPFP_WFAWLANConfig_ResetAP
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> ResetAP
*/
UPnP_ActionHandler_WFAWLANConfig_ResetAP UPnPFP_WFAWLANConfig_ResetAP;
/*! \var UPnPFP_WFAWLANConfig_ResetSTA
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> ResetSTA
*/
UPnP_ActionHandler_WFAWLANConfig_ResetSTA UPnPFP_WFAWLANConfig_ResetSTA;
/*! \var UPnPFP_WFAWLANConfig_SetAPSettings
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> SetAPSettings
*/
UPnP_ActionHandler_WFAWLANConfig_SetAPSettings UPnPFP_WFAWLANConfig_SetAPSettings;
/*! \var UPnPFP_WFAWLANConfig_SetSelectedRegistrar
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> SetSelectedRegistrar
*/
UPnP_ActionHandler_WFAWLANConfig_SetSelectedRegistrar UPnPFP_WFAWLANConfig_SetSelectedRegistrar;
/*! \var UPnPFP_WFAWLANConfig_SetSTASettings
\brief Dispatch Pointer for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> SetSTASettings
*/
UPnP_ActionHandler_WFAWLANConfig_SetSTASettings UPnPFP_WFAWLANConfig_SetSTASettings;


int g_TotalUPnPEventSubscribers = 0;

const int UPnPDeviceDescriptionTemplateLengthUX = 913;
const int UPnPDeviceDescriptionTemplateLength = 539;
const char UPnPDeviceDescriptionTemplate[539]={
	0x5A,0x3C,0x3F,0x78,0x6D,0x6C,0x20,0x76,0x65,0x72,0x73,0x69,0x6F,0x6E,0x3D,0x22,0x31,0x2E,0x30,0x22
	,0x20,0x65,0x6E,0x63,0x6F,0x64,0x69,0x6E,0x67,0x3D,0x22,0x75,0x74,0x66,0x2D,0x38,0x22,0x3F,0x3E,0x3C
	,0x72,0x6F,0x6F,0x74,0x20,0x78,0x6D,0x6C,0x6E,0x73,0x3D,0x22,0x75,0x72,0x6E,0x3A,0x73,0x63,0x68,0x65
	,0x6D,0x61,0x73,0x2D,0x75,0x70,0x6E,0x70,0x2D,0x6F,0x72,0x67,0x3A,0x64,0x65,0x76,0x69,0x63,0x65,0x2D
	,0x31,0x2D,0x30,0x22,0x3E,0x3C,0x73,0x70,0x65,0x63,0x56,0xC6,0x14,0x0B,0x3E,0x3C,0x6D,0x61,0x6A,0x6F
	,0x72,0x3E,0x31,0x3C,0x2F,0x46,0x02,0x0A,0x3C,0x6D,0x69,0x6E,0x6F,0x72,0x3E,0x30,0x3C,0x2F,0x46,0x02
	,0x02,0x3C,0x2F,0x8D,0x0B,0x00,0x06,0x12,0x00,0x08,0x02,0x05,0x54,0x79,0x70,0x65,0x3E,0x0C,0x1C,0x0C
	,0x77,0x69,0x66,0x69,0x61,0x6C,0x6C,0x69,0x61,0x6E,0x63,0x65,0x0B,0x1E,0x05,0x3A,0x57,0x46,0x41,0x44
	,0x86,0x02,0x03,0x31,0x3C,0x2F,0x0B,0x0F,0x12,0x3C,0x66,0x72,0x69,0x65,0x6E,0x64,0x6C,0x79,0x4E,0x61
	,0x6D,0x65,0x3E,0x25,0x73,0x3C,0x2F,0x4D,0x04,0x21,0x3C,0x6D,0x61,0x6E,0x75,0x66,0x61,0x63,0x74,0x75
	,0x72,0x65,0x72,0x3E,0x49,0x6E,0x74,0x65,0x6C,0x20,0x43,0x6F,0x72,0x70,0x6F,0x72,0x61,0x74,0x69,0x6F
	,0x6E,0x3C,0x2F,0x0D,0x08,0x00,0x8D,0x0B,0x10,0x55,0x52,0x4C,0x3E,0x68,0x74,0x74,0x70,0x3A,0x2F,0x2F
	,0x77,0x77,0x77,0x2E,0x69,0x04,0x0F,0x06,0x2E,0x63,0x6F,0x6D,0x3C,0x2F,0x90,0x09,0x0D,0x3C,0x6D,0x6F
	,0x64,0x65,0x6C,0x44,0x65,0x73,0x63,0x72,0x69,0x70,0xC4,0x15,0x0D,0x3E,0x53,0x61,0x6D,0x70,0x6C,0x65
	,0x20,0x55,0x50,0x6E,0x50,0x20,0xC6,0x2D,0x15,0x20,0x55,0x73,0x69,0x6E,0x67,0x20,0x41,0x75,0x74,0x6F
	,0x2D,0x47,0x65,0x6E,0x65,0x72,0x61,0x74,0x65,0x64,0x46,0x08,0x07,0x53,0x74,0x61,0x63,0x6B,0x3C,0x2F
	,0x51,0x11,0x00,0xC6,0x15,0x00,0x05,0x36,0x00,0x07,0x14,0x00,0x8F,0x0F,0x00,0x46,0x44,0x02,0x3C,0x2F
	,0x0A,0x0A,0x00,0xC7,0x0C,0x0A,0x75,0x6D,0x62,0x65,0x72,0x3E,0x58,0x31,0x3C,0x2F,0x0C,0x04,0x06,0x3C
	,0x73,0x65,0x72,0x69,0x61,0x88,0x07,0x00,0xC4,0x4A,0x00,0x4D,0x04,0x0A,0x3C,0x55,0x44,0x4E,0x3E,0x75
	,0x75,0x69,0x64,0x3A,0x84,0x51,0x03,0x55,0x44,0x4E,0x45,0x0C,0x00,0x44,0x7D,0x04,0x4C,0x69,0x73,0x74
	,0x49,0x03,0x00,0x89,0x05,0x00,0xE2,0x6E,0x03,0x73,0x65,0x72,0x08,0x6F,0x0C,0x57,0x4C,0x41,0x4E,0x43
	,0x6F,0x6E,0x66,0x69,0x67,0x3A,0x31,0x05,0x1E,0x00,0x4A,0x70,0x00,0x47,0x19,0x02,0x49,0x64,0x05,0x82
	,0x00,0x58,0x11,0x02,0x49,0x64,0xCE,0x11,0x00,0x8A,0x11,0x08,0x49,0x64,0x3E,0x3C,0x53,0x43,0x50,0x44
	,0x04,0x6C,0x00,0x8D,0x1A,0x0B,0x2F,0x73,0x63,0x70,0x64,0x2E,0x78,0x6D,0x6C,0x3C,0x2F,0x08,0x08,0x08
	,0x3C,0x63,0x6F,0x6E,0x74,0x72,0x6F,0x6C,0x12,0x0B,0x00,0x47,0x06,0x02,0x3C,0x2F,0x8B,0x08,0x09,0x3C
	,0x65,0x76,0x65,0x6E,0x74,0x53,0x75,0x62,0xD2,0x16,0x00,0x85,0x06,0x02,0x3C,0x2F,0x4C,0x08,0x00,0xC9
	,0x35,0x03,0x3E,0x3C,0x2F,0x4D,0x4E,0x01,0x2F,0xC8,0xBC,0x01,0x2F,0x44,0xD9,0x01,0x3E,0x00,0x00};
/* WFAWLANConfig */
const int UPnPWFAWLANConfigDescriptionLengthUX = 4932;
const int UPnPWFAWLANConfigDescriptionLength = 953;
const char UPnPWFAWLANConfigDescription[953] = {
	0x4E,0x48,0x54,0x54,0x50,0x2F,0x31,0x2E,0x31,0x20,0x32,0x30,0x30,0x20,0x20,0x4F,0x4B,0x0D,0x0A,0x43
	,0x4F,0x4E,0x54,0x45,0x4E,0x54,0x2D,0x54,0x59,0x50,0x45,0x3A,0x20,0x20,0x74,0x65,0x78,0x74,0x2F,0x78
	,0x6D,0x6C,0x3B,0x20,0x63,0x68,0x61,0x72,0x73,0x65,0x74,0x3D,0x22,0x75,0x74,0x66,0x2D,0x38,0x22,0x0D
	,0x0A,0x53,0x65,0x72,0x76,0x65,0x72,0x3A,0x20,0x50,0x4F,0x53,0x49,0x58,0x2C,0x20,0x55,0x50,0x6E,0xC4
	,0x12,0x13,0x30,0x2C,0x20,0x49,0x6E,0x74,0x65,0x6C,0x20,0x4D,0x69,0x63,0x72,0x6F,0x53,0x74,0x61,0x63
	,0x6B,0x84,0x05,0x3B,0x2E,0x32,0x31,0x32,0x36,0x0D,0x0A,0x43,0x6F,0x6E,0x74,0x65,0x6E,0x74,0x2D,0x4C
	,0x65,0x6E,0x67,0x74,0x68,0x3A,0x20,0x34,0x37,0x39,0x36,0x0D,0x0A,0x0D,0x0A,0x3C,0x3F,0x78,0x6D,0x6C
	,0x20,0x76,0x65,0x72,0x73,0x69,0x6F,0x6E,0x3D,0x22,0x31,0x2E,0x30,0x22,0x20,0x65,0x6E,0x63,0x6F,0x64
	,0x69,0x6E,0x67,0x88,0x1C,0x37,0x3F,0x3E,0x3C,0x73,0x63,0x70,0x64,0x20,0x78,0x6D,0x6C,0x6E,0x73,0x3D
	,0x22,0x75,0x72,0x6E,0x3A,0x73,0x63,0x68,0x65,0x6D,0x61,0x73,0x2D,0x75,0x70,0x6E,0x70,0x2D,0x6F,0x72
	,0x67,0x3A,0x73,0x65,0x72,0x76,0x69,0x63,0x65,0x2D,0x31,0x2D,0x30,0x22,0x3E,0x3C,0x73,0x70,0x65,0x63
	,0x56,0x06,0x15,0x0B,0x3E,0x3C,0x6D,0x61,0x6A,0x6F,0x72,0x3E,0x31,0x3C,0x2F,0x46,0x02,0x0A,0x3C,0x6D
	,0x69,0x6E,0x6F,0x72,0x3E,0x30,0x3C,0x2F,0x46,0x02,0x02,0x3C,0x2F,0x8D,0x0B,0x0A,0x61,0x63,0x74,0x69
	,0x6F,0x6E,0x4C,0x69,0x73,0x74,0x08,0x03,0x16,0x3E,0x3C,0x6E,0x61,0x6D,0x65,0x3E,0x44,0x65,0x6C,0x41
	,0x50,0x53,0x65,0x74,0x74,0x69,0x6E,0x67,0x73,0x3C,0x2F,0x05,0x05,0x09,0x3C,0x61,0x72,0x67,0x75,0x6D
	,0x65,0x6E,0x74,0x07,0x0C,0x00,0x87,0x03,0x00,0x87,0x0C,0x03,0x4E,0x65,0x77,0x92,0x0C,0x04,0x64,0x69
	,0x72,0x65,0xC6,0x15,0x04,0x69,0x6E,0x3C,0x2F,0x8A,0x03,0x16,0x3C,0x72,0x65,0x6C,0x61,0x74,0x65,0x64
	,0x53,0x74,0x61,0x74,0x65,0x56,0x61,0x72,0x69,0x61,0x62,0x6C,0x65,0x3E,0x8C,0x1C,0x00,0x55,0x08,0x02
	,0x3C,0x2F,0x4A,0x1D,0x01,0x2F,0x8E,0x23,0x04,0x2F,0x61,0x63,0x74,0xCB,0x34,0x00,0xCA,0x31,0x03,0x53
	,0x54,0x41,0x30,0x32,0x03,0x53,0x54,0x41,0x7E,0x32,0x03,0x53,0x54,0x41,0xBF,0x32,0x00,0x91,0x32,0x03
	,0x47,0x65,0x74,0x72,0x64,0x07,0x4D,0x65,0x73,0x73,0x61,0x67,0x65,0xB6,0x63,0x00,0x49,0x0F,0x00,0xE1
	,0x62,0x00,0xAE,0x82,0x03,0x6F,0x75,0x74,0xFF,0x82,0x00,0x78,0x50,0x02,0x44,0x65,0x84,0xCD,0x04,0x49
	,0x6E,0x66,0x6F,0xA8,0xB4,0x00,0x92,0x0C,0x00,0x2F,0x32,0x00,0xCC,0x1C,0x00,0x7F,0x82,0x00,0x4A,0x82
	,0x03,0x53,0x54,0x41,0xBF,0x82,0x00,0xBF,0x82,0x00,0xAB,0x82,0x03,0x53,0x54,0x41,0xFF,0x82,0x00,0x7F
	,0xD3,0x00,0x54,0xD3,0x03,0x50,0x75,0x74,0xCF,0xC6,0x00,0xA0,0xD2,0x02,0x49,0x6E,0x3D,0xD3,0x02,0x49
	,0x6E,0xBC,0xD3,0x03,0x4F,0x75,0x74,0x99,0xF2,0x00,0xA5,0xD3,0x03,0x4F,0x75,0x74,0xAA,0xF3,0x00,0xA8
	,0x50,0x0C,0x57,0x4C,0x41,0x4E,0x52,0x65,0x73,0x70,0x6F,0x6E,0x73,0x65,0xFF,0xA1,0x00,0xFF,0xA1,0x00
	,0xE3,0xA1,0x00,0x44,0x2B,0x08,0x45,0x76,0x65,0x6E,0x74,0x54,0x79,0x70,0xB7,0xC1,0x00,0xCF,0x10,0x00
	,0x7C,0x21,0x03,0x4D,0x41,0x43,0x3F,0x21,0x03,0x4D,0x41,0x43,0x7F,0xC3,0x00,0x49,0xC3,0x08,0x52,0x65
	,0x62,0x6F,0x6F,0x74,0x41,0x50,0xE8,0xC2,0x02,0x41,0x50,0x5A,0xF4,0x00,0x24,0xC3,0x02,0x41,0x50,0xFF
	,0xF3,0x00,0x97,0x30,0x03,0x53,0x54,0x41,0xA8,0xF3,0x03,0x53,0x54,0x41,0x3F,0x31,0x00,0x3F,0x31,0x00
	,0x94,0x61,0x05,0x73,0x65,0x74,0x41,0x50,0x7F,0xD2,0x00,0x7F,0xD2,0x00,0xFB,0x2E,0x03,0x53,0x54,0x41
	,0x3F,0x2F,0x00,0x3F,0x2F,0x00,0x76,0xBF,0x03,0x53,0x65,0x74,0x12,0xB4,0x00,0x9D,0xC0,0x00,0xFF,0xBF
	,0x00,0xFF,0xBF,0x00,0x17,0x31,0x11,0x53,0x65,0x6C,0x65,0x63,0x74,0x65,0x64,0x52,0x65,0x67,0x69,0x73
	,0x74,0x72,0x61,0x72,0x3F,0x63,0x00,0x3F,0x63,0x00,0x3A,0x32,0x02,0x54,0x41,0x6D,0x63,0x00,0x20,0xF3
	,0x03,0x6F,0x75,0x74,0x62,0xF3,0x03,0x53,0x54,0x41,0xBF,0xF3,0x02,0x6F,0x6E,0xC9,0xF5,0x00,0x06,0xF9
	,0x07,0x73,0x65,0x72,0x76,0x69,0x63,0x65,0xC5,0xDD,0x01,0x54,0x86,0xD5,0x01,0x73,0xCC,0xE0,0x10,0x20
	,0x73,0x65,0x6E,0x64,0x45,0x76,0x65,0x6E,0x74,0x73,0x3D,0x22,0x6E,0x6F,0x22,0x47,0xF6,0x0B,0x57,0x4C
	,0x41,0x4E,0x52,0x65,0x73,0x70,0x6F,0x6E,0x73,0xCA,0xF6,0x14,0x61,0x74,0x61,0x54,0x79,0x70,0x65,0x3E
	,0x62,0x69,0x6E,0x2E,0x62,0x61,0x73,0x65,0x36,0x34,0x3C,0x2F,0x49,0x05,0x03,0x3C,0x2F,0x73,0x4E,0xEF
	,0x00,0xE8,0x19,0x00,0x45,0x1F,0x03,0x54,0x79,0x70,0x12,0x1A,0x03,0x75,0x69,0x31,0x7F,0x18,0x03,0x3E
	,0x49,0x6E,0x10,0xF9,0x00,0x7F,0x31,0x00,0x13,0x4B,0x03,0x4F,0x75,0x74,0x7F,0x19,0x00,0x63,0x64,0x00
	,0x53,0xFB,0x00,0xFF,0x63,0x00,0x93,0x7D,0x00,0x3F,0x4B,0x00,0x19,0x96,0x03,0x79,0x65,0x73,0x48,0x96
	,0x00,0x04,0xD6,0x05,0x74,0x61,0x74,0x75,0x73,0xBF,0x7B,0x00,0xDE,0x93,0x03,0x4D,0x41,0x43,0x91,0xAD
	,0x06,0x73,0x74,0x72,0x69,0x6E,0x67,0xBF,0xAC,0x03,0x3E,0x44,0x65,0xC4,0xD3,0x04,0x49,0x6E,0x66,0x6F
	,0xFF,0xC5,0x00,0x9C,0xDF,0x03,0x53,0x54,0x41,0xBF,0x7B,0x00,0x25,0x63,0x02,0x41,0x50,0xFF,0x62,0x00
	,0x5C,0x7A,0x00,0xC9,0xF6,0x00,0xB7,0xDE,0x04,0x2F,0x73,0x65,0x72,0xC4,0x5A,0x00,0x84,0x8B,0x02,0x65
	,0x54,0x08,0x05,0x01,0x63,0x00,0x00,0x03,0x70,0x64,0x3E,0x00,0x00};





struct UPnPDataObject;

//
// It should not be necessary to expose/modify any of these structures. They
// are used by the internal stack
//

struct SubscriberInfo
{
	char* SID;		// Subscription ID
	int SIDLength;
	int SEQ;


	int Address;
	unsigned short Port;
	char* Path;
	int PathLength;
	int RefCount;
	int Disposing;

	struct timeval RenewByTime;

	struct SubscriberInfo *Next;
	struct SubscriberInfo *Previous;
};
struct UPnPDataObject
{
	//
	// Absolutely DO NOT put anything above these 3 function pointers
	//
	ILibChain_PreSelect PreSelect;
	ILibChain_PostSelect PostSelect;
	ILibChain_Destroy Destroy;

	void *EventClient;
	void *Chain;
	int UpdateFlag;

	/* Network Poll */
	unsigned int NetworkPollTime;

	int ForceExit;
	char *UUID;
	char *UDN;
	char *Serial;
	void *User;

	void *WebServerTimer;
	void *HTTPServer;
	char *DeviceDescription;
	int DeviceDescriptionLength;
	int InitialNotify;

	char* WFAWLANConfig_STAStatus;
	char* WFAWLANConfig_APStatus;
	char* WFAWLANConfig_WLANEvent;


	struct sockaddr_in addr;
	int addrlen;

	struct ip_mreq mreq;
	int *AddressList;
	int AddressListLength;

	int _NumEmbeddedDevices;
	int WebSocketPortNumber;

	void *NOTIFY_RECEIVE_sock;
	void **NOTIFY_SEND_socks;



	struct timeval CurrentTime;
	struct timeval NotifyTime;

	int SID;
	int NotifyCycleTime;


	sem_t EventLock;
	struct SubscriberInfo *HeadSubscriberPtr_WFAWLANConfig;
	int NumberOfSubscribers_WFAWLANConfig;

};

struct MSEARCH_state
{
	char *ST;
	int STLength;
	void *upnp;
	struct sockaddr_in dest_addr;
};
struct UPnPFragmentNotifyStruct
{
	struct UPnPDataObject *upnp;
	int packetNumber;
};

/* Pre-declarations */

void UPnPFragmentedSendNotify(void *data);
void UPnPSendNotify(const struct UPnPDataObject *upnp);
void UPnPSendByeBye(const struct UPnPDataObject *upnp);
void UPnPMainInvokeSwitch();
void UPnPSendDataXmlEscaped(const void* UPnPToken, const char* Data, const int DataLength, const int Terminate);
void UPnPSendData(const void* UPnPToken, const char* Data, const int DataLength, const int Terminate);
int UPnPPeriodicNotify(struct UPnPDataObject *upnp);
void UPnPSendEvent_Body(void *upnptoken, char *body, int bodylength, struct SubscriberInfo *info);
void UPnPProcessMSEARCH(struct UPnPDataObject *upnp, struct packetheader *packet);
struct in_addr UPnP_inaddr;

/*! \fn UPnPGetWebServerToken(const UPnPMicroStackToken MicroStackToken)
\brief Converts a MicroStackToken to a WebServerToken
\par
\a MicroStackToken is the void* returned from a call to UPnPCreateMicroStack. The returned token, is the server token
not the session token.
\param MicroStackToken MicroStack Token
\returns WebServer Token
*/
void* UPnPGetWebServerToken(const UPnPMicroStackToken MicroStackToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPGetWebServerToken\n"));
	return(((struct UPnPDataObject*)MicroStackToken)->HTTPServer);
}

#if 1 // muyz, add path
#define UPnPBuildSsdpResponsePacket(outpacket,outlength,ipaddr,port,EmbeddedDeviceNumber,USN,USNex,ST,NTex,NotifyTime)\
{\
	UPnP_inaddr.s_addr = ipaddr;\
	*outlength = sprintf(outpacket,"HTTP/1.1 200 OK\r\nLOCATION: http://%s:%d/WFAWLANConfig/scpd.xml\r\nEXT:\r\nSERVER: %s, UPnP/1.0, Intel MicroStack/1.0.2126\r\nUSN: uuid:%s%s\r\nCACHE-CONTROL: max-age=%d\r\nST: %s%s\r\n\r\n" ,inet_ntoa(UPnP_inaddr),port,UPnPPLATFORM,USN,USNex,NotifyTime,ST,NTex);\
}
#define UPnPBuildSsdpNotifyPacket(outpacket,outlength,ipaddr,port,EmbeddedDeviceNumber,USN,USNex,NT,NTex,NotifyTime)\
{\
	UPnP_inaddr.s_addr = ipaddr;\
	*outlength = sprintf(outpacket,"NOTIFY * HTTP/1.1\r\nLOCATION: http://%s:%d/WFAWLANConfig/scpd.xml\r\nHOST: 239.255.255.250:1900\r\nSERVER: %s, UPnP/1.0, Intel MicroStack/1.0.2126\r\nNTS: ssdp:alive\r\nUSN: uuid:%s%s\r\nCACHE-CONTROL: max-age=%d\r\nNT: %s%s\r\n\r\n",inet_ntoa(UPnP_inaddr),port,UPnPPLATFORM,USN,USNex,NotifyTime,NT,NTex);\
}
#else
#define UPnPBuildSsdpResponsePacket(outpacket,outlength,ipaddr,port,EmbeddedDeviceNumber,USN,USNex,ST,NTex,NotifyTime)\
{\
	UPnP_inaddr.s_addr = ipaddr;\
	*outlength = sprintf(outpacket,"HTTP/1.1 200 OK\r\nLOCATION: http://%s:%d/\r\nEXT:\r\nSERVER: %s, UPnP/1.0, Intel MicroStack/1.0.2126\r\nUSN: uuid:%s%s\r\nCACHE-CONTROL: max-age=%d\r\nST: %s%s\r\n\r\n" ,inet_ntoa(UPnP_inaddr),port,UPnPPLATFORM,USN,USNex,NotifyTime,ST,NTex);\
}
#define UPnPBuildSsdpNotifyPacket(outpacket,outlength,ipaddr,port,EmbeddedDeviceNumber,USN,USNex,NT,NTex,NotifyTime)\
{\
	UPnP_inaddr.s_addr = ipaddr;\
	*outlength = sprintf(outpacket,"NOTIFY * HTTP/1.1\r\nLOCATION: http://%s:%d/\r\nHOST: 239.255.255.250:1900\r\nSERVER: %s, UPnP/1.0, Intel MicroStack/1.0.2126\r\nNTS: ssdp:alive\r\nUSN: uuid:%s%s\r\nCACHE-CONTROL: max-age=%d\r\nNT: %s%s\r\n\r\n",inet_ntoa(UPnP_inaddr),port,UPnPPLATFORM,USN,USNex,NotifyTime,NT,NTex);\
}
#endif



void UPnPSetDisconnectFlag(UPnPSessionToken token,void *flag)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPSetDisconnectFlag\n"));
	((struct ILibWebServer_Session*)token)->Reserved10=flag;
}


/*! \fn UPnPIPAddressListChanged(UPnPMicroStackToken MicroStackToken)
\brief Tell the underlying MicroStack that an IPAddress may have changed
\param MicroStackToken Microstack
*/
void UPnPIPAddressListChanged(UPnPMicroStackToken MicroStackToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPIPAddressListChanged\n"));

	((struct UPnPDataObject*)MicroStackToken)->UpdateFlag = 1;
	ILibForceUnBlockChain(((struct UPnPDataObject*)MicroStackToken)->Chain);
}


void UPnPSSDPSink(ILibAsyncUDPSocket_SocketModule socketModule,char* buffer, int bufferLength, int remoteInterface, unsigned short remotePort, void *user, void *user2, int *PAUSE)
{
	struct packetheader *packet;
	struct sockaddr_in addr;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPSSDPSink\n"));

	memset(&addr,0,sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = remoteInterface;
	addr.sin_port = htons(remotePort);

	packet = ILibParsePacketHeader(buffer,0,bufferLength);
	if(packet!=NULL)
	{
		packet->Source = &addr;
		packet->ReceivingAddress = 0;
		if(packet->StatusCode==-1 && memcmp(packet->Directive,"M-SEARCH",8)==0)
		{
			//
			// Process the search request with our Multicast M-SEARCH Handler
			//
#if 0	// muyz, ignore it
			UPnPProcessMSEARCH(user, packet);
#endif
		}
		ILibDestructPacket(packet);
	}
}


//
//	Internal underlying Initialization, that shouldn't be called explicitely
//
// <param name="state">State object</param>
// <param name="NotifyCycleSeconds">Cycle duration</param>
// <param name="PortNumber">Port Number, is NOT used in UPnPInit right now.</param>
// This function initilize state, enumerate br0 and eth0 if's ip addr, create async recv udp socket
// bind on local UPnP Port 1900, create notify send async udp socket bind on <br0 ip, 1900> and <eth0 ip, 1900>.
// If eth0 ip is not set, then only br0 sending socket is create and bound.
// Finally, add recv udp socket and all notify sending sockets to UPnP multicast group address
void UPnPInit(struct UPnPDataObject *state, void *chain, const int NotifyCycleSeconds,const unsigned short PortNumber)
{
	int i;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPInit\n"));

	state->Chain = chain;

	/* Setup Notification Timer */
	state->NotifyCycleTime = NotifyCycleSeconds;


	gettimeofday(&(state->CurrentTime),NULL);
	(state->NotifyTime).tv_sec = (state->CurrentTime).tv_sec  + (state->NotifyCycleTime/2);

	memset((char *)&(state->addr), 0, sizeof(state->addr));
	state->addr.sin_family = AF_INET;
	state->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// Here set the UPNP_PORT, 1900
	state->addr.sin_port = (unsigned short)htons(UPNP_PORT);
	state->addrlen = sizeof(state->addr);


	/* Set up socket */
	state->AddressListLength = ILibGetLocalIPAddressList(&(state->AddressList));
	state->NOTIFY_SEND_socks = (void**)malloc(sizeof(void*)*(state->AddressListLength));

	state->NOTIFY_RECEIVE_sock = ILibAsyncUDPSocket_Create(
	state->Chain,
	UPNP_MAX_SSDP_HEADER_SIZE,
	INADDR_ANY,
	UPNP_PORT,
	ILibAsyncUDPSocket_Reuse_SHARED,	// shared access, = 1
	&UPnPSSDPSink,
	NULL,
	state);

	//
	// Iterate through all the current IP Addresses
	//
	for(i=0;i<state->AddressListLength;++i)
	{
printf("Micro Stack: ip[%d] = %08x\n", i, state->AddressList[i]);
		state->NOTIFY_SEND_socks[i] = ILibAsyncUDPSocket_Create(
		chain,
		UPNP_MAX_SSDP_HEADER_SIZE,
		state->AddressList[i],
		UPNP_PORT,
		ILibAsyncUDPSocket_Reuse_SHARED,
		NULL,
		NULL,
		state);
		ILibAsyncUDPSocket_JoinMulticastGroup(
		state->NOTIFY_SEND_socks[i],
		state->AddressList[i],
		inet_addr(UPNP_GROUP));

		//This is required by UPnP Device Architecture Spec.
		ILibAsyncUDPSocket_SetMulticastTTL(state->NOTIFY_SEND_socks[i],UPNP_SSDP_TTL);

		ILibAsyncUDPSocket_JoinMulticastGroup(
		state->NOTIFY_RECEIVE_sock,
		state->AddressList[i],
		inet_addr(UPNP_GROUP));
	}

}
void UPnPPostMX_Destroy(void *object)
{
	struct MSEARCH_state *mss = (struct MSEARCH_state*)object;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPPostMX_Destroy\n"));
	free(mss->ST);
	free(mss);
}

void UPnPPostMX_MSEARCH(void *object)
{
	struct MSEARCH_state *mss = (struct MSEARCH_state*)object;

	char *b = (char*)malloc(sizeof(char)*5000);
	int packetlength;
	void **response_socket;
	void *subChain;

	int i;
	char *ST = mss->ST;
	int STLength = mss->STLength;
	struct UPnPDataObject *upnp = (struct UPnPDataObject*)mss->upnp;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPPostMX_MSEARCH\n"));

	response_socket = (void**)malloc(upnp->AddressListLength*sizeof(void*));
	subChain = ILibCreateChain();

	//
	// Iterate through all the current IP Addresses
	//
	for(i=0;i<upnp->AddressListLength;++i)
	{
		//
		// Create a socket to respond with
		//
		response_socket[i] = ILibAsyncUDPSocket_Create(
		subChain,
		UPNP_MAX_SSDP_HEADER_SIZE,
		upnp->AddressList[i],
		0,
		ILibAsyncUDPSocket_Reuse_SHARED,
		NULL,
		NULL,
		subChain);
	}

	//
	// Search for root device
	//
	if(STLength==15 && memcmp(ST,"upnp:rootdevice",15)==0)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{

			UPnPBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::upnp:rootdevice","upnp:rootdevice","",upnp->NotifyCycleTime);


			ILibAsyncUDPSocket_SendTo(response_socket[i],mss->dest_addr.sin_addr.s_addr, ntohs(mss->dest_addr.sin_port), b, packetlength, ILibAsyncSocket_MemoryOwnership_USER);
		}
	}
	//
	// Search for everything
	//
	else if(STLength==8 && memcmp(ST,"ssdp:all",8)==0)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UPnPBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::upnp:rootdevice","upnp:rootdevice","",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(response_socket[i],mss->dest_addr.sin_addr.s_addr, ntohs(mss->dest_addr.sin_port), b, packetlength, ILibAsyncSocket_MemoryOwnership_USER);
			UPnPBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"",upnp->UUID,"",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(response_socket[i],mss->dest_addr.sin_addr.s_addr, ntohs(mss->dest_addr.sin_port), b, packetlength, ILibAsyncSocket_MemoryOwnership_USER);
			UPnPBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-wifialliance-org:device:WFADevice:1","urn:schemas-wifialliance-org:device:WFADevice:1","",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(response_socket[i],mss->dest_addr.sin_addr.s_addr, ntohs(mss->dest_addr.sin_port), b, packetlength, ILibAsyncSocket_MemoryOwnership_USER);
			UPnPBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-wifialliance-org:service:WFAWLANConfig:1","urn:schemas-wifialliance-org:service:WFAWLANConfig:1","",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(response_socket[i],mss->dest_addr.sin_addr.s_addr, ntohs(mss->dest_addr.sin_port), b, packetlength, ILibAsyncSocket_MemoryOwnership_USER);
		}

	}
	if(STLength==(int)strlen(upnp->UUID) && memcmp(ST,upnp->UUID,(int)strlen(upnp->UUID))==0)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UPnPBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"",upnp->UUID,"",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(response_socket[i],mss->dest_addr.sin_addr.s_addr, ntohs(mss->dest_addr.sin_port), b, packetlength, ILibAsyncSocket_MemoryOwnership_USER);
		}
	}
	if(STLength>=46 && memcmp(ST,"urn:schemas-wifialliance-org:device:WFADevice:",46)==0 && atoi(ST+46)<=1)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UPnPBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-wifialliance-org:device:WFADevice:1",ST,"",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(response_socket[i],mss->dest_addr.sin_addr.s_addr, ntohs(mss->dest_addr.sin_port), b, packetlength, ILibAsyncSocket_MemoryOwnership_USER);
		}
	}
	if(STLength>=51 && memcmp(ST,"urn:schemas-wifialliance-org:service:WFAWLANConfig:",51)==0 && atoi(ST+51)<=1)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UPnPBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-wifialliance-org:service:WFAWLANConfig:1",ST,"",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(response_socket[i],mss->dest_addr.sin_addr.s_addr, ntohs(mss->dest_addr.sin_port), b, packetlength, ILibAsyncSocket_MemoryOwnership_USER);
		}
	}


	ILibChain_DestroyEx(subChain);

	free(response_socket);
	free(mss->ST);
	free(mss);
	free(b);
}
void UPnPProcessMSEARCH(struct UPnPDataObject *upnp, struct packetheader *packet)
{
	char* ST = NULL;
	int STLength = 0;
	struct packetheader_field_node *node;
	int MANOK = 0;
	unsigned long MXVal;
	int MXOK = 0;
	int MX;
	struct MSEARCH_state *mss = NULL;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPProcessMSEARCH\n"));

	if(memcmp(packet->DirectiveObj,"*",1)==0)
	{
		if(memcmp(packet->Version,"1.1",3)==0)
		{
			node = packet->FirstField;
			while(node!=NULL)
			{
				if(node->FieldLength==2 && strncasecmp(node->Field,"ST",2)==0)
				{
					//
					// This is what is being searched for
					//
					ST = (char*)malloc(1+node->FieldDataLength);
					memcpy(ST,node->FieldData,node->FieldDataLength);
					ST[node->FieldDataLength] = 0;
					STLength = node->FieldDataLength;
				}
				else if(node->FieldLength==3 && strncasecmp(node->Field,"MAN",3)==0 && memcmp(node->FieldData,"\"ssdp:discover\"",15)==0)
				{
					//
					// This is a required header field
					//
					MANOK = 1;
				}
				else if(node->FieldLength==2 && strncasecmp(node->Field,"MX",2)==0 && ILibGetULong(node->FieldData,node->FieldDataLength,&MXVal)==0)
				{
					//
					// If the timeout value specified is greater than 10 seconds, just force it
					// down to 10 seconds
					//
					MXOK = 1;
					MXVal = MXVal>10?10:MXVal;
				}
				node = node->NextField;
			}
			if(MANOK!=0 && MXOK!=0)
			{
				if(MXVal==0)
				{
					MX = 0;
				}
				else
				{
					//
					// The timeout value should be a random number between 0 and the
					// specified value
					//
					MX = (int)(0 + ((unsigned short)rand() % MXVal));
				}
				mss = (struct MSEARCH_state*)malloc(sizeof(struct MSEARCH_state));
				mss->ST = ST;
				mss->STLength = STLength;
				mss->upnp = upnp;
				memset((char *)&(mss->dest_addr), 0, sizeof(mss->dest_addr));
				mss->dest_addr.sin_family = AF_INET;
				mss->dest_addr.sin_addr = packet->Source->sin_addr;
				mss->dest_addr.sin_port = packet->Source->sin_port;

				//
				// Register for a timed callback, so we can respond later
				//
				ILibLifeTime_Add(upnp->WebServerTimer,mss,MX,&UPnPPostMX_MSEARCH,&UPnPPostMX_Destroy);
			}
			else
			{
				free(ST);
			}
		}
	}
}
void UPnPDispatch_WFAWLANConfig_DelAPSettings(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewAPSettings = NULL;
	int p_NewAPSettingsLength = 0;
	unsigned char* _NewAPSettings = NULL;
	int _NewAPSettingsLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_DelAPSettings\n"));

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==13 && memcmp(xnode->Name,"DelAPSettings",13)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==13 && memcmp(xnode->Name,"NewAPSettings",13)==0)
								{
									p_NewAPSettingsLength = ILibReadInnerXML(xnode,&p_NewAPSettings);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewAPSettingsLength = ILibBase64Decode(p_NewAPSettings,p_NewAPSettingsLength,&_NewAPSettings);
	if(UPnPFP_WFAWLANConfig_DelAPSettings == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_DelAPSettings((void*)ReaderObject,_NewAPSettings,_NewAPSettingsLength);
	free(_NewAPSettings);
}

void UPnPDispatch_WFAWLANConfig_DelSTASettings(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewSTASettings = NULL;
	int p_NewSTASettingsLength = 0;
	unsigned char* _NewSTASettings = NULL;
	int _NewSTASettingsLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_DelSTASettings\n"));

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==14 && memcmp(xnode->Name,"DelSTASettings",14)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==14 && memcmp(xnode->Name,"NewSTASettings",14)==0)
								{
									p_NewSTASettingsLength = ILibReadInnerXML(xnode,&p_NewSTASettings);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewSTASettingsLength = ILibBase64Decode(p_NewSTASettings,p_NewSTASettingsLength,&_NewSTASettings);
	if(UPnPFP_WFAWLANConfig_DelSTASettings == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_DelSTASettings((void*)ReaderObject,_NewSTASettings,_NewSTASettingsLength);
	free(_NewSTASettings);
}

void UPnPDispatch_WFAWLANConfig_GetAPSettings(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewMessage = NULL;
	int p_NewMessageLength = 0;
	unsigned char* _NewMessage = NULL;
	int _NewMessageLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;


        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_GetAPSettings\n"));

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==13 && memcmp(xnode->Name,"GetAPSettings",13)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==10 && memcmp(xnode->Name,"NewMessage",10)==0)
								{
									p_NewMessageLength = ILibReadInnerXML(xnode,&p_NewMessage);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewMessageLength = ILibBase64Decode(p_NewMessage,p_NewMessageLength,&_NewMessage);
	if(UPnPFP_WFAWLANConfig_GetAPSettings == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_GetAPSettings((void*)ReaderObject,_NewMessage,_NewMessageLength);
	free(_NewMessage);
}

#define UPnPDispatch_WFAWLANConfig_GetDeviceInfo(buffer,offset,bufferLength, session)\
{\
	if(UPnPFP_WFAWLANConfig_GetDeviceInfo == NULL)\
	UPnPResponse_Error(session,501,"No Function Handler");\
	else\
	UPnPFP_WFAWLANConfig_GetDeviceInfo((void*)session);\
}

void UPnPDispatch_WFAWLANConfig_GetSTASettings(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewMessage = NULL;
	int p_NewMessageLength = 0;
	unsigned char* _NewMessage = NULL;
	int _NewMessageLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_GetSTASettings\n"));

        if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==14 && memcmp(xnode->Name,"GetSTASettings",14)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==10 && memcmp(xnode->Name,"NewMessage",10)==0)
								{
									p_NewMessageLength = ILibReadInnerXML(xnode,&p_NewMessage);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewMessageLength = ILibBase64Decode(p_NewMessage,p_NewMessageLength,&_NewMessage);
	if(UPnPFP_WFAWLANConfig_GetSTASettings == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_GetSTASettings((void*)ReaderObject,_NewMessage,_NewMessageLength);
	free(_NewMessage);
}

void UPnPDispatch_WFAWLANConfig_PutMessage(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewInMessage = NULL;
	int p_NewInMessageLength = 0;
	unsigned char* _NewInMessage = NULL;
	int _NewInMessageLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_PutMessage\n"));

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==10 && memcmp(xnode->Name,"PutMessage",10)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==12 && memcmp(xnode->Name,"NewInMessage",12)==0)
								{
									p_NewInMessageLength = ILibReadInnerXML(xnode,&p_NewInMessage);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewInMessageLength = ILibBase64Decode(p_NewInMessage,p_NewInMessageLength,&_NewInMessage);
	if(UPnPFP_WFAWLANConfig_PutMessage == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
    {
       printf("invoke UPnPFP_WFAWLANConfig_PutMessage/n");
	   UPnPFP_WFAWLANConfig_PutMessage((void*)ReaderObject,_NewInMessage,_NewInMessageLength);
	}
    free(_NewInMessage);

}

void UPnPDispatch_WFAWLANConfig_PutWLANResponse(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	unsigned long TempULong;
	int OK = 0;
	char *p_NewMessage = NULL;
	int p_NewMessageLength = 0;
	unsigned char* _NewMessage = NULL;
	int _NewMessageLength;
	char *p_NewWLANEventType = NULL;
	int p_NewWLANEventTypeLength = 0;
	unsigned char _NewWLANEventType = 0;
        char tmpWLANEventMAC[20];
	char *p_NewWLANEventMAC = NULL;
	int p_NewWLANEventMACLength = 0;
	char* _NewWLANEventMAC = "";
	int _NewWLANEventMACLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_PutWLANResponse\n"));

        int i;
        for(i=0;i<19;i++)
        {
           tmpWLANEventMAC[i]='1';
        }
        tmpWLANEventMAC[19]=0;

        p_NewWLANEventMAC = tmpWLANEventMAC;

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==15 && memcmp(xnode->Name,"PutWLANResponse",15)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==10 && memcmp(xnode->Name,"NewMessage",10)==0)
								{
									p_NewMessageLength = ILibReadInnerXML(xnode,&p_NewMessage);
									OK |= 1;
								}
								else if(xnode->NameLength==16 && memcmp(xnode->Name,"NewWLANEventType",16)==0)
								{
									p_NewWLANEventTypeLength = ILibReadInnerXML(xnode,&p_NewWLANEventType);
									OK |= 2;
								}
								else if(xnode->NameLength==15 && memcmp(xnode->Name,"NewWLANEventMAC",15)==0)
								{
#if 0 //the MAC address is not currently used , some ER send empty MAC address. try to avoid memory corruption.
									p_NewWLANEventMACLength = ILibReadInnerXML(xnode,&p_NewWLANEventMAC);
                                                                        TUTRACE((TUTRACE_UPNP, "p_NewWLANEventMACLength=%d\n", p_NewWLANEventMACLength));

#else
									p_NewWLANEventMAC[19]=0;
#endif
									OK |= 4;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 7)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewMessageLength = ILibBase64Decode(p_NewMessage,p_NewMessageLength,&_NewMessage);
	OK = ILibGetULong(p_NewWLANEventType,p_NewWLANEventTypeLength, &TempULong);
	if(OK!=0)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
                TUTRACE((TUTRACE_ERR, "402: Illegal value\n"));
		return;
	}
	_NewWLANEventType = (unsigned char)TempULong;
	_NewWLANEventMACLength = ILibInPlaceXmlUnEscape(p_NewWLANEventMAC);
	_NewWLANEventMAC = p_NewWLANEventMAC;
        TUTRACE((TUTRACE_UPNP, "_NewWLANEventMAC=%s\n", p_NewWLANEventMAC));
        TUTRACE((TUTRACE_UPNP, "_NewWLANEventMACLEN=%d\n", _NewWLANEventMACLength));

	if(UPnPFP_WFAWLANConfig_PutWLANResponse == NULL)
        {
	    UPnPResponse_Error(ReaderObject,501,"No Function Handler");
            TUTRACE((TUTRACE_ERR, "Enter UPnPProcessUNSUBSCRIBE\n"));
        }
	else
	UPnPFP_WFAWLANConfig_PutWLANResponse((void*)ReaderObject,_NewMessage,_NewMessageLength,_NewWLANEventType,_NewWLANEventMAC);
	free(_NewMessage);
}

void UPnPDispatch_WFAWLANConfig_RebootAP(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewAPSettings = NULL;
	int p_NewAPSettingsLength = 0;
	unsigned char* _NewAPSettings = NULL;
	int _NewAPSettingsLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_RebootAP\n"));

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"RebootAP",8)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==13 && memcmp(xnode->Name,"NewAPSettings",13)==0)
								{
									p_NewAPSettingsLength = ILibReadInnerXML(xnode,&p_NewAPSettings);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewAPSettingsLength = ILibBase64Decode(p_NewAPSettings,p_NewAPSettingsLength,&_NewAPSettings);
	if(UPnPFP_WFAWLANConfig_RebootAP == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_RebootAP((void*)ReaderObject,_NewAPSettings,_NewAPSettingsLength);
	free(_NewAPSettings);
}

void UPnPDispatch_WFAWLANConfig_RebootSTA(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewSTASettings = NULL;
	int p_NewSTASettingsLength = 0;
	unsigned char* _NewSTASettings = NULL;
	int _NewSTASettingsLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_RebootSTA\n"));

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==9 && memcmp(xnode->Name,"RebootSTA",9)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==14 && memcmp(xnode->Name,"NewSTASettings",14)==0)
								{
									p_NewSTASettingsLength = ILibReadInnerXML(xnode,&p_NewSTASettings);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewSTASettingsLength = ILibBase64Decode(p_NewSTASettings,p_NewSTASettingsLength,&_NewSTASettings);
	if(UPnPFP_WFAWLANConfig_RebootSTA == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_RebootSTA((void*)ReaderObject,_NewSTASettings,_NewSTASettingsLength);
	free(_NewSTASettings);
}

void UPnPDispatch_WFAWLANConfig_ResetAP(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewMessage = NULL;
	int p_NewMessageLength = 0;
	unsigned char* _NewMessage = NULL;
	int _NewMessageLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_ResetAP\n"));


	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==7 && memcmp(xnode->Name,"ResetAP",7)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==10 && memcmp(xnode->Name,"NewMessage",10)==0)
								{
									p_NewMessageLength = ILibReadInnerXML(xnode,&p_NewMessage);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewMessageLength = ILibBase64Decode(p_NewMessage,p_NewMessageLength,&_NewMessage);
	if(UPnPFP_WFAWLANConfig_ResetAP == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_ResetAP((void*)ReaderObject,_NewMessage,_NewMessageLength);
	free(_NewMessage);
}

void UPnPDispatch_WFAWLANConfig_ResetSTA(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewMessage = NULL;
	int p_NewMessageLength = 0;
	unsigned char* _NewMessage = NULL;
	int _NewMessageLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_ResetSTA\n"));

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"ResetSTA",8)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==10 && memcmp(xnode->Name,"NewMessage",10)==0)
								{
									p_NewMessageLength = ILibReadInnerXML(xnode,&p_NewMessage);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewMessageLength = ILibBase64Decode(p_NewMessage,p_NewMessageLength,&_NewMessage);
	if(UPnPFP_WFAWLANConfig_ResetSTA == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_ResetSTA((void*)ReaderObject,_NewMessage,_NewMessageLength);
	free(_NewMessage);
}

void UPnPDispatch_WFAWLANConfig_SetAPSettings(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_APSettings = NULL;
	int p_APSettingsLength = 0;
	unsigned char* _APSettings = NULL;
	int _APSettingsLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_SetAPSettings\n"));

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==13 && memcmp(xnode->Name,"SetAPSettings",13)==0)
						{
							// Inside the interesting part of the SOAP
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==10 && memcmp(xnode->Name,"APSettings",10)==0)
								{
									p_APSettingsLength = ILibReadInnerXML(xnode,&p_APSettings);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_APSettingsLength = ILibBase64Decode(p_APSettings,p_APSettingsLength,&_APSettings);
	if(UPnPFP_WFAWLANConfig_SetAPSettings == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_SetAPSettings((void*)ReaderObject,_APSettings,_APSettingsLength);
	free(_APSettings);
}

void UPnPDispatch_WFAWLANConfig_SetSelectedRegistrar(char *buffer, int offset, int bufferLength, struct ILibWebServer_Session *ReaderObject)
{
	int OK = 0;
	char *p_NewMessage = NULL;
	int p_NewMessageLength = 0;
	unsigned char* _NewMessage = NULL;
	int _NewMessageLength;
	struct ILibXMLNode *xnode = ILibParseXML(buffer,offset,bufferLength);
	struct ILibXMLNode *root = xnode;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPDispatch_WFAWLANConfig_SetSelectedRegistrar\n"));

	if(ILibProcessXMLNodeList(root)!=0)
	{
		/* The XML is not well formed! */
		ILibDestructXMLNodeList(root);
		UPnPResponse_Error(ReaderObject,501,"Invalid XML");
                TUTRACE((TUTRACE_ERR, "Enter UPnPDispatch_WFAWLANConfig_SetSelectedRegistrar\n"));
		return;
	}
	while(xnode!=NULL)
	{
		if(xnode->StartTag!=0 && xnode->NameLength==8 && memcmp(xnode->Name,"Envelope",8)==0)
		{
			// Envelope
			xnode = xnode->Next;
			while(xnode!=NULL)
			{
				if(xnode->StartTag!=0 && xnode->NameLength==4 && memcmp(xnode->Name,"Body",4)==0)
				{
					// Body
					xnode = xnode->Next;
					while(xnode!=NULL)
					{
						if(xnode->StartTag!=0 && xnode->NameLength==20 && memcmp(xnode->Name,"SetSelectedRegistrar",20)==0)
						{
							// Inside the interesting part of the SOAP
                                                        TUTRACE((TUTRACE_UPNP, "Inside the interesting part of the SOAP\n"));
							xnode = xnode->Next;
							while(xnode!=NULL)
							{
								if(xnode->NameLength==10 && memcmp(xnode->Name,"NewMessage",10)==0)
								{
									p_NewMessageLength = ILibReadInnerXML(xnode,&p_NewMessage);
									OK |= 1;
								}
								if(xnode->Peer==NULL)
								{
									xnode = xnode->Parent;
									break;
								}
								else
								{
									xnode = xnode->Peer;
								}
							}
						}
						if(xnode->Peer==NULL)
						{
							xnode = xnode->Parent;
							break;
						}
						else
						{
							xnode = xnode->Peer;
						}
					}
				}
				if(xnode->Peer==NULL)
				{
					xnode = xnode->Parent;
					break;
				}
				else
				{
					xnode = xnode->Peer;
				}
			}
		}
		xnode = xnode->Peer;
	}
	ILibDestructXMLNodeList(root);
	if (OK != 1)
	{
		UPnPResponse_Error(ReaderObject,402,"Illegal value");
		return;
	}

	/* Type Checking */
	_NewMessageLength = ILibBase64Decode(p_NewMessage,p_NewMessageLength,&_NewMessage);
	if(UPnPFP_WFAWLANConfig_SetSelectedRegistrar == NULL)
	UPnPResponse_Error(ReaderObject,501,"No Function Handler");
	else
	UPnPFP_WFAWLANConfig_SetSelectedRegistrar((void*)ReaderObject,_NewMessage,_NewMessageLength);
	free(_NewMessage);
}

#define UPnPDispatch_WFAWLANConfig_SetSTASettings(buffer,offset,bufferLength, session)\
{\
	if(UPnPFP_WFAWLANConfig_SetSTASettings == NULL)\
	UPnPResponse_Error(session,501,"No Function Handler");\
	else\
	UPnPFP_WFAWLANConfig_SetSTASettings((void*)session);\
}


int UPnPProcessPOST(struct ILibWebServer_Session *session, struct packetheader* header, char *bodyBuffer, int offset, int bodyBufferLength)
{
	struct packetheader_field_node *f = header->FirstField;
	char* HOST;
	char* SOAPACTION = NULL;
	int SOAPACTIONLength = 0;
	struct parser_result *r,*r2;
	struct parser_result_field *prf;

	int RetVal = 0;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPProcessPOST\n"));


	//
	// Iterate through all the HTTP Headers
	//
	while(f!=NULL)
	{
		if(f->FieldLength==4 && strncasecmp(f->Field,"HOST",4)==0)
		{
			HOST = f->FieldData;
		}
		else if(f->FieldLength==10 && strncasecmp(f->Field,"SOAPACTION",10)==0)
		{
			r = ILibParseString(f->FieldData,0,f->FieldDataLength,"#",1);
			SOAPACTION = r->LastResult->data;
			SOAPACTIONLength = r->LastResult->datalength-1;
			ILibDestructParserResults(r);
		}
		else if(f->FieldLength==10 && strncasecmp(f->Field,"USER-AGENT",10)==0)
		{
			// Check UPnP version of the Control Point which invoked us
			r = ILibParseString(f->FieldData,0,f->FieldDataLength," ",1);
			prf = r->FirstResult;
			while(prf!=NULL)
			{
				if(prf->datalength>5 && memcmp(prf->data,"UPnP/",5)==0)
				{
					r2 = ILibParseString(prf->data+5,0,prf->datalength-5,".",1);
					r2->FirstResult->data[r2->FirstResult->datalength]=0;
					r2->LastResult->data[r2->LastResult->datalength]=0;
					if(atoi(r2->FirstResult->data)==1 && atoi(r2->LastResult->data)>0)
					{
						session->Reserved9=1;
					}
					ILibDestructParserResults(r2);
				}
				prf = prf->NextResult;
			}
			ILibDestructParserResults(r);
		}
		f = f->NextField;
	}

	if(header->DirectiveObjLength==22 && memcmp((header->DirectiveObj)+1,"WFAWLANConfig/control",21)==0)
	{
		if(SOAPACTIONLength==13 && memcmp(SOAPACTION,"DelAPSettings",13)==0)
		{
			UPnPDispatch_WFAWLANConfig_DelAPSettings(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==14 && memcmp(SOAPACTION,"DelSTASettings",14)==0)
		{
			UPnPDispatch_WFAWLANConfig_DelSTASettings(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==13 && memcmp(SOAPACTION,"GetAPSettings",13)==0)
		{
			UPnPDispatch_WFAWLANConfig_GetAPSettings(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==13 && memcmp(SOAPACTION,"GetDeviceInfo",13)==0)
		{
			UPnPDispatch_WFAWLANConfig_GetDeviceInfo(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==14 && memcmp(SOAPACTION,"GetSTASettings",14)==0)
		{
			UPnPDispatch_WFAWLANConfig_GetSTASettings(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==10 && memcmp(SOAPACTION,"PutMessage",10)==0)
		{
			UPnPDispatch_WFAWLANConfig_PutMessage(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==15 && memcmp(SOAPACTION,"PutWLANResponse",15)==0)
		{
			UPnPDispatch_WFAWLANConfig_PutWLANResponse(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==8 && memcmp(SOAPACTION,"RebootAP",8)==0)
		{
			UPnPDispatch_WFAWLANConfig_RebootAP(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==9 && memcmp(SOAPACTION,"RebootSTA",9)==0)
		{
			UPnPDispatch_WFAWLANConfig_RebootSTA(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==7 && memcmp(SOAPACTION,"ResetAP",7)==0)
		{
			UPnPDispatch_WFAWLANConfig_ResetAP(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==8 && memcmp(SOAPACTION,"ResetSTA",8)==0)
		{
			UPnPDispatch_WFAWLANConfig_ResetSTA(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==13 && memcmp(SOAPACTION,"SetAPSettings",13)==0)
		{
			UPnPDispatch_WFAWLANConfig_SetAPSettings(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==20 && memcmp(SOAPACTION,"SetSelectedRegistrar",20)==0)
		{
			UPnPDispatch_WFAWLANConfig_SetSelectedRegistrar(bodyBuffer, offset, bodyBufferLength, session);
		}
		else if(SOAPACTIONLength==14 && memcmp(SOAPACTION,"SetSTASettings",14)==0)
		{
			UPnPDispatch_WFAWLANConfig_SetSTASettings(bodyBuffer, offset, bodyBufferLength, session);
		}
		else
		{
			RetVal=1;
		}
	}
	else
	{
		RetVal=1;
	}


	return(RetVal);
}
struct SubscriberInfo* UPnPRemoveSubscriberInfo(struct SubscriberInfo **Head, int *TotalSubscribers,char* SID, int SIDLength)
{
	struct SubscriberInfo *info = *Head;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPRemoveSubscriberInfo\n"));

	while(info!=NULL)
	{
		if(info->SIDLength==SIDLength && memcmp(info->SID,SID,SIDLength)==0)
		{
			if ( info->Previous )
			info->Previous->Next = info->Next;
			else
			*Head = info->Next;
			if ( info->Next )
			info->Next->Previous = info->Previous;
			break;
		}
		info = info->Next;

	}
	if(info!=NULL)
	{
		info->Previous = NULL;
		info->Next = NULL;
		--(*TotalSubscribers);
	}
	return(info);
}

#define UPnPDestructSubscriberInfo(info)\
{\
	free(info->Path);\
	free(info->SID);\
	free(info);\
}

#define UPnPDestructEventObject(EvObject)\
{\
	free(EvObject->PacketBody);\
	free(EvObject);\
}

#define UPnPDestructEventDataObject(EvData)\
{\
	free(EvData);\
}
void UPnPExpireSubscriberInfo(struct UPnPDataObject *d, struct SubscriberInfo *info)
{
	struct SubscriberInfo *t = info;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPExpireSubscriberInfo\n"));

	while(t->Previous!=NULL)
	{
		t = t->Previous;
	}
	if(d->HeadSubscriberPtr_WFAWLANConfig==t)
	{
		--(d->NumberOfSubscribers_WFAWLANConfig);
	}


	if(info->Previous!=NULL)
	{
		// This is not the Head
		info->Previous->Next = info->Next;
		if(info->Next!=NULL)
		{
			info->Next->Previous = info->Previous;
		}
	}
	else
	{
		// This is the Head
		if(d->HeadSubscriberPtr_WFAWLANConfig==info)
		{
			d->HeadSubscriberPtr_WFAWLANConfig = info->Next;
			if(info->Next!=NULL)
			{
				info->Next->Previous = NULL;
			}
		}
		else
		{
			// Error
			return;
		}

	}
	--info->RefCount;
	if(info->RefCount==0)
	{
		UPnPDestructSubscriberInfo(info);
	}
}

int UPnPSubscriptionExpired(struct SubscriberInfo *info)
{
	int RetVal = 0;

	struct timeval tv;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPSubscriptionExpired\n"));

	gettimeofday(&tv,NULL);
	if((info->RenewByTime).tv_sec < tv.tv_sec) {RetVal = -1;}

	return(RetVal);
}

void UPnPGetInitialEventBody_WFAWLANConfig(struct UPnPDataObject *UPnPObject,char ** body, int *bodylength)
{
	int TempLength;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPGetInitialEventBody_WFAWLANConfig\n"));

	TempLength = (int)(121+(int)strlen(UPnPObject->WFAWLANConfig_STAStatus)+(int)strlen(UPnPObject->WFAWLANConfig_APStatus)+(int)strlen(UPnPObject->WFAWLANConfig_WLANEvent));
	*body = (char*)malloc(sizeof(char)*TempLength);
	*bodylength = sprintf(*body,"STAStatus>%s</STAStatus></e:property><e:property><APStatus>%s</APStatus></e:property><e:property><WLANEvent>%s</WLANEvent",UPnPObject->WFAWLANConfig_STAStatus,UPnPObject->WFAWLANConfig_APStatus,UPnPObject->WFAWLANConfig_WLANEvent);
}


void UPnPProcessUNSUBSCRIBE(struct packetheader *header, struct ILibWebServer_Session *session)
{
	char* SID = NULL;
	int SIDLength = 0;
	struct SubscriberInfo *Info;
	struct packetheader_field_node *f;
	char* packet = (char*)malloc(sizeof(char)*50);
	int packetlength;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPProcessUNSUBSCRIBE\n"));

	//
	// Iterate through all the HTTP headers
	//
	f = header->FirstField;
	while(f!=NULL)
	{
		if(f->FieldLength==3)
		{
			if(strncasecmp(f->Field,"SID",3)==0)
			{
				//
				// Get the Subscription ID
				//
				SID = f->FieldData;
				SIDLength = f->FieldDataLength;
			}
		}
		f = f->NextField;
	}
	sem_wait(&(((struct UPnPDataObject*)session->User)->EventLock));
	if(header->DirectiveObjLength==20 && memcmp(header->DirectiveObj + 1,"WFAWLANConfig/event",19)==0)
	{
		Info = UPnPRemoveSubscriberInfo(&(((struct UPnPDataObject*)session->User)->HeadSubscriberPtr_WFAWLANConfig),&(((struct UPnPDataObject*)session->User)->NumberOfSubscribers_WFAWLANConfig),SID,SIDLength);
		if(Info!=NULL)
		{
			--Info->RefCount;
			if(Info->RefCount==0)
			{
				UPnPDestructSubscriberInfo(Info);
			}
			packetlength = sprintf(packet,"HTTP/1.1 %d %s\r\nContent-Length: 0\r\n\r\n",200,"OK");
			ILibWebServer_Send_Raw(session,packet,packetlength,0,1);
		}
		else
		{
			packetlength = sprintf(packet,"HTTP/1.1 %d %s\r\nContent-Length: 0\r\n\r\n",412,"Invalid SID");
			ILibWebServer_Send_Raw(session,packet,packetlength,0,1);
		}
	}

	sem_post(&(((struct UPnPDataObject*)session->User)->EventLock));
}
void UPnPTryToSubscribe(char* ServiceName, long Timeout, char* URL, int URLLength,struct ILibWebServer_Session *session)
{
	int *TotalSubscribers = NULL;
	struct SubscriberInfo **HeadPtr = NULL;
	struct SubscriberInfo *NewSubscriber,*TempSubscriber;
	int SIDNumber,rnumber;
	char *SID;
	char *TempString;
	int TempStringLength;
	char *TempString2;
	long TempLong;
	char *packet;
	int packetlength;
	char* path;

	char* escapedURI;
	int escapedURILength;

	char *packetbody = NULL;
	int packetbodyLength;

	struct parser_result *p;
	struct parser_result *p2;

	struct UPnPDataObject *dataObject = (struct UPnPDataObject*)session->User;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPTryToSubscribe\n"));

	if(strncmp(ServiceName,"WFAWLANConfig",13)==0)
	{
		TotalSubscribers = &(dataObject->NumberOfSubscribers_WFAWLANConfig);
		HeadPtr = &(dataObject->HeadSubscriberPtr_WFAWLANConfig);
                TUTRACE((TUTRACE_UPNP, " received WFAWLANConfig\n"));
	}


	if(*HeadPtr!=NULL)
	{
		NewSubscriber = *HeadPtr;
		while(NewSubscriber!=NULL)
		{
			if(UPnPSubscriptionExpired(NewSubscriber)!=0)
			{
				TempSubscriber = NewSubscriber->Next;
				NewSubscriber = UPnPRemoveSubscriberInfo(HeadPtr,TotalSubscribers,NewSubscriber->SID,NewSubscriber->SIDLength);
				UPnPDestructSubscriberInfo(NewSubscriber);
				NewSubscriber = TempSubscriber;
                                TUTRACE((TUTRACE_UPNP, "UPnPSubscriptionExpired\n"));
			}
			else
			{
				NewSubscriber = NewSubscriber->Next;
                                TUTRACE((TUTRACE_UPNP, "NewSubscriber in the list \n"));
			}
		}
	}
	//
	// The Maximum number of subscribers can be bounded
	//
	if(*TotalSubscribers<10)
	{
		NewSubscriber = (struct SubscriberInfo*)malloc(sizeof(struct SubscriberInfo));
		memset(NewSubscriber,0,sizeof(struct SubscriberInfo));


		//
		// The SID must be globally unique, so lets generate it using
		// a bunch of random hex characters
		//
		SID = (char*)malloc(43);
		memset(SID,0,38);
		sprintf(SID,"uuid:");
		for(SIDNumber=5;SIDNumber<=12;++SIDNumber)
		{
			rnumber = rand()%16;
			sprintf(SID+SIDNumber,"%x",rnumber);
		}
		sprintf(SID+SIDNumber,"-");
		for(SIDNumber=14;SIDNumber<=17;++SIDNumber)
		{
			rnumber = rand()%16;
			sprintf(SID+SIDNumber,"%x",rnumber);
		}
		sprintf(SID+SIDNumber,"-");
		for(SIDNumber=19;SIDNumber<=22;++SIDNumber)
		{
			rnumber = rand()%16;
			sprintf(SID+SIDNumber,"%x",rnumber);
		}
		sprintf(SID+SIDNumber,"-");
		for(SIDNumber=24;SIDNumber<=27;++SIDNumber)
		{
			rnumber = rand()%16;
			sprintf(SID+SIDNumber,"%x",rnumber);
		}
		sprintf(SID+SIDNumber,"-");
		for(SIDNumber=29;SIDNumber<=40;++SIDNumber)
		{
			rnumber = rand()%16;
			sprintf(SID+SIDNumber,"%x",rnumber);
		}

                TUTRACE((TUTRACE_UPNP, "SID+SIDNumber=%s\n",SID));

		p = ILibParseString(URL,0,URLLength,"://",3);
		if(p->NumResults==1)
		{
			ILibWebServer_Send_Raw(session,"HTTP/1.1 412 Precondition Failed\r\nContent-Length: 0\r\n\r\n",55,1,1);
                        TUTRACE((TUTRACE_UPNP,"HTTP/1.1 412 Precondition Failed\r\nContent-Length: 0\r\n\r\n",55,1,1 ));
			ILibDestructParserResults(p);
			return;
		}
		TempString = p->LastResult->data;
		TempStringLength = p->LastResult->datalength;
		ILibDestructParserResults(p);
		p = ILibParseString(TempString,0,TempStringLength,"/",1);
		p2 = ILibParseString(p->FirstResult->data,0,p->FirstResult->datalength,":",1);
		TempString2 = (char*)malloc(1+sizeof(char)*p2->FirstResult->datalength);
		memcpy(TempString2,p2->FirstResult->data,p2->FirstResult->datalength);
		TempString2[p2->FirstResult->datalength] = '\0';
		NewSubscriber->Address = inet_addr(TempString2);
		if(p2->NumResults==1)
		{
			NewSubscriber->Port = 80;
			path = (char*)malloc(1+TempStringLength - p2->FirstResult->datalength -1);
			memcpy(path,TempString + p2->FirstResult->datalength,TempStringLength - p2->FirstResult->datalength -1);
			path[TempStringLength - p2->FirstResult->datalength - 1] = '\0';
			NewSubscriber->Path = path;
			NewSubscriber->PathLength = (int)strlen(path);
                        TUTRACE((TUTRACE_UPNP, "p2->NumResults==1\n"));
		}
		else
		{
			ILibGetLong(p2->LastResult->data,p2->LastResult->datalength,&TempLong);
			NewSubscriber->Port = (unsigned short)TempLong;
			if(TempStringLength==p->FirstResult->datalength)
			{
				path = (char*)malloc(2);
				memcpy(path,"/",1);
				path[1] = '\0';
			}
			else
			{
				path = (char*)malloc(1+TempStringLength - p->FirstResult->datalength -1);
				memcpy(path,TempString + p->FirstResult->datalength,TempStringLength - p->FirstResult->datalength -1);
				path[TempStringLength - p->FirstResult->datalength -1] = '\0';
			}
			NewSubscriber->Path = path;
			NewSubscriber->PathLength = (int)strlen(path);
                        TUTRACE((TUTRACE_UPNP, "p2->NumResults!=1\n"));
		}
		ILibDestructParserResults(p);
		ILibDestructParserResults(p2);
		free(TempString2);


		escapedURI = (char*)malloc(ILibHTTPEscapeLength(NewSubscriber->Path));
		escapedURILength = ILibHTTPEscape(escapedURI,NewSubscriber->Path);

		free(NewSubscriber->Path);
		NewSubscriber->Path = escapedURI;
		NewSubscriber->PathLength = escapedURILength;


		NewSubscriber->RefCount = 1;
		NewSubscriber->Disposing = 0;
		NewSubscriber->Previous = NULL;
		NewSubscriber->SID = SID;
		NewSubscriber->SIDLength = (int)strlen(SID);
		NewSubscriber->SEQ = 0;

		//
		// Determine what the subscription renewal cycle is
		//

		gettimeofday(&(NewSubscriber->RenewByTime),NULL);
		(NewSubscriber->RenewByTime).tv_sec += (int)Timeout;

		NewSubscriber->Next = *HeadPtr;
		if(*HeadPtr!=NULL) {(*HeadPtr)->Previous = NewSubscriber;}
		*HeadPtr = NewSubscriber;
		++(*TotalSubscribers);
		g_TotalUPnPEventSubscribers = *TotalSubscribers; // keep track of number of subscribers
		LVL3DEBUG(printf("\r\n\r\nSubscribed [%s] %d.%d.%d.%d:%d FOR %d Duration\r\n",NewSubscriber->SID,(NewSubscriber->Address)&0xFF,(NewSubscriber->Address>>8)&0xFF,(NewSubscriber->Address>>16)&0xFF,(NewSubscriber->Address>>24)&0xFF,NewSubscriber->Port,Timeout);)

		LVL3DEBUG(printf("TIMESTAMP: %d <%d>\r\n\r\n",(NewSubscriber->RenewByTime).tv_sec-Timeout,NewSubscriber);)

		packet = (char*)malloc(134 + (int)strlen(SID) + (int)strlen(UPnPPLATFORM) + 4);
		packetlength = sprintf(packet,"HTTP/1.1 200 OK\r\nSERVER: %s, UPnP/1.0, Intel MicroStack/1.0.2126\r\nSID: %s\r\nTIMEOUT: Second-%ld\r\nContent-Length: 0\r\n\r\n",UPnPPLATFORM,SID,Timeout);
		if(strcmp(ServiceName,"WFAWLANConfig")==0)
		{
			UPnPGetInitialEventBody_WFAWLANConfig(dataObject,&packetbody,&packetbodyLength);
		}

		if (packetbody != NULL)	    {
			ILibWebServer_Send_Raw(session,packet,packetlength,0,1);

			UPnPSendEvent_Body(dataObject,packetbody,packetbodyLength,NewSubscriber);
			free(packetbody);
		}
                TUTRACE((TUTRACE_UPNP, " g_TotalUPnPEventSubscribers=%d\n", g_TotalUPnPEventSubscribers));
	}
	else
	{
                TUTRACE((TUTRACE_ERR, "Too Many Subscribers\n"));
		/* Too many subscribers */
		ILibWebServer_Send_Raw(session,"HTTP/1.1 412 Too Many Subscribers\r\nContent-Length: 0\r\n\r\n",56,1,1);
	}
}
void UPnPSubscribeEvents(char* path,int pathlength,char* Timeout,int TimeoutLength,char* URL,int URLLength,struct ILibWebServer_Session* session)
{
	long TimeoutVal;
	char* buffer = (char*)malloc(1+sizeof(char)*pathlength);

        TUTRACE((TUTRACE_ERR, "Enter UPnPSubscribeEvents\n"));

	ILibGetLong(Timeout,TimeoutLength,&TimeoutVal);
	memcpy(buffer,path,pathlength);
	buffer[pathlength] = '\0';
	free(buffer);
	if(TimeoutVal>UPnP_MAX_SUBSCRIPTION_TIMEOUT) {TimeoutVal=UPnP_MAX_SUBSCRIPTION_TIMEOUT;}

	if(pathlength==20 && memcmp(path+1,"WFAWLANConfig/event",19)==0)
	{
                TUTRACE((TUTRACE_UPNP, "UPnPSubscribeEvents UPnPTryToSubscribe\n"));
		UPnPTryToSubscribe("WFAWLANConfig",TimeoutVal,URL,URLLength,session);
	}
	else
	{
                TUTRACE((TUTRACE_UPNP, "UPnPSubscribeEvents ILibWebServer_Send_Raw\n"));
		ILibWebServer_Send_Raw(session,"HTTP/1.1 412 Invalid Service Name\r\nContent-Length: 0\r\n\r\n",56,1,1);
	}

}
void UPnPRenewEvents(char* path,int pathlength,char *_SID,int SIDLength, char* Timeout, int TimeoutLength, struct ILibWebServer_Session *ReaderObject)
{
	struct SubscriberInfo *info = NULL;
	long TimeoutVal;

	struct timeval tv;

	char* packet;
	int packetlength;
	char* SID = (char*)malloc(SIDLength+1);
	memcpy(SID,_SID,SIDLength);
	SID[SIDLength] ='\0';

        TUTRACE((TUTRACE_UPNP, "Enter UPnPRenewEvents\n"));

	LVL3DEBUG(gettimeofday(&tv,NULL);)
	LVL3DEBUG(printf("\r\n\r\nTIMESTAMP: %d\r\n",tv.tv_sec);)

	LVL3DEBUG(printf("SUBSCRIBER [%s] attempting to Renew Events for %s Duration [",SID,Timeout);)

	if(pathlength==20 && memcmp(path+1,"WFAWLANConfig/event",19)==0)
	{
		info = ((struct UPnPDataObject*)ReaderObject->User)->HeadSubscriberPtr_WFAWLANConfig;
	}


	//
	// Find this SID in the subscriber list, and recalculate
	// the expiration timeout
	//
	while(info!=NULL && strcmp(info->SID,SID)!=0)
	{
		info = info->Next;
	}
	if(info!=NULL)
	{
		ILibGetLong(Timeout,TimeoutLength,&TimeoutVal);

		gettimeofday(&tv,NULL);
		(info->RenewByTime).tv_sec = tv.tv_sec + TimeoutVal;

		packet = (char*)malloc(134 + (int)strlen(SID) + 4);
		packetlength = sprintf(packet,"HTTP/1.1 200 OK\r\nSERVER: %s, UPnP/1.0, Intel MicroStack/1.0.2126\r\nSID: %s\r\nTIMEOUT: Second-%ld\r\nContent-Length: 0\r\n\r\n",UPnPPLATFORM,SID,TimeoutVal);
		ILibWebServer_Send_Raw(ReaderObject,packet,packetlength,0,1);
		LVL3DEBUG(printf("OK] {%d} <%d>\r\n\r\n",TimeoutVal,info);)
	}
	else
	{
		LVL3DEBUG(printf("FAILED]\r\n\r\n");)
		ILibWebServer_Send_Raw(ReaderObject,"HTTP/1.1 412 Precondition Failed\r\nContent-Length: 0\r\n\r\n",55,1,1);
	}
	free(SID);
}
void UPnPProcessSUBSCRIBE(struct packetheader *header, struct ILibWebServer_Session *session)
{
	char* SID = NULL;
	int SIDLength = 0;
	char* Timeout = NULL;
	int TimeoutLength = 0;
	char* URL = NULL;
	int URLLength = 0;
	struct parser_result *p;

	struct packetheader_field_node *f;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPProcessSUBSCRIBE \n"));

	//
	// Iterate through all the HTTP Headers
	//
	f = header->FirstField;
	while(f!=NULL)
	{
		if(f->FieldLength==3 && strncasecmp(f->Field,"SID",3)==0)
		{
			//
			// Get the Subscription ID
			//
			SID = f->FieldData;
			SIDLength = f->FieldDataLength;
                        TUTRACE((TUTRACE_UPNP, " Get the Subscription ID\n"));
		}
		else if(f->FieldLength==8 && strncasecmp(f->Field,"Callback",8)==0)
		{
			//
			// Get the Callback URL
			//
			URL = f->FieldData;
			URLLength = f->FieldDataLength;
                        TUTRACE((TUTRACE_UPNP, " Get the Callback URL\n"));
                        TUTRACE((TUTRACE_UPNP, " URLLength=%d\n",URLLength));
                        TUTRACE((TUTRACE_UPNP, "%s\n",URL));
		}
		else if(f->FieldLength==7 && strncasecmp(f->Field,"Timeout",7)==0)
		{
			//
			// Get the requested timeout value
			//
			Timeout = f->FieldData;
			TimeoutLength = f->FieldDataLength;
                        TUTRACE((TUTRACE_UPNP, " Get the requested timeout value\n"));
		}

		f = f->NextField;
	}
	if(Timeout==NULL)
	{
		//
		// It a timeout wasn't specified, force it to a specific value
		//
		Timeout = "7200";
		TimeoutLength = 4;
	}
	else
	{
		p = ILibParseString(Timeout,0,TimeoutLength,"-",1);
		if(p->NumResults==2)
		{
			Timeout = p->LastResult->data;
			TimeoutLength = p->LastResult->datalength;
			if(TimeoutLength==8 && strncasecmp(Timeout,"INFINITE",8)==0)
			{
				//
				// Infinite timeouts will cause problems, so we don't allow it
				//
				Timeout = "7200";
				TimeoutLength = 4;
			}
		}
		else
		{
			Timeout = "7200";
			TimeoutLength = 4;
		}
		ILibDestructParserResults(p);
                TUTRACE((TUTRACE_UPNP, " ILibDestructParserResults\n"));
	}
	if(SID==NULL)
	{
		//
		// If not SID was specified, this is a subscription request
		//

                TUTRACE((TUTRACE_UPNP, "UPnPProcessSUBSCRIBE UPnPSubscribeEvents\n"));
		/* Subscribe */
		UPnPSubscribeEvents(header->DirectiveObj,header->DirectiveObjLength,Timeout,TimeoutLength,URL,URLLength,session);
	}
	else
	{
		//
		// If a SID was specified, it is a renewal request for an existing subscription
		//

		/* Renew */
                TUTRACE((TUTRACE_UPNP, "UPnPProcessSUBSCRIBE UPnPRenewEvents\n"));

		UPnPRenewEvents(header->DirectiveObj,header->DirectiveObjLength,SID,SIDLength,Timeout,TimeoutLength,session);
	}
}

void UPnPProcessHTTPPacket(struct ILibWebServer_Session *session, struct packetheader* header, char *bodyBuffer, int offset, int bodyBufferLength)
{

	struct UPnPDataObject *dataObject = (struct UPnPDataObject*)session->User;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPProcessHTTPPacket\n"));
#if 0
        int i;
        char * pTmp;
        pTmp=bodyBuffer;
        printf("http body= %d\n", bodyBufferLength);
        for(i=0; i< bodyBufferLength;i++)
        {
            printf("%c,",*pTmp++);
            if (i % 16 ==0)
                printf("\n");
        }
#endif
	#if defined(WIN32) || defined(_WIN32_WCE)
	char *responseHeader = "\r\nCONTENT-TYPE:  text/xml; charset=\"utf-8\"\r\nServer: WINDOWS, UPnP/1.0, Intel MicroStack/1.0.2126";
	#else
	char *responseHeader = "\r\nCONTENT-TYPE:  text/xml; charset=\"utf-8\"\r\nServer: POSIX, UPnP/1.0, Intel MicroStack/1.0.2126";
	#endif
	char *errorTemplate = "HTTP/1.1 %d %s\r\nServer: %s, UPnP/1.0, Intel MicroStack/1.0.2126\r\nContent-Length: 0\r\n\r\n";
	char *errorPacket;
	int errorPacketLength;
	char *buffer;

	LVL3DEBUG(errorPacketLength=ILibGetRawPacket(header,&errorPacket);)
	LVL3DEBUG(printf("%s\r\n",errorPacket);)
	LVL3DEBUG(free(errorPacket);)

	if(header->DirectiveLength==4 && memcmp(header->Directive,"HEAD",4)==0)
	{
                TUTRACE((TUTRACE_UPNP, "HTTP receive HEAD\n"));
		if(header->DirectiveObjLength==1 && memcmp(header->DirectiveObj,"/",1)==0)
		{
                        TUTRACE((TUTRACE_UPNP, "HTTP receive /\n"));
			//
			// A HEAD request for the device description document.
			// We stream the document back, so we don't return content length or anything
			// because the actual response won't have it either
			//
			ILibWebServer_StreamHeader_Raw(session,200,"OK",responseHeader,1);
			ILibWebServer_StreamBody(session,NULL,0,ILibAsyncSocket_MemoryOwnership_STATIC,1);
		}

		else if(header->DirectiveObjLength==23 && memcmp((header->DirectiveObj)+1,"WFAWLANConfig/scpd.xml",22)==0)
		{
                        TUTRACE((TUTRACE_UPNP, "HTTP receive WFAWLANConfig/scpd.xml\n"));
			ILibWebServer_StreamHeader_Raw(session,200,"OK",responseHeader,1);
			ILibWebServer_StreamBody(session,NULL,0,ILibAsyncSocket_MemoryOwnership_STATIC,1);
		}

		else
		{
			//
			// A HEAD request for something we don't have
			//
                        TUTRACE((TUTRACE_ERR, "HTTP receive a header we do not have\n"));
			errorPacket = (char*)malloc(128);
			errorPacketLength = sprintf(errorPacket,errorTemplate,404,"File Not Found",UPnPPLATFORM);
			ILibWebServer_Send_Raw(session,errorPacket,errorPacketLength,0,1);
		}
	}
	else if(header->DirectiveLength==3 && memcmp(header->Directive,"GET",3)==0)
	{
                TUTRACE((TUTRACE_ERR, "HTTP receive GET\n"));
		if(header->DirectiveObjLength==1 && memcmp(header->DirectiveObj,"/",1)==0)
		{
			//
			// A GET Request for the device description document, so lets stream
			// it back to the client
			//
                        TUTRACE((TUTRACE_INFO, "HTTP receive /\n"));
			ILibWebServer_StreamHeader_Raw(session,200,"OK",responseHeader,1);
			ILibWebServer_StreamBody(session,dataObject->DeviceDescription,dataObject->DeviceDescriptionLength,1,1);

		}

		else if(header->DirectiveObjLength==23 && memcmp((header->DirectiveObj)+1,"WFAWLANConfig/scpd.xml",22)==0)
		{
                        TUTRACE((TUTRACE_INFO, "HTTP receive WFAWLANConfig/scpd.xml\n"));
			buffer = ILibDecompressString((char*)UPnPWFAWLANConfigDescription,UPnPWFAWLANConfigDescriptionLength,UPnPWFAWLANConfigDescriptionLengthUX);
			ILibWebServer_Send_Raw(session,buffer,UPnPWFAWLANConfigDescriptionLengthUX,0,1);
		}

		else
		{
			//
			// A GET Request for something we don't have
			//
                        TUTRACE((TUTRACE_ERR, "HTTP receive something we don't have =&s\n", (header->DirectiveObj)+1));
			errorPacket = (char*)malloc(128);
			errorPacketLength = sprintf(errorPacket,errorTemplate,404,"File Not Found",UPnPPLATFORM);
			ILibWebServer_Send_Raw(session,errorPacket,errorPacketLength,0,1);
		}
	}
	else if(header->DirectiveLength==4 && memcmp(header->Directive,"POST",4)==0)
	{
		//
		// Defer Control to the POST Handler
		//
                TUTRACE((TUTRACE_INFO, "HTTP receive POST\n"));
		if(UPnPProcessPOST(session,header,bodyBuffer,offset,bodyBufferLength)!=0)
		{
			//
			// A POST for an action that doesn't exist
			//
			UPnPResponse_Error(session,401,"Invalid Action");
		}
	}
	else if(header->DirectiveLength==9 && memcmp(header->Directive,"SUBSCRIBE",9)==0)
	{
		//
		// Subscription Handler
		//
                TUTRACE((TUTRACE_INFO, "HTTP receive SUBSCRIBE\n"));
		UPnPProcessSUBSCRIBE(header,session);
	}
	else if(header->DirectiveLength==11 && memcmp(header->Directive,"UNSUBSCRIBE",11)==0)
	{
		//
		// UnSubscribe Handler
		//
                TUTRACE((TUTRACE_INFO, "HTTP receive UNSUBSCRIBE\n"));
		UPnPProcessUNSUBSCRIBE(header,session);
	}
	else
	{
		//
		// The client tried something we didn't expect/support
		//
                TUTRACE((TUTRACE_ERR, "HTTP receive something we didn't expect/support\n"));
		errorPacket = (char*)malloc(128);
		errorPacketLength = sprintf(errorPacket,errorTemplate,400,"Bad Request",UPnPPLATFORM);
		ILibWebServer_Send_Raw(session,errorPacket,errorPacketLength,1,1);
	}
}
void UPnPFragmentedSendNotify_Destroy(void *data);
void UPnPMasterPreSelect(void* object,fd_set *socketset, fd_set *writeset, fd_set *errorset, int* blocktime)
{
	int i;
	struct UPnPDataObject *UPnPObject = (struct UPnPDataObject*)object;
	struct UPnPFragmentNotifyStruct *f;
	int timeout;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPMasterPreSelect\n"));

	if(UPnPObject->InitialNotify==0)
	{
		//
		// The initial "HELLO" packets were not sent yet, so lets send them
		//
		UPnPObject->InitialNotify = -1;
		//
		// In case we were interrupted, we need to flush out the caches of
		// all the control points by sending a "byebye" first, to insure
		// control points don't ignore our "hello" packets thinking they are just
		// periodic re-advertisements.
		//
		UPnPSendByeBye(UPnPObject);

		//
		// PacketNumber 0 is the controller, for the rest of the packets. Send
		// one of these to send out an advertisement "group"
		//
		f = (struct UPnPFragmentNotifyStruct*)malloc(sizeof(struct UPnPFragmentNotifyStruct));
		f->packetNumber=0;
		f->upnp = UPnPObject;
		//
		// We need to inject some delay in these packets to space them out,
		// otherwise we could overflow the inbound buffer of the recipient, causing them
		// to lose packets. And UPnP/1.0 control points are not as robust as UPnP/1.1 control points,
		// so they need all the help they can get ;)
		//
		timeout = (int)(0 + ((unsigned short)rand() % (500)));
		do
		{
			f->upnp->InitialNotify = rand();
		}while(f->upnp->InitialNotify==0);
		//
		// Register for the timed callback, to actually send the packet
		//
		ILibLifeTime_AddEx(f->upnp->WebServerTimer,f,timeout,&UPnPFragmentedSendNotify,&UPnPFragmentedSendNotify_Destroy);

	}
	if(UPnPObject->UpdateFlag!=0)
	{
		//
		// Somebody told us that we should recheck our IP Address table,
		// as one of them may have changed
		//
		UPnPObject->UpdateFlag = 0;

		/* Clear Sockets */


		//
		// Iterate through all the currently bound IP addresses
		// and release the sockets
		//
		for(i=0;i<UPnPObject->AddressListLength;++i)
		{
			ILibChain_SafeRemove(UPnPObject->Chain,UPnPObject->NOTIFY_SEND_socks[i]);
		}
		free(UPnPObject->NOTIFY_SEND_socks);


		//
		// Fetch a current list of ip addresses
		//
		free(UPnPObject->AddressList);
		UPnPObject->AddressListLength = ILibGetLocalIPAddressList(&(UPnPObject->AddressList));


		//
		// Re-Initialize our SEND socket
		//
		UPnPObject->NOTIFY_SEND_socks = (void**)malloc(sizeof(void*)*(UPnPObject->AddressListLength));

		//
		// Now that we have a new list of IP addresses, re-initialise everything
		//
		for(i=0;i<UPnPObject->AddressListLength;++i)
		{
			UPnPObject->NOTIFY_SEND_socks[i] = ILibAsyncUDPSocket_Create(
			UPnPObject->Chain,
			UPNP_MAX_SSDP_HEADER_SIZE,
			UPnPObject->AddressList[i],
			UPNP_PORT,
			ILibAsyncUDPSocket_Reuse_SHARED,
			NULL,
			NULL,
			UPnPObject);
			ILibAsyncUDPSocket_JoinMulticastGroup(
			UPnPObject->NOTIFY_SEND_socks[i],
			UPnPObject->AddressList[i],
			inet_addr(UPNP_GROUP));

			ILibAsyncUDPSocket_SetMulticastTTL(UPnPObject->NOTIFY_SEND_socks[i],UPNP_SSDP_TTL);

			ILibAsyncUDPSocket_JoinMulticastGroup(
			UPnPObject->NOTIFY_RECEIVE_sock,
			UPnPObject->AddressList[i],
			inet_addr(UPNP_GROUP));
		}


		//
		// Iterate through all the packet types, and re-broadcast
		//
		for(i=1;i<=4;++i)
		{
			f = (struct UPnPFragmentNotifyStruct*)malloc(sizeof(struct UPnPFragmentNotifyStruct));
			f->packetNumber=i;
			f->upnp = UPnPObject;
			//
			// Inject some random delay, to spread these packets out, to help prevent
			// the inbound buffer of the recipient from overflowing, causing dropped packets.
			//
			timeout = (int)(0 + ((unsigned short)rand() % (500)));
			ILibLifeTime_AddEx(f->upnp->WebServerTimer,f,timeout,&UPnPFragmentedSendNotify,&UPnPFragmentedSendNotify_Destroy);
		}
	}
}

void UPnPFragmentedSendNotify_Destroy(void *data)
{
	free(data);
}
void UPnPFragmentedSendNotify(void *data)
{
	struct UPnPFragmentNotifyStruct *FNS = (struct UPnPFragmentNotifyStruct*)data;
	int timeout,timeout2;
	int subsetRange;
	int packetlength;
	char* packet = (char*)malloc(5000);
	int i,i2;
	struct UPnPFragmentNotifyStruct *f;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPFragmentedSendNotify\n"));
printf("Enter UPnPFragmentedSendNotify\n");

	if(FNS->packetNumber==0)
	{
		subsetRange = 5000/5; // Make sure all our packets will get out within 5 seconds

		// Send the first "group"
		for(i2=0;i2<4;++i2)
		{
			f = (struct UPnPFragmentNotifyStruct*)malloc(sizeof(struct UPnPFragmentNotifyStruct));
			f->packetNumber=i2+1;
			f->upnp = FNS->upnp;
			timeout2 = (rand() % subsetRange);
printf("first notify cycle timeout2 = %dms\n", timeout2);
			ILibLifeTime_AddEx(FNS->upnp->WebServerTimer,f,timeout2,&UPnPFragmentedSendNotify,&UPnPFragmentedSendNotify_Destroy);
		}

		// Now Repeat this "group" after 7 seconds, to insure there is no overlap
		for(i2=0;i2<4;++i2)
		{
			f = (struct UPnPFragmentNotifyStruct*)malloc(sizeof(struct UPnPFragmentNotifyStruct));
			f->packetNumber=i2+1;
			f->upnp = FNS->upnp;
			timeout2 = 7000 + (rand() % subsetRange);
printf("notify cycle timeout2 = %dms\n", timeout2);
			ILibLifeTime_AddEx(FNS->upnp->WebServerTimer,f,timeout2,&UPnPFragmentedSendNotify,&UPnPFragmentedSendNotify_Destroy);
		}

		// Calculate the next transmission window and spread the packets
		timeout = (int)((FNS->upnp->NotifyCycleTime/4) + ((unsigned short)rand() % (FNS->upnp->NotifyCycleTime/2 - FNS->upnp->NotifyCycleTime/4)));
printf("notify cycle timeout = %dms\n", timeout);
		ILibLifeTime_Add(FNS->upnp->WebServerTimer,FNS,timeout,&UPnPFragmentedSendNotify,&UPnPFragmentedSendNotify_Destroy);
	}

	for(i=0;i<FNS->upnp->AddressListLength;++i)
	{
		ILibAsyncUDPSocket_SetMulticastInterface(FNS->upnp->NOTIFY_SEND_socks[i],FNS->upnp->AddressList[i]);
		switch(FNS->packetNumber)
		{
			case 1:
			UPnPBuildSsdpNotifyPacket(packet,&packetlength,FNS->upnp->AddressList[i],(unsigned short)FNS->upnp->WebSocketPortNumber,0,FNS->upnp->UDN,"::upnp:rootdevice","upnp:rootdevice","",FNS->upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(FNS->upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			break;
			case 2:
			UPnPBuildSsdpNotifyPacket(packet,&packetlength,FNS->upnp->AddressList[i],(unsigned short)FNS->upnp->WebSocketPortNumber,0,FNS->upnp->UDN,"","uuid:",FNS->upnp->UDN,FNS->upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(FNS->upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			break;
			case 3:
			UPnPBuildSsdpNotifyPacket(packet,&packetlength,FNS->upnp->AddressList[i],(unsigned short)FNS->upnp->WebSocketPortNumber,0,FNS->upnp->UDN,"::urn:schemas-wifialliance-org:device:WFADevice:1","urn:schemas-wifialliance-org:device:WFADevice:1","",FNS->upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(FNS->upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			break;
			case 4:
			UPnPBuildSsdpNotifyPacket(packet,&packetlength,FNS->upnp->AddressList[i],(unsigned short)FNS->upnp->WebSocketPortNumber,0,FNS->upnp->UDN,"::urn:schemas-wifialliance-org:service:WFAWLANConfig:1","urn:schemas-wifialliance-org:service:WFAWLANConfig:1","",FNS->upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(FNS->upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			break;


		}
	}
	free(packet);
	if(FNS->packetNumber!=0)
	{
		free(FNS);
	}
}
void UPnPSendNotify(const struct UPnPDataObject *upnp)
{
	int packetlength;
	char* packet = (char*)malloc(5000);
	int i,i2;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPSendNotify\n"));

	for(i=0;i<upnp->AddressListLength;++i)
	{
		ILibAsyncUDPSocket_SetMulticastInterface(upnp->NOTIFY_SEND_socks[i],upnp->AddressList[i]);
		for (i2=0;i2<2;i2++)
		{
			UPnPBuildSsdpNotifyPacket(packet,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::upnp:rootdevice","upnp:rootdevice","",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			UPnPBuildSsdpNotifyPacket(packet,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"","uuid:",upnp->UDN,upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			UPnPBuildSsdpNotifyPacket(packet,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-wifialliance-org:device:WFADevice:1","urn:schemas-wifialliance-org:device:WFADevice:1","",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			UPnPBuildSsdpNotifyPacket(packet,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-wifialliance-org:service:WFAWLANConfig:1","urn:schemas-wifialliance-org:service:WFAWLANConfig:1","",upnp->NotifyCycleTime);
			ILibAsyncUDPSocket_SendTo(upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);

		}
	}
	free(packet);
}

#define UPnPBuildSsdpByeByePacket(outpacket,outlength,USN,USNex,NT,NTex,DeviceID)\
{\
	if(DeviceID==0)\
	{\
		*outlength = sprintf(outpacket,"NOTIFY * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nNTS: ssdp:byebye\r\nUSN: uuid:%s%s\r\nNT: %s%s\r\nContent-Length: 0\r\n\r\n",USN,USNex,NT,NTex);\
	}\
	else\
	{\
		if(memcmp(NT,"uuid:",5)==0)\
		{\
			*outlength = sprintf(outpacket,"NOTIFY * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nNTS: ssdp:byebye\r\nUSN: uuid:%s_%d%s\r\nNT: %s%s_%d\r\nContent-Length: 0\r\n\r\n",USN,DeviceID,USNex,NT,NTex,DeviceID);\
		}\
		else\
		{\
			*outlength = sprintf(outpacket,"NOTIFY * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nNTS: ssdp:byebye\r\nUSN: uuid:%s_%d%s\r\nNT: %s%s\r\nContent-Length: 0\r\n\r\n",USN,DeviceID,USNex,NT,NTex);\
		}\
	}\
}


void UPnPSendByeBye(const struct UPnPDataObject *upnp)
{

	int packetlength;
	char* packet = (char*)malloc(5000);
	int i, i2;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPSendByeBye\n"));

	for(i=0;i<upnp->AddressListLength;++i)
	{
		ILibAsyncUDPSocket_SetMulticastInterface(upnp->NOTIFY_SEND_socks[i],upnp->AddressList[i]);

		for (i2=0;i2<2;i2++)
		{
			UPnPBuildSsdpByeByePacket(packet,&packetlength,upnp->UDN,"::upnp:rootdevice","upnp:rootdevice","",0);
			ILibAsyncUDPSocket_SendTo(upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			UPnPBuildSsdpByeByePacket(packet,&packetlength,upnp->UDN,"","uuid:",upnp->UDN,0);
			ILibAsyncUDPSocket_SendTo(upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			UPnPBuildSsdpByeByePacket(packet,&packetlength,upnp->UDN,"::urn:schemas-wifialliance-org:device:WFADevice:1","urn:schemas-wifialliance-org:device:WFADevice:1","",0);
			ILibAsyncUDPSocket_SendTo(upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);
			UPnPBuildSsdpByeByePacket(packet,&packetlength,upnp->UDN,"::urn:schemas-wifialliance-org:service:WFAWLANConfig:1","urn:schemas-wifialliance-org:service:WFAWLANConfig:1","",0);
			ILibAsyncUDPSocket_SendTo(upnp->NOTIFY_SEND_socks[i],inet_addr(UPNP_GROUP),UPNP_PORT,packet,packetlength,ILibAsyncSocket_MemoryOwnership_USER);

		}
	}
	free(packet);
}

/*! \fn UPnPResponse_Error(const UPnPSessionToken UPnPToken, const int ErrorCode, const char* ErrorMsg)
\brief Responds to the client invocation with a SOAP Fault
\param UPnPToken UPnP token
\param ErrorCode Fault Code
\param ErrorMsg Error Detail
*/
void UPnPResponse_Error(const UPnPSessionToken UPnPToken, const int ErrorCode, const char* ErrorMsg)
{
	char* body;
	int bodylength;
	char* head;
	int headlength;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_Error\n"));

	body = (char*)malloc(395 + (int)strlen(ErrorMsg));
	bodylength = sprintf(body,"<s:Envelope\r\n xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><s:Fault><faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring><detail><UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\"><errorCode>%d</errorCode><errorDescription>%s</errorDescription></UPnPError></detail></s:Fault></s:Body></s:Envelope>",ErrorCode,ErrorMsg);
	head = (char*)malloc(59);
	headlength = sprintf(head,"HTTP/1.1 500 Internal\r\nContent-Length: %d\r\n\r\n",bodylength);
	ILibWebServer_Send_Raw((struct ILibWebServer_Session*)UPnPToken,head,headlength,0,0);
	ILibWebServer_Send_Raw((struct ILibWebServer_Session*)UPnPToken,body,bodylength,0,1);
}

/*! \fn UPnPGetLocalInterfaceToHost(const UPnPSessionToken UPnPToken)
\brief When a UPnP request is dispatched, this method determines which ip address actually received this request
\param UPnPToken UPnP token
\returns IP Address
*/
int UPnPGetLocalInterfaceToHost(const UPnPSessionToken UPnPToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPGetLocalInterfaceToHost\n"));

	return(ILibWebServer_GetLocalInterface((struct ILibWebServer_Session*)UPnPToken));
}

void UPnPResponseGeneric(const UPnPMicroStackToken UPnPToken,const char* ServiceURI,const char* MethodName,const char* Params)
{
	char* packet;
	int packetlength;
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)UPnPToken;
	int RVAL=0;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponseGeneric\n"));

	packet = (char*)malloc(239+strlen(ServiceURI)+strlen(Params)+(strlen(MethodName)*2));
	packetlength = sprintf(packet,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:%sResponse xmlns:u=\"%s\">%s</u:%sResponse></s:Body></s:Envelope>",MethodName,ServiceURI,Params,MethodName);
	LVL3DEBUG(printf("SendBody: %s\r\n",packet);)
	#if defined(WIN32) || defined(_WIN32_WCE)
	RVAL=ILibWebServer_StreamHeader_Raw(session,200,"OK","\r\nEXT:\r\nCONTENT-TYPE: text/xml; charset=\"utf-8\"\r\nSERVER: WINDOWS, UPnP/1.0, Intel MicroStack/1.0.2126",1);
	#else
	RVAL=ILibWebServer_StreamHeader_Raw(session,200,"OK","\r\nEXT:\r\nCONTENT-TYPE: text/xml; charset=\"utf-8\"\r\nSERVER: POSIX, UPnP/1.0, Intel MicroStack/1.0.2126",1);
	#endif
	if(RVAL!=ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RVAL != ILibWebServer_SEND_RESULTED_IN_DISCONNECT)
	{
		RVAL=ILibWebServer_StreamBody(session,packet,packetlength,0,1);
	}
}

/*! \fn UPnPResponse_WFAWLANConfig_DelAPSettings(const UPnPSessionToken UPnPToken)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> DelAPSettings
\param UPnPToken MicroStack token
*/
void UPnPResponse_WFAWLANConfig_DelAPSettings(const UPnPSessionToken UPnPToken)
{
	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","DelAPSettings","");
}

/*! \fn UPnPResponse_WFAWLANConfig_DelSTASettings(const UPnPSessionToken UPnPToken)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> DelSTASettings
\param UPnPToken MicroStack token
*/
void UPnPResponse_WFAWLANConfig_DelSTASettings(const UPnPSessionToken UPnPToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_DelSTASettings\n"));
	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","DelSTASettings","");
}

/*! \fn UPnPResponse_WFAWLANConfig_GetAPSettings(const UPnPSessionToken UPnPToken, const unsigned char* NewAPSettings, const int _NewAPSettingsLength)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> GetAPSettings
\param UPnPToken MicroStack token
\param NewAPSettings Value of argument NewAPSettings
\param NewAPSettingsLength Length of \a NewAPSettings
*/
void UPnPResponse_WFAWLANConfig_GetAPSettings(const UPnPSessionToken UPnPToken, const unsigned char* NewAPSettings, const int _NewAPSettingsLength)
{
	char* body;
	unsigned char* NewAPSettings_Base64;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_GetAPSettings\n"));

	ILibBase64Encode((unsigned char*)NewAPSettings,_NewAPSettingsLength,&NewAPSettings_Base64);
	body = (char*)malloc(32+strlen(NewAPSettings_Base64));
	sprintf(body,"<NewAPSettings>%s</NewAPSettings>",NewAPSettings_Base64);
	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","GetAPSettings",body);
	free(body);
	free(NewAPSettings_Base64);
}

/*! \fn UPnPResponse_WFAWLANConfig_GetDeviceInfo(const UPnPSessionToken UPnPToken, const unsigned char* NewDeviceInfo, const int _NewDeviceInfoLength)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> GetDeviceInfo
\param UPnPToken MicroStack token
\param NewDeviceInfo Value of argument NewDeviceInfo
\param NewDeviceInfoLength Length of \a NewDeviceInfo
*/
void UPnPResponse_WFAWLANConfig_GetDeviceInfo(const UPnPSessionToken UPnPToken, const unsigned char* NewDeviceInfo, const int _NewDeviceInfoLength)
{
	char* body;
	unsigned char* NewDeviceInfo_Base64;
        int NewDeviceInfo_Base64Length;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_GetDeviceInfo\n"));
printf("Enter UPnPResponse_WFAWLANConfig_GetDeviceInfo\r\n");

	NewDeviceInfo_Base64Length = ILibBase64Encode((unsigned char*)NewDeviceInfo,_NewDeviceInfoLength,&NewDeviceInfo_Base64);
	body = (char*)malloc(32+ NewDeviceInfo_Base64Length);
	sprintf(body,"<NewDeviceInfo>%s</NewDeviceInfo>",NewDeviceInfo_Base64);
	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","GetDeviceInfo",body);
	free(body);
	free(NewDeviceInfo_Base64);
}

/*! \fn UPnPResponse_WFAWLANConfig_GetSTASettings(const UPnPSessionToken UPnPToken, const unsigned char* NewSTASettings, const int _NewSTASettingsLength)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> GetSTASettings
\param UPnPToken MicroStack token
\param NewSTASettings Value of argument NewSTASettings
\param NewSTASettingsLength Length of \a NewSTASettings
*/
void UPnPResponse_WFAWLANConfig_GetSTASettings(const UPnPSessionToken UPnPToken, const unsigned char* NewSTASettings, const int _NewSTASettingsLength)
{
	char* body;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_GetSTASettings\n"));

	unsigned char* NewSTASettings_Base64;
	ILibBase64Encode((unsigned char*)NewSTASettings,_NewSTASettingsLength,&NewSTASettings_Base64);
	body = (char*)malloc(34+strlen(NewSTASettings_Base64));
	sprintf(body,"<NewSTASettings>%s</NewSTASettings>",NewSTASettings_Base64);
	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","GetSTASettings",body);
	free(body);
	free(NewSTASettings_Base64);
}

/*! \fn UPnPResponse_WFAWLANConfig_PutMessage(const UPnPSessionToken UPnPToken, const unsigned char* NewOutMessage, const int _NewOutMessageLength)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> PutMessage
\param UPnPToken MicroStack token
\param NewOutMessage Value of argument NewOutMessage
\param NewOutMessageLength Length of \a NewOutMessage
*/
void UPnPResponse_WFAWLANConfig_PutMessage(const UPnPSessionToken UPnPToken, const unsigned char* NewOutMessage, const int _NewOutMessageLength)
{
	char* body;

	unsigned char* NewOutMessage_Base64;
    int NewOutMessage_Base64Length;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_PutMessage\n"));

	NewOutMessage_Base64Length = ILibBase64Encode((unsigned char*)NewOutMessage,_NewOutMessageLength,&NewOutMessage_Base64);
	body = (char*)malloc(32+ NewOutMessage_Base64Length);
	sprintf(body,"<NewOutMessage>%s</NewOutMessage>",NewOutMessage_Base64);
	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","PutMessage",body);
	free(body);
	free(NewOutMessage_Base64);
}

/*! \fn UPnPResponse_WFAWLANConfig_PutWLANResponse(const UPnPSessionToken UPnPToken)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> PutWLANResponse
\param UPnPToken MicroStack token
*/
void UPnPResponse_WFAWLANConfig_PutWLANResponse(const UPnPSessionToken UPnPToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_PutWLANResponse\n"));

	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","PutWLANResponse","");
}

/*! \fn UPnPResponse_WFAWLANConfig_RebootAP(const UPnPSessionToken UPnPToken)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> RebootAP
\param UPnPToken MicroStack token
*/
void UPnPResponse_WFAWLANConfig_RebootAP(const UPnPSessionToken UPnPToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_RebootAP\n"));

	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","RebootAP","");
}

/*! \fn UPnPResponse_WFAWLANConfig_RebootSTA(const UPnPSessionToken UPnPToken)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> RebootSTA
\param UPnPToken MicroStack token
*/
void UPnPResponse_WFAWLANConfig_RebootSTA(const UPnPSessionToken UPnPToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_RebootSTA\n"));

	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","RebootSTA","");
}

/*! \fn UPnPResponse_WFAWLANConfig_ResetAP(const UPnPSessionToken UPnPToken)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> ResetAP
\param UPnPToken MicroStack token
*/
void UPnPResponse_WFAWLANConfig_ResetAP(const UPnPSessionToken UPnPToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_ResetAP\n"));

	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","ResetAP","");
}

/*! \fn UPnPResponse_WFAWLANConfig_ResetSTA(const UPnPSessionToken UPnPToken)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> ResetSTA
\param UPnPToken MicroStack token
*/
void UPnPResponse_WFAWLANConfig_ResetSTA(const UPnPSessionToken UPnPToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_ResetSTA\n"));

	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","ResetSTA","");
}

/*! \fn UPnPResponse_WFAWLANConfig_SetAPSettings(const UPnPSessionToken UPnPToken)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> SetAPSettings
\param UPnPToken MicroStack token
*/
void UPnPResponse_WFAWLANConfig_SetAPSettings(const UPnPSessionToken UPnPToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_SetAPSettings\n"));

	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","SetAPSettings","");
}

/*! \fn UPnPResponse_WFAWLANConfig_SetSelectedRegistrar(const UPnPSessionToken UPnPToken)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> SetSelectedRegistrar
\param UPnPToken MicroStack token
*/
void UPnPResponse_WFAWLANConfig_SetSelectedRegistrar(const UPnPSessionToken UPnPToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_SetSelectedRegistrar\n"));

	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","SetSelectedRegistrar","");
}

/*! \fn UPnPResponse_WFAWLANConfig_SetSTASettings(const UPnPSessionToken UPnPToken, const unsigned char* NewSTASettings, const int _NewSTASettingsLength)
\brief Response Method for WFAWLANConfig >> urn:schemas-wifialliance-org:service:WFAWLANConfig:1 >> SetSTASettings
\param UPnPToken MicroStack token
\param NewSTASettings Value of argument NewSTASettings
\param NewSTASettingsLength Length of \a NewSTASettings
*/
void UPnPResponse_WFAWLANConfig_SetSTASettings(const UPnPSessionToken UPnPToken, const unsigned char* NewSTASettings, const int _NewSTASettingsLength)
{
	char* body;
	unsigned char* NewSTASettings_Base64;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPResponse_WFAWLANConfig_SetSTASettings\n"));

	ILibBase64Encode((unsigned char*)NewSTASettings,_NewSTASettingsLength,&NewSTASettings_Base64);
	body = (char*)malloc(34+strlen(NewSTASettings_Base64));
	sprintf(body,"<NewSTASettings>%s</NewSTASettings>",NewSTASettings_Base64);
	UPnPResponseGeneric(UPnPToken,"urn:schemas-wifialliance-org:service:WFAWLANConfig:1","SetSTASettings",body);
	free(body);
	free(NewSTASettings_Base64);
}



void UPnPSendEventSink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *subscriber,
void *upnp,
int *PAUSE)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPSendEventSink\n"));

	if(done!=0 && ((struct SubscriberInfo*)subscriber)->Disposing==0)
	{
		sem_wait(&(((struct UPnPDataObject*)upnp)->EventLock));
		--((struct SubscriberInfo*)subscriber)->RefCount;
		if(((struct SubscriberInfo*)subscriber)->RefCount==0)
		{
			LVL3DEBUG(printf("\r\n\r\nSubscriber at [%s] %d.%d.%d.%d:%d was/did UNSUBSCRIBE while trying to send event\r\n\r\n",((struct SubscriberInfo*)subscriber)->SID,(((struct SubscriberInfo*)subscriber)->Address&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>8)&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>16)&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>24)&0xFF),((struct SubscriberInfo*)subscriber)->Port);)
			UPnPDestructSubscriberInfo(((struct SubscriberInfo*)subscriber));
		}
		else if(header==NULL)
		{
			LVL3DEBUG(printf("\r\n\r\nCould not deliver event for [%s] %d.%d.%d.%d:%d UNSUBSCRIBING\r\n\r\n",((struct SubscriberInfo*)subscriber)->SID,(((struct SubscriberInfo*)subscriber)->Address&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>8)&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>16)&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>24)&0xFF),((struct SubscriberInfo*)subscriber)->Port);)
			// Could not send Event, so unsubscribe the subscriber
			((struct SubscriberInfo*)subscriber)->Disposing = 1;
			UPnPExpireSubscriberInfo(upnp,subscriber);
		}
		sem_post(&(((struct UPnPDataObject*)upnp)->EventLock));
	}
}
void UPnPSendEvent_Body(void *upnptoken,char *body,int bodylength,struct SubscriberInfo *info)
{
	struct UPnPDataObject* UPnPObject = (struct UPnPDataObject*)upnptoken;
	struct sockaddr_in dest;
	int packetLength;
	char *packet;
	int ipaddr;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPSendEvent_Body\n"));

	memset(&dest,0,sizeof(dest));
	dest.sin_addr.s_addr = info->Address;
	dest.sin_port = htons(info->Port);
	dest.sin_family = AF_INET;
	ipaddr = info->Address;

	//XXX, shit, 483, always find this kind of magic number...
	packet = (char*)malloc(info->PathLength + bodylength + 483);
	// 137, faint...
	packetLength = sprintf(packet,"NOTIFY %s HTTP/1.1\r\nSERVER: %s, UPnP/1.0, Intel MicroStack/1.0.2126\r\nHOST: %s:%d\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nNT: upnp:event\r\nNTS: upnp:propchange\r\nSID: %s\r\nSEQ: %d\r\nContent-Length: %d\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\"?><e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\"><e:property><%s></e:property></e:propertyset>",info->Path,UPnPPLATFORM,inet_ntoa(dest.sin_addr),info->Port,info->SID,info->SEQ,bodylength+137,body);
	++info->SEQ;

	//What's the meaning of incrementing refcount??
	++info->RefCount;
	ILibWebClient_PipelineRequestEx(UPnPObject->EventClient,&dest,packet,packetLength,0,NULL,0,0,&UPnPSendEventSink,info,upnptoken);
}

/*
 * A wrapper function of UPnPSendEvent_Body, filter events other than WFAWLANConfig
 */
void UPnPSendEvent(void *upnptoken, char* body, const int bodylength, const char* eventname)
{
	struct SubscriberInfo *info = NULL;
	struct UPnPDataObject* UPnPObject = (struct UPnPDataObject*)upnptoken;
	//struct sockaddr_in dest;
	LVL3DEBUG(struct timeval tv;)

        TUTRACE((TUTRACE_UPNP, "Enter UPnPSendEvent %s\n", eventname));

	if(UPnPObject==NULL)
	{
		free(body);
		return;
	}
	sem_wait(&(UPnPObject->EventLock));
	if(strncmp(eventname,"WFAWLANConfig",13)==0)
	{
		info = UPnPObject->HeadSubscriberPtr_WFAWLANConfig;
	}

	// XXX, weird, any usage??
	//memset(&dest,0,sizeof(dest));
	while(info!=NULL)
	{
		if(!UPnPSubscriptionExpired(info))
		{
			UPnPSendEvent_Body(upnptoken,body,bodylength,info);
		}
		else
		{
			//Remove Subscriber
			// XXX, where represent remove actions????
			LVL3DEBUG(gettimeofday(&tv,NULL);)
			LVL3DEBUG(printf("\r\n\r\nTIMESTAMP: %d\r\n",tv.tv_sec);)
			LVL3DEBUG(printf("Did not renew [%s] %d.%d.%d.%d:%d UNSUBSCRIBING <%d>\r\n\r\n",((struct SubscriberInfo*)info)->SID,(((struct SubscriberInfo*)info)->Address&0xFF),((((struct SubscriberInfo*)info)->Address>>8)&0xFF),((((struct SubscriberInfo*)info)->Address>>16)&0xFF),((((struct SubscriberInfo*)info)->Address>>24)&0xFF),((struct SubscriberInfo*)info)->Port,info);)
		}

		info = info->Next;
	}

	sem_post(&(UPnPObject->EventLock));
}

/*! \fn UPnPSetState_WFAWLANConfig_STAStatus(UPnPMicroStackToken upnptoken, unsigned char val)
\brief Sets the state of STAStatus << urn:schemas-wifialliance-org:service:WFAWLANConfig:1 << WFAWLANConfig \par
\b Note: Must be called at least once prior to start
\param upnptoken The MicroStack token
\param val The new value of the state variable
*/
void UPnPSetState_WFAWLANConfig_STAStatus(UPnPMicroStackToken upnptoken, unsigned char val)
{
	struct UPnPDataObject *UPnPObject = (struct UPnPDataObject*)upnptoken;
	char* body;
	int bodylength;
	char* valstr;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPSetState_WFAWLANConfig_STAStatus\n"));

	valstr = (char*)malloc(10);
	sprintf(valstr,"%d",val);
	if (UPnPObject->WFAWLANConfig_STAStatus != NULL) free(UPnPObject->WFAWLANConfig_STAStatus);
	UPnPObject->WFAWLANConfig_STAStatus = valstr;
	body = (char*)malloc(28 + (int)strlen(valstr));
	bodylength = sprintf(body,"%s>%s</%s","STAStatus",valstr,"STAStatus");
	UPnPSendEvent(upnptoken,body,bodylength,"WFAWLANConfig");
	free(body);
}

/*! \fn UPnPSetState_WFAWLANConfig_APStatus(UPnPMicroStackToken upnptoken, unsigned char val)
\brief Sets the state of APStatus << urn:schemas-wifialliance-org:service:WFAWLANConfig:1 << WFAWLANConfig \par
\b Note: Must be called at least once prior to start
\param upnptoken The MicroStack token
\param val The new value of the state variable
*/
void UPnPSetState_WFAWLANConfig_APStatus(UPnPMicroStackToken upnptoken, unsigned char val)
{
	struct UPnPDataObject *UPnPObject = (struct UPnPDataObject*)upnptoken;
	char* body;
	int bodylength;
	char* valstr;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPSetState_WFAWLANConfig_APStatus\n"));

	valstr = (char*)malloc(10);
	sprintf(valstr,"%d",val);
	if (UPnPObject->WFAWLANConfig_APStatus != NULL) free(UPnPObject->WFAWLANConfig_APStatus);
	UPnPObject->WFAWLANConfig_APStatus = valstr;
	body = (char*)malloc(26 + (int)strlen(valstr));
	bodylength = sprintf(body,"%s>%s</%s","APStatus",valstr,"APStatus");
	UPnPSendEvent(upnptoken,body,bodylength,"WFAWLANConfig");
	free(body);
}

/*! \fn UPnPSetState_WFAWLANConfig_WLANEvent(UPnPMicroStackToken upnptoken, unsigned char* val,int vallen)
\brief Sets the state of WLANEvent << urn:schemas-wifialliance-org:service:WFAWLANConfig:1 << WFAWLANConfig \par
\b Note: Must be called at least once prior to start
\param upnptoken The MicroStack token
\param val The new value of the state variable
\param vallen Length of \a val
*/
void UPnPSetState_WFAWLANConfig_WLANEvent(UPnPMicroStackToken upnptoken, unsigned char* val,int vallen)
{
	struct UPnPDataObject *UPnPObject = (struct UPnPDataObject*)upnptoken;
	char* body;
	int bodylength;
	unsigned char* valstr;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPSetState_WFAWLANConfig_WLANEvent\n"));

        ILibBase64Encode(val,vallen,&valstr);
	if (UPnPObject->WFAWLANConfig_WLANEvent != NULL) free(UPnPObject->WFAWLANConfig_WLANEvent);
	UPnPObject->WFAWLANConfig_WLANEvent = valstr;
	body = (char*)malloc(28 + (int)strlen(valstr));
	bodylength = sprintf(body,"%s>%s</%s","WLANEvent",valstr,"WLANEvent");

        TUTRACE((TUTRACE_UPNP, "send WFAWLANConfig =%s\n",body));

	UPnPSendEvent(upnptoken,body,bodylength,"WFAWLANConfig");
	free(body);
}



void UPnPDestroyMicroStack(void *object)
{
	struct UPnPDataObject *upnp = (struct UPnPDataObject*)object;
	struct SubscriberInfo  *sinfo,*sinfo2;
        TUTRACE((TUTRACE_UPNP, "Enter UPnPDestroyMicroStack\n"));

	UPnPSendByeBye(upnp);

	sem_destroy(&(upnp->EventLock));

	free(upnp->WFAWLANConfig_STAStatus);
	free(upnp->WFAWLANConfig_APStatus);
	free(upnp->WFAWLANConfig_WLANEvent);


	free(upnp->AddressList);
	free(upnp->NOTIFY_SEND_socks);
	free(upnp->UUID);
	free(upnp->Serial);
	free(upnp->DeviceDescription);

	sinfo = upnp->HeadSubscriberPtr_WFAWLANConfig;
	while(sinfo!=NULL)
	{
		sinfo2 = sinfo->Next;
		UPnPDestructSubscriberInfo(sinfo);
		sinfo = sinfo2;
	}

}
int UPnPGetLocalPortNumber(UPnPSessionToken token)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPGetLocalPortNumber\n"));

	return(ILibWebServer_GetPortNumber(((struct ILibWebServer_Session*)token)->Parent));
}
void UPnPSessionReceiveSink(
struct ILibWebServer_Session *sender,
int InterruptFlag,
struct packetheader *header,
char *bodyBuffer,
int *beginPointer,
int endPointer,
int done)
{

        TUTRACE((TUTRACE_UPNP, "Enter UPnPSessionReceiveSink\n"));

	char *txt;
	if(header!=NULL && sender->User3==NULL && done==0)
	{
		sender->User3 = (void*)~0;
		txt = ILibGetHeaderLine(header,"Expect",6);
		if(txt!=NULL)
		{
			if(strcasecmp(txt,"100-Continue")==0)
			{
				//
				// Expect Continue
				//
				ILibWebServer_Send_Raw(sender,"HTTP/1.1 100 Continue\r\n\r\n",25,ILibAsyncSocket_MemoryOwnership_STATIC,0);
			}
			else
			{
				//
				// Don't understand
				//
				ILibWebServer_Send_Raw(sender,"HTTP/1.1 417 Expectation Failed\r\n\r\n",35,ILibAsyncSocket_MemoryOwnership_STATIC,1);
				ILibWebServer_DisconnectSession(sender);
				return;
			}
		}
	}

	if(header!=NULL && done !=0 && InterruptFlag==0)
	{
		UPnPProcessHTTPPacket(sender,header,bodyBuffer,beginPointer==NULL?0:*beginPointer,endPointer);
		if(beginPointer!=NULL) {*beginPointer = endPointer;}
	}
}
void UPnPSessionSink(struct ILibWebServer_Session *SessionToken, void *user)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPSessionSink\n"));
	SessionToken->OnReceive = &UPnPSessionReceiveSink;
	SessionToken->User = user;
}
void UPnPSetTag(const UPnPMicroStackToken token, void *UserToken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPSetTag\n"));

	((struct UPnPDataObject*)token)->User = UserToken;
}
void *UPnPGetTag(const UPnPMicroStackToken token)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPGetTag\n"));

	return(((struct UPnPDataObject*)token)->User);
}
UPnPMicroStackToken UPnPGetMicroStackTokenFromSessionToken(const UPnPSessionToken token)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnPGetMicroStackTokenFromSessionToken\n"));

	return(((struct ILibWebServer_Session*)token)->User);
}
UPnPMicroStackToken UPnPCreateMicroStack(void *Chain, const char* FriendlyName,const char* UDN, const char* SerialNumber, const int NotifyCycleSeconds, const unsigned short PortNum)

{
	struct UPnPDataObject* RetVal = (struct UPnPDataObject*)malloc(sizeof(struct UPnPDataObject));
	char* DDT;

	struct timeval tv;

        TUTRACE((TUTRACE_UPNP, "Enter UPnPCreateMicroStack\n"));

	gettimeofday(&tv,NULL);
	srand((int)tv.tv_sec);



	/* Complete State Reset */
	memset(RetVal,0,sizeof(struct UPnPDataObject));

	RetVal->ForceExit = 0;
	RetVal->PreSelect = &UPnPMasterPreSelect;
	RetVal->PostSelect = NULL;
	RetVal->Destroy = &UPnPDestroyMicroStack;
	RetVal->InitialNotify = 1;      //Set this to 1, disable initial notify sending.
	if (UDN != NULL)
	{
		RetVal->UUID = (char*)malloc((int)strlen(UDN)+6);
		sprintf(RetVal->UUID,"uuid:%s",UDN);
		RetVal->UDN = RetVal->UUID + 5;
	}
	if (SerialNumber != NULL)
	{
		RetVal->Serial = (char*)malloc((int)strlen(SerialNumber)+1);
		strcpy(RetVal->Serial,SerialNumber);
	}


	RetVal->DeviceDescription = (char*)malloc(10+UPnPDeviceDescriptionTemplateLengthUX+ (int)strlen(FriendlyName)  + (((int)strlen(RetVal->Serial) + (int)strlen(RetVal->UUID)) * 1));



	RetVal->WebServerTimer = ILibCreateLifeTime(Chain);

	// Create web server, web server state module, socket module ...
	// Would have a server listen on port WebSocketPortNumber, which is indicated by PortNum
	// This should be the control url??
	RetVal->HTTPServer = ILibWebServer_Create(Chain,UPNP_HTTP_MAXSOCKETS,PortNum,&UPnPSessionSink,RetVal);
	RetVal->WebSocketPortNumber=(int)ILibWebServer_GetPortNumber(RetVal->HTTPServer);



	ILibAddToChain(Chain,RetVal);
	// Create UPnP ssdp discover recv and Notify related modules.
	// For function discover ?
	UPnPInit(RetVal,Chain,NotifyCycleSeconds,PortNum);

	// For event ??
	RetVal->EventClient = ILibCreateWebClient(5,Chain);
	RetVal->UpdateFlag = 0;

	// What kind of compressed algorithm???
	DDT = ILibDecompressString((char*)UPnPDeviceDescriptionTemplate,UPnPDeviceDescriptionTemplateLength,UPnPDeviceDescriptionTemplateLengthUX);
	RetVal->DeviceDescriptionLength = sprintf(RetVal->DeviceDescription,DDT,FriendlyName,RetVal->Serial,RetVal->UDN);

	free(DDT);


	sem_init(&(RetVal->EventLock),0,1);
	return(RetVal);
}


void UPnP_MS_ILibWebServer_AddRef(void * upnptoken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnP_MS_ILibWebServer_AddRef\n"));
	ILibWebServer_AddRef((struct ILibWebServer_Session*)upnptoken);
}

void UPnP_MS_ILibWebServer_Release(void * upnptoken)
{
        TUTRACE((TUTRACE_UPNP, "Enter UPnP_MS_ILibWebServer_Release\n"));
	ILibWebServer_Release((struct ILibWebServer_Session*)upnptoken);
}


