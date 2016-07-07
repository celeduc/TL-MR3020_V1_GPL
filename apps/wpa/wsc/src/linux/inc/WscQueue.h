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
//  File Name: WscQueue.h
//  Description: Implements a simple queue for passing messages between
//               different components.
//
****************************************************************************/

#ifndef _QUEUE_
#define _QUEUE_

#include <cerrno>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
// #include "tutrace.h"

class CWscQueue
{
public:
    CWscQueue();
    virtual ~CWscQueue();
    uint32 Init();
    uint32 DeInit();
    uint32 Enqueue(uint32 dwCount, uint32 dwSequenceNumber, void* pData);
    uint32 Dequeue(uint32* pdwCount, uint32 dwSequenceNumber, uint32 dwTimeout, void** ppData);

protected:
    typedef struct _QUEUE_ELEMENT
    {
        uint32                    dwCount;
        uint32                    dwSequenceNumber;
        void*                    pData;
        struct _QUEUE_ELEMENT*    pPrev;
        struct _QUEUE_ELEMENT*    pNext;
    } QUEUE_ELEMENT, *PQUEUE_ELEMENT;
    bool m_fInited;
    PQUEUE_ELEMENT m_pFirst;
    PQUEUE_ELEMENT m_pLast;
    
    pthread_mutex_t        m_CriticalSection; 
    pthread_mutex_t        m_EventMutex;
    pthread_cond_t        m_Event; 
};


inline CWscQueue::CWscQueue()
{
    m_fInited = false;
}


inline CWscQueue::~CWscQueue()
{
    if (m_fInited)
        DeInit();
}

inline uint32 CWscQueue::Init()
{
    if (m_fInited)
        return CQUEUE_ERR_INTERNAL;

    m_pFirst = NULL;
    m_pLast = NULL;
    // m_Event = PTHREAD_COND_INITIALIZER;
    // Initialize conditional variable
    pthread_cond_init( &m_Event, NULL );
    // m_EventMutex = PTHREAD_MUTEX_INITIALIZER;
    // Initalize event mutex variable
    pthread_mutex_init( &m_EventMutex, NULL );
    // m_CriticalSection = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init( &m_CriticalSection, NULL );
    m_fInited = true;
    return WSC_SUCCESS;
}


inline uint32 CWscQueue::DeInit()
{
    if (!m_fInited)
        return CQUEUE_ERR_INTERNAL;

    pthread_mutex_lock(&m_CriticalSection);
    while (m_pFirst != NULL)
    {
        PQUEUE_ELEMENT pQueueElement = m_pFirst;
        m_pFirst = m_pFirst->pNext;
        delete (uint8 *) (pQueueElement->pData);
        delete pQueueElement;
    }
    
    pthread_mutex_lock(&m_EventMutex);
    pthread_cond_destroy(&m_Event);
    pthread_mutex_unlock(&m_EventMutex);

    m_fInited = false;
    pthread_mutex_unlock(&m_CriticalSection);
    
    pthread_mutex_destroy(&m_EventMutex);
    pthread_mutex_destroy(&m_CriticalSection);
    return WSC_SUCCESS;
}


