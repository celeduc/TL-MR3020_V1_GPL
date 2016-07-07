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
//  File: InbIp.cpp
//  Description: This file contains implementation of functions for the
//               Inband IP manager.
//  
//===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>

#include "tutrace.h"
#include "slist.h"
#include "WscCommon.h"
#include "WscError.h"
#include "Portability.h"
#include "UdpLib.h"
#include "InbIp.h"

#define TERMINATE_STR           "Terminate Thread"
#define PIPE_RECV_BUFFER_SIZE    128


CInbIp::CInbIp()
{
    TUTRACE((TUTRACE_INFO, "CInbIp Construction\n"));
    m_initialized = false;
    m_monitoring = false;
    m_server = false;
    m_listenSock = -1;
    m_connectSock = -1;
    m_activeSock = -1;
    m_socketPipe[0] = -1;
    m_socketPipe[1] = -1;
    m_connectThreadHandle = (uint32) NULL;
    m_listenThreadHandle = (uint32) NULL;
}

CInbIp::~CInbIp()
{
    TUTRACE((TUTRACE_INFO, "CInbIp Destruction\n"));
    Deinit();
}

uint32 CInbIp::Init()
{
    return Init(false);
}

uint32 CInbIp::Init(bool serverFlag)
{
    m_server = serverFlag;

    m_initialized = true;

    if (pipe(m_socketPipe))
    {
        TUTRACE((TUTRACE_ERR, "Unable to create pipe.\n"));
        return WSC_ERR_SYSTEM;    
    }

    return WSC_SUCCESS;
}

