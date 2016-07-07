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
//  File Name: MasterControl.cpp
//  Description: Provides overall control of the WSC stack. Instantiates
//      all other modules, and is the interface to the UI. Decides when the
//      WSC Registration protocol must be run, and sets up communication
//      channels between the protocol and transport modules.
//
****************************************************************************/

#ifndef __linux__
#include <stdio.h>
#include <windows.h>
#endif

#ifdef __linux__
#include <string.h>      // for memset
#endif

#include <memory>        // for auto_ptr
// #include <assert.h>

#include <openssl/rand.h>
#include <openssl/bn.h>
#include <openssl/dh.h>

#include "Portability.h"
#include "WscHeaders.h"
#include "WscCommon.h"
#include "StateMachineInfo.h"
#include "WscError.h"
#include "WscQueue.h"
#include "Transport.h"
#include "StateMachineInfo.h"
#include "RegProtoMsgs.h"
#include "RegProtocol.h"
#include "StateMachine.h"
#include "tutrace.h"
#include "slist.h"
#include "Info.h"
#include "MasterControl.h"

#define SSR_WALK_TIME 120

// ****************************
// public methods
// ****************************

/*
 * Name        : CMasterControl
 * Description : Class constructor.
 * Arguments   : none
 * Return type : none
 */
CRegistrarSM * CMasterControl::mp_regSM = NULL;

CMasterControl::CMasterControl()
{
    TUTRACE((TUTRACE_MC, "MasterControl constructor\n"));
    mb_initialized	= false;
    mb_stackStarted	= false;
    mb_canWritePin	= false;
    mb_canWriteRegistrarPin	= false;
    mb_canReadPin	= false;
    mb_requestedPwd	= false;
    mb_restartSupp	= false;
    mb_APSetupLock	= false;
    me_modeTarget 	= EModeUnknown;

    mp_regProt  = NULL;
    mp_trans    = NULL;
    //mp_regSM    = NULL;
    mp_enrSM    = NULL;
    mp_info     = NULL;

    mp_regInfoList		= NULL;
    mp_enrInfoList		= NULL;
    mp_neighborInfoList = NULL;

    mp_tlvEsM7Ap	= NULL;
    mp_tlvEsM7Enr	= NULL;
    mp_tlvEsM8Ap	= NULL;
    mp_tlvEsM8Sta	= NULL;
    m_timerThrdId = 0;
    mb_SSR_Called = false;

    mb_pinEntered = false;

    mp_cbQ = NULL;
    m_cbThreadHandle = 0;
    memset( &m_mainCallbackInfo, 0, sizeof(S_CALLBACK_INFO) );
    memset( &m_peerMacAddr, 0, sizeof(m_peerMacAddr) );

} // Constructor

/*
 * Name        : ~CMasterControl
 * Description : Class destructor. Cleanup if necessary.
 * Arguments   : none
 * Return type : none
 */
CMasterControl::~CMasterControl()
{
    TUTRACE((TUTRACE_MC, "MasterControl destructor\n"));

    DeInit();
} // Destructor

/*
 * Name        : Init
 * Description : Initialize member variables, set callback pointers.
 * Arguments   : IN CALLBACK_FN p_mcCallbackFn - pointer to callback routine,
 *                 IN void* cookie - NULL
 * Return type : uint32 - result of the init operation
 */
uint32
CMasterControl::Init( IN CALLBACK_FN p_mainCallbackFn,
                    IN void* cookie )
{
    int    i_ret;

    // If stack is started, then it must also be initialized
    // TODO: good place to add in a check
    if ( mb_initialized )
    {
        TUTRACE((TUTRACE_MC, "MC::Init: Already initialized\n"));
        return WSC_ERR_ALREADY_INITIALIZED;
    }

    // Do the initialization
    try
    {
        // Create callback queue
	// auto_ptr is a template class in STL
        auto_ptr<CWscQueue> p_autoCbQ( new CWscQueue() );
        if ( NULL == p_autoCbQ.get() )
            throw "MC::Init: cbQ not created";
        mp_cbQ = p_autoCbQ.release();
        mp_cbQ->Init();

        // Create callback thread
        i_ret = WscCreateThread(
                        &m_cbThreadHandle,
                        StaticCBThreadProc,	/* true thread */
                        this );
        if (WSC_SUCCESS != i_ret)
            throw "MC::Init: m_cbThread not created";

        // Instantiate CInfo
        auto_ptr<CInfo> p_autoInfo( new CInfo() );
        if ( NULL == p_autoInfo.get() )
            throw "MC::Init: info not created";

        // Instantiate classes
        auto_ptr<CRegProtocol> p_autoRegProt( new CRegProtocol() );
        if ( NULL == p_autoRegProt.get() )
            throw "MC::Init: regProt not created";
        auto_ptr<CTransport> p_autoTrans( new CTransport() );
        if ( NULL == p_autoTrans.get() )
            throw "MC::Init: trans not created";

        if ( !p_mainCallbackFn )
            throw "MC::Init: main callback Fn undefined";

        // Everything's initialized ok
        // Transfer control to member variables
        mp_info = p_autoInfo.release();
        mp_regProt = p_autoRegProt.release();
        mp_trans = p_autoTrans.release();

        m_mainCallbackInfo.pf_callback = p_mainCallbackFn;
        m_mainCallbackInfo.p_cookie = cookie;

        // Init and set their callback addresses
        mp_regProt->SetMCCallback( CMasterControl::StaticCallbackProc, this );

        mp_trans->Init();
        mp_trans->SetMCCallback( CMasterControl::StaticCallbackProc, this );
        mp_trans->SetSMCallback( NULL, NULL );

        TUTRACE((TUTRACE_MC, "MC::Init: Init complete ok\n"));
    }
    catch( char *err )
    {
        TUTRACE( (TUTRACE_MC, "MC::Init: Failed, Runtime error: %s\n", err ));
        mb_initialized = false;
        return WSC_ERR_SYSTEM;
    }

    mb_initialized = true;
    return WSC_SUCCESS;
} // Init

/*
 * Name        : DeInit
 * Description : Deinitialize the stack.
 * Arguments   : none
 * Return type : uint32 - result of the de-init operation
 */
