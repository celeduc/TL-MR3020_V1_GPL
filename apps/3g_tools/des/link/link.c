/******************************************************************************
 *
 * Copyright (c) 2009 TP-LINK Technologies CO.,LTD.
 * All rights reserved.
 *
 * FILE NAME  :   link.c
 * VERSION    :   1.0
 * DESCRIPTION:   common link structure.use for queue or list.
 *
 * AUTHOR     :   Huanglifu <Huanglifu@tp-link.net>
 * CREATE DATE:   09/21/2011
 *
 * HISTORY    :
 * 01   09/21/2011  Huanglifu     Create.
 *
 ******************************************************************************/
#include "link.h"
#include "../log/log.h"


/******************************************************************************
 * FUNCTION      :   LINKLIST_init()
 * DESCRIPTION   :   link list init operation. 

 ******************************************************************************/
void LINKLIST_init(LINKLIST *linkList)
{
	linkList->head = NULL;
	linkList->tail = NULL;
	linkList->num = 0;
//	linkList->linkMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_init(&linkList->linkMutex,NULL);
	pthread_cond_init(&linkList->linkCond,NULL);
}

/******************************************************************************
 * FUNCTION      :   LINK_new()
 * DESCRIPTION   :   malloc a new link list's element. 

 ******************************************************************************/
LINK * LINK_new(LINKLIST *linkList, void *content)
{
	pthread_mutex_lock(&linkList->linkMutex);
	LINK *new = (LINK *) malloc(sizeof(LINK));

	if(new == NULL)
	{	
		myLog(ERROR,"malloc fail.");
		return NULL;
	}
	new->content = content;
	new->next = NULL;
	pthread_mutex_unlock(&linkList->linkMutex);
	return new;
}

/******************************************************************************
 * FUNCTION      :   getConsumeALinkElement()
 * DESCRIPTION   :   get a header element in link list wait until have element. 

 ******************************************************************************/
LINK *getConsumeALinkElement(LINKLIST *linkList)
{
	pthread_mutex_lock(&linkList->linkMutex);
		while (linkList->head == NULL) {
			//myLog(INFO, "%s", "wait for link element.");
			pthread_cond_wait(&linkList->linkCond, &linkList->linkMutex);
		}
	pthread_mutex_unlock(&linkList->linkMutex);
	
	return linkList->head;	
}
/******************************************************************************
 * FUNCTION      :   LINKLIST_insert()
 * DESCRIPTION   :   insert a element into linklist. 

 ******************************************************************************/
void LINKLIST_insert(LINKLIST *linkList, LINK *element)
{
	if (element == NULL)
	{
		myLog(WARN, "%s", "link list insert element is null!");
		return;	
	}

	pthread_mutex_lock(&linkList->linkMutex);
	element->next = NULL;

	if (linkList->head == NULL)
	{
		linkList->head = element;		
		linkList->tail = element;		
	}else
	{
		
		linkList->tail->next = element;		
		linkList->tail = element;		
	}
			
	pthread_cond_signal(&linkList->linkCond);
	pthread_mutex_unlock(&linkList->linkMutex);
			
}
/******************************************************************************
 * FUNCTION      :   LINKLIST_del()
 * DESCRIPTION   :   del a element in link list. 

 ******************************************************************************/
void LINKLIST_del(LINKLIST *linkList, LINK *element)
{
	if (element == NULL)
	{
		myLog(WARN, "%s", "link list del element is null!");
		return;	
	}
	pthread_mutex_lock(&linkList->linkMutex);
	LINK *ptr = linkList->head;
	LINK *ptrPrev = linkList->head;
	
	while (ptr != NULL){
		if(ptr == element)	
		{
			if(ptr == linkList->head)
			{
				linkList->head = ptr->next;
				free(element);
			}else
			{
				ptrPrev->next = ptr->next;
				if(ptrPrev->next == NULL) 
					linkList->tail = ptrPrev;
				free(element);
				
			}
				
			break;	
		}
		ptrPrev = ptr;
		ptr = ptr->next ;
	}
	pthread_mutex_unlock(&linkList->linkMutex);
}
/******************************************************************************
 * FUNCTION      :   LINKLIST_desLink()
 * DESCRIPTION   :   destructure link list structure. 
 ******************************************************************************/
void LINKLIST_desLink(LINKLIST *linkList )
{

	pthread_mutex_lock(&linkList->linkMutex);
	LINK *ptr = linkList->head;
	LINK *ptrPrev = linkList->head;
	while (ptr != NULL)
	{
		ptrPrev =ptr;
		void *pvoid;
		pvoid = ptrPrev->content;
		ptr = ptr->next;
		free(pvoid);
		free(ptrPrev);
	}
	pthread_mutex_unlock(&linkList->linkMutex);
}

/******************************************************************************
 * FUNCTION      :   LINKLIST_Start()
 * DESCRIPTION   :   get started element  of link list. 
 *
 * INPUT         :   N/A
 * OUTPUT        :   N/A
 * RETURN        :   N/A
 ******************************************************************************/
void LINKLIST_Start(LINKLIST *linkList )
{
	pthread_mutex_lock(&linkList->linkMutex);
	linkList->current = linkList->head;
	pthread_mutex_unlock(&linkList->linkMutex);
}
/******************************************************************************
 * FUNCTION      :   hasNextLinkElement()
 * DESCRIPTION   :   judge have next element. 
 ******************************************************************************/
int hasNextLinkElement(LINKLIST *linkList)
{
	pthread_mutex_lock(&linkList->linkMutex);
	int res =(linkList->current != NULL);
	pthread_mutex_unlock(&linkList->linkMutex);
	return res;
}
/******************************************************************************
 * FUNCTION      :   getNextLinkElement()
 * DESCRIPTION   :   get a element in link list. 
 ******************************************************************************/
LINK *getNextLinkElement(LINKLIST *linkList )
{
	pthread_mutex_lock(&linkList->linkMutex);
	LINK *res;
	res = linkList->current;
	linkList->current = linkList->current->next;
	pthread_mutex_unlock(&linkList->linkMutex);
	return res;
}

