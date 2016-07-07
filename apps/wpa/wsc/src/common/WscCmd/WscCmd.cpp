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
//  File Name: WscCmd.cpp
//  Description: Implements a basic command-line UI for the WSC stack.
//      Instantiates the Master Control module, and then provides a basic
//      interface between the user and the Master Control.
//
****************************************************************************/

#ifdef WIN32
#include <windows.h>
#endif // WIN32

#include <stdio.h>
#include <memory>        // for auto_ptr
#include <unistd.h>

//OpenSSL includes
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "WscHeaders.h"
#include "WscCommon.h"
#include "StateMachineInfo.h"
#include "WscError.h"
#include "Portability.h"
#include "WscQueue.h"

#ifdef __linux__
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif // ___linux__

#include "Transport.h"
#include "StateMachine.h"
#include "RegProtocol.h"
#include "tutrace.h"
#include "slist.h"
#include "Info.h"
#include "MasterControl.h"
#include "WscCmd.h"
#include "UdpLib.h"
#include "led.h"
#include "InbEap.h"

#define WSC_VERSION_STRING	"Build 1.0.1, May 17 2006"

#define WSC_INBSTATUS_PORT 			26525
#define WSC_INBSTATUS_ADDR 			"127.0.0.1"
#define WSC_INBSTATUS_RECV_BUFSIZE 	64

static uint32 CopyOneHostapdConfFile(char * srcFileName, char * destFileName);

// 20071123, by chenyan. wsc ui q

#include <sys/types.h>
#include <sys/msg.h>
#include <unistd.h>

// These things are shared between WscCmd and httpd, so must keep
// them synchronized.
#define WPS_MSGQ_PATH               "/sbin/wsccmd"
#define WPS_MSGQ_TYPE_STATUS        100
#define WPS_MSGQ_TYPE_ACTION        101
key_t  g_wscUIQKey = 0;
typedef struct
{
    char action[36];
    char ssid[36];
    char authType;
    char encrType;
   /* July 01, 2008, LiangXin, changed keysize to 100 */
    char key[100];
  //  uint16 keyLen;  // mainly for WPA-PSK 64...
} WPS_MSG;

typedef struct
{
	int  type;
	char data[1000];
} WPS_MSG_BUF;

void connectUIQKey()
{
    int gflags, rflags;
    key_t key;
    int msgid;
    int reval;
    WPS_MSG_BUF msgRcvBuf;

	// Create a message Q
	key = ftok(WPS_MSGQ_PATH, 'a');
	msgid = msgget(key, IPC_CREAT|00666);
	if(msgid == -1)
	{
        printf("connectUIQKey: msg create error\n");
        return;
    }

    g_wscUIQKey = msgid;
    printf("connectUIQKey: connect to UI msg Q\n");
}

void sendUIQMsg(int type, char *p, int len)
{
	int sflags;
	int reval;
    WPS_MSG_BUF msgSndBuf;

    sflags = IPC_NOWAIT;
    msgSndBuf.type = type;
    memset(msgSndBuf.data, 0, 1000);
    memcpy(msgSndBuf.data, p, len);
    reval = msgsnd(g_wscUIQKey, &msgSndBuf, len, sflags);
    if(reval == -1)
    {
        printf("sendUIQMsg: message send error\n");
    }
        printf("sendUIQMsg: message send OK\n");
}

// sent actions and wireless configurations to httpd.
void setWscAction(const char *p)
{
    WPS_MSG msg;
    uint16 ssidLen;
    uint32 keyLen;
    char *key;
    int i = 0;

    //initialize msg
    memset((void *)&msg, 0, sizeof(msg));
    strcpy(msg.action, p);
    strcpy(msg.ssid, gp_mc->mp_info->GetSSID(ssidLen));
	//sep,11,08,modify by weizhengqin
    //msg.authType = gp_mc->mp_info->GetAuthTypeFlags();
    //msg.encrType = gp_mc->mp_info->GetEncrTypeFlags();
    msg.authType = gp_mc->mp_info->GetAuthType();
    msg.encrType = gp_mc->mp_info->GetEncrType();
	
    key = gp_mc->mp_info->GetNwKey(keyLen);
    TUTRACE((TUTRACE_ERR, "setWscAction, keyLen = %d.\r\n", keyLen));
    memcpy(msg.key, key, keyLen);
    msg.key[keyLen] = 0;


    /* Here we do special handling for Windows Vista configuration with
     * 64 Hex PSK.
     * The most weird thing is if the PSK is 1234567890...1234(64 in total),
     * what we get here is 0x01,0x02,0x03,...,0x01,0x02,0x03,0x04, but not
     * 0x12, 0x34, 0x56,...0x12,0x34, or ASCII coded string. So here we need
     * to transform them to ASCII string.
     */
    if (keyLen == 64)
    {
        for (i = 0; i < keyLen; i++)
        {
            #if 0
            fprintf(stderr, "%02x", msg.key[i]);
            if (i & 1)
            {
                fprintf(stderr, " ");
            }
            if ((i % 10) == 0)
            {
                fprintf(stderr, "\r\n");
            }
            #endif
            if ((msg.key[i] < 10) && (msg.key[i] >= 0))
            {
                msg.key[i] += '0';
            }
            else if ((msg.key[i] <= 0xf) && (msg.key[i] >= 0xa))
            {
                msg.key[i] += 'a';
            }
        }
        #if 0
        for (i = 0; i < keyLen; i++)
        {
            fprintf(stderr, "%c", msg.key[i]);
        }
        #endif
    }

    sendUIQMsg(WPS_MSGQ_TYPE_ACTION, (char*) &msg, sizeof(msg));
}


// 20071119, by chenyan. wsc status thread
void setWscStatus(char *p)
{
    sendUIQMsg(WPS_MSGQ_TYPE_STATUS, p, strlen(p)+1);
}

// 20071116, by chenyan. get default pin
void getInputDevPwd(char* para)
{
    if (NULL == para)
    {
        printf("error: please input pin\n");
        exit(1);
    }

    if (8 != strlen(para))
    {
        printf("error: wrong pin length\n");
        exit(1);
    }

    strcpy(g_pin, para);

    return;
}

// 20071122, by chenyan. if we start hostapd
bool g_enableHostapd = false;
void getHostapdEnabled(char* para)
{
	int a;
    if (NULL == para)
    {
        printf("error: please input hostapd status\n");
		exit(1);
    }

    a = atoi(para);
	if (a == 1)
	{
		printf("enable hostapd\n");
		g_enableHostapd = true;
	}
	else
	{
		printf("disable hostapd\n");
		g_enableHostapd = false;
	}

    return;
}
/*
 * Name        : main
 * Description : Main entry point for the WSC stack
 * Arguments   : int argc, char *argv[] - command line parameters
 * Return type : int
 */
int
main(int argc, char* argv[])
{
    uint32 i_ret = 0;

    if(argc < 2)
    {
       printf("error: parameters missing!\nusage: wsccmd -B  configpath   radionum pin\n");
       exit(1);
    }

    if(strncmp(argv[1],"-B",2)==0)
    {
         //daemon(1, 1);
    }

    CInfo::SetWscConfigPath(argv[2]);

    CInfo::SetAPNumRadio(atoi(argv[3]));

    // 20071116, by chenyan. get default pin
    getInputDevPwd(argv[4]);
    // getHostapdEnabled(argv[5]);
    connectUIQKey();

#ifdef DEBUGPRINT
    printf("\n**************************************************************\n");
    printf("Wi-Fi Simple Config Application - TP-LINK Technologies CO.,LTD\n");
    //printf("Version: %s\n", WSC_VERSION_STRING);
    printf("**************************************************************\n");
#endif
    i_ret = Init();
    if ( WSC_SUCCESS == i_ret )
    {
        TUTRACE((TUTRACE_DBG, "WscCmd::Init ok, starting stack...\n"));
        i_ret = gp_mc->StartStack();
        if ( WSC_SUCCESS == i_ret )
        {
            TUTRACE((TUTRACE_DBG, "WscCmd:: Stack started ok\n"));
            // Wait until the UI thread dies
            // Note: this thread will die when the user
            // selects the "quit" option
#ifdef __linux__
            WscDestroyThread( g_uiThreadHandle );
#else
            // Call WaitForSingleObject rather than WscDestroyThread as
            // the latter only waits for 2000ms
            WaitForSingleObject( (HANDLE)g_uiThreadHandle, INFINITE );
#endif
            // Now stop the stack
            gp_mc->StopStack();
        } // StartStack ok
        else
        {
#ifdef DEBUGPRINT
            printf( "Unable to start the stack\n" );
            printf( "Hit enter to quit application\n" );
#endif
            char inp[2];
            fgets( inp, 2, stdin );
            fflush( stdin );
            KillUIThread();
            WscDestroyThread( g_uiThreadHandle );
        }

        // Stop wpa_supplicant/hostapd if running
        if ( gb_apRunning )
        {
            APRestartNetwork();
        }

        DeInit();
    }
    else
    {
#ifdef DEBUGPRINT
        TUTRACE((TUTRACE_ERR, "WscCmd::Init failed\n"));
        printf( "Hit enter to quit application\n" );
#endif
        char inp[2];
        fgets( inp, 2, stdin );
        fflush( stdin );
    }
    return 0;
} // main

