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
//  File Name: Info.h
//  Description: Prototypes for methods implemented in Info.cpp
//
****************************************************************************/

#ifndef _INFO_
#define _INFO_

#ifdef __linux__
#include <stdio.h>	// for file operations
#endif

#include <openssl/dh.h>

#pragma pack( push, 1 )

#define WSC_CFG_FILE_NAME	"wsc_config.txt"
#define DEFAULT_WSC_SSID        "WscAtherosAP"

class CInfo
{
protected:
	S_DEVICE_INFO		*mp_deviceInfo;
	DH			*mp_dhKeyPair;
	uint8			m_pubKey[SIZE_PUB_KEY];
	uint8			m_sha256Hash[SIZE_256_BITS];
	char			m_nwKey[SIZE_64_BYTES];
	uint32			m_nwKeyLen;
	bool			mb_nwKeySet;
	char			*mcp_devPwd;
	uint32			*mh_lock;
	bool			mb_infoConfigSet;
	EMode			e_mode;
	bool			mb_useUsbKey;
	bool			mb_regWireless;
	bool			mb_useUpnp;
	uint32			m_dbgLevel;

public:
	CInfo();
	~CInfo();

	uint32 ReadConfigFile();
	uint32 WriteConfigFile();

	S_DEVICE_INFO * GetDeviceInfo();
	uint32 * GetLock();

	EMode GetConfiguredMode();
        void  SetConfiguredMode( IN EMode mode);
	bool IsInfoConfigSet();

	uint8 * GetUUID();
	// void GetMacAddr( OUT uint8(&macAddr)[SIZE_6_BYTES] );
	uint8 * GetMacAddr();
	uint8 GetVersion();
	char * GetDeviceName( OUT uint16 &len );
	void SetDeviceName( char * deviceName );
	char * GetDeviceType( OUT uint16 &len );
	char * GetManufacturer( OUT uint16 &len );
	char * GetModelName( OUT uint16 &len );
	char * GetModelNumber( OUT uint16 &len );
	char * GetSerialNumber( OUT uint16 &len );
	uint16 GetConfigMethods();
	uint16 GetAuthTypeFlags();
	void   SetAuthTypeFlags(uint16 authType);
	uint16 GetEncrTypeFlags();
	void   SetEncrTypeFlags(uint16 encrTypeFlags);
	uint8 GetConnTypeFlags();
	//sep,11,08,add by weizhengqin
	uint16 GetAuthType();
	void   SetAuthType(uint16 authType);
	uint16 GetEncrType();
	void   SetEncrType(uint16 encrType);
	//end
        uint8 GetRFBand();
	uint32 GetOsVersion();
	uint32 GetFeatureId();
	uint16 GetAssocState();
	uint16 GetDevicePwdId();
	uint16 GetConfigError();
	bool IsAP();
	void SetAP(bool b_ap);
	char * GetSSID( OUT uint16 &len );
	void SetSSID( char * ssid);
	char * GetKeyMgmt( OUT uint16 &len );
    	void SetKeyMgmt( char * keyMgmt );
    	void CInfo::ClearNwKey(void);
	DH * GetDHKeyPair();
	uint8 * GetPubKey();
	uint8 * GetSHA256Hash();
	char * GetNwKey( OUT uint32 &len );
	char * GetDevPwd( OUT uint32 &len );
	bool UseUsbKey();
	bool IsRegWireless();
	bool UseUpnp();
	bool IsNwKeySet();
	uint16 GetPrimDeviceCategory();
	uint32 GetPrimDeviceOui();
	uint32 GetPrimDeviceSubCategory();
	uint32 SetPrimDeviceSubCategory(uint32 DevSubCat);

	void SetRFBand(uint8 rfBand);
	uint32 SetDHKeyPair( IN DH *p_dhKeyPair );
	uint32 SetPubKey( IN BufferObj &bo_pubKey );
	uint32 SetSHA256Hash( IN BufferObj &bo_sha256Hash );
	uint32 SetNwKey( IN char *p_nwKey, IN uint32 nwKeyLen ); 
	uint32 SetDevPwd( IN char *c_devPwd );
	uint32 SetDevPwdId( IN uint16 devPwdId );
	

	static void SetWscConfigPath(char * p_path);
	static char * GetWscConfigPath();

	static void SetAPNumRadio(uint8 numRadio);
	static uint8 GetAPNumRadio();    // Get Number of Radios in the AP
private:
	void SkipBlanksF( IN FILE *fp );
	uint32 ProcessLine( IN char *p_line );
	static char WscConfigPath[60];
	static uint8 apNumRadio;

}; //CInfo

#pragma pack( pop )
#endif // _INFO_
