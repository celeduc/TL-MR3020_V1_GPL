/***************************************************************
 *
 * Copyright(c) 2005-2008 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename		:	msgq.h
 * Version		:	1.0
 * Abstract		:	
 * Author		:	LI SHAOZHANG (lishaozhang@tp-link.net)
 * Created Date	:	19Jul08
 *
 * Modified History:
 ***************************************************************/

#ifndef _MSG_Q_H_
#define _MSG_Q_H_

typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef int BOOL;

#define TP_IPC_MAGIC		0xbabeface
#define TP_IPC_VERSION		0x10		/* 1.0 */
#define MSG_Q_KEY			0x3F
#define MSG_MAX_LEN			(1024 * 2)

typedef struct tp_ipc_msg_struct
{
	UINT32 magic;		/* Must be 0xbabeface */
	UINT32 version;		/* Header version */
	UINT32 dstMid;		/* ID of receiver module */
	UINT32 srcMid;		/* ID of sender module */
	UINT32 msgType;		/* data type of the IPC msg */
	BOOL   bFrag;		/* There is fragment data in next msg */
	UINT8  payload[0];	/* real data */
}tp_ipc_msg;

typedef enum 
{
	IPC_ANNOUNCEMENT,
	IPC_ECHO_REQUEST,
	IPC_ECHO_REPLY,
	IPC_DATA_SET,
	IPC_DATA_GET,
	IPC_TYPE_MAX
}tp_ipc_msg_type;

typedef enum
{
	HTTPD = 1,
	PPPD,
	PPPOE,
	L2TP,
	PPTP,
	DHCPC,
	DHCPS,
	ADVSEC,
	NTP,
	DNS_DETECT,
	SYS_STATS,
	MODULE_ID_MAX
}tp_ipc_module_id;



#endif

