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
// $Workfile: UPnPControlPoint.h
// $Revision: #1.0.2126.26696
// $Author:   Intel Corporation, Intel Device Builder
// $Date:     Tuesday, January 17, 2006
*/

#ifndef __UPnPControlPoint__
#define __UPnPControlPoint__

#include "UPnPControlPointStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \file UPnPControlPoint.h 
	\brief MicroStack APIs for Control Point Implementation
*/

/*! \defgroup ControlPoint Control Point Module
	\{
*/


/* Complex Type Parsers */


/* Complex Type Serializers */




/*! \defgroup CPReferenceCounter Reference Counter Methods
	\ingroup ControlPoint
	\brief Reference Counting for the UPnPDevice and UPnPService objects.
	\para
	Whenever a user application is going to keep the pointers to the UPnPDevice object that is obtained from
	the add sink (or any pointers inside them), the application <b>must</b> increment the reference counter. Failure to do so
	will lead to references to invalid pointers, when the device leaves the network.
	\{
*/
void UPnPAddRef(struct UPnPDevice *device);
void UPnPRelease(struct UPnPDevice *device);
/*! \} */   



struct UPnPDevice* UPnPGetDevice1(struct UPnPDevice *device,int index);
int UPnPGetDeviceCount(struct UPnPDevice *device);
struct UPnPDevice* UPnPGetDeviceEx(struct UPnPDevice *device, char* DeviceType, int start,int number);
void PrintUPnPDevice(int indents, struct UPnPDevice *device);





/*! \defgroup CPAdministration Administrative Methods
	\ingroup ControlPoint
	\brief Basic administrative functions, used to setup/configure the control point application
	\{
*/
void *UPnPCreateControlPoint(void *Chain, void(*A)(struct UPnPDevice*),void(*R)(struct UPnPDevice*));
struct UPnPDevice* UPnPGetDeviceAtUDN(void *v_CP,char* UDN);
void UPnP_CP_IPAddressListChanged(void *CPToken);
int UPnPHasAction(struct UPnPService *s, char* action);
void UPnPUnSubscribeUPnPEvents(struct UPnPService *service);
void UPnPSubscribeForUPnPEvents(struct UPnPService *service, void(*callbackPtr)(struct UPnPService* service,int OK));
struct UPnPService *UPnPGetService(struct UPnPDevice *device, char* ServiceName, int length);

void UPnPSetUser(void *token, void *user);
void* UPnPGetUser(void *token);

struct UPnPService *UPnPGetService_WFAWLANConfig(struct UPnPDevice *device);

/*! \} */


/*! \defgroup InvocationEventingMethods Invocation/Eventing Methods
	\ingroup ControlPoint
	\brief Methods used to invoke actions and receive events from a UPnPService
	\{
*/
extern void (*UPnPEventCallback_WFAWLANConfig_STAStatus)(struct UPnPService* Service,unsigned char STAStatus);
extern void (*UPnPEventCallback_WFAWLANConfig_APStatus)(struct UPnPService* Service,unsigned char APStatus);
extern void (*UPnPEventCallback_WFAWLANConfig_WLANEvent)(struct UPnPService* Service,unsigned char* WLANEvent, int WLANEventLength);

void UPnPInvoke_WFAWLANConfig_DelAPSettings(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, unsigned char* NewAPSettings, int NewAPSettingsLength);
void UPnPInvoke_WFAWLANConfig_DelSTASettings(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, unsigned char* NewSTASettings, int NewSTASettingsLength);
void UPnPInvoke_WFAWLANConfig_GetAPSettings(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,unsigned char* NewAPSettings,int NewAPSettingsLength),void* _user, unsigned char* NewMessage, int NewMessageLength);
void UPnPInvoke_WFAWLANConfig_GetDeviceInfo(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,unsigned char* NewDeviceInfo,int NewDeviceInfoLength),void* _user);
void UPnPInvoke_WFAWLANConfig_GetSTASettings(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,unsigned char* NewSTASettings,int NewSTASettingsLength),void* _user, unsigned char* NewMessage, int NewMessageLength);
void UPnPInvoke_WFAWLANConfig_PutMessage(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,unsigned char* NewOutMessage,int NewOutMessageLength),void* _user, unsigned char* NewInMessage, int NewInMessageLength);
void UPnPInvoke_WFAWLANConfig_PutWLANResponse(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, unsigned char* NewMessage, int NewMessageLength, unsigned char NewWLANEventType, char* unescaped_NewWLANEventMAC);
void UPnPInvoke_WFAWLANConfig_RebootAP(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, unsigned char* NewAPSettings, int NewAPSettingsLength);
void UPnPInvoke_WFAWLANConfig_RebootSTA(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, unsigned char* NewSTASettings, int NewSTASettingsLength);
void UPnPInvoke_WFAWLANConfig_ResetAP(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, unsigned char* NewMessage, int NewMessageLength);
void UPnPInvoke_WFAWLANConfig_ResetSTA(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, unsigned char* NewMessage, int NewMessageLength);
void UPnPInvoke_WFAWLANConfig_SetAPSettings(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, unsigned char* APSettings, int APSettingsLength);
void UPnPInvoke_WFAWLANConfig_SetSelectedRegistrar(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, unsigned char* NewMessage, int NewMessageLength);
void UPnPInvoke_WFAWLANConfig_SetSTASettings(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,unsigned char* NewSTASettings,int NewSTASettingsLength),void* _user);

/*! \} */


/*! \} */

#ifdef __cplusplus
}
#endif

#endif
