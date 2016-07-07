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
//  File Name: WscCmd.h
//  Description: Prototypes for methods implemented in WscCmd.cpp
//
****************************************************************************/

#ifndef _WSC_CMD_
#define _WSC_CMD_

#pragma pack(push, 1)

#define AP_CONF_TEMPLATE         "template.conf"
#define AP_CONF_TEMPLATE_ATH1    "template.conf_1"
#define AP_CONF_OPEN             "hostapd.open"
#define AP_CONF_OPEN_ATH1        "hostapd.open_1"
#define AP_CONF_WEP              "hostapd.wep"
#define AP_CONF_WEP_ATH1         "hostapd.wep_1"
#define AP_CONF_FILENAME         "hostapd.conf"
#define AP_CONF_FILENAME_ATH1    "hostapd.conf_1"
#define SUPP_CONF_FILENAME       "config.conf"

#define WSC_EVENT_PORT  38100
#define WSC_EVENT_ADDR  "127.0.0.1"
#define WSC_EVENT_ACK_STRING    "WSC_EVENT_ACK"
#define EVENT_BUF_SIZE  40


/* XXX, what's the meaning of LCB? */
typedef enum 
{
    LCB_QUIT = 0,
    LCB_MENU_UNCONF_AP,	/* unconfigured AP, 1 */
    LCB_MENU_CLIENT,	/* client, means normal enrollee??  */
    LCB_MENU_REGISTRAR,	/* registrar */
	LCB_MENU_AP_PROXY,	/* pure proxy AP */
    LCB_MENU_AP_PROXY_REGISTRAR,/* AP with internal registrar & proxy ??? */
	LCB_MENU_REQUEST_PWD
} ELCBType;

/* this header type has lots of type variations,
 * seems like a TLV header
 */
typedef struct 
{
    ELCBType	eType;
    uint32      dataLength;
} __attribute__ ((__packed__)) S_LCB_HEADER, S_LCB_MENU_UNCONF_AP,
    S_LCB_MENU_CLIENT, S_LCB_MENU_REGISTRAR,
    S_LCB_MENU_AP_PROXY, S_LCB_MENU_AP_PROXY_REGISTRAR;

/* SIZE_32_BYTES... */
typedef struct
{
	S_LCB_HEADER	cbHeader;
	char			deviceName[SIZE_32_BYTES];
    char			modelNumber[SIZE_32_BYTES];
    char			serialNumber[SIZE_32_BYTES];	
	uint8			uuid[SIZE_16_BYTES];
} __attribute__ ((__packed__)) S_LCB_MENU_REQUEST_PWD;


// ******** data ********
CMasterControl  *gp_mc;		/* global Master Control object */
bool            gb_apRunning,
				gb_regDone, gb_useUsbKey, gb_useUpnp,
				gb_writeToUsb;
//EMode           ge_mode;
int             event_sock;	
int             m_wsc_event_port;

// callback data structures
CWscQueue    *gp_cbQ, *gp_uiQ;	/* callback queue */

/* thread handle */
uint32        g_cbThreadHandle, g_uiThreadHandle, g_ledThreadHandle;	

// 20071116, chenyan. no need here, input pin is usable
char 	g_pin[16];	/* XXX? */

// ******** functions ********
// local functions
uint32 Init();
uint32 DeInit();

// callback thread functions
void * ActualCBThreadProc( IN void *p_data = NULL );
void KillCallbackThread();
void * ActualUIThreadProc( IN void *p_data = NULL );
void KillUIThread();

// callback function for CMasterControl to call in to
void CallbackProc(IN void *p_callbackMsg, IN void *p_thisObj);

// helper functions for hostapd
uint32 APCopyConfFile();
uint32 APCopyOpenConfFile();
uint32 APCopyWepConfFile();
uint32 APAddParams( IN char *ssid, IN char *keyMgmt, 
					IN char *nwKey, IN uint32 nwKeyLen );
uint32 APRestartNetwork();

// helper functions for wpa_supp
uint32 SuppWriteConfFile( IN char *ssid, IN char *keyMgmt, IN char *nwKey,
							IN uint32 nwKeyLen, IN char *identity,  
							IN bool b_startWsc );

void TerminateCurrentSession();

#pragma pack(pop)
#endif
