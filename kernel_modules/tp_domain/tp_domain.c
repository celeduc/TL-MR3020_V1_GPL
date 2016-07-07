/******************************************************************************
*
* Copyright (c) 2010 TP-LINK Technologies CO.,LTD.
* All rights reserved.
*
* FILE NAME	:	tp_domain.c
* VERSION	:	1.0
* DESCRIPTION:	????·????ҳ??????????¼.
*
* AUTHOR	:	huangwenzhong <huangwenzhong@tp-link.net>
* CREATE DATE:	12/02/2010
*
* HISTORY	:
* 01	12/02/2010	huangwenzhong		Create.
*
******************************************************************************/

#include <linux/vermagic.h>
#include <linux/compiler.h>
#include <linux/sockios.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include "tp_domain.h"
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/sock.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/flow.h>

#include <linux/version.h>

#include <linux/netfilter_bridge.h>
#include <linux/version.h>
#include <linux/random.h>
#include <linux/rculist.h>
#include <linux/jhash.h>
#include <linux/rculist.h>
#include <linux/spinlock.h>
#include <linux/times.h>
#include <linux/etherdevice.h>
#include <asm/atomic.h>
#include <asm/unaligned.h>
#include <linux/inetdevice.h>

#define MAX_LAN_DOMAIN_LEN 64

static __u32 lan_ip = 0xc0a80101;
static char *lan_domain = "tplinklogin.net";

module_param(lan_ip, uint, 0);
module_param(lan_domain, charp, 0);

static char query[MAX_LAN_DOMAIN_LEN];

#define DNS_QUERY_LEN	strlen(query) + 1
#define DNS_DATA_LEN	(DNS_QUERY_LEN + 4 + 16)

struct proc_dir_entry *tp_domain_config_entry = NULL;
EXPORT_SYMBOL(tp_domain_config_entry);
static struct proc_dir_entry *root_ap_connected_entry = NULL;

int g_rootap_status = 1; /* 1 connect rootap; 0 disconnection rootap */
static struct proc_dir_entry * rootap_status_entry = NULL;

static int rootap_status_entry_read (char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
    return sprintf (page, "%d\n", g_rootap_status);
}

static int rootap_status_entry_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
	u_int32_t val;
	
	if (sscanf(buf, "%d", &val) != 1)
        return -EINVAL;

	if ((val < 0) || (val > 1))
	{
		return -EINVAL;
	}

	g_rootap_status = val;

	return count;
}

static int create_tp_domain_config_proc()
{
	if (tp_domain_config_entry  != NULL)
	{
		printk ("Already have a proc entry for /proc/tp_domain_config!\n");
		return -ENOENT;
	}

	tp_domain_config_entry  = proc_mkdir("tp_domain_config", NULL);
	if (!tp_domain_config_entry )
	{
		return -ENOENT;
	}

	rootap_status_entry  = create_proc_entry("rootap_status", 0666, tp_domain_config_entry);
	if (!rootap_status_entry )
	{
		return -ENOENT;
	}

	rootap_status_entry ->read_proc = rootap_status_entry_read;
	rootap_status_entry ->write_proc = rootap_status_entry_write; 
	
	return 0;
}

/**
 * by dyf, on 19Apr2013, parse domain string to packet matching buf @query.
 */
static void parse_domain(void)
{
	char *pstr = NULL;
	char *tmp = query;

	strncpy(query + 1, lan_domain, MAX_LAN_DOMAIN_LEN - 1);

	pstr = strchr(tmp + 1, '.');
	while (pstr) {
		*tmp = pstr - tmp - 1;
		tmp = pstr;
		pstr = strchr(tmp + 1, '.');
	}

	*tmp = strlen(query) - (tmp - query) - 1;
}

