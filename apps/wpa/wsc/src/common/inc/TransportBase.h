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
//  File: TransportBase.h
//  Description: This file contains CTransportBase class definition
//===========================================================================*/

#ifndef _TRANSPORT_BASE_H
#define _TRANSPORT_BASE_H

#include "WscTlvBase.h"
// @doc

// @module TransportBase |
//
// This module declares the abstract base class (interface) for Transport classes.    
// Each transport object implements functions for startup/shutdown, monitoring
// its particular media, and for data read and write operations.  It also sends
// asynchronous events to a callback message queue serviced by the Transport 
// Manager module. 

// @class Pure virtual base class for Transport objects
class CTransportBase
{
    //@access protected
protected:
    
    //@cmember Used for callbacks to the Transport Manager object. S_CALLBACK_INFO 
    // contains a callback method pointer and a "cookie" that is passed back to 
    // the callback method.  The cookie is typically used to pass an object 
    // pointer so that a static callback method can be dispatched to an instance
    // of the class receiving the callback.
    S_CALLBACK_INFO m_trCallbackInfo; 

    bool m_initialized; //@cmember Indicates if this Transport object has been initialized.

public:
    //@access public

    //@cmember Constructor.
    CTransportBase() : m_initialized(false) { 
        m_trCallbackInfo.pf_callback = NULL; 
        m_trCallbackInfo.p_cookie = NULL;
    }; 

    //@cmember Destructor.  Calls Deinit() if it has not already been called.
    virtual ~CTransportBase() { };

    //@cmember Sets the callback pointer and cookie in m_trCallbackInfo.
    uint32 SetTrCallback(IN CALLBACK_FN p_trCallbackFn, IN void* cookie) {
        m_trCallbackInfo.pf_callback = p_trCallbackFn;
        m_trCallbackInfo.p_cookie = cookie;
        return WSC_SUCCESS;
    };

    //@cmember Initializes the transport object, preparing it for use. This method
    // should be called immediately after the Constructor.
    virtual uint32 Init() = 0;

    //@cmember Causes the transport object to begin monitoring its media. StartMonitor
    // and StopMonitor turn on and off, respectively, incoming 
    // data and connection notifications that are passed up to the
    // Transport manager via the callback.
    virtual uint32 StartMonitor() = 0;

    //@cmember Causes the transport object to stop monitoring its media. StartMonitor
    // and StopMonitor turn on and off, respectively, incoming 
    // data and connection notifications that are passed up to the
    // Transport manager via the callback.
    virtual uint32 StopMonitor() = 0;

    //@cmember Sends data to the transport media.  It is permitted to call 
    // WriteData even after StopMonitor has been called.
    virtual uint32 WriteData(char * dataBuffer, uint32 dataLen) = 0;

    //@cmember Blocking read on data from the transport media.  This
    // function is not used much, because most transports deliver incoming
    // data via an asynchronous callback to the Transport manager.  It is 
    // permitted to call ReadData even after StopMonitor has been called.
    virtual uint32 ReadData(char * dataBuffer, uint32 * dataLen) = 0;

    virtual uint32 ReadOobData(EOobDataType type, uint8 *macAddr, 
			BufferObj &buff)
	{
		return WSC_ERR_NOT_IMPLEMENTED;
	}

    virtual uint32 WriteOobData(EOobDataType type, uint8 *macAddr, 
			BufferObj &buff)
	{
		return WSC_ERR_NOT_IMPLEMENTED;
	}


    //@cmember Shuts down the transport object.  This method is automatically called
    // by the Destructor if it has not already been called.
    virtual uint32 Deinit() = 0;

}; // CTransportBase


#endif // _TRANSPORT_BASE_H

