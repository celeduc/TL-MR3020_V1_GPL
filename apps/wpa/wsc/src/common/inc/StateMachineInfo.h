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
//  File Name: StateMachineInfo.h
//  Description: Definitions for State Machine data structures.
//
****************************************************************************/

#ifndef _SM_INFO_H
#define _SM_INFO_H

#pragma pack(push, 1)

/* Moved to inbeap.h
//WSC Message types
#define WSC_Start 0x01
#define WSC_ACK   0x02
#define WSC_NACK  0x03
#define WSC_MSG   0x04
#define WSC_Done  0x05

//WSC packet header
typedef struct {
    uint8 opCode;
    uint8 flags;
}S_WSC_HEADER;

typedef struct {
    S_WSC_HEADER header;
    uint16       messageLength;
}S_WSC_FRAGMENT_HEADER;

*/
// data structures for each instance of registration protocol
typedef enum {
    START = 0,
    CONTINUE,
    RESTART,
    SUCCESS, 
    FAILURE
} ESMState;

typedef enum {
    MSTART = 0,
    M1,
    M2,
    M2D,
    M3,
    M4,
    M5,
    M6,
    M7,
    M8,
    DONE,
    NACK,
    MNONE = 99
} EMsg;

// data structure to store info about a particular instance
// of the Registration protocol
typedef struct {
    ESMState    e_smState;
    EMsg        e_lastMsgRecd;
    EMsg        e_lastMsgSent;

    // TODO: must store previous message as well to compute hash
    
    // enrollee endpoint - filled in by the Registrar, NULL for Enrollee
    S_DEVICE_INFO    *p_enrolleeInfo;        
    // Registrar endpoint - filled in by the Enrollee, NULL for Registrar
    S_DEVICE_INFO    *p_registrarInfo;    

    //Diffie Hellman parameters
    BIGNUM      *DH_PubKey_Peer; //peer's pub key stored in bignum format
    DH          *DHSecret;       //local key pair in bignum format
    uint8       pke[SIZE_PUB_KEY]; //enrollee's raw pub key
    uint8       pkr[SIZE_PUB_KEY]; //registrar's raw pub key

    BufferObj   password;
    void        *staEncrSettings; // to be sent in M2/M8 by reg & M7 by enrollee
    void        *apEncrSettings;

    uint16      enrolleePwdId;
    uint8       enrolleeNonce[SIZE_128_BITS];//N1
    uint8       registrarNonce[SIZE_128_BITS];//N2
    

    uint8       psk1[SIZE_128_BITS];
    uint8       psk2[SIZE_128_BITS];

    uint8       eHash1[SIZE_256_BITS];
    uint8       eHash2[SIZE_256_BITS];
    uint8       es1[SIZE_128_BITS];
    uint8       es2[SIZE_128_BITS];

    uint8       rHash1[SIZE_256_BITS];
    uint8       rHash2[SIZE_256_BITS];
    uint8       rs1[SIZE_128_BITS];
    uint8       rs2[SIZE_128_BITS];

    BufferObj   authKey;
    BufferObj   keyWrapKey;
    BufferObj   emsk;
    //BufferObj   iv;

    BufferObj   x509csr;
    BufferObj   x509Cert;

    BufferObj   inMsg;        // A recd msg will be stored here
    BufferObj   outMsg;     // Contains msg to be transmitted
}__attribute__ ((__packed__)) S_REGISTRATION_DATA;

#pragma pack(pop)
#endif //_SM_INFO_H