inline uint32 CWscQueue::Enqueue(uint32 dwCount, uint32 dwSequenceNumber, void* pData)
{
    // TUTRACE((TUTRACE_INFO, "-- Enqueue() enter [%d] -- \n", dwSequenceNumber );
   // TUTRACE((TUTRACE_INFO, "CWscQueue::Enqueue(): m_fInited = %d, dwSequenceNumber = %d\r\n",
 //               m_fInited, dwSequenceNumber));
    if (!m_fInited)
        return CQUEUE_ERR_INTERNAL;

    PQUEUE_ELEMENT pQueueElement = new QUEUE_ELEMENT;
   // TUTRACE((TUTRACE_INFO, "CWscQueue::Enqueue(): pQueueElement = %08x\r\n", pQueueElement));
    if (pQueueElement == NULL)
        return WSC_ERR_OUTOFMEMORY;

    pthread_mutex_lock(&m_CriticalSection);

    pQueueElement->dwCount = dwCount;
    pQueueElement->dwSequenceNumber = dwSequenceNumber;
    pQueueElement->pData = pData;
    pQueueElement->pPrev = m_pLast;
    if (m_pLast != NULL)
    {
        assert(m_pFirst != NULL);
        m_pLast->pNext = pQueueElement;
    }
    else
    {
        assert(m_pFirst == NULL);
    }
    pQueueElement->pNext = NULL;
    m_pLast = pQueueElement;
    
    pthread_mutex_lock(&m_EventMutex);
    if (m_pFirst == NULL)
    {
        m_pFirst = pQueueElement;
        pthread_cond_signal(&m_Event);
    }
    pthread_mutex_unlock(&m_EventMutex);

    pthread_mutex_unlock(&m_CriticalSection);

   // TUTRACE((TUTRACE_INFO, "CWscQueue::Enqueue(): OK\r\n"));
    // TUTRACE((TUTRACE_INFO, "-- Enqueue() exit [%d] -- \n", dwSequenceNumber));
    return WSC_SUCCESS;
}


inline uint32 CWscQueue::Dequeue(uint32* pdwCount, uint32 dwSequenceNumber, uint32 dwTimeout, void** ppData)
{
    // TUTRACE((TUTRACE_INFO, "-- Dequeue() enter [%d] -- \n", dwSequenceNumber));
    if ((!m_fInited) || (ppData == NULL))
        return CQUEUE_ERR_INTERNAL;

    bool fDone = false;
    uint32 dwStatus;
    while (!fDone)
    {
        // check to see if we have any data in the queue
        if ( NULL == m_pFirst )
        {
            // no data, so block
            pthread_mutex_lock(&m_EventMutex);
            // uint32 dwStatus = WaitForSingleObject(m_hEvent, dwTimeout);
            if (0 == dwTimeout)
            {
                // infinite timeout
                dwStatus = pthread_cond_wait(&m_Event, &m_EventMutex); 
            } 
            else
            {
                // timed wait
                struct timeval    now;
                struct timespec timeout;
                gettimeofday(&now, NULL);
                timeout.tv_sec = now.tv_sec + dwTimeout;
                timeout.tv_nsec = now.tv_usec * 1000;

                dwStatus = pthread_cond_timedwait(&m_Event, &m_EventMutex, 
                                    &timeout);
            }
            pthread_mutex_unlock(&m_EventMutex);
            if (0 != dwStatus)
                return CQUEUE_ERR_IPC;
        }
        
        pthread_mutex_lock(&m_CriticalSection);

        assert(m_pFirst != NULL);
        assert(m_pLast != NULL);

        PQUEUE_ELEMENT pQueueElement = m_pFirst;
        m_pFirst = m_pFirst->pNext;
        if (m_pFirst == NULL)
        {
            assert(m_pLast == pQueueElement);
            m_pLast = NULL;
            // pthread_mutex_lock(&m_EventMutex);
            // ResetEvent(m_hEvent);
            // pthread_cond_init(&m_Event, NULL);
            // pthread_mutex_unlock(&m_EventMutex);
        }
        else
        {
            m_pFirst->pPrev = NULL;
            // pthread_mutex_lock(&m_EventMutex);
            // SetEvent(m_hEvent);
            // pthread_cond_signal(&m_Event);
            // pthread_mutex_unlock(&m_EventMutex);
        }
        // if ((dwSequenceNumber == pQueueElement->dwSequenceNumber) || (dwSequenceNumber == 0))
        // using dwSequenceNumber as an ID tag now
        if ( 1 )
        {
            if (pdwCount != NULL)
                *pdwCount = pQueueElement->dwCount;
            *ppData = pQueueElement->pData;
            delete pQueueElement;
            fDone = true;
        }
        else
        {
            delete (uint8 *) (pQueueElement->pData);
            delete pQueueElement;
        }
        pthread_mutex_unlock(&m_CriticalSection);
    }
    // TUTRACE((TUTRACE_INFO, "-- Dequeue() exit [%d] -- \n", dwSequenceNumber));
    return WSC_SUCCESS;
}

#endif // _QUEUE
