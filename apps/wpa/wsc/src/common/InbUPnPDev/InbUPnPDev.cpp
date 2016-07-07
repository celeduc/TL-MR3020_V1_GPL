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
//  File Name: InbUPnPDev.cpp
//  Description: This file contains implementation of functions for the
//               Inband EAP manager.
//
//==========================================================================*/

#include <stdio.h>
#ifdef __linux__
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>
#endif // __linux__
#ifdef WIN32
#include <windows.h>
#endif // WIN32

#include "tutrace.h"
#include "slist.h"
#include "WscCommon.h"
#include "WscError.h"
#include "Portability.h"
#include "ILibParsers.h"
#include "ILibWebServer.h"
#include "InbUPnPDev.h"
#include "UPnPMicroStack.h"
#include "RegProtoTlv.h"
#include "WscHeaders.h"

#ifdef __linux__
#define SLEEP(X) sleep(X)
#endif // __linux__
#ifdef WIN32
#define SLEEP(X) Sleep(X * 1000)
#endif // WIN32

CInbUPnPDev * g_inbUPnPDev;

void UPnPWFAWLANConfig_DelAPSettings(UPnPSessionToken upnptoken,unsigned char* NewAPSettings,int _NewAPSettingsLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_DelAPSettings(BINARY(%d));\r\n",_NewAPSettingsLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_DelAPSettings(upnptoken);
}

void UPnPWFAWLANConfig_DelSTASettings(UPnPSessionToken upnptoken,unsigned char* NewSTASettings,int _NewSTASettingsLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_DelSTASettings(BINARY(%d));\r\n",_NewSTASettingsLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_DelSTASettings(upnptoken);
}

void UPnPWFAWLANConfig_GetAPSettings(UPnPSessionToken upnptoken,unsigned char* NewMessage,int _NewMessageLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_GetAPSettings(BINARY(%d));\r\n",_NewMessageLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_GetAPSettings(upnptoken,(const uint8 *)"Sample Binary",13);
}

#if 1
int SendWscEvent(char * pEvent)
{
    #define WSC_UDP_ADDR   "127.0.0.1"
    #define WSC_EVENT_PORT  38100

    static int upnp_sock = 0;
    struct sockaddr_in to;

    if(upnp_sock==0)
    {
        upnp_sock = (int) socket(AF_INET, SOCK_DGRAM, 0);
    }

    bzero(&to,sizeof(to));

    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr(WSC_UDP_ADDR);
    to.sin_port = htons(WSC_EVENT_PORT);

    if (sendto(upnp_sock, pEvent, strlen(pEvent), 0,
              (struct sockaddr *) &to, sizeof(to))<0)
    {
        TUTRACE((TUTRACE_ERR, "sending WSC config message failed\n"));
        return -1;
    }
    TUTRACE((TUTRACE_DBG, "WSC config send to WSC\n"));

}
#endif

void UPnPWFAWLANConfig_GetDeviceInfo(UPnPSessionToken upnptoken)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_GetDeviceInfo();\r\n"));

        //SendWscEvent("WSC_UPNP_GETDEVINFO");
	//TUTRACE((TUTRACE_UPNP, "WSC_UPNP_GETDEVINFO send;\r\n"));
printf("Invoke: UPnPWFAWLANConfig_GetDeviceInfo();\r\n");


	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

    printf("UPnPWFAWLANConfig_GetDeviceInfo(): g_inbUPnPDev = %08x\r\n", g_inbUPnPDev);
    if (g_inbUPnPDev)
    {
        UPnP_MS_ILibWebServer_AddRef(upnptoken);
        g_inbUPnPDev->m_waitForGetDevInfoResp = true;
        g_inbUPnPDev->m_saved_upnptoken = upnptoken;

        g_inbUPnPDev->InvokeCallback(NULL, 0);  //this mean with NO INMSG, and len==0,
                                                // ssr==false.
	}
	else
	{
		UPnPResponse_WFAWLANConfig_GetDeviceInfo(upnptoken,(const uint8 *)"Sample Binary",13);
	}

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	// UPnPResponse_WFAWLANConfig_GetDeviceInfo(upnptoken,(const uint8 *)"Sample Binary",13);
}

void UPnPWFAWLANConfig_GetSTASettings(UPnPSessionToken upnptoken,unsigned char* NewMessage,int _NewMessageLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_GetSTASettings(BINARY(%d));\r\n",_NewMessageLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_GetSTASettings(upnptoken,(const uint8 *)"Sample Binary",13);
}

