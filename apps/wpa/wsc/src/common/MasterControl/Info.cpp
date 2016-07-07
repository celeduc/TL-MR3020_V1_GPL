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
//  File Name: Info.cpp
//  Description: Helper calls for MasterControl. Read from and write to the
//        config file, and provide config file info to MasterControl.
//
****************************************************************************/

#ifdef WIN32
#include <stdio.h>
#include <windows.h>
#endif // WIN32

#ifdef __linux__
#include <string.h>    // for memset
#include <ctype.h>
#include <stdlib.h>
#endif

#include "WscHeaders.h"
#include "WscCommon.h"
#include "WscError.h"
#include "Info.h"
#include "Portability.h"
#include "tutrace.h"
#include "UdpLib.h"

// ****************************
// public methods
// ****************************

char CInfo::WscConfigPath[60]="wsc_config.txt";
uint8 CInfo::apNumRadio=2;

void CInfo::SetWscConfigPath(char * p_path)
{
	TUTRACE((TUTRACE_DBG, "Info::SetWscConfigPath: called\n"));
	strncpy(WscConfigPath,p_path, strlen(p_path));
	WscConfigPath[strlen(p_path)] = 0;
}

char * CInfo::GetWscConfigPath(void)
{
	TUTRACE((TUTRACE_DBG, "Info::GetWscConfigPath: called\n"));
	return WscConfigPath;
}

void CInfo::SetAPNumRadio(uint8 numRadio)
{
	TUTRACE((TUTRACE_DBG, "Info::SetAPNumRadio: called\n"));
	apNumRadio = numRadio;
}

uint8 CInfo::GetAPNumRadio(void)
{
	TUTRACE((TUTRACE_DBG, "Info::GetAPNumRadio: called\n"));
	return apNumRadio;
}

/*
 * Name        : CInfo
 * Description : Class constructor.
 * Arguments   : none
 * Return type : none
 */
CInfo::CInfo()
{
    if ( WSC_SUCCESS != WscSyncCreate( &mh_lock ))
    {
        TUTRACE((TUTRACE_ERR, "Info::Info: Could not create lock\n"));
        throw "Info::Info: Could not create lock";
    }
    mp_deviceInfo = new S_DEVICE_INFO;
    if ( !mp_deviceInfo )
    {
        TUTRACE((TUTRACE_ERR, "Info::Info: Could not create deviceInfo\n"));
        throw "Info::Info: Could not create mp_deviceInfo";
    }

	// Initialize other member variables
    memset( mp_deviceInfo, 0, sizeof(S_DEVICE_INFO) );
    mb_infoConfigSet	= false;
    mb_useUsbKey		= false;
    mb_regWireless		= false;
    mb_useUpnp			= false;
    mb_nwKeySet			= false;
    m_nwKeyLen			= 0;
    mp_dhKeyPair		= NULL;
    mcp_devPwd			= NULL;
    memset( m_pubKey, 0, SIZE_PUB_KEY );
    memset( m_sha256Hash, 0, SIZE_256_BITS );
// modified by chenyan, 20071116
#ifdef _TUDEBUGTRACE
    m_dbgLevel			= TUTRACELEVEL;
#else
    m_dbgLevel			= 0;
#endif
} // Constructor

/*
 * Name        : ~CInfo
 * Description : Class destructor.
 * Arguments   : none
 * Return type : none
 */
CInfo::~CInfo()
{
    WscSyncDestroy( mh_lock );
	if ( mcp_devPwd )
		delete [] mcp_devPwd;
    if ( mp_deviceInfo )
        delete mp_deviceInfo;
	// Don't delete mp_dhKeyPair - deleted by StateMachine
} // Destructor

/*
 * Name        : ReadConfigFile
 * Description : Read the configuration file.
 * Arguments   : none
 * Return type : uint32 - result of the read operation
 */
uint32
CInfo::ReadConfigFile()
{
    // read in config file
    FILE *fp;
    char line[100];
    uint32 ret = WSC_SUCCESS;

    strcpy(line,CInfo::WscConfigPath);
    strcat(line, WSC_CFG_FILE_NAME);
    TUTRACE((TUTRACE_INFO, "Read WSC Config File:%s\n", line));

    fp = fopen( line, "r" );
    if ( !fp )
    {
        // config file open failed
        TUTRACE((TUTRACE_ERR, "Info::ReadConfigFile: File open failed\n"));

        // TODO: if config file open failed,
        // create default config file, and init config data structure
        ret = MC_ERR_CFGFILE_OPEN;
    }
    else
    {
        // config file open ok
        TUTRACE((TUTRACE_DBG, "Info::ReadConfigFile:File open ok\n"));

	// Optimistically set mb_infoConfigSet to true
	// Will be set to false if an error is encountered
	mb_infoConfigSet = true;
	mb_nwKeySet = false;

        while ( !feof(fp) )
        {
            SkipBlanksF( fp );
            if ( feof(fp) )
                break;
            fscanf( fp, "%[^\n]", line );
	    if ( WSC_SUCCESS != ProcessLine( line ) ) {
		mb_infoConfigSet = false;
		ret = MC_ERR_CFGFILE_CONTENT;
		break; // out of while loop
	    }
        } // while

        fclose( fp );

	// Set some other variables in deviceInfo
	mp_deviceInfo->assocState = WSC_ASSOC_NOT_ASSOCIATED;
	mp_deviceInfo->configError = 0; // No error
	mp_deviceInfo->devPwdId = WSC_DEVICEPWDID_DEFAULT;
    }
    return ret;
} // ReadConfigFile

/*
 * Name        : WriteConfigFile
 * Description : Write the locally stored configuration file.
 * Arguments   : none
 * Return type : uint32 - result of the write operation
 */
