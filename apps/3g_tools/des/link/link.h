/******************************************************************************
 *
 * Copyright (c) 2009 TP-LINK Technologies CO.,LTD.
 * All rights reserved.
 *
 * FILE NAME  :   link.h
 * VERSION    :   1.0
 * DESCRIPTION:   common link structure.
 				  use for queue or list.
 *
 * AUTHOR     :   Huanglifu <Huanglifu@tp-link.net>
 * CREATE DATE:   09/21/2011
 *
 * HISTORY    :
 * 01   09/21/2011  Huanglifu     Create.
 *
 ******************************************************************************/
#ifndef __LINK_H
#define __LINK_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*link list element struct*/
typedef struct _LINK {
	void *content;
	struct _LINK *next;
}LINK;
/*link list main obj*/
typedef struct _LINKLIST
{
	LINK *head;/*link list header*/
	LINK *tail;/*link list tail*/
	int num;
	pthread_mutex_t linkMutex;/*mutex for muti pthread call*/
	pthread_cond_t linkCond;  /*use for gernerator and comsumper*/
	LINK *current;            /*use for trace all element*/
}LINKLIST;


void LINKLIST_init(LINKLIST *linkList);
void LINKLIST_insert(LINKLIST *linkLis,LINK *element);
void LINKLIST_del(LINKLIST *linkList, LINK *element);
void LINKLIST_desLink(LINKLIST *linkList);
void LINKLIST_Start(LINKLIST *linkList);


LINK * LINK_new(LINKLIST *linkList,void *content);
LINK *getNextLinkElement(LINKLIST *linkList );
int hasNextLinkElement(LINKLIST *linkList);
LINK *getConsumeALinkElement(LINKLIST *linkList);
#endif