uint32
CMasterControl::DeInit()
{
    if ( !mb_initialized )
    {
        TUTRACE((TUTRACE_MC, "MC::DeInit: Not initialized\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    if ( mb_stackStarted )
    {
        // Need to stop stack first
        StopStack();
    }

    // Cleanup
    mp_trans->SetMCCallback( NULL, NULL );
    mp_trans->SetSMCallback( NULL, NULL );
    mp_trans->Deinit();
    mp_regProt->SetMCCallback( NULL, NULL );

    delete mp_regProt;
    delete mp_trans;

    delete mp_info;

    // Kill the callback thread
    // mp_cbQ deleted in the callback thread
    KillCallbackThread();
    WscDestroyThread( m_cbThreadHandle );

	// These data objects should have been already deleted,
	// but double-check to make sure
	if ( mp_tlvEsM7Ap )
		delete mp_tlvEsM7Ap;
	if ( mp_tlvEsM7Enr )
		delete mp_tlvEsM7Enr;
	if ( mp_tlvEsM8Ap )
		delete mp_tlvEsM8Ap;
	if ( mp_tlvEsM8Sta )
		delete mp_tlvEsM8Sta;

    mb_initialized = false;
    return WSC_SUCCESS;
} // DeInit

/*
 * Name        : StartStack
 * Description : Once stack is initialized, start operations.
 * Arguments   : none
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::StartStack()
{
    uint32 ret;

    if ( !mb_initialized )
    {
        TUTRACE((TUTRACE_MC, "MC::StartStack: Not initialized\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }
    if ( mb_stackStarted )
    {
        TUTRACE((TUTRACE_MC, "MC::StartStack: Already started\n"));
        return MC_ERR_STACK_ALREADY_STARTED;
    }

    // Stack initialized now. Can be started.

    // Create enrollee and registrar lists
    try
    {
        mp_regInfoList = ListCreate();
        if ( !mp_regInfoList )
            throw "MC::Init: mp_regInfoList not created";
        mp_enrInfoList = ListCreate();
        if ( !mp_enrInfoList )
            throw "MC::Init: mp_enrInfoList not created";
        mp_neighborInfoList = ListCreate();
        if ( !mp_neighborInfoList )
            throw "MC::Inti: mp_neighborInfoList not created";
    }
    catch ( char *err )
    {
        TUTRACE((TUTRACE_MC, "MC::StartStack: could not create lists\n"));
        TUTRACE((TUTRACE_MC, "%s\n", err));
        if ( mp_regInfoList )
            ListDelete( mp_regInfoList );
        if ( mp_enrInfoList )
            ListDelete( mp_enrInfoList);
        if ( mp_neighborInfoList )
            ListDelete( mp_neighborInfoList );
        return MC_ERR_STACK_NOT_STARTED;
    }

    // Read in config file
    if ( WSC_SUCCESS != mp_info->ReadConfigFile() )
    {
        TUTRACE((TUTRACE_MC, "MC::StartStack: Config info not setup\n"));
        return MC_ERR_STACK_NOT_STARTED;
    }

	// Initialize other member variables
	mb_canWritePin	= false;
	mb_canReadPin	= false;
	mb_requestedPwd = false;
	mb_restartSupp	= false;

    // Explicitly perform some actions if the stack is configured
    // in a particular mode
	EMode e_mode = mp_info->GetConfiguredMode();
	if ( WSC_SUCCESS != SwitchModeOn( e_mode ))
    {
        TUTRACE((TUTRACE_MC, "MC::StartStack: Could not start trans\n"));
        return MC_ERR_STACK_NOT_STARTED;
    }

    mb_stackStarted = true;

    ret = WSC_SUCCESS;
    // Basic startup done
    // Tell app what mode we're in
    TUTRACE((TUTRACE_MC,
                "MC::StartStack: Informing app of mode\n"));
    S_CB_MAIN_PUSH_MODE *p_msg = new S_CB_MAIN_PUSH_MODE;
    if ( !p_msg )
    {
        TUTRACE((TUTRACE_ERR,
            "MC::SwitchModeOn: Could not allocate p_msg\n"));
        ret = WSC_ERR_OUTOFMEMORY;
    }
    else
    {
	p_msg->cbHeader.eType = CB_MAIN_PUSH_MODE;
	p_msg->cbHeader.dataLength = sizeof(S_CB_MAIN_PUSH_MODE) -
                            sizeof(S_CB_HEADER);
        p_msg->e_mode = e_mode;
	p_msg->b_useUsbKey = mp_info->UseUsbKey();
	p_msg->b_useUpnp = mp_info->UseUpnp();
    TUTRACE((TUTRACE_DBG, "Sending mode selection msg to main callback, e_mode=%d...\r\n",
                        p_msg->e_mode));
        m_mainCallbackInfo.pf_callback( (void *)p_msg,
                                        m_mainCallbackInfo.p_cookie );
        TUTRACE((TUTRACE_MC,
                "MC::SwitchModeOn: Done callback\n"));
    }
    return ret;
} // StartStack

/*
 * Name        : StopStack
 * Description : Stop the stack.
 * Arguments   : none
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::StopStack()
{
    if ( !mb_initialized )
        return WSC_ERR_NOT_INITIALIZED;
    if ( !mb_stackStarted )
        return MC_ERR_STACK_NOT_STARTED;

    // TODO: need to purge data from callback threads here as well

    // Explicitly perform some actions if the stack is configured
    // in a particular mode
	SwitchModeOff( mp_info->GetConfiguredMode() );

    // Reset other member variables
    mb_canWritePin	= false;
    mb_canReadPin	= false;
    mb_restartSupp	= false;

    // Delete all old data from Registrar and Enrollee Info lists
    if ( mp_regInfoList )
    {
        S_DEVICE_INFO *p_deviceInfo;
        LPLISTITR listItr = ListItrCreate( mp_regInfoList );
        if ( !listItr )
            throw WSC_ERR_OUTOFMEMORY;

        while ((p_deviceInfo = (S_DEVICE_INFO *)ListItrGetNext(listItr)))
        {
            delete p_deviceInfo;
        }
        ListItrDelete( listItr );
        ListDelete( mp_regInfoList );
    }
    if ( mp_enrInfoList )
    {
        S_DEVICE_INFO *p_deviceInfo;
        LPLISTITR listItr = ListItrCreate( mp_enrInfoList );
        if ( !listItr )
            throw WSC_ERR_OUTOFMEMORY;

        while ((p_deviceInfo = (S_DEVICE_INFO *)ListItrGetNext(listItr)))
        {
            delete p_deviceInfo;
        }
        ListItrDelete( listItr );
        ListDelete( mp_enrInfoList );
    }

    if ( mp_neighborInfoList )
    {
        S_NEIGHBOR_INFO *p_neighborInfo;
        LPLISTITR listItr = ListItrCreate( mp_neighborInfoList );
        if ( !listItr )
            throw WSC_ERR_OUTOFMEMORY;

        while ((p_neighborInfo = (S_NEIGHBOR_INFO *)ListItrGetNext(listItr)))
        {
            delete p_neighborInfo;
        }
        ListItrDelete( listItr );
        ListDelete( mp_neighborInfoList );
    }

    mb_stackStarted = false;
    return WSC_SUCCESS;
} // StopStack

/*
 * Name        : StaticCallbackProc
 * Description : Static callback method that other objects use to pass
 *                    info back to the MasterControl
 * Arguments   : IN void *p_callBackMsg - pointer to the data being
 *                    passed in
 *                 IN void *p_thisObj - pointer to MC
 * Return type : none
 */
void
CMasterControl::StaticCallbackProc(
                        IN void *p_callbackMsg,
                        IN void *p_thisObj)
{
    S_CB_HEADER *p_header = (S_CB_HEADER *)p_callbackMsg;

    uint32 dw_length = sizeof(p_header->dataLength) +
                        sizeof(S_CB_HEADER);

    TUTRACE((TUTRACE_DBG, "Enter MC::StaticCallbackProc()\r\n"));
    TUTRACE((TUTRACE_MC, "MC::StaticCallbackProc: Enqueue \n"));

    uint32 h_status = (((CMasterControl *)p_thisObj)->mp_cbQ)->Enqueue(
                                    dw_length,        // size of the data
                                    3,                // sequence number
                                    p_callbackMsg );// pointer to the data

    TUTRACE((TUTRACE_DBG, "MC::StaticCallbackProc(): h_status = %d\r\n", h_status));
    if ( WSC_SUCCESS != h_status )
    {
    TUTRACE((TUTRACE_ERR, "MC::StaticCallbackProc: Enqueue failed\n"));
    }
    return;
} // StaticCallbackProc
void GetDevPwdString(char * pwdbuff, long maxsize, CTlvOobDevPwd &pwd)
{
	pwdbuff[0] = '\0';
	if (pwd.devPwdLength * 2 > maxsize) {
		return;
	}
	uint8 * tmp = pwd.ip_devPwd;
	int j = 0;
	for ( int i=0; i<pwd.devPwdLength; i++ )
	{
		sprintf( &(pwdbuff[j]), "%02X", tmp[i] );
		j+=2;
	}
	pwdbuff[j] = '\0';
}

// Called by the app to initiate registration
uint32
CMasterControl::InitiateRegistration(
                        IN EMode e_currMode, IN EMode e_targetMode,
                        IN char *devPwd, IN char *ssid,
			IN bool b_useIe /*= false*/,
			IN uint16 devPwdId /* = WSC_DEVICEPWDID_DEFAULT */ )
{
    uint32 ret = WSC_SUCCESS;

    switch( e_currMode )
    {
        case EModeUnconfAp:
        {
	     // moved to SwitchModeOn
             break;
        }

        case EModeApProxyRegistrar:
        case EModeRegistrar:
        {
	    // Reset the local device info in the SM if it has changed
	    // SetDevPwdId() will return MC_ERR_VALUE_UNCHANGED if the value
	    // is the same, WSC_SUCCESS if the value was different and the set
	    // succeeded
	    if ( WSC_SUCCESS == mp_info->SetDevPwdId( devPwdId ))
	    {
	        // Reset local device info in SM
	 	mp_regSM->UpdateLocalDeviceInfo( mp_info->GetDeviceInfo() );
	    }
            // Registrar needs to configure AP or add Enrollee

            if ( EModeUnconfAp == e_targetMode )
            {
                uint16 data16, ssidLen;
                char *cp_enrSSID;
                char *cp_data;

                // Start the WPA-SUPP, if needed
		if ( mp_info->IsRegWireless() )
		{
			TUTRACE((TUTRACE_MC,
				"MC::InitiateRegistration: Starting Supplicant\n"));
			S_CB_MAIN_START_WPASUPP *p_cbMsg =
							new S_CB_MAIN_START_WPASUPP;
			if ( !p_cbMsg )
			{
				TUTRACE((TUTRACE_ERR, "MC::InitiateRegistration: "
							"Could not allocate p_cbMsg\n"));
				ret = WSC_ERR_OUTOFMEMORY;
				break;
			}
			p_cbMsg->eType = CB_MAIN_START_WPASUPP;
			p_cbMsg->dataLength = sizeof(S_CB_MAIN_START_WPASUPP) -
									sizeof(S_CB_HEADER);
			m_mainCallbackInfo.pf_callback( (void *)p_cbMsg,
							m_mainCallbackInfo.p_cookie );
			WscSleep( 1 );

			// We're going to initiate a WSC registration now,
			// so configure the supplicant with this info
			// Reconfigure the supplicant only if we aren't using IEs.
         		// If we are, do the reconfigure once we receive a
			// Beacon/ProbeResp
			if ( !b_useIe )
			{
				TUTRACE((TUTRACE_MC,
					"MC::InitiateRegistration: Resetting "
					"Supplicant\n"));
				S_CB_MAIN_RESET_WPASUPP *p_cbMsg2 =
								new S_CB_MAIN_RESET_WPASUPP;
				if ( !p_cbMsg2 )
				{
					TUTRACE((TUTRACE_ERR, "MC::InitiateRegistration: "
							"Could not allocate p_cbMsg2\n"));
	        			ret = WSC_ERR_OUTOFMEMORY;
		         		break;
				}
				p_cbMsg2->cbHeader.eType = CB_MAIN_RESET_WPASUPP;
				p_cbMsg2->cbHeader.dataLength = sizeof(S_CB_MAIN_RESET_WPASUPP) -
								sizeof(S_CB_HEADER);
				// Fill in SSID
				cp_enrSSID = mp_info->GetSSID( ssidLen );
				strncpy( p_cbMsg2->ssid, cp_enrSSID, ssidLen );
				p_cbMsg2->ssid[ssidLen] = '\0';

				// Fill in keyMgmt
				cp_data = mp_info->GetKeyMgmt( data16 );
				strncpy( p_cbMsg2->keyMgmt, cp_data, data16 );
				p_cbMsg2->keyMgmt[data16] = '\0';
				// PSK not used here - leave blank
				memset( p_cbMsg2->nwKey, 0, SIZE_64_BYTES );
				p_cbMsg2->nwKeyLen = 0;
				// Fill in identity
				strcpy( p_cbMsg2->identity, REGISTRAR_ID_STRING );
				p_cbMsg2->b_startWsc = true;
				m_mainCallbackInfo.pf_callback(	(void *)p_cbMsg2,
							        m_mainCallbackInfo.p_cookie );
				TUTRACE((TUTRACE_MC, "MC::InitiateRegistration: "
									"Pushed supp restart req\n"));
			}
			else
			{
				// Using IEs

				// Wait until the supp has had a chance to start
				WscSleep( 3 );
				// Set the ProbeReq in the transport
            			// SetProbeReqIE( WSC_MSGTYPE_REGISTRAR,
                                //     WSC_ASSOC_NOT_ASSOCIATED, WSC_ERROR_NO_ERROR );

				// Tell the transport that we're interested in receiving beacons
				mp_trans->SendBeaconsUp( true );

				// Shortcut: Set a flag so that ProcessBeaconIE() will know to
	        		// restart the supplicant when it receives a (beacon)
				// probe response
				mb_restartSupp = true;
			}
		}

		// We've already set tlvEsM8Sta, now we have the ssid, so
		// set tlvEsM8Ap.  Note that this assumes we are setting up an unconfigured
		// AP, not establishing a Registrar for a configured AP using the IE method.
		//
		CreateTlvEsM8Ap(NULL);
                // Create the encrypted settings TLV for STA
                CreateTlvEsM8Sta();
		mp_regSM->SetEncryptedSettings(
						(void *)mp_tlvEsM8Sta,
						(void *)mp_tlvEsM8Ap );

		// If not wireless and UPnP is enabled, we need to trigger
		// the registrar SM
		if ( !(mp_info->IsRegWireless()) &&  mp_info->UseUpnp() )
		{
			// Send a Step to SM
			S_CB_HEADER *p_step = new S_CB_HEADER;
			p_step->eType = CB_TRUPNP_CP;
			p_step->dataLength = 0;
			CRegistrarSM::StaticCallbackProc(
							(void *)p_step,
        						mp_regSM );
			// Also set me_modeTarget, so if the password
			// comes down later, we can do another step
			me_modeTarget = EModeUnconfAp;
	        }
	    }
            else if ( EModeClient == e_targetMode )
            {
	         	// Added to support SelectedRegistrar
			// Also set me_modeTarget, so if the password
			// comes down later, we can ask the transport to
			// send a UPnP message to the AP
			me_modeTarget = EModeClient;

			// Nothing else here for now
                        // Create the encrypted settings TLV for STA
                        CreateTlvEsM8Sta();
		        mp_regSM->SetEncryptedSettings(
						(void *)mp_tlvEsM8Sta,
						(void *)mp_tlvEsM8Ap );

            }
	    // [Added to support SelectedRegistrar]
	    bool b_setSelRegNeeded = false;

	    // Check to see if the devPwd from the app is valid
	    if ( !devPwd )
	    {
		// We need to use the pwd obtained from the USB key
		if ( mp_info->UseUsbKey() )
		{
			TUTRACE((TUTRACE_ERR, "Using pwd from UFD\n"));
			uint32 len;
	        	devPwd = mp_info->GetDevPwd( len );
			if ( 0 == len )
			{
				TUTRACE((TUTRACE_ERR, "No stored pwd\n"));
		        	ret = WSC_ERR_SYSTEM;
				break;
			}

			// Pass the device password on to the SM
			mp_regSM->SetPassword( devPwd, len );
			b_setSelRegNeeded = true;
			}
			else
			{
			        // App sent in a NULL, and USB key isn't sent
				TUTRACE((TUTRACE_ERR, "MC::InitiateRegistration: "
							"No password to use, expecting one\n"));
				ret = WSC_ERR_SYSTEM;
				break;
		        }
		}
		else
		{
			if ( 0 == strlen(devPwd) )
			{
				// No password to use
				// Will need to send an M2D when Enrollee connects
			}
			else
			{
				TUTRACE((TUTRACE_DBG, "Using pwd from app\n"));
				// Pass the device password on to the SM
				mp_regSM->SetPassword( devPwd, (uint32)(strlen(devPwd)) );
				b_setSelRegNeeded = true;
			}
		}

		// Added to support SelectedRegistrar for mode where
		// registrar is configuring a client via a proxy AP
		if ( (EModeRegistrar == e_currMode && EModeClient == e_targetMode) &&
					b_setSelRegNeeded )
		{
			// Ask transport to send a UPnP message to the AP
			// indicating that this registrar has been selected
			TUTRACE((TUTRACE_MC, "MC::InitiateRegistration: "
					"Asking transport to inform AP about SSR\n"));

			SendSetSelRegistrar( b_setSelRegNeeded );
		}
            break;
        }

	case EModeApProxy:
	{
		// Nothing to be done here
		break;
	}

        default:
        {
            break;
        }
    } // switch

    return ret;
} // InitiateRegistration

// Called by the app to indicate the PIN should be written to an
// OOB device
uint32
CMasterControl::CanWritePin()
{
	// Set the canWritePin flag to true, so that whenever the
	// USB key is inserted, the PIN can be written out
	mb_canWritePin = true;
	return WSC_SUCCESS;
} // CanWritePin
// Called by the app to indicate the Registrar PIN should be written to an
// OOB device
uint32
CMasterControl::CanWriteRegistrarPin()
{
	// Set the canWriteRegistrarPin flag to true, so that whenever the
	// USB key is inserted, the PIN can be written out
	mb_canWriteRegistrarPin = true;
	return WSC_SUCCESS;
} // CanWriteRegistrarPin

// Called by the app to indicate the PIN can be read from an OOB device
uint32
CMasterControl::CanReadPin()
{
	// Set the canReadPin flag to true, so that whenever the
	// USB key is inserted, the Pin can be read
	mb_canReadPin = true;
	return WSC_SUCCESS;
} // CanReadPin

bool
CMasterControl::ValidateChecksum( IN unsigned long int PIN )
{
    unsigned long int accum = 0;
	accum += 3 * ((PIN / 10000000) % 10);
	accum += 1 * ((PIN / 1000000) % 10);
	accum += 3 * ((PIN / 100000) % 10);
	accum += 1 * ((PIN / 10000) % 10);
	accum += 3 * ((PIN / 1000) % 10);
	accum += 1 * ((PIN / 100) % 10);
	accum += 3 * ((PIN / 10) % 10);
	accum += 1 * ((PIN / 1) % 10);

    return (0 == (accum % 10));
} // ValidateChecksum

uint32
CMasterControl::SetDevicePassword( IN char *devPwd, IN uint8 *uuid )
{
	if ( !devPwd )
	{
		return WSC_ERR_SYSTEM;
	}

	mp_regSM->SetPassword( devPwd, (uint32)strlen(devPwd) );
	// Also reset the requestedPwd flag, so that subsequent requests
	// will be sent up to the app
	mb_requestedPwd = false;
	// Added to support Selected Registrar
	// Check if we need to have the transport communicate this to the AP
	// Only if we are a registrar and we are configuring a client
	if ( (EModeRegistrar == mp_info->GetConfiguredMode() &&
			EModeClient == me_modeTarget) &&
			mp_info->UseUpnp() )
	{
		SendSetSelRegistrar( true );

		// Reset modeTarget
		// This should not invalidate the check below since that is
		// looking for EModeUnconfAp
		me_modeTarget = EModeUnknown;
	}

	// Check if we need to send a step to the SM
	if ( !(mp_info->IsRegWireless()) &&
			(mp_info->UseUpnp()) &&
			(EModeUnconfAp == me_modeTarget) )
	{
		// Reset modeTarget
		me_modeTarget = EModeUnknown;

		// Send the Step to the SM
		S_CB_HEADER *p_step = new S_CB_HEADER;
		p_step->eType = CB_TRUPNP_CP;
		p_step->dataLength = 0;

		CRegistrarSM::StaticCallbackProc(
						(void *)p_step,
						mp_regSM );
	}
	return WSC_SUCCESS;
} // SetDevicePassword


// ****************************
// private methods
// ****************************

/*
 * Name        : StaticCBThreadProc
 * Description : This is a static thread procedure for the callback thread.
 *                 Call the actual thread proc.
 * Arguments   : IN void *p_data
 * Return type : none
 */
void *
CMasterControl::StaticCBThreadProc(IN void *p_data)
{
    ((CMasterControl *)p_data)->ActualCBThreadProc();
    return NULL;
} // StaticCBThreadProc

/*
 * Name        : ActualCBThreadProc
 * Description : This is the thread procedure for the callback thread.
 *                 Monitor the callbackQueue, and process all callbacks that
 *                 are received.
 *                 Make sure to delete the callback data in the switch
 *                 statement where it is processed.
 * Arguments   : none
 * Return type : none
 */
void *
CMasterControl::ActualCBThreadProc()
{
    bool    b_done = false;
    uint32    h_status;
    void    *p_cbData;
    S_CB_HEADER *p_header;

    // Keep doing this until the thread is killed
    while ( !b_done )
    {
        WscSleep(0);
        p_cbData = NULL;
        // Block on the callbackQueue
        h_status = mp_cbQ->Dequeue(
                            NULL,        // size of dequeued msg
                            3,            // sequence number
                            0,            // infinite timeout
                            (void **) &p_cbData);
                                        // pointer to the dequeued data

        if ( WSC_SUCCESS != h_status )
        {
            // Something went wrong
            TUTRACE((TUTRACE_ERR, "MC::ActualCBThreadProc: "
                        "something wrong, stop main loop\n"));

            b_done = true;
            break; // from while loop
        }

        p_header = (S_CB_HEADER *)p_cbData;
        printf("CMasterControl::ActualCBThreadProc(): p_header->eType = %d\r\n", p_header->eType);

        // Once we get something, parse it,
        // do whats necessary, and then block
        switch( p_header->eType )
        {
            case CB_QUIT:
            {
                // No params
                // Destroy the queue
                if ( mp_cbQ )
                {
                    mp_cbQ->DeInit();
                    delete mp_cbQ;
                }
                // Kill the thread
                b_done = true;
                break;
            }

            case CB_TRWLAN_BEACON:
            {
                S_CB_TRWLAN *p_cbTrWlan = (S_CB_TRWLAN *)p_cbData;
		                ProcessBeaconIE( p_cbTrWlan->ssid,
		                        p_cbTrWlan->macAddr,
                                (uint8 *)(p_cbTrWlan->data),
                                p_cbTrWlan->cbHeader.dataLength  );
                break;
            }

            case CB_TRWLAN_PR_REQ:
            {
                S_CB_TRWLAN *p_cbTrWlan = (S_CB_TRWLAN *)p_cbData;
                ProcessProbeReqIE( p_cbTrWlan->macAddr,
                                (uint8 *)(p_cbTrWlan->data),
                                p_cbTrWlan->cbHeader.dataLength  );

                break;
            }

            case CB_TRWLAN_PR_RESP:
            {
                S_CB_TRWLAN *p_cbTrWlan = (S_CB_TRWLAN *)p_cbData;
                ProcessProbeRespIE( p_cbTrWlan->macAddr,
                                (uint8 *)(p_cbTrWlan->data),
                                p_cbTrWlan->cbHeader.dataLength  );

                break;
            }

            case CB_SM:
            {
                S_CB_SM *p_cbSM = (S_CB_SM *)p_cbData;

                TUTRACE((TUTRACE_DBG, "MasterControl::ActualCBThreadProc, handle CB_SM msg.\r\n"));
                if ( (SM_FAILURE == p_cbSM->result ) ||
                        (SM_SUCCESS== p_cbSM->result) )
            {
            // Process contents of this message
            bool b_result;
            b_result = (SM_FAILURE == p_cbSM->result)?false:true;
            TUTRACE((TUTRACE_DBG, "MC::Calling ProcessRegCompleted(), b_result==%s.\r\n",
                        b_result?"true":"false"));

             ProcessRegCompleted( b_result,
                            p_cbSM->encrSettings,
                            p_cbSM->peerInfo );

            // Inform app of the result
            S_CB_MAIN_PUSH_REG_RESULT *p_cbPushResult =
                    new S_CB_MAIN_PUSH_REG_RESULT;
            p_cbPushResult->cbHeader.eType = CB_MAIN_PUSH_REG_RESULT;
            p_cbPushResult->cbHeader.dataLength =
                        sizeof(S_CB_MAIN_PUSH_REG_RESULT) -
                        sizeof(S_CB_HEADER);
            p_cbPushResult->b_result =
                    (SM_SUCCESS == p_cbSM->result)?true:false;
		    m_mainCallbackInfo.pf_callback( (void *)p_cbPushResult,
								m_mainCallbackInfo.p_cookie );

		    // Delete CB data
		    // peerInfo:
		    // Deleting for now, but need to save this info - TODO
		    delete p_cbSM->peerInfo;
		    // encrSettings deleted in ProcessRegCompleted()
		}
		else if ( SM_SET_PASSWD == p_cbSM->result )
		{
	            // Ask the app for a password only if we haven't already
		    // sent up a request
		    if ( !mb_requestedPwd )
		    {
		       	// Request the app for a password
			S_CB_MAIN_REQUEST_PWD *p_cbReqPwd =
							new S_CB_MAIN_REQUEST_PWD;
			p_cbReqPwd->cbHeader.eType = CB_MAIN_REQUEST_PWD;
		        p_cbReqPwd->cbHeader.dataLength =
							sizeof(S_CB_MAIN_REQUEST_PWD) -
							sizeof(S_CB_HEADER);
		        strcpy( p_cbReqPwd->deviceName,	p_cbSM->peerInfo->deviceName );
			strcpy( p_cbReqPwd->modelNumber,p_cbSM->peerInfo->modelNumber );
                        strcpy( p_cbReqPwd->serialNumber,p_cbSM->peerInfo->serialNumber );
                        memcpy( p_cbReqPwd->uuid,p_cbSM->peerInfo->uuid,SIZE_16_BYTES );
                        memcpy( m_peerMacAddr, p_cbSM->peerInfo->macAddr,SIZE_6_BYTES );
                        m_mainCallbackInfo.pf_callback( (void *)p_cbReqPwd,
			m_mainCallbackInfo.p_cookie );
			mb_requestedPwd = true;
		     }

		// Delete CB data
		delete p_cbSM->peerInfo;
		// encrSettings will be NULL
		}

                break;
            }

            case CB_TRNFC:
            {
                // Sent as S_CB_COMMON, but cast it to S_CB_MAIN_NFC_DATA
                S_CB_MAIN_NFC_DATA *p_cbTr = (S_CB_MAIN_NFC_DATA *)p_cbData;

                // Push this up to the app
                // Calculate the length of the data recd from TrNfc
                uint32 len = sizeof(S_CB_MAIN_NFC_DATA);
                len += (uint32)(strlen(p_cbTr->c_msg));

                uint8 *p_temp = new uint8[len + 1];
                S_CB_MAIN_NFC_DATA *p_cbMsg = (S_CB_MAIN_NFC_DATA *)p_temp;
                if ( !p_cbMsg )
                {
                    TUTRACE((TUTRACE_ERR,
                        "MC::ActualCBThreadProc: "
                        "Could not allocate p_cbMsg\n"));
                    break;
                }
                p_cbMsg->cbHeader.eType = CB_MAIN_NFC_DATA;
                p_cbMsg->cbHeader.dataLength = len  - sizeof(S_CB_HEADER);
                memcpy( p_cbMsg->c_msg, p_cbTr->c_msg, strlen(p_cbTr->c_msg) );
                p_cbMsg->c_msg[strlen(p_cbTr->c_msg)] = '\0';

                m_mainCallbackInfo.pf_callback( (void *)p_cbMsg,
                                            m_mainCallbackInfo.p_cookie );
                break;
            }

			case CB_SSR_TIMEOUT:
			{
				SetBeaconIE( /*configured*/ true, /* SSR */ false , 0, 0);
				// Enable probe response IE
				SetProbeRespIE(
							WSC_MSGTYPE_AP_WLAN_MGR,
							WSC_SCSTATE_CONFIGURED,
							false, 0, 0);
				break;
			}
			case CB_TRUPNP_DEV_SSR:
			{
				TUTRACE((TUTRACE_MC, "MC::Recd CB_TRUPNP_DEV_SSR\n"));

				// Sent as S_CB_COMMON, but cast it to S_CB_DATA_TEMPLATE
                S_CB_DATA_TEMPLATE *p_cbTr =
								(S_CB_DATA_TEMPLATE *)p_cbData;

				// Added to support SetSelectedRegistrar
				// If we are an AP+Proxy, then add the SelectedRegistrar TLV
				// to the WSC IE in the Beacon
				// This call will fail if the right WLAN drivers are not used.

				//if ( EModeApProxy == mp_info->GetConfiguredMode() )
                                if ( EModeApProxyRegistrar == mp_info->GetConfiguredMode() )
				{
					// De-serialize the data to get the TLVs
					BufferObj bufObj(
							(uint8 *)p_cbTr->c_msg,
							p_cbTr->cbHeader.dataLength );

					// Version
					CTlvVersion version	=
							CTlvVersion( WSC_ID_VERSION, bufObj );
					// Selected Registrar
					CTlvSelRegistrar selReg =
								CTlvSelRegistrar(
											WSC_ID_SEL_REGISTRAR, bufObj );
					// Device Password ID
					CTlvDevicePwdId devPwdId =
								CTlvDevicePwdId(
										WSC_ID_DEVICE_PWD_ID, bufObj );
					// Selected Registrar Config Methods
					CTlvSelRegCfgMethods selRegCfgMethods =
								CTlvSelRegCfgMethods(
										WSC_ID_SEL_REG_CFG_METHODS, bufObj );

					// UUID-E
					// Per 1.0b spec, SetSelectedRegistrar no longer includes the UUID
					// CTlvUuid uuid = CTlvUuid( WSC_ID_UUID_E, bufObj );

					// Add or remove the Sel Reg TLV from the beacon
					// and Probe Resp depending on the value of selReg
					SetBeaconIE( true,
								selReg.Value(),
								devPwdId.Value(),
								selRegCfgMethods.Value());
					// Enable probe response IE
					SetProbeRespIE(
								WSC_MSGTYPE_AP_WLAN_MGR,
								WSC_SCSTATE_CONFIGURED,
								selReg.Value(),
								devPwdId.Value(),
								selRegCfgMethods.Value());

					// Remove from the beacon after a timeout.
					//
					mb_SSR_Called = true;		// signal flag that SSR has been called
					if (m_timerThrdId == 0) {	// start thread if not already running...
						int err = WscCreateThread(
									&m_timerThrdId,     // thread ID
									SetSelectedRegistrarTimerThread,     // thread proc
									(void *)this);      // data to pass to thread
						if (WSC_SUCCESS != err)
						{
							TUTRACE((TUTRACE_MC, "MC: Unable to start SetSelectedRegistrarTimerThread\n"));
						} else
						{
							TUTRACE((TUTRACE_MC, "MC: Started SetSelectedRegistrarTimerThread\n"));
						}
						WscSleep(1); // give thread a chance to start.
					}
				}
				break;
			}

            default:
            {
                // Not understood, do nothing
                TUTRACE((TUTRACE_MC,
                    "MC::ActualCBThreadProc: Unrecognized message\n"));
                break;
            }
        } // switch

        // Free the data
        delete (uint8 *)p_cbData;
    } // while

#ifdef __linux__
    pthread_exit(NULL);
#endif

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
CMasterControl::KillCallbackThread() const
{
    // Enqueue a CB_QUIT
    S_CB_HEADER *p = new S_CB_HEADER;
    p->eType = CB_QUIT;
    p->dataLength = 0;

    mp_cbQ->Enqueue(sizeof(S_CB_HEADER), 3, p);
    return;
} // KillCallbackThread

/*
 * Name        : SwitchModeOn
 * Description : Switch to the specified mode of operation.
 *                 Turn off other modes, if necessary.
 * Arguments   : IN EMode e_mode - mode of operation to switch to
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::SwitchModeOn( IN EMode e_mode )
{
    uint32 ret = WSC_SUCCESS;

    // Create DH Key Pair
    DH *p_dhKeyPair;
    BufferObj bo_pubKey, bo_sha256Hash;

    ret = mp_regProt->GenerateDHKeyPair( &p_dhKeyPair, bo_pubKey );
    printf("*************************Wsc SwitchMOdeOn: Mode: %d***********************\n",
                  e_mode);
    if ( WSC_SUCCESS != ret )
    {
        TUTRACE((TUTRACE_ERR,
                    "MC::SwitchModeOn: Could not generate DH Key\n"));
        return ret;
    }
    else
    {
        TUTRACE((TUTRACE_DBG, "MC:: DH Key pair generated.\r\n"));
        // DH Key generated
        // Now generate the SHA 256 hash
        mp_regProt->GenerateSHA256Hash( bo_pubKey, bo_sha256Hash );

        // Store this info in mp_info
        WscLock( mp_info->GetLock() );
        mp_info->SetDHKeyPair( p_dhKeyPair );
        if ( WSC_SUCCESS != (mp_info->SetPubKey( bo_pubKey )) )
		{
			TUTRACE((TUTRACE_ERR,
                    "MC::SwitchModeOn: Could not set pubKey\n"));
			ret = WSC_ERR_SYSTEM;
		}
        if ( WSC_SUCCESS != (mp_info->SetSHA256Hash( bo_sha256Hash )) )
		{
			TUTRACE((TUTRACE_ERR,
                    "MC::SwitchModeOn: Could not set sha256Hash\n"));
			ret = WSC_ERR_SYSTEM;
		}
        WscUnlock( mp_info->GetLock() );
		if ( WSC_ERR_SYSTEM == ret )
		{
			return ret;
		}
    }

printf("SwitchModeOn(): e_mode = %d, mp_trans= %08x\r\n", e_mode, mp_trans);
    // Now perform mode-specific startup
    switch( e_mode )
    {
        // Make sure we have the config data to work with
        // assert( mp_info->IsInfoConfigSet() );
	// This is quite different from Intel WSC Reference Implementation
	// Need carefully checking.
        case EModeUnconfAp:
        case EModeApProxyRegistrar:
        {
            TUTRACE((TUTRACE_MC,
                    "MC::SwitchModeOn: EModeApProxyRegistrar enter\n"));

            // Instantiate the Registrar SM
            mp_regSM = new CRegistrarSM( mp_regProt, mp_trans );
            if ( !mp_regSM )
            {
                TUTRACE((TUTRACE_ERR,
                    "MC::SwitchModeOn: Could not allocate mp_regSM\n"));
                ret = WSC_ERR_OUTOFMEMORY;
                break;
            }
            mp_regSM->SetMCCallback( CMasterControl::StaticCallbackProc, this );
            mp_regSM->SetLocalDeviceInfo( mp_info->GetDeviceInfo(),
                                mp_info->GetLock(), p_dhKeyPair );

            // Generate the PSK to use for configuring enrollees
	    // First check if we have one from the config file
	    if ( !(mp_info->IsNwKeySet()) )
	    {
		if ( WSC_SUCCESS != GeneratePsk() )
		{
	            ret = WSC_ERR_SYSTEM;
		    break;
		}
	    }
	    else
	    {
	        // Have a PSK to use
		// Print out the value
		uint32 len;
		char *nwKey = mp_info->GetNwKey( len );
		PrintPskValue( nwKey, len );
            }

	    // Create the encrypted settings TLV for AP
	    CreateTlvEsM8Ap( NULL );

	    // Create the encrypted settings TLV for STA
	    CreateTlvEsM8Sta();

	   // Initialize the Registrar SM
	   mp_regSM->InitializeSM( NULL,            // p_enrolleeInfo
	                           true,            // enableLocalSM
                         true );          // enablePassthru
           // Send the encrypted settings value to the SM
	   mp_regSM->SetEncryptedSettings(
	                		(void *)mp_tlvEsM8Sta,
			    		(void *)mp_tlvEsM8Ap );

            // Startup the transport modules
	    ret = mp_trans->StartMonitor( TRANSPORT_TYPE_EAP );
	    if ( ret != WSC_SUCCESS )
	    {
	         TUTRACE((TUTRACE_ERR, "MC::SwitchModeOn: "
						"Could not start trans/EAP\n"));
		 break;
	    }

	    ret = mp_trans->StartMonitor( TRANSPORT_TYPE_WLAN );
	    if ( ret != WSC_SUCCESS )
	    {
	  	TUTRACE((TUTRACE_ERR, "MC::InitiateRegistration: "
	         			"Could not start trans/WLAN\n"));
		break;
	    }

            ret = mp_trans->StartMonitor( TRANSPORT_TYPE_UPNP_DEV );
	    if ( ret != WSC_SUCCESS )
	    {
		TUTRACE((TUTRACE_ERR, "MC::SwitchModeOn: "
					"Could not start trans/UPNP_DEV\n"));
		break;
	    }
            // Jerry init Enrolle state machine
            // Instantiate the Enrollee SM
	    // Seems here is the start of Atheros added code??
	    // To function as unconfigured AP(i.e., enrollee)??
            mp_enrSM = new CEnrolleeSM( mp_regProt, mp_trans );
            if ( !mp_enrSM )
            {
                TUTRACE((TUTRACE_ERR,
                    "MC::SwitchModeOn: Could not allocate mp_enrSM\n"));
                ret = WSC_ERR_OUTOFMEMORY;
                break;
            }
            mp_enrSM->SetMCCallback( CMasterControl::StaticCallbackProc, this );
            mp_enrSM->SetLocalDeviceInfo( mp_info->GetDeviceInfo(),
                                mp_info->GetLock(), p_dhKeyPair );

            // By default, set transport to call back into Enrollee State Machine
            //mp_trans->SetSMCallback( CEnrolleeSM::StaticCallbackProc, mp_enrSM );

            // Generate a device password, 8 characters long
	    // GenerateDevPwd has been modified by chenyan, it does not generate random
	    // valid pin values as before, instead it just copy previously stored pin as cmd-line args
	    // into mp_info
	    ret = GenerateDevPwd( true );
	    if ( WSC_SUCCESS != ret )
	    {
		TUTRACE((TUTRACE_ERR, "MC::SwitchModeOn: Could not generate dev pwd\n"));
    		break;
            }

            //Added after plugfest
            if ( WSC_SUCCESS == mp_info->SetDevPwdId(WSC_DEVICEPWDID_DEFAULT))
	    {
	 	// Reset local device info in SM
	  	mp_enrSM->UpdateLocalDeviceInfo( mp_info->GetDeviceInfo() );
	    }

	    // enrollee things start.
            char	*cp_data;
            uint16  data16;

	    // Unconfigured AP, should function as enrollee in Reg Protocol.
	    // So, here we should prepare M7, which is sent by enrollee to Registrar.
	    // Encrypted settings is something critically important provided by Unconfigured AP
	    // , which is different from other enrollees(they don't need to provide Es in M7).
	    if ( mp_tlvEsM7Ap )
		delete mp_tlvEsM7Ap;

            mp_tlvEsM7Ap = new CTlvEsM7Ap();
            // SSID
            cp_data = mp_info->GetSSID( data16 );
            mp_tlvEsM7Ap->ssid.Set( WSC_ID_SSID, (uint8 *)cp_data, data16 );
            // MAC addr:  should really get this from the NIC driver
			uint8 *p_macAddr = mp_info->GetMacAddr();
            data16 = SIZE_MAC_ADDR;
            mp_tlvEsM7Ap->macAddr.Set( WSC_ID_MAC_ADDR, p_macAddr, data16 );
            // Auth type
            //sep,11,08,modify by weizhengqin
            //data16 = mp_info->GetAuthTypeFlags();
            data16 = mp_info->GetAuthType();
            mp_tlvEsM7Ap->authType.Set( WSC_ID_AUTH_TYPE, data16 );
            //mp_tlvEsM7Ap->authType.Set( WSC_ID_AUTH_TYPE, WSC_AUTHTYPE_WPAPSK);
            // Encr type
            //sep,11,08,modify by weizhengqin
            //data16 = mp_info->GetEncrTypeFlags();
            data16 = mp_info->GetEncrType();
            mp_tlvEsM7Ap->encrType.Set( WSC_ID_ENCR_TYPE, data16 );
            //mp_tlvEsM7Ap->encrType.Set( WSC_ID_ENCR_TYPE, WSC_ENCRTYPE_TKIP);
            // nwKey
            CTlvNwKey *p_tlvKey = new CTlvNwKey();
            char *cp_psk = new char[SIZE_64_BYTES+1];
            // should really initialize this psk...
            uint32 keyLen;
            strncpy(cp_psk, mp_info->GetNwKey(keyLen), keyLen);
            p_tlvKey->Set( WSC_ID_NW_KEY, cp_psk, keyLen );
            ListAddItem( mp_tlvEsM7Ap->nwKey, p_tlvKey );

	    // Call into the State Machine
	    // If devPwd == NULL,
	    // device password is auto-generated for Enrollees - use
	    // locally-stored password
	    uint32 length;
	    char *cp_devPwd;

            // Not doing PBC
	    cp_devPwd = mp_info->GetDevPwd( length );
	    if ( 0 == length )
	    {
	         ret = WSC_ERR_SYSTEM;
	         break;
	    }
            TUTRACE((TUTRACE_MC, "*** Init Enrolle Stat machine\n"));
	    // Call in to the SM
	    mp_enrSM->InitializeSM( NULL,       // p_regInfo
	                            NULL,       // M7 Enr encrSettings
                            (void *)mp_tlvEsM7Ap, // M7 AP encrSettings
                     cp_devPwd,             // device password
                        length );

            /* July 01, 2008, LiangXin.
             * Move following code from UICallback thread, to resolve
             * the problem sometimes mp_trans has NULL SM callback.
             */
             if (EModeUnconfAp != e_mode)
                            {
                    TUTRACE((TUTRACE_DBG, "CMasterControl:SwitchOn() calling "
                                "SetSMCallback(CRegistrarSM::StaticCallbackProc)\r\n"));
                                mp_trans->SetSMCallback(
                                                  CRegistrarSM::StaticCallbackProc, mp_regSM );
                                /* maybe it's too early to set Beacon and ProbeResp IE here
                                 * because AP may not started???
                                 * So move it to WscCmd main msg handle thread...
                                 */
                                 #if 0
                                SetBeaconIE(true,                    /* configured */
                                         false,                      /* selected   */
                                         WSC_DEVICEPWDID_DEFAULT,    /* DevPwdID, default:PIN */
                                         WSC_CONFMET_LABEL);         /* selRegCfgMethods */

                                SetProbeRespIE(WSC_MSGTYPE_AP_WLAN_MGR,   /* response type      */
                                 WSC_SCSTATE_CONFIGURED,     /* WSC state          */
                                 false,                      /* selected registrar */
                                 WSC_DEVICEPWDID_DEFAULT,    /* devPwdId */
                                 WSC_CONFMET_LABEL );        /* selRegCfgMethods */
                                #endif

                            } else {
                                #if 0
                                 SetBeaconIE(false,            /* configured */
                                         false,                       /* selected   */
                                         WSC_DEVICEPWDID_DEFAULT,     /* DevPwdID*/
                                         WSC_CONFMET_LABEL);          /* selRegCfgMethods */

                                 SetProbeRespIE(WSC_MSGTYPE_AP_WLAN_MGR,/* response type      */
                                 WSC_SCSTATE_UNCONFIGURED,    /* WSC state          */
                             false,                       /* selected registrar */
                             WSC_DEVICEPWDID_DEFAULT,     /* devPwdId */
                             WSC_CONFMET_LABEL);          /* selRegCfgMethods */
                                 #endif
            TUTRACE((TUTRACE_DBG, "CMasterControl:SwitchOn() calling SetSMCallback(CEnrolleeSM::StaticCallbackProc)\r\n"));
                                 mp_trans->SetSMCallback(
                                         CEnrolleeSM::StaticCallbackProc,
                                         mp_enrSM );
                            }
             /* end of added code */

            // Tell the app to start the AP
            TUTRACE((TUTRACE_MC, "MC::SwitchModeOn: Sending Starting "
                        "AP msg to main callback...\r\n"));
	    // using callback mechanism, sending msg to main callback thread.
            uint16 len;
            WscLock( mp_info->GetLock() );
            S_CB_MAIN_START_AP *p_cbMsg = new S_CB_MAIN_START_AP;
            if ( !p_cbMsg )
            {
                TUTRACE((TUTRACE_ERR,
                    "MC::SwitchModeOn: Could not allocate m_cbMsg\n"));
                ret = WSC_ERR_OUTOFMEMORY;
                break;
            }
            p_cbMsg->cbHeader.eType = CB_MAIN_START_AP;
            p_cbMsg->cbHeader.dataLength = sizeof(S_CB_MAIN_START_AP) -
                                            sizeof(S_CB_HEADER);
            strcpy( p_cbMsg->ssid, mp_info->GetSSID(len) );
            strcpy( p_cbMsg->keyMgmt, mp_info->GetKeyMgmt(len) );
			memset( p_cbMsg->nwKey, 0, SIZE_64_BYTES );
			char *nwKey = mp_info->GetNwKey( p_cbMsg->nwKeyLen );
			memcpy( p_cbMsg->nwKey, nwKey, p_cbMsg->nwKeyLen );
            p_cbMsg->b_restart = false;
            //p_cbMsg->b_restart = true;
            p_cbMsg->b_configured = true;	//Why set to configured status???

            m_mainCallbackInfo.pf_callback( (void *)p_cbMsg,
                                            m_mainCallbackInfo.p_cookie );
            WscUnlock( mp_info->GetLock() );


            break;
        }


        default:
        {
            // Unknown mode
            TUTRACE((TUTRACE_ERR, "MC::SwitchModeOn: Unknown mode\n"));
            ret = WSC_ERR_GENERIC;
            break;
        }
    } // switch
    TUTRACE((TUTRACE_MC, "MC::SwitchModeOn: Exit\n"));
    return ret;
} // SwitchModeOn

/*
 * Name        : SwitchModeOff
 * Description : Switch off the specified mode of operation.
 * Arguments   : IN EMode e_mode - mode of operation to switch off
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::SwitchModeOff( IN EMode e_mode )
{
    // TODO:
    // 1. Write out non-transient data

    switch( e_mode )
    {
        case EModeUnconfAp:
        {
            if ( mp_enrSM )
            {
                // TODO: Clear beacons and probe response IEs

                mp_enrSM->SetMCCallback( NULL, NULL );
                mp_enrSM->SetLocalDeviceInfo( NULL, NULL, NULL );
                mp_trans->SetSMCallback( NULL, NULL );

                delete mp_enrSM;
            }

            // Transport module stop now happens in ProcessRegCompleted()

            break;
        }
		case EModeApProxyRegistrar:
        {
            if ( mp_regSM )
            {
                // TODO: Clear beacons and probe response IEs

                mp_regSM->SetMCCallback( NULL, NULL );
                mp_regSM->SetLocalDeviceInfo( NULL, NULL, NULL );
                mp_trans->SetSMCallback( NULL, NULL );

                delete mp_regSM;
            }

			// Stop the transport modules
			mp_trans->StopMonitor( TRANSPORT_TYPE_EAP );
			mp_trans->StopMonitor( TRANSPORT_TYPE_UPNP_DEV );
            mp_trans->StopMonitor( TRANSPORT_TYPE_WLAN );
            break;
        }
        default:
        {
            // Unknown mode
			TUTRACE((TUTRACE_ERR, "MC::SwitchModeOff: Unknown mode\n"));
            break;
        }
    } // switch

    return WSC_SUCCESS;
} // SwitchModeOff

/*
 * Name        : SetBeaconIE
 * Description : Push Beacon WSC IE information to transport
 * Arguments   : IN bool b_configured - is the AP configured?
				 IN bool b_selRegistrar - is this flag set?
				 IN uint16 devPwdId - valid if b_selRegistrar is true
				 IN uint16 selRegCfgMethods - valid if b_selRegistrar is true
				 // IN uint8 *uuid - valid if b_selRegistrar is true
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::SetBeaconIE( IN bool b_configured,
                             IN bool b_selRegistrar,
                             IN uint16 devPwdId,
                             IN uint16 selRegCfgMethods)
{
    uint32	ret;
    BufferObj	bufObj;
    uint8	data8, *p_data8;
    uint16      data16;

    // Create the IE
    // Version
    data8 = mp_info->GetVersion();
    CTlvVersion( WSC_ID_VERSION, bufObj, &data8 );

    // Simple Config State
    if ( b_configured )
        data8 = WSC_SCSTATE_CONFIGURED;
    else
        data8 = WSC_SCSTATE_UNCONFIGURED;

    CTlvScState( WSC_ID_SC_STATE, bufObj, &data8 );
    // AP Setup Locked - optional if false.  If implemented and TRUE,
    // include this attribute.
    CTlvAPSetupLocked( WSC_ID_AP_SETUP_LOCKED, bufObj, &mb_APSetupLock );

    // Selected Registrar - optional
    // Add this TLV only if b_selRegistrar is true
    if ( b_selRegistrar )
    {
         CTlvSelRegistrar( WSC_ID_SEL_REGISTRAR, bufObj, &b_selRegistrar );
	 // Add in other related params as well
	 // Device Password ID
         CTlvDevicePwdId( WSC_ID_DEVICE_PWD_ID, bufObj, &devPwdId );
         // Selected Registrar Config Methods
         CTlvSelRegCfgMethods( WSC_ID_SEL_REG_CFG_METHODS, bufObj,
							&selRegCfgMethods );
	// Enrollee UUID removed
    }

    // UUID
    p_data8 = mp_info->GetUUID();
    data16 = WSC_ID_UUID_E;
    CTlvUuid( data16, bufObj, p_data8, 16 ); // change 16 to SIZE_UUID

    // RF Bands
    data8 = mp_info->GetRFBand();
    CTlvRfBand( WSC_ID_RF_BAND, bufObj, &data8 );

    // Send a pointer to the serialized data to Transport
    if ( WSC_SUCCESS !=
            mp_trans->SetBeaconIE( bufObj.GetBuf(), bufObj.Length() ))
    {
        TUTRACE((TUTRACE_ERR,
            "MC::SetBeaconIE: call to trans->SetBeaconIE() failed\n"));
        ret = WSC_ERR_GENERIC;
    } else {
        TUTRACE((TUTRACE_DBG,
            "MC::SetBeaconIE: call to trans->SetBeaconIE() ok\n"));
        ret = WSC_SUCCESS;
    }
    // bufObj will be destroyed when we exit this function
    return ret;
} // SetBeaconIE

/*
 * Name        : SetProbeReqIE
 * Description : Push WSC Probe Req WSC IE information to transport
 * Arguments   :
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::SetProbeReqIE( IN uint8 reqType,
                              IN uint16 assocState,
                              IN uint16 configError )
{
    uint32 ret;
    BufferObj bufObj;
    uint16 data16;
    uint8 data8, *p_data8;

    // Create the IE
    // Version
    data8 = mp_info->GetVersion();
    CTlvVersion( WSC_ID_VERSION, bufObj, &data8 );

    // Request Type
    CTlvReqType( WSC_ID_REQ_TYPE, bufObj, &reqType );

    // Config Methods
    data16 = mp_info->GetConfigMethods();
    CTlvConfigMethods( WSC_ID_CONFIG_METHODS, bufObj, &data16 );

    // UUID
    p_data8 = mp_info->GetUUID();
    if ( WSC_MSGTYPE_REGISTRAR == reqType )
        data16 = WSC_ID_UUID_R;
    else
        data16 = WSC_ID_UUID_E;
    CTlvUuid( data16, bufObj, p_data8, 16 ); // change 16 to SIZE_UUID

	// Primary Device Type
	// This is a complex TLV, so will be handled differently
	CTlvPrimDeviceType tlvPrimDeviceType;
	tlvPrimDeviceType.categoryId = mp_info->GetPrimDeviceCategory();
	tlvPrimDeviceType.oui = mp_info->GetPrimDeviceOui();
	tlvPrimDeviceType.subCategoryId = mp_info->GetPrimDeviceSubCategory();
	tlvPrimDeviceType.write( bufObj );

    // RF Bands
    data8 = mp_info->GetRFBand();
    CTlvRfBand( WSC_ID_RF_BAND, bufObj, &data8 );

    // Association State
    CTlvAssocState( WSC_ID_ASSOC_STATE, bufObj, &assocState );

    // Configuration Error
    CTlvConfigError(WSC_ID_CONFIG_ERROR, bufObj, &configError );

    // Device Password ID
    data16 = mp_info->GetDevicePwdId();
    CTlvDevicePwdId( WSC_ID_DEVICE_PWD_ID, bufObj, &data16 );
    // Portable Device - optional

    // Vendor Extension - optional

    // Send a pointer to the serialized data to Transport
    if ( WSC_SUCCESS !=
            mp_trans->SetProbeReqIE( bufObj.GetBuf(), bufObj.Length() ))
    {
        TUTRACE((TUTRACE_ERR,
            "MC::SetProbeReqIE: call to trans->SetProbeReqIE() failed\n"));
        ret = WSC_ERR_GENERIC;
    }
    else
    {
        TUTRACE((TUTRACE_MC,
            "MC::SetProbeReqIE: call to trans->SetProbeReqIE() ok\n"));
        ret = WSC_SUCCESS;
    }

    // bufObj will be destroyed when we exit this function

    return ret;
} // SetProbeReqIE

/*
 * Name        : SetProbeRespIE
 * Description : Push WSC Probe Resp WSC IE information to transport
 * Arguments   :
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::SetProbeRespIE( IN uint8 respType,
                                IN uint8 scState,
                                IN bool b_selRegistrar,
								IN uint16 devPwdId,
								IN uint16 selRegCfgMethods)
{
    uint32 ret;
    BufferObj bufObj;
    uint16 data16;
    uint8 data8, *p_data8;
    char *pc_data;

    // Create the IE
    // Version
    data8 = mp_info->GetVersion();
    CTlvVersion( WSC_ID_VERSION, bufObj, &data8 );

    // Simple Config State
    CTlvScState( WSC_ID_SC_STATE, bufObj, &scState );

    // AP Setup Locked - optional if false.  If implemented and TRUE, include this attribute.
    CTlvAPSetupLocked( WSC_ID_AP_SETUP_LOCKED, bufObj, &mb_APSetupLock );

    // Selected Registrar
    CTlvSelRegistrar( WSC_ID_SEL_REGISTRAR, bufObj, &b_selRegistrar );

    // Selected Registrar Config Methods - optional, required if b_selRegistrar
    // is true

    // Device Password ID - optional, required if b_selRegistrar is true
    // Enrollee UUID - optional, required if b_selRegistrar is true
    if ( b_selRegistrar )
    {
	// Device Password ID
 	CTlvDevicePwdId( WSC_ID_DEVICE_PWD_ID, bufObj, &devPwdId );
	// Selected Registrar Config Methods
	CTlvSelRegCfgMethods( WSC_ID_SEL_REG_CFG_METHODS, bufObj,
                &selRegCfgMethods );
	// Per 1.0b spec, removed Enrollee UUID
    }

    // Response Type
    CTlvRespType( WSC_ID_RESP_TYPE, bufObj, &respType );

    p_data8 = mp_info->GetUUID();
    if ( WSC_MSGTYPE_REGISTRAR == respType )
        data16 = WSC_ID_UUID_R;
    else
        data16 = WSC_ID_UUID_E;
    CTlvUuid( data16, bufObj, p_data8, 16 ); // change 16 to SIZE_UUID

    // Manufacturer
    pc_data = mp_info->GetManufacturer( data16 );
    CTlvManufacturer( WSC_ID_MANUFACTURER, bufObj, pc_data, data16 );

    // Model Name
    pc_data = mp_info->GetModelName( data16 );
    CTlvModelName( WSC_ID_MODEL_NAME, bufObj, pc_data, data16 );

    // Model Number
    pc_data = mp_info->GetModelNumber( data16 );
    CTlvModelNumber( WSC_ID_MODEL_NUMBER, bufObj, pc_data, data16 );

    // Serial Number
    pc_data = mp_info->GetSerialNumber( data16 );
    CTlvSerialNum( WSC_ID_SERIAL_NUM, bufObj, pc_data, data16 );

    // Primary Device Type
    // This is a complex TLV, so will be handled differently
    CTlvPrimDeviceType tlvPrimDeviceType;
    tlvPrimDeviceType.categoryId = mp_info->GetPrimDeviceCategory();
    tlvPrimDeviceType.oui = mp_info->GetPrimDeviceOui();
    tlvPrimDeviceType.subCategoryId = mp_info->GetPrimDeviceSubCategory();
    tlvPrimDeviceType.write( bufObj );

    // Device Name
    pc_data = mp_info->GetDeviceName( data16 );
    CTlvDeviceName( WSC_ID_DEVICE_NAME, bufObj, pc_data, data16 );

    // Config Methods
    data16 = mp_info->GetConfigMethods();
    CTlvConfigMethods( WSC_ID_CONFIG_METHODS, bufObj, &data16 );

    // RF Bands
    data8 = mp_info->GetRFBand();
    CTlvRfBand( WSC_ID_RF_BAND, bufObj, &data8 );

    // Vendor Extension - optional

    // Send a pointer to the serialized data to Transport
    if ( WSC_SUCCESS !=
            mp_trans->SetProbeRespIE( bufObj.GetBuf(), bufObj.Length() )) {
        TUTRACE((TUTRACE_ERR,
            	"MC::SetProbeRespIE: call to trans->SetProbeRespIE() failed\n"));
        ret = WSC_ERR_GENERIC;
    } else {
	TUTRACE((TUTRACE_DBG,
           	"MC::SetProbeRespIE: call to trans->SetProbeRespIE() ok\n"));
        ret = WSC_SUCCESS;
    }
    // bufObj will be destroyed when we exit this function
    return ret;
} // SetProbeRespIE

/*
 * Name        : ProcessBeaconIE
 * Description : De-serialize data recd from Transport. Perform required
 *                 processing
 * Arguments   : IN void *p_data - pointer to IE data
 *                 IN uint32 len - length of the data
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::ProcessBeaconIE( IN char *ssid,
								IN uint8 (&macAddr)[SIZE_MAC_ADDR],
                                IN uint8 *p_data, IN uint32 len )
{
	uint32 ret = WSC_SUCCESS;

	TUTRACE((TUTRACE_MC, "MC::ProcessBeaconIE: Going to start"
					" working on our beacon\n"));

    // De-serialize this IE to get to the data
    BufferObj bufObj( p_data , len );

	// TUTRACE((TUTRACE_MC, "MC %d\n", bufObj));

    WSC_BEACON_IE beaconIE;
	beaconIE.apSetupLocked.Set(WSC_ID_AP_SETUP_LOCKED,false);
	beaconIE.selRegistrar.Set(WSC_ID_SEL_REGISTRAR,false);
	beaconIE.pwdId.Set(WSC_ID_DEVICE_PWD_ID,0);
	beaconIE.selRegConfigMethods.Set(WSC_ID_SEL_REG_CFG_METHODS,0);

    beaconIE.version = CTlvVersion( WSC_ID_VERSION, bufObj );
    beaconIE.scState = CTlvScState( WSC_ID_SC_STATE, bufObj );
    if ( WSC_ID_AP_SETUP_LOCKED == bufObj.NextType() )
    {
        beaconIE.apSetupLocked	= CTlvAPSetupLocked(
									WSC_ID_AP_SETUP_LOCKED, bufObj );
    }
	if ( WSC_ID_SEL_REGISTRAR == bufObj.NextType() )
	{
		beaconIE.selRegistrar   = CTlvSelRegistrar( WSC_ID_SEL_REGISTRAR, bufObj );
		if (beaconIE.selRegistrar.Value() == true) {
			if ( WSC_ID_DEVICE_PWD_ID == bufObj.NextType() )
			{
				beaconIE.pwdId	= CTlvDevicePwdId( WSC_ID_DEVICE_PWD_ID, bufObj );
			}
			if ( WSC_ID_SEL_REG_CFG_METHODS == bufObj.NextType() )
			{
				beaconIE.selRegConfigMethods	= CTlvSelRegCfgMethods(
														WSC_ID_SEL_REG_CFG_METHODS,
														bufObj );
			}
		}
	}

	// TODO:  Add code for processing Selected Registrar data.

    // Process beacon
    // See if we have seen anything previously from this MAC addr
    S_NEIGHBOR_INFO *p_neighborInfo;
    LPLISTITR listItr = ListItrCreate( mp_neighborInfoList );
    if ( !listItr )
        throw WSC_ERR_OUTOFMEMORY;

    p_neighborInfo = (S_NEIGHBOR_INFO *)ListItrGetNext( listItr );
    while ( p_neighborInfo )
    {
		if ( strncmp((char *)macAddr,
				(char *)p_neighborInfo->macAddr, SIZE_MAC_ADDR))
            break; // out of while
        p_neighborInfo = (S_NEIGHBOR_INFO *)ListItrGetNext( listItr );
    }
    ListItrDelete( listItr );

    if ( p_neighborInfo )
    {
        // Found beacon in the list
        if ( p_neighborInfo->b_active )
        {
            // Nothing to be done
        }
        else if ( !(p_neighborInfo->b_sentProbeReq) )
        {
            // Send Probe Request to this MAC addr
			// TODO
            // mp_trans->SendProbeRequest( macAddr );
        }
    }
    else
    {
        // Beacon from new entity
        // Add to the list
        p_neighborInfo = new S_NEIGHBOR_INFO;
		strncpy( p_neighborInfo->ssid, ssid, SIZE_32_BYTES + 1);
        strncpy( (char *)(p_neighborInfo->macAddr),
                (char *)macAddr, SIZE_MAC_ADDR );
        p_neighborInfo->beaconVersion = beaconIE.version.Value();
        p_neighborInfo->beaconSCState = beaconIE.scState.Value();
		p_neighborInfo->APSetupLocked = beaconIE.apSetupLocked.Value();
		p_neighborInfo->selectedRegistrar = beaconIE.selRegistrar.Value();
		p_neighborInfo->devPwdId = beaconIE.pwdId.Value();
		p_neighborInfo->selRegConfigMethods = beaconIE.selRegConfigMethods.Value();

        p_neighborInfo->b_sentProbeReq = false;
        p_neighborInfo->b_recdProbeResp = false;
        p_neighborInfo->b_active = false;
        ListAddItem( mp_neighborInfoList, p_neighborInfo );

		// Uncomment for PROBE-REQ/RESP functionality
		/*
		// Tell the transport to stop sending beacons up here
		mp_trans->SendBeaconsUp( false );
		// Tell Transport we're ready to receive Probe responses now
		mp_trans->SendProbeResponsesUp( true );

        // Send a Probe Request to this SSID
		// We should technically send this out to the AP's MAC addr
        mp_trans->SendProbeRequest( ssid );
		// push down the probe request IE data
		SetProbeReqIE( WSC_MSGTYPE_REGISTRAR,
            WSC_ASSOC_NOT_ASSOCIATED, WSC_ERROR_NO_ERROR );

		p_neighborInfo->b_sentProbeReq = true;
		*/


		// Comment out this block of code when enabling
		// PROBE-REQ/RESP functionality

		// Old code, when we started the supplicant right after
		// receiving a beacon
		// What we're going to do instead is send the Probe Request and
		// wait for the Probe Response

		// <start old code>
		// Instead of sending the probe request, we're going to take a
		// shortcut here
		if ( WSC_SCSTATE_UNCONFIGURED == p_neighborInfo->beaconSCState &&
				mb_restartSupp )
		{
			// Reset the flag that brought us in
			mb_restartSupp = false;

			// Tell the transport to stop sending beacons up here
			mp_trans->SendBeaconsUp( false );

			// Provide the beacon details to the app
			uint32 structLen = sizeof(S_CB_MAIN_PUSH_MSG);
			char c_disp[100]; // make this big enough to hold the message.
			sprintf( c_disp, "AP %s will now be configured", ssid );
			uint32 dispLen = (uint32)(strlen( c_disp ));
			structLen += dispLen;

			uint8 *p_temp = new uint8[structLen];
			S_CB_MAIN_PUSH_MSG *p = (S_CB_MAIN_PUSH_MSG *)p_temp;
			p->cbHeader.eType = CB_MAIN_PUSH_MSG;
			p->cbHeader.dataLength = structLen - sizeof(S_CB_HEADER);
			memcpy( p->c_msg, c_disp, dispLen+sizeof(char) );
			m_mainCallbackInfo.pf_callback( (void *)p,
									m_mainCallbackInfo.p_cookie );

			// Restart the supplicant
			TUTRACE((TUTRACE_MC, "MC::ProcessBeaconIE: Resetting "
						"supplicant\n"));
			S_CB_MAIN_RESET_WPASUPP *p_cbMsg2 =
								new S_CB_MAIN_RESET_WPASUPP;
			if ( !p_cbMsg2 )
			{
				TUTRACE((TUTRACE_ERR, "MC::ProcessBeaconIE: "
							"Could not allocate p_cbMsg2\n"));
				ret = WSC_ERR_OUTOFMEMORY;
			}
			else
			{
				char *cp_data;
				uint16 data16;

				p_cbMsg2->cbHeader.eType = CB_MAIN_RESET_WPASUPP;
				p_cbMsg2->cbHeader.dataLength =
									sizeof(S_CB_MAIN_RESET_WPASUPP) -
									sizeof(S_CB_HEADER);
				// Fill in SSID
				uint32 ssidLen = (uint32) strlen( ssid );
				strncpy( p_cbMsg2->ssid, ssid, ssidLen );
				p_cbMsg2->ssid[ssidLen] = '\0';
				// Fill in keyMgmt
				cp_data = mp_info->GetKeyMgmt( data16 );
				strncpy( p_cbMsg2->keyMgmt, cp_data, data16 );
				p_cbMsg2->keyMgmt[data16] = '\0';
				// PSK not used here - leave blank
				memset( p_cbMsg2->nwKey, 0, SIZE_64_BYTES );
				p_cbMsg2->nwKeyLen = 0;
				// Fill in identity
				strcpy( p_cbMsg2->identity, REGISTRAR_ID_STRING );
				p_cbMsg2->b_startWsc = true;

				m_mainCallbackInfo.pf_callback(
									(void *)p_cbMsg2,
									m_mainCallbackInfo.p_cookie );
				TUTRACE((TUTRACE_MC, "MC::ProcessBeaconIE: "
							"Pushed supp restart req\n"));
			}
		}
		// <end old code>*/
    }

    return ret;
} // ProcessBeaconIE