/*
 * Name        : Init
 * Description : Initialize member variables
 * Arguments   : none
 * Return type : uint32 - result of the initialize operation
 */
uint32
Init()
{
    int i_ret;

    // Basic initialization
    gp_mc				= NULL;
    gb_apRunning        = false;
    gb_regDone          = false;
	gb_useUsbKey		= false;
	gb_useUpnp			= false;
    gp_cbQ              = NULL;
    gp_uiQ              = NULL;
    g_cbThreadHandle    = 0;
    g_uiThreadHandle    = 0;

    try
    {
        if ( (event_sock = udp_open()) < 0 )
            throw "WscCmd::Init: event_sock open failed";
	/*
	 * bind wsc_event to UDP port 38100
	 */
        if ( udp_bind(event_sock, WSC_EVENT_PORT) < 0 )
           throw "WscCmd::Init: UDP Bind failed.";

        // create callback queue
        gp_cbQ = new CWscQueue();
        if ( !gp_cbQ )
            throw "WscCmd::Init: cbQ not created";
        gp_cbQ->Init();

        // create callback thread
        i_ret = WscCreateThread(
                        &g_cbThreadHandle,
                        ActualCBThreadProc,
                        NULL );
        if ( WSC_SUCCESS != i_ret )
            throw "WscCmd::Init: cbThread not created";
        WscSleep( 1 );

        // create UI queue
        gp_uiQ = new CWscQueue();
        if ( !gp_uiQ )
            throw "WscCmd::Init: uiQ not created";
        gp_uiQ->Init();

        // create UI thread
        i_ret = WscCreateThread(
                        &g_uiThreadHandle,
                        ActualUIThreadProc,
                        NULL );
        if ( WSC_SUCCESS != i_ret )
            throw "WscCmd::Init: uiThread not created";
        WscSleep( 1 );

#if 1 //defined(BOARD_AP61) || defined(BOARD_AP71)
        // create LED thread
        i_ret = WscCreateThread(
                        &g_ledThreadHandle,
                        led_loop,
                        NULL );
        if ( WSC_SUCCESS != i_ret )
            throw "WscCmd::Init: ledThread not created";
        WscSleep( 1 );
#if 0
        i_ret = WscCreateThread(
                        &g_statusThreadHandle,
                        WscStatusThread,
                        NULL );
        if ( WSC_SUCCESS != i_ret )
            throw "WscCmd::Init: statusThread not created";
        WscSleep( 1 );
#endif
#endif
        // initialize MasterControl
        gp_mc = new CMasterControl();
        if ( gp_mc )
        {
            TUTRACE((TUTRACE_DBG, "WscCmd::MC instantiated ok\n"));
            if ( WSC_SUCCESS == gp_mc->Init( CallbackProc, NULL ) )
                TUTRACE((TUTRACE_DBG, "WscCmd::MC intialized ok\n"));
            else
                throw "WscCmd::Init: MC initialization failed\n";
        }
        else
        {
            throw "WscCmd::Init: MC instantiation failed\n";
        }

        // Everything's initialized ok
        // Transfer control to member variables
    }
    catch ( char *err )
    {
        TUTRACE((TUTRACE_ERR, "WscCmd::Init failed\n %s\n", err));
	if ( event_sock )
	{
	    udp_close(event_sock);
	}

        if ( gp_mc )
        {
            gp_mc->DeInit();
            delete gp_mc;
        }
        if ( g_uiThreadHandle )
        {
            KillUIThread();
            WscDestroyThread( g_uiThreadHandle );
        }
        if ( gp_uiQ )
        {
            gp_uiQ->DeInit();
            delete gp_uiQ;
        }
        if ( g_cbThreadHandle )
        {
            KillCallbackThread();
            WscDestroyThread( g_cbThreadHandle );
        }
        if ( gp_cbQ )
        {
            gp_cbQ->DeInit();
            delete gp_cbQ;
        }

        return WSC_ERR_SYSTEM;
    }

    return WSC_SUCCESS;
} // Init

/*
 * Name        : DeInit
 * Description : Deinitialize member variables
 * Arguments   : none
 * Return type : uint32
 */
uint32
DeInit()
{
    if ( gp_mc ) {
        gp_mc->DeInit();
    }
    TUTRACE((TUTRACE_DBG, "WscCmd::MC deinitialized\n"));

    // kill the callback thread
    // gp_cbQ deleted in the callback thread
    KillCallbackThread();
    WscDestroyThread( g_cbThreadHandle );

    return WSC_SUCCESS;
} // DeInit

/*
 * Name        : WaitAPReboot
 * Description :
 * Arguments   :
 * Return type : void
 */
void WaitAPReboot()
{
    // gp_mc->mp_trans->ClearAPStartedFlag();

    while (true != gp_mc->mp_trans->CheckAPStartedFlag())
    {
        WscSleep(2);
    }

    gb_apRunning = true;
}


/*
 * Name        : ActualCBThreadProc
 * Description : This is the thread procedure for the callback thread.
 *                 Monitor the callbackQueue, and process all callbacks that
 *                 are received.
 * Arguments   : void *p_data = NULL
 * Return type : void *
 */
