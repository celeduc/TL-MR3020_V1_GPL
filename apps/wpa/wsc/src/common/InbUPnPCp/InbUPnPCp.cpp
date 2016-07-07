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
//  File Name: InbUPnPCp.cpp
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
#include "UPnPControlPoint.h"
#include "InbUPnPCp.h"

#ifdef __linux__
#define SLEEP(X) sleep(X)
#endif // __linux__
#ifdef WIN32
#define SLEEP(X) Sleep(X * 1000)
#endif // WIN32

CInbUPnPCp * g_inbUPnPCp;

void UPnPEventSink_WFAWLANConfig_STAStatus(struct UPnPService* Service,unsigned char STAStatus)
{
    static bool firstTime = true;

    TUTRACE((TUTRACE_UPNP, "UPnP Event from %s/WFAWLANConfig/STAStatus: %u\r\n",Service->Parent->FriendlyName,STAStatus));

    if (firstTime)
    {
        TUTRACE((TUTRACE_UPNP, "First Time; Returning\n"));
        firstTime = false;
        return;
    }
}

void UPnPEventSink_WFAWLANConfig_APStatus(struct UPnPService* Service,unsigned char APStatus)
{
    static bool firstTime = true;

	TUTRACE((TUTRACE_UPNP, "UPnP Event from %s/WFAWLANConfig/APStatus: %u\r\n",Service->Parent->FriendlyName,APStatus));

    if (firstTime)
    {
        TUTRACE((TUTRACE_UPNP, "First Time; Returning\n"));
        firstTime = false;
        return;
    }
}

void UPnPEventSink_WFAWLANConfig_WLANEvent(struct UPnPService* Service,unsigned char* WLANEvent,int WLANEventLength)
{
    static bool firstTime = true;

	TUTRACE((TUTRACE_UPNP, "UPnP Event from %s/WFAWLANConfig/WLANEvent: [BINARY:%d]\r\n",Service->Parent->FriendlyName,WLANEventLength));

    if (firstTime)
    {
        TUTRACE((TUTRACE_UPNP, "First Time; Returning\n"));
        firstTime = false;
        return;
    }

    if (g_inbUPnPCp)
    {
        g_inbUPnPCp->m_waitForWlanEventResp = true;
		// Now strip off the UPnP header containing the length and MAC address of the wireless client
		const uint8 EventTypeEAP = 2;

		if (WLANEventLength > 18 && (WLANEvent[0] == EventTypeEAP)) {
			g_inbUPnPCp->InvokeCallback((char *)WLANEvent + 18, WLANEventLength - 18);
		}
    }
}


void UPnPResponseSink_WFAWLANConfig_DelAPSettings(struct UPnPService* Service,int ErrorCode,void *User)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/DelAPSettings[ErrorCode:%d]()\r\n",ErrorCode));
}

void UPnPResponseSink_WFAWLANConfig_DelSTASettings(struct UPnPService* Service,int ErrorCode,void *User)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/DelSTASettings[ErrorCode:%d]()\r\n",ErrorCode));
}

void UPnPResponseSink_WFAWLANConfig_GetAPSettings(struct UPnPService* Service,int ErrorCode,void *User,unsigned char* NewAPSettings,int NewAPSettingsLength)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/GetAPSettings[ErrorCode:%d](%s)\r\n",ErrorCode,NewAPSettings));
}

void UPnPResponseSink_WFAWLANConfig_GetDeviceInfo(struct UPnPService* Service,int ErrorCode,void *User,unsigned char* NewDeviceInfo,int NewDeviceInfoLength)
{
	//TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/GetDeviceInfo[ErrorCode:%d](%s)\r\n",ErrorCode,NewDeviceInfo));
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/GetDeviceInfo[ErrorCode:%d][length:%d]\r\n",ErrorCode,NewDeviceInfoLength));
    if (g_inbUPnPCp)
    {
        g_inbUPnPCp->m_waitForWlanEventResp = false; // vbl - switch into Ethernet mode here
        g_inbUPnPCp->InvokeCallback((char *)NewDeviceInfo, NewDeviceInfoLength);
    }
}

void UPnPResponseSink_WFAWLANConfig_GetSTASettings(struct UPnPService* Service,int ErrorCode,void *User,unsigned char* NewSTASettings,int NewSTASettingsLength)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/GetSTASettings[ErrorCode:%d](%s)\r\n",ErrorCode,NewSTASettings));
}