/*
 * Name        : ProcessProbeReqIE
 * Description : De-serialize data recd from Transport. Perform required
 *                 processing
 * Arguments   : IN void *p_data - pointer to IE data
 *                 IN uint32 len - length of the data
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::ProcessProbeReqIE( IN uint8 (&macAddr)[SIZE_MAC_ADDR],
                                IN uint8 *p_data, IN uint32 len )
{
    // De-serialize this IE to get to the data
    TUTRACE((TUTRACE_MC, "Got Probe Request IE\n"));

    BufferObj bufObj( p_data, len );

    WSC_PROBE_REQUEST_IE prReqIE;

    prReqIE.version		= CTlvVersion( WSC_ID_VERSION, bufObj );
    prReqIE.reqType     = CTlvReqType( WSC_ID_REQ_TYPE, bufObj );
    prReqIE.confMethods	= CTlvConfigMethods( WSC_ID_CONFIG_METHODS, bufObj );
    if ( WSC_ID_UUID_E == bufObj.NextType() )
    {
        prReqIE.uuid    = CTlvUuid( WSC_ID_UUID_E, bufObj, SIZE_16_BYTES );
    }
    else if ( WSC_ID_UUID_R == bufObj.NextType() )
    {
        prReqIE.uuid    = CTlvUuid( WSC_ID_UUID_R, bufObj, SIZE_16_BYTES );
    }
	// Primary Device Type is a complex TLV - handle differently
	prReqIE.primDevType.parse( bufObj );
    prReqIE.rfBand		= CTlvRfBand( WSC_ID_RF_BAND, bufObj );
    prReqIE.assocState  = CTlvAssocState( WSC_ID_ASSOC_STATE, bufObj );
    prReqIE.confErr     = CTlvConfigError( WSC_ID_CONFIG_ERROR, bufObj );
    prReqIE.pwdId		= CTlvDevicePwdId( WSC_ID_DEVICE_PWD_ID, bufObj );

    // Ignore portable device and vendor extensions for now

    // Process message
    // Probe Request is received by an AP

    EMode e_mode = mp_info->GetConfiguredMode();
    switch( e_mode )
    {
		case EModeApProxyRegistrar:
        case EModeRegistrar:
        {
            // We are a wireless Registrar
            // Add the info here to the Neighbor List
            // TODO, if needed

            // Send a Probe Resp
            // Should be done automatically by the driver
            break;
        }

		case EModeApProxy:
		{
			// Send a Probe Resp
			// Should be done automatically by the driver
			break;
		}

        case EModeClient:
        {
            // We should not see a probe request in this mode
            break;
        }

        case EModeUnconfAp:
        {
            // Add the info here to the Neighbor List
            // TODO, if needed

            // Send a Probe Resp
            // Should be done automatically by the driver
            break;
        }

        default:
        {
            break;
        }
    } // switch

    return WSC_SUCCESS;
} // ProcessProbeReqIE

/*
 * Name        : ProcessProbeRespIE
 * Description : De-serialize data recd from Transport. Perform required
 *                 processing
 * Arguments   : IN void *p_data - pointer to IE data
 *                 IN uint32 len - length of the data
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::ProcessProbeRespIE( IN uint8 (&macAddr)[SIZE_MAC_ADDR],
                                   IN uint8 *p_data, IN uint32 len )
{
	uint32 ret = WSC_SUCCESS;

	TUTRACE((TUTRACE_MC, "Got Probe Response IE\n"));

    // De-serialize this IE to get to the data
    BufferObj bufObj( p_data, len );

    WSC_PROBE_RESPONSE_IE prRespIE;

    prRespIE.version	= CTlvVersion( WSC_ID_VERSION, bufObj );
    prRespIE.respType   = CTlvRespType( WSC_ID_RESP_TYPE, bufObj );
    prRespIE.uuid        = CTlvUuid( WSC_ID_UUID_E, bufObj, SIZE_16_BYTES );

	prRespIE.scState	= CTlvScState( WSC_ID_SC_STATE, bufObj );
    prRespIE.manuf		= CTlvManufacturer( WSC_ID_MANUFACTURER,
                                    bufObj, SIZE_64_BYTES );
    prRespIE.modelName  = CTlvModelName( WSC_ID_MODEL_NAME,
                                    bufObj, SIZE_32_BYTES );
    prRespIE.modelNumber	= CTlvModelNumber( WSC_ID_MODEL_NUMBER,
                                    bufObj, SIZE_32_BYTES );
    prRespIE.serialNumber   = CTlvSerialNum( WSC_ID_SERIAL_NUM,
                                    bufObj, SIZE_32_BYTES );
	// Primary Device Type is a complex TLV - handle differently
	prRespIE.primDevType.parse( bufObj );
    prRespIE.devName        = CTlvDeviceName( WSC_ID_DEVICE_NAME,
                                    bufObj, SIZE_32_BYTES );
	prRespIE.confMethods    = CTlvConfigMethods( WSC_ID_CONFIG_METHODS,
                                    bufObj );
    if ( WSC_ID_AP_SETUP_LOCKED == bufObj.NextType() )
    {
        prRespIE.apSetupLocked	= CTlvAPSetupLocked(
									WSC_ID_AP_SETUP_LOCKED, bufObj );
    }
	if ( WSC_ID_SEL_REGISTRAR == bufObj.NextType() )
	{
		prRespIE.selRegistrar   = CTlvSelRegistrar( WSC_ID_SEL_REGISTRAR, bufObj );
		if (prRespIE.selRegistrar.Value() == true) {
			if ( WSC_ID_DEVICE_PWD_ID == bufObj.NextType() )
			{
				prRespIE.pwdId	= CTlvDevicePwdId( WSC_ID_DEVICE_PWD_ID, bufObj );
			}
			if ( WSC_ID_SEL_REG_CFG_METHODS == bufObj.NextType() )
			{
				prRespIE.selRegConfigMethods	= CTlvSelRegCfgMethods(
														WSC_ID_SEL_REG_CFG_METHODS,
														bufObj );
			}
		}
	}


    // Ignore any other optional attributes for now

    // Process message
    // Registrars and Client receive Probe Responses

    EMode e_mode = mp_info->GetConfiguredMode();
    switch( e_mode )
    {
        case EModeRegistrar:
		{
			// Get the right SSID, and then re-configure the WPA-supplicant
			// Look at the neighbor list to find the node corresponding to
			// this MAC address
			S_NEIGHBOR_INFO *p_neighborInfo;
			LPLISTITR listItr = ListItrCreate( mp_neighborInfoList );
			if ( !listItr )
				throw WSC_ERR_OUTOFMEMORY;

			p_neighborInfo = (S_NEIGHBOR_INFO *)ListItrGetNext( listItr );
			while ( p_neighborInfo )
			{
				if ( strncmp((char *)macAddr,
						(char *)p_neighborInfo->macAddr, SIZE_MAC_ADDR))
					break; // out of while
				p_neighborInfo = (S_NEIGHBOR_INFO *)ListItrGetNext( listItr );
			}
			ListItrDelete( listItr );

			if ( p_neighborInfo )
			{
				TUTRACE((TUTRACE_MC, "Found the AP in the neighbor list\n"));

				// Uncomment out this block when enabling
				// PROBE-REQ/RESP functionality

#ifdef notyet
				// Tell the transport we don't need any more Probe Responses
				mp_trans->SendProbeResponsesUp( false );

				// Reconfigure the WPA supplicant to connect if this AP is
				// unconfigured

				if ( WSC_SCSTATE_UNCONFIGURED == p_neighborInfo->beaconSCState
						&&  mb_restartSupp )
				{
					// Reset the flag that brought us in
					mb_restartSupp = false;

					// Provide the AP details to the app
					uint32 structLen = sizeof(S_CB_MAIN_PUSH_MSG);
					char c_disp[128];
					sprintf( c_disp, "AP %s will now be configured",
										p_neighborInfo->ssid );
					uint32 dispLen = (uint32)(strlen( c_disp ));
					structLen += dispLen;

					uint8 *p_temp = new uint8[structLen];
					S_CB_MAIN_PUSH_MSG *p = (S_CB_MAIN_PUSH_MSG *)p_temp;
					p->cbHeader.eType = CB_MAIN_PUSH_MSG;
					p->cbHeader.dataLength = structLen - sizeof(S_CB_HEADER);
					memcpy( p->c_msg, c_disp, dispLen+sizeof(char) );
					m_mainCallbackInfo.pf_callback( (void *)p,
											m_mainCallbackInfo.p_cookie );

					// Restart the supplicant
					TUTRACE((TUTRACE_MC, "MC::ProcessProbeRespIE: "
								"Resetting supplicant\n"));
					S_CB_MAIN_RESET_WPASUPP *p_cbMsg2 =
										new S_CB_MAIN_RESET_WPASUPP;
					if ( !p_cbMsg2 )
					{
						TUTRACE((TUTRACE_ERR, "MC::ProcessProberespIE: "
									"Could not allocate p_cbMsg2\n"));
						ret = WSC_ERR_OUTOFMEMORY;
					}
					else
					{
						char *cp_data;
						uint16 data16;

						p_cbMsg2->cbHeader.eType = CB_MAIN_RESET_WPASUPP;
						p_cbMsg2->cbHeader.dataLength =
											sizeof(S_CB_MAIN_RESET_WPASUPP) -
											sizeof(S_CB_HEADER);
						// Fill in SSID
						uint32 ssidLen = (uint32)
											strlen( p_neighborInfo->ssid );
						strncpy( p_cbMsg2->ssid, p_neighborInfo->ssid,
									ssidLen );
						p_cbMsg2->ssid[ssidLen] = '\0';
						// Fill in keyMgmt
						cp_data = mp_info->GetKeyMgmt( data16 );
						strncpy( p_cbMsg2->keyMgmt, cp_data, data16 );
						p_cbMsg2->keyMgmt[data16] = '\0';
						// PSK not used here - leave blank
						memset( p_cbMsg2->nwKey, 0, SIZE_64_BYTES );
						p_cbMsg2->nwKeyLen = 0;
						// Fill in identity
						strcpy( p_cbMsg2->identity, REGISTRAR_ID_STRING );
						p_cbMsg2->b_startWsc = true;

						m_mainCallbackInfo.pf_callback(
											(void *)p_cbMsg2,
											m_mainCallbackInfo.p_cookie );
						TUTRACE((TUTRACE_MC, "MC::ProcessBeaconIE: "
									"Pushed supp restart req\n"));
					}
				}
#endif /* notyet */
			}
			break;
		}

		case EModeApProxyRegistrar:
        {
            // We should not see a probe response in this mode
            break;
        }

		case EModeApProxy:
		{
			// We should not see a probe response in this mode
			break;
		}

        case EModeClient:
        {
            // Add the info here to the Neighbor List
            // TODO

            // Prompt the user for action (join the network?)
            break;
        }

        case EModeUnconfAp:
        {
            // We should not see a probe response in this mode
            break;
        }

        default:
        {
            break;
        }
    } // switch
    return ret;
} // ProcessProbeRespIE

