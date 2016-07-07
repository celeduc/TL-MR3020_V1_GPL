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
//  File Name: slist.h
//  Description: Provides prototypes for all the methods used in slist.c
//
****************************************************************************/

#if !defined SLIST_HDR
#define SLIST_HDR

//#include <tucommon.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef int BOOL;

/* this is an item in the list and stores pointers to data, next and
   previous list items */
typedef struct tag_LISTITEM
{
    void *      data;
    struct tag_LISTITEM *  next;      
    struct tag_LISTITEM *  prev;      
} LISTITEM, * LPLISTITEM;

typedef const LISTITEM * LPCLISTITEM;
    

/* this is created when Createlist is called and stores the head and tail
   of the list. */
typedef struct tag_LIST
{
    LPLISTITEM  head;
    LPLISTITEM  tail;
    short       count;
} LIST, * LPLIST;

typedef const LIST * LPCLIST;
    

/* this is created whenever an iterator is needed for iterating the list */
typedef struct tag_LISTITR
{
    LPLIST      list;
    LPLISTITEM  current;
} LISTITR, * LPLISTITR;

typedef const LISTITR * LPCLISTITR;
    
/* Function prototypes */

/* List operations */
extern LPLIST ListCreate(void);
extern BOOL ListAddItem(LPLIST list, void * data);
extern BOOL ListRemoveItem(LPLIST list, void * data);
extern BOOL ListFindItem(LPLIST list, void * data);
extern void * ListGetFirst(LPLIST list);
extern void * ListGetLast(LPLIST list);
extern int ListGetCount(LPLIST list);
extern void ListDelete(LPLIST list);
extern void ListFreeDelete(LPLIST list);


/* List Iterator operations */
extern LPLISTITR ListItrCreate(LPLIST list);
extern LPLISTITR ListItrFirst(LPLIST list, LPLISTITR listItr);
extern void * ListItrGetNext(LPLISTITR listItr);
extern BOOL ListItrInsertAfter(LPLISTITR listItr, void * data);
extern BOOL ListItrInsertBefore(LPLISTITR listItr, void * data);
extern BOOL ListItrRemoveItem(LPLISTITR listItr);
extern void ListItrDelete(LPLISTITR listItr);

#ifdef __cplusplus
}
#endif


#endif /* SLIST_HDR */