void UPnPWFAWLANConfig_PutMessage(UPnPSessionToken upnptoken,unsigned char* NewInMessage,int _NewInMessageLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_PutMessage(BINARY(%d));\r\n",_NewInMessageLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	if (g_inbUPnPDev)
	{
            while (g_inbUPnPDev->m_waitForPutMessageResp == true) {
                    TUTRACE((TUTRACE_DBG, "wait m_waitForPutMessageResp...\n"));
                    sleep(1);
        	}

		UPnP_MS_ILibWebServer_AddRef(upnptoken);
		g_inbUPnPDev->m_waitForPutMessageResp = true;
		g_inbUPnPDev->m_saved_upnptoken = upnptoken;

        	g_inbUPnPDev->InvokeCallback((char *)NewInMessage, _NewInMessageLength);
	} else {
		UPnPResponse_WFAWLANConfig_PutMessage(upnptoken,(const uint8 *)"Sample Binary",13);
	}

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	// UPnPResponse_WFAWLANConfig_PutMessage(upnptoken,(const uint8 *)"Sample Binary",13);
}

void UPnPWFAWLANConfig_PutWLANResponse(UPnPSessionToken upnptoken,unsigned char* NewMessage,int _NewMessageLength,unsigned char NewWLANEventType,char* NewWLANEventMAC)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_PutWLANResponse(BINARY(%d),%u,%s);\r\n",_NewMessageLength,NewWLANEventType,NewWLANEventMAC));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */
    	if (g_inbUPnPDev) {
        	g_inbUPnPDev->InvokeCallback((char *)NewMessage, _NewMessageLength);
    	}

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_PutWLANResponse(upnptoken);
}

void UPnPWFAWLANConfig_RebootAP(UPnPSessionToken upnptoken,unsigned char* NewAPSettings,int _NewAPSettingsLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_RebootAP(BINARY(%d));\r\n",_NewAPSettingsLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_RebootAP(upnptoken);
}

void UPnPWFAWLANConfig_RebootSTA(UPnPSessionToken upnptoken,unsigned char* NewSTASettings,int _NewSTASettingsLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_RebootSTA(BINARY(%d));\r\n",_NewSTASettingsLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_RebootSTA(upnptoken);
}

void UPnPWFAWLANConfig_ResetAP(UPnPSessionToken upnptoken,unsigned char* NewMessage,int _NewMessageLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_ResetAP(BINARY(%d));\r\n",_NewMessageLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_ResetAP(upnptoken);
}

void UPnPWFAWLANConfig_ResetSTA(UPnPSessionToken upnptoken,unsigned char* NewMessage,int _NewMessageLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_ResetSTA(BINARY(%d));\r\n",_NewMessageLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_ResetSTA(upnptoken);
}

void UPnPWFAWLANConfig_SetAPSettings(UPnPSessionToken upnptoken,unsigned char* APSettings,int _APSettingsLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_SetAPSettings(BINARY(%d));\r\n",_APSettingsLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_SetAPSettings(upnptoken);
}

void UPnPWFAWLANConfig_SetSelectedRegistrar(UPnPSessionToken upnptoken,unsigned char* NewMessage,int _NewMessageLength)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_SetSelectedRegistrar(BINARY(%d));\r\n",_NewMessageLength));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

    if (g_inbUPnPDev)
    {
        g_inbUPnPDev->InvokeCallback((char *)NewMessage, _NewMessageLength, true);
    }
	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_SetSelectedRegistrar(upnptoken);
}

void UPnPWFAWLANConfig_SetSTASettings(UPnPSessionToken upnptoken)
{
	TUTRACE((TUTRACE_UPNP, "Invoke: UPnPWFAWLANConfig_SetSTASettings();\r\n"));

	/* If you intend to make the response later, you MUST reference count upnptoken with calls to ILibWebServer_AddRef() */
	/* and ILibWebServer_Release() */

	/* TODO: Place Action Code Here... */

	/* UPnPResponse_Error(upnptoken,404,"Method Not Implemented"); */
	UPnPResponse_WFAWLANConfig_SetSTASettings(upnptoken,(const uint8 *)"Sample Binary",13);
}


CInbUPnPDev::CInbUPnPDev()
{
    TUTRACE((TUTRACE_UPNP, "CInbUPnPDev Construction\n"));
    g_inbUPnPDev = this;
    m_UPnPThreadHandle = 0;
    m_microStackChain = NULL;
    m_UPnPmicroStack = NULL;
    m_waitForGetDevInfoResp = false;
    m_waitForPutMessageResp = false;
	m_saved_upnptoken = NULL;
}