/******************************************************************************
* FUNCTION		: udp_csum_calc()
* AUTHOR		: huangwenzhong <huangwenzhong@tp-link.net>
* DESCRIPTION	: calculate UDP checksum
* INPUT			: iph - IP header
*				  udp - UDP header
*				  payload_len - UDP payload length
* OUTPUT		: N/A
* RETURN		: N/A
* OTHERS		:
******************************************************************************/
static unsigned short udp_csum_calc(struct iphdr *iph, struct udphdr *udph, int payload_len)
{
	unsigned long csum = 0;
	unsigned long csumed_len = payload_len + sizeof(*udph);
	unsigned short *word_ptr = (unsigned short *)udph;

	while ( csumed_len >1 )
	{
		csum += *word_ptr++;
		csumed_len -= sizeof(unsigned short);
	}

	if (csumed_len)
	{
		csum += (*((unsigned char *)word_ptr)) << 8;

	}
	
	csum += ( htonl(iph->saddr) )  & 0x0000ffff;
	csum += ( htonl(iph->saddr) >> 16 ) & 0x0000ffff;

	csum += htonl( iph->daddr ) & 0x0000ffff;
	csum += ( htonl(iph->daddr) >> 16 ) & 0x0000ffff;

	csum += udph->len;

	csum += htons(IPPROTO_UDP) & 0x00ff;

	while (csum > 0x10000)
	{ 
		csum = (csum & 0x0000ffff) + (csum >> 16);
	}
	return (unsigned short)(~csum);
}

