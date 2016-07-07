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
//  File Name: EnrolleeSM.cpp
//  Description: Implements Enrollee State Machine Class.
//
****************************************************************************/

#ifdef WIN32
#include <windows.h>
#endif

//OpenSSL includes
#include <openssl/bn.h>
#include <openssl/dh.h>

#include "WscHeaders.h"
#include "WscCommon.h"
#include "WscError.h"
#include "Portability.h"
#include "WscQueue.h"
#include "tutrace.h"
#include "Transport.h"
#include "StateMachineInfo.h"
#include "RegProtoMsgs.h"
#include "RegProtocol.h"
#include "StateMachine.h"

#include "WscTlvBase.h"

/* any special reason to use 10 seconds??? */
#if 0
#define M2D_SLEEP_TIME 10 // 10 seconds
#else
#define M2D_SLEEP_TIME 30   //30 seconds
#endif

/* July 01, 2008, Liang Xin.
 * Support M4 timeout
 */
#define M4_SLEEP_TIME 50

extern int SendWscEvent(char * pEvent);

CEnrolleeSM::CEnrolleeSM(IN CRegProtocol *pc_regProt, IN CTransport *pc_trans)
        :CStateMachine(pc_regProt, pc_trans, MODE_ENROLLEE)
{
    WscSyncCreate(&mp_m2dLock);

    /* July 01, 2008, LiangXin.
     * This is for WCN Logo Test Error Handling Test 1
     */
    memset(m_bufferedM1, 0, sizeof(m_bufferedM1));
    m_bufferedM1Len = 0;
    /* Required for Wireless Discovery */
    m_configuredAP = SM_AP_UNCONFIGURED;    //default state

    /* Added by Liang Xin, for WCN Logo Test Error Handling Test 7.
     * Support 60s timeout after Sent M3, without recved M4
     */
     WscSyncCreate(&mp_m4Lock);
}

CEnrolleeSM::~CEnrolleeSM()
{
    WscSyncDestroy(mp_m2dLock);
    /* July 01, 2008, LiangXin */
    WscSyncDestroy(mp_m4Lock);
}

uint32
CEnrolleeSM::InitializeSM(IN S_DEVICE_INFO *p_registrarInfo,
                          IN void * p_StaEncrSettings,
                          IN void * p_ApEncrSettings,
                          IN char *p_devicePasswd,
                          IN uint32 passwdLength)
{
    // if(!p_registrarInfo)
    //    return WSC_ERR_INVALID_PARAMETERS;

    uint32 err = CStateMachine::InitializeSM();
    if(WSC_SUCCESS != err)
        return err;

    mps_regData->p_enrolleeInfo = mps_localInfo;
    mps_regData->p_registrarInfo = p_registrarInfo;

    SetPassword(p_devicePasswd, passwdLength);
    SetEncryptedSettings(p_StaEncrSettings, p_ApEncrSettings);

    /* July 01, 2008, Liang Xin, WCN Logo Test Error XXX 1 */
    if (m_bufferedM1Len != 0)
    {
        memset(m_bufferedM1, 0, sizeof(m_bufferedM1));
        m_bufferedM1Len = 0;
    }
    return WSC_SUCCESS;
}