/*
 * Name        : ProcessRegCompleted
 * Description : Handle actions that need to be performed on completion of
 *               the registration protocol.
 * Arguments   : IN bool b_result - result of the registration process
 *               IN void *p_encrSettings - the encrSettings that were sent
 *               in M7 or received in M8, depending on the mode of operation
 *               IN S_DEVICE_INFO *p_peerInfo - information on the peer entity
 * Return type : uint32 - result of the operation
 */
uint32
CMasterControl::ProcessRegCompleted(
                            IN bool b_result, IN void *p_encrSettings,
                            IN S_DEVICE_INFO *p_peerInfo )
{
    uint32 ret = WSC_SUCCESS;

    TUTRACE((TUTRACE_MC, "MC::ProcessRegCompleted: ""start result=%d\n", b_result));
   // printf("MC::ProcessRegCompleted: ""start result=%d\n", b_result);
    if ( b_result )
    {

        // Make sure to delete p_encrSettings when we're done
        // Registration process succeeded

	#if 1
        //sep,11,08,add by weizhengqin
        if(p_encrSettings == NULL && p_peerInfo == NULL)
        {
        	return ret;
        }
	  //
	#endif
        switch( m_process_status )
        {
            case AP_CFG_STARTED:
            {
                TUTRACE((TUTRACE_MC, "MC::AP config done \n"));
                // AP (Enrollee) must have just completed initial setup
                // p_encrSettings contain CTlvEsM8Ap
                CTlvEsM8Ap *p_tlvEncr = (CTlvEsM8Ap *)p_encrSettings;

                // Restart the AP with the new settings
                TUTRACE((TUTRACE_DBG, "Prepare restarting AP with the new settings...\n"));
                S_CB_MAIN_START_AP *p_cbMsg = new S_CB_MAIN_START_AP;
                if ( !p_cbMsg )
                {
                    TUTRACE((TUTRACE_ERR, "MC::ProcessRegCompleted: "
                                "Could not allocate m_cbMsg\n"));
                    ret = WSC_ERR_OUTOFMEMORY;
                    break;
                }
                p_cbMsg->cbHeader.eType = CB_MAIN_START_AP;
                p_cbMsg->cbHeader.dataLength = sizeof(S_CB_MAIN_START_AP) -
                                                sizeof(S_CB_HEADER);
                char     *cp_data;
                uint16     data16;
                // Fill in SSID
                cp_data = (char *)(p_tlvEncr->ssid.Value());
                data16 = p_tlvEncr->ssid.Length();
                strncpy( p_cbMsg->ssid, cp_data, data16 );
                p_cbMsg->ssid[data16] = '\0';
                mp_info->SetSSID(p_cbMsg->ssid);
                TUTRACE((TUTRACE_DBG, "New settings:\r\n"));
                TUTRACE((TUTRACE_DBG, "SSID: %s\n", p_cbMsg->ssid));

                // Fill in Authentication type and encrption type
                //sep,11,08,comment by weizhengqin
                /*
                mp_info->SetAuthTypeFlags(p_tlvEncr->authType.Value());
                mp_info->SetEncrTypeFlags(p_tlvEncr->encrType.Value());
		  */
		  //sep,11,08,add by weizhengqin
		  mp_info->SetAuthType(p_tlvEncr->authType.Value());
                mp_info->SetEncrType(p_tlvEncr->encrType.Value());
		  //
                TUTRACE((TUTRACE_ERR, "MC::  authType=%d. encr=%d\n",
                         p_tlvEncr->authType.Value(),p_tlvEncr->encrType.Value() ));

                TUTRACE((TUTRACE_INFO, "AuthType:%d, EncrType:%d\n",
				p_tlvEncr->authType.Value(),p_tlvEncr->encrType.Value()));
                // Fill in KeyMgmt
                if (p_tlvEncr->authType.Value() == WSC_AUTHTYPE_ATHR_REPEAT)
                {
                      mp_info->SetPrimDeviceSubCategory(WSC_AUTHTYPE_ATHR_REPEAT);
                      mp_info->SetDeviceName("AtherosRepeater");
                      mp_info->WriteConfigFile();
                      system("reboot");
                } else if (p_tlvEncr->authType.Value() == WSC_AUTHTYPE_ATHR_ETH_CLIENT)
                {
                      mp_info->SetPrimDeviceSubCategory(WSC_AUTHTYPE_ATHR_ETH_CLIENT);
                      mp_info->SetDeviceName("AtherosEthClient");
                      mp_info->WriteConfigFile();
                      system("reboot");
                } else if(p_tlvEncr->authType.Value() == WSC_AUTHTYPE_ATHR_AP)
                {
                      mp_info->SetPrimDeviceSubCategory(1);// AP device sub category is 1
                      mp_info->SetDeviceName("AtherosAP");
                      mp_info->WriteConfigFile();
                      system("reboot");
                } else if (p_tlvEncr->authType.Value() == WSC_AUTHTYPE_OPEN)
                {
                    TUTRACE((TUTRACE_DBG, "Auth: Open\r\n"));
                    if(p_tlvEncr->encrType.Value() == WSC_ENCRTYPE_WEP)
                    {
                        strcpy( p_cbMsg->keyMgmt, "WEP" );
                        p_cbMsg->keyMgmt[3] = '\0';
                        TUTRACE((TUTRACE_DBG, "ENCRYPT: WEP\r\n"));
                    } else {
                       strcpy( p_cbMsg->keyMgmt, "OPEN" );
                        p_cbMsg->keyMgmt[4] = '\0';
                        TUTRACE((TUTRACE_DBG, "ENCRYPT:OPEN\r\n"));
                    }
                } else if(p_tlvEncr->authType.Value() == WSC_AUTHTYPE_WPA2PSK)
                {
                    strcpy( p_cbMsg->keyMgmt, "WPA2-PSK" );
                    p_cbMsg->keyMgmt[8] = '\0';
                    TUTRACE((TUTRACE_DBG, "AUTH:WPA2-PSK(OPEN)\r\n"));
                } else {
                    strcpy( p_cbMsg->keyMgmt, "WPA-PSK" );
                    p_cbMsg->keyMgmt[7] = '\0';
                    TUTRACE((TUTRACE_DBG, "AUTH:WPA-PSK(OPEN)\r\n"));
                }

                // Fill in NwKey
                if((p_tlvEncr->authType.Value() == WSC_AUTHTYPE_OPEN) &&
                   (p_tlvEncr->encrType.Value() == WSC_ENCRTYPE_NONE))
                {
                    memset( p_cbMsg->nwKey, 0, SIZE_64_BYTES );
                    mp_info->SetNwKey(p_cbMsg->nwKey,0);
                    TUTRACE((TUTRACE_DBG, "Networkey:NULL.\r\n"));
                } else {
                    CTlvNwKey *p_tlvKey;
                    p_tlvKey = (CTlvNwKey *)(ListGetFirst( p_tlvEncr->nwKey ));
                    char *nwKey = (char *)p_tlvKey->Value();
                    if(p_tlvEncr->encrType.Value() == WSC_ENCRTYPE_WEP)
                    {
                        char WepKey[65];
                        unsigned char * pTmp = (unsigned char *)p_tlvKey->Value();
                        /* July 04, 2008, Liang Xin.
                         * Modified here to pass WCN Wireless WEP Configuration
                         * & Query Test.
                         * Here is the thing: Test tools say give us 10-hex key,
                         * which is 0xaaaaaaaaaa, but finally we get here is 10
                         * ASCII "aaaaaaaaaa", so we can not transform it again
                         * , just copy it.
                         */
                        int len = 0;
                        if ((p_tlvKey->Length() == 5) ||
                            (p_tlvKey->Length() == 13) ||
                            (p_tlvKey->Length() == 16))
                        {   //only when 5, 13, 16 ASCII char
                        for (int i=0; i< p_tlvKey->Length(); i++)
                        {
                            sprintf(&WepKey[i*2], "%02x",*pTmp++);
                        }
                            len = p_tlvKey->Length() * 2;
                             WepKey[len] = 0;
                        }
                        else if ((p_tlvKey->Length() == 10) ||
                                (p_tlvKey->Length() == 26)  ||
                                (p_tlvKey->Length() == 32))
                        {
                            /* just copy them */
                            memcpy(WepKey, pTmp, p_tlvKey->Length());
                            len = p_tlvKey->Length();
                            WepKey[len] = 0;
                        }
                        else {
                            /* XXX, invalid WepKey, what we should do here */
                        }
                        TUTRACE((TUTRACE_DBG, "WepKey:%s\r\n", WepKey));

                        p_cbMsg->nwKeyLen = len;
                        memset( p_cbMsg->nwKey, 0, SIZE_64_BYTES );
                        memcpy( p_cbMsg->nwKey, WepKey , p_cbMsg->nwKeyLen );
                        mp_info->SetNwKey( WepKey, p_cbMsg->nwKeyLen );
                    } else {
	                    p_cbMsg->nwKeyLen = p_tlvKey->Length();
                        memset( p_cbMsg->nwKey, 0, SIZE_64_BYTES );
                        memcpy( p_cbMsg->nwKey, nwKey, p_cbMsg->nwKeyLen );
                        mp_info->SetNwKey( nwKey, p_cbMsg->nwKeyLen );
                        TUTRACE((TUTRACE_DBG, "KeyLen %d, Otherkey:%s\n", p_cbMsg->nwKeyLen,
                            p_cbMsg->nwKey));
                    }
                }

                // Fill in the rest of the info in the struct
                p_cbMsg->b_restart = true;
                p_cbMsg->b_configured = true;

                //switch mode
                SwitchWscMode(EModeApProxyRegistrar);

		        mp_info->WriteConfigFile();
                // Write out the config file
		        TUTRACE((TUTRACE_MC, "MC::ProcessRegCompleted: "
	                "Writing out the config file\n"));


                // Delete the locally stored encrypted settings for this
		// registration instance
		if ( mp_tlvEsM7Ap )
		{
		     delete mp_tlvEsM7Ap;
		     mp_tlvEsM7Ap = NULL;
		}

                // Delete encrypted settings that were sent from SM
                delete p_tlvEncr;
                p_tlvEncr = NULL;

                mp_tlvEsM7Ap = new CTlvEsM7Ap();
                // SSID
                cp_data = mp_info->GetSSID( data16 );
                mp_tlvEsM7Ap->ssid.Set( WSC_ID_SSID, (uint8 *)cp_data, data16 );
                // MAC addr:  should really get this from the NIC driver
		        uint8 *p_macAddr = mp_info->GetMacAddr();
                data16 = SIZE_MAC_ADDR;
                mp_tlvEsM7Ap->macAddr.Set( WSC_ID_MAC_ADDR, p_macAddr, data16 );
                // Auth type
                //sep,11,08,modify by weizhengqin
                //data16 = mp_info->GetAuthTypeFlags();
                data16 = mp_info->GetAuthType();
                mp_tlvEsM7Ap->authType.Set( WSC_ID_AUTH_TYPE, data16 );
                
                // Encr type
                //sep,11,08,modify by weizhengqin
                //data16 = mp_info->GetEncrTypeFlags() ;
                data16 = mp_info->GetEncrType() ;
                mp_tlvEsM7Ap->encrType.Set( WSC_ID_ENCR_TYPE, data16 );
                
                // nwKey
                CTlvNwKey *p_tlvKey = new CTlvNwKey();
                char *cp_psk = new char[SIZE_64_BYTES+1];
                // should really initialize this psk...
                uint32 keyLen;
                memset(cp_psk, 0, SIZE_64_BYTES + 1);
                strncpy(cp_psk, mp_info->GetNwKey(keyLen), keyLen);
                p_tlvKey->Set( WSC_ID_NW_KEY, cp_psk, keyLen);
                ListAddItem( mp_tlvEsM7Ap->nwKey, p_tlvKey );

                // Call into the State Machine
            // If devPwd == NULL,
            // device password is auto-generated for Enrollees - use
            // locally-stored password
            uint32 length;
            char *cp_devPwd;

                // Not doing PBC
	        cp_devPwd = mp_info->GetDevPwd( length );
	        if ( 0 == length )
	        {
	             ret = WSC_ERR_SYSTEM;
	             break;
	        }
                TUTRACE((TUTRACE_MC, "*** Init Enrolle Stat machine\n"));
	        // Call in to the SM
	        mp_enrSM->InitializeSM( NULL,       // p_regInfo
	                            NULL,           // M7 Enr encrSettings
                    (void *)mp_tlvEsM7Ap,       // M7 AP encrSettings
                    cp_devPwd,                  // device password
                    length );

                //sending to main callback
                TUTRACE((TUTRACE_DBG, "Sending to Main callback...\n"));
                m_mainCallbackInfo.pf_callback( (void *)p_cbMsg,
                                                m_mainCallbackInfo.p_cookie );

		break;
            }

            case AP_AUTO_CFG_STARTED:
            {
                TUTRACE((TUTRACE_MC, "MC::ProcessRegCompleted: ""AP auto config done \n"));

                // Restart the AP with the new settings
                S_CB_MAIN_START_AP *p_cbMsg = new S_CB_MAIN_START_AP;
                if ( !p_cbMsg )
                {
                    TUTRACE((TUTRACE_ERR, "MC::ProcessRegCompleted: "
                                "Could not allocate m_cbMsg\n"));
                    ret = WSC_ERR_OUTOFMEMORY;
                    break;
                }
                p_cbMsg->cbHeader.eType = CB_MAIN_START_AP;
                p_cbMsg->cbHeader.dataLength = sizeof(S_CB_MAIN_START_AP) -
                                                         sizeof(S_CB_HEADER);

                // Fill in the rest of the info in the struct
                p_cbMsg->b_restart = true;
                p_cbMsg->b_configured = false;


                m_mainCallbackInfo.pf_callback( (void *)p_cbMsg,
                                                m_mainCallbackInfo.p_cookie );
                break;
            }

            case TP_ADDED_EVENT:
                SwitchWscMode(EModeApProxyRegistrar);
                break;

            default:
            {
                TUTRACE((TUTRACE_MC, "MC::STA enroll done \n"));
                //any success will cause switch mode
                SwitchWscMode(EModeApProxyRegistrar);
                mp_info->WriteConfigFile();

                //case EModeApProxyRegistrar:
                // Check device info to determine if enrollee peer is an AP
                if ( p_peerInfo->b_ap )
                {
                    // Peer Enrollee is an AP
                    // p_encrSettings contains CTlvEsM7Ap
                    // Nothing to do with it for now
                    CTlvEsM7Ap *p_tlvEncr = (CTlvEsM7Ap *)p_encrSettings;
                    TUTRACE((TUTRACE_MC,
				"MC::ProcessRegCompleted: "
				"update CTlvEsM7Ap\n"));

                    // Extract the new SSID from mp_tlvEsM8Ap
		    char *cp_data;
		    uint16 data16;

		    CTlvCredential *p_tlvCred =
                        (CTlvCredential*)ListGetLast(mp_tlvEsM8Sta->credential);
		    // Fill in SSID
		    cp_data = (char *)(mp_tlvEsM8Ap->ssid.Value());
		    data16 = mp_tlvEsM8Ap->ssid.Length();
		    if (data16 > 32) {
			TUTRACE((TUTRACE_ERR, "MC::ProcessRegCompleted: "
					      "SSID too big, truncating\n"));
		        data16 = 32; // truncate if SSID is too big
		        cp_data[data16] = '\0';
		    }
#if 0
                    if ( e_mode == EModeRegistrar && p_tlvCred)
                    {
                        // update credential with new SSID
                        p_tlvCred->ssid.Set( WSC_ID_SSID, (uint8 *)cp_data,
                                            data16 );
                    }
#endif

		    if ( mp_info->IsRegWireless() )
		    {
			// Need to tell the app to restart the supp
			TUTRACE((TUTRACE_MC,
				"MC::ProcessRegCompleted: "
				"Resetting Supplicant (Client)\n"));
			S_CB_MAIN_RESET_WPASUPP *p_cbMsg =
						new S_CB_MAIN_RESET_WPASUPP;
			if ( !p_cbMsg )
			{
				TUTRACE((TUTRACE_ERR,
				"MC::ProcessRegCompleted: Could not allocate "
							"p_cbMsg2\n"));
				ret = WSC_ERR_OUTOFMEMORY;
				break;
			}
			p_cbMsg->cbHeader.eType = CB_MAIN_RESET_WPASUPP;
			p_cbMsg->cbHeader.dataLength =
				    sizeof(S_CB_MAIN_RESET_WPASUPP) -
				    sizeof(S_CB_HEADER);
			// Use our variable mp_tlvEsM8Ap to obtain
			// config data for resetting the supplicant
			strncpy( p_cbMsg->ssid, cp_data, data16 );
			p_cbMsg->ssid[data16] = '\0';
			// Fill in keyMgmt
			// TODO: Determine correct key management value
			// from the encr settings TLV
			//strncpy( p_cbMsg->keyMgmt, "WPA-PSK", 7 );
			//p_cbMsg->keyMgmt[7] = '\0';
			// Fill in PSK
			// Can either get this from CTlvNwKey in mp_tlvEsM8Ap
			// or from mp_info
			/*
			CTlvNwKey *p_tlvKey;
			p_tlvKey = (CTlvNwKey *)
			(ListGetFirst( mp_tlvEsM8Ap->nwKey ));
			cp_data = (char *)(p_tlvKey->Value());
			data16 = p_tlvKey->Length();
			*/
			char *nwKey =
					mp_info->GetNwKey( p_cbMsg->nwKeyLen );
			memcpy( p_cbMsg->nwKey, nwKey, SIZE_64_BYTES );
	                // Print this value out as well
			PrintPskValue( nwKey, p_cbMsg->nwKeyLen );

			p_cbMsg->identity[0] = '\0'; // Identity not used here - leave blank
			p_cbMsg->b_startWsc = false;

			m_mainCallbackInfo.pf_callback(
							(void *)p_cbMsg,
							m_mainCallbackInfo.p_cookie );
			TUTRACE((TUTRACE_MC, "MC::InitiateRegistration: "
						"Pushed supp restart req\n"));
	             }

		     // Delete the locally stored encrypted settings for this
	             // registration instance
		     // Don't do this deletion now, since we're storing this
		     // for future registrations
		     /*
		     delete mp_tlvEsM8Ap;
		     mp_tlvEsM8Ap = NULL;
		     */
                     // Delete encrypted settings that were sent from SM
                     delete p_tlvEncr;
        	     // Write out the config file
		     TUTRACE((TUTRACE_MC, "MC::ProcessRegCompleted: "
				"Writing out the config file\n"));
		     mp_info->WriteConfigFile();
                }
                else
                {
                    // Peer Enrollee is a station (client)
                    // p_encrSettings contains CTlvEsM7Enr
                    // Nothing to do with it for now
                    CTlvEsM7Enr *p_tlvEncr = (CTlvEsM7Enr *)p_encrSettings;

                    // Come in here when we have finished
	            // configuring the client

                    // Nothing needs to be done
                    // The client should restart the supplicant and connect
                    // AP will be configured already

	            // Delete the locally stored encrypted settings for this
		    // registration instance
		    // Don't do this deletion now, since we're storing this
		    // for future registrations
		    /*
		    if ( mp_tlvEsM8Sta )
		    {
			delete mp_tlvEsM8Sta;
			mp_tlvEsM8Sta = NULL;
		    }
		    */

                    // Delete encrypted settings that were sent from SM
                    delete p_tlvEncr;
                }
                break;
            }

        } // switch
    }
    else
    {
         // Registration process failed
         // TODO

         char *cp_data;
         uint16 data16;

         // Delete the locally stored encrypted settings for this
	 // registration instance
	 if ( mp_tlvEsM7Ap )
	 {
	     delete mp_tlvEsM7Ap;
	     mp_tlvEsM7Ap = NULL;
	 }

         mp_tlvEsM7Ap = new CTlvEsM7Ap();
         // SSID
         cp_data = mp_info->GetSSID( data16 );
         mp_tlvEsM7Ap->ssid.Set( WSC_ID_SSID, (uint8 *)cp_data, data16 );
         // MAC addr:  should really get this from the NIC driver
         uint8 *p_macAddr = mp_info->GetMacAddr();
         data16 = SIZE_MAC_ADDR;
         mp_tlvEsM7Ap->macAddr.Set( WSC_ID_MAC_ADDR, p_macAddr, data16 );
         // Auth type
         //sep,11,08,modify by weizhengqin
        //data16 = mp_info->GetAuthTypeFlags();
        data16 = mp_info->GetAuthType();
         mp_tlvEsM7Ap->authType.Set( WSC_ID_AUTH_TYPE, data16 );
         
         // Encr type
         //sep,11,08,modify by weizhengqin
         //data16 = mp_info->GetEncrTypeFlags() ;
         data16 = mp_info->GetEncrType() ;
         mp_tlvEsM7Ap->encrType.Set( WSC_ID_ENCR_TYPE, data16 );
         
         // nwKey
         CTlvNwKey *p_tlvKey = new CTlvNwKey();
         char *cp_psk = new char[SIZE_64_BYTES+1];
         // should really initialize this psk...
         uint32 keyLen;
         strncpy(cp_psk, mp_info->GetNwKey(keyLen), keyLen);
         p_tlvKey->Set( WSC_ID_NW_KEY, cp_psk, keyLen);
         ListAddItem( mp_tlvEsM7Ap->nwKey, p_tlvKey );

         // Call into the State Machine
	 // If devPwd == NULL,
	 // device password is auto-generated for Enrollees - use
	 // locally-stored password
	 uint32 length;
	 char *cp_devPwd;

         // Not doing PBC
	 cp_devPwd = mp_info->GetDevPwd( length );
	 if ( 0 == length )
	 {
              TUTRACE((TUTRACE_ERR, "GetDevPwd error\n"));
	      ret = WSC_ERR_SYSTEM;
	      return ret;
         }
	 // Call in to the SM
	 mp_enrSM->InitializeSM( NULL,			// p_regInfo
	                         NULL,			// M7 Enr encrSettings
	                         (void *)mp_tlvEsM7Ap, 	// M7 AP encrSettings
			         cp_devPwd,	        	// device password
			         length );
    }
#if 0 // Jerry: do not stop the transport
    // Shutdown the transport modules if mode==UnconfAp or Client
    if ( EModeUnconfAp == e_mode )
    {
        // Stop the transport modules
		mp_trans->StopMonitor( TRANSPORT_TYPE_EAP );
		if ( mp_info->UseUpnp() )
		{
			mp_trans->StopMonitor( TRANSPORT_TYPE_UPNP_DEV );
		}
		// We may have started the WLAN transport
		// Stop it here - if not started, this will return a harmless error
		mp_trans->StopMonitor( TRANSPORT_TYPE_WLAN );
    }
#endif
    return ret;
} // ProcessRegCompleted