/******************************************************************************
* FUNCTION		: tp_send_dns_packet()
* AUTHOR		: huangwenzhong <huangwenzhong@tp-link.net>
* DESCRIPTION	: construct and send DNS packet
* INPUT			: skb - DNS request
*				  old_iphdr - DNS request Packet IP header
*				  old_udphdr - DNS request Packet UDP header
*				  old_dnshdr - DNS request Packet DNSP header
*
* OUTPUT		: N/A
* RETURN		: N/A
* OTHERS		:
******************************************************************************/
static void tp_send_dns_packet(struct sk_buff *skb ,
				struct iphdr *old_iphdr,
				struct udphdr *old_udphdr,
				DNS_HEADER *old_dnshdr,
				unsigned char * skb_payload
)
{
	struct sk_buff *dns_skb;

	int skb_len;
	int data_len;
	int ret;
	int dns_query_len;

	struct iphdr *new_iphdr;	/* DNS Reply Packet IP header */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
	struct net_device* br0_dev;
#endif

	struct udphdr *new_udphdr;
	DNS_HEADER *new_dnshdr;
	unsigned char *dns_payload;
	unsigned char *tmp;
	struct in_device * in_dev = NULL;

	__u16 short_val;
	__u16 udp_sum;
	__u32 int_val;

	struct rtable *rt;

	struct flowi fl = {
		.nl_u = {
			.ip4_u = {
				.daddr = old_iphdr->saddr,
				}
			},
	};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
	br0_dev = dev_get_by_name(&init_net, "br0");
	ret = ip_route_output_key(dev_net(br0_dev), &rt, &fl);
	in_dev = (struct in_device *)br0_dev->ip_ptr;
	dev_put(br0_dev);
#else
	ret = ip_route_output_key(&rt, &fl);
#endif
	if (ret)
	{
		printk("ip_route_output_key() failed in file create_tp_dns_skb.c, ret = %d\r\n", ret);
		return;
	}

	if (g_rootap_status == 1)
	{
		data_len = DNS_DATA_LEN;
	}
	else
	{
		//printk("dns_query_len = %d\n", skb->tail - skb_payload);
		dns_query_len = skb->tail - skb_payload;
		data_len = dns_query_len + 16;
	}

	skb_len = ETH_HEADER_LEN + old_iphdr->ihl * 4 +
					UDP_HEADER_LEN + DNS_HEADER_LEN + data_len;

	dns_skb = alloc_skb(skb_len, GFP_ATOMIC);
	if (!dns_skb)
	{
		return;
	}
	nf_reset(dns_skb);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
	dns_skb->mark = 0;
#else
	dns_skb->nfmark = 0;
#endif

	nf_ct_attach(dns_skb, skb);	/* needed by NAT */
	dns_skb->pkt_type = PACKET_OTHERHOST;
	dns_skb->protocol = htons(ETH_P_IP);
	dns_skb->ip_summed = CHECKSUM_NONE;
	dns_skb->priority = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
	skb_dst_set(dns_skb, &rt->u.dst);
#else
	dns_skb->dst = &rt->u.dst;
#endif

	skb_reserve(dns_skb, ETH_HEADER_LEN);

	/* create IP header */
	new_iphdr = (struct iphdr *)skb_put(dns_skb, sizeof(struct iphdr));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
	/* we must specify the network header to avoid kernel panic.
	   when ip_nat hook was called, it use iphr(skb)to get network header.
	   by HouXB, 12May11 */
	//skb_set_network_header(dns_skb, ETH_HEADER_LEN);
	dns_skb->network_header = new_iphdr;
#else
	dns_skb->nh.iph = new_iphdr;
#endif

	memcpy((unsigned char *)new_iphdr, (unsigned char *)old_iphdr, old_iphdr->ihl *4);

	new_iphdr->ttl = IP_PACKET_TTL;
	new_iphdr->frag_off |= htons(0x4000);

	memcpy((unsigned char *)&new_iphdr->saddr,
					(unsigned char *)&old_iphdr->daddr, IP_ADDR_LEN);
	memcpy((unsigned char *)&new_iphdr->daddr,
					(unsigned char *)&old_iphdr->saddr, IP_ADDR_LEN);

	ip_select_ident(new_iphdr, &rt->u.dst, NULL);/* always 0 */

	new_iphdr->tot_len = new_iphdr->ihl * 4 + UDP_HEADER_LEN
									+ DNS_HEADER_LEN + data_len;
	new_iphdr->tot_len = htons(new_iphdr->tot_len);
	new_iphdr->check = 0x0000;
	new_iphdr->check = ip_fast_csum((unsigned char *)new_iphdr, new_iphdr->ihl);

	/* create udp header */
	new_udphdr = (struct udphdr *)skb_put(dns_skb, sizeof(struct udphdr));
	memcpy((unsigned char *)&new_udphdr->dest,
				(unsigned char *)&old_udphdr->source, PORT_LEN);
	memcpy((unsigned char *)&new_udphdr->source,
				(unsigned char *)&old_udphdr->dest, PORT_LEN);
	new_udphdr->len = htons(UDP_HEADER_LEN + DNS_HEADER_LEN + data_len);
	new_udphdr->check = 0x0000;

	/* create dns header */
	new_dnshdr = (DNS_HEADER *)skb_put(dns_skb, sizeof(DNS_HEADER));
	memcpy((unsigned char *)&new_dnshdr->transaction_id,
								(unsigned char *)&old_dnshdr->transaction_id, sizeof(__u16));
	memcpy((unsigned char *)&new_dnshdr->flag,
								(unsigned char *)&old_dnshdr->flag, sizeof(__u16));
	new_dnshdr->flag |= htons(DNS_RESPONSE_FLAG);
	memcpy((unsigned char *)&new_dnshdr->questions,
								(unsigned char *)&old_dnshdr->questions, sizeof(__u16));
	new_dnshdr->answers_rrs = htons(0x0001);
	new_dnshdr->authority_rrs = 0x0000;
	new_dnshdr->additional_rrs = 0x0000;

	/* create dns questions */
	dns_payload = (unsigned char *)skb_put(dns_skb, data_len);
	tmp = dns_payload;

	if (g_rootap_status == 1 )
	{
		memcpy(tmp, query, DNS_QUERY_LEN);
		tmp = dns_payload + DNS_QUERY_LEN - 1;
		*tmp = 0x00;
		tmp++;

		short_val = htons(DNS_QUERY_TYPE);
		memcpy(tmp, &short_val, sizeof(short_val));

		tmp += sizeof(short_val);

		short_val = htons(DNS_QUERY_CLASS);
		memcpy(tmp, &short_val, sizeof(short_val));
	}
	else
	{
		memcpy( tmp, skb_payload, dns_query_len-2 );

		tmp +=	skb->tail - skb_payload -2;
	}

	/* create dns answer */
	tmp += sizeof(short_val);
	short_val = htons(DNS_RESPONSE_POINTER);
	memcpy(tmp, &short_val, sizeof(short_val));

	tmp += sizeof(short_val);
	short_val = htons(DNS_QUERY_TYPE);
	memcpy(tmp, &short_val, sizeof(short_val));

	tmp += sizeof(short_val);
	short_val = htons(DNS_QUERY_CLASS);
	memcpy(tmp, &short_val, sizeof(short_val));

	tmp += sizeof(short_val);
	int_val = htonl(DNS_ANSWER_TTL);
	memcpy(tmp, &int_val, sizeof(int_val));

	tmp += sizeof(int_val);
	short_val = htons(DNS_RESOURCE_LEN);
	memcpy(tmp, &short_val, sizeof(short_val));

	tmp += sizeof(short_val);
	lan_ip = in_dev->ifa_list->ifa_address;
	int_val =  lan_ip ;
	memcpy(tmp, &int_val, sizeof(int_val));

	unsigned int udphoff;
	udphoff = ip_hdrlen(dns_skb);
	dns_skb->csum = skb_checksum(dns_skb, udphoff, dns_skb->len - udphoff, 0);	
	new_udphdr->check = csum_tcpudp_magic(new_iphdr->saddr,
								  new_iphdr->daddr,
								  dns_skb->len - udphoff,
								  new_iphdr->protocol,
								  dns_skb->csum);
#if 0	
	udp_sum = udp_csum_calc(new_iphdr, new_udphdr, DNS_HEADER_LEN + data_len);
	new_udphdr->check = htons(udp_sum);
#endif
	dst_output(dns_skb);
	return;
}


