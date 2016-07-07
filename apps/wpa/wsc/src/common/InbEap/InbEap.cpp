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
//  File Name: InbEap.cpp
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
#include <sys/un.h>
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
#include "UdpLib.h"
#include "RegProtoTlv.h"
#include "InbEap.h"

#include <openssl/bn.h>
#include <openssl/dh.h>
#include "WscHeaders.h"
#include "StateMachineInfo.h"
#include "WscQueue.h"
#include "Transport.h"
#include "RegProtoMsgs.h"
#include "RegProtocol.h"
#include "StateMachine.h"
#include "Info.h"
#include "MasterControl.h"

#ifdef __linux__
#define SLEEP(X) sleep(X)
#endif // __linux__
#ifdef WIN32
#define SLEEP(X) Sleep(X * 1000)
#endif // WIN32

#define WSC_EAP_UDP_PORT    37000
#define WSC_EAP_UDP_ADDR    "127.0.0.1"

//#define WSC_COMMANDS_UDP_PORT       38000

#define WSC_MSGTYPE_OFFSET  9

//bool CInbEap::m_readDataTimeoutEnable = false;

CInbEap::CInbEap()
{
    TUTRACE((TUTRACE_DBG, "CInbEap Construction\n"));
    m_recvEvent = 0;
    m_recvThreadHandle = 0;
    m_udpFdEap = -1;
    m_recvPort = 0;
    m_gotReq = false;
    ClearStaMacAddr();
}

CInbEap::~CInbEap()
{
    TUTRACE((TUTRACE_DBG, "CInbEap Destruction\n"));
}

uint32 CInbEap::Init()
{
    uint32 retVal;
	int32 udpRet;

    TUTRACE((TUTRACE_DBG, "Sizeof(WSC_NOTIFY_DATA) = %d\n", sizeof(WSC_NOTIFY_DATA)));
    retVal = WscCreateEvent(&m_recvEvent);
    if (retVal != WSC_SUCCESS)
    {
        TUTRACE((TUTRACE_ERR, "CreateEvent failed.\n"));
        return retVal;
    }
    m_udpFdEap = udp_open();
    if (m_udpFdEap == -1)
    {
        TUTRACE((TUTRACE_ERR, "UDP Open failed.\n"));
        return WSC_ERR_SYSTEM;
    }

    udpRet = udp_bind(m_udpFdEap, WSC_EAP_UDP_PORT);
    if (udpRet == -1)
    {
        TUTRACE((TUTRACE_ERR, "UDP Bind failed.\n"));
        return WSC_ERR_SYSTEM;
    }
    TUTRACE((TUTRACE_INFO, "Bind m_udpFdEap[%d] to WSC_EAP_UDP_PORT(%d) OK.\r\n",
                            m_udpFdEap, WSC_EAP_UDP_PORT));
    /*
    m_udpFdCom = udp_open();
    if (m_udpFdCom == -1)
    {
        TUTRACE((TUTRACE_ERR, "UDP Open for second socket failed.\n"));
        return WSC_ERR_SYSTEM;
    }
    */

    m_initialized = true;

    return WSC_SUCCESS;
}

uint32 CInbEap::StartMonitor()
{
    uint32 retVal;

    if ( ! m_initialized)
    {
        return WSC_ERR_NOT_INITIALIZED;
    }

    retVal = WscCreateThread(&m_recvThreadHandle, StaticRecvThread, this);

    if (retVal != WSC_SUCCESS)
    {
        TUTRACE((TUTRACE_ERR,  "CreateThread failed.\n"));
        return retVal;
    }

    SLEEP(0);
    /* reset event?? */
    TUTRACE((TUTRACE_INFO, "Calling WscResetEvent to reset m_recvEvent.\r\n"));
    WscResetEvent(m_recvEvent);

    return WSC_SUCCESS;
}

uint32 CInbEap::StopMonitor()
{
    if ( ! m_initialized)
    {
        return WSC_ERR_NOT_INITIALIZED;
    }

    SendQuit();
    SLEEP(0);

    if (m_recvThreadHandle)
    {
        WscDestroyThread(m_recvThreadHandle);
        m_recvThreadHandle = 0;
    }

    SLEEP(0);

    return WSC_SUCCESS;
}