// 20071116, chenyan. no need here, input pin is usable
extern char g_pin[16];

uint32
CMasterControl::GenerateDevPwd( IN bool b_display )
{
	BufferObj bo_devPwd;
	char c_devPwd[32];
	uint8 *devPwd = NULL;

	// Use the GeneratePSK() method in RegProtocol to help with this
    bo_devPwd.Reset();
    if ( WSC_SUCCESS != mp_regProt->GeneratePSK( 8, bo_devPwd ))
	{
		TUTRACE((TUTRACE_ERR,
            "MC::GenerateDevPwd: Could not generate enrDevPwd\n"));
		return WSC_ERR_SYSTEM;
	}
#if 0
    devPwd = bo_devPwd.GetBuf();
    sprintf( c_devPwd, "%08u", *(uint32 *)devPwd );
#else

// 20071116, chenyan. no need here, input pin is usable
#if 0
    sprintf( c_devPwd, "%08u", 12345670 );
#else
    // this is risky, must keep in mind that g_pin is NULL-terminated.
    strcpy(c_devPwd, g_pin);
#endif

#endif

// 20071116, chenyan. no need here, input pin is usable
#if 0
    // Compute the checksum
	c_devPwd[7] = '\0';
	uint32 val = strtoul( c_devPwd, NULL, 10 );
	uint32 checksum = ComputeChecksum( val );
	val = val*10 + checksum;
	sprintf( c_devPwd, "%d", val );
	c_devPwd[8] = '\0';
#endif
	// Set this in our local store
	if ( WSC_SUCCESS != (mp_info->SetDevPwd( c_devPwd )) )
	{
		TUTRACE((TUTRACE_ERR,
            "MC::GenerateDevPwd: Could not set enrDevPwd\n"));
        return WSC_ERR_SYSTEM;
	}

	TUTRACE((TUTRACE_MC, "Generated devPwd: %s\n", c_devPwd));

	if ( b_display )
	{
		// Display this pwd
		uint32 structLen = sizeof(S_CB_MAIN_PUSH_MSG);
		char c_disp[32] = "DEVICE PIN: ";
		strcat( c_disp, c_devPwd );
		uint32 dispLen = (uint32)(strlen( c_disp ));
		structLen += dispLen;

		uint8 *p_temp = new uint8[structLen];
		S_CB_MAIN_PUSH_MSG *p = (S_CB_MAIN_PUSH_MSG *)p_temp;
		p->cbHeader.eType = CB_MAIN_PUSH_MSG;
		p->cbHeader.dataLength = structLen - sizeof(S_CB_HEADER);
		memcpy( p->c_msg, c_disp, dispLen+sizeof(char) );
		m_mainCallbackInfo.pf_callback( (void *)p,
								m_mainCallbackInfo.p_cookie );
	}
	return WSC_SUCCESS;
} // GenerateDevPwd

