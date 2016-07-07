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
//  File Name: StateMachine.cpp
//  Description: Implements the overall State Machine Class.
//
****************************************************************************/

#include <openssl/bn.h>
#include <openssl/dh.h>

#include "WscHeaders.h"
#include "WscCommon.h"
#include "WscError.h"
#include "Portability.h"
#include "WscQueue.h"
#include "tutrace.h"
#include "slist.h"
//#include "OobUfd.h"
#include "Transport.h"
#include "StateMachineInfo.h"
#include "RegProtoMsgs.h"
#include "RegProtocol.h"
#include "StateMachine.h"

// ****************************
// public methods
// ****************************

/*
 * Name        : CStateMachine
 * Description : Class constructor. Initialize member variables, set
 *                    callback function.
 * Arguments   : none
 * Return type : none
 */
CStateMachine::CStateMachine(IN CRegProtocol *pc_regProt,
                             IN CTransport *pc_trans,
                             IN uint32 operatingMode)
{
    uint32  ret;

    TUTRACE((TUTRACE_DBG, "RegistrarSM constructor\n"));

    try
    {
        m_initialized = false;

        // create callback queue
        mp_cbQ = new CWscQueue();
        if (!mp_cbQ)
            throw "RegSM: mp_cbQ not created";
        mp_cbQ->Init();

        // create callback thread
        ret = WscCreateThread(
                    &m_cbThread,            // thread ID
                    StaticCBThreadProc,     // thread proc
                    (void *)this);            // data to pass to thread

        if (WSC_SUCCESS != ret)
        {
            throw "RegSM: m_cbThread not created";
        }

        if (!pc_regProt || !pc_trans)
            throw "RegSM: pc_regProt or pc_trans invalid";
        mpc_regProt = pc_regProt;
        mpc_trans = pc_trans;
        mps_regData = NULL;
        m_mode = operatingMode;

        memset(&ms_mcCallbackInfo, 0, sizeof(ms_mcCallbackInfo));
    }
    catch( char *err )
    {
        TUTRACE((TUTRACE_ERR, "RegSM Runtime error: %s", err ));
        // TODO: handle runtime error
    }
} // constructor

/*
 * Name        : ~CStateMachine
 * Description : Class destructor. Cleanup if necessary.
 * Arguments   : none
 * Return type : none
 */
CStateMachine::~CStateMachine()
{
    // kill the callback thread
    // mp_cbQ deleted in the callback thread
    KillCallbackThread();
    WscDestroyThread(m_cbThread);

    if(mps_regData)
    {
        TUTRACE((TUTRACE_INFO, "SM: Deleting regData\n"));
        delete mps_regData;
    }

    mps_regData = NULL;
} // destructor

/*
 * Name        : StaticCallbackProc
 * Description : Static callback method that CTransport uses to pass
 *                    info back to RegSM
 * Arguments   : IN void *p_callBackMsg - pointer to the data being
 *                    passed in
 *                 IN void *p_thisObj - pointer to RegSM
 * Return type : none
 */
void
CStateMachine::StaticCallbackProc(
                        IN void *p_callbackMsg,
                        IN void *p_thisObj)
{
    if((!p_callbackMsg) || (!p_thisObj))
        return;

    S_CB_HEADER *p_header = (S_CB_HEADER *)p_callbackMsg;

    uint32 dw_length = sizeof(p_header->dataLength) +
                        sizeof(S_CB_HEADER);

TUTRACE((TUTRACE_DBG, "In CStateMachine::ActualCBThreadProc(): p_header->eType = %d\r\n", p_header->eType));
    uint32 h_status = (((CStateMachine *)p_thisObj)->mp_cbQ)->Enqueue(
                                    dw_length,        // size of the data
                                    4,                // sequence number
                                    p_callbackMsg );// pointer to the data

TUTRACE((TUTRACE_DBG, "CStateMachine::StaticCallbackProc(): h_status = %d\r\n", h_status));
    if (WSC_SUCCESS != h_status)
    {
        TUTRACE((TUTRACE_ERR, "RegSM: Enqueue failed"));
    }

    return;
} // StaticCallbackProc

/*
 * Name        : SetMCCallback
 * Description : Set callback information for MC
 * Arguments   : IN CALLBACK_FN p_mcCallbackFn - pointer to callback function
 *               IN void *cookie - pointer that we pass back in the callback fn
 * Return type : none
 */
uint32
CStateMachine::SetMCCallback(IN CALLBACK_FN p_mcCallbackFn, IN void* cookie)
{
    if (!p_mcCallbackFn)
        return SM_ERR_INVALID_PTR;
    ms_mcCallbackInfo.pf_callback = p_mcCallbackFn;
    ms_mcCallbackInfo.p_cookie = cookie;
    return WSC_SUCCESS;
} // SetMCCallback