void *
ActualCBThreadProc( IN void *p_data )
{
    bool    b_done = false;
    uint32  h_status;
    void    *p_cbData;
    S_CB_HEADER *p_header;

    TUTRACE((TUTRACE_DBG, "WscCmd:ActualCBThreadProc: Started\n"));
    // keep doing this until the thread is killed
    while ( !b_done )
    {
        WscSleep(0);
        // block on the callbackQueue
        h_status = gp_cbQ->Dequeue(
                            NULL,        // size of dequeued msg
                            1,            // sequence number
                            0,            // infinite timeout
                            (void **) &p_cbData);
                                        // pointer to the dequeued msg

        if ( WSC_SUCCESS != h_status )
        {
            // something went wrong
            TUTRACE((TUTRACE_ERR, "WscCmd:ActualCBThreadProc: "
                        "Error in Dequeue!\n"));
            b_done = true;
            break; // from while loop
        }

        p_header = (S_CB_HEADER *)p_cbData;

        TUTRACE((TUTRACE_INFO, "WscCmd:ActualCBThreadProc: Msg type %d.\n",
                p_header->eType));
        // once we get something, parse it,
        // do whats necessary, and then block
        switch( p_header->eType )
        {
            case CB_QUIT:
            {
                // no params
                // destroy the queue
                if ( gp_cbQ )
                {
                    gp_cbQ->DeInit();
                    delete gp_cbQ;
                }
                // kill the thread
                b_done = true;
                break;
            }

            case CB_MAIN_PUSH_MSG:
            {
                TUTRACE((TUTRACE_DBG,
                    "WscCmd:ActualCBThreadProc: CB_MAIN_PUSH_MSG recd\n"));

  		// Display the message
		S_CB_MAIN_PUSH_MSG *p = (S_CB_MAIN_PUSH_MSG *)p_cbData;
                break;
            }

            case CB_MAIN_PUSH_REG_RESULT:
	    {
		TUTRACE((TUTRACE_DBG,
                    "WscCmd:ActualCBThreadProc: "
				"CB_MAIN_PUSH_REG_RESULT recd\n"));
		S_CB_MAIN_PUSH_REG_RESULT *p =
				(S_CB_MAIN_PUSH_REG_RESULT *)p_cbData;

                // Check whether the SM returned TRUE or FALSE from
                // the registration process
		if ( p->b_result )
		{
            		system("exec echo 0 >/proc/simple_config/tricolor_led");
                    led_display(LED_SUCCESS);
					setWscStatus("WSC_SUCCESS");
	            TUTRACE((TUTRACE_PROTO, "WSC registration protocol successfully completed\n"));
		}
		else
		{
                    led_display(LED_ERROR);
					setWscStatus("WSC_ERROR");
		    TUTRACE((TUTRACE_PROTO, "WSC registration protocol failed\n"));
		}

                gb_regDone = true;

                // Post a message to the UI thread
                // Display menu options for Registrar
                S_LCB_MENU_AP_PROXY_REGISTRAR *p_menu =
                                   new S_LCB_MENU_AP_PROXY_REGISTRAR;
                p_menu->eType = LCB_MENU_AP_PROXY_REGISTRAR;
                p_menu->dataLength = 0;
                gp_uiQ->Enqueue(sizeof(S_LCB_MENU_AP_PROXY_REGISTRAR),
                                2, p_menu );

                break;
            }

            case CB_MAIN_START_AP:
            {
                // NOTE: AP code is intended to be run only on Linux
                gp_mc->mp_trans->ClearHostapdCtrlPort();
                gp_mc->mp_trans->ClearAPStartedFlag();

                S_CB_MAIN_START_AP *p = (S_CB_MAIN_START_AP *)p_cbData;
                if (p->b_restart)
                {
                    TUTRACE((TUTRACE_INFO, "set WscAction Restart AP...\r\n"));
                    setWscAction("WSC_ACTION_RESTART_AP");
                }
                else
                {
                    TUTRACE((TUTRACE_INFO, "set WscAction Start AP...\r\n"));
                    setWscAction("WSC_ACTION_START_AP");
                }

                WaitAPReboot();
                sleep(2);
                gb_apRunning = true;

                TUTRACE((TUTRACE_DBG, "WscCmd: ******AP started\n"));
#if 0
//#ifdef __linux__
// 20071122, by chenyan. if we start hostapd
                uint32 ret;
                TUTRACE((TUTRACE_DBG,
                    "WscCmd:ActualCBThreadProc: CB_MAIN_START_AP recd\n"));
                S_CB_MAIN_START_AP *p = (S_CB_MAIN_START_AP *)p_cbData;

                if ( gb_apRunning && !p->b_restart )
                {
                    TUTRACE((TUTRACE_ERR,
                        "WscCmd:ActualCBThreadProc: STARTAP: "
                        "AP already running\n"));
                    break;
                }

                if ( p->b_configured )
                {
	            if(strncmp(p->keyMgmt, "OPEN", strlen("OPEN"))==0)
		    {
			APCopyOpenConfFile();
			ret = APAddParams( p->ssid, NULL, NULL, NULL);
                        gp_mc->mp_info->ClearNwKey();
                    } else if(strncmp(p->keyMgmt, "WEP", strlen("WEP"))==0)
		    {
			APCopyWepConfFile();
			ret = APAddParams( p->ssid, NULL, p->nwKey, p->nwKeyLen);
                    } else {
 		        // Re-write the config file
                        if ( WSC_SUCCESS != APCopyConfFile() )
                        {
                            TUTRACE((TUTRACE_DBG,
				     "WscCmd:ActualCBThreadProc: STARTAP: "
                                     "Cannot copy file\n"));
                             break;
                        }
                        // we do not separate WPA and WPA2 on hostapd, we always support both
                        ret = APAddParams( p->ssid, "WPA-PSK",
                                       p->nwKey, p->nwKeyLen );
		    }
                    // sync wsc_config.txt.
                    gp_mc->mp_info->SetSSID( p->ssid);
                    gp_mc->mp_info->SetKeyMgmt(p->keyMgmt); // use real keyMgmt for wsc_config.txt
                    gp_mc->mp_info->WriteConfigFile();
                }  else if (gp_mc->getProcessStatus() == AP_AUTO_CFG_STARTED) {
                    // use wpa-psk for AP auto config
                    gp_mc->mp_info->SetKeyMgmt( "WPA-PSK" );
                    //switch mode
                    gp_mc->SwitchWscMode(EModeApProxyRegistrar);

                    // Write out the config file
                    TUTRACE((TUTRACE_MC, "MC::ProcessRegCompleted: "
                                  "Writing out the config file\n"));
                    gp_mc->mp_info->WriteConfigFile();

                    // update hostapd config file
                    APCopyConfFile();
                    uint32 len;
                    uint16 data16;
                    char *p_nwKey = gp_mc->mp_info->GetNwKey( len );
                    APAddParams(  gp_mc->mp_info->GetSSID( data16 ), "WPA-PSK" ,p_nwKey , len );
                }

                if ( p->b_restart )
                {
                    APRestartNetwork();
                }
                // Now start hostapd
                gp_mc->mp_trans->ClearHostapdCtrlPort();
                gp_mc->mp_trans->ClearAPStartedFlag();

		char tmpBuf[100];
		strcpy(tmpBuf, "/sbin/hostapd -B ");
                strcat(tmpBuf, CInfo::GetWscConfigPath());
                strcat(tmpBuf, AP_CONF_FILENAME);
		system(tmpBuf);

                // if AP has dual-bands and both are enabled for WSC, start ATH1
    		if( (CInfo::GetAPNumRadio() == 2) &&
                	(gp_mc->mp_info->GetRFBand() ==
                              (WSC_RFBAND_24GHZ|WSC_RFBAND_50GHZ)))
                {
                    strcpy(tmpBuf, "/sbin/hostapd -B ");
                    strcat(tmpBuf, CInfo::GetWscConfigPath());
                    strcat(tmpBuf, AP_CONF_FILENAME_ATH1);
		    system(tmpBuf);
                    TUTRACE((TUTRACE_INFO,
                                 "WscCmd: ****** Second AP started\n"));
                }

                WaitAPReboot();
                gb_apRunning = true;

                TUTRACE((TUTRACE_DBG, "WscCmd: ******AP started\n"));
#else
                TUTRACE((TUTRACE_ERR, "WscCmd: AP code not implemented "
                            "for WinXP\n"));
#endif
                break;
            }

            case CB_MAIN_STOP_AP:
            {
                // NOTE: AP code is intended to be run only on Linux
                setWscAction("WSC_ACTION_STOP_AP");
#if  0
// #ifdef __linux__
                if ( gb_apRunning )
                {
                    APRestartNetwork();
                }
#else
                TUTRACE((TUTRACE_ERR, "WscCmd: AP code not implemented "
                            "for WinXP\n"));
#endif
                break;
            }

            case CB_MAIN_PUSH_MODE:
            {
                TUTRACE((TUTRACE_DBG,
                    "WscCmd:ActualCBThreadProc: CB_MAIN_PUSH_MODE recd\n"));

                S_CB_MAIN_PUSH_MODE *p = (S_CB_MAIN_PUSH_MODE *)p_cbData;

                // Save the UsbKey and Upnp flags
                gb_useUsbKey = p->b_useUsbKey;
                gb_useUpnp = p->b_useUpnp;

                // Process the mode
                switch( p->e_mode )
                {
                    case EModeUnconfAp:
                    {
                        TUTRACE((TUTRACE_DBG, "\n******* MODE: Unconf Access Point *******\n"));


                        // Display menu options for AP
                        S_LCB_MENU_UNCONF_AP *p = new S_LCB_MENU_UNCONF_AP;
                        p->eType = LCB_MENU_UNCONF_AP;
                        p->dataLength = 0;
                        gp_uiQ->Enqueue( sizeof(S_LCB_MENU_UNCONF_AP), 2, p );
                        break;
                    }


                    case EModeApProxyRegistrar:
                    {
                        TUTRACE((TUTRACE_DBG, "\n******* MODE: AP with built-in "
                                "Registrar and UPnP Proxy *******\n"));

                        // Display menu options for AP+Proxy+Registrar
                        S_LCB_MENU_AP_PROXY_REGISTRAR *p =
                                    new S_LCB_MENU_AP_PROXY_REGISTRAR;
                        p->eType = LCB_MENU_AP_PROXY_REGISTRAR;
                        p->dataLength = 0;
                        gp_uiQ->Enqueue( sizeof(S_LCB_MENU_AP_PROXY_REGISTRAR),
                                                            2, p );
                        break;
                    }

                    default:
                    {
                        TUTRACE((TUTRACE_ERR, "******* MODE: Unknown [Error!] *******\n"));
                        break;
                    }
                } // switch
                break;
            }

            default:
                // not understood, do nothing
                break;
        } // switch

        // free the data
        delete (uint8 *)p_cbData;
    } // while

    TUTRACE((TUTRACE_DBG, "WscCmd:ActualCBThreadProc: Exiting.\n"));
#ifdef __linux__
    pthread_exit(NULL);
#endif

    return NULL;
}

/*
 * Name        : KillCallbackThread
 * Description : Attempt to terminate the callback thread. Enqueue a
 *                 CB_QUIT in the callbackQueue
 * Arguments   : none
 * Return type : void
 */
void
KillCallbackThread()
{
    // enqueue a CB_QUIT
    S_CB_HEADER *p = new S_CB_HEADER;
    p->eType = CB_QUIT;
    p->dataLength = 0;

    gp_cbQ->Enqueue( sizeof(S_CB_HEADER), 1, p );
    return;
} // KillCallbackThread

