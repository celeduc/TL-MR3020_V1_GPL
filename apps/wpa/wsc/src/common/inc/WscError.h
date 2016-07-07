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
//  File Name: WscError.h
//  Description: Description of error codes used in the WSC implementation.
//
****************************************************************************/

#ifndef _WSC_ERROR_
#define _WSC_ERROR_

#pragma pack(push, 1)

// generic
#define WSC_BASE                    0x1000
#define WSC_SUCCESS                 WSC_BASE+1
#define WSC_ERR_OUTOFMEMORY         WSC_BASE+2
#define WSC_ERR_SYSTEM              WSC_BASE+3
#define WSC_ERR_NOT_INITIALIZED     WSC_BASE+4
#define WSC_ERR_INVALID_PARAMETERS  WSC_BASE+5
#define WSC_ERR_BUFFER_TOO_SMALL    WSC_BASE+6
#define WSC_ERR_NOT_IMPLEMENTED     WSC_BASE+7
#define WSC_ERR_ALREADY_INITIALIZED WSC_BASE+8
#define WSC_ERR_GENERIC             WSC_BASE+9
#define WSC_ERR_FILE_OPEN           WSC_BASE+10
#define WSC_ERR_FILE_READ           WSC_BASE+11
#define WSC_ERR_FILE_WRITE          WSC_BASE+12

// CQueue
#define CQUEUE_BASE               0x2000
#define CQUEUE_ERR_INTERNAL       CQUEUE_BASE+1
#define CQUEUE_ERR_IPC            CQUEUE_BASE+2

// State machine
#define SM_BASE                   0x3000
#define SM_ERR_INVALID_PTR        SM_BASE+1
#define SM_ERR_WRONG_STATE        SM_BASE+2
#define SM_ERR_MESSAGE_DATA       SM_BASE+3

// MasterControl
#define MC_BASE							0x4000
#define MC_ERR_CFGFILE_CONTENT          MC_BASE+1
#define MC_ERR_CFGFILE_OPEN             MC_BASE+2
#define MC_ERR_STACK_ALREADY_STARTED    MC_BASE+3
#define MC_ERR_STACK_NOT_STARTED        MC_BASE+4
#define MC_ERR_VALUE_UNCHANGED			MC_BASE+5

// Transport
#define TRANS_BASE                 0x5000

#define TRUFD_BASE                 0x5100
#define TRUFD_ERR_DRIVE_REMOVED    TRUFD_BASE+1
#define TRUFD_ERR_FILEOPEN         TRUFD_BASE+2
#define TRUFD_ERR_FILEREAD         TRUFD_BASE+3
#define TRUFD_ERR_FILEWRITE        TRUFD_BASE+4
#define TRUFD_ERR_FILEDELETE       TRUFD_BASE+5

#define TRNFC_BASE                 0x5200
#define TRNFC_ERR_NO_TAG           TRNFC_BASE+1
#define TRNFC_ERR_NO_READER        TRNFC_BASE+2

#define TREAP_BASE                 0x5300
#define TREAP_ERR_SENDRECV         TREAP_BASE+1
#define TREAP_ERR_RECV_TIMEOUT     TREAP_BASE+2

#define TRWLAN_BASE                0x5400
#define TRWLAN_ERR_SENDRECV        TRWLAN_BASE+1

#define TRIP_BASE                       0x5500
#define TRIP_ERR_SENDRECV               TRIP_BASE+1
#define TRIP_ERR_NETWORK                TRIP_BASE+2
#define TRIP_ERR_NOT_MONITORING         TRIP_BASE+3
#define TRIP_ERR_ALREADY_MONITORING     TRIP_BASE+4
#define TRIP_ERR_INVALID_SOCKET         TRIP_BASE+5

#define TRUPNP_BASE                 0x5600
#define TRUPNP_ERR_SENDRECV         TRUPNP_BASE+1

// RegProtocol
#define RPROT_BASE                 0x6000

#define RPROT_ERR_REQD_TLV_MISSING RPROT_BASE+1
#define RPROT_ERR_CRYPTO           RPROT_BASE+2
#define RPROT_ERR_INCOMPATIBLE     RPROT_BASE+3
#define RPROT_ERR_INVALID_VALUE    RPROT_BASE+4
#define RPROT_ERR_NONCE_MISMATCH   RPROT_BASE+5
#define RPROT_ERR_WRONG_MSGTYPE    RPROT_BASE+6
/* July 01, 2008, Liang Xin.
 * To support WCN Logo Test.
 */
#define RPROT_ERR_AUTH_INCOMPATIBLE RPROT_BASE+7
#define RPROT_ERR_ENCR_INCOMPATIBLE RPROT_BASE+8

//Portability
#define PORTAB_BASE                 0x7000
#define PORTAB_ERR_SYNCHRONIZATION  PORTAB_BASE+1
#define PORTAB_ERR_THREAD           PORTAB_BASE+2
#define PORTAB_ERR_EVENT            PORTAB_BASE+3
#define PORTAB_ERR_WAIT_ABANDONED   PORTAB_BASE+4
#define PORTAB_ERR_WAIT_TIMEOUT     PORTAB_BASE+5



#pragma pack(pop)
#endif // _WSC_ERROR_
