/*============================================================================
//
// Copyright(c) 2006 Intel Corporation. All rights reserved.
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
// $Workfile: UPnPControlPointStructs.h
// $Revision: #1.0.2126.26696
// $Author:   Intel Corporation, Intel Device Builder
// $Date:     Tuesday, January 17, 2006
*/

#ifndef __UPNP_CONTROLPOINT_STRUCTS__
#define __UPNP_CONTROLPOINT_STRUCTS__

/*! \file ILibParsers.h 
	\brief MicroStack APIs for various functions and tasks
*/

#define UPNP_ERROR_SCPD_NOT_WELL_FORMED 5

struct UPnPDevice;
typedef void(*UPnPDeviceHandler)(struct UPnPDevice *device);

typedef enum
{
	UPnPSSDP_MSEARCH = 1,
	UPnPSSDP_NOTIFY = 2
}UPnPSSDP_MESSAGE;

/*! \struct UPnPDevice
	\brief A heirarchical representation of a device structure
*/
struct UPnPDevice
{
	int ReservedID;

	/*! \var CP
		\brief A pointer to the UPnPControlPoint that instantiated this device
	*/
	void* CP;
	/*! \var DeviceType
		\brief The Device Type URN for this device
	*/
	char* DeviceType;
	/*! \var UDN
		\brief The Unique Device Name for this device
	*/
	char* UDN;
	UPnPDeviceHandler fpDestroy;

	/*! \var LocationURL
		\brief The URI for the device description document
	*/
	char* LocationURL;
	char* IconURL;
	
	/*! \var PresentationURL
		\brief The URI where the presentation page for this device is located
	*/
	char* PresentationURL;
	/*! \var FriendlyName
		\brief The friendly name of this device
	*/
	char* FriendlyName;

	/*! \var ManufacturerName
		\brief The Manufacturer Name of the device
	*/
	char* ManufacturerName;
	/*! \var ManufacturerURL
		\brief The URI for the manufacturer of this device
	*/
	char* ManufacturerURL;

	/*! \var ModelName
		\brief The Model Name of this device
	*/
	char* ModelName;
	/*! \var ModelDescription
		\brief The Description for this model device
	*/
	char* ModelDescription;
	/*! \var ModelNumber
		\brief The model number of this device
	*/
	char* ModelNumber;
	/*! \var ModelURL
		\brief The manufacturer supplied URL for this model device
	*/
	char* ModelURL;
	
	/*! \var MaxVersion
		\brief The highest version number this device supports
	*/
	int MaxVersion;

	int SCPDError;
	int SCPDLeft;
	int ReferenceCount;
	int ReferenceTiedToEvents;

	/*! \var InterfaceToHost
		\brief The local IP address that is used to route network traffic to this device
		\para
		The IP Address is in dotted quad format. Eg: <b>192.168.0.1</b>
	*/
	char* InterfaceToHost;
	int CacheTime;
	/*! \var Tag
		\brief User modifiable pointer
	*/
	void *Tag;

	int Reserved;
	long Reserved2;
	char *Reserved3;
	void *CustomTagTable;
	
	/*! \var Parent
		\brief Pointer to the parent device. NULL if this is the root device
	*/
	struct UPnPDevice *Parent;
	/*! \var EmbeddedDevices
		\brief Pointer to the list of contained embedded devices, if they exist
	*/
	struct UPnPDevice *EmbeddedDevices;
	/*! \var Services
		\brief Pointer to the list of contained services, if they exist
	*/
	struct UPnPService *Services;
	/*! \var Next
		\brief Pointer to the next sibling device
	*/
	struct UPnPDevice *Next;
};

/*! \struct UPnPService
	\brief A logical heirarchical representation for a service
*/
struct UPnPService
{
	/*! \var MaxVersion
		\brief The highest version number this service supports
	*/
	int MaxVersion;
	/*! \var ServiceType
		\brief The Service Type URN for this service
	*/
	char* ServiceType;
	/*! \var ServiceId
		\brief The Unique Service ID for this service
	*/
	char* ServiceId;
	/*! \var ControlURL
		\brief The URL to be used to invoke methods on this device.
		\para
		<b>DO NOT</b> modify this value. It is provided for informational purposes only
	*/
	char* ControlURL;
	/*! \var SubscriptionURL
		\brief The URL to be used to subscribe for published events
		\para
		<b>DO NOT</b> modify this value. It is provided for informational purposes only
	*/
	char* SubscriptionURL;
	/*! \var SCPDURL
		\brief The URL to be used to subscribe for published events
		\para
		<b>DO NOT</b> modify this value. It is provided for informational purposes only
	*/
	char* SCPDURL;
	/*! \var SubscriptionID
		\brief The unique subscription id, obtained from the service.
		\para
		<b>DO NOT</b> modify this value. It is provided for informational purposes only
	*/
	char* SubscriptionID;
	
	/*! \var Actions
		\brief A pointer to the list of actions exposed by this service
	*/
	struct UPnPAction *Actions;
	/*! \var Variables
		\brief A pointer to the list of state variables exposed by this service
	*/
	struct UPnPStateVariable *Variables;
	/*! \var Parent
		\brief A pointer to the device that contains this service
	*/
	struct UPnPDevice *Parent;
	/*! \var Next
		\brief A pointer to the next sibling service
	*/
	struct UPnPService *Next;
};

/*! \struct UPnPStateVariable
	\brief State Variable Meta-data information
*/
struct UPnPStateVariable
{
	/*! \var Next
		\brief A pointer to the next state variable in the list
	*/
	struct UPnPStateVariable *Next;
	/*! \var Parent
		\brief A pointer to the service that contains this state variable
	*/
	struct UPnPService *Parent;
	
	/*! \var Name
		\brief The name of this state variable
	*/
	char* Name;
	 
	/*! \var AllowedValues
		\brief An array of allowed string values for this state variable
		\para
		NULL if an allowed value list is not specified for this variable.
	*/
	char **AllowedValues;
	/*! \var NumAllowedValues
		\brief The size of \a AlowedValues
	*/
	int NumAllowedValues;

	/*! \var Min
		\brief The minimum allowed value
		\para
		Only defined if one is specified. NULL otherwise
	*/
	char* Min;
	/*! \var Max
		\brief The maximum allowed value
		\para
		Only defined if one is specified. NULL otherwise
	*/
	char* Max;
	/*! \var Step
		\brief The defined stepping for the allowed range
		\para
		Only defined if one is specified. NULL otherwise
	*/
	char* Step;
};

/*! \struct UPnPAction
	\brief An object representation of actions/methods exposed by a service
*/
struct UPnPAction
{
	/*! \var Name
		\brief The name of the action/method
	*/
	char* Name;
	/*! \var Next
		\brief A pointer to the next action/method exposed
	*/
	struct UPnPAction *Next;
};

struct UPnPAllowedValue
{
	struct UPnPAllowedValue *Next;
	
	char* Value;
};

#endif