void wsc_enable_default_cfg()
{
    uint16 data16;
    char tmpBuf[65];

    TUTRACE((TUTRACE_DBG, "WSC::Enable wsc default config.\n"));

    // Send notice early
    TUTRACE((TUTRACE_DBG, "GetSSID=%s\n, DEF=%s\n, len=%d\n",
		gp_mc->mp_info->GetSSID( data16 ),
                DEFAULT_WSC_SSID,
		strlen(DEFAULT_WSC_SSID) ));

    if(strncmp(gp_mc->mp_info->GetSSID( data16 ), DEFAULT_WSC_SSID,
                        strlen(DEFAULT_WSC_SSID))==0)
    {

        if(WSC_SUCCESS !=gp_mc->GeneratePsk())
        {
            TUTRACE((TUTRACE_ERR, "WSC::Can not generate key\n"));
        }


        uint8 *p_mac = gp_mc->mp_info->GetMacAddr();
        sprintf( (char *)tmpBuf, "Athr%02x%02x%02x%02x",
                                      p_mac[2],p_mac[3],p_mac[4],p_mac[5]);
        tmpBuf[13] = 0;
        gp_mc->mp_info->SetSSID((char *)tmpBuf);

        gp_mc->mp_info->SetKeyMgmt( "WPA-PSK" );
        gp_mc->mp_info->SetAuthTypeFlags(WSC_AUTHTYPE_WPA2PSK);
        gp_mc->mp_info->SetEncrTypeFlags(WSC_ENCRTYPE_AES);
	//sep,11,08,add by weizhengqin
	  gp_mc->mp_info->SetAuthType(WSC_AUTHTYPE_WPA2PSK);
        gp_mc->mp_info->SetEncrType(WSC_ENCRTYPE_AES);
	//end
    }
}

static void rebootAP(void)
{
    TUTRACE((TUTRACE_DBG, "WSC::AP is down\n"));
    APRestartNetwork();

    // Now start hostapd
    // Do a fork() to run hostapd
    gp_mc->mp_trans->ClearAPStartedFlag();

    pid_t childPid;
    childPid = fork();
    if ( childPid >= 0 )
    {
         // fork succeeded
         if ( 0 == childPid )
         {
              char tmpBuf[100];
	      strcpy(tmpBuf, "/sbin/hostapd -B ");
              strcat(tmpBuf, CInfo::GetWscConfigPath());
              strcat(tmpBuf, AP_CONF_FILENAME);
              system(tmpBuf);
              // system("exec ./hostapd ./hostapd.conf");
              exit( 0 );
         }
        else
         {
              TUTRACE((TUTRACE_DBG,
                            "WscCmd:wsc_enable_default_cfg STARTAP: "
                            "After fork\n"));
         }
     } else {
         // fork failed
         TUTRACE((TUTRACE_ERR,
                        "WscCmd:wsc_enable_default_cfg: STARTAP: "
                        "Could not start hostap\n"));
     }

     WaitAPReboot();
     TUTRACE((TUTRACE_DBG, "WSC::AP is up by default settings\n"));
}

int send_wsc_event_ack(int from)
{
    struct sockaddr_in to;
    char tmpBuffer[EVENT_BUF_SIZE];
    unsigned int sentBytes = 0;

    bzero(&to,sizeof(to));

    to.sin_addr.s_addr = inet_addr(WSC_EVENT_ADDR);
    to.sin_family = AF_INET;
    to.sin_port = htons(from);

    strncpy(tmpBuffer, WSC_EVENT_ACK_STRING, sizeof(WSC_EVENT_ACK_STRING) );

    sentBytes = udp_write(event_sock, tmpBuffer, sizeof(WSC_EVENT_ACK_STRING), &to);

    if (sentBytes < sizeof(WSC_EVENT_ACK_STRING))
    {
        TUTRACE((TUTRACE_ERR, "UDP WSC_EVNT_ACK send failed; sentBytes = %d\n", sentBytes));
        return WSC_ERR_SYSTEM;
    }

    return WSC_SUCCESS;
}

/*
 * return 0: caller shall continue, 1: caller shall break
 */
int handle_wsc_cfg_cmd(char *buf, char *pOption, char *pVal)
{
	char cfg_cmd[16];
	char val[65];

	memset(cfg_cmd,0,16);
	memset(val,0,65);
	// input from wsc_cfg command
	sscanf(buf, "%s %s", cfg_cmd, val);

	TUTRACE((TUTRACE_PROTO, "wsc_cfg cmd = %s, val = %s\n", cfg_cmd, val));
	if (!strcasecmp(cfg_cmd, "pin")) {
		if ( val) {
			if(gp_mc->ValidateChecksum( atoi(val))==0)
			{
				printf("Invalid pin entered, can't pass checksum!\n");
				memset(buf,0,EVENT_BUF_SIZE);
				return 0;
			}
			else
			{
				strncpy(pVal, val, 8);
				*(pVal +8) = 0;
			}
		}
		if ( pOption )
			*pOption = '1';
		return 1;
	} else if (!strcasecmp(cfg_cmd, "band")) {
		gp_mc->mp_info->SetRFBand( atoi(val) );
		gp_mc->mp_info->WriteConfigFile();
	} else if (!strcasecmp(cfg_cmd, "ssid")) {
		gp_mc->mp_info->SetSSID( val);
		gp_mc->mp_info->SetConfiguredMode( EModeApProxyRegistrar);
		gp_mc->mp_info->WriteConfigFile();

	} else if (!strcasecmp(cfg_cmd, "passphrase")) {
		if(val)
		{
			gp_mc->mp_info->SetNwKey( val,strlen(val) );
			gp_mc->mp_info->SetConfiguredMode( EModeApProxyRegistrar);
			gp_mc->mp_info->WriteConfigFile();
		}
	} else if (!strcasecmp(cfg_cmd, "authentication")) {
		if(val)
		{
			if(!strcasecmp(val,"open")) {
				gp_mc->mp_info->SetKeyMgmt( "OPEN" );
				//sep,11,08,modify by weizhengqin
				//gp_mc->mp_info->SetAuthTypeFlags(WSC_AUTHTYPE_OPEN);
				gp_mc->mp_info->SetAuthType(WSC_AUTHTYPE_OPEN);
			}else if(!strcasecmp(val,"shared")) {
	//			gp_mc->mp_info->SetKeyMgmt( "SHARED" );
	//			gp_mc->mp_info->SetAuthTypeFlags(WSC_AUTHTYPE_SHARED);
			}else if(!strcasecmp(val,"wpa")) {
	//			gp_mc->mp_info->SetKeyMgmt( "WPA" );
	//			gp_mc->mp_info->SetAuthTypeFlags(WSC_AUTHTYPE_WPA);
			}else if(!strcasecmp(val,"wpa-psk")) {
				gp_mc->mp_info->SetKeyMgmt( "WPA-PSK" );
				//sep,11,08,modify by weizhengqin
				//gp_mc->mp_info->SetAuthTypeFlags(WSC_AUTHTYPE_WPAPSK);
				gp_mc->mp_info->SetAuthType(WSC_AUTHTYPE_WPAPSK);
			}else if(!strcasecmp(val,"wpa2")) {
	//			gp_mc->mp_info->SetKeyMgmt( "WPA2" );
	//			gp_mc->mp_info->SetAuthTypeFlags(WSC_AUTHTYPE_WPA2);
			}else if(!strcasecmp(val,"wpa2-psk")) {
	//			gp_mc->mp_info->SetKeyMgmt( "WPA2-PSK" );
	//			gp_mc->mp_info->SetAuthTypeFlags(WSC_AUTHTYPE_WPA2PSK);
			}else {
//				printf("Warning: unsupported authentication type. Supported type are: open,shared,wpa,wpa-psk,wpa2,wpa2-psk\n");
				printf("Warning: unsupported authentication type. Supported type are: open,wpa-psk\n");
			}
			gp_mc->mp_info->SetConfiguredMode( EModeApProxyRegistrar);
			gp_mc->mp_info->WriteConfigFile();
		}
	} else if (!strcasecmp(cfg_cmd, "cipher")) {
		if(val)
		{
			if(!strcasecmp(val,"none")) {
				//sep,11,08,modify by weizhengqin
				//gp_mc->mp_info->SetEncrTypeFlags(WSC_ENCRTYPE_NONE);
				gp_mc->mp_info->SetEncrType(WSC_ENCRTYPE_NONE);
			}else if(!strcasecmp(val,"wep")) {
				//sep,11,08,modify by weizhengqin
				//gp_mc->mp_info->SetEncrTypeFlags(WSC_ENCRTYPE_WEP);
				gp_mc->mp_info->SetEncrType(WSC_ENCRTYPE_WEP);
			}else if(!strcasecmp(val,"tkip")) {
				//sep,11,08,modify by weizhengqin
				//gp_mc->mp_info->SetEncrTypeFlags(WSC_ENCRTYPE_TKIP);
				gp_mc->mp_info->SetEncrType(WSC_ENCRTYPE_TKIP);
			}else if(!strcasecmp(val,"aes")) {
				//sep,11,08,modify by weizhengqin
				//gp_mc->mp_info->SetEncrTypeFlags(WSC_ENCRTYPE_AES);
				gp_mc->mp_info->SetEncrType(WSC_ENCRTYPE_AES);
			}else {
				printf("Warning: unsupported encryption type. Supported type are:NONE,WEP,TKIP,AES\n");
			}
			gp_mc->mp_info->SetConfiguredMode( EModeApProxyRegistrar);
			gp_mc->mp_info->WriteConfigFile();
		}
	} else if (!strcasecmp(cfg_cmd, "selreg")) {
		gp_mc->SetBeaconIE( true, /* configured */
				    true, /* selected   */
				    WSC_DEVICEPWDID_DEFAULT,  /* DevPwdID*/
				    WSC_CONFMET_LABEL);       /* selRegCfgMethods */
				    gp_mc->mp_trans->EnableWalkTimer(true);
				    TUTRACE((TUTRACE_PROTO, "AP is selected\n"));
	} else if (!strcasecmp(cfg_cmd, "dbg")) {
        	PrintTraceLevelSet(atoi(val));
	} else if (!strcasecmp(cfg_cmd, "get")) {
		if(!strcasecmp(val,"ssid")){
			uint16 len16;
			printf("ssid: %s\n",gp_mc->mp_info->GetSSID( len16));
		}
		else if(!strcasecmp(val,"passphrase")){
			uint32 len;
			printf("passphrase: %s\n",gp_mc->mp_info->GetNwKey(len));
		}
	} else {
		TUTRACE((TUTRACE_ERR, "invalid cmd: %s", buf));
	}
	memset(buf,0,EVENT_BUF_SIZE);
	return 0;
}