/******************************************************************************
* FUNCTION		: tp_dns_handler()
* AUTHOR		: huangwenzhong <huangwenzhong@tp-link.net>
* DESCRIPTION	: register to FORWARD,
				  intercept ${lan_domain} DNS request and send Reply packets
* INPUT			: N/A
* OUTPUT		: N/A
* RETURN		: N/A
* OTHERS		:
******************************************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
static unsigned int tp_dns_handler(unsigned int hooknum,
							struct sk_buff *skb,
							const struct net_device *in,
							const struct net_device *out,
							int (*okfn)(struct sk_buff *))
{
	struct sk_buff *pskb = skb;
	struct iphdr *iph = ip_hdr(pskb);
#else
static unsigned int tp_dns_handler(unsigned int hooknum,
							struct sk_buff **skb,
							const struct net_device *in,
							const struct net_device *out,
							int (*okfn)(struct sk_buff *))
{
	struct sk_buff *pskb = *skb;
	struct iphdr *iph;
#endif

	struct udphdr *udp_hdr;
	DNS_HEADER *dns_hdr;
	unsigned char * dns_payload;
	unsigned char * tmp;

	__u16 dport;
	__u16 query_type;

	int ret;

	if (ETH_P_IP != htons(pskb->protocol)) /* not IP packets */
	{
		return NF_ACCEPT;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
	/* nothing */
#else
	iph = pskb->nh.iph;
#endif

	if ( NULL == iph || 4 != iph->version)/* not IPv4 packets */
	{
		return NF_ACCEPT;
	}

	if (IPPROTO_UDP != iph->protocol) /* not UDP packets */
	{
		return NF_ACCEPT;
	}

	udp_hdr = (struct udphdr *)((unsigned char *)iph + iph->ihl * 4);
	if ( NULL == udp_hdr )
	{
		return NF_ACCEPT;
	}

	dport = udp_hdr->dest;
	dport = ntohs(dport);
	if (DOMAIN_PORT != dport) /* not dns request packet */
	{
		return NF_ACCEPT;
	}
	dns_hdr = (DNS_HEADER *)((unsigned char *)udp_hdr + UDP_HEADER_LEN);
	if ( NULL == dns_hdr)
	{
		return NF_ACCEPT;
	}
	if ((ntohs(dns_hdr->flag) & DNS_RESPONSE_PACKET)) /* not dns request? */
	{
		return NF_ACCEPT;
	}

	dns_payload = (unsigned char *)((unsigned char *)dns_hdr + DNS_HEADER_LEN);
	if ( NULL == dns_payload )
	{
		return NF_ACCEPT;
	}

	if (g_rootap_status == 1)
	{
		ret = memcmp(dns_payload, query, DNS_QUERY_LEN);

		if ( 0 == ret )
		{
			tmp = dns_payload + DNS_QUERY_LEN;
		}
		else
		{			
			return NF_ACCEPT;
		}
	
		memcpy(&query_type, tmp, sizeof(query_type));
		query_type = ntohs(query_type);

		if (DNS_QUERY_TYPE != query_type && DNS_QUERY_TYPE_AAAA != query_type) /* is query type not A? */
		{
			return NF_ACCEPT;
		}

		tp_send_dns_packet( pskb, iph, udp_hdr, dns_hdr, dns_payload);

		return NF_DROP;
	}
	else
	{
		dns_payload = ( unsigned char * )( ( unsigned char * )dns_hdr + DNS_HEADER_LEN );
		tp_send_dns_packet( skb, iph, udp_hdr, dns_hdr, dns_payload);		
		return NF_DROP;
	}

}

