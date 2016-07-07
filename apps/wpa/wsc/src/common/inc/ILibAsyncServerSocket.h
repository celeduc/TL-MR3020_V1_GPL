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
// $Workfile: ILibAsyncServerSocket.h
// $Revision: #1.0.2126.26696
// $Author:   Intel Corporation, Intel Device Builder
// $Date:     Tuesday, January 17, 2006
*/

#ifndef ___ILibAsyncServerSocket___
#define ___ILibAsyncServerSocket___

/*! \file ILibAsyncServerSocket.h 
	\brief MicroStack APIs for TCP Server Functionality
*/

/*! \defgroup ILibAsyncServerSocket ILibAsyncServerSocket Module
	\{
*/

#if defined(WIN32) || defined(_WIN32_WCE)
#include <STDDEF.H>
#else
#include <malloc.h>
#endif

#include "ILibAsyncSocket.h"

/*! \typedef ILibAsyncServerSocket_ServerModule
	\brief The handle for an ILibAsyncServerSocket module
*/
typedef void* ILibAsyncServerSocket_ServerModule;

/*! \typedef ILibAsyncServerSocket_ConnectionToken
	\brief Connection state, for a connected session
*/
typedef ILibAsyncSocket_SocketModule ILibAsyncServerSocket_ConnectionToken;

/*! \typedef ILibAsyncServerSocket_BufferReAllocated
	\brief BufferReAllocation Handler
	\param AsyncServerSocketToken The ILibAsyncServerSocket token
	\param ConnectionToken The ILibAsyncServerSocket_Connection token
	\param user The User object
	\param newOffset The buffer has shifted by this offset
*/
typedef void (*ILibAsyncServerSocket_BufferReAllocated)(ILibAsyncServerSocket_ServerModule AsyncServerSocketToken, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user, ptrdiff_t newOffset);
void ILibAsyncServerSocket_SetReAllocateNotificationCallback(ILibAsyncServerSocket_ServerModule AsyncServerSocketToken, ILibAsyncServerSocket_ConnectionToken ConnectionToken, ILibAsyncServerSocket_BufferReAllocated Callback);

/*! \typedef ILibAsyncServerSocket_OnInterrupt
	\brief Handler for when a session was interrupted by a call to ILibStopChain
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param user
*/
typedef void (*ILibAsyncServerSocket_OnInterrupt)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user);
/*! \typedef ILibAsyncServerSocket_OnReceive
	\brief Handler for when data is received
	\par
	<B>Note on memory handling:</B>
	When you process the received buffer, you must advance \a p_beginPointer the number of bytes that you 
	have processed. If \a p_beginPointer does not equal \a endPointer when this method completes,
	the system will continue to reclaim any memory that has already been processed, and call this method again
	until no more memory has been processed. If no memory has been processed, and more data has been received
	on the network, the buffer will be automatically grown (according to a specific alogrythm), to accomodate any new data.
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param buffer The data that was received
	\param[in,out] p_beginPointer The start index of the data that was received
	\param endPointer The end index of the data that was received
	\param[in,out] OnInterrupt Set this pointer to receive notification if this session is interrupted
	\param[in,out] user Set a custom user object
	\param[out] PAUSE Flag to indicate if the system should continue reading data off the network
*/
typedef void (*ILibAsyncServerSocket_OnReceive)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken,char* buffer,int *p_beginPointer, int endPointer, ILibAsyncServerSocket_OnInterrupt *OnInterrupt,void **user, int *PAUSE);
/*! \typedef ILibAsyncServerSocket_OnConnect
	\brief Handler for when a connection is made
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param[in,out] user Set a user object to associate with this connection
*/
typedef void (*ILibAsyncServerSocket_OnConnect)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken,void **user);
/*! \typedef ILibAsyncServerSocket_OnDisconnect
	\brief Handler for when a connection is terminated normally
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param user User object that was associated with this connection
*/
typedef void (*ILibAsyncServerSocket_OnDisconnect)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user);
/*! \typedef ILibAsyncServerSocket_OnSendOK
	\brief Handler for when pending send operations have completed
	\par
	This handler will only be called if a call to \a ILibAsyncServerSocket_Send returned a value greater
	than 0, which indicates that not all of the data could be sent.
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param user User object that was associated with this connection
*/
typedef void (*ILibAsyncServerSocket_OnSendOK)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user);



ILibAsyncServerSocket_ServerModule ILibCreateAsyncServerSocketModule(void *Chain, int MaxConnections, int PortNumber, int initialBufferSize, ILibAsyncServerSocket_OnConnect OnConnect,ILibAsyncServerSocket_OnDisconnect OnDisconnect,ILibAsyncServerSocket_OnReceive OnReceive,ILibAsyncServerSocket_OnInterrupt OnInterrupt,ILibAsyncServerSocket_OnSendOK OnSendOK);

void *ILibAsyncServerSocket_GetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule);
void ILibAsyncServerSocket_SetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *user);

unsigned short ILibAsyncServerSocket_GetPortNumber(ILibAsyncServerSocket_ServerModule ServerSocketModule);

/*! \def ILibAsyncServerSocket_Send
	\brief Sends data onto the TCP stream
	\param ServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param buffer The data to be sent
	\param bufferLength The length of \a buffer
	\param UserFreeBuffer The \a ILibAsyncSocket_MemoryOwnership enumeration, that identifies how the memory pointer to by \a buffer is to be handled
	\returns \a ILibAsyncSocket_SendStatus indicating the send status
*/
#define ILibAsyncServerSocket_Send(ServerSocketModule, ConnectionToken, buffer, bufferLength, UserFreeBuffer) ILibAsyncSocket_Send(ConnectionToken,buffer,bufferLength,UserFreeBuffer)

/*! \def ILibAsyncServerSocket_Disconnect
	\brief Disconnects a TCP stream
	\param ServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
*/
#define ILibAsyncServerSocket_Disconnect(ServerSocketModule, ConnectionToken) ILibAsyncSocket_Disconnect(ConnectionToken)
/*! \def ILibAsyncServerSocket_GetPendingBytesToSend
	\brief Gets the outstanding number of bytes to be sent
*/
#define ILibAsyncServerSocket_GetPendingBytesToSend(ServerSocketModule, ConnectionToken) ILibAsyncSocket_GetPendingBytesToSend(ConnectionToken)
/*! \def ILibAsyncServerSocket_GetTotalBytesSent
	\brief Gets the total number of bytes that have been sent
	\param ServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
*/
#define ILibAsyncServerSocket_GetTotalBytesSent(ServerSocketModule, ConnectionToken) ILibAsyncSocket_GetTotalBytesSent(ConnectionToken)
/*! \def ILibAsyncServerSocket_ResetTotalBytesSent
	\brief Resets the total bytes sent counter
	\param ServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
*/
#define ILibAsyncServerSocket_ResetTotalBytesSent(ServerSocketModule, ConnectionToken) ILibAsyncSocket_ResetTotalBytesSent(ConnectionToken)

/*! \} */

#endif