// ****************************
// private methods
// ****************************

/*
 * Name        : StaticCBThreadProc
 * Description : This is a static thread procedure for the callback thread.
 *                 Call the actual thread proc.
 * Arguments   : IN void *p_data
 * Return type : none
 *               The method doesn't really return anything but the thread
 *               creation function needs something that returns void *
 */
void *
CStateMachine::StaticCBThreadProc(IN void *p_data)
{
    ((CStateMachine *)p_data)->ActualCBThreadProc();
    return NULL;
} // StaticCBThreadProc

/*
 * Name        : ActualCBThreadProc
 * Description : This is the thread procedure for the callback thread.
 *                 Monitor the callbackQueue, and process all callbacks that
 *                 are received.
 * Arguments   : none
 * Return type : none
 *               The method doesn't really return anything but the thread
 *               creation function needs something that returns void *
 */
void *
CStateMachine::ActualCBThreadProc()
{
    bool    b_done = false;
    uint32    h_status;
    void    *p_message;
    S_CB_HEADER *p_header;

    // keep doing this until the thread is killed
    while (!b_done)
    {
        WscSleep(0);
        // block on the callbackQueue
        h_status = mp_cbQ->Dequeue(
                            NULL,        // size of dequeued msg
                            4,            // sequence number
                            0,            // infinite timeout
                            (void **) &p_message);
                                        // pointer to the dequeued msg

        if (WSC_SUCCESS != h_status)
        {
            // something went wrong
            TUTRACE((TUTRACE_ERR, "RegSM: Dequeue failed"));
            b_done = true;
            break; // from while loop
        }

        p_header = (S_CB_HEADER *)p_message;
TUTRACE((TUTRACE_DBG, "In CStateMachine::ActualCBThreadProc(): p_header->eType = %d\r\n",
                p_header->eType));

        // once we get something, parse it,
        // do whats necessary, and then block
        switch(p_header->eType)
        {
            case CB_QUIT:
            {
                // no params
                // destroy the queue
                if (mp_cbQ)
                {
                    mp_cbQ->DeInit();
                    delete mp_cbQ;
                }
                // kill the thread
                b_done = true;
                break;
            }

            //Bunch all supported transport types here.
            //Let them fall through and handle them the same way
            case CB_TREAP:
            case CB_TRIP:
            case CB_TRUPNP_CP:
            case CB_TRUPNP_DEV:
            {

                if(!TranslateTrType(p_header->eType, m_transportType))
                {
                    TUTRACE((TUTRACE_ERR, "Unknown transport type\n"));
                    break;
                }
                TUTRACE((TUTRACE_INFO, "Calling CStateMachine::Step, Transport Type %d(CB_TRxx). \r\n", p_header->eType));
                if(WSC_SUCCESS != Step(p_header->dataLength,
                                        ((uint8 *)p_message)+sizeof(S_CB_HEADER)))
                {
                    TUTRACE((TUTRACE_ERR, "CStateMachine::Step failed, RestartSM().\r\n"));
                    RestartSM();
                }
            }
                break;
            case CB_SM_RESET:
                TUTRACE((TUTRACE_INFO, "SM: Got CB_SM_RESET\n"));
                RestartSM();
                break;
            default:
                TUTRACE((TUTRACE_ERR, "Unknown eType. Doing nothing...\n"));
                // not understood, do nothing
                break;
        } // switch

        // free the data
        delete (uint8 *) p_message;
    } // while

    //pthread_exit(NULL);
    return NULL;
} // ActualCBThreadProc

/*
 * Name        : KillCallbackThread
 * Description : Attempt to terminate the callback thread. Enqueue a
 *                 CB_QUIT in the callbackQueue
 * Arguments   : none
 * Return type : void
 */
void
CStateMachine::KillCallbackThread() const
{
    // enqueue a CB_QUIT
    S_CB_HEADER *p = new S_CB_HEADER;
    p->eType = CB_QUIT;
    p->dataLength = 0;

    mp_cbQ->Enqueue(sizeof(S_CB_HEADER), 4, p);
    return;
} // KillCallbackThread

