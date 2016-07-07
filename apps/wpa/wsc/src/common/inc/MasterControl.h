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
//  File Name: MasterControl.h
//  Description: Prototypes for methods implemented in MasterControl.cpp
//
****************************************************************************/

#ifndef _MASTERCONTROL_
#define _MASTERCONTROL_

#include "RegProtoMsgs.h"    // for CTlvEsM* definitions
#pragma pack(push, 1)

class CRegProtocol;
class CTransport;
class CRegistrarSM;
class CEnrolleeSM;
class CInfo;

typedef enum {
    WAIT_WSC_EVENT,
    AP_CFG_STARTED,
    AP_AUTO_CFG_STARTED,
    TP_ADDED_EVENT,
} PROCESS_STATUS_TYPE;

typedef struct { 
	char	ssid[SIZE_32_BYTES+1];
    uint8   macAddr[SIZE_MAC_ADDR];
    uint8   beaconVersion;
    uint8   beaconSCState;
	bool	APSetupLocked;
	bool	selectedRegistrar;
	uint16	devPwdId;
	uint16	selRegConfigMethods;
    bool    b_sentProbeReq;
    bool    b_recdProbeResp;
    // Set b_active to true if user decides to use 
    // this node as a Registrar/AP
    bool    b_active;
    WSC_PROBE_REQUEST_IE    *p_prReqIE;
    WSC_PROBE_RESPONSE_IE   *p_prRespIE;
} __attribute__ ((__packed__)) S_NEIGHBOR_INFO;

class CMasterControl
{
private:
    // pointers to classes
    //CRegProtocol    *mp_regProt;
    //CTransport      *mp_trans;
    //CRegistrarSM    *mp_regSM;
    //CEnrolleeSM     *mp_enrSM;
    
    LPLIST  mp_regInfoList; // store S_DEVICE_INFO *
    LPLIST  mp_enrInfoList; // store S_DEVICE_INFO *
    // mp_neighborInfoList is a temp list to store info on neighbors.
    // Also used to map probe req/resps to the beacon info.
    LPLIST  mp_neighborInfoList; // store S_NEIGHBOR_INFO *

    bool    mb_initialized;
    bool    mb_stackStarted;
	bool	mb_canWritePin;
	bool	mb_canWriteRegistrarPin;
	bool	mb_canReadPin;
	bool	mb_requestedPwd;
	bool	mb_restartSupp;
	bool	mb_APSetupLock;
	bool    mb_pinEntered;
	bool    mb_SSR_Called;
	EMode	me_modeTarget;
    //CInfo   *mp_info;

    // Data to be stored for app configuration
	// Will currently only handle one registration instance at 
	// a time. To enable multiple registrations to occur
	// simultaneously, store each of these settings in a list
    CTlvEsM7Ap	*mp_tlvEsM7Ap;
	CTlvEsM7Enr	*mp_tlvEsM7Enr;
	CTlvEsM8Ap  *mp_tlvEsM8Ap;
	CTlvEsM8Sta	*mp_tlvEsM8Sta;
	uint8        m_peerMacAddr[SIZE_6_BYTES];
    // callback data structures
    CWscQueue    *mp_cbQ;
    uint32       m_cbThreadHandle;
    // store callback paramts for main routine
    S_CALLBACK_INFO m_mainCallbackInfo;

    // callback threads
    static void * StaticCBThreadProc( IN void *p_data );
	void * ActualCBThreadProc();
    void KillCallbackThread() const;
	static void * SetSelectedRegistrarTimerThread(IN void *p_data);  