static int get_one_wsc_event(char *buf, int *from_port)
{
	struct timeval tv;
	int ret = 0;
	fd_set rfds;

	tv.tv_sec = 3;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(event_sock, &rfds);
	if ( (ret = select(event_sock+1, &rfds, NULL, NULL, &tv)) <= 0) {
		return ret;
	}

	if (FD_ISSET(event_sock, &rfds)) {
		struct sockaddr_in from;
		if ( (ret = udp_read(event_sock, buf, EVENT_BUF_SIZE, &from)) < 0 )
			return ret;
		*from_port = ntohs(from.sin_port);
	} else {
		ret = 0;
	}
	return ret;
}

bool check_AP_setup_lock()
{
    static int AP_setup_count = 0;
    bool   AP_setup_lock;

    AP_setup_count ++;
    if (AP_setup_count > 20)
    {
        AP_setup_lock = true;
    } else {
        AP_setup_lock = false;
    }

    //update AP Setuo lock status
    gp_mc->setAPSetupLock(AP_setup_lock);
    //update WSC IE
    if (EModeUnconfAp == gp_mc->mp_info->GetConfiguredMode())
    {
        gp_mc->SetBeaconIE(false,                         /* configured */
                             false,                       /* selected   */
                             WSC_DEVICEPWDID_DEFAULT,     /* DevPwdID*/
                             WSC_CONFMET_LABEL);          /* selRegCfgMethods */

        gp_mc->SetProbeRespIE(WSC_MSGTYPE_AP_WLAN_MGR,    /* response type      */
		             WSC_SCSTATE_UNCONFIGURED,	  /* WSC state          */
			     false,                       /* selected registrar */
			     WSC_DEVICEPWDID_DEFAULT,     /* devPwdId */
			     WSC_CONFMET_LABEL);          /* selRegCfgMethods */
     } else {
         gp_mc->SetBeaconIE(true,                         /* configured */
                            false,                        /* selected   */
                            WSC_DEVICEPWDID_DEFAULT,      /* DevPwdID, fefault:PIN */
                            WSC_CONFMET_LABEL);           /* selRegCfgMethods */

         gp_mc->SetProbeRespIE(WSC_MSGTYPE_AP_WLAN_MGR,   /* response type      */
		            WSC_SCSTATE_CONFIGURED,       /* WSC state          */
			    false,                        /* selected registrar */
			    WSC_DEVICEPWDID_DEFAULT,      /* devPwdId */
			    WSC_CONFMET_LABEL );          /* selRegCfgMethods */
     }
    return AP_setup_lock;
}

static int wait_wsc_event(char *pOption, char *pDevPwd)
{
    char buf[EVENT_BUF_SIZE] = {0};
    for (;;) {

        //TUTRACE((TUTRACE_PROTO, "Waiting for event\n"));

	if ( get_one_wsc_event(buf, &m_wsc_event_port) <= 0 ) {
		continue;
        }

	if (strncmp(buf, "PUSH_BUTTON_NOOVERLAP",
            strlen("PUSH_BUTTON_NOOVERLAP")) == 0){
		*pOption = '2';
		led_display(LED_INPROGRESS);
		TUTRACE((TUTRACE_PROTO, "LED PUSH_BUTTON_NOOVERLAP\n"));
		break;
	} else if (strncmp(buf, "PUSH_BUTTON_OVERLAP",
			strlen("PUSH_BUTTON_OVERLAP")) == 0){
		send_wsc_event_ack(m_wsc_event_port);
		TUTRACE((TUTRACE_PROTO, "PUSH_BUTTON_OVERLAP...\n"));
		led_display(LED_OVERLAP);
        setWscStatus("OVERLAP");
        continue;
        } else if ((strncmp(buf, "STA_TIMEOUT:",
            strlen("STA_TIMEOUT:")) == 0)) {
        TUTRACE((TUTRACE_PROTO, "STA_TIMEOUT\n"));
        send_wsc_event_ack(m_wsc_event_port);
        continue;
    } else if ((strncmp(buf, "WSC_PEER_ID_REG",
        strlen("WSC_PEER_ID_REG")) == 0)) {
        TUTRACE((TUTRACE_PROTO, "Get event WSC_PEER_ID_REG\n"));
                if(true==check_AP_setup_lock())
                {
                     TUTRACE((TUTRACE_PROTO, "AP Setup Locked\n"));
                     send_wsc_event_ack(m_wsc_event_port);
	             continue;
                }
                else
                {
                     *pOption = '3';
                     //External regsitrar cfg flag. Set it asap,
                     TUTRACE((TUTRACE_INFO, "Adding external registrar, so "
                                "preparing change to AP(enrollee) mode.\r\n"));


                     gp_mc->mp_trans->SetSMCallback(
                                           CEnrolleeSM::StaticCallbackProc,
                                           gp_mc->mp_enrSM );
                     TUTRACE((TUTRACE_PROTO,
                                   "WscCmd:state machine switch to Enrolle.\n"));
                     /* July 1, 2008, LiangXin, for WCN Logo Test */
                     //Set ENRSM AP Configured state to configured
                     gp_mc->mp_enrSM->SetAPConfiguredStat(SM_AP_CONFIGURED);

                     //Allow AP Configuration.
                     gp_mc->setProcessStatus(AP_CFG_STARTED);
                     //Why we use TP_ADDED_EVENT???
                     //gp_mc->setProcessStatus(TP_ADDED_EVENT);
                }
		break;
	} else if ((strncmp(buf, "WSC_UPNP_M2",
			strlen("WSC_UPNP_M2")) == 0)) {
		TUTRACE((TUTRACE_PROTO, "WSC_UPNP_M2\n"));
                gp_mc->setProcessStatus(AP_CFG_STARTED);
		*pOption = '4';
		break;
	} else if ((strncmp(buf, "WSC_PEER_ID_STA",
		strlen("WSC_PEER_ID_STA")) == 0)) {
		send_wsc_event_ack(m_wsc_event_port);
		TUTRACE((TUTRACE_PROTO, "WSC_PEER_ID_STA\n"));
		// Set the transport callback for proxy functionality
		gp_mc->mp_trans->SetSMCallback( CRegistrarSM::StaticCallbackProc,
						gp_mc->mp_regSM );
		continue;
	} else {
		/*
		 * should be user configuration inputs
		 * When user inputs PIN, the state machine shall run.
		 * When user inputs device or security configs, just update
		 * the config file.
		 */
		if ( handle_wsc_cfg_cmd(buf, pOption, pDevPwd ) != 0 )
			break;
		else
			continue;
	}
    } // for
    return 0;
}