uint32
CEnrolleeSM::Step(IN uint32 msgLen, IN uint8 *p_msg)
{
    BufferObj *inMsg=NULL;
    uint32 err;

    TUTRACE((TUTRACE_DBG, "ENRSM: Entering Step, msgLen=%d, p_msg=%8x.\r\n",
                msgLen, p_msg));

    if(false == m_initialized)
    {
        TUTRACE((TUTRACE_ERR, "ENRSM: Not yet initialized.\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    if(START == mps_regData->e_smState)
    {
        //No special processing here
        TUTRACE((TUTRACE_DBG, "ENRSM: e_smState == START, calling CEnrolleeSM"\
                        "::HandleMessage(), inMsg ==%8x\r\n", inMsg));
        HandleMessage(*inMsg);
    }
    else
    {
        //do the regular processing
        if(!p_msg || !msgLen)   // any one is zero, means request for M1
        {

             TUTRACE((TUTRACE_DBG, "ENRSM: p_msg==%8x, msgLen==%d(another request for M1)\r\n",
                        p_msg, msgLen));
            //Preferential treatment for UPnP
            if(mps_regData->e_lastMsgSent == M1)
            {
                //If we have already sent M1 and we get here, assume that it is
                //another request for M1 rather than an error.
                //Send the bufferred M1 message


                TUTRACE((TUTRACE_PROTO, "ENRSM: Got another request for M1. "
                                       "Resending the earlier M1\r\n"));
                /* M1 is stored in outMsg. */
                err = mpc_trans->TrWrite(m_transportType,
                                        (char *)mps_regData->outMsg.GetBuf(),
                                        mps_regData->outMsg.Length());
                if(WSC_SUCCESS != err)
                {
                    mps_regData->e_smState = FAILURE;
                    TUTRACE((TUTRACE_ERR, "ENRSM: TrWrite generated an "
                                        "error when resent M1: %d\n", err));
                    return err;
                }
                TUTRACE((TUTRACE_ERR, "ENRSM: Resent M1 success.\r\n"));
                return WSC_SUCCESS;
            }
            else
            {
                /* consider this condition as request for M1 again,
                 * and response with buffered M1, but not change current ENRSM state.
                 */
                #if 0
                TUTRACE((TUTRACE_ERR, "ENRSM: Wrong input parameters.\n"));
                //Notify the MasterControl
                TUTRACE((TUTRACE_INFO, "ENRSM: Calling CStateMachine::NotifyMasterControl() with status = SM_FAILURE \r\n"));
                NotifyMasterControl(SM_FAILURE, NULL, NULL);
                m_initialized = false;
                return WSC_ERR_INVALID_PARAMETERS;
                #else
                TUTRACE((TUTRACE_DBG, "ENRSM: Request for M1 after M2 recved, resending buffered M1"
                        ", Len==%d\r\n", m_bufferedM1Len));
                assert(m_bufferedM1Len != 0);
                err = mpc_trans->TrWrite(m_transportType, (char *)m_bufferedM1,
                                            m_bufferedM1Len);
                 if(WSC_SUCCESS != err)
                {
                    mps_regData->e_smState = FAILURE;
                    TUTRACE((TUTRACE_ERR, "ENRSM: TrWrite generated an "
                                        "error when resent M1 after M2 recved: %d\n", err));
                    return err;
                }
                 TUTRACE((TUTRACE_INFO, "ENRSM: Resent M1 after M2 recved success.\r\n"));
                 return WSC_SUCCESS;
                #endif
            }
        }

        TUTRACE((TUTRACE_INFO, "ENRSM: e_smState != START, and not requesting for M1.\r\n"));
        BufferObj regProtoMsg(p_msg, msgLen);
        inMsg = &regProtoMsg;
        HandleMessage(*inMsg);
    }

    //now check the state so we can act accordingly
    switch(mps_regData->e_smState)
    {
    case START:
    case CONTINUE:
        //do nothing.
        break;
    case SUCCESS:
        {
            m_initialized = false;
            //Notify the MasterControl
            TUTRACE((TUTRACE_ERR, "Notifying MC of success, mp_peerEncrSettings==%8x.\r\n",
                                    mp_peerEncrSettings));
            NotifyMasterControl(SM_SUCCESS,
                                mps_regData->p_registrarInfo,
                                mp_peerEncrSettings);

            //reset the transport connection
            mpc_trans->TrWrite(m_transportType, NULL, 0);

            //reset the SM
            RestartSM();
        }
           break;
    case FAILURE:
        {
            TUTRACE((TUTRACE_ERR, "ENRSM: Notifying MC of failure, mp_peerEncrSettings==%8x.\r\n",
                                mp_peerEncrSettings));
            m_initialized = false;

		//sep,11,08,add by weizhengqin
            //Notify the MasterControl
            
            if (mps_regData->e_lastMsgRecd == M8 && 
				mps_regData->e_lastMsgSent == DONE)
            	{
			NotifyMasterControl(SM_SUCCESS, NULL, NULL);
            	}
		else
		//
		{
            NotifyMasterControl(SM_FAILURE, NULL, NULL);
		}
            //reset the transport connection
            mpc_trans->TrWrite(m_transportType, NULL, 0);

            //reset the SM
            RestartSM();
        }
         break;
    default:
        break;
    }
    return WSC_SUCCESS;
}

/* July 01, 2008, LiangXin
 * To Support Wireless Discovery
 */
void CEnrolleeSM::SetAPConfiguredStat(IN uint32 configured)
{
    m_configuredAP = configured;
    return;
}

void CEnrolleeSM::HandleMessage(BufferObj &msg)
{
    uint32 err;
    char errMsg[256];
    uint8 nonce[SIZE_128_BITS];
    BufferObj outBuf, tempBuf;
    void *encrSettings = NULL;
    uint32 msgType = 0;
    try
    {
        //Append the header before doing any processing
        //S_WSC_HEADER hdr;
        //hdr.opCode = WSC_MSG;
        //hdr.flags = 0;
        //outBuf.Append(sizeof(hdr), (uint8 *)&hdr);
        TUTRACE((TUTRACE_INFO, "Entering CEnrolleeSM::HandleMessage().\n"));
        //If we get a valid message, extract the message type received.
        if(MNONE != mps_regData->e_lastMsgSent)
        {

            err = mpc_regProt->GetMsgType(msgType, msg);
            if(WSC_SUCCESS != err)
            {
                /* send NACK for WCN Wireless Miss M4 Test */
                /* system here is a WAR... */
                system("echo Preparing Nack for Miss MsgType>/tmp/debug");
               // WscSleep(1);
                SendNack(WSC_ERROR_DEV_PWD_AUTH_FAIL + 1);
                stringPrintf(errMsg, 256,
                            "ENRSM: GetMsgType returned error: %d\n", err);
                throw errMsg;
            }
            TUTRACE((TUTRACE_INFO, "extracted msg type %d from input msg.\n", msgType));

            /* July 01, 2008, LiangXin.
             * Following codes respond with NACK when got M2/M2D after recved M2,
             * to furfill the requirement of WCN Logo Test
             */
            if((SM_RECVD_M2 == m_m2dStatus) &&
               (msgType <= WSC_ID_MESSAGE_M2D))
            {
                TUTRACE((TUTRACE_INFO, "ENRSM: Have recved M2, and this msgType <= 0x6(M2D), respond with NACK\r\n"));
                /* Here we do NOT use SendNack(), because we need give correct
                 * R-Nonce in our Nack msg.
                 */
                /* extract r-nonce from recved M2 */
                err = mpc_regProt->GetNonceFromMsg(nonce, msg, WSC_ID_REGISTRAR_NONCE);
                if (WSC_SUCCESS != err)
                {
                    TUTRACE((TUTRACE_ERR, "ENRSM: GetNonceFromMsg failed: %d\r\n",
                                err));
                    stringPrintf(errMsg, 256, "ENRSM: GetNonceFromMsg failed: %d\r\n",
                                    err);
                    throw errMsg;
                }
                /* Build Nack with correct R-Nonce */
                TUTRACE((TUTRACE_INFO, "ENRSM: Prepare to build Nack from mps_regData==%8x.\r\n",
                                mps_regData));
                err = mpc_regProt->BuildMessageNack(mps_regData, nonce,
                                                WSC_ID_REGISTRAR_NONCE, tempBuf,
                                                WSC_ERROR_ROGUE_SUSPECTED);
                if (WSC_SUCCESS != err)
                {
                    TUTRACE((TUTRACE_ERR, "ENRSM: BuildMessageNack failed: %d\r\n",
                                    err));
                    stringPrintf(errMsg, 256, "ENRSM: BuildMessageNack failed: %d\r\n",
                                    err);
                    throw errMsg;
                }
                /* Write to Tr */
                TUTRACE((TUTRACE_INFO, "ENRSM: Calling TrWrite to send NACK() for rogue M2.\r\n"));
                err = mpc_trans->TrWrite(m_transportType,
                                (char *)tempBuf.GetBuf(),
                                 tempBuf.Length());
                if (WSC_SUCCESS != err)
                {
                    TUTRACE((TUTRACE_INFO, "ENRSM: Sending NACK with TrWrite() failed:"\
                                    " %d\r\n", err));
                    stringPrintf(errMsg, 256,
                            "ENRSM: SendingNACK with TrWrite() failed: %d\r\n", err);
                    throw errMsg;
                }
                TUTRACE((TUTRACE_INFO, "ENRSM: SendNack() success.\r\n"));
                return;
            }
        }
        TUTRACE((TUTRACE_INFO, "Last sent MsgType: %d.\r\n", mps_regData->e_lastMsgSent));
        switch(mps_regData->e_lastMsgSent)
        {
        case MNONE:
            err = mpc_regProt->BuildMessageM1(mps_regData, tempBuf);
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "BuildMessageM1: %d", err);
                throw errMsg;
            }
            mps_regData->e_lastMsgSent = M1;

            outBuf.Append(tempBuf.Length(), tempBuf.GetBuf());

            /* July 01, 2008, LiangXin
             * Buffer new generated M1.
             */
            assert(tempBuf.Length() <= sizeof(m_bufferedM1));
            memcpy(m_bufferedM1, tempBuf.GetBuf(), tempBuf.Length());
            m_bufferedM1Len = tempBuf.Length();

            //Now send the message to the transport
            TUTRACE((TUTRACE_INFO, "ENRSM::Calling Transport TrWrite() to send M1,transportType=%d\r\n",
                        m_transportType));

            err = mpc_trans->TrWrite(m_transportType,
                               (char *)outBuf.GetBuf(),
                               outBuf.Length());
            if(WSC_SUCCESS != err)
            {
                mps_regData->e_smState = FAILURE;
                TUTRACE((TUTRACE_DBG, "ENRSM::Transport TrWrite() failure, errno %d, transportType=%d.\r\n",
                            err, m_transportType));
                return;
            }

            //Set the m2dstatus.
            m_m2dStatus = SM_AWAIT_M2;

            //set the message state to CONTINUE
            mps_regData->e_smState = CONTINUE;
            break;
        case M1:
            //Check whether this is M2D
            if(WSC_ID_MESSAGE_M2D == msgType)
            {
                TUTRACE((TUTRACE_INFO, "ENRSM: M2D recved in HandleMessage,"\
                                " begin special handle...\r\n"));
                err = mpc_regProt->ProcessMessageM2D(mps_regData, msg);
                if(WSC_SUCCESS != err)
                {
                    stringPrintf(errMsg, 256, "ProcessMessageM2D: %d", err);
                    //Send NACK
                    CStateMachine::SendNack(err);
                    mps_regData->e_lastMsgSent = NACK;
                    mps_regData->e_smState = CONTINUE;
                    return;
                }
                /* July 01, 2008, LiangXin
                 * Respond with NACK while recved M2D under AP configured status.
                 * This is required by WCN Wireless Discovery Test.
                 */
                TUTRACE((TUTRACE_INFO, "ENRSM: SM mode %s.\r\n", (MODE_REGISTRAR == m_mode)
                                ? "Registrar":"Enrollee"));
                if (SM_AP_CONFIGURED == m_configuredAP)
                {
                    TUTRACE((TUTRACE_INFO, "ENRSM: configured AP recved M2D, NACK it.\r\n"));
                    err = SendNack(WSC_ERROR_REG_SESSION_TIMEOUT);
                    mps_regData->e_lastMsgSent = NACK;
                    /* it's required to set continue */
                    mps_regData->e_smState = CONTINUE;
                    return;
                }
                /* end of added code */

                //Send an ACK to the registrar
                TUTRACE((TUTRACE_INFO, "ENRSM: Sending ACK to registrar after recved M2D\r\n"));
                err = SendAck();
                if(WSC_SUCCESS != err)
                {
                    stringPrintf(errMsg, 256, "SendAck: %d", err);
                    throw errMsg;
                }

                //Now, schedule a thread to sleep for some time to allow other
                //registrars to send M2 or M2D messages.
                WscLock(mp_m2dLock);
                if(SM_AWAIT_M2 == m_m2dStatus)
                {
                    //if the M2D status is 'await', set the timer. For all
                    //other cases, don't do anything, because we've either
                    //already received an M2 or M2D, and possibly, the SM reset
                    //process has already been initiated
                    TUTRACE((TUTRACE_INFO, "ENRSM: Starting M2DThread when first recv M2D.\r\n"));
                    m_m2dStatus = SM_RECVD_M2D;

                    err = WscCreateThread(
                                &m_timerThrdId,     // thread ID
                                M2DTimerThread,     // thread proc
                                (void *)this);      // data to pass to thread
                    if (WSC_SUCCESS != err)
                    {
                        throw "RegSM: m_cbThread not created";
                    }
                    TUTRACE((TUTRACE_INFO, "ENRSM: Started M2DThread\n"));

                    WscSleep(1);

                    //set the message state to CONTINUE
                    mps_regData->e_smState = CONTINUE;
                    WscUnlock(mp_m2dLock);
                    return;
                }
                else
                {
                    TUTRACE((TUTRACE_ERR, "ENRSM: Did not start M2DThread. "
                             "status = %d\n", m_m2dStatus));
                }
                WscUnlock(mp_m2dLock);

                break; //done processing for M2D, return
            }//if(M2D == msgType)

            WscLock(mp_m2dLock);
            if(SM_M2D_RESET == m_m2dStatus)
            {
                WscUnlock(mp_m2dLock);
                TUTRACE((TUTRACE_INFO, "ENRSM: SM Resetting, do not process M2.\r\n"));
                return; //a SM reset has been initiated. Don't process any M2s
            }
            else
            {
                TUTRACE((TUTRACE_INFO, "ENRSM: Recved M2 when last sent was M1.\r\n"));
                m_m2dStatus = SM_RECVD_M2;
            }
            WscUnlock(mp_m2dLock);

            //Notice wsc that UPnP registrar will config AP
            if((m_transportType == TRANSPORT_TYPE_UPNP_DEV)||
               (m_transportType == TRANSPORT_TYPE_UPNP_CP))
            {
                SendWscEvent("WSC_UPNP_M2");
	        TUTRACE((TUTRACE_PROTO, "Wsc Meaage WSC_UPNP_M2 send;\r\n"));
            }
            TUTRACE((TUTRACE_INFO, "ENRSM: preparing process M2. mps_regData==%8x"
                    ", encrSettings==%8x.\r\n", mps_regData, encrSettings));

            err =mpc_regProt->ProcessMessageM2(mps_regData, msg, &encrSettings);
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "ProcessMessageM2: %d", err);
                //Send NACK
                CStateMachine::SendNack(err);
                mps_regData->e_lastMsgSent = NACK;
                mps_regData->e_smState = CONTINUE;
                return;
            }
            mps_regData->e_lastMsgRecd = M2;
             TUTRACE((TUTRACE_INFO, "ENRSM: M2 Processed, mps_regData==%8x, encrSettings==%8x"
                    ".\r\n", mps_regData, encrSettings));
             TUTRACE((TUTRACE_INFO, "ENRSM: Building M3.\r\n"));
            err = mpc_regProt->BuildMessageM3(mps_regData, tempBuf);
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "BuildMessageM3: %d", err);
                throw errMsg;
            }
            mps_regData->e_lastMsgSent = M3;

            outBuf.Append(tempBuf.Length(), tempBuf.GetBuf());

            //Now send the message to the transport
            err = mpc_trans->TrWrite(m_transportType,
                               (char *)outBuf.GetBuf(),
                               outBuf.Length());
            if(WSC_SUCCESS != err)
            {
                mps_regData->e_smState = FAILURE;
                TUTRACE((TUTRACE_ERR, "ENRSM: TrWrite generated an "
                                      "error: %d\n", err));
                return;
            }
            TUTRACE((TUTRACE_INFO, "ENRSM: M3 Sent successfully.\r\n"));

            /* July 01, 2008, LiangXin
             * Start the M4 recv timeout thread.
             * This time out is required by WCN Test.
             */
            m_m4Status = SM_AWAIT_M4;
            TUTRACE((TUTRACE_INFO, "EMRSM: Starting M4TimerThread after M3 sent...\r\n"));
            err = WscCreateThread(&m_m4TimerThrdId,
                                    M4TimerThread,
                                    (void *)this);
            if (WSC_SUCCESS != err)
            {
                mps_regData->e_smState = FAILURE;
                TUTRACE((TUTRACE_INFO, "ENRSM: start M4TimerThread failure, %d\r\n",
                            err));
                return;
            }
            WscSleep(1);    /* run M4TimerThread?? */
            /* end of added code */

            //set the message state to CONTINUE
            mps_regData->e_smState = CONTINUE;
            break;
        case M3:
            TUTRACE((TUTRACE_INFO, "ENRSM: Preparing to process M4.\r\n"));

            err = mpc_regProt->ProcessMessageM4(mps_regData, msg);
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "ProcessMessageM4: %d", err);
                //Send NACK
                TUTRACE((TUTRACE_INFO, "ENRSM: M4 invalid, preparing to reset SM.\r\n"));
                CStateMachine::SendNack(err);
                mps_regData->e_lastMsgSent = NACK;
                /* July 01, 2008, LiangXin
                 * Immediatelly set this session as failure
                 * which is required by WCN Error Handling Test 4.
                 */
                #if 0
                mps_regData->e_smState = CONTINUE;
                #else
                mps_regData->e_smState = FAILURE;
                #endif
                return;
            }
            mps_regData->e_lastMsgRecd = M4;
            /* July 01, 2008, Liang Xin
             * M4 time out related handling
             */
            WscLock(mp_m4Lock);
            if (SM_TIMEOUT_M4 == m_m4Status) {
                 /* already time out, respond with NACK */
                WscUnlock(mp_m4Lock);
                TUTRACE((TUTRACE_INFO, "ENRSM: M4 recved but time out, respnd NACK.\r\n"));
                SendNack(WSC_ERROR_MSG_TIMEOUT);
                mps_regData->e_lastMsgSent = NACK;
                mps_regData->e_smState = FAILURE;   /* force restart SM */
                return;
            }
            else
            {
                /* not time out, set status to recved */
                TUTRACE((TUTRACE_INFO, "ENRSM: M4 recved and not time out, set RECVED"\
                        " status.\r\n"));
                m_m4Status = SM_RECVD_M4;
            }
            WscUnlock(mp_m4Lock);
            /* end of added code */

            err = mpc_regProt->BuildMessageM5(mps_regData, tempBuf);
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "BuildMessageM5: %d", err);
                throw errMsg;
            }
            mps_regData->e_lastMsgSent = M5;

            outBuf.Append(tempBuf.Length(), tempBuf.GetBuf());

            //Now send the message to the transport
            err = mpc_trans->TrWrite(m_transportType,
                               (char *)outBuf.GetBuf(),
                               outBuf.Length());
            if(WSC_SUCCESS != err)
            {
                mps_regData->e_smState = FAILURE;
                TUTRACE((TUTRACE_ERR, "ENRSM: TrWrite generated an "
                                      "error: %d\n", err));
                return;
            }

            //set the message state to CONTINUE
            mps_regData->e_smState = CONTINUE;
            break;
        case M5:
            err = mpc_regProt->ProcessMessageM6(mps_regData, msg);
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "ProcessMessageM6: %d", err);
                //Send NACK
                CStateMachine::SendNack(err);
                mps_regData->e_lastMsgSent = NACK;
                mps_regData->e_smState = CONTINUE;
                return;
            }
            mps_regData->e_lastMsgRecd = M6;

            //Build message 7 with the appropriate encrypted settings
            if(mps_regData->p_enrolleeInfo->b_ap)
            {
                err = mpc_regProt->BuildMessageM7(mps_regData,
                                                  tempBuf,
                                                  mps_regData->apEncrSettings);
            }
            else
            {
                err = mpc_regProt->BuildMessageM7(mps_regData,
                                                  tempBuf,
                                                  mps_regData->staEncrSettings);
            }

            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "BuildMessageM7: %d", err);
                throw errMsg;
            }
            mps_regData->e_lastMsgSent = M7;

            outBuf.Append(tempBuf.Length(), tempBuf.GetBuf());

            //Now send the message to the transport
            err = mpc_trans->TrWrite(m_transportType,
                               (char *)outBuf.GetBuf(),
                               outBuf.Length());
            if(WSC_SUCCESS != err)
            {
                mps_regData->e_smState = FAILURE;
                TUTRACE((TUTRACE_ERR, "ENRSM: TrWrite generated an "
                                      "error: %d\n", err));
                return;
            }
            //set the message state to CONTINUE
            mps_regData->e_smState = CONTINUE;

            break;
        case M7:
            if(WSC_ID_MESSAGE_NACK == msgType)
            {
                uint16 configError;
                err = mpc_regProt->ProcessMessageNack(mps_regData,
                                                      msg,
                                                      &configError);
		if (WSC_SUCCESS != err)
                {
                    stringPrintf(errMsg, 256, "ProcessMessageNack: %d", err);
                    throw errMsg;
                }
                TUTRACE((TUTRACE_ERR, "M8REGENR: Recvd NACK with err code: %d",
                                       configError));
                // mps_regData->e_smState = FAILURE;
                // why we success here??? If we success,
                // then must provide mp_peerEncrSettings
                // which would be NULL??...
	            mps_regData->e_smState = FAILURE;
                mps_regData->e_lastMsgRecd = M8;
                mps_regData->e_lastMsgSent = DONE;

            }
            else
            {
                err = mpc_regProt->ProcessMessageM8(mps_regData, msg,&encrSettings);
                if(WSC_SUCCESS != err)
                {
                    stringPrintf(errMsg, 256, "ProcessMessageM8: %d", err);
                    //Send NACK
                    CStateMachine::SendNack(err);
                    mps_regData->e_lastMsgSent = NACK;
                    mps_regData->e_smState = CONTINUE;
                    return;
                }
                mps_regData->e_lastMsgRecd = M8;
                mp_peerEncrSettings = encrSettings;

                //Send a Done message
                SendDone();
                mps_regData->e_lastMsgSent = DONE;

                //Decide if we need to wait for an ACK
                //Wait only if we're an AP AND we're running EAP
                if((!mps_regData->p_enrolleeInfo->b_ap) ||
                   (m_transportType != TRANSPORT_TYPE_EAP))
                {
                    //set the message state to success
                    mps_regData->e_smState = SUCCESS;
                }
                else
                {
                    //Wait for ACK. set the message state to continue
                    mps_regData->e_smState = CONTINUE;
                }
            }
            break;
        case DONE:
            err = mpc_regProt->ProcessMessageAck(mps_regData, msg);
            if (RPROT_ERR_NONCE_MISMATCH == err) {
                mps_regData->e_smState = CONTINUE; // ignore nonce mismatches
            } else if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "ProcessMessageAck: %d", err);
                throw errMsg;
            }

            //set the message state to success
            mps_regData->e_smState = SUCCESS;
            break;

        case NACK:
            if(WSC_ID_MESSAGE_NACK == msgType)
            {
                uint16 configError;
                err = mpc_regProt->ProcessMessageNack(mps_regData,
                                                      msg,
                                                      &configError);
                if (WSC_SUCCESS != err)
                {
                    stringPrintf(errMsg, 256, "ProcessMessageNack: %d", err);
                    throw errMsg;
                }
                TUTRACE((TUTRACE_ERR, "REGSM: Recvd NACK with err code: %d",
                                       configError));
                mps_regData->e_smState = FAILURE;
            }
            else
            {
                //set the message state to continue
                mps_regData->e_smState = CONTINUE;
                TUTRACE((TUTRACE_ERR, "REGSM: Recvd unexpected message: %d",
                                       msgType));
            }
            break;

        default:
            throw "Unexpected message received";
        }
    }
    catch(uint32 err)
    {
        TUTRACE((TUTRACE_ERR, "ENRSM: HandleMessage threw an exception: %d\n",
                              err));
        //send an empty message to the transport
        mpc_trans->TrWrite(m_transportType, NULL, 0);

        //set the message state to failure
        mps_regData->e_smState = FAILURE;

    }
    catch(char *str)
    {
        TUTRACE((TUTRACE_ERR, "ENRSM: HandleMessage threw an exception: %s\n",
                              str));
        //send an empty message to the transport
        mpc_trans->TrWrite(m_transportType, NULL, 0);

        //set the message state to failure
        mps_regData->e_smState = FAILURE;
    }
    catch(...)
    {
        TUTRACE((TUTRACE_ERR, "ENRSM: HandleMessage threw an unknown "
                              "exception\n"));
        //send an empty message to the transport
        mpc_trans->TrWrite(m_transportType, NULL, 0);

        //set the message state to failure
        mps_regData->e_smState = FAILURE;
    }
}