uint32
CMasterControl::GeneratePsk()
{
	BufferObj	bo_psk;
	uint32		ret;

	bo_psk.Reset();
	mp_regProt->GeneratePSK( SIZE_256_BITS, bo_psk );

	// Set this in our local store
	char nwKey[SIZE_64_BYTES+1];
	unsigned char tmp[SIZE_32_BYTES];
	memcpy( tmp, bo_psk.GetBuf(), SIZE_32_BYTES );
	int j = 0;
	for ( int i=0; i<32; i++ )
	{
		sprintf( &(nwKey[j]), "%02X", tmp[i] );
		j+=2;
	}
	ret = mp_info->SetNwKey( nwKey, 64 );
	if ( WSC_SUCCESS != ret )
	{
		TUTRACE((TUTRACE_ERR, "MC::GeneratePsk: "
					"Could not be locally stored\n"));
	}
	else
	{
		// Print out the value
		uint32 len;
		char *p_nwKey = mp_info->GetNwKey( len );
		PrintPskValue( p_nwKey, len );
	}

	return ret;
} // GeneratePsk

uint32
CMasterControl::ComputeChecksum( IN unsigned long int PIN )
{
    unsigned long int accum = 0;

	PIN *= 10;
	accum += 3 * ((PIN / 10000000) % 10);
	accum += 1 * ((PIN / 1000000) % 10);
	accum += 3 * ((PIN / 100000) % 10);
	accum += 1 * ((PIN / 10000) % 10);
	accum += 3 * ((PIN / 1000) % 10);
	accum += 1 * ((PIN / 100) % 10);
	accum += 3 * ((PIN / 10) % 10);

	int digit = (accum % 10);
	return (10 - digit) % 10;
} // ComputeChecksum

