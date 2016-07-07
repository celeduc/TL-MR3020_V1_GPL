/******************************************************************************
 *
 * Copyright (c) 2009 TP-LINK Technologies CO.,LTD.
 * All rights reserved.
 *
 * FILE NAME  :   tp_domain.h
 * VERSION    :   1.0
 * DESCRIPTION:   用于实现我司路由器产品可以域名登录进web管理页面.
 *
 * AUTHOR     :   huangwenzhong <huangwenzhong@tp-link.net>
 * CREATE DATE:   11/30/2010
 *
 * HISTORY    :
 * 01   11/30/2010  huangwenzhong     Create.
 *
 ******************************************************************************/
#ifndef _TP_DOMAIN_H
#define _TP_DOMAIN_H

#define DOMAIN_PORT				53

#define ETH_HEADER_LEN			(2 + 14)
#define IP_PACKET_TTL			128
//#define DNS_ANSWER_TTL ( 2 * 24 * 60 *60 )
#define DNS_ANSWER_TTL 0

#define IP_ADDR_LEN				sizeof(__u32)
#define PORT_LEN				sizeof(__u16)
#define UDP_HEADER_LEN			sizeof(struct udphdr)
#define DNS_HEADER_LEN			sizeof(DNS_HEADER)

#define DNS_RESPONSE_PACKET		(0x8000)	/* response packet flag */
#define DNS_QUERY_TYPE			(0x0001)	/* query type,type A */
#define DNS_QUERY_TYPE_AAAA	( 0x001c ) 		/* query type,type AAAA */
#define DNS_QUERY_CLASS			(0x0001)	/* query class,clase internet */
#define DNS_RESPONSE_FLAG		(0x8080)	/* response flag value */
#define DNS_RESPONSE_POINTER	(0xc00c)	/* response field pointer */
#define DNS_RESOURCE_LEN		(0x0004)	/* response field IP length */
/*	DNS报文头部结构.added by  huangwenzhong.  11/30/2010.	*/

typedef struct
{
	__u16 transaction_id;
	__u16 flag;
	__u16 questions;
	__u16 answers_rrs;
	__u16 authority_rrs;
	__u16 additional_rrs;
}__attribute__ ((__packed__))DNS_HEADER;


#endif