static struct nf_hook_ops tp_dns_forward_hook_ops = {
	.hook		=	tp_dns_handler,
	.owner		=	THIS_MODULE,
	.pf		=	PF_BRIDGE,
	.hooknum =	NF_BR_FORWARD,
	.priority	=	NF_BR_PRI_FIRST,
};

static struct nf_hook_ops tp_dns_localin_hook_ops = {
	.hook	=	tp_dns_handler,
	.owner	=	THIS_MODULE,
	.pf		=	PF_BRIDGE,
	.hooknum =	NF_BR_LOCAL_IN,
	.priority	=	NF_BR_PRI_FIRST,
};

static int __init dns_init(void)
{
	int ret;

	create_tp_domain_config_proc();
	
	/* parse domain string at module init */
	parse_domain();

	ret = nf_register_hook( &tp_dns_forward_hook_ops );
	if (ret < 0)
	{
		return ret;
	}

	ret = nf_register_hook( &tp_dns_localin_hook_ops );
	if ( ret < 0 )
	{
		return ret;
	}
	return 0;
}

static void __exit dns_exit(void)
{
	// [xuyulong start] remove the proc entry when the module exit
	if (rootap_status_entry)
	{
		remove_proc_entry("rootap_status", tp_domain_config_entry);
		rootap_status_entry = NULL;
	}

	if (tp_domain_config_entry)
	{
		remove_proc_entry("tp_domain_config", NULL);
		tp_domain_config_entry = NULL;
	}
	// [xuyulong end]

	nf_unregister_hook( &tp_dns_forward_hook_ops );
	nf_unregister_hook( &tp_dns_localin_hook_ops );	
}

module_init(dns_init);
module_exit(dns_exit);
/* change license from GPL to BSD. beacasue we must observe GPL. by Xiongliuzhong, 10Dec12 */
/*MODULE_LICENSE("GPL");*/
MODULE_LICENSE("BSD");
MODULE_AUTHOR("Huang Wenzhong <huangwenzhong@tp-link.net>");
MODULE_DESCRIPTION("The domain name tplinklogin.net for web page manage.");
/* by HouXB, 12May11 */
MODULE_ALIAS("tp_domain");