    uint32 SwitchModeOn( IN EMode e_mode );
    uint32 SwitchModeOff( IN EMode e_mode );
    uint32 ProcessBeaconIE( IN char *ssid,
							IN uint8 (&macAddr)[SIZE_MAC_ADDR], 
                            IN uint8 *p_data, IN uint32 len );
    uint32 ProcessProbeReqIE( IN uint8 (&macAddr)[SIZE_MAC_ADDR], 
                            IN uint8 *p_data, IN uint32 len );
    uint32 ProcessProbeRespIE( IN uint8 (&macAddr)[SIZE_MAC_ADDR],
                            IN uint8 *p_data, IN uint32 len );
    uint32 ProcessRegCompleted( IN bool b_result, IN void *p_encrSettings,
                            IN S_DEVICE_INFO *p_peerInfo );
	uint32 GenerateDevPwd( IN bool b_display );
	//uint32 GeneratePsk();
	uint32 ComputeChecksum( IN unsigned long int PIN );
	uint32 CreateTlvEsM8Ap( IN char *cp_ssid );
	uint32 CreateTlvEsM8Sta();
	void PrintPskValue( IN char *nwKey, IN uint32 nwKeyLen );
	uint32 SendSetSelRegistrar( IN bool b_setSelReg );
        PROCESS_STATUS_TYPE m_process_status;
public:
    CInfo   *mp_info;
    uint32 GeneratePsk();
    // pointers to classes
    CRegProtocol    *mp_regProt;
    CTransport      *mp_trans;
    CEnrolleeSM     *mp_enrSM;
    static CRegistrarSM    *mp_regSM;

    
    CMasterControl();
    ~CMasterControl();
   
    static CRegistrarSM * GetRegSM () { return mp_regSM; } 
    uint32 SetBeaconIE( IN bool b_configured, 
						IN bool b_selRegistrar, 
						IN uint16 devPwdId,
						IN uint16 selRegCfgMethods);
    uint32 SetProbeReqIE( IN uint8 reqType, IN uint16 assocState, 
                            IN uint16 configError );
    uint32 SetProbeRespIE( IN uint8 respType, IN uint8 scState, 
							IN bool b_selRegistrar,
							IN uint16 devPwdId,
							IN uint16 selRegCfgMethods);

    uint32 Init( IN CALLBACK_FN p_mcCallbackFn, 
                    IN void* cookie );
    uint32 DeInit();
    uint32 StartStack();
    uint32 StopStack();

    // callback function for CTransport to call in
    static void StaticCallbackProc(IN void *p_callbackMsg, 
                                    IN void *p_thisObj);

    // Called by the app to initiate registration
    uint32 InitiateRegistration( IN EMode e_currMode, IN EMode e_targetMode,
                            IN char *devPwd, IN char *ssid,
			    IN bool b_useIe = false,
			    IN uint16 devPwdId = WSC_DEVICEPWDID_DEFAULT );
	// Called by the app to indicate the PIN can be written/read to an 
	// OOB device
	uint32 CanWritePin();
	uint32 CanWriteRegistrarPin();
	uint32 CanReadPin();
	// Called by the app to validate the PIN that the user types in
	bool ValidateChecksum( IN unsigned long int PIN );
	// Called by the app to send the device password down to the 
	// Registrar SM
	uint32 SetDevicePassword( IN char *devPwd, IN uint8 *uuid );

    uint32  m_timerThrdId;

    /*
    // methods for manipulating the Registrar list
    S_REGISTRAR_INFO * FindRegistrar(IN uint8 *p_uuid);
    uint32 AddRegistrar(IN S_REGISTRAR_INFO *ps_regInfo);
    uint32 DelRegistrar(IN S_REGISTRAR_INFO *ps_regInfo);

    // methods for manipulating the Enrollee list
    S_ENROLLEE_INFO * FindEnrollee(IN uint8 *p_uuid);
    uint32 AddEnrollee(IN S_ENROLLEE_INFO *ps_enrInfo);
    uint32 DelEnrollee(IN S_ENROLLEE_INFO *ps_enrInfo);
    */

    bool IsPinEntered();
    char *GetDevPwd(uint32 &len)
    {
	return mp_info->GetDevPwd(len);
    }

    PROCESS_STATUS_TYPE getProcessStatus(void);
    void setProcessStatus(PROCESS_STATUS_TYPE processStatus);

    void SwitchWscMode( EMode mode);

    void setAPSetupLock(bool APSetupLock);

}; // CMasterControl

#pragma pack(pop)

#endif // _MASTERCONTROL_