CInbUPnPDev::~CInbUPnPDev()
{
    TUTRACE((TUTRACE_UPNP, "CInbUPnPDev Destruction\n"));
    g_inbUPnPDev = NULL;
    Deinit();
}

uint32 CInbUPnPDev::Init()
{
    m_initialized = true;
    return WSC_SUCCESS;
}

uint32 CInbUPnPDev::StartMonitor()
{
    uint32 retVal;

    TUTRACE((TUTRACE_UPNP, "CInbUPnPDev StartMonitor\n"));
    if ( ! m_initialized)
    {
        return WSC_ERR_NOT_INITIALIZED;
    }

    retVal = WscCreateThread(&m_UPnPThreadHandle, StaticUPnPThread, this);

    if (retVal != WSC_SUCCESS)
    {
        TUTRACE((TUTRACE_ERR,  "CreateThread failed.\n"));
        return retVal;
    }

    SLEEP(0);

    return WSC_SUCCESS;
}

uint32 CInbUPnPDev::StopMonitor()
{
    TUTRACE((TUTRACE_UPNP, "CInbUPnPDev StopMonitor\n"));
    if ( ! m_initialized)
    {
        return WSC_ERR_NOT_INITIALIZED;
    }

	ILibStopChain(m_microStackChain);
    SLEEP(1);

    return WSC_SUCCESS;
}