static int check_wsc_event_within_2minutes_cycle(void)
{
    char buf[EVENT_BUF_SIZE] = {0};
    char macAddr[SIZE_MAC_ADDR];

    int rtn;

    if ( (rtn = get_one_wsc_event(buf, &m_wsc_event_port)) <= 0 ) {
	return rtn;
    }

    if (strncmp(buf, "PUSH_BUTTON_NOOVERLAP",
			strlen("PUSH_BUTTON_NOOVERLAP")) == 0) {
	TUTRACE((TUTRACE_PROTO, "timeout enabled again\n" ));
	gp_mc->mp_trans->EnableWalkTimer(true);
    } else if ((strncmp(buf, "STA_TIMEOUT:",
			strlen("STA_TIMEOUT:")) == 0)) {
	TUTRACE((TUTRACE_PROTO, "STA_TIMEOUT\n"));
        //Check the STA mac address and reset SM if it matches
	gp_mc->mp_trans->GetStaMacAddr(macAddr);
	if( !strncmp(&buf[11], macAddr, SIZE_MAC_ADDR)){
		TerminateCurrentSession();
	}
    } else {
	TUTRACE((TUTRACE_PROTO, "received evenet %s in 2 minutes, ignored\n",buf ));
        // ignore the other events here
    }
    send_wsc_event_ack(m_wsc_event_port);
    return 0;
}

/*
 * Name        : ActualUIThreadProc
 * Description : This is the thread procedure for the UI thread.
 *                 Monitor the uiQueue, and process all callbacks that
 *                 are received.
 * Arguments   : void *p_data = NULL
 * Return type : void *
 */
void *
ActualUIThreadProc( IN void *p_data )
{
    bool    b_done = false;
    uint32    h_status;
    void    *p_cbData;
    S_LCB_HEADER *p_header;

    TUTRACE((TUTRACE_DBG, "WscCmd:ActualUIThreadProc: Started.\n"));
 	setWscStatus("STOPED");
    // keep doing this until the thread is killed
    while ( !b_done )
    {
        WscSleep(0);
        // block on the uiQueue
	while(1) {
            h_status = gp_uiQ->Dequeue(
                            NULL,        // size of dequeued msg
                            2,            // sequence number
                            4,            // 0:infinite timeout
                            (void **) &p_cbData);
                                          // pointer to the dequeued msg
            if ( h_status != WSC_SUCCESS ) {
                check_wsc_event_within_2minutes_cycle();
	    } else {
                break;
	    }
        }

        TUTRACE((TUTRACE_DBG, "WscCmd:ActualUIThreadProc: Started.\n"));

        p_header = (S_LCB_HEADER *)p_cbData;
		//printf("::ActualCBThreadProc(): p_header->eType = %d\r\n", p_header->eType);

        // once we get something, parse it,
        // do whats necessary, and then block
        switch( p_header->eType )
        {
            case LCB_QUIT:
            {
                // no params
                // destroy the queue
                if ( gp_uiQ )
                {
                    gp_uiQ->DeInit();
                    delete gp_uiQ;
                }
                // kill the thread
                b_done = true;
                break;
            }
            case LCB_MENU_UNCONF_AP:
            case LCB_MENU_AP_PROXY_REGISTRAR:
            {
                char inp[3];
                char devPwd[65];
                uint16 devPwdId = WSC_DEVICEPWDID_DEFAULT;
                uint16 selRegCfgMethods=WSC_CONFMET_LABEL;

                gp_mc->mp_trans->EnableWalkTimer(false);

                if(CMasterControl::GetRegSM())
                             CMasterControl::GetRegSM()->EnablePassthru(true);

                /* July 1, 2008, LiangXin.
                 * Move WLAN Mode switch code from here to CMaster::SwitchModeOn.
                 * XXX, will this cause problem???
                 */
                 if (EModeUnconfAp == gp_mc->mp_info->GetConfiguredMode())
                 {
                     // We should move SetBeaconIE and SetProbeRespIE here
                        gp_mc->SetBeaconIE(false,                    /* configured */
                                           false,                      /* selected   */
                                           WSC_DEVICEPWDID_DEFAULT,    /* DevPwdID, default:PIN */
                                           WSC_CONFMET_LABEL);         /* selRegCfgMethods */

                        gp_mc->SetProbeRespIE(WSC_MSGTYPE_AP_WLAN_MGR,   /* response type      */
                                              WSC_SCSTATE_UNCONFIGURED,     /* WSC state          */
                                              false,                      /* selected registrar */
                                              WSC_DEVICEPWDID_DEFAULT,    /* devPwdId */
                                              WSC_CONFMET_LABEL );        /* selRegCfgMethods */
                 }
                 else {
                     gp_mc->SetBeaconIE(true,                    /* configured */
                               false,                      /* selected   */
                        WSC_DEVICEPWDID_DEFAULT,    /* DevPwdID, default:PIN */
                   WSC_CONFMET_LABEL);         /* selRegCfgMethods */

                     gp_mc->SetProbeRespIE(WSC_MSGTYPE_AP_WLAN_MGR,   /* response type      */
                         WSC_SCSTATE_CONFIGURED,     /* WSC state          */
                     false,                      /* selected registrar */
                        WSC_DEVICEPWDID_DEFAULT,    /* devPwdId */
                        WSC_CONFMET_LABEL );        /* selRegCfgMethods */
                 }
                 gp_mc->setProcessStatus(WAIT_WSC_EVENT);
                 gp_mc->mp_trans->ClearStaMacAddr();

                 // reset stat machien
                 TerminateCurrentSession();

                 // wait for wsc events
                 wait_wsc_event(inp, devPwd);
                 TUTRACE((TUTRACE_DBG, "WscCmd::*****inp[0]=%c\n",inp[0]));

                 gp_mc->mp_trans->EnableWalkTimer(true);

                 if(CMasterControl::GetRegSM())
                             CMasterControl::GetRegSM()->EnablePassthru(false);

                 if( inp[0] == '1') //PIN
                 {
                    devPwdId = WSC_DEVICEPWDID_DEFAULT;
                    selRegCfgMethods = WSC_CONFMET_LABEL;
                 } else
                 if(inp[0] == '2')  //PBC
                 {
                    devPwdId = WSC_DEVICEPWDID_PUSH_BTN;
                    selRegCfgMethods = WSC_CONFMET_PBC;
                 }

                    /* Is this neccessary?? -- neccessary ! 
                     * or the QSS will be disabled if the card choose auto selection 
                     * by lsz, 080924
                     */
                    #if 1
                 if (EModeUnconfAp != gp_mc->mp_info->GetConfiguredMode())
                 {
                     gp_mc->SetBeaconIE(
                                true,            /* configured */
                                true,            /* selected   */
                                devPwdId,        /* DevPwdID*/
                                selRegCfgMethods);/* selRegCfgMethods */

                     gp_mc->SetProbeRespIE(
                                WSC_MSGTYPE_AP_WLAN_MGR,    /* response type */
				WSC_SCSTATE_CONFIGURED,     /* WSC state*/
                                true,                       /* selected registrar */
				devPwdId,                   /* devPwdId */
				selRegCfgMethods); /* selRegCfgMethods */

                 } else {
                      gp_mc->SetBeaconIE(false,        /* configured */
                                true,         /* selected   */
                                devPwdId,     /* DevPwdID*/
                                selRegCfgMethods);/* selRegCfgMethods */

                      gp_mc->SetProbeRespIE(
                                WSC_MSGTYPE_AP_WLAN_MGR,  /* response type      */
				WSC_SCSTATE_UNCONFIGURED, /* WSC state          */
				true,                    /* selected registrar */
				devPwdId,             /* devPwdId */
				selRegCfgMethods);    /* selRegCfgMethods */

                      if(( inp[0] == '1') || ( inp[0] == '2'))
                      {
                           //AP is unconfigured, but received event to use
                           // internal registrar,enable the default wsc
                           // configuration.
                           // Not required by WiFi-org
                           wsc_enable_default_cfg();
                           gp_mc->setProcessStatus(AP_AUTO_CFG_STARTED);
                     }
                 }
#endif
                 if (EModeUnconfAp == gp_mc->mp_info->GetConfiguredMode())
                 {
                     if(( inp[0] == '1') || ( inp[0] == '2'))
                      {
                           //AP is unconfigured, but received event to use
                           // internal registrar,enable the default wsc
                           // configuration.
                           // Not required by WiFi-org
                           wsc_enable_default_cfg();
                           gp_mc->setProcessStatus(AP_AUTO_CFG_STARTED);
                     }
                 }
                 switch( inp[0] )
                 {

                        case '0':
                        {
                            // Do same functionality as LCB_QUIT
                            // destroy the queue
                            if ( gp_uiQ )
                            {
                                gp_uiQ->DeInit();
                                delete gp_uiQ;
                            }
                            // kill the thread
                            b_done = true;
                            break;
                        }

                        case '1': // Configure Client via pin
                        {
                            // Set the transport callback
                            gp_mc->mp_trans->SetSMCallback(
                                        CRegistrarSM::StaticCallbackProc,
                                        gp_mc->mp_regSM );
                            TUTRACE((TUTRACE_DBG, "Waiting for Client to connect...\n"));
                            led_display(LED_INPROGRESS);
                            setWscStatus("PIN_PROCESS");
                            if ( 0 != devPwd[0] )
                {
                gp_mc->InitiateRegistration(
                                                      EModeApProxyRegistrar,
                        EModeClient, devPwd, NULL );
                 }
                 else
                 {
                 gp_mc->InitiateRegistration(
                                                      EModeApProxyRegistrar,
                            EModeClient, NULL, NULL );
                            }

                            TUTRACE((TUTRACE_DBG,
                                "WscCmd::Registration initiated\n"));
                            break;
                        }

			case '2': // Configure client via push-button
			{
			    char devPwd[9];

                            // Set the transport callback
                            gp_mc->mp_trans->SetSMCallback(
                                      CRegistrarSM::StaticCallbackProc, gp_mc->mp_regSM );

			    strcpy( devPwd, "00000000\0" );
				setWscStatus("PBC_PROCESS");
			    TUTRACE((TUTRACE_DBG, "Waiting for Client to connect...\n"));
			    gp_mc->InitiateRegistration(EModeApProxyRegistrar,
							EModeClient, devPwd, NULL,
							false, WSC_DEVICEPWDID_PUSH_BTN );
		            TUTRACE((TUTRACE_DBG, "WscCmd::Registration initiated\n"));

                            send_wsc_event_ack(m_wsc_event_port);

			    break;
			}
			case '3': // configured by external registrar through EAP
         		{

                            gp_mc->InitiateRegistration(
                                            EModeUnconfAp, EModeRegistrar,
                                            NULL, NULL );

			    TUTRACE((TUTRACE_PROTO, "Waiting for Registrar to connect...\n"));
                            TUTRACE((TUTRACE_DBG, "WscCmd::Registration initiated\n"));

                            send_wsc_event_ack(m_wsc_event_port);

                            break;
			}

                        case '4': // configured by external registrar through UPnP
         		{

                            gp_mc->InitiateRegistration(
                                            EModeUnconfAp, EModeRegistrar,
                                            NULL, NULL );
                            if ((gp_mc->mp_regSM == gp_mc->mp_trans->GetSMCallback()) ||
                                (0 == gp_mc->mp_trans->GetSMCallback()))
                            {
                                 gp_mc->mp_trans->SetSMCallback(
                                                  CEnrolleeSM::StaticCallbackProc,
                                                  gp_mc->mp_enrSM );
                                 TUTRACE((TUTRACE_DBG,
                                          "WscCmd:state machine switch to Enrolle.\n"));
                            }

			    TUTRACE((TUTRACE_DBG, "Waiting for Registrar to connect...\n"));
                            TUTRACE((TUTRACE_DBG, "WscCmd::Registration initiated\n"));

                            break;
			}
                        default:
                        {
                            TUTRACE((TUTRACE_ERR,"ERROR: Invalid input.\n"));
                            break;
                        }
                    } // switch
                break;
            }

            default:
            {
                // not understood, do nothing
                TUTRACE((TUTRACE_DBG,
                    "WscCmd:: Unknown callback type received\n"));
                break;
            }
        } // switch

        // free the data
        delete (uint8 *)p_cbData;
    } // while

    TUTRACE((TUTRACE_DBG, "WscCmd:ActualUIThreadProc: Exiting.\n"));
#ifdef __linux__
    pthread_exit(NULL);
#endif

    return NULL;
}

