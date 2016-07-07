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
//  File Name: RegistrarSM.cpp
//  Description: Implements Registrar State Machine Class.
//
****************************************************************************/

// RegistrarSM.cpp
#ifdef WIN32
#include <windows.h>
#endif

#include <string.h>

//OpenSSL includes
#include <openssl/bn.h>
#include <openssl/dh.h>

#include "WscHeaders.h"
#include "WscCommon.h"
#include "WscError.h"
#include "Portability.h"
#include "WscQueue.h"
#include "tutrace.h"
//#include "slist.h"
//#include "OobUfd.h"
#include "Transport.h"
#include "StateMachineInfo.h"
#include "RegProtoMsgs.h"
#include "RegProtocol.h"
#include "StateMachine.h"

// Include the UPnP Microsostack header file so an AP can send an EAP_FAIL right after sending M2D if there
// are no currently-subscribed external Registrars.
#include "UPnPMicroStack.h" 


uint32 
CRegistrarSM::InitializeSM(IN S_DEVICE_INFO *p_enrolleeInfo, 
                           bool enableLocalSM,
                           bool enablePassthru)
{
    TUTRACE((TUTRACE_DBG, "InitializeSM, locEna=%d, Passthr=%d\n",
		enableLocalSM,enablePassthru));
    
    if((!enableLocalSM) && (!enablePassthru))
    {
        TUTRACE((TUTRACE_ERR, "REGSM: enableLocalSM and enablePassthru are "
                              "both false.\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    //if the device is an AP, passthru MUST be enabled
    if((mps_localInfo->b_ap) && (!enablePassthru))
    {
        TUTRACE((TUTRACE_ERR, "REGSM: AP Passthru not enabled.\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    m_localSMEnabled = enableLocalSM;
    m_passThruEnabled = enablePassthru;

    uint32 err = CStateMachine::InitializeSM();
    if(WSC_SUCCESS != err)
        return err;

    mps_regData->p_enrolleeInfo = p_enrolleeInfo;
    mps_regData->p_registrarInfo = mps_localInfo;

    return WSC_SUCCESS;
}

uint32 
CRegistrarSM::EnablePassthru(bool enablePassthru)
{
    TUTRACE((TUTRACE_DBG, "REGSM: Passthrug=%d\n",enablePassthru));
    m_passThruEnabled = enablePassthru;
}

//This method is used by the Proxy to send packets to 
//the various registrars. If the local registrar is 
//enabled, this method will call it using RegStepSM
uint32 
CRegistrarSM::Step(IN uint32 msgLen, IN uint8 *p_msg)
{
    uint32 err = WSC_SUCCESS;

    TUTRACE((TUTRACE_DBG, "REGSM: Entering Step.\n"));
    if(false == m_initialized)
    {
        TUTRACE((TUTRACE_ERR, "REGSM: Not yet initialized.\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    //Irrespective of whether the local registrar is enabled or whether we're 
    //using an external registrar, we need to send a WSC_Start over EAP to
    //kickstart the protocol.
    //If we send a WSC_START msg, we don't need to do anything else i.e. 
    //invoke the local registrar or pass the message over UPnP
    if( (!msgLen) && (START == mps_regData->e_smState)&& 
        (  (TRANSPORT_TYPE_EAP == m_transportType) ||
           (TRANSPORT_TYPE_UPNP_CP == m_transportType)) )

    {
        err = mpc_trans->SendStartMessage(m_transportType);
        if(WSC_SUCCESS != err)
        {
            TUTRACE((TUTRACE_ERR, "REGSM: SendStartMessage failed. Err = %d\n",
                     err));
        }
        return err; //no more processing for WSC_START
    }
    
    //Now send the message over UPnP. Else, if the message has come over UPnP
    //send it over EAP.
    //if(m_passThruEnabled)

    //temporarily disable forwarding to/from UPnP if we've already sent M2
    //this is a stop-gap measure to prevent multiple registrars from cluttering
    //up the EAP session.
    if((m_passThruEnabled) && (!m_sentM2))
    {
        switch(m_transportType)
        {
	case TRANSPORT_TYPE_EAP:
            {
                TUTRACE((TUTRACE_PROTO, "REGSM: Forwarding message from "
                                        "EAP to UPnP.\n"));
                TRANSPORT_TYPE trType;

                if(mps_localInfo->b_ap)
                    trType = TRANSPORT_TYPE_UPNP_DEV;
                else
                    trType = TRANSPORT_TYPE_UPNP_CP;
                
                TUTRACE((TUTRACE_PROTO, "REGSM: Message type = %x\n",
                                      p_msg[WSC_MSG_TYPE_OFFSET]));
                //alwarys forward DONE message to external registrar
                //recvd over EAP, send over UPnP
                err = mpc_trans->TrWrite(trType, 
                                    (char *)p_msg, 
                                    msgLen);

                if(p_msg[WSC_MSG_TYPE_OFFSET] == WSC_ID_MESSAGE_DONE)
                {
                    TUTRACE((TUTRACE_DBG, "REGSM: MESSAGE_DONE received, fail the session\n"));

                    //reset the transport connection
                    mpc_trans->TrWrite(m_transportType, NULL, 0);

                    //reset the SM
                    RestartSM();
                }
                mpc_trans->ClearStaMacAddr();
            }
            break;
        default:
            {
                TUTRACE((TUTRACE_PROTO, "REGSM: Forwarding message from "
                                        "UPnP to EAP.\n"));
                TUTRACE((TUTRACE_PROTO, "REGSM: Message type = %x\n",
                                      p_msg[WSC_MSG_TYPE_OFFSET]));

                //recvd over UPNP, send it out over EAP
                if(msgLen)
                {
			err = mpc_trans->TrWrite(TRANSPORT_TYPE_EAP, 
					    (char *)p_msg, 
					    msgLen);
                }
            }
        }//switch
    }
    
    TUTRACE((TUTRACE_DBG, "REGSM: locRegEna=%d, PassThr=%d.\n",m_localSMEnabled,m_passThruEnabled));
    TUTRACE((TUTRACE_DBG, "REGSM: m_transportType=%d.\n",m_transportType));

    //Send this message to the local registrar only if it hasn't
    //arrived from UPnP. This is needed on an AP with passthrough 
    //enabled so that the local registrar doesn't 
    //end up processing messages coming over UPnP from a remote registrar 
    // Turn on M2D if( (m_localSMEnabled) && (( ! m_passThruEnabled) ||
    if( (m_localSMEnabled) && (( ! m_passThruEnabled) &&
         ((m_transportType != TRANSPORT_TYPE_UPNP_DEV) && 
          (m_transportType != TRANSPORT_TYPE_UPNP_CP))))
    {
        TUTRACE((TUTRACE_PROTO, "REGSM: Sending msg to local registrar.\n"));
        err = RegSMStep(msgLen, p_msg);
    }

    if(WSC_SUCCESS != err)
    {
        mps_regData->e_smState = FAILURE;
        TUTRACE((TUTRACE_ERR, "REGSM: TrWrite generated an "
                                "error: %d\n", err));
        return err;
    }

    return WSC_SUCCESS;
}

uint32 
CRegistrarSM::RegSMStep(IN uint32 msgLen, IN uint8 *p_msg)

{
    uint32 err;
    BufferObj *inMsg = NULL;

    TUTRACE((TUTRACE_DBG, "REGSM: Entering RegSMStep.\n"));

    TUTRACE((TUTRACE_DBG, "REGSM: Recvd message of length %d\n", msgLen));

    if((!p_msg || !msgLen) && (START != mps_regData->e_smState))
    {
        TUTRACE((TUTRACE_ERR, "REGSM: Wrong input parameters.\n"));
        //Notify the MasterControl
        NotifyMasterControl(SM_FAILURE, NULL, NULL);
        return WSC_ERR_INVALID_PARAMETERS;
    }


    if((MNONE == mps_regData->e_lastMsgSent) && (msgLen))
    {
        //We've received a msg from the enrollee, we don't want to send
        //WSC_Start in this case
        //Change last message sent from MNONE to MSTART
        mps_regData->e_lastMsgSent = MSTART;
    }

    //S_WSC_HEADER * hdr = (S_WSC_HEADER *)p_msg;
    //if((hdr->opCode < WSC_Start) || (hdr->opCode > WSC_Done))
    //{
    //    TUTRACE((TUTRACE_ERR, "REGSM: Wrong opcode.\n"));
    //    //Notify the MasterControl
    //    m_initialized = false;
    //    NotifyMasterControl(SM_FAILURE, NULL, NULL);
    //    return SM_ERR_MESSAGE_DATA;
    //}

    //if(hdr->flags & 0x02)
    //{
    //    //the message length field is included
    //    //ignore it for now and just get the message
    //    temp = p_msg+sizeof(S_WSC_FRAGMENT_HEADER);
    //    tempLen = msgLen - sizeof(S_WSC_FRAGMENT_HEADER);
    //}
    //else
    //{
    //    temp = p_msg+sizeof(S_WSC_HEADER);
    //    tempLen = msgLen - sizeof(S_WSC_HEADER);
    //}

    //BufferObj regProtoMsg(temp, tempLen);
    //inMsg = &regProtoMsg;

    //switch(hdr->opCode)
    //{
    //case WSC_Start:
    //case WSC_ACK:
    //case WSC_NACK:
    //case WSC_Done:
    //case WSC_MSG:
    //    //First, check if we need to process this message.
    //    //If the nonce isn't ours, we should ignore it.
    //    if(tempLen)
    //    {
    //    }
    //        
    //    break;
    //default:
    //    //set the message state to failure
    //    mps_regData->e_smState = FAILURE;
    //}

    BufferObj regProtoMsg(p_msg, msgLen);
    inMsg = &regProtoMsg;

    err = mpc_regProt->CheckNonce(mps_regData->registrarNonce, 
                                    *inMsg, 
                                    WSC_ID_REGISTRAR_NONCE);
    //make an exception for M1. It won't have the registrar nonce
    if((WSC_SUCCESS != err) && (RPROT_ERR_REQD_TLV_MISSING != err))
    {
        TUTRACE((TUTRACE_PROTO, "REGSM: Recvd message is not meant for"
                                " this registrar. Ignoring...\n"));
        return WSC_SUCCESS;
    }

    TUTRACE((TUTRACE_DBG, "REGSM: Calling HandleMessage...\n"));
    HandleMessage(*inMsg);

    //now check the state so we can act accordingly
    switch(mps_regData->e_smState)
    {
    case START:
    case CONTINUE:
        //do nothing.
        break;
    case RESTART:
        {
            //reset the SM
            RestartSM();
        }
        break;
    case SUCCESS:
        {
            TUTRACE((TUTRACE_ERR, "REGSM: Notifying MC of success.\n"));
            NotifyMasterControl(SM_SUCCESS, 
                                mps_regData->p_enrolleeInfo, 
                                mp_peerEncrSettings);
            
            //reset the transport connection
            mpc_trans->TrWrite(m_transportType, NULL, 0);

            //reset the SM
            RestartSM();
        }
        break;
    case FAILURE:
        {
            TUTRACE((TUTRACE_ERR, "REGSM: Notifying MC of failure.\n"));
            NotifyMasterControl(SM_FAILURE, NULL, NULL);

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

void CRegistrarSM::PBCTimeOut()
{
    TUTRACE((TUTRACE_DBG, "REGSM: *******Entering PBCTimeOut.\n"));
    
    NotifyMasterControl(SM_FAILURE, NULL, NULL);

    //reset the transport connection
    //mpc_trans->TrWrite(m_transportType, NULL, 0);
 
    //reset the SM
    RestartSM();
}

void CRegistrarSM::NotifySessionFail()
{
    TUTRACE((TUTRACE_DBG, "REGSM: NotifySessionFail.\n"));
    
    NotifyMasterControl(SM_FAILURE, NULL, NULL);

    //reset the SM
    RestartSM();
}

void CRegistrarSM::HandleMessage(BufferObj &msg)
{
    uint32 err;
    char errMsg[256];
    void *encrSettings = NULL;
    BufferObj tempBuf1;
    BufferObj tempBuf;
    BufferObj outBuf;
    uint32 msgType;
    
    try
    {
        //Append the header before doing any processing
        //S_WSC_HEADER hdr;
        //hdr.opCode = WSC_MSG;
        //hdr.flags = 0;
        //outBuf.Append(sizeof(hdr), (uint8 *)&hdr);

        err = mpc_regProt->GetMsgType(msgType, msg);
        if(WSC_SUCCESS != err)
        {
            stringPrintf(errMsg, 256, 
                            "ENRSM: GetMsgType returned error: %d\n", err);
            throw errMsg;
        }

        switch(mps_regData->e_lastMsgSent)
        {
        case MSTART:
            if(WSC_ID_MESSAGE_M1 != msgType)
            {
                TUTRACE((TUTRACE_ERR, "REGSM: Expected M1, received %d\n",
                          msgType));
                return;
            }

            err = mpc_regProt->ProcessMessageM1(mps_regData, msg);
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "ProcessMessageM1: %d", err);
                throw errMsg;
            }
            mps_regData->e_lastMsgRecd = M1;

            if(mps_regData->password.Length())
            {
                //We don't send any encrypted settings currently
                err = mpc_regProt->BuildMessageM2(mps_regData, tempBuf, NULL);
                if(WSC_SUCCESS != err)
                {
                    stringPrintf(errMsg, 256, "BuildMessageM2: %d", err);
                    throw errMsg;
                }
                mps_regData->e_lastMsgSent = M2;            
                //temporary change
                m_sentM2 = true;
            }
            else
            {
                //there is no password present, so send M2D
                err = mpc_regProt->BuildMessageM2D(mps_regData, tempBuf);
                if(WSC_SUCCESS != err)
                {
                    stringPrintf(errMsg, 256, "BuildMessageM2D: %d", err);
                    throw errMsg;
                }
                mps_regData->e_lastMsgSent = M2D;

                //Ask the master control to set the password
                //pass the enrollee's info to help the master control make
                //the appropriate decision
                NotifyMasterControl(SM_SET_PASSWD, 
                                    mps_regData->p_enrolleeInfo, 
                                    NULL);
            }
            
            outBuf.Append(tempBuf.Length(), tempBuf.GetBuf());

            //Now send the message to the transport
            err = mpc_trans->TrWrite(m_transportType, 
                               (char *)outBuf.GetBuf(), 
                               outBuf.Length());
            if(WSC_SUCCESS != err)
            {
                mps_regData->e_smState = FAILURE;
                TUTRACE((TUTRACE_ERR, "REGSM: TrWrite generated an "
                                      "error: %d\n", err));
                return;
            }

            //Set state to CONTINUE
            mps_regData->e_smState = CONTINUE;
            break;
        case M2:
            if(WSC_ID_MESSAGE_M3 != msgType)
            {
                TUTRACE((TUTRACE_ERR, "REGSM: Expected M3, received %d\n",
                          msgType));
                return;
            }

            err = mpc_regProt->ProcessMessageM3(mps_regData, msg);
			if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "ProcessMessageM3: %d", err);
                //Send NACK
                CStateMachine::SendNack(err);
                mps_regData->e_lastMsgSent = NACK;
                mps_regData->e_smState = CONTINUE;
                return;
            }
            mps_regData->e_lastMsgRecd = M3;

            err = mpc_regProt->BuildMessageM4(mps_regData, tempBuf);
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "BuildMessageM4: %d", err);
                throw errMsg;
            }
            mps_regData->e_lastMsgSent = M4;
            
            outBuf.Append(tempBuf.Length(), tempBuf.GetBuf());

            //Now send the message to the transport
            err = mpc_trans->TrWrite(m_transportType, 
                               (char *)outBuf.GetBuf(), 
                               outBuf.Length());
            if(WSC_SUCCESS != err)
            {
                mps_regData->e_smState = FAILURE;
                TUTRACE((TUTRACE_ERR, "REGSM: TrWrite generated an "
                                      "error: %d\n", err));
                return;
            }

            //set the message state to CONTINUE
            mps_regData->e_smState = CONTINUE;
            break;
        case M2D:
            if(WSC_ID_MESSAGE_ACK == msgType)
            {
                err = mpc_regProt->ProcessMessageAck(mps_regData, msg);
		if ( WSC_SUCCESS == err && g_TotalUPnPEventSubscribers == 0 ) {
			stringPrintf(errMsg, 256, "No external registrars, send EAP_FAIL and restart %d", err);
					throw errMsg;
		}
            }
            else if(WSC_ID_MESSAGE_NACK == msgType)
            {
                uint16 configError;
                err = mpc_regProt->ProcessMessageNack(mps_regData, 
                                                      msg, 
                                                      &configError);
                if(configError != WSC_ERROR_NO_ERROR)
                {
                    TUTRACE((TUTRACE_ERR, "REGSM: Recvd NACK with config err: "
                                          "%d", configError));
                }
            }
            else
            {
                err = RPROT_ERR_WRONG_MSGTYPE;
            }
            if(WSC_SUCCESS != err)
                throw err;
            else
                mps_regData->e_smState = RESTART; //Done processing for now
            break;
        case M4:
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
                TUTRACE((TUTRACE_ERR, "REGENR: Recvd NACK with err code: %d", 
                                       configError));
                mps_regData->e_smState = FAILURE;
		return;
            }
            else if(WSC_ID_MESSAGE_M5 != msgType)
            {
                TUTRACE((TUTRACE_ERR, "REGSM: Expected M5 (0x09), "
                                      "received %0x\n", msgType));
                return;
            }

            err = mpc_regProt->ProcessMessageM5(mps_regData, msg);
			if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "ProcessMessageM5: %d", err);
                //Send NACK
                CStateMachine::SendNack(err);
                mps_regData->e_lastMsgSent = NACK;
                mps_regData->e_smState = CONTINUE;
                return;
            }
            mps_regData->e_lastMsgRecd = M5;

            err = mpc_regProt->BuildMessageM6(mps_regData, tempBuf);
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "BuildMessageM6: %d", err);
                throw errMsg;
            }
            mps_regData->e_lastMsgSent = M6;            
            
            outBuf.Append(tempBuf.Length(), tempBuf.GetBuf());

            //Now send the message to the transport
            err = mpc_trans->TrWrite(m_transportType, 
                               (char *)outBuf.GetBuf(), 
                               outBuf.Length());
            if(WSC_SUCCESS != err)
            {
                mps_regData->e_smState = FAILURE;
                TUTRACE((TUTRACE_ERR, "REGSM: TrWrite generated an "
                                      "error: %d\n", err));
                return;
            }

            //set the message state to CONTINUE
            mps_regData->e_smState = CONTINUE;
            break;
        case M6:
            if(WSC_ID_MESSAGE_M7 != msgType)
            {
                TUTRACE((TUTRACE_ERR, "REGSM: Expected M7, received %d\n",
                          msgType));
                return;
            }

            err = mpc_regProt->ProcessMessageM7(mps_regData, msg,&encrSettings);
			if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "ProcessMessageM7: %d", err);
                //Send NACK
                CStateMachine::SendNack(err);
                mps_regData->e_lastMsgSent = NACK;
                mps_regData->e_smState = CONTINUE;
                return;
            }
            mps_regData->e_lastMsgRecd = M7;
            mp_peerEncrSettings = encrSettings;

            //Build message 8 with the appropriate encrypted settings
            if(mps_regData->p_enrolleeInfo->b_ap)
            {
                TUTRACE((TUTRACE_PROTO, "REGSM: EncrSet b_ap\n"));
                err = mpc_regProt->BuildMessageM8(mps_regData, 
                                                  tempBuf, 
                                                  mps_regData->apEncrSettings);
            }
            else
            {
                TUTRACE((TUTRACE_PROTO, "REGSM: EncrSet STA\n"));
                err = mpc_regProt->BuildMessageM8(mps_regData, 
                                                  tempBuf, 
                                                  mps_regData->staEncrSettings);
            }
            if(WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "BuildMessageM8: %d", err);
                throw errMsg;
            }
            mps_regData->e_lastMsgSent = M8;

            outBuf.Append(tempBuf.Length(), tempBuf.GetBuf());

            //Now send the message to the transport
            err = mpc_trans->TrWrite(m_transportType, 
                               (char *)outBuf.GetBuf(), 
                               outBuf.Length());
            if(WSC_SUCCESS != err)
            {
                mps_regData->e_smState = FAILURE;
                TUTRACE((TUTRACE_ERR, "REGSM: TrWrite generated an "
                                      "error: %d\n", err));
                return;
            }

            //set the message state to continue
            mps_regData->e_smState = CONTINUE;
            break;
        case M8:
            if(WSC_ID_MESSAGE_DONE != msgType)
            {
                TUTRACE((TUTRACE_ERR, "REGSM: Expected DONE, received %d\n",
                          msgType));
                return;
            }

            err = mpc_regProt->ProcessMessageDone(mps_regData, msg);
			if (WSC_SUCCESS != err)
            {
                stringPrintf(errMsg, 256, "ProcessMessageDone: %d", err);
                throw errMsg;
            }
            
            if(	mps_regData->p_enrolleeInfo->b_ap && 
				(m_transportType == TRANSPORT_TYPE_EAP)) {
                SendAck();
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
                throw "Unexpected message received";
            }
        }
    }
    catch(uint32 err)
    {
        TUTRACE((TUTRACE_ERR, "REGSM: HandleMessage threw an exception: %d\n",
                              err));
        //send an empty message to the transport
        // mpc_trans->TrWrite(m_transportType, NULL, 0); // this will be done by RegSMStep

        //set the message state to failure
        mps_regData->e_smState = FAILURE;

    }
    catch(char *str)
    {
        TUTRACE((TUTRACE_ERR, "REGSM: HandleMessage threw an exception: %s\n",
                              str));
        //send an empty message to the transport
        // mpc_trans->TrWrite(m_transportType, NULL, 0); // this will be done by RegSMStep

        //set the message state to failure
        mps_regData->e_smState = FAILURE;
    }
    catch(...)
    {
        TUTRACE((TUTRACE_ERR, "REGSM: HandleMessage threw an unknown "
                              "exception\n"));
        //send an empty message to the transport
        // mpc_trans->TrWrite(m_transportType, NULL, 0); // this will be done by RegSMStep

        //set the message state to failure
        mps_regData->e_smState = FAILURE;
    }
}