uint32 CInbUPnPDev::WriteData(char * dataBuffer, uint32 dataLen)
{
    uint32 retVal = WSC_SUCCESS;
    char *upnpData;
    uint32 upnpLen;

    TUTRACE((TUTRACE_UPNP, "In CInbUPnPDev::WriteData buffer Length = %d\n",
            dataLen));

printf("WriteData(): begins\r\n");
printf("WriteData(): dataBuffer = %08x\r\n", dataBuffer);
printf("WriteData(): dataLen = %d\r\n", dataLen);
    if ( (! dataBuffer) || (! dataLen))
    {
        TUTRACE((TUTRACE_ERR, "Invalid Parameters\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

	if (m_waitForGetDevInfoResp)
	{
printf("WriteData(): m_waitForGetDevInfoResp\r\n");
        UPnPResponse_WFAWLANConfig_GetDeviceInfo(
            m_saved_upnptoken, (const uint8 *) dataBuffer, dataLen);
    UPnP_MS_ILibWebServer_Release(m_saved_upnptoken);
    m_waitForGetDevInfoResp = false;
    m_saved_upnptoken = NULL;
    }
    else if (m_waitForPutMessageResp)
    {
    UPnPResponse_WFAWLANConfig_PutMessage(m_saved_upnptoken,
                (const uint8 *) dataBuffer, dataLen);
        UPnP_MS_ILibWebServer_Release(m_saved_upnptoken);
        m_waitForPutMessageResp = false;
        m_saved_upnptoken = NULL;
    }
	else
	{

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif
        // Intel patch add the UPnP header. Total of 18 bytes.
            unsigned char *tmp = (unsigned char *)dataBuffer;
            static char mac[sizeof(MACSTR)];

            if ( *(tmp + 9) == 0x4) {
            // Get station MAC address from M1
            snprintf(mac, sizeof(mac), MACSTR, MAC2STR(tmp+34));
	       		TUTRACE((TUTRACE_DBG, "mac=%s\n", mac));
	        }

		upnpLen = dataLen + 18;
		upnpData = (char *)malloc(upnpLen);
		upnpData[0] = 0x2;
		memcpy(upnpData+1,mac,17);
		memcpy(upnpData+18, dataBuffer, dataLen);
		UPnPSetState_WFAWLANConfig_WLANEvent(GetUPnPMicroStack(),
			     (uint8 *) upnpData, upnpLen);
		free(upnpData);

	}

    return retVal;
}

uint32 CInbUPnPDev::ReadData(char * dataBuffer, uint32 * dataLen)
{
    TUTRACE((TUTRACE_ERR, "In CInbUPnPDev::ReadData; NOT IMPLEMENTED\n"));
    return WSC_ERR_NOT_IMPLEMENTED;
}

uint32 CInbUPnPDev::Deinit()
{
    TUTRACE((TUTRACE_UPNP, "In CInbUPnPDev::Deinit\n"));
    if ( ! m_initialized)
    {
        TUTRACE((TUTRACE_ERR, "Not initialized; Returning\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    m_initialized = false;

    return WSC_SUCCESS;
}

void * CInbUPnPDev::StaticUPnPThread(IN void *p_data)
{
    TUTRACE((TUTRACE_UPNP, "In CInbUPnPDev::StaticUPnPThread\n"));
    ((CInbUPnPDev *)p_data)->ActualUPnPThread();
    return 0;
}

uint32 BuildMessageAck(BufferObj &msg);

void * CInbUPnPDev::ActualUPnPThread()
{
    TUTRACE((TUTRACE_UPNP, "CInbUPnPDev::ActualUPnPThread Started\n"));

	m_microStackChain = ILibCreateChain();
#if 1	// muyz, specify the http/soap port
	// Currently write const uuid
	m_UPnPmicroStack = UPnPCreateMicroStack(m_microStackChain,"WFADevice","565aa949-67c1-4c0e-aa8f-f349e6f59311","0000001",1800,1910);
#else
    m_UPnPmicroStack = UPnPCreateMicroStack(m_microStackChain,"WFADevice","565aa949-67c1-4c0e-aa8f-f349e6f59311","0000001",1800,0);
#endif
	// Assigned Action Handler
    UPnPFP_WFAWLANConfig_DelAPSettings=(UPnP_ActionHandler_WFAWLANConfig_DelAPSettings)&UPnPWFAWLANConfig_DelAPSettings;
    UPnPFP_WFAWLANConfig_DelSTASettings=(UPnP_ActionHandler_WFAWLANConfig_DelSTASettings)&UPnPWFAWLANConfig_DelSTASettings;
    UPnPFP_WFAWLANConfig_GetAPSettings=(UPnP_ActionHandler_WFAWLANConfig_GetAPSettings)&UPnPWFAWLANConfig_GetAPSettings;
    UPnPFP_WFAWLANConfig_GetDeviceInfo=(UPnP_ActionHandler_WFAWLANConfig_GetDeviceInfo)&UPnPWFAWLANConfig_GetDeviceInfo;
    UPnPFP_WFAWLANConfig_GetSTASettings=(UPnP_ActionHandler_WFAWLANConfig_GetSTASettings)&UPnPWFAWLANConfig_GetSTASettings;
    UPnPFP_WFAWLANConfig_PutMessage=(UPnP_ActionHandler_WFAWLANConfig_PutMessage)&UPnPWFAWLANConfig_PutMessage;
    UPnPFP_WFAWLANConfig_PutWLANResponse=(UPnP_ActionHandler_WFAWLANConfig_PutWLANResponse)&UPnPWFAWLANConfig_PutWLANResponse;
    UPnPFP_WFAWLANConfig_RebootAP=(UPnP_ActionHandler_WFAWLANConfig_RebootAP)&UPnPWFAWLANConfig_RebootAP;
    UPnPFP_WFAWLANConfig_RebootSTA=(UPnP_ActionHandler_WFAWLANConfig_RebootSTA)&UPnPWFAWLANConfig_RebootSTA;
    UPnPFP_WFAWLANConfig_ResetAP=(UPnP_ActionHandler_WFAWLANConfig_ResetAP)&UPnPWFAWLANConfig_ResetAP;
    UPnPFP_WFAWLANConfig_ResetSTA=(UPnP_ActionHandler_WFAWLANConfig_ResetSTA)&UPnPWFAWLANConfig_ResetSTA;
    UPnPFP_WFAWLANConfig_SetAPSettings=(UPnP_ActionHandler_WFAWLANConfig_SetAPSettings)&UPnPWFAWLANConfig_SetAPSettings;
    UPnPFP_WFAWLANConfig_SetSelectedRegistrar=(UPnP_ActionHandler_WFAWLANConfig_SetSelectedRegistrar)&UPnPWFAWLANConfig_SetSelectedRegistrar;
    UPnPFP_WFAWLANConfig_SetSTASettings=(UPnP_ActionHandler_WFAWLANConfig_SetSTASettings)&UPnPWFAWLANConfig_SetSTASettings;

	/* All evented state variables MUST be initialized before UPnPStart is called. */
    UPnPSetState_WFAWLANConfig_STAStatus(GetUPnPMicroStack(),WSC_UPNP_DEV_STATUS_CHANGED);
    UPnPSetState_WFAWLANConfig_APStatus(GetUPnPMicroStack(),WSC_UPNP_DEV_STATUS_CHANGED);

	BufferObj ack;
	if (BuildMessageAck(ack) == WSC_SUCCESS) {
		// Initialize this state variable to an WSC_ACK message with NULL nonces.  This is a
		// harmless message that should be ignored by any recipients, but it will at least be
		// well-formed.
		int upnpLen = ack.Length() + 18;
		char * upnpData = (char *)malloc(upnpLen);
		if (upnpData) {
			upnpData[0] = WSC_WLAN_EVENT_TYPE_EAP_FRAME;
			// WARNING.. hard-coded "client MAC address" is used here.  This is a bogus message anyway.
			sprintf(upnpData+1, "00:09:6B:E0:B3:12");
			memcpy(upnpData+18, ack.GetBuf(), ack.Length());
			UPnPSetState_WFAWLANConfig_WLANEvent(GetUPnPMicroStack(),(uint8 *)upnpData,upnpLen);
			free(upnpData);
		}
	} else { // fall back to bogus data, should never reach here.
	    UPnPSetState_WFAWLANConfig_WLANEvent(GetUPnPMicroStack(),(uint8 *)"Sample Binary",13);
	}

    TUTRACE((TUTRACE_UPNP, "Calling ILibStartChain\n"));

    // Here is the place things got done.
    ILibStartChain(m_microStackChain);

    TUTRACE((TUTRACE_UPNP, "ActualUPnPThread Finished\n"));
    return 0;
}

void CInbUPnPDev::InvokeCallback(char * buf, uint32 len, bool ssr)
{
    char * sendBuf;
    S_CB_COMMON * upnpComm;

    printf("CInbUPnPDev::InvokeCallback(), %d\r\n", __LINE__);
    sendBuf = new char[sizeof(S_CB_COMMON) + len];
    printf("CInbUPnPDev::InvokeCallback(): sendBuf = %08x, %d\r\n", sendBuf, __LINE__);
    if (sendBuf == NULL)
    {
        TUTRACE((TUTRACE_ERR, "Allocating memory for Sendbuf failed\n"));
        return;
    }
    // call callback
    upnpComm = (S_CB_COMMON *) sendBuf;
    if (ssr)
        upnpComm->cbHeader.eType = CB_TRUPNP_DEV_SSR;
    else
        upnpComm->cbHeader.eType = CB_TRUPNP_DEV;
    upnpComm->cbHeader.dataLength = len;

    if (buf) {
        memcpy(sendBuf + sizeof(S_CB_COMMON), buf, len);
    }


    if (m_trCallbackInfo.pf_callback)
    {
        TUTRACE((TUTRACE_UPNP, "UPnPDev: Calling Transport Callback\n"));
        printf("UPnPDev: Calling Transport Callback: buf = %08x, cookie = %08x, %d\r\n",
                        sendBuf, m_trCallbackInfo.p_cookie, __LINE__);
        m_trCallbackInfo.pf_callback(sendBuf, m_trCallbackInfo.p_cookie);
        TUTRACE((TUTRACE_UPNP, "Transport Callback Returned\n"));
    }
    else
    {
        printf("CInbUPnPDev::InvokeCallback(): no callback function set\r\n");
        TUTRACE((TUTRACE_ERR, "No Callback function set\n"));
    }
}

uint32 BuildMessageAck(BufferObj &msg)
{
    	uint8 message = WSC_ID_MESSAGE_ACK;
	uint8 version = WSC_VERSION;
	uint8 null_nonce[16];
	memset(null_nonce,'\0',sizeof(null_nonce));
	try
	{
		CTlvVersion(WSC_ID_VERSION, msg, &version);
		CTlvMsgType(WSC_ID_MSG_TYPE, msg, &message);
		CTlvEnrolleeNonce(
				WSC_ID_ENROLLEE_NONCE,
				msg,
				null_nonce,
				SIZE_128_BITS);
		CTlvRegistrarNonce(
				WSC_ID_REGISTRAR_NONCE,
				msg,
				null_nonce,
				SIZE_128_BITS);
		return WSC_SUCCESS;
	}
	catch(uint32 err)
	{
		TUTRACE((TUTRACE_ERR, "UPnPDev: BuildMessageAck generated an "
			 "error: %d\n", err));
		return err;
	}
	catch(char *str)
	{
		TUTRACE((TUTRACE_ERR, "UPnPDev: BuildMessageAck generated an "
			 "exception: %s\n", str));
		return WSC_ERR_SYSTEM;
	}
	catch(...)
	{
		TUTRACE((TUTRACE_ERR, "UPnPDev: BuildMessageAck generated an "
			 "unknown exception\n"));
		return WSC_ERR_SYSTEM;
	}
}//BuildMessageAck