/*
 * Name        : KillUIThread
 * Description : Attempt to terminate the UI thread. Enqueue a CB_QUIT in the
 *               UI message queue.
 * Arguments   : none
 * Return type : void
 */
void
KillUIThread()
{
    // enqueue a CB_QUIT
    S_LCB_HEADER *p = new S_LCB_HEADER;
    p->eType = LCB_QUIT;
    p->dataLength = 0;

    gp_uiQ->Enqueue( sizeof(S_LCB_HEADER), 2, p );
    return;
} // KillUIThread

/*
 * Name        : CallbackProc
 * Description : Callback method that MasterControl uses to pass
 *                    info back to main()
 * Arguments   : IN void *p_callBackMsg - pointer to the data being
 *                    passed in
 *                 IN void *p_thisObj - NULL
 * Return type : none
 */
void
CallbackProc(IN void *p_callbackMsg, IN void *p_thisObj)
{
    S_CB_HEADER *p_header = (S_CB_HEADER *)p_callbackMsg;

    uint32 dw_length = sizeof(p_header->dataLength) +
                        sizeof(S_CB_HEADER);

    TUTRACE((TUTRACE_INFO, "Main callback:Ready to enqueue msg type %d.\n",
            p_header->eType));
    uint32 h_status = gp_cbQ->Enqueue( dw_length,    // size of the data
                                    1,                // sequence number
                                    p_callbackMsg );// pointer to the data
    if ( WSC_SUCCESS != h_status )
    {
        TUTRACE((TUTRACE_ERR, "WscCmd::CallbackProc Enqueue failed\n"));
    }
    else
    {
        TUTRACE((TUTRACE_DBG, "WscCmd::CallbackProc Enqueue done\n"));
    }
    return;
} // CallbackProc

static uint32 CopyOneHostapdConfFile(char * srcFileName, char * destFileName)
{
#if 0
    char buf[100];

    // Open files
    strcpy(buf, "cp ");
    strcat(buf, CInfo::GetWscConfigPath());
    strcat(buf, srcFileName);
    TUTRACE((TUTRACE_INFO, "WscCmd::AP tmp file =%s\n",buf));
    strcat(buf, " ");
    strcat(buf, CInfo::GetWscConfigPath());
    strcat(buf, destFileName);
    // Copy contents of the template into the hostapd.conf file
    system(buf);
    TUTRACE((TUTRACE_INFO, "WscCmd::AP Config file copied: cmd=%s\n"));
#else
	char c;
	char srcPath[64], dstPath[64];
	FILE *ps = NULL, *pd = NULL;

    // Get path
    memset(srcPath, 0, sizeof(srcPath));
    memset(dstPath, 0, sizeof(dstPath));
    strcpy(srcPath, "/etc/ath/");
    strcat(srcPath, srcFileName);
    strcpy(dstPath, CInfo::GetWscConfigPath());
    strcat(dstPath, destFileName);

	printf("CopyOneHostapdConfFile: Open file fail [%s] --> [%s] \n", srcPath, dstPath);
	// Open file
	ps = fopen(srcPath, "r");
	pd = fopen(dstPath, "w");
	if (ps == NULL || pd == NULL)
	{
		printf("CopyOneHostapdConfFile: Open file fail [%s] --> [%s] \n",
			srcPath, dstPath);
		return 0;
	}

    // Copy file
	while (EOF != (c = fgetc(ps)))
	{
		fputc(c, pd);
	}

	fclose(ps);
	fclose(pd);

	return 1;

#endif
}

/*
 * Name        : APCopyConfFile
 * Description : Copy the hostapd template config file
 * Arguments   : none
 * Return type : uint32 - result of the operation
 */
uint32 APCopyConfFile()
{
    if(gp_mc->mp_info->GetRFBand() == WSC_RFBAND_24GHZ)
    {
    	if(CInfo::GetAPNumRadio() == 1)  // Single Radio AP
	{
        	CopyOneHostapdConfFile(AP_CONF_TEMPLATE, AP_CONF_FILENAME);
	}
	else
	{
        	CopyOneHostapdConfFile(AP_CONF_TEMPLATE_ATH1, AP_CONF_FILENAME);
	}
    }
    else
    if(gp_mc->mp_info->GetRFBand() == WSC_RFBAND_50GHZ)
    {
        CopyOneHostapdConfFile(AP_CONF_TEMPLATE, AP_CONF_FILENAME);
    } else
    if(gp_mc->mp_info->GetRFBand() == (WSC_RFBAND_24GHZ|WSC_RFBAND_50GHZ))
    {
	// both bands are selected, make sure we are a dual band AP
    	if(CInfo::GetAPNumRadio() == 2)  // Dual Radio AP
	{
        	CopyOneHostapdConfFile(AP_CONF_TEMPLATE, AP_CONF_FILENAME );
	        CopyOneHostapdConfFile(AP_CONF_TEMPLATE_ATH1, AP_CONF_FILENAME_ATH1);
	}
	else  // Single Radio
	{
        	CopyOneHostapdConfFile(AP_CONF_TEMPLATE, AP_CONF_FILENAME);
	}
    }
    return WSC_SUCCESS;

} // APCopyConfFile

/*
 * Name        : APCopyOpenConfFile
 * Description : Copy the open hostapd config file
 * Arguments   : none
 * Return type : uint32 - result of the operation
 */
