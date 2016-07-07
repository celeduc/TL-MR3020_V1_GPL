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
//  File Name: Portability.c
//  Description: Provides a simple portability layer for Linux.
//
****************************************************************************/

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "WscError.h"
#include "tutrace.h"
#include "Portability.h"

/*
 * Name        : WscSyncCreate
 * Description : Creates and initializes a lock
 * Arguments   : OUT uint32 **handle - a handle to the synchronization object
 * Return type : uint32
 */
uint32 WscSyncCreate(OUT uint32 **handle)
{
    int err;

    // TUTRACE((TUTRACE_INFO, "WscSyncCreate(....\n"));
    if ((!handle))
    {
		TUTRACE(( TUTRACE_ERR, "WscSyncCreate: Invalid Handle \n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    pthread_mutex_t *mutex = malloc( sizeof( pthread_mutex_t ) );

    if ( !mutex ) 
        return WSC_ERR_OUTOFMEMORY;

    err = pthread_mutex_init( mutex, NULL );
    if(0 != err)
    {    
        switch(err)
        {
        case EBUSY:
            TUTRACE((TUTRACE_ERR, "PORTAB: EBUSY\n"));
        break;
        case EINVAL:
            TUTRACE((TUTRACE_ERR, "PORTAB: EINVAL\n"));
        break;
        case EAGAIN:
            TUTRACE((TUTRACE_ERR, "PORTAB: ENOSPC\n"));
        break;
        case ENOMEM:
            TUTRACE((TUTRACE_ERR, "PORTAB: ENOMEM\n"));
        break;
        case EPERM:
            TUTRACE((TUTRACE_ERR, "PORTAB: EPERM\n"));
        break;
        default:
            TUTRACE((TUTRACE_ERR, "PORTAB: Unknown error\n"));
        }
        return WSC_ERR_SYSTEM; 
    }

    *handle = (uint32 *)mutex;
    return WSC_SUCCESS;
}

/*
 * Name        : WscSyncDestroy
 * Description : Deinitializes and destroys a lock
 * Arguments   : IN uint32 *handle - handle to the synchronization object
 * Return type : uint32
 */
uint32 WscSyncDestroy(IN uint32 *handle)
{
    int err;

    // TUTRACE((TUTRACE_INFO, "WscUnlock(....\n"));

    if (!handle)
    {
		TUTRACE(( TUTRACE_ERR, "WscSyncDestroy: Invalid Handle\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    err = pthread_mutex_destroy((pthread_mutex_t *)handle);

    if(handle)
        free( (pthread_mutex_t *)handle );

    /*Finally, check the return code from the mutex destroy and return 
      the appropriate error code*/
    if(0 != err)
        return PORTAB_ERR_SYNCHRONIZATION;   
    else 
        return WSC_SUCCESS;
}

/*
 * Name        : WscLock
 * Description : Obtains a lock on the specified synchronization object
 * Arguments   : IN uint32 *handle - handle to the synchronization object
 * Return type : uint32
 */
uint32 WscLock(IN uint32 *handle)
{
    int err;

    // TUTRACE((TUTRACE_INFO, "WscLock(....\n"));

    if (!handle)
    {
		TUTRACE(( TUTRACE_ERR, "WscLock: Invalid Handle\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    err = pthread_mutex_lock((pthread_mutex_t *)handle);
    if(0 != err)
        return PORTAB_ERR_SYNCHRONIZATION;
    else
        return WSC_SUCCESS;
}

/*
 * Name        : WscUnlock
 * Description : Releases a lock on the specified synchronization object
 * Arguments   : IN uint32 *handle - handle to the synchronization object
 * Return type : uint32
 */
uint32 WscUnlock(IN uint32 *handle)
{
    int err;

    // TUTRACE((TUTRACE_INFO, "WscUnlock(....\n"));

    if (!handle)
    {
		TUTRACE(( TUTRACE_ERR, "WscUnlock: Invalid Handle\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    err = pthread_mutex_unlock((pthread_mutex_t *)handle);
    if(0 != err)
        return PORTAB_ERR_SYNCHRONIZATION;
    else
        return WSC_SUCCESS;
}
    
/*Thread functions*/
/*
 * Name        : WscCreateThread
 * Description : Creates a thread with the specified parameters and returns the
 *               thread handle
 * Arguments   : OUT uint32 *handle - handle to the thread. MUST be allocated by
 *                                   the caller.
 *               IN void *(*threadFunc)(void *) - pointer to the start routine
 *               IN void *arg - pointer to the arguments to pass to the start
 *                              routine.
 * Return type : uint32
 */
uint32 WscCreateThread(OUT uint32 *handle,
                       IN void *(*threadFunc)(void *),
                       IN void *arg)
{
    int ret;

    // TUTRACE((TUTRACE_INFO, "WscCreateThread(....\n"));

    if (!handle)
    {
		TUTRACE(( TUTRACE_ERR, "WscCreateThread: Invalid Handle\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    ret = pthread_create((pthread_t *)handle,
                         NULL,
                         threadFunc,
                         arg);

    if(0 != ret)
    {
        TUTRACE((TUTRACE_ERR, "WscCreateThread: Thread creation failed\n"));
        return WSC_ERR_SYSTEM;
    }

    return WSC_SUCCESS;
}

/*
 * Name        : WscDestroyThread
 * Description : Destroys a thread. In the current implementation, this 
 *               function only waits for the thread to exit.
 * Arguments   : IN uint32 handle - thread ID.
 * Return type : void
 */
void WscDestroyThread(IN uint32 handle)
{
    // TUTRACE((TUTRACE_INFO, "WscDestroyThread(....\n"));

    if (0 == handle)
    {
		TUTRACE(( TUTRACE_ERR, "WscDestroyThread: Invalid Handle\n"));
        return;
    }

    pthread_join(handle, NULL);
}


uint32 WscCreateEvent(uint32 *handle)
{
    int err;
    uint32 ret = WSC_SUCCESS;
	pthread_cond_t *cond;

    // TUTRACE((TUTRACE_INFO, "WscCreateThread(....\n"));

    if (!handle)
    {
		TUTRACE(( TUTRACE_ERR, "WscCreateEvent: Invalid Handle\n"));
        return WSC_ERR_INVALID_PARAMETERS;
    }

    cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	if(!cond)
	{
		TUTRACE(( TUTRACE_ERR, "WscCreateEvent: Malloc failed\n"));
        return WSC_ERR_OUTOFMEMORY;
	}

    err = pthread_cond_init(cond, NULL);
    if(0 != err)
    {
        switch(errno)
        {
        case EBUSY:
        case EINVAL:
        case ENOMEM:
            ret = PORTAB_ERR_EVENT;
        break;
        default:
            ret = WSC_ERR_SYSTEM;
        }
    }

    *handle = (uint32) cond;
    return ret;
}

uint32 WscDestroyEvent(uint32 handle)
{
    int err;

    err = pthread_cond_destroy((pthread_cond_t *)handle);
    free((pthread_cond_t *)handle);
    if(0 != err)
        return PORTAB_ERR_EVENT;
    else
        return WSC_SUCCESS;
}

uint32 WscSetEvent(uint32 handle)
{
    int err;

    err = pthread_cond_signal((pthread_cond_t *)handle);
    if(0 != err)
        return PORTAB_ERR_EVENT;
    else
        return WSC_SUCCESS;
}

uint32 WscResetEvent(uint32 handle)
{
    int err;

    err = pthread_cond_broadcast((pthread_cond_t *)handle);
    if(0 != err)
        return PORTAB_ERR_EVENT;
    else
        return WSC_SUCCESS;
}

uint32 WscSingleWait(uint32 handle, uint32 *lock, uint32 timeout)
{
    int err;
    uint32 ret = WSC_SUCCESS;
    struct timeval    now;
    struct timespec waittime;
//    extern int errno;

    WscLock(lock);
    /*pthread_mutex_lock((pthread_mutex_t *)lock);*/

    if(0 == timeout)
    {
        /*Infinite wait*/
        err = pthread_cond_wait((pthread_cond_t *)&handle, 
                                (pthread_mutex_t *)lock); 
    }
    else
    {
        gettimeofday(&now, NULL);
        waittime.tv_sec = now.tv_sec + timeout;
        waittime.tv_nsec = now.tv_usec * 1000;
        err = pthread_cond_timedwait((pthread_cond_t *)&handle, 
                                     (pthread_mutex_t *)lock, &waittime); 
    }

    if(0 != err)
    {
        switch(errno)
        {
        case ETIMEDOUT:
            ret = PORTAB_ERR_WAIT_TIMEOUT;   
        break;
        case EINVAL:
        case EPERM:
            ret = PORTAB_ERR_WAIT_ABANDONED;   
        break;
        default:
            ret = WSC_ERR_SYSTEM;              
        }
    }

    WscUnlock(lock);
    /*pthread_mutex_unlock((pthread_mutex_t *)lock);*/
    return ret;
}

uint32 WscHtonl(uint32 intlong)
{
    return htonl(intlong);
}

uint16 WscHtons(uint16 intshort)
{
    return htons(intshort);
}

uint32 WscNtohl(uint32 intlong)
{
    return ntohl(intlong);
}

uint16 WscNtohs(uint16 intshort)
{
    return ntohs(intshort);
}

void WscSleep(uint32 seconds)
{
    sleep(seconds);
}