void *
CEnrolleeSM::M2DTimerThread(IN void *p_data)
{
    TUTRACE((TUTRACE_DBG, "ENRSM: In M2DTimerThread\n"));
    CEnrolleeSM *enrollee = (CEnrolleeSM *)p_data;
    //We need to wait for some time to allow additional M2/M2D messages
    //to arrive
#ifndef __linux__
    Sleep(M2D_SLEEP_TIME*1000);
#else
    sleep(M2D_SLEEP_TIME);
#endif

    WscLock(enrollee->mp_m2dLock);
    if(SM_RECVD_M2 == enrollee->m_m2dStatus)
    {
        //Nothing to be done.
        WscUnlock(enrollee->mp_m2dLock);
        return NULL;
    }
    WscUnlock(enrollee->mp_m2dLock);

    //Push a message into the queue to restart the SM.
    //This has better thread safety
    TUTRACE((TUTRACE_INFO, "ENRSM: M2DTimerThread, 10s timeout, preparing to reset SM.\r\n"));
    S_CB_COMMON *p_NotifyBuf = new S_CB_COMMON;
    p_NotifyBuf->cbHeader.eType = CB_SM_RESET;
    p_NotifyBuf->cbHeader.dataLength = 0;

    TUTRACE((TUTRACE_DBG, "ENRSM: Sending RESET callback to SM\n"));

    StaticCallbackProc(p_NotifyBuf, p_data);

    //Now make the transport send an EAP fail
    TUTRACE((TUTRACE_PROTO, "ENRSM: Sending EAP FAIL\n"));

    TUTRACE((TUTRACE_INFO, "ENRSM: M2DTimerThread, Sending EAP FAIL because M2D Timeout.\r\n"));
    enrollee->mpc_trans->TrWrite(TRANSPORT_TYPE_EAP, NULL, 0);
    return NULL;
}