uint32 CInbEap::WriteData(char * dataBuffer, uint32 dataLen)
{
    uint32 retVal;

    TUTRACE((TUTRACE_DBG, "In CInbEap::WriteData buffer Length = %d\n",
            dataLen));
    //if ( (! dataBuffer) || (! dataLen))
    //{
    //    TUTRACE((TUTRACE_ERR, "Invalid Parameters\n"));
    //    return WSC_ERR_INVALID_PARAMETERS;
    //}
    if ( (dataBuffer == NULL && dataLen != 0) ||
         (dataBuffer != NULL && dataLen == 0))
    {
        TUTRACE((TUTRACE_ERR, "Invalid Parameters\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    if (m_recvPort == 0)
    {
        TUTRACE((TUTRACE_ERR, "No data has been received from lower layer\n"));
        return TREAP_ERR_SENDRECV;
    }

    switch (m_recentType)
    {
    case WSC_NOTIFY_TYPE_PROCESS_REQ:
        retVal = AttachHeaderAndSend(dataBuffer, dataLen,
                WSC_NOTIFY_TYPE_PROCESS_RESULT, WSC_EAP_CODE_RESPONSE);
        break;
    case WSC_NOTIFY_TYPE_BUILDREQ:
        retVal = AttachHeaderAndSend(dataBuffer, dataLen,
                WSC_NOTIFY_TYPE_BUILDREQ_RESULT, WSC_EAP_CODE_REQUEST);
        break;
    case WSC_NOTIFY_TYPE_PROCESS_RESP:
        if (dataBuffer != NULL) {
            memcpy(m_storeBuf, dataBuffer, dataLen);
            m_storeLen = dataLen;
            m_gotReq = true;
        }
        retVal = AttachHeaderAndSend(NULL, 0,
                WSC_NOTIFY_TYPE_PROCESS_RESULT, WSC_EAP_CODE_RESPONSE);
        break;
    default:
        break;
    }

    return retVal;
}

uint32 CInbEap::ReadDataTimeoutEnable(bool enable)
{
    TUTRACE((TUTRACE_DBG, "********TimeoutEnable = %d \n", enable));

    m_readDataTimeoutEnable = enable;

    if(enable == true){ //Start time count??
        m_readDataStartTime= time(NULL);
    }
    return WSC_SUCCESS;
}

bool CInbEap::CheckEapTimeout()
{
    const unsigned int TIMEOUT_VALUE=120;

    if((time(NULL) - m_readDataStartTime) > TIMEOUT_VALUE)
        return true;
    else
        return false;
}

uint32 CInbEap::ReadData(char * dataBuffer, uint32 * dataLen)
{
    struct sockaddr_in from;
    int recvBytes = 0;
    int len;

    if (dataBuffer && (! dataLen))
    {
        return WSC_ERR_INVALID_PARAMETERS;
    }

    memset(&from, 0, sizeof(struct sockaddr_in));

#if 0  // Jerry
    len = (int) *dataLen;
    recvBytes = udp_read(m_udpFdEap, dataBuffer, len, &from);

    if (recvBytes == -1)
    {
        TUTRACE((TUTRACE_ERR, "UDP recv failed; recvBytes = %d\n", recvBytes));
        return TREAP_ERR_SENDRECV;
    }
#else
    len = (int) *dataLen;
    for(;;)
    {
        TUTRACE((TUTRACE_INFO, "UDP start reading from Eap socket %d.\r\n",
                                    m_udpFdEap));
        recvBytes = udp_read_timed(m_udpFdEap, dataBuffer, len, &from, 5);
        if (recvBytes == -1)
        {
            TUTRACE((TUTRACE_ERR, "UDP recv failed; recvBytes = %d\n",recvBytes));
            return TREAP_ERR_SENDRECV;
        }
        if (recvBytes == -2)   // time out
        {
            if(m_readDataTimeoutEnable == true)
            {
                if(CheckEapTimeout() == true){
                    if(CMasterControl::GetRegSM())
                          CMasterControl::GetRegSM()->PBCTimeOut();
                    TUTRACE((TUTRACE_ERR, "UDP recv timeout and failed.\r\n"));
                    ReadDataTimeoutEnable(false);
                    return TREAP_ERR_RECV_TIMEOUT;
                }
            }
            // keep waiting for data
            continue;
        }
        // get data
        break;
    }
#endif
    *dataLen = recvBytes;
    m_recvPort = ntohs(from.sin_port);

    return WSC_SUCCESS;
}

uint32 CInbEap::Deinit()
{
    TUTRACE((TUTRACE_DBG, "In CInbEap::Deinit\n"));
    if ( ! m_initialized)
    {
        TUTRACE((TUTRACE_ERR, "Not initialized; Returning\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    if (m_udpFdEap != -1)
    {
        udp_close(m_udpFdEap);
        m_udpFdEap = -1;
        m_recvPort = 0;
    }

    if (m_recvEvent)
    {
        WscDestroyEvent(m_recvEvent);
        m_recvEvent = 0;
    }

    /*
    if (m_udpFdCom != -1)
    {
        udp_close(m_udpFdCom);
        m_udpFdCom = -1;
    }
    */

    m_initialized = false;

    return WSC_SUCCESS;
}

void * CInbEap::StaticRecvThread(IN void *p_data)
{
    TUTRACE((TUTRACE_DBG, "In CInbEap::StaticRecvThread\n"));
    ((CInbEap *)p_data)->ActualRecvThread();

    return 0;
} // StaticRecvThread


void * CInbEap::ActualRecvThread()
{
    char buf[WSC_EAP_DATA_MAX_LENGTH];
    uint32 len;

    TUTRACE((TUTRACE_DBG, "CInbEap::ActualRecvThread Started\n"));

    while (1)
    {
        WscSleep(0);
        len = WSC_EAP_DATA_MAX_LENGTH;
        if (ReadData(buf, &len) == WSC_SUCCESS)
        {
            TUTRACE((TUTRACE_DBG, "EAP Received Data; Length = %d\n", len));
            if (strncmp(buf, "QUIT", 4) == 0)
            {
                break;
            }
            TUTRACE((TUTRACE_INFO, "Calling ProcessData.\r\n"));
            ProcessData(buf, len);
        }
        else
        {
            //TUTRACE((TUTRACE_DBG, "ReadData Failed for some reason; Continue\n"));
            SLEEP(1);
            continue;
        }
    } // while

    TUTRACE((TUTRACE_INFO, "EapRecvThread Finished\n"));
    return 0;
}

void CInbEap::ProcessData(char * buf, uint32 len)
{
    WSC_NOTIFY_DATA * recvNotify;
    WSC_EAP_HEADER * wscEapHdr;
    uint8            empty_mac_addr[6] = {0,0,0,0,0,0};

    recvNotify = (WSC_NOTIFY_DATA *) buf;

    if (recvNotify->type == WSC_NOTIFY_TYPE_BUILDREQ)
    { //somebody notify to build eap-request??
        TUTRACE((TUTRACE_INFO, "In ProcessData, notify type BuildReq, Id=%d\r\n",
                recvNotify->u.bldReq.id));
        m_recentType = recvNotify->type;
        m_recentId = recvNotify->u.bldReq.id;
        m_recentState = recvNotify->u.bldReq.state;
        if(memcmp(m_sta_mac_addr, empty_mac_addr, SIZE_MAC_ADDR)==0)
        {
            TUTRACE((TUTRACE_DBG, "copy recvNotify sta mac addr to m_sta_mac_addr.\r\n"));
            memcpy(m_sta_mac_addr, recvNotify->sta_mac_addr, SIZE_MAC_ADDR);
        } else {
            if(memcmp(m_sta_mac_addr,recvNotify->sta_mac_addr,SIZE_MAC_ADDR)!=0)
            {
                 TUTRACE((TUTRACE_INFO, "mac addr mismatch, discarding...\r\n"));
                 return; /* if the packet is not from the current enrolle, drop it */
            }
        }
        TUTRACE((TUTRACE_INFO, "m_sta_mac_addr=%2x:%2x:%2x:%2x:%2x:%2x\r\n",
                    m_sta_mac_addr[0], m_sta_mac_addr[1], m_sta_mac_addr[2],
                    m_sta_mac_addr[3], m_sta_mac_addr[4], m_sta_mac_addr[5]
                    ));
        if (m_gotReq)
        {
            TUTRACE((TUTRACE_INFO, "have got req, calling AttachHeaderAndSend,"
                            " m_storeBuf=%8x, m_storeLen=%d\r\n", m_storeBuf, m_storeLen));
            AttachHeaderAndSend(m_storeBuf, m_storeLen,
                    WSC_NOTIFY_TYPE_BUILDREQ_RESULT, WSC_EAP_CODE_REQUEST);
            m_gotReq = false;
        }
        else
        {
            TUTRACE((TUTRACE_INFO, "m_gotReq==false, Calling InvokeCallback with NULL, 0.\r\n"));
            InvokeCallback(NULL, 0);
        }
    }
    else if (recvNotify->type == WSC_NOTIFY_TYPE_PROCESS_REQ ||
             recvNotify->type == WSC_NOTIFY_TYPE_PROCESS_RESP)
    {
        TUTRACE((TUTRACE_INFO, "In ProcessData, notify type PROCESS_REQ|RESP, state"
                    "=%d\r\n", recvNotify->u.process.state));
        m_recentType = recvNotify->type;
        m_recentState = recvNotify->u.process.state;
        if(memcmp(m_sta_mac_addr,"000000", SIZE_MAC_ADDR)==0)
        {
            printf("mac from 0 to new sta\n");
            memcpy(m_sta_mac_addr, recvNotify->sta_mac_addr, SIZE_MAC_ADDR);
        } else {
            if(memcmp(m_sta_mac_addr,recvNotify->sta_mac_addr,SIZE_MAC_ADDR)!=0)
            {
                 printf(" reject second sta\n");
                 return; /* if the packet is not from the current enrolle, drop it */
            }
        }
        wscEapHdr = (WSC_EAP_HEADER *) (recvNotify + 1);
        m_recentId = wscEapHdr->id;
        TUTRACE((TUTRACE_INFO, "Calling InvokeCallback with data, Id=%d\r\n",
                    m_recentId));
        InvokeCallback((char *)(wscEapHdr + 1), len - sizeof(WSC_NOTIFY_DATA) -
                sizeof(WSC_EAP_HEADER));
    }
    else
    {
        TUTRACE((TUTRACE_ERR, "Invalid WSC_NOTIFY_TYPE\n"));
    }
}

void CInbEap::InvokeCallback(char * buf, uint32 len)
{
//    char sendBuf[WSC_EAP_DATA_MAX_LENGTH];
    char * sendBuf;
    S_CB_COMMON * eapComm;

    sendBuf = new char[sizeof(S_CB_COMMON) + len];
    if (sendBuf == NULL)
    {
        TUTRACE((TUTRACE_ERR, "Allocating memory for Sendbuf failed\n"));
        return;
    }
    // call callback
    eapComm = (S_CB_COMMON *) sendBuf;
    eapComm->cbHeader.eType = CB_TREAP;
    eapComm->cbHeader.dataLength = len;

    if (buf) {
        memcpy(sendBuf + sizeof(S_CB_COMMON), buf, len);
    }
    printf("EAP InvokeCallback func ptr %08x, sending to transport layer.\n",
            m_trCallbackInfo.pf_callback);
    if (m_trCallbackInfo.pf_callback)
    {
        TUTRACE((TUTRACE_DBG, "EAP: Calling Transport Callback\n"));
        m_trCallbackInfo.pf_callback(sendBuf, m_trCallbackInfo.p_cookie);
        TUTRACE((TUTRACE_DBG, "Transport Callback Returned\n"));
    }
    else
    {
        TUTRACE((TUTRACE_ERR, "No Callback function set\n"));
    }
}

uint32 CInbEap::AttachHeaderAndSend(char * dataBuffer, uint32 dataLen,
                                    uint8 wscNotifyCode, uint8 eapCode)
{
    char sendBuf[WSC_EAP_DATA_MAX_LENGTH];
    WSC_NOTIFY_DATA * notifyData;
    WSC_EAP_HEADER * wscEapHdr;

    TUTRACE((TUTRACE_INFO, "In AttachHeaderAndSend(), wscNotifyCode=%d"
                    ", eapCode=%d\r\n", wscNotifyCode, eapCode));
    // attach header and send it down
    notifyData = (WSC_NOTIFY_DATA *) sendBuf;
    notifyData->type = wscNotifyCode;
    switch (wscNotifyCode)
    {
        case WSC_NOTIFY_TYPE_BUILDREQ_RESULT:
            if (dataBuffer != NULL) {
                notifyData->u.bldReqResult.result = WSC_NOTIFY_RESULT_SUCCESS;
            }
            else {
                notifyData->u.bldReqResult.result = WSC_NOTIFY_RESULT_FAILURE;
            }
            break;
        case WSC_NOTIFY_TYPE_PROCESS_RESULT:
            notifyData->u.processResult.done = 0;
            if (dataBuffer != NULL) {
                notifyData->u.processResult.result = WSC_NOTIFY_RESULT_SUCCESS;
            }
            else {
                notifyData->u.processResult.result = m_gotReq ?
                    WSC_NOTIFY_RESULT_SUCCESS : WSC_NOTIFY_RESULT_FAILURE;
            }
            break;
        default:
            break;
    }

    notifyData->length = dataLen + sizeof(WSC_EAP_HEADER);

    wscEapHdr = (WSC_EAP_HEADER *) (notifyData + 1);
    /* encap */
    wscEapHdr->code = eapCode;
    wscEapHdr->id = m_recentId;
    wscEapHdr->length = WscHtons(sizeof(WSC_EAP_HEADER) + dataLen);

    wscEapHdr->type = WSC_EAP_TYPE;
    wscEapHdr->vendorId[0] = WSC_VENDORID1;
	wscEapHdr->vendorId[1] = WSC_VENDORID2;
    wscEapHdr->vendorId[2] = WSC_VENDORID3;
    wscEapHdr->vendorType = WscHtonl(WSC_VENDORTYPE);

    if (dataBuffer)
    {
		if (dataBuffer[WSC_MSGTYPE_OFFSET] >= WSC_ID_MESSAGE_M1 &&
			dataBuffer[WSC_MSGTYPE_OFFSET] <= WSC_ID_MESSAGE_M8)
		{
			wscEapHdr->opcode = WSC_MSG;
		}
		else if (dataBuffer[WSC_MSGTYPE_OFFSET] == WSC_ID_MESSAGE_ACK)
		{
			wscEapHdr->opcode = WSC_ACK;
		}
		else if (dataBuffer[WSC_MSGTYPE_OFFSET] == WSC_ID_MESSAGE_NACK)
		{
			wscEapHdr->opcode = WSC_NACK;
		}
		else if (dataBuffer[WSC_MSGTYPE_OFFSET] == WSC_ID_MESSAGE_DONE)
		{
			wscEapHdr->opcode = WSC_Done;
		}
		else
		{
			TUTRACE((TUTRACE_ERR, "Unknown Message Type code %d; "
				"Not sending msg\n", dataBuffer[WSC_MSGTYPE_OFFSET]));
			return TREAP_ERR_SENDRECV;
		}
		// TBD: Flags are always set to zero for now, if message is too big
		// fragmentation must be done here and flags will have some bits set
		// and message length field is added
		wscEapHdr->flags = 0;

        memcpy(wscEapHdr + 1, dataBuffer,  dataLen);
    }
    TUTRACE((TUTRACE_INFO, "Calling SendDataDown, bytes=%d.\r\n",
                dataLen + sizeof(WSC_EAP_HEADER) + sizeof(WSC_NOTIFY_DATA)));
    return SendDataDown(sendBuf, dataLen + sizeof(WSC_EAP_HEADER) +
            sizeof(WSC_NOTIFY_DATA));
}

uint32 CInbEap::SendDataDown(char * dataBuffer, uint32 dataLen)
{
    int sentBytes = 0;
    struct sockaddr_in to;

    TUTRACE((TUTRACE_DBG, "In CInbEap::SendDataDown buffer Length = %d\n",
            dataLen));
    if ( (! dataBuffer) || (! dataLen))
    {
        TUTRACE((TUTRACE_ERR, "Invalid Parameters\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    if (m_recvPort == 0)
    {
        TUTRACE((TUTRACE_ERR, "No data has been received from lower layer\n"));
        return TREAP_ERR_SENDRECV;
    }

    memset(&to,0,sizeof(to));


    to.sin_addr.s_addr = inet_addr(WSC_EAP_UDP_ADDR);
    to.sin_family = AF_INET;
    to.sin_port = htons(m_recvPort);

    TUTRACE((TUTRACE_INFO, "UDP writing to loopback if, port %d.\r\n", m_recvPort));
    sentBytes = udp_write(m_udpFdEap, dataBuffer, dataLen, &to);

    if (sentBytes != (int32) dataLen)
    {
        TUTRACE((TUTRACE_ERR, "UDP send failed; sentBytes = %d\n", sentBytes));
        return TREAP_ERR_SENDRECV;
    }
    TUTRACE((TUTRACE_INFO, "UDP send success.\r\n"));
    return WSC_SUCCESS;
}
uint32 CInbEap::SendQuit()
{
    int sentBytes = 0;
    struct sockaddr_in to;

    memset(&to,0,sizeof(to));


    to.sin_addr.s_addr = inet_addr(WSC_EAP_UDP_ADDR);
    to.sin_family = AF_INET;
    to.sin_port = htons(WSC_EAP_UDP_PORT);

    sentBytes = udp_write(m_udpFdEap, "QUIT", 4, &to);

    if (sentBytes != 4)
    {
        TUTRACE((TUTRACE_ERR, "UDP send failed; sentBytes = %d\n", sentBytes));
        return TREAP_ERR_SENDRECV;
    }
    TUTRACE((TUTRACE_INFO, "UDP SendQuit() success.\r\n"));
    return WSC_SUCCESS;
}

uint32 CInbEap::SendStartMessage(void)
{
    char sendBuf[WSC_EAP_DATA_MAX_LENGTH];
    WSC_NOTIFY_DATA * notifyData;
    WSC_EAP_HEADER * wscEapHdr;
    // attach header and send it down
    notifyData = (WSC_NOTIFY_DATA *) sendBuf;
    notifyData->type = WSC_NOTIFY_TYPE_BUILDREQ_RESULT;
    notifyData->u.bldReqResult.result = WSC_NOTIFY_RESULT_SUCCESS;
    notifyData->length = sizeof(WSC_EAP_HEADER);
    wscEapHdr = (WSC_EAP_HEADER *) (notifyData + 1);
    wscEapHdr->code = WSC_EAP_CODE_REQUEST;
    wscEapHdr->id = m_recentId;
    wscEapHdr->length = WscHtons(sizeof(WSC_EAP_HEADER));
    wscEapHdr->type = WSC_EAP_TYPE;
    wscEapHdr->vendorId[0] = WSC_VENDORID1;
    wscEapHdr->vendorId[1] = WSC_VENDORID2;
    wscEapHdr->vendorId[2] = WSC_VENDORID3;
    wscEapHdr->vendorType = WscHtonl(WSC_VENDORTYPE);
    wscEapHdr->opcode = WSC_Start;
    wscEapHdr->flags = 0;

    TUTRACE((TUTRACE_INFO, "EAP SendStartMessage(), WSC_START.\r\n"));
    return SendDataDown(sendBuf, sizeof(WSC_EAP_HEADER) +
            sizeof(WSC_NOTIFY_DATA));
}

void CInbEap::ClearStaMacAddr(void)
{
    memset(m_sta_mac_addr, 0, SIZE_MAC_ADDR);

}

void CInbEap::GetStaMacAddr(char * macAddr)
{
    memcpy(macAddr, m_sta_mac_addr, SIZE_MAC_ADDR);
}

