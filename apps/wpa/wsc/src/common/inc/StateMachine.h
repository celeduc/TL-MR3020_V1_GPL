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
//  File Name: StateMachine.h
//  Description: Header file for State Machines.
//
****************************************************************************/

#ifndef _REGISTRAR_SM_
#define _REGISTRAR_SM_

#pragma pack(push, 1)

#ifdef __linux__
    #define stringPrintf snprintf
#else
    #define stringPrintf _snprintf
#endif

//M2D Status values
#define SM_AWAIT_M2     0
#define SM_RECVD_M2     1
#define SM_RECVD_M2D    2
#define SM_M2D_RESET    3

/* July 01, 2008, Liang Xin, for m2-m4 timeout */
#define SM_AWAIT_M4     0
#define SM_RECVD_M4     1
/* SM_RESET_M4 means timeout*/
#define SM_TIMEOUT_M4     2

//State Machine operating modes
#define MODE_REGISTRAR 1
#define MODE_ENROLLEE  2

/* July 01, 2008, for wirless discovery */
#define SM_AP_UNCONFIGURED  0
#define SM_AP_CONFIGURED    1

// pre-declared classes
class CRegProtocol;
class CTransport;
class BufferObj;

class CStateMachine
{
protected:
    CRegProtocol *      mpc_regProt;
    CTransport *        mpc_trans;
    TRANSPORT_TYPE      m_transportType; //transport type to use for the next message

    // callback data structures
    CWscQueue *         mp_cbQ;
    //pthread_t         m_cbThread;
    uint32              m_cbThread;

    // flag to indicate whether the SM has been initialized
    bool                m_initialized;

    // enable callback into MasterControl
    S_CALLBACK_INFO     ms_mcCallbackInfo;

    // registration protocol data is stored here
    S_REGISTRATION_DATA *   mps_regData;

    // info for local registrar
    S_DEVICE_INFO *     mps_localInfo;

    //lock for accessing the registration info struct
    uint32 *            mp_infoLock;

    // Diffie Hellman key pair
    DH *                mp_dhKeyPair; //this is copied into mps_regData

    //operating mode - Enrollee or Registrar
    uint32              m_mode;

    // callback thread
    static void * StaticCBThreadProc(IN void *p_data);
    void * ActualCBThreadProc();
    void KillCallbackThread() const;
    bool TranslateTrType(int cbType, TRANSPORT_TYPE &trType);
    uint32 DuplicateDeviceInfo(S_DEVICE_INFO *inINfo, S_DEVICE_INFO **outInfo);
    uint32 SendAck();
    uint32 SendNack(uint16 configError);
    uint32 SendDone();
    virtual void HandleMessage(BufferObj &msg) = 0; //Pure virtual function

public:
    CStateMachine(IN CRegProtocol *pc_regProt,
                  IN CTransport *pc_trans,
                  IN uint32 operatingMode);
    virtual ~CStateMachine();

    // callback function for CTransport to call in
    static void StaticCallbackProc( IN void *p_callbackMsg,
                                    IN void *p_thisObj );

    uint32 SetMCCallback(IN CALLBACK_FN p_mcCallbackFn, IN void* cookie);

    uint32 SetLocalDeviceInfo(IN S_DEVICE_INFO *p_localInfo,
                              IN uint32 *p_lock,
                              DH *p_dhKeyPair);

    uint32 UpdateLocalDeviceInfo(IN S_DEVICE_INFO *p_localInfo);

    uint32 InitializeSM();

    void   NotifyMasterControl(uint32 status,
                               S_DEVICE_INFO *p_peerDeviceInfo,
                               void *p_peerSettings);

    void SetPassword(IN char *p_devicePasswd,
                     IN uint32 passwdLength);
    void SetEncryptedSettings(IN void * p_StaEncrSettings,
                              IN void * p_ApEncrSettings);
    virtual void RestartSM();

    //Pure virtual functions
    virtual uint32 Step(IN uint32 msgLen, IN uint8 *p_msg) = 0;
}; // CStateMachine

class CRegistrarSM:public CStateMachine
{
public:
    CRegistrarSM(IN CRegProtocol *pc_regProt, IN CTransport *pc_trans)
        :CStateMachine(pc_regProt, pc_trans, MODE_REGISTRAR), m_sentM2(false){}

    uint32 InitializeSM(IN S_DEVICE_INFO *p_enrolleeInfo,
                        IN bool enableLocalSM,
                        IN bool enablePassthru);

    uint32 Step(IN uint32 msgLen, IN uint8 *p_msg);
    uint32 RegSMStep(IN uint32 msgLen, IN uint8 *p_msg);
    void RestartSM(){CStateMachine::RestartSM(); m_sentM2 = false;}
    void CRegistrarSM::PBCTimeOut();
    uint32  EnablePassthru(bool enablePassthru);
    void CRegistrarSM::NotifySessionFail();

private:
    void HandleMessage(BufferObj &msg);

    //The peer's encrypted settings will be stored here
    void * mp_peerEncrSettings;
    bool m_localSMEnabled;
    bool m_passThruEnabled;

    //Temporary state variable
    bool m_sentM2;
};//CRegistrarSM

class CEnrolleeSM:public CStateMachine
{
public:
    CEnrolleeSM(IN CRegProtocol *pc_regProt, IN CTransport *pc_trans);
    ~CEnrolleeSM();
    void SetAPConfiguredStat(IN uint32 configured);

    uint32 InitializeSM(IN S_DEVICE_INFO *p_registrarInfo,
                        IN void * p_StaEncrSettings,
                        IN void * p_ApEncrSettings,
                        IN char *p_devicePasswd,
                        IN uint32 passwdLength);

    uint32 Step(IN uint32 msgLen, IN uint8 *p_msg);
    uint32  *mp_m2dLock;
    uint32  m_m2dStatus;
    /* July 01, 2008,  for WCN Logo Test Error Handling 7 */
    uint32 *mp_m4Lock;
    uint32 m_m4Status;
private:
    void HandleMessage(IN BufferObj &msg);


    static void *M2DTimerThread(IN void *p_data);
    static void *M4TimerThread(IN void *p_data);
    //The peer's encrypted settings will be stored here
    void    *mp_peerEncrSettings;

    /* M1 will stored here.XXX, only one for all registrar??
     * June 25, 2008. Liang Xin.
     */
    uint8   m_bufferedM1[512+128];
    uint16  m_bufferedM1Len;
    uint32  m_timerThrdId;

    uint32  m_m4TimerThrdId;

    /* Configured AP and unconf AP function as enrollee,
     * have some difference, the most significant one is that
     * unconf AP should respond Ack to M2D, while Conf AP with
     * NACK
     */
    uint32  m_configuredAP;
};//CEnrolleeSM

#pragma pack(pop)
#endif // _REGISTRAR_SM_