/* July 01, 2008, Liang Xin
 * M4 Timer Thread, spawned every time after M3 sent out.
 * It's a little low efficiency, but right now just do it like this.
 */
void *CEnrolleeSM::M4TimerThread(IN void * p_data)
{

    TUTRACE((TUTRACE_INFO, "ENRSM: M4TimerThread Started...\r\n"));
    CEnrolleeSM *enrollee = (CEnrolleeSM *)p_data;

#ifndef __linux__
    Sleep(M4_SLEEP_TIME*1000);
#else
    sleep(M4_SLEEP_TIME);
#endif
    TUTRACE((TUTRACE_INFO, "ENRSM: M4TimerThread awaked...\r\n"));
    WscLock(enrollee->mp_m4Lock);
    if(SM_RECVD_M4 != enrollee->m_m4Status)
    {
        /* set time out status */
        enrollee->m_m4Status = SM_TIMEOUT_M4;
        TUTRACE((TUTRACE_INFO, "ENRSM: M4TimerThread set M4 timeout, m_m4Status==%d\r\n",
                        enrollee->m_m4Status));
    }
    WscUnlock(enrollee->mp_m4Lock);
    TUTRACE((TUTRACE_INFO, "ENRSM: M4TimerThread terminating...\r\n"));
    return NULL;
}