uint32 CInbIp::StartMonitor()
{
    struct sockaddr_in  sockAddr;
    uint32 retVal;

    if ( ! m_initialized)
    {
        TUTRACE((TUTRACE_ERR, "Not initialized; Returning\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    if (m_monitoring)
    {
        TUTRACE((TUTRACE_ERR, "Already Monitoring; Returning\n"));
        return TRIP_ERR_ALREADY_MONITORING;
    }

    if (m_server)
    {
        if ((m_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            TUTRACE((TUTRACE_ERR, "Allocating socket failed\n"));
            return WSC_ERR_SYSTEM;
        }
        TUTRACE((TUTRACE_INFO, "New Listen Socket = %d\n", m_listenSock));

        /*Make this a Non Blocking socket*/ 
        if (fcntl(m_listenSock, F_SETFL, O_NONBLOCK) == -1)
        {
            TUTRACE((TUTRACE_ERR, "Could not set Non Blocking socket\n"));
            close(m_listenSock);
            return WSC_ERR_SYSTEM;
        }

        sockAddr.sin_family = AF_INET;
        sockAddr.sin_port = htons(WSC_INBIP_IP_PORT);
        sockAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(m_listenSock, (struct sockaddr *) &sockAddr, 
                        sizeof(sockAddr)) < 0)
        {
            TUTRACE((TUTRACE_ERR, "binding listen socket failed %d\n", errno));
            close(m_listenSock);
            m_listenSock = -1;
            return WSC_ERR_SYSTEM;
        }

        retVal = WscCreateThread(&m_listenThreadHandle, 
                    StaticListenThread, this);
        if (retVal != WSC_SUCCESS)
        {
            TUTRACE((TUTRACE_ERR,  "CreateThread for StaticListenThread "
                    "failed.\n"));
            return retVal;
        }

        sleep(1);
    }
    else
    {
        if ((m_connectSock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            TUTRACE((TUTRACE_ERR, "Allocating socket failed\n"));
            return WSC_ERR_SYSTEM;
        }

        retVal = WscCreateThread(&m_connectThreadHandle, 
                    StaticConnectThread, this);
        if (retVal != WSC_SUCCESS)
        {
            TUTRACE((TUTRACE_ERR,  "CreateThread for StaticConnectThread "
                    "failed.\n"));
            return retVal;
        }

        sleep(1);
    }

    m_monitoring = true;

    return WSC_SUCCESS;
}

uint32 CInbIp::StopMonitor()
{
    ssize_t retval;

    if ( ! m_initialized)
    {
        TUTRACE((TUTRACE_ERR, "Not initialized; Returning\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    if ( ! m_monitoring)
    {
        TUTRACE((TUTRACE_ERR, "Not Monitoring; Returning\n"));
        return TRIP_ERR_NOT_MONITORING;
    }

    // Send QUIT to all waiting threads and do a pthread_join
    if (m_server)
    {
        retval = write(m_socketPipe[1],TERMINATE_STR , sizeof(TERMINATE_STR ));

        TUTRACE((TUTRACE_INFO,"WRITE RETURNED %d BYTES\n ", retval));
        if(retval == sizeof(TERMINATE_STR )) 
        {
            TUTRACE( (TUTRACE_INFO, 
                    "Waiting for listening thread\n"));

            pthread_join(m_listenThreadHandle, NULL);

            TUTRACE((TUTRACE_INFO, "Successfully terminated ListenThread\n"));   
        }
        else
        {
            TUTRACE((TUTRACE_ERR, "Couldn't send TerminateStr to "
                    "ListenThread\n"));
        }

        if (m_listenSock != -1)
        {
            close(m_listenSock);
            m_listenSock = -1;
        }
    }
    else  // client
    {
        retval = write(m_socketPipe[1],TERMINATE_STR , sizeof(TERMINATE_STR ));

        TUTRACE((TUTRACE_INFO,"WRITE RETURNED %d BYTES\n ", retval));
        if(retval == sizeof(TERMINATE_STR )) 
        {
            TUTRACE((TUTRACE_INFO, 
                    "Waiting for connect thread\n"));

            pthread_join(m_connectThreadHandle, NULL);

            TUTRACE((TUTRACE_INFO, "Successfully terminated ConnectThread.\n"));
        }
        else
        {
            TUTRACE((TUTRACE_ERR, "Couldn't send TerminateStr to "
                    "ConnectThread\n"));
        }

        if (m_connectSock != -1)
        {
            close(m_connectSock);
            m_connectSock = -1;
        }
    }

    m_monitoring = false;

    return WSC_SUCCESS;
}

uint32 CInbIp::WriteData(char * dataBuffer, uint32 dataLen)
{
    TUTRACE((TUTRACE_INFO, "In CInbIp::WriteData buffer Length = %d\n", 
            dataLen));

    if ( ! m_initialized)
    {
        TUTRACE((TUTRACE_ERR, "Not initialized; Returning\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    if ( (! dataBuffer) || (! dataLen))
    {
        TUTRACE((TUTRACE_ERR, "Invalid Parameters\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    if (m_activeSock == -1)
    {
        TUTRACE((TUTRACE_ERR, "activeSock is invalid\n"));
        return TRIP_ERR_INVALID_SOCKET;
    }

    if (send(m_activeSock, dataBuffer, dataLen, 0) == -1) 
    {
        TUTRACE((TUTRACE_ERR, "Sending data failed!\n"));
        return TRIP_ERR_SENDRECV;
    }

    return WSC_SUCCESS;
}

uint32 CInbIp::ReadData(char * dataBuffer, uint32 * dataLen)
{
    /*
    int recvBytes = 0;
    int len;

    if (dataBuffer && (! dataLen))
    {
        return WSC_ERR_INVALID_PARAMETERS;
    }

    if (m_activeSock == -1)
    {
        TUTRACE((TUTRACE_ERR, "ActiveSock is invalid; returning...\n"));
        return TRIP_ERR_INVALID_SOCKET;
    }

    len = (int) *dataLen;
    recvBytes = recv(m_activeSock, dataBuffer, len, 0);

    if (recvBytes == SOCKET_ERROR)
    {
        TUTRACE((TUTRACE_ERR, "IP Recv failed; returning...\n"));
        return TRIP_ERR_SENDRECV;
    }

    *dataLen = recvBytes;
    */

    TUTRACE((TUTRACE_ERR, "ReadData is not used. ReadThread does this job\n"));
    return WSC_ERR_NOT_IMPLEMENTED;
    // return WSC_SUCCESS;
}

uint32 CInbIp::Deinit()
{
    TUTRACE((TUTRACE_INFO, "In CInbIp::Deinit\n"));
    if ( ! m_initialized)
    {
        TUTRACE((TUTRACE_ERR, "Not initialized; Returning\n"));
        return WSC_ERR_NOT_INITIALIZED;
    }

    if (m_monitoring)
    {
        StopMonitor();
    }

    m_activeSock = -1;

    close(m_socketPipe[0]);
    close(m_socketPipe[1]);

    m_initialized = false;

    return WSC_SUCCESS;
}

void CInbIp::InvokeCallback(char * buf, uint32 len)
{
    char * sendBuf;
    S_CB_COMMON * eapComm;

    sendBuf = new char[sizeof(S_CB_COMMON) + len];
    if (sendBuf == NULL)
    {
        TUTRACE((TUTRACE_ERR, "Allocating memory for Sendbuf failed\n"));
        return;
    }
    // call callback
    eapComm = (S_CB_COMMON *) sendBuf;
    eapComm->cbHeader.eType = CB_TRIP;
    eapComm->cbHeader.dataLength = len;
    
    if (buf) {
        memcpy(sendBuf + sizeof(S_CB_COMMON), buf, len);
    }
    
    if (m_trCallbackInfo.pf_callback)
    {
        TUTRACE((TUTRACE_INFO, "IP: Calling Transport Callback\n"));
        m_trCallbackInfo.pf_callback(sendBuf, m_trCallbackInfo.p_cookie);
        TUTRACE((TUTRACE_INFO, "Transport Callback Returned\n"));
    }
    else
    {
        TUTRACE((TUTRACE_ERR, "No Callback function set\n"));
    }
}

void CInbIp::SendConnectedNotification(void)
{
#ifdef _MIDD
    char * sendBuf;
    bool runThread = true;
    S_CB_COMMON * eapComm;

    sendBuf = new char[sizeof(S_CB_COMMON)];
    if (sendBuf == NULL)
    {
        TUTRACE((TUTRACE_ERR, "Allocating memory for Sendbuf failed\n"));
        return;
    }
    // call callback
    eapComm = (S_CB_COMMON *) sendBuf;
    eapComm->cbHeader.eType = CB_TRIPCONNECT;
    eapComm->cbHeader.dataLength = 0;
    
    if (m_trCallbackInfo.pf_callback)
    {
        TUTRACE((TUTRACE_INFO, "IP: Calling Transport Callback for CONNECT\n"));
        m_trCallbackInfo.pf_callback(sendBuf, m_trCallbackInfo.p_cookie);
        TUTRACE((TUTRACE_INFO, "Transport Callback Returned for CONNECT\n"));
    }
    else
    {
        TUTRACE((TUTRACE_ERR, "No Callback function set\n"));
    }
#endif // _MIDD
}

void * CInbIp::StaticReadThread(IN void *p_data)
{
    TUTRACE((TUTRACE_INFO, "In CInbIp::StaticReadThread\n"));
    ((CInbIp *)p_data)->ActualReadThread();
    return 0;
}


void * CInbIp::ActualReadThread()
{
    char buffer[WSC_INBIP_RECV_BUFSIZE];
    int nread = 0;

    TUTRACE((TUTRACE_INFO, "ActualReadThread Started\n"));

    /*read data */
    while(1)
    {
        nread = read(m_activeSock, buffer, WSC_INBIP_RECV_BUFSIZE);
        if(nread < 0)
        {
            /* error in read */
            TUTRACE((TUTRACE_ERR, "Read returned %d!\n", nread));

            switch(errno)
            {
            case EINTR:
            case EAGAIN:
                nread=0;
                break;
            case EIO:
            case EISDIR:
            case EBADF:
            case EINVAL:
            case EFAULT:
            default:
                TUTRACE((TUTRACE_INFO,"read error! socket already"
                                 "deleted?!\n"));

                TUTRACE((TUTRACE_ERR, "Terminating ActualReadThread\n"));
                close(m_activeSock);
                m_activeSock = -1;
                pthread_exit(NULL);                    
            }/*switch*/

        } /* if( nread < 0 ) */
        else if(nread == 0)
        {                                        
            TUTRACE((TUTRACE_ERR, "EOF detected!!!\n"));
            TUTRACE((TUTRACE_ERR, "Terminating ActualReadThread\n"));
            close(m_activeSock);
            m_activeSock = -1;
            pthread_exit(NULL);                        
        }
        else
        {
            /* we have new data! */
            TUTRACE((TUTRACE_INFO, "Number of bytes "
               "received %d,Socket %X\n", nread, m_activeSock));

            InvokeCallback(buffer, nread);
    
        }/*else*/

    } /* while(1) */                        

    TUTRACE((TUTRACE_ERR, "Exiting ActualReadThread\n"));
    close(m_activeSock);
    m_activeSock = -1;
    pthread_exit(NULL);                    
    return 0;
}


void * CInbIp::StaticListenThread(IN void *p_data)
{
    TUTRACE((TUTRACE_INFO, "In CInbIp::StaticListenThread\n"));
    ((CInbIp *)p_data)->ActualListenThread();
    return 0;
}


void * CInbIp::ActualListenThread()
{
    int       acceptSock = -1;
    int       highestFD;
    fd_set    rfds;
    int       retval;
    uint32    retVal;
    char      recvBuffer[PIPE_RECV_BUFFER_SIZE];
    uint32 threadHandle;
    socklen_t connectedCliAddrLen;
    struct sockaddr_in  connectedCliAddr;

    TUTRACE((TUTRACE_INFO, "ActualListenThread Started\n"));

    // Establish a socket to listen for incoming connections.
    if (listen(m_listenSock, 5) <0 ) 
    {
        TUTRACE((TUTRACE_ERR, "Listening socket failed\n"));
        pthread_exit(NULL);
    }

    /*Enter loop for accepting new connections*/
    while(1)
    {
        acceptSock = accept(m_listenSock, 
                           (struct sockaddr *) &connectedCliAddr,
                           &connectedCliAddrLen);

        if(acceptSock == -1 )
        {
            switch(errno)
            {
            case EWOULDBLOCK:
                    TUTRACE((TUTRACE_INFO, "Received EWOULDBLOCK."
                                            " Everything OK\n"));
                    break;
            case EBADF:
                    TUTRACE((TUTRACE_ERR, "EBADF\n"));
                    pthread_exit(NULL);
                    break;
            case ENOTSOCK:
                    TUTRACE((TUTRACE_ERR, "ENOTSOCK\n"));
                    pthread_exit(NULL);
                    break;
            case EOPNOTSUPP:
                    TUTRACE((TUTRACE_ERR, "EOPNOTSUPP\n"));
                    pthread_exit(NULL);
                    break;
            case EFAULT:
                    TUTRACE((TUTRACE_ERR, "EFAULT\n"));
                    pthread_exit(NULL);
                    break;
            case EPERM:
                    TUTRACE((TUTRACE_ERR, "EPERM\n"));
                    pthread_exit(NULL);
                    break;
            default:
                    TUTRACE((TUTRACE_ERR, "Unknown\n"));
                    pthread_exit(NULL);
                    break;
            }/*switch*/
        }
        else
        {
            /*New Socket connected*/
            m_activeSock = acceptSock;

            /*Create new thread for the newly conected socket*/
            retVal = WscCreateThread(&threadHandle, 
                        StaticReadThread, this);
            if (retVal != WSC_SUCCESS)
            {
                TUTRACE((TUTRACE_ERR,  "CreateThread for StaticReadThread "
                        "failed.\n"));
                close(acceptSock);
                m_activeSock = -1;
                continue;
            }

            sleep(1);

        }

        /*Do a select now. Wait for either a new connection, or the 
          terminate command on the pipe*/

        FD_ZERO(&rfds);
        FD_SET(m_listenSock, &rfds);
        FD_SET(m_socketPipe[0], &rfds);

        //if( *((int *)listenSock) > m_socketPipe[0])
        if(m_listenSock > m_socketPipe[0])
                highestFD = m_listenSock + 1;
        else
                highestFD = m_socketPipe[0] + 1;

        retval = select(highestFD, &rfds, NULL, NULL, NULL);

        if(retval == -1)
        {
            TUTRACE((TUTRACE_INFO, "SELECT returned -1."
                " Listen thread exiting...\n"));

            if (acceptSock != -1)
            {
                shutdown(acceptSock, SHUT_RDWR);
                pthread_join(threadHandle, NULL);

                TUTRACE( (TUTRACE_INFO, "Successfully stopped "
                        "ReadThread.\n") );        
                close(acceptSock);
                m_activeSock = -1;
            }
            pthread_exit(NULL);
        }

        /*check if data was written to the pipe */
        if(FD_ISSET(m_socketPipe[0], &rfds))
        {
            retval = read(m_socketPipe[0], recvBuffer, PIPE_RECV_BUFFER_SIZE);
                
            /*if there is an error, or if the pipe contains the 
              terminate string, exit */
            if( (retval <= 0) || 
                !strncmp(recvBuffer,TERMINATE_STR , retval) )
            {
                TUTRACE((TUTRACE_ERR, "Terminate command "
                                      "received\n"));
                if (acceptSock != -1)
                {
                    shutdown(acceptSock, SHUT_RDWR);
                    pthread_join(threadHandle, NULL);
    
                    TUTRACE( (TUTRACE_INFO, "Successfully stopped "
                            "ReadThread.\n") );        
                    close(acceptSock);
                    m_activeSock = -1;
                }
                pthread_exit(NULL);
            }

            /*go to the start of the loop*/
            continue;        
        }/*if(FD_ISSET.... */

        TUTRACE((TUTRACE_INFO, "Going back to top of listen thread\n"));

        /*Select was for the socket. So go back to the top*/

    }/*while(1)*/

    return 0;
}


void * CInbIp::StaticConnectThread(IN void *p_data)
{
    TUTRACE((TUTRACE_INFO, "In CInbIp::StaticConnectThread\n"));
    ((CInbIp *)p_data)->ActualConnectThread();
    return 0;
}


void * CInbIp::ActualConnectThread()
{
    uint32 retVal;
    uint32 threadHandle;
    
    TUTRACE((TUTRACE_INFO, "ActualConnectThread Started\n"));

    struct sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(WSC_INBIP_IP_PORT);
    sockAddr.sin_addr.s_addr = inet_addr(WSC_INBIP_IP_ADDR);

    m_activeSock = -1;

    while (1)
    {
        if (m_connectSock == -1)
        {
            if ((m_connectSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) 
                    == -1)
            {
                TUTRACE((TUTRACE_ERR, "Allocating socket failed\n"));
                break;
            }
        }

        TUTRACE((TUTRACE_INFO,"Connecting to %s\n", WSC_INBIP_IP_ADDR));
        if (connect(m_connectSock, (struct sockaddr *) &sockAddr, 
                        sizeof(struct sockaddr_in)) == -1)
        {
            TUTRACE((TUTRACE_ERR, "Connect failed, Error: %d\n", errno));

            if (WaitForShortTime())
                pthread_exit(NULL);
            else
                continue;
        }

        m_activeSock = m_connectSock;
#ifdef _MIDD
        SendConnectedNotification();
#endif // _MIDD
        
        /*Create new thread for the newly conected socket*/
        retVal = WscCreateThread(&threadHandle, 
                    StaticReadThread, this);
        if (retVal != WSC_SUCCESS)
        {
            TUTRACE((TUTRACE_ERR,  "CreateThread for StaticReadThread "
                    "failed.\n"));
            close(m_connectSock);
            m_activeSock = -1;
            continue;
        }

        if (WaitFunction(threadHandle))
            pthread_exit(NULL);
        else
            continue;

    } // outer while (1)

    close(m_connectSock);
    m_connectSock = m_activeSock = -1;
    return 0;
}

bool CInbIp::WaitForShortTime()
{
    struct timeval tv;
    fd_set    rfds;
    int       retval;
    char      recvBuffer[PIPE_RECV_BUFFER_SIZE];

    TUTRACE((TUTRACE_INFO, "In CInbIp::WaitForShortTime\n"));
    while (1)
    {
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        FD_ZERO(&rfds);
        FD_SET(m_socketPipe[0], &rfds);

        retval = select(m_socketPipe[0] + 1, &rfds, NULL, NULL, &tv);

        if(retval == -1)
        {
            TUTRACE((TUTRACE_INFO, "SELECT returned -1."
                " Connect thread exiting...\n"));

            if (m_connectSock != -1)
            {
                close(m_connectSock);
                m_connectSock = m_activeSock = -1;
            }
            return true;
        }
        else if (retval)
        {
            /*check if data was written to the pipe */
            if(FD_ISSET(m_socketPipe[0], &rfds))
            {
                retval = read(m_socketPipe[0], recvBuffer, 
                        PIPE_RECV_BUFFER_SIZE);
                    
                /*if there is an error, or if the pipe contains the 
                    terminate string, exit */
                if( (retval <= 0) || 
                    !strncmp(recvBuffer,TERMINATE_STR , retval) )
                {
                    TUTRACE((TUTRACE_ERR, "Terminate command "
                                            "received\n"));
                    if (m_connectSock != -1)
                    {
                        close(m_connectSock);
                        m_connectSock = m_activeSock = -1;
                    }
                    return true;
                }
            }/*if(FD_ISSET.... */
        }
        else // timed out
        {
            return false;
        }
    } // while (1)

    return false;
}

bool CInbIp::WaitFunction(uint32 threadHandle)
{
    struct timeval tv;
    fd_set    rfds;
    int       retval;
    char      recvBuffer[PIPE_RECV_BUFFER_SIZE];

    TUTRACE((TUTRACE_INFO, "In CInbIp::WaitFunction\n"));
    while (1)
    {
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        FD_ZERO(&rfds);
        FD_SET(m_socketPipe[0], &rfds);

        retval = select(m_socketPipe[0] + 1, &rfds, NULL, NULL, &tv);

        if(retval == -1)
        {
            TUTRACE((TUTRACE_INFO, "SELECT returned -1."
                " Connect thread exiting...\n"));

            if (m_connectSock != -1)
            {
                shutdown(m_connectSock, SHUT_RDWR);
                pthread_join(threadHandle, NULL);

                TUTRACE( (TUTRACE_INFO, "Successfully stopped "
                        "ReadThread.\n") );        
                close(m_connectSock);
                m_connectSock = m_activeSock = -1;
            }
            return true;
        }
        else if (retval)
        {
            /*check if data was written to the pipe */
            if(FD_ISSET(m_socketPipe[0], &rfds))
            {
                retval = read(m_socketPipe[0], recvBuffer, 
                        PIPE_RECV_BUFFER_SIZE);
                    
                /*if there is an error, or if the pipe contains the 
                    terminate string, exit */
                if( (retval <= 0) || 
                    !strncmp(recvBuffer,TERMINATE_STR , retval) )
                {
                    TUTRACE((TUTRACE_ERR, "Terminate command "
                                            "received\n"));
                    if (m_connectSock != -1)
                    {
                        shutdown(m_connectSock, SHUT_RDWR);
                        pthread_join(threadHandle, NULL);

                        TUTRACE( (TUTRACE_INFO, "Successfully stopped "
                                "ReadThread.\n") );        
                        close(m_connectSock);
                        m_connectSock = m_activeSock = -1;
                    }
                    return true;
                }
            }/*if(FD_ISSET.... */
        }
        else // timed out
        {
            // check if read thread has closed
            if (m_activeSock == -1)
            {
                // try to connect again
                m_connectSock = -1;
                return false;
            }
        }
    } // while (1)

    return false;
}