uint32
CMasterControl::CreateTlvEsM8Ap( IN char *cp_ssid )
{
	char   *cp_data;
	uint16 data16;

	// Create the Encrypted Settings TLV for AP config
	// Also store this info locally so it can be used
	// when registration completes for reconfiguration.

	// We will also need to delete this blob eventually, as the
	// SM will not delete it.
	if ( mp_tlvEsM8Ap )
	{
		delete mp_tlvEsM8Ap;
	}
        mp_tlvEsM8Ap = new CTlvEsM8Ap();

	// Fill in items
	// ssid
	if ( cp_ssid )
	{
		// Use this instead of the one from the config file
		cp_data = cp_ssid;
		data16 = (uint16)strlen(cp_ssid);
	}
	else
	{
		// Use the SSID from the config file
		cp_data = mp_info->GetSSID( data16 );
	}
        mp_tlvEsM8Ap->ssid.Set( WSC_ID_SSID, (uint8 *)cp_data,
                        data16 );

	// authType
	//sep,11,08,modify by weizhengqin
        //data16 = mp_info->GetAuthTypeFlags();
        data16 = mp_info->GetAuthType();
        mp_tlvEsM8Ap->authType.Set( WSC_ID_AUTH_TYPE, data16 );
        
        // encrType
        //sep,11,08,modify by weizhengqin
        //data16 = mp_info->GetEncrTypeFlags();
        data16 = mp_info->GetEncrType();
        mp_tlvEsM8Ap->encrType.Set( WSC_ID_ENCR_TYPE, data16 );
        
	// nwKey
        CTlvNwKey *p_tlvKey = new CTlvNwKey();
        uint32 nwKeyLen = 0;
        char *p_nwKey = mp_info->GetNwKey( nwKeyLen );
	/*
        if((mp_info->GetAuthTypeFlags()==WSC_AUTHTYPE_OPEN) &&
            (mp_info->GetEncrTypeFlags() == WSC_ENCRTYPE_NONE))
      */
      if((mp_info->GetAuthType()==WSC_AUTHTYPE_OPEN) &&
            (mp_info->GetEncrType() == WSC_ENCRTYPE_NONE))
        {
            p_tlvKey->Set( WSC_ID_NW_KEY, p_nwKey, 0 );
        } else {
            p_tlvKey->Set( WSC_ID_NW_KEY, p_nwKey, nwKeyLen );
        }
        ListAddItem( mp_tlvEsM8Ap->nwKey, p_tlvKey );
	// macAddr
	uint8 *p_macAddr = mp_info->GetMacAddr();
        data16 = SIZE_MAC_ADDR;
        mp_tlvEsM8Ap->macAddr.Set( WSC_ID_MAC_ADDR, p_macAddr, data16 );

	return WSC_SUCCESS;
} // CreateTlvEsM8Ap

