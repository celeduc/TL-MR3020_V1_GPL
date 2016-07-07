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
//  File Name: RegProtocol.h
//  Description: Header file for CRegProtocol Class and associated
//               definitions.
//
****************************************************************************/

#ifndef _REGPROT_
#define _REGPROT_

using namespace std;

#pragma pack(push, 1)

class CRegProtocol
{
private:
    uint8 version;

public:
    CRegProtocol();
    ~CRegProtocol();

    uint32 SetMCCallback(IN CALLBACK_FN p_mcCallbackFn, IN void* cookie);

    // build message methods
    uint32 BuildMessageM1(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 BuildMessageM2(S_REGISTRATION_DATA *regInfo, BufferObj &msg, void *encrSettings);
    uint32 BuildMessageM2D(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 BuildMessageM3(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 BuildMessageM4(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 BuildMessageM5(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 BuildMessageM6(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 BuildMessageM7(S_REGISTRATION_DATA *regInfo, BufferObj &msg, void *encrSettings);
    uint32 BuildMessageM8(S_REGISTRATION_DATA *regInfo, BufferObj &msg, void *encrSettings);
    uint32 BuildMessageAck(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 BuildMessageNack(S_REGISTRATION_DATA *regInfo, BufferObj &msg, uint16 configError);
    /* July 01, 2008, Liang Xin.
     * For WCN Logo Test.
     */
    uint32 BuildMessageNack(S_REGISTRATION_DATA *regInfo, IN uint8 *nonce,
                                IN uint16 nonceType,
                                BufferObj &msg, uint16 configError);
    uint32 BuildMessageDone(S_REGISTRATION_DATA *regInfo, BufferObj &msg);


    // process message methods
    uint32 ProcessMessageM1(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 ProcessMessageM2(S_REGISTRATION_DATA *regInfo, BufferObj &msg, void **encrSettings);
    uint32 ProcessMessageM2D(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 ProcessMessageM3(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 ProcessMessageM4(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 ProcessMessageM5(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 ProcessMessageM6(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 ProcessMessageM7(S_REGISTRATION_DATA *regInfo, BufferObj &msg, void **encrSettings);
    uint32 ProcessMessageM8(S_REGISTRATION_DATA *regInfo, BufferObj &msg, void **encrSettings);
    uint32 ProcessMessageAck(S_REGISTRATION_DATA *regInfo, BufferObj &msg);
    uint32 ProcessMessageNack(S_REGISTRATION_DATA *regInfo, BufferObj &msg, uint16 *configError);
    uint32 ProcessMessageDone(S_REGISTRATION_DATA *regInfo, BufferObj &msg);

    //utility methods
    uint32 GenerateDHKeyPair(DH **DHKeyPair, BufferObj &pubKey);
    void GenerateSHA256Hash(BufferObj &inBuf, BufferObj &outBuf);
    void DeriveKey(BufferObj &KDK, BufferObj &prsnlString, uint32 keyBits, BufferObj &key);
    bool ValidateMac(BufferObj &data, uint8 *hmac, BufferObj &key);
    bool ValidateKeyWrapAuth(BufferObj &data, uint8 *hmac, BufferObj &key);
    void EncryptData(BufferObj &plainText,
                        BufferObj &encrKey,
                        BufferObj &authKey,
                        BufferObj &cipherText,
                        BufferObj &iv);
    void DecryptData(BufferObj &cipherText,
                          BufferObj &iv,
                          BufferObj &encrKey,
                          BufferObj &authKey,
                          BufferObj &plainText);
    uint32 GeneratePSK(IN uint32 length, OUT BufferObj &PSK);
    uint32 CheckNonce(IN uint8 *nonce, IN BufferObj &msg, IN int nonceType);
    /* July 01, 2008, Liang Xin
     * For WCN Logo Test.
     */
    uint32 GetNonceFromMsg(OUT uint8 *nonce, IN BufferObj &msg, IN int nonceType);
    uint32 GetMsgType(uint32 &msgType, BufferObj &msg);
/*    uint32 CreatePrivateKey(char *name, EVP_PKEY **key);
    uint32 GetPublicKey(char *Name, EVP_PKEY **key);
    uint32 GetPrivateKey(char *Name, EVP_PKEY **key);
*/
}; // CRegProtocol

#pragma pack(pop)
#endif // _REGPROT_