uint32
CInfo::WriteConfigFile()
{
    FILE *fp;
    char line[100];

    strcpy(line,CInfo::WscConfigPath);
    strcat(line, WSC_CFG_FILE_NAME);
    fp = fopen( line, "w" );
    if ( !fp )
    {
        // config file open failed
        TUTRACE((TUTRACE_ERR, "Info::WriteConfigFile: File open failed\n"));
        return MC_ERR_CFGFILE_OPEN;
    }

    // config file open ok
    // write
    strcpy( line, "# Simple Config Configuration File\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Lines that start with # are treated as comments\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Each line should not exceed 80 characters\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Format: TYPE=value\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "#\n" );
    fprintf( fp, "%s", line );
    strcpy( line,
        "# Configured Mode: 1=Unconfigured AP, 2=Client, 3=Registrar,\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# 4=AP with Proxy, 5 = AP with Proxy and Registrar\n" );
    fprintf( fp, "%s", line );
    if(e_mode == EModeUnconfAp)
       sprintf( line, "CONFIGURED_MODE=%d\n", 1);
    else
       sprintf( line, "CONFIGURED_MODE=%d\n", 5);
    fprintf( fp, "%s", line );
    strcpy( line, "# Is the standalone Registrar (mode 3) wireless-enabled\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Yes: 1, No:0\n" );
    fprintf( fp, "%s", line );
    sprintf( line, "REGISTRAR_WIRELESS=%d\n", mb_regWireless );
    fprintf( fp, "%s", line );
    strcpy( line, "# Should UPnP be used (for modes 1 and 3)\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Yes: 1, No:0\n" );
    fprintf( fp, "%s", line );
    sprintf( line, "USE_UPNP=%d\n", mb_useUpnp );
    fprintf( fp, "%s", line );
    sprintf( line,
     "UUID=0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
        mp_deviceInfo->uuid[0], mp_deviceInfo->uuid[1], mp_deviceInfo->uuid[2],
        mp_deviceInfo->uuid[3], mp_deviceInfo->uuid[4], mp_deviceInfo->uuid[5],
        mp_deviceInfo->uuid[6], mp_deviceInfo->uuid[7], mp_deviceInfo->uuid[8],
        mp_deviceInfo->uuid[9], mp_deviceInfo->uuid[10], mp_deviceInfo->uuid[11],
        mp_deviceInfo->uuid[12], mp_deviceInfo->uuid[13], mp_deviceInfo->uuid[14],
        mp_deviceInfo->uuid[15] );
    fprintf( fp, "%s", line );
    sprintf( line, "VERSION=0x%x\n", mp_deviceInfo->version );
    fprintf( fp, "%s", line );
    sprintf( line, "DEVICE_NAME=%s\n", mp_deviceInfo->deviceName );
    fprintf( fp, "%s", line );
    strcpy( line, "# Primary Device Categories: Please refer to the SC spec for\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# values for the following types\n" );
    fprintf( fp, "%s", line );
    sprintf( line, "PRI_DEV_CATEGORY=%d\n", mp_deviceInfo->primDeviceCategory );
    fprintf( fp, "%s", line );
    sprintf( line, "PRI_DEV_OUI=0x%x\n", mp_deviceInfo->primDeviceOui );
    fprintf( fp, "%s", line );
    sprintf( line, "PRI_DEV_SUB_CATEGORY=%d\n", mp_deviceInfo->primDeviceSubCategory );
    fprintf( fp, "%s", line );
    strcpy( line, "# MAC Address of the local device, 6 byte value\n" );
    fprintf( fp, "%s", line );
    sprintf( line, "MAC_ADDRESS=0x%02x%02x%02x%02x%02x%02x\n",
		mp_deviceInfo->macAddr[0], mp_deviceInfo->macAddr[1],
		mp_deviceInfo->macAddr[2], mp_deviceInfo->macAddr[3],
		mp_deviceInfo->macAddr[4], mp_deviceInfo->macAddr[5] );
    fprintf( fp, "%s", line );
    sprintf( line, "MANUFACTURER=%s\n", mp_deviceInfo->manufacturer );
    fprintf( fp, "%s", line );
    sprintf( line, "MODEL_NAME=%s\n", mp_deviceInfo->modelName );
    fprintf( fp, "%s", line );
    sprintf( line, "MODEL_NUMBER=%s\n", mp_deviceInfo->modelNumber );
    fprintf( fp, "%s", line );
    sprintf( line, "SERIAL_NUMBER=%s\n", mp_deviceInfo->serialNumber );
    fprintf( fp, "%s", line );
    strcpy( line, "# Config Methods: bitwise OR of values \n" );
    fprintf( fp, "%s", line );
    sprintf( line, "CONFIG_METHODS=0x%x\n", mp_deviceInfo->configMethods );
    fprintf( fp, "%s", line );
    strcpy( line, "# Auth type flags: bitwise OR of values \n" );
    fprintf( fp, "%s", line );
    sprintf( line, "AUTH_TYPE_FLAGS=0x%x\n", mp_deviceInfo->authTypeFlags );
    fprintf( fp, "%s", line );
    strcpy( line, "# Encr type flags: bitwise OR of values \n" );
    fprintf( fp, "%s", line );
    sprintf( line, "ENCR_TYPE_FLAGS=0x%x\n", mp_deviceInfo->encrTypeFlags );
    fprintf( fp, "%s", line );
    sprintf( line, "CONN_TYPE_FLAGS=0x%x\n", mp_deviceInfo->connTypeFlags );
    fprintf( fp, "%s", line );
    sprintf( line, "RF_BAND=%d\n", mp_deviceInfo->rfBand );
    fprintf( fp, "%s", line );
    sprintf( line, "OS_VER=0x%x\n", mp_deviceInfo->osVersion );
    fprintf( fp, "%s", line );
    sprintf( line, "FEATURE_ID=0x%x\n", mp_deviceInfo->featureId );
    fprintf( fp, "%s", line );
    strcpy( line, "# SSID:\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# For unconfigured client: What it should connect "
                    "to when\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# starting EAP-WSC\n" );
    fprintf( fp, "%s", line );
	strcpy( line, "# Example: SSID=WscSecureAP\n" );
	fprintf( fp, "%s", line );
    strcpy( line, "# For unconfigured AP: Initial broadcast SSID\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Example: SSID=WscNewAP\n" );
	fprintf( fp, "%s", line );
	strcpy( line, "# For Registrar: SSID that the supplicant must connect "
                    "to when\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# starting EAP-WSC\n" );
	fprintf( fp, "%s", line );
	strcpy( line, "# Example: SSID=WscNewAP\n" );
	fprintf( fp, "%s", line );
    strcpy( line, "# For AP with Registrar: Broadcast SSID\n" );
    fprintf( fp, "%s", line );
	strcpy( line, "# Example: SSID=WscSecureAP\n" );
	fprintf( fp, "%s", line );
    sprintf( line, "SSID=%s\n", mp_deviceInfo->ssid );
    fprintf( fp, "%s", line );
    strcpy( line, "# Key Mgmt for Supplicant (Client, Registrar):\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Unconfigured, doing WSC: WPA-EAP IEEE8021X\n" );
    fprintf( fp, "%s", line );
    strcpy( line,
        "# Configured after WSC (will be done by the s/w): WPA-PSK\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Key Mgmt for Hostapd (AP, AP with Registrar):\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Unconfigured, doing WSC: WPA-EAP\n" );
    fprintf( fp, "%s", line );
    strcpy( line,
        "# Configured after WSC (will be done by the s/w): WPA-PSK\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Configured, plus Registrar: WPA-EAP WPA-PSK\n" );
    fprintf( fp, "%s", line );
    sprintf( line, "KEY_MGMT=%s\n", mp_deviceInfo->keyMgmt );
    //sprintf( line, "KEY_MGMT=%s\n", "WPA-EAP WPA-PSK");
    fprintf( fp, "%s", line );
    strcpy( line, "# Are we using a USB key to transfer PIN/Credential?\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Yes: 1, No:0\n" );
    fprintf( fp, "%s", line );
    sprintf( line, "USB_KEY=%d\n", mb_useUsbKey );
    fprintf( fp, "%s", line );
    strcpy( line, "# Is the Network Key set?\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# Yes: 0xValue or passphrase, No: comment out line\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# NW_KEY=0x000102030405060708090A0B0C0D0E0F000102030405060708090A0B0C0D0E0F\n" );
    fprintf( fp, "%s", line );
    strcpy( line, "# NW_KEY=passphrase\n" );
    fprintf( fp, "%s", line );
    TUTRACE((TUTRACE_DBG, "mb_nwKeySet = %d\n", mb_nwKeySet));
    if ( mb_nwKeySet )
    {
	sprintf( line, "NW_KEY=" );
	if ( m_nwKeyLen == 64 )
	{
            strcat( line, "0x" );
	}
        TUTRACE((TUTRACE_DBG, "m_nwKey = %s\n", m_nwKey));
        strncat( line, m_nwKey, m_nwKeyLen );
	fprintf( fp, "%s\n", line );
    }
    strcpy( line, "# DBG_LEVEL bit mask: 0:ERR, 1:INFO, 2:REG, 3:UPNP, 4:MC, 16:DBG\n" );
    fprintf( fp, "%s", line );
    sprintf( line, "DBG_LEVEL=%d\n", m_dbgLevel);
    fprintf( fp, "%s", line );
	//sep,11,08,add by weizhengqin
	strcpy( line, "# Auth type : specific one of values \n" );
    fprintf( fp, "%s", line );
    sprintf( line, "AUTH_TYPE=0x%x\n", mp_deviceInfo->authType);
    fprintf( fp, "%s", line );
    strcpy( line, "# Encr types: specific one  of values \n" );
    fprintf( fp, "%s", line );
    sprintf( line, "ENCR_TYPE=0x%x\n", mp_deviceInfo->encrType );
	fprintf( fp, "%s", line );
	//
    fclose( fp );
    return WSC_SUCCESS;
} // WriteConfigFile

/*
 * Name        : GetDeviceInfo
 * Description : Return the config data structure.
 * Arguments   : none
 * Return type : S_DEVICE_INFO * - pointer to the info data structure
 */
S_DEVICE_INFO *
CInfo::GetDeviceInfo()
{
    return mp_deviceInfo;
} // GetDeviceInfo

uint32 *
CInfo::GetLock()
{
    return mh_lock;
} // GetLock

/*
 * Name        : GetConfiguredMode
 * Description : Determine if the device is configured to be a
 *                 registrar or enrollee
 * Arguments   : none
 * Return type : EMode - configured mode of the device
 */
EMode
CInfo::GetConfiguredMode()
{
    return e_mode;
} // GetConfiguredMode

void
CInfo::SetConfiguredMode(EMode mode)
{
    e_mode = mode;

    TUTRACE((TUTRACE_DBG, "Set device simple config state %d.\r\n",
                mp_deviceInfo->scState));
    if(mode == EModeUnconfAp)
        mp_deviceInfo->scState = 0x01; // Unconfigured
    else
        mp_deviceInfo->scState = 0x02; // Configured

} // SetConfiguredMode

/*
 * Name        : IsInfoConfigSet
 * Description : Determine if the config info has been set.
 * Arguments   : none
 * Return type : bool - true if yes, false if no
 */
bool
CInfo::IsInfoConfigSet()
{
    return mb_infoConfigSet;
} // IsInfoConfigSet

/*
 * Name        : GetUUID
 * Description : Return a pointer to the UUID array
 * Arguments   : none
 * Return type : uint8 * - pointer to the UUID array
 */
uint8 *
CInfo::GetUUID()
{
    return mp_deviceInfo->uuid;
} // GetUUID

/*
void
CInfo::GetMacAddr( uint8(&macAddr)[SIZE_6_BYTES] )
{
    strncpy( (char *)macAddr,
            (char *)mp_deviceInfo->macAddr, SIZE_6_BYTES );
    return;
} // GetMacAddr
*/
uint8 *
CInfo::GetMacAddr()
{
	return mp_deviceInfo->macAddr;
} // GetMacAddr

/*
 * Name        : GetVersion
 * Description : Return the WSC Version that has been configured
 * Arguments   : none
 * Return type : uint8 - version
 */
uint8
CInfo::GetVersion()
{
    return mp_deviceInfo->version;
} // GetVersion

/*
 * Name        : GetDeviceName
 * Description : Return a pointer to deviceName. Also set the length
 *                 of the fielld in len.
 * Arguments   : OUT uint16 &len - store length of deviceName in this field
 * Return type : char * - pointer to deviceName
 */
char *
CInfo::GetDeviceName( OUT uint16 &len )
{
    // len = strlen + terminating NULL
    len = (uint16) strlen( mp_deviceInfo->deviceName );
    return mp_deviceInfo->deviceName;
} // GetDeviceName
void
CInfo::SetDeviceName( char * deviceName )
{
    strcpy(mp_deviceInfo->deviceName,deviceName);
    mp_deviceInfo->deviceName[strlen(deviceName)]=0;
} // GetDeviceName

/*
 * Name        : GetDeviceType
 * Description : Return a pointer to deviceType. Also set the length
 *                 of the field in len.
 * Arguments   : OUT uint16 &len - store length of deviceType in this field
 * Return type : char * - pointer to deviceType
 */
char *
CInfo::GetDeviceType( OUT uint16 &len )
{
    // len = strlen + terminating NULL
    // len = (uint16) strlen( mp_deviceInfo->deviceType );
    // return mp_deviceInfo->deviceType;
	// RSNARJAL - to be fixed
	return NULL;
} // GetDeviceType

/*
 * Name        : GetManufacturer
 * Description : Return a pointer to manufacturer. Also set the length
 *                 of the field in len.
 * Arguments   : OUT uint16 &len - store length of manufacturer in this field
 * Return type : char * - pointer to manufacturer
 */
char *
CInfo::GetManufacturer( OUT uint16 &len )
{
    // len = strlen + terminating NULL
    len = (uint16) strlen( mp_deviceInfo->manufacturer );
    return mp_deviceInfo->manufacturer;
} // GetManufacturer

/*
 * Name        : GetModelName
 * Description : Return a pointer to modelName. Also set the length
 *                 of the field in len.
 * Arguments   : OUT uint16 &len - store length of modelName in this field
 * Return type : char * - pointer to modelName
 */
char *
CInfo::GetModelName( OUT uint16 &len )
{
    // len = strlen + terminating NULL
    len = (uint16) strlen( mp_deviceInfo->modelName );
    return mp_deviceInfo->modelName;
} // GetModelName

/*
 * Name        : GetModelNumber
 * Description : Return a pointer to modelNumber. Also set the length
 *                 of the field in len.
 * Arguments   : OUT uint16 &len - store length of modelNumber in this field
 * Return type : char * - pointer to modelNumber
 */
char *
CInfo::GetModelNumber( OUT uint16 &len )
{
    // len = strlen + terminating NULL
    len = (uint16) strlen( mp_deviceInfo->modelNumber );
    return mp_deviceInfo->modelNumber;
} // GetModelNumber

/*
 * Name        : GetSerialNumber
 * Description : Return a pointer to serialNumber. Also set the length
 *                 of the field in len.
 * Arguments   : OUT uint16 &len - store length of serialNumber in this field
 * Return type : char * - pointer to serialNumber
 */
char *
CInfo::GetSerialNumber( OUT uint16 &len )
{
    // len = strlen + terminating NULL
    len = (uint16) strlen( mp_deviceInfo->serialNumber );
    return mp_deviceInfo->serialNumber;
} // GetSerialNumber

/*
 * Name        : GetConfigMethods
 * Description : Return the ConfigMethods that has been configured
 * Arguments   : none
 * Return type : uint16 - value of configMethods
 */
uint16
CInfo::GetConfigMethods()
{
    return mp_deviceInfo->configMethods;
} // GetConfigMethods

/*
 * Name        : GetAuthTypeFlags
 * Description : Return the AuthTypeFlags that has been configured
 * Arguments   : none
 * Return type : uint16 - value of authTypeFlags
 */
uint16
CInfo::GetAuthTypeFlags()
{
    return mp_deviceInfo->authTypeFlags;
} // GetAuthTypeFlags()
void
CInfo::SetAuthTypeFlags(uint16 authType)
{
    mp_deviceInfo->authTypeFlags = authType;
}
/*
 * Name        : GetEncrTypeFlags
 * Description : Return the EncrTypeFlags that has been configured
 * Arguments   : none
 * Return type : uint16 - value of encrTypeFlags
 */
uint16
CInfo::GetEncrTypeFlags()
{
    return mp_deviceInfo->encrTypeFlags;
} // GetEncrTypeFlags

void
CInfo::SetEncrTypeFlags(uint16 encrTypeFlags)
{
    mp_deviceInfo->encrTypeFlags = encrTypeFlags;
} // SetEncrTypeFlags

/*
 * Name        : GetConnTypeFlags
 * Description : Return the connTypeFlags that has been configured
 * Arguments   : none
 * Return type : uint8 - value of connTypeFlags
 */
uint8
CInfo::GetConnTypeFlags()
{
    return mp_deviceInfo->connTypeFlags;
} // GetConnTypeFlags

//sep,11,08,add by weizhengqin

/*
 * Name        : GetAuthType
 * Description : Return the AuthType that has been configured
 * Arguments   : none
 * Return type : uint16 - value of authType
 */
uint16
CInfo::GetAuthType()
{
    return mp_deviceInfo->authType;
} // GetAuthType()

void
CInfo::SetAuthType(uint16 authType)
{
    mp_deviceInfo->authType = authType;
}
/*
 * Name        : GetEncrType
 * Description : Return the EncrType that has been configured
 * Arguments   : none
 * Return type : uint16 - value of encrType
 */
uint16
CInfo::GetEncrType()
{
    return mp_deviceInfo->encrType;
} // GetEncrType

void
CInfo::SetEncrType(uint16 encrType)
{
    mp_deviceInfo->encrType = encrType;
} // SetEncrType

//end

/*
 * Name        : GetRFBand
 * Description : Return the value of rfBand
 * Arguments   : none
 * Return type : uint8 - value of rfBand
 */
uint8
CInfo::GetRFBand()
{
    return mp_deviceInfo->rfBand;
} // GetRFBand

/*
 * Name        : SetRFBand
 * Description : Set the value of rfBand
 * Arguments   : none
 * Return type : uint8 - value of rfBand
 */
void
CInfo::SetRFBand(uint8 rfBand)
{
    mp_deviceInfo->rfBand = rfBand;
} // SetRFBand

/*
 * Name        : GetOsVersion
 * Description : Return the osVersion that has been configured
 * Arguments   : none
 * Return type : uint32 - value of osVersion
 */
uint32
CInfo::GetOsVersion()
{
    return mp_deviceInfo->osVersion;
}// GetOsVersion

/*
 * Name        : GetFeatureId
 * Description : Return the featureId that has been configured
 * Arguments   : none
 * Return type : uint32 - value of featureId
 */
uint32
CInfo::GetFeatureId()
{
    return mp_deviceInfo->featureId;
} // GetFeatureId

/*
 * Name        : GetAssocState
 * Description : Return the assocState that has been configured
 * Arguments   : none
 * Return type : uint16 - value of assocState
 */
uint16
CInfo::GetAssocState()
{
    return mp_deviceInfo->assocState;
} // GetAssocState

/*
 * Name        : GetDevicePwdId
 * Description : Return the devicePwdId that has been configured
 * Arguments   : none
 * Return type : uint16 - value of devicePwdId
 */
uint16
CInfo::GetDevicePwdId()
{
    return mp_deviceInfo->devPwdId;
} // GetDevicePwdId

/*
 * Name        : GetConfigError
 * Description : Return the configError that has been configured
 * Arguments   : none
 * Return type : uint16 - value of configError
 */
uint16
CInfo::GetConfigError()
{
    return mp_deviceInfo->configError;
} // GetConfigError

/*
 * Name        : IsAP
 * Description : Return the value of b_ap
 * Arguments   : none
 * Return type : bool - value of b_ap
 */
bool
CInfo::IsAP()
{
    return mp_deviceInfo->b_ap;
} // IsAP
/*
 * Name        : SetAP
 * Description : Set the value of b_ap
 * Arguments   : none
 * Return type : bool - value of b_ap
 */
void
CInfo::SetAP(bool b_ap)
{
    mp_deviceInfo->b_ap = b_ap;
} // SetAP

/*
 * Name        : GetSSID
 * Description : Return the value of SSID
 * Arguments   : uint16 &len - input to this method
 * Return type : char * - pointer to string containing the SSID
 *                 uint16& len - length of the SSID
 */
char *
CInfo::GetSSID( OUT uint16 &len )
{
    // len = strlen + terminating NULL
    len = (uint16) strlen( mp_deviceInfo->ssid );
    return mp_deviceInfo->ssid;
} // GetSSID


/*
 * Name        : SetSSID
 * Description : Set the value of SSID
 * Arguments   : char *  SSID
 * Return type : void
 */
void
CInfo::SetSSID( char * ssid )
{
    strncpy(mp_deviceInfo->ssid,ssid,strlen(ssid));
    mp_deviceInfo->ssid[strlen(ssid)]=0;
} // SetSSID

/*
 * Name        : GetKeyMgmt
 * Description : Return the value of keyMgmt
 * Arguments   : uint16 &len - input to this method
 * Return type : char * - pointer to string containing the keyMgmt value
 *                 uint16& len - length of the keyMgmt string
 */
char *
CInfo::GetKeyMgmt( OUT uint16 &len )
{
    // len = strlen + terminating NULL
    len = (uint16) strlen( mp_deviceInfo->keyMgmt );
    return mp_deviceInfo->keyMgmt;
} // GetKeyMgmt

/*
 * Name        : SetKeyMgmt
 * Description : Set the value of keyMgmt
 * Arguments   :
 * Return type :
 */
void
CInfo::SetKeyMgmt( char * keyMgmt )
{
    strcpy(mp_deviceInfo->keyMgmt,keyMgmt);
} //

/*
 * Name        : GetDHKeyPair
 * Description : Return the Diffie-Hellman key pair value
 * Arguments   : none
 * Return type : DH * - pointer to the DH key pair value
 */
DH *
CInfo::GetDHKeyPair()
{
    return mp_dhKeyPair;
} // GetDHKeyPair

/*
 * Name        : GetPubKey
 * Description : Return the Diffie-Hellman public key
 * Arguments   : none
 * Return type : uint8 * - pointer to the DH public key
 */
uint8 *
CInfo::GetPubKey()
{
    return m_pubKey;
} // GetPubKey

/*
 * Name        : GetSHA256Hash
 * Description : Return the SHA 256 hash
 * Arguments   : none
 * Return type : uint8 * - pointer to the hash
 */
uint8 *
CInfo::GetSHA256Hash()
{
    return m_sha256Hash;
} // GetSHA256Hash

/*
 * Name        : GetNwKey
 * Description : Return the nwKey
 * Arguments   : none
 * Return type : char * - pointer to the PSK
 */
char *
CInfo::GetNwKey( OUT uint32 &len )
{
	if ( mb_nwKeySet )
	{
		len = m_nwKeyLen;
		return m_nwKey;
	}
	else
	{
		TUTRACE((TUTRACE_ERR,
                "Info::GetNwKey: NwKeynot set\n"));
		len = 0;
		return NULL;
	}
} // GetNwKey

void
CInfo::ClearNwKey(void)
{
    memset(m_nwKey, 0, m_nwKeyLen);
    m_nwKeyLen = 0;
} // ClearNwKey

/*
 * Name        : GetDevPwd
 * Description : Return the device pwd
 * Arguments   : OUT uint16 &len - len of the pwd
 * Return type : char * - pointer to the pwd
 */
char *
CInfo::GetDevPwd( OUT uint32 &len )
{
	if ( mcp_devPwd )
	{
		len = (uint32) strlen( mcp_devPwd );
		return mcp_devPwd;
	}
	else
	{
		len = 0;
		return NULL;
	}
} // GetDevPwd

bool
CInfo::UseUsbKey()
{
	return mb_useUsbKey;
} // UseUsbKey

bool
CInfo::IsRegWireless()
{
	return mb_regWireless;
} // IsRegWireless

bool
CInfo::UseUpnp()
{
	return mb_useUpnp;
} // UseUpnp

bool
CInfo::IsNwKeySet()
{
	return mb_nwKeySet;
} // IsNwKeySet

uint16
CInfo::GetPrimDeviceCategory()
{
	return mp_deviceInfo->primDeviceCategory;
} // GetPrimDeviceCategory

uint32
CInfo::GetPrimDeviceOui()
{
	return mp_deviceInfo->primDeviceOui;
} // GetPrimDeviceOui

uint32
CInfo::GetPrimDeviceSubCategory()
{
	return mp_deviceInfo->primDeviceSubCategory;
} // GetPrimDeviceSubCategory

uint32
CInfo::SetPrimDeviceSubCategory(uint32 DevSubCat)
{
	mp_deviceInfo->primDeviceSubCategory = DevSubCat ;
} // SetPrimDeviceSubCategory


/*
 * Name        : SetDHKeyPair
 * Description : Set the value of the Diffie-Hellman key pair
 * Arguments   : DH *p_dhKeyPair - pointer to the DH key pair to be
 *                 taken as input
 * Return type : uint32 - result of the operation
 */
uint32
CInfo::SetDHKeyPair( IN DH *p_dhKeyPair )
{
    mp_dhKeyPair = p_dhKeyPair;
    return WSC_SUCCESS;
} // SetDHKeyPair

/*
 * Name        : SetPubKey
 * Description : Set the value of the public key
 * Arguments   : BufferObj &bo_pubKey - buffer object containing the public key
 * Return type : uint32 - result of the operation
 */
uint32
CInfo::SetPubKey( IN BufferObj &bo_pubKey )
{
	if ( SIZE_PUB_KEY == bo_pubKey.Length() )
	{
		// Copy the data over
		memcpy( m_pubKey, bo_pubKey.GetBuf(), SIZE_PUB_KEY );
		return WSC_SUCCESS;
	}
	else
	{
		return WSC_ERR_SYSTEM;
	}
} // SetPubKey

/*
 * Name        : SetSHA256Hash
 * Description : Set the value of the hash
 * Arguments   : BufferObj &bo_sha256Hash - buffer object containing the hash
 * Return type : uint32 - result of the operation
 */
uint32
CInfo::SetSHA256Hash( IN BufferObj &bo_sha256Hash )
{
	if ( SIZE_256_BITS == bo_sha256Hash.Length() )
	{
        // Copy the data over
		memcpy( m_sha256Hash, bo_sha256Hash.GetBuf(), SIZE_256_BITS );
		return WSC_SUCCESS;
	}
	else
	{
		return WSC_ERR_SYSTEM;
	}
} // SetSHA256Hash

/*
 * Name        : SetNwKey
 * Description : Set the value of the NwKey
 * Arguments   : IN char *p_nwKey - pointer to the nw Key
 *				 IN uint32 nwKeyLen - length of the key in bytes
 * Return type : uint32 - result of the operation
 */
uint32
CInfo::SetNwKey( IN char *p_nwKey, IN uint32 nwKeyLen )
{

        memset( m_nwKey, 0, SIZE_64_BYTES );

	if ( nwKeyLen > 0 && nwKeyLen <= SIZE_64_BYTES )
	{
               // Copy the data over
		memcpy( m_nwKey, p_nwKey, nwKeyLen );
	}
        m_nwKeyLen = nwKeyLen;
        mb_nwKeySet = true;
	return WSC_SUCCESS;

} // SetNwKey

/*
 * Name        : SetDevPwd
 * Description : Set the value of the dev pwd
 * Arguments   : char *c_devPwd - device password.
 *				 Must be NULL terminated.
 * Return type : uint32 - result of the operation
 */
uint32
CInfo::SetDevPwd( IN char *c_devPwd )
{
	uint32 len = (uint32) strlen( c_devPwd );
	if ( len <= 0 )
	{
		return WSC_ERR_SYSTEM;
	}
	else
	{
		if ( mcp_devPwd )
			delete [] mcp_devPwd;

		mcp_devPwd = new char[len+1];
		strncpy( mcp_devPwd, c_devPwd, len );
		mcp_devPwd[len] = '\0';
		return WSC_SUCCESS;
	}
} // SetDevPwd

uint32
CInfo::SetDevPwdId( uint16 devPwdId )
{
	if ( devPwdId == mp_deviceInfo->devPwdId )
	{
		return MC_ERR_VALUE_UNCHANGED;
	}
	else
	{
		mp_deviceInfo->devPwdId = devPwdId;
		return WSC_SUCCESS;
	}
} // SetDevPwdId

// ****************************
// private methods
// ****************************
/*
 * Name        : SkipBlanksF
 * Description : Skip blank characters in the configuration file
 * Arguments   : FILE *fp - pointer to the file to process
 * Return type : void
 */
void
CInfo::SkipBlanksF( IN FILE *fp )
{
    char c;
    while ( !feof(fp) )
    {
        if ( isspace((c=fgetc(fp))) )
            continue;
        ungetc( c, fp );
        break;
    } // while
} // SkipBlanksF

/*
 * Name        : ProcessLine
 * Description : Process a line read in from the configuration file
 * Arguments   : char *p_line - pointer to the line to process
 * Return type : uint32 - result of the process operation
 */
uint32
CInfo::ProcessLine( IN char *p_line )
{
    char *p = NULL;

    // check for comment line
    if ( '#' == *p_line )
        return WSC_SUCCESS;

    if ( strstr(p_line, "CONFIGURED_MODE"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <VERSION>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        int val = atoi( ++p );
        if ( 1 == val )
        {
            e_mode = EModeUnconfAp;
            mp_deviceInfo->b_ap = true;
			mp_deviceInfo->scState = 0x01; // Unconfigured
        }
        else if ( 2 == val )
		{
            e_mode = EModeClient;
			mp_deviceInfo->b_ap = false;
			mp_deviceInfo->scState = 0x01; // Unconfigured
		}
        else if ( 3 == val )
		{
            e_mode = EModeRegistrar;
			mp_deviceInfo->b_ap = false;
			mp_deviceInfo->scState = 0x02; // Configured
		}
        else if ( 4 == val )
		{
            e_mode = EModeApProxy;
			mp_deviceInfo->b_ap = true;
			mp_deviceInfo->scState = 0x02; // Configured
		}
		else
		{
			e_mode = EModeApProxyRegistrar;
			mp_deviceInfo->b_ap = true;
			mp_deviceInfo->scState = 0x02; // Configured
		}
    }
	else if ( strstr(p_line, "REGISTRAR_WIRELESS"))
	{
		p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <REGISTRAR_WIRELESS>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
		int val = atoi( ++p );
		mb_regWireless = ( 0==val )?false:true;
	}
	else if ( strstr(p_line, "USE_UPNP"))
	{
		p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <REGISTRAR_WIRELESS>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
		int val = atoi( ++p );
		mb_useUpnp = ( 0==val )?false:true;
	}
    else if ( strstr(p_line, "UUID"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
		{
			TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <UUID>\n"));
            return MC_ERR_CFGFILE_CONTENT;
		}
        int i;
        char temp[10];
        temp[0] = '0';
        temp[1] = 'x';
        // move past the '0x'
        p+=1;

        for ( i = 0; i <= 15; i++ )
        {
            p+=2;
            strncpy( &temp[2], p, 2 );
            mp_deviceInfo->uuid[i] = (uint8) (strtoul( temp, NULL, 16 ));
        }
    }
    else if ( strstr(p_line, "VERSION"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <VERSION>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        mp_deviceInfo->version = (uint8) (strtoul( ++p, NULL, 16 ));
    }
    else if ( strstr(p_line, "DEVICE_NAME"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <DEVICE_NAME>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        strcpy( mp_deviceInfo->deviceName, ++p );
    }
    else if ( strstr(p_line, "PRI_DEV_CATEGORY"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <PRI_DEV_CATEGORY>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
		mp_deviceInfo->primDeviceCategory = atoi( ++ p );
    }
	else if ( strstr(p_line, "PRI_DEV_OUI"))
    {
		p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <PRI_DEV_OUI>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
		mp_deviceInfo->primDeviceOui = (strtoul( ++p, NULL, 16 ));
	}
	else if ( strstr(p_line, "PRI_DEV_SUB_CATEGORY"))
    {
		p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, "
				"<PRI_DEV_SUB_CATEGORY>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
		mp_deviceInfo->primDeviceSubCategory = atoi( ++ p );
	}
	else if ( strstr(p_line, "MAC_ADDRESS"))
	{
		p = strchr( p_line, '=' );
        	if ( !p )
		{
			TUTRACE((TUTRACE_ERR,
                		"Info::ProcessLine: Error in cfg file, <MAC_ADDRESS>\n"));
            		return MC_ERR_CFGFILE_CONTENT;
		}
        	int i;
        	char temp[10];

        	temp[0] = '0';
        	temp[1] = 'x';

		// move past the '0x'
		p+=1;

		for ( i = 0; i <= 5; i++ )
		{
		    p+=2;
		    strncpy( &temp[2], p, 2 );
		    mp_deviceInfo->macAddr[i] = (uint8) (strtoul( temp, NULL, 16 ));
		}
		get_mac_address("br0", (char *)mp_deviceInfo->macAddr);
		TUTRACE((TUTRACE_INFO, "Use eth0 address %2x:%2x:%2x:%2x:%2x:%2x\n",
			mp_deviceInfo->macAddr[0], mp_deviceInfo->macAddr[1],
			mp_deviceInfo->macAddr[2], mp_deviceInfo->macAddr[3],
			mp_deviceInfo->macAddr[4], mp_deviceInfo->macAddr[5] ));

	}
    else if ( strstr(p_line, "MANUFACTURER"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <MANUFACTURER>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        strcpy( mp_deviceInfo->manufacturer, ++p );
    }
    else if ( strstr(p_line, "MODEL_NAME"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <MODEL_NAME>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        strcpy( mp_deviceInfo->modelName, ++p );
    }
    else if ( strstr(p_line, "MODEL_NUMBER"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <MODEL_NUMBER>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        strcpy( mp_deviceInfo->modelNumber, ++p );
    }
    else if ( strstr(p_line, "SERIAL_NUMBER"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <SERIAL_NUMBER>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        strcpy( mp_deviceInfo->serialNumber, ++p );
    }
    else if ( strstr(p_line, "CONFIG_METHODS"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <CONFIG_METHODS>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        mp_deviceInfo->configMethods = (uint16) (strtoul( ++p, NULL, 16 ));
    }
    else if ( strstr(p_line, "AUTH_TYPE_FLAGS"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <AUTH_TYPE_FLAGS>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        mp_deviceInfo->authTypeFlags = (uint16) (strtoul( ++p, NULL, 16 ));
    }
    else if ( strstr(p_line, "ENCR_TYPE_FLAGS"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <ENCR_TYPE_FLAGS>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        mp_deviceInfo->encrTypeFlags = (uint16) (strtoul( ++p, NULL, 16 ));
    }
    else if ( strstr(p_line, "CONN_TYPE_FLAGS"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <CONN_TYPE_FLAGS>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        mp_deviceInfo->connTypeFlags = (uint8) (strtoul( ++p, NULL, 16 ));
    }
    else if ( strstr(p_line, "RF_BAND"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <RF_BAND>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        mp_deviceInfo->rfBand = atoi( ++p );
    }
    else if ( strstr(p_line, "OS_VER"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <OS_VER>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        mp_deviceInfo->osVersion = (strtoul( ++p, NULL, 16 ));
    }
    else if ( strstr(p_line, "FEATURE_ID"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <FEATURE_ID>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        mp_deviceInfo->featureId = (strtoul( ++p, NULL, 16 ));
    }
    else if ( strstr(p_line, "SSID"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <SSID>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        strcpy( mp_deviceInfo->ssid, ++p );
    }
    else if ( strstr(p_line, "KEY_MGMT"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <KEY_MGMT>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        strcpy( mp_deviceInfo->keyMgmt, ++p );
    }
    else if ( strstr(p_line, "USB_KEY"))
    {
	p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <USB_KEY>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
	int val = atoi( ++p );
	mb_useUsbKey = ( 0==val ) ? false:true;
    }
    else if ( strstr(p_line, "NW_KEY"))
    {
	char *p2 = NULL;

       	p = strchr( p_line, '=' );
       	if ( !p ) {
		TUTRACE((TUTRACE_ERR,
               		"Info::ProcessLine: Error in cfg file, <NW_KEY>\n"));
       		return MC_ERR_CFGFILE_CONTENT;
	}

	// check if we need to move past the 0x
	p+=1;
	p2 = p+1;
	if ( '0' == *p && 'x' == *p2 )
		p+=2;

	memset( m_nwKey, 0, SIZE_64_BYTES );
	m_nwKeyLen = (uint32) strlen( p );
	strncpy( m_nwKey, p, m_nwKeyLen );

	mb_nwKeySet = true;
    }
    else if ( strstr(p_line, "DBG_LEVEL"))
    {
        p = strchr( p_line, '=' );
        if ( !p )
        {
            TUTRACE((TUTRACE_ERR,
                "Info::ProcessLine: Error in cfg file, <DBG_LEVEL>\n"));
            return MC_ERR_CFGFILE_CONTENT;
        }
        m_dbgLevel = atoi( ++p );
	PrintTraceLevelSet(m_dbgLevel);
    }
  	//sep,11,08,add by weizhengqin
  	else if(strstr(p_line,"AUTH_TYPE"))
  	{
  		p = strchr(p_line,'=');
		if(!p)
		{
			TUTRACE((TUTRACE_ERR,
                		"Info::ProcessLine: Error in cfg file, <AUTH_TYPE>\n"));
            		return MC_ERR_CFGFILE_CONTENT;
		}
		mp_deviceInfo->authType = (uint16) (strtoul( ++p, NULL, 16 ));
  	}
	else if(strstr(p_line,"ENCR_TYPE"))
  	{
  		p = strchr(p_line,'=');
		if(!p)
		{
			TUTRACE((TUTRACE_ERR,
                		"Info::ProcessLine: Error in cfg file, <ENCR_TYPE>\n"));
            		return MC_ERR_CFGFILE_CONTENT;
		}
		mp_deviceInfo->encrType = (uint16) (strtoul( ++p, NULL, 16 ));
  	}
  	//end
    return WSC_SUCCESS;
} // ProcessLine