uint32
CMasterControl::CreateTlvEsM8Sta()
{
    char    *cp_data;
    uint16    data16;

    // Create the Encrypted Settings TLV for STA config

    // We will also need to delete this blob eventually, as the
    // SM will not delete it.
    if ( mp_tlvEsM8Sta )
    {
	delete mp_tlvEsM8Sta;
    }
    mp_tlvEsM8Sta = new CTlvEsM8Sta();

    // credential
    CTlvCredential *p_tlvCred = new CTlvCredential();
    // Fill in credential items
    // nwIndex
    p_tlvCred->nwIndex.Set( WSC_ID_NW_INDEX, 1 );
    // ssid
    cp_data = mp_info->GetSSID( data16 );
    p_tlvCred->ssid.Set( WSC_ID_SSID, (uint8 *)cp_data,
                        data16 );
    //Auth type
    //sep,11,08,modify by weizhengqin
   // data16 = mp_info->GetAuthTypeFlags();
   data16 = mp_info->GetAuthType();
    p_tlvCred->authType.Set( WSC_ID_AUTH_TYPE, data16 );
    
    // Encr Type
    //sep,11,08,modify by weizhengqin
    //data16 = mp_info->GetEncrTypeFlags();
    data16 = mp_info->GetEncrType();
    p_tlvCred->encrType.Set( WSC_ID_ENCR_TYPE, data16 );
   
    // nwKeyIndex
    p_tlvCred->nwKeyIndex.Set( WSC_ID_NW_KEY_INDEX, 1 );
    // nwKey
    uint32 nwKeyLen = 0;
    char *p_nwKey = mp_info->GetNwKey( nwKeyLen );
    p_tlvCred->nwKey.Set( WSC_ID_NW_KEY, p_nwKey, nwKeyLen );
    // macAddr
    uint8 *p_macAddr = mp_info->GetMacAddr();
    data16 = SIZE_MAC_ADDR;
    p_tlvCred->macAddr.Set( WSC_ID_MAC_ADDR,
                        (uint8 *)p_macAddr, data16 );
    ListAddItem( mp_tlvEsM8Sta->credential, p_tlvCred );

    // New pwd
    // TODO

    // PwdId
    // TODO

    return WSC_SUCCESS;
} // CreateTlvEsM8Sta()

void
CMasterControl::PrintPskValue( IN char *nwKey, uint32 nwKeyLen )
{
#ifdef _TUDEBUGTRACE
	char line[100];
	sprintf( line, "***** WPA_PSK = " );
	strncat( line, nwKey, nwKeyLen );
	/*
	char val[4];
	for ( int i=0; i<=31; i++ )
	{
		sprintf( val, "%02x", psk[i] );
		strcat( line, val );
	}
	*/
	TUTRACE((TUTRACE_MC, "%s\n", line));
#endif // _TUDEBUGTRACE
} // PrintPskValue
uint32
CMasterControl::SendSetSelRegistrar( IN bool b_setSelReg )
{
	uint32 ret = WSC_SUCCESS;

	// Create the TLVs that need to be sent
	BufferObj	bufObj;
    uint8		data8;
	uint16		data16;
	// uint8		enrNonce[SIZE_128_BITS];

	// Version
	data8 = mp_info->GetVersion();
	CTlvVersion( WSC_ID_VERSION, bufObj, &data8 );
	// Selected Registrar
	CTlvSelRegistrar( WSC_ID_SEL_REGISTRAR, bufObj,
						&b_setSelReg );
	if (b_setSelReg) { // only include this data if flag is true
		// Device Password ID
		data16 = mp_info->GetDevicePwdId();
		CTlvDevicePwdId( WSC_ID_DEVICE_PWD_ID, bufObj, &data16 );
		// Selected Registrar Config Methods
		data16 = mp_info->GetConfigMethods();
		CTlvSelRegCfgMethods( WSC_ID_SEL_REG_CFG_METHODS, bufObj,
								&data16 );
	}
	// No longer include Enrollee Nonce or UUID-E

	// Send this to the transport
	ret = mp_trans->SendSetSelectedRegistrar(
				(char *)bufObj.GetBuf(), bufObj.Length() );

        if ( WSC_SUCCESS != ret )
	{
		TUTRACE((TUTRACE_ERR, "MC::SendSetSelRegistrar: "
				"trans->SendSetSelReg failed\n"));
	}
	else
	{
		TUTRACE((TUTRACE_MC, "MC::SendSetSelRegistrar: "
				"trans->SendSetSelReg ok\n"));
	}
	return ret;
} // SendSetSelRegistrar()
bool CMasterControl::IsPinEntered()
{
	return mb_pinEntered;
}
void *
CMasterControl::SetSelectedRegistrarTimerThread(IN void *p_data)
{
    TUTRACE((TUTRACE_MC, "MC:SetSelectedRegistrarTimerThread\n"));
    CMasterControl *mc = (CMasterControl *)p_data;

	while (mc->mb_SSR_Called) {
		mc->mb_SSR_Called = false; // turn off this flag

#ifndef __linux__
    Sleep(SSR_WALK_TIME*1000);
#else
    sleep(SSR_WALK_TIME);
#endif
	}

    S_CB_COMMON *p_NotifyBuf = new S_CB_COMMON;
    p_NotifyBuf->cbHeader.eType = CB_SSR_TIMEOUT;
    p_NotifyBuf->cbHeader.dataLength = 0;

    TUTRACE((TUTRACE_MC, "ENRSM: Timing out SetSelectedRegistrar Data\n"));

	mc->m_timerThrdId = 0; // Indicate that timer thread is no longer running.
    mc->StaticCallbackProc(p_NotifyBuf, p_data);

    return NULL;
}

void
CMasterControl::setProcessStatus(PROCESS_STATUS_TYPE processStatus)
{
    m_process_status = processStatus;
    TUTRACE((TUTRACE_MC, "ENRSM:setProcessStatus:%d \n",processStatus));
}
/*
 * Name        : SwitchWscMode
 * Description :
 * Arguments   :
 * Return type : void
 */
void
CMasterControl::SwitchWscMode( EMode mode)
{
    mp_info->SetConfiguredMode(mode);
    TUTRACE((TUTRACE_MC, "MasterControl:*******switch mode to %d.\n",mode));
}

PROCESS_STATUS_TYPE
CMasterControl::getProcessStatus(void)
{
    return m_process_status;
}
void
CMasterControl::setAPSetupLock(bool APSetupLock)
{
    mb_APSetupLock = APSetupLock;
}