uint32
CStateMachine::SetLocalDeviceInfo(IN S_DEVICE_INFO *p_localInfo,
                                IN uint32 *p_lock,
                                DH *p_dhKeyPair)
{
    if((!p_localInfo) || (!p_lock) || (!p_dhKeyPair))
    {
        TUTRACE((TUTRACE_ERR, "SM: invalid input parameters\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    mps_localInfo = p_localInfo;
    mp_infoLock = p_lock;
    mp_dhKeyPair = p_dhKeyPair;

    return WSC_SUCCESS;
}//SetLocalDeviceInfo

bool
CStateMachine::TranslateTrType(int cbType, TRANSPORT_TYPE &trType)
{
    switch(cbType)
    {
    case CB_TRUFD:
        trType = TRANSPORT_TYPE_UFD;
        break;
    case CB_TREAP:
        trType = TRANSPORT_TYPE_EAP;
        break;
    case CB_TRNFC:
        trType = TRANSPORT_TYPE_NFC;
        break;
    case CB_TRIP:
        trType = TRANSPORT_TYPE_IP;
        break;
    case CB_TRUPNP_CP:
        trType = TRANSPORT_TYPE_UPNP_CP;
        break;
    case CB_TRUPNP_DEV:
        trType = TRANSPORT_TYPE_UPNP_DEV;
        break;
    default:
        return false;
    }
    return true;
}//TranslateTrType

uint32
CStateMachine::InitializeSM()
{
    try
    {
        //remove old info if any
        if(mps_regData)
        {
            TUTRACE((TUTRACE_DBG, "SM: Deleting regData\n"));
            delete mps_regData;
        }

        if(!(mps_regData = new S_REGISTRATION_DATA))
            throw WSC_ERR_OUTOFMEMORY;

        mps_regData->e_smState = START;
        mps_regData->e_lastMsgRecd = MNONE;
        mps_regData->e_lastMsgSent = MNONE;
        mps_regData->DHSecret = mp_dhKeyPair;
        mps_regData->staEncrSettings = NULL;
        mps_regData->apEncrSettings = NULL;
        mps_regData->p_enrolleeInfo = NULL;
        mps_regData->p_registrarInfo = NULL;

        m_initialized = true;
        return WSC_SUCCESS;
    }
    catch(uint32 err)
    {
        TUTRACE((TUTRACE_ERR, "REGSM: InitializeSM generated an exception: "
                              "%d\n", err));
        return err;
    }
}//InitializeSM

void
CStateMachine:: SetPassword(IN char *p_devicePasswd,
                     IN uint32 passwdLength)
{
        if(p_devicePasswd && passwdLength)
        {
            mps_regData->password.Reset();
            mps_regData->password.Append(passwdLength,(uint8 *)p_devicePasswd);
        }
}//SetPassword

void
CStateMachine::NotifyMasterControl(uint32 status,
                                   S_DEVICE_INFO *p_peerDeviceInfo,
                                   void *p_peerSettings)
{

    TUTRACE((TUTRACE_DBG, "In SM: NotifyMasterControl, status==%d, p_peerDeviceInfo==%8x"\
            ", p_peerSettings==%8x\r\n", status, p_peerDeviceInfo,
                    p_peerSettings));
    S_CB_SM *p_mcNotifyBuf = new S_CB_SM;
    p_mcNotifyBuf->cbHeader.eType = CB_SM;
    p_mcNotifyBuf->cbHeader.dataLength =sizeof(S_CB_SM)-sizeof(S_CB_HEADER);
    p_mcNotifyBuf->result = status;
    p_mcNotifyBuf->encrSettings = p_peerSettings;

    S_DEVICE_INFO *tempInfo = NULL;
    DuplicateDeviceInfo((S_DEVICE_INFO *)p_peerDeviceInfo, &tempInfo);

    p_mcNotifyBuf->peerInfo = tempInfo;
    TUTRACE((TUTRACE_DBG, "Sending msg eType==%d(CB_SM) to MasterControl Callback: pf_callback==%8x"\
            ", p_cookie==%8x\r\n", CB_SM, ms_mcCallbackInfo.pf_callback));
    if (ms_mcCallbackInfo.pf_callback)
        ms_mcCallbackInfo.pf_callback((void *)p_mcNotifyBuf,
                                    ms_mcCallbackInfo.p_cookie);
}//NotifyMasterControl

void
CStateMachine::RestartSM()
{
    BufferObj passwd;
    void *apEncrSettings, *staEncrSettings;;

    TUTRACE((TUTRACE_DBG, "SM: Restarting the State Machine\n"));

    //first extract the all info we need to save
    passwd.Append(mps_regData->password.Length(),
                  mps_regData->password.GetBuf());
    staEncrSettings = mps_regData->staEncrSettings;
    apEncrSettings = mps_regData->apEncrSettings;

    if(MODE_REGISTRAR == m_mode)
    {
        if(mps_regData->p_enrolleeInfo)
        {
            delete mps_regData->p_enrolleeInfo;
            mps_regData->p_enrolleeInfo = NULL;
        }
    }
    else
    {
        if(mps_regData->p_registrarInfo)
        {
            delete mps_regData->p_registrarInfo;
            mps_regData->p_registrarInfo = NULL;
        }
    }

    //next, reinitialize the base SM class
    uint32 err = InitializeSM();
    if(WSC_SUCCESS != err)
        return;

    //Finally, store the data back into the regData struct
    if(MODE_REGISTRAR == m_mode)
    {
        mps_regData->p_registrarInfo = mps_localInfo;
    }
    else
    {
        mps_regData->p_enrolleeInfo = mps_localInfo;
    }

    SetPassword((char *)passwd.GetBuf(), passwd.Length());
    SetEncryptedSettings(staEncrSettings, apEncrSettings);

    m_initialized = true;
}//RestartSM

uint32
CStateMachine::DuplicateDeviceInfo(S_DEVICE_INFO *inInfo,
                                   S_DEVICE_INFO **outInfo)
{
    if(!inInfo)
        return WSC_ERR_INVALID_PARAMETERS;

    *outInfo = new S_DEVICE_INFO;
    if(!*outInfo)
    {
        TUTRACE((TUTRACE_ERR, "SM: Unable to allocate memory for duplicating"
                              " deviceInfo\n"));
        return WSC_ERR_OUTOFMEMORY;
    }

    memcpy(*outInfo, inInfo, sizeof(S_DEVICE_INFO));
    return WSC_SUCCESS;
}//DuplicateDeviceInfo

void
CStateMachine::SetEncryptedSettings(IN void * p_StaEncrSettings,
                                    IN void * p_ApEncrSettings)
{
    if(!mps_regData)
        return;

    mps_regData->staEncrSettings = p_StaEncrSettings;
    mps_regData->apEncrSettings = p_ApEncrSettings;
}//SetEncryptedSettings

uint32 CStateMachine::SendAck()
{
    uint32 err;
    BufferObj msg;

    err = mpc_regProt->BuildMessageAck(mps_regData, msg);
    if(WSC_SUCCESS != err)
    {
        TUTRACE((TUTRACE_ERR, "REGSM: BuildMessageAck generated an "
                                "error: %d\n", err));
        return err;
    }

    //Now send the message to the transport
    err = mpc_trans->TrWrite(m_transportType,
                        (char *)msg.GetBuf(),
                        msg.Length());
    if(WSC_SUCCESS != err)
    {
        mps_regData->e_smState = FAILURE;
        TUTRACE((TUTRACE_ERR, "REGSM: TrWrite generated an "
                                "error: %d\n", err));
        return err;
    }
    return WSC_SUCCESS;
}//SendAck

uint32 CStateMachine::SendNack(uint16 configError)
{
    uint32 err;
    BufferObj msg;

    err = mpc_regProt->BuildMessageNack(mps_regData, msg, configError);
    if(WSC_SUCCESS != err)
    {
        TUTRACE((TUTRACE_ERR, "REGSM: BuildMessageNack generated an "
                                "error: %d\n", err));
        return err;
    }

    //Now send the message to the transport
    err = mpc_trans->TrWrite(m_transportType,
                        (char *)msg.GetBuf(),
                        msg.Length());
    if(WSC_SUCCESS != err)
    {
        mps_regData->e_smState = FAILURE;
        TUTRACE((TUTRACE_ERR, "REGSM: TrWrite generated an "
                                "error: %d\n", err));
        /* a WAR for WCN Wireless Missing M4 Type Test. */
        system("echo TrWrite err>/tmp/debug");
        return err;
    }
    /* a WAR for WCN Wireless Missing M4 Type Test, maybe we sleep here
     * is OK??
     */
    system("echo TrWrite send Nack success>/tmp/debug");
    return WSC_SUCCESS;
}//SendNack

uint32 CStateMachine::SendDone()
{
    uint32 err;
    BufferObj msg;

    err = mpc_regProt->BuildMessageDone(mps_regData, msg);
    if(WSC_SUCCESS != err)
    {
        TUTRACE((TUTRACE_ERR, "REGSM: BuildMessageDone generated an "
                                "error: %d\n", err));
        return err;
    }

    //Now send the message to the transport
    err = mpc_trans->TrWrite(m_transportType,
                        (char *)msg.GetBuf(),
                        msg.Length());
    if(WSC_SUCCESS != err)
    {
        mps_regData->e_smState = FAILURE;
        TUTRACE((TUTRACE_ERR, "REGSM: TrWrite generated an "
                                "error: %d\n", err));
        return err;
    }
    return WSC_SUCCESS;
}//SendDone

uint32 CStateMachine::UpdateLocalDeviceInfo(IN S_DEVICE_INFO *p_localInfo)
{
    if(!p_localInfo)
    {
        TUTRACE((TUTRACE_ERR, "SM: invalid input parameters\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    mps_localInfo = p_localInfo;

    return WSC_SUCCESS;
}