void UPnPResponseSink_WFAWLANConfig_PutMessage(struct UPnPService* Service,int ErrorCode,void *User,unsigned char* NewOutMessage,int NewOutMessageLength)
{
	//TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/PutMessage[ErrorCode:%d](%s)\r\n",ErrorCode,NewOutMessage));
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/PutMessage[ErrorCode:%d](length:%d)\r\n",ErrorCode,NewOutMessageLength));
	if (ErrorCode)
	{
		TUTRACE((TUTRACE_ERR, "Some error from UPnP; Returning\r\n"));
		return;
	}
    if (g_inbUPnPCp)
    {
        g_inbUPnPCp->InvokeCallback((char *)NewOutMessage, NewOutMessageLength);
	}
}

void UPnPResponseSink_WFAWLANConfig_PutWLANResponse(struct UPnPService* Service,int ErrorCode,void *User)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/PutWLANResponse[ErrorCode:%d]()\r\n",ErrorCode));
}

void UPnPResponseSink_WFAWLANConfig_RebootAP(struct UPnPService* Service,int ErrorCode,void *User)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/RebootAP[ErrorCode:%d]()\r\n",ErrorCode));
}

void UPnPResponseSink_WFAWLANConfig_RebootSTA(struct UPnPService* Service,int ErrorCode,void *User)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/RebootSTA[ErrorCode:%d]()\r\n",ErrorCode));
}

void UPnPResponseSink_WFAWLANConfig_ResetAP(struct UPnPService* Service,int ErrorCode,void *User)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/ResetAP[ErrorCode:%d]()\r\n",ErrorCode));
}

void UPnPResponseSink_WFAWLANConfig_ResetSTA(struct UPnPService* Service,int ErrorCode,void *User)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/ResetSTA[ErrorCode:%d]()\r\n",ErrorCode));
}

void UPnPResponseSink_WFAWLANConfig_SetAPSettings(struct UPnPService* Service,int ErrorCode,void *User)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/SetAPSettings[ErrorCode:%d]()\r\n",ErrorCode));
}

void UPnPResponseSink_WFAWLANConfig_SetSelectedRegistrar(struct UPnPService* Service,int ErrorCode,void *User)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/SetSelectedRegistrar[ErrorCode:%d]()\r\n",ErrorCode));
}

void UPnPResponseSink_WFAWLANConfig_SetSTASettings(struct UPnPService* Service,int ErrorCode,void *User,unsigned char* NewSTASettings,int NewSTASettingsLength)
{
	TUTRACE((TUTRACE_UPNP, "UPnP Invoke Response: WFAWLANConfig/SetSTASettings[ErrorCode:%d](%s)\r\n",ErrorCode,NewSTASettings));
}



/* Called whenever a new device on the correct type is discovered */
void UPnPDeviceDiscoverSink(struct UPnPDevice *device)
{
	struct UPnPDevice *tempDevice = device;
	struct UPnPService *tempService;

	TUTRACE((TUTRACE_UPNP, "UPnPDevice Added: %s\r\n", device->FriendlyName));
	
	/* This call will print the device, all embedded devices and service to the console. */
	/* It is just used for debugging. */
	/* 	UPnPPrintUPnPDevice(0,device); */
	
	/* The following subscribes for events on all services */
	while(tempDevice!=NULL)
	{
		tempService = tempDevice->Services;
		while(tempService!=NULL)
		{
			UPnPSubscribeForUPnPEvents(tempService,NULL);
			tempService = tempService->Next;
		}
		tempDevice = tempDevice->Next;
	}
	
	/* The following will call every method of every service in the device with sample values */
	/* You can cut & paste these lines where needed. The user value is NULL, it can be freely used */
	/* to pass state information. */
	/* The UPnPGetService call can return NULL, a correct application must check this since a device */
	/* can be implemented without some services. */
	
	/* You can check for the existence of an action by calling: UPnPHasAction(serviceStruct,serviceType) */
	/* where serviceStruct is the struct like tempService, and serviceType, is a null terminated string representing */
	/* the service urn. */
	
	tempService = UPnPGetService_WFAWLANConfig(device);

    /*
    uint8 tmpStr[10] = "ABC123";
    
    UPnPInvoke_WFAWLANConfig_DelAPSettings(tempService, &UPnPResponseSink_WFAWLANConfig_DelAPSettings,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_DelSTASettings(tempService, &UPnPResponseSink_WFAWLANConfig_DelSTASettings,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_GetAPSettings(tempService, &UPnPResponseSink_WFAWLANConfig_GetAPSettings,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_GetDeviceInfo(tempService, &UPnPResponseSink_WFAWLANConfig_GetDeviceInfo,NULL);
    UPnPInvoke_WFAWLANConfig_GetSTASettings(tempService, &UPnPResponseSink_WFAWLANConfig_GetSTASettings,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_PutMessage(tempService, &UPnPResponseSink_WFAWLANConfig_PutMessage,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_PutWLANResponse(tempService, &UPnPResponseSink_WFAWLANConfig_PutWLANResponse,NULL,tmpStr,6,250,"Sample String");
    UPnPInvoke_WFAWLANConfig_RebootAP(tempService, &UPnPResponseSink_WFAWLANConfig_RebootAP,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_RebootSTA(tempService, &UPnPResponseSink_WFAWLANConfig_RebootSTA,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_ResetAP(tempService, &UPnPResponseSink_WFAWLANConfig_ResetAP,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_ResetSTA(tempService, &UPnPResponseSink_WFAWLANConfig_ResetSTA,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_SetAPSettings(tempService, &UPnPResponseSink_WFAWLANConfig_SetAPSettings,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_SetSelectedRegistrar(tempService, &UPnPResponseSink_WFAWLANConfig_SetSelectedRegistrar,NULL,tmpStr,6);
    UPnPInvoke_WFAWLANConfig_SetSTASettings(tempService, &UPnPResponseSink_WFAWLANConfig_SetSTASettings,NULL);
    */

    g_inbUPnPCp->SetDeviceAndService(tempService, tempDevice);
}