uint32 APCopyOpenConfFile()
{
    if(gp_mc->mp_info->GetRFBand() == WSC_RFBAND_24GHZ)
    {
        CopyOneHostapdConfFile(AP_CONF_OPEN, AP_CONF_FILENAME);
    }
    else
    if(gp_mc->mp_info->GetRFBand() == WSC_RFBAND_50GHZ)
    {
        CopyOneHostapdConfFile(AP_CONF_OPEN, AP_CONF_FILENAME);
    }
    else
    if(gp_mc->mp_info->GetRFBand() == (WSC_RFBAND_24GHZ|WSC_RFBAND_50GHZ))
    {
        CopyOneHostapdConfFile(AP_CONF_OPEN, AP_CONF_FILENAME );
        CopyOneHostapdConfFile(AP_CONF_OPEN_ATH1, AP_CONF_FILENAME_ATH1);
    }
    return WSC_SUCCESS;
} // APCopyOpenConfFile

/*
 * Name        : APCopyWepConfFile
 * Description : Copy the WEP hostapd config file
 * Arguments   : none
 * Return type : uint32 - result of the operation
 */
uint32 APCopyWepConfFile()
{
    if(gp_mc->mp_info->GetRFBand() == WSC_RFBAND_24GHZ)
    {
        CopyOneHostapdConfFile(AP_CONF_WEP, AP_CONF_FILENAME);
    }
    else
    if(gp_mc->mp_info->GetRFBand() == WSC_RFBAND_50GHZ)
    {
        CopyOneHostapdConfFile(AP_CONF_WEP, AP_CONF_FILENAME);
    }
    else
    if(gp_mc->mp_info->GetRFBand() == (WSC_RFBAND_24GHZ|WSC_RFBAND_50GHZ))
    {
        CopyOneHostapdConfFile(AP_CONF_WEP, AP_CONF_FILENAME );
        CopyOneHostapdConfFile(AP_CONF_WEP_ATH1, AP_CONF_FILENAME_ATH1);
    }
    return WSC_SUCCESS;
} // APCopyWepConfFile

/*
 * Name        : APAddParams
 * Description : Add specific params to the hostapd config file
 * Arguments   : char *ssid - ssid
 *               char *keyMgmt - value of keyMgmt
 *               char *nwKey - the psk to be used
 *				 uint32 nwKeyLen - length of the key
 * Return type : uint32 - result of the operation
 */
static uint32 AppParamsToOneHostapd(IN char * HostApdFileName,
                             IN char *ssid, IN char *keyMgmt,
                             IN char *nwKey, IN uint32 nwKeyLen )
{
    if ( !ssid && !keyMgmt && !nwKey )
        return WSC_SUCCESS;

    char tmpBuf[100];

    strcpy(tmpBuf, CInfo::GetWscConfigPath());
    strcat(tmpBuf, HostApdFileName);
    FILE *fp = fopen(tmpBuf , "a" );
    if( !fp )
        return WSC_ERR_FILE_OPEN;

    strcpy(tmpBuf, CInfo::GetWscConfigPath());
    strcat(tmpBuf, "hostapd.eap_user");
    tmpBuf[strlen(tmpBuf)] = 0;
    fprintf( fp,"eap_user_file=%s\n", tmpBuf);

    if ( ssid )
    {
        fprintf( fp, "ssid=%s\n", ssid );
        printf( "ssid=%s\n", ssid );
    }
    if ( keyMgmt )
    {
        fprintf( fp, "wpa_key_mgmt=%s\n", keyMgmt );
    }

    if ( nwKeyLen > 0 )
    {
	// If nwKeyLen == 64 bytes, then it is the PSK
	// If psk < 64 bytes , it should be interpreted as the passphrase
	char line[100];

        if (gp_mc->mp_info->GetEncrTypeFlags()==WSC_ENCRTYPE_WEP)
        {
            //handle WEP key
            sprintf( line, "wsc_wep_key=");
        } else {
            // handle WPA key
            if ( nwKeyLen < 64 )
	    {
                 // Interpret as passphrase
	         // Should be < 63 characters
	         sprintf( line, "wpa_passphrase=" );
	    }
	    else
	    {
	         // Interpret as psk
	         // Should be 64 characters
	         sprintf( line, "wpa_psk=" );
	    }
        }

        strncat( line, nwKey, nwKeyLen );

        fprintf( fp, "%s\n", line );
    }
    fclose(fp);

    TUTRACE((TUTRACE_INFO, "WscCmd::Params added to AP config file\n"));
    return WSC_SUCCESS;
}


/*
 * Name        : APAddParams
 * Description : Add specific params to the hostapd config file
 * Arguments   : char *ssid - ssid
 *               char *keyMgmt - value of keyMgmt
 *               char *nwKey - the psk to be used
 *				 uint32 nwKeyLen - length of the key
 * Return type : uint32 - result of the operation
 */
uint32 APAddParams( IN char *ssid, IN char *keyMgmt,
					IN char *nwKey, IN uint32 nwKeyLen )
{
    uint32 ret = 0;

    ret = AppParamsToOneHostapd(AP_CONF_FILENAME, ssid,
                                keyMgmt,nwKey,nwKeyLen);
    ret = AppParamsToOneHostapd(AP_CONF_FILENAME_ATH1, ssid,
                              keyMgmt,nwKey,nwKeyLen);
    return ret;
} // APAddParams

/*
 * Name        : APRestartNetwork
 * Description : Restart network config if necessary
 * Arguments   : none
 * Return type : uint32 - result of the operation
 */
uint32 APRestartNetwork()
{
    gb_apRunning = false;
    system( "killall hostapd" );
    TUTRACE((TUTRACE_DBG, "WscCmd::******hostapd is killed, wait 2 seconds, hostapd eloop bug\n"));
    WscSleep(3);
    TUTRACE((TUTRACE_DBG, "WscCmd::******hostapd is down\n"));
    return WSC_SUCCESS;
} // APRestart

/*
 * Name        : SuppWriteConfFile
 * Description : Write out a specific version of the config file for
 *               wpa_supplicant. Dependant on the mode that we are
 *               currently in
 * Arguments   : char *ssid - ssid
 *               char *keyMgmt - value of keyMgmt to be used
 *               char *nwKey - psk to be used
 *				 uint32 nwKeyLen - length of the key
 *               char *identity - identity to be used for the EAP-WSC method
 *               bool b_startWsc - flag that indicates whether the EAP-WSC
 *               method needs to be run
 * Return type : uint32 - result of the operation
 */
uint32 SuppWriteConfFile( IN char *ssid, IN char *keyMgmt, IN char *nwKey,
                            IN uint32 nwKeyLen, IN char *identity,
							IN bool b_startWsc )
{
    FILE *fp = fopen( SUPP_CONF_FILENAME, "w" );
    if ( !fp )
        return WSC_ERR_FILE_OPEN;

    fprintf( fp, "ctrl_interface=/var/run/wpa_supplicant\n" );
    fprintf( fp, "eapol_version=1\n" );
    fprintf( fp, "ap_scan=1\n" );
    fprintf( fp, "network={\n" );
    fprintf( fp, "\tssid=\"%s\"\n", ssid );
    fprintf( fp, "\tkey_mgmt=%s\n", keyMgmt );
    if ( b_startWsc )
    {
        // set the profile so that we use EAP-WSC
        fprintf( fp, "\teap=WSC\n" );
        fprintf( fp, "\tidentity=\"%s\"\n", identity );
    }
    else
    {
		// Assuming non-zero nwKeyLen
		if ( nwKeyLen > 0 )
		{
			fprintf( fp, "\tproto=WPA\n" );
			fprintf( fp, "\tpairwise=TKIP\n" );
			char line[100];
			sprintf( line, "\tpsk=" );
			if ( nwKeyLen < 64 )
			{
				// Interpret as passphrase
				// Must be 8-63 characters
				strcat( line, "\"" );
				strcat( line, nwKey );
				strcat( line, "\"" );
			}
			else
			{
				// Interpret as PSK
				// Must be exactly 64 characters
				strncat( line, nwKey, 64 );
			}
			fprintf( fp, "%s\n", line );
		}
    }
    fprintf( fp, "\tscan_ssid=1\n" );
    fprintf( fp, "}\n" );

    fclose( fp );
    WscSleep( 1 );
    TUTRACE((TUTRACE_DBG, "WscCmd::Supp config file written\n"));
    return WSC_SUCCESS;
} // SuppWriteConfFile

/*
 * Name        : TerminateCurrentSession
 * Description : Terminate current session
 * Arguments   : none
 * Return type : void
 */
void TerminateCurrentSession()
{
    // sta maybe retry .gp_mc->mp_regSM->NotifySessionFail();
    gp_mc->mp_regSM->RestartSM();
    gp_mc->mp_enrSM->RestartSM();
} // TerminateCurrentSession
