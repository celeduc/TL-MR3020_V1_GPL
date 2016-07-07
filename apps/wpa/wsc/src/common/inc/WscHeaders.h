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
//  File Name: WscHeaders.h
//  Description: Definition of WSC-specific headers used in Beacons, Probe 
                 Requests, and Probe Responses. 
//
****************************************************************************/

#ifndef _WSC_HEADERS_
#define _WSC_HEADERS_

#include "WscTypes.h"
#include "slist.h"
#include "WscTlvBase.h"

//Include the following until we figure out where to put the beacons
#include "RegProtoTlv.h"

#pragma pack(push, 1)

#define WSC_VERSION                0x10
// Beacon Info
typedef struct
{
    CTlvVersion			version;
    CTlvScState			scState;
    CTlvAPSetupLocked	apSetupLocked;
    CTlvSelRegistrar    selRegistrar;
    CTlvDevicePwdId     pwdId;
	CTlvSelRegCfgMethods  selRegConfigMethods;
} __attribute__ ((__packed__)) WSC_BEACON_IE;

// Probe Request Info
typedef struct {
    CTlvVersion           version;
    CTlvReqType           reqType;
    CTlvConfigMethods     confMethods;
    CTlvUuid              uuid;
    CTlvPrimDeviceType    primDevType;
    CTlvRfBand            rfBand;
    CTlvAssocState        assocState;
    CTlvConfigError       confErr;
    CTlvDevicePwdId       pwdId;
	CTlvPortableDevice	  portableDevice;
    CTlvVendorExt         vendExt;
} __attribute__ ((__packed__)) WSC_PROBE_REQUEST_IE;

// Probe Response Info
typedef struct {
    CTlvVersion           version;
    CTlvRespType          respType;
    CTlvUuid              uuid;
	CTlvScState			  scState;
    CTlvManufacturer      manuf;
    CTlvModelName         modelName;
    CTlvModelNumber       modelNumber;
    CTlvSerialNum         serialNumber;
    CTlvPrimDeviceType    primDevType;
    CTlvDeviceName        devName;
	CTlvConfigMethods     confMethods;
    CTlvAPSetupLocked	  apSetupLocked;
    CTlvSelRegistrar      selRegistrar;
    CTlvDevicePwdId       pwdId;
	CTlvSelRegCfgMethods  selRegConfigMethods;
    CTlvVendorExt         vendExt;
} __attribute__ ((__packed__)) WSC_PROBE_RESPONSE_IE;

#pragma pack(pop)

#endif // _WSC_HEADERS_