/* Called whenever a discovered device was removed from the network */
void UPnPDeviceRemoveSink(struct UPnPDevice *device)
{
	TUTRACE((TUTRACE_UPNP, "UPnPDevice Removed: %s\r\n", device->FriendlyName));
    g_inbUPnPCp->SetDeviceAndService(NULL, NULL);
}


CInbUPnPCp::CInbUPnPCp()
{
    TUTRACE((TUTRACE_UPNP, "CInbUPnPCp Construction\n"));
    g_inbUPnPCp = this;
    m_UPnPThreadHandle = 0;
    m_microStackChain = NULL;
    m_UPnPCp = NULL;
    m_device = NULL;
	m_service = NULL;
    strcpy(m_macAddr, "11:22:33:44:55:66");
    m_waitForWlanEventResp = false;
}

CInbUPnPCp::~CInbUPnPCp()
{
    TUTRACE((TUTRACE_UPNP, "CInbUPnPCp Destruction\n"));
    g_inbUPnPCp = NULL;
    Deinit();
}

uint32 CInbUPnPCp::Init()
{
    m_initialized = true;
    return WSC_SUCCESS;
}

uint32 CInbUPnPCp::StartMonitor()
{
    uint32 retVal;

    TUTRACE((TUTRACE_UPNP, "CInbUPnPCp StartMonitor\n"));
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

uint32 CInbUPnPCp::StopMonitor()
{
    TUTRACE((TUTRACE_UPNP, "CInbUPnPCp StopMonitor\n"));
    if ( ! m_initialized)
    {
        return WSC_ERR_NOT_INITIALIZED;
    }

	ILibStopChain(m_microStackChain);
    SLEEP(1);

    return WSC_SUCCESS;
}

uint32 CInbUPnPCp::WriteData(char * dataBuffer, uint32 dataLen)
{
    uint32 retVal = WSC_SUCCESS;

    TUTRACE((TUTRACE_UPNP, "In CInbUPnPCp::WriteData buffer Length = %d\n", 
            dataLen));
    if ( (! dataBuffer) || (! dataLen))
    {
        TUTRACE((TUTRACE_ERR, "Invalid Parameters\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

	if (m_waitForWlanEventResp)
	{
        m_waitForWlanEventResp = false;
        UPnPInvoke_WFAWLANConfig_PutWLANResponse(m_service, 
            &UPnPResponseSink_WFAWLANConfig_PutWLANResponse,
            NULL, (uint8 *) dataBuffer, dataLen, WSC_WLAN_EVENT_TYPE_EAP_FRAME, m_macAddr);
    }
    else
    {
        UPnPInvoke_WFAWLANConfig_PutMessage(m_service,
                &UPnPResponseSink_WFAWLANConfig_PutMessage,
                NULL, (uint8 *) dataBuffer, dataLen);
    }

    return retVal;
}

uint32 CInbUPnPCp::ReadData(char * dataBuffer, uint32 * dataLen)
{
    TUTRACE((TUTRACE_ERR, "In CInbUPnPCp::ReadData; NOT IMPLEMENTED\n"));
    return WSC_ERR_NOT_IMPLEMENTED;
}

uint32 CInbUPnPCp::Deinit()
{
    TUTRACE((TUTRACE_UPNP, "In CInbUPnPCp::Deinit\n"));
    if ( ! m_initialized)
    {
        TUTRACE((TUTRACE_ERR, "Not initialized; Returning\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    m_initialized = false;

    return WSC_SUCCESS;
}

void * CInbUPnPCp::StaticUPnPThread(IN void *p_data)
{
    TUTRACE((TUTRACE_UPNP, "In CInbUPnPCp::StaticUPnPThread\n"));
    ((CInbUPnPCp *)p_data)->ActualUPnPThread();
    return 0;
}


void * CInbUPnPCp::ActualUPnPThread()
{
    TUTRACE((TUTRACE_UPNP, "CInbUPnPCp::ActualUPnPThread Started\n"));

	m_microStackChain = ILibCreateChain();
	m_UPnPCp = UPnPCreateControlPoint(m_microStackChain,
                UPnPDeviceDiscoverSink,
                UPnPDeviceRemoveSink);

	/* TODO: Each device must have a unique device identifier (UDN) */
	/* All evented state variables MUST be initialized before UPnPStart is called. */
	TUTRACE((TUTRACE_UPNP, "Intel MicroStack 1.0 - \r\n\r\n"));

    UPnPEventCallback_WFAWLANConfig_STAStatus=&UPnPEventSink_WFAWLANConfig_STAStatus;
    UPnPEventCallback_WFAWLANConfig_APStatus=&UPnPEventSink_WFAWLANConfig_APStatus;
    UPnPEventCallback_WFAWLANConfig_WLANEvent=&UPnPEventSink_WFAWLANConfig_WLANEvent;

	TUTRACE((TUTRACE_UPNP, "Calling ILibStartChain\n"));
	ILibStartChain(m_microStackChain);
	
    TUTRACE((TUTRACE_UPNP, "ActualUPnPThread Finished\n"));
    return 0;
}

void CInbUPnPCp::InvokeCallback(char * buf, uint32 len)
{
//    char sendBuf[WSC_EAP_DATA_MAX_LENGTH];
    char * sendBuf;
    S_CB_COMMON * upnpComm;

    sendBuf = new char[sizeof(S_CB_COMMON) + len];
    if (sendBuf == NULL)
    {
        TUTRACE((TUTRACE_ERR, "Allocating memory for Sendbuf failed\n"));
        return;
    }
    // call callback
    upnpComm = (S_CB_COMMON *) sendBuf;
    upnpComm->cbHeader.eType = CB_TRUPNP_CP;
    upnpComm->cbHeader.dataLength = len;
    
    if (buf) {
        memcpy(sendBuf + sizeof(S_CB_COMMON), buf, len);
    }
    
    if (m_trCallbackInfo.pf_callback)
    {
        TUTRACE((TUTRACE_UPNP, "EAP: Calling Transport Callback\n"));
        m_trCallbackInfo.pf_callback(sendBuf, m_trCallbackInfo.p_cookie);
        TUTRACE((TUTRACE_UPNP, "Transport Callback Returned\n"));
    }
    else
    {
        TUTRACE((TUTRACE_ERR, "No Callback function set\n"));
    }
}

void CInbUPnPCp::SetDeviceAndService(struct UPnPService* Service, struct UPnPDevice * Device)
{
    m_service = Service;
    m_device = Device;
}

uint32 CInbUPnPCp::SendStartMessage(void)
{
    UPnPInvoke_WFAWLANConfig_GetDeviceInfo(m_service, &UPnPResponseSink_WFAWLANConfig_GetDeviceInfo,NULL);
	return WSC_SUCCESS;
}


uint32 CInbUPnPCp::SendSetSelectedRegistrar(char * dataBuffer, uint32 dataLen)
{
    if ( ! dataBuffer || ! dataLen)
    {
        TUTRACE((TUTRACE_ERR, "dataLen is 0 or dataBuffer is NULL\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    UPnPInvoke_WFAWLANConfig_SetSelectedRegistrar(m_service, &UPnPResponseSink_WFAWLANConfig_SetSelectedRegistrar,NULL, (uint8 *)dataBuffer,dataLen);
    return WSC_SUCCESS;
}
