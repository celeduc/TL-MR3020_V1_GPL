/* STAT make statistics for the LAN hosts' traffic through linux router
 *
 * (C) 2007 TP-LINK Technologies Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * 070702 Li Shaozhang <lishaozhang@tp-link.net>
 */
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/version.h> /* for judgement kernel version, by lqt */


#include "statistics.h"

/* change license from BSD to BSD/GPL. beacasue kernel said use BSD will taint kernel. by HouXB, 11Nov10 */
/* MODULE_LICENSE("BSD"); */
MODULE_LICENSE("Dual BSD/GPL");

MODULE_AUTHOR("Li Shaozhang <lishaozhang@tp-link.net>");
MODULE_DESCRIPTION("network statistics");

/* Spin lock for STAT entries */
static DEFINE_SPINLOCK(stat_lock);


/* which subnet we care about */
static u_int32_t subnet_ip = 0x00000000;
static u_int32_t subnet_mask = 0xFFFFFF00;

static int interval = 10;

/* statistics */
static int total = 0;
static ipt_stat_host_entry_t entries[IPT_STAT_ENTRIES_MAX];

struct {
	int recv_pkts;
	int recv_bytes;
	int send_pkts;
	int send_bytes;
}wan_statistics = {0, 0, 0, 0};

/* timer */
static struct timer_list stat_timer;

#if 0
int print_entries(void)
{
	int i = 0;
	for (i = 0; i < total; i ++)
	{
		if (entries[i].used == 1)
		{
			printk("<1>subnet_ip:%#X  all_pkts:%d  all_bytes:%d  "  
					"tx_pkts:%d  tx_bytes:%d  tx_pkts_icmp:%d  "
					"tx_pkts_udp:%d  tx_pkts_tcp:%d  tx_pkts_tcp_syn:%d\n",
					entries[i].ip, entries[i].all_pkts, entries[i].all_bytes,
					entries[i].tx_pkts, entries[i].tx_bytes, entries[i].tx_pkts_icmp,
					entries[i].tx_pkts_udp, entries[i].tx_pkts_tcp, entries[i].tx_pkts_tcp_syn);
		}
	}

	return 0;
}

static u_int32_t stat_hash(u_int32_t nIp)
{
	u_int32_t hash = nIp;
	
	hash ^= 0x3e74bd85;
	hash ^= (hash >> 16);
	hash = (hash ^ (hash >> 8)) & 0xFF;
	
	return hash;
}

#endif

int find_existed_index(u_int32_t nIp)
{
	int i = 0;

	for (i = 0; i < total; i ++)
	{
		if (entries[i].ip == nIp)
			return i;
	}

	return -1;
}
int find_index(u_int32_t nIp)
{
	int i = 0;

	int index = find_existed_index(nIp);

	if (index >= 0)
		return index;

	for (i = total; i < IPT_STAT_ENTRIES_MAX; i ++)
	{
		if (entries[i].used == 0)
		{
			entries[i].used = 1;
			entries[i].ip = nIp;
			total ++;
			return i;
		}
	}

	return -1;
}

/* our hook function */

/* add support for kernel 2.6.31 by HouXB, 11Nov10 */
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31) ) 

	unsigned int record_stat(unsigned int hooknum,
	                       struct sk_buff *skb,
	                       const struct net_device *in,
	                       const struct net_device *out,
	                       int (*okfn)(struct sk_buff *))
	{
		struct sk_buff *pskb = skb;
		const struct iphdr *iph = ip_hdr(pskb);

#else
unsigned int record_stat(unsigned int hooknum,
                       struct sk_buff **skb,
                       const struct net_device *in,
                       const struct net_device *out,
                       int (*okfn)(struct sk_buff *))
{
	struct sk_buff *pskb = *skb;
	const struct iphdr *iph = pskb->nh.iph;
#endif /* end #if ( LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31) ) */
/* end add support for kernel 2.6.31 by HouXB, 11Nov10 */

    struct tcphdr *tcph = (void *)iph + iph->ihl*4;	/* Might be TCP, UDP */
	
	int index = 0;
	u_int32_t proto = iph->protocol;
	u_int32_t len 	= ntohs(iph->tot_len) + ETHER_PAYLOAD_LEN;


	if (subnet_ip == 0 && subnet_mask == 0)
		return NF_ACCEPT;
	
	/* tx */
	if ((ntohl(iph->saddr) & subnet_mask) == (subnet_ip & subnet_mask)) 
	{
		/* begin to do our work */
		spin_lock_bh(&stat_lock);			/* LOCK */
		
		index = find_index(ntohl(iph->saddr));
		if (index == -1)
		{
			spin_unlock_bh(&stat_lock);		/* UNLOCK */
			return NF_ACCEPT;
		}

		/* obtain mac */
		/* Is mac pointer valid? */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
		if (skb_mac_header(pskb) >= pskb->head && (skb_mac_header(pskb) + ETH_HLEN) <= pskb->data)
#else		
    	if (pskb->mac.raw >= pskb->head && (pskb->mac.raw + ETH_HLEN) <= pskb->data)
#endif			
	    {
			memcpy(entries[index].mac, eth_hdr(pskb)->h_source, ETH_ALEN);
		}
		
		entries[index].total.all_pkts ++;
		entries[index].total.all_bytes += len;

		wan_statistics.send_pkts ++;
		wan_statistics.send_bytes += len;
		
		switch (proto)
		{
			case IPPROTO_ICMP:
				entries[index].total.icmp_pkts_tx ++;
				break;
			case IPPROTO_TCP:
				entries[index].total.tcp_pkts_tx ++;
				if (tcph->syn == 0x1)
					entries[index].total.tcp_syn_pkts_tx ++;
				break;
			case IPPROTO_UDP:
				entries[index].total.udp_pkts_tx ++;
				break;
			default:
				break;
		}

		/* work done */
		spin_unlock_bh(&stat_lock);			/* UNLOCK */
		
	}	
	/* rx */
	else if ((ntohl(iph->daddr) & subnet_mask) == (subnet_ip & subnet_mask)) 
	{
		/* begin to do our work */
		spin_lock_bh(&stat_lock);			/* LOCK */
		
		index = find_index(ntohl(iph->daddr));		
		if (index == -1)
		{
			spin_unlock_bh(&stat_lock);		/* UNLOCK */
			return NF_ACCEPT;
		}

		/* obtain mac -- it is no need and wrong to obtain dst mac here, 
		 * because the pkt now is in the FORWARD chain, and DNAT will be 
		 * made in POSTROUTING. so if we get the mac now, it will be the
		 * router's WAN interface mac, but not the host mac which we really want.
		 */
		#if 0
		/* Is mac pointer valid? */
    	if (pskb->mac.raw >= pskb->head && (pskb->mac.raw + ETH_HLEN) <= pskb->data)
	    {
			memcpy(entries[index].mac, eth_hdr(pskb)->h_dest, ETH_ALEN);
		}
		#endif

		entries[index].total.all_pkts ++;
		entries[index].total.all_bytes += len;

		wan_statistics.recv_pkts ++;
		wan_statistics.recv_bytes += len;

		/* work done */
		spin_unlock_bh(&stat_lock);			/* UNLOCK */	
	}

	return NF_ACCEPT;
}

void ipt_stat_proc(unsigned long nul)
{
	int i;
	for (i = 0; i < total; i++)
	{
		spin_lock_bh(&stat_lock);			/* LOCK */
		
		/* FIXME: if total overflow and less than refer ? */
		if (entries[i].total.all_pkts != entries[i].refer.all_pkts)
		{
			entries[i].recent.all_pkts = entries[i].total.all_pkts - entries[i].refer.all_pkts;
			entries[i].recent.all_bytes = entries[i].total.all_bytes - entries[i].refer.all_bytes;
			entries[i].recent.icmp_pkts_tx = entries[i].total.icmp_pkts_tx;
			entries[i].recent.udp_pkts_tx = entries[i].total.udp_pkts_tx;
			entries[i].recent.tcp_pkts_tx = entries[i].total.tcp_pkts_tx;
			entries[i].recent.tcp_syn_pkts_tx = entries[i].total.tcp_syn_pkts_tx;

			if (entries[i].max_icmp_pkts_tx < entries[i].recent.icmp_pkts_tx)
			{
				entries[i].max_icmp_pkts_tx = entries[i].recent.icmp_pkts_tx;
			}
			if (entries[i].max_tcp_syn_pkts_tx < entries[i].recent.tcp_syn_pkts_tx)
			{
				entries[i].max_tcp_syn_pkts_tx = entries[i].recent.tcp_syn_pkts_tx;
			}
			if (entries[i].max_udp_pkts_tx < entries[i].recent.udp_pkts_tx)
			{
				entries[i].max_udp_pkts_tx = entries[i].recent.udp_pkts_tx;
			}

			entries[i].refer.all_pkts  = entries[i].total.all_pkts;
			entries[i].refer.all_bytes = entries[i].total.all_bytes;
			
			entries[i].total.icmp_pkts_tx		= 0;
			entries[i].total.tcp_pkts_tx		= 0;
			entries[i].total.tcp_syn_pkts_tx	= 0;
			entries[i].total.udp_pkts_tx 		= 0;
		}
		else
		{
			entries[i].recent.all_pkts = 0;
			entries[i].recent.all_bytes = 0;
			entries[i].recent.icmp_pkts_tx = 0;
			entries[i].recent.udp_pkts_tx = 0;
			entries[i].recent.tcp_pkts_tx = 0;
			entries[i].recent.tcp_syn_pkts_tx = 0;
		}
		
		spin_unlock_bh(&stat_lock);			/* UNLOCK */
	}

	mod_timer(&stat_timer, jiffies + interval * HZ);
}

int ipt_stat_del_one(u_int32_t nIp)
{
	int index = find_existed_index(nIp);
	
	if (index < 0 || index >= total)
				return -1;
	
	if (index == total - 1)
	{
		/* clear the last one and total num reduce by 1 */
		memset(&(entries[--total]), 0, sizeof(ipt_stat_host_entry_t));
	}
	else
	{
		/* last one cover it */
		entries[index] = entries[--total];
		/* clear the last one */
		memset(&(entries[total]), 0, sizeof(ipt_stat_host_entry_t));
	}
	
	return 0;
}

int ipt_stat_del_all(void)
{
	total = 0;
	memset(entries, 0, sizeof(ipt_stat_host_entry_t) * IPT_STAT_ENTRIES_MAX);
	
	return 0;
}

int ipt_stat_reset_one_by_index(int index)
{
	if (index < 0 || index >= total)
		return -1;
	
	memset(&entries[index].total,  0, sizeof(ipt_stat_statistic_t));
	memset(&entries[index].recent, 0, sizeof(ipt_stat_statistic_t));
	memset(&entries[index].refer,  0, sizeof(ipt_stat_reference_t));
	
	entries[index].max_icmp_pkts_tx 	= 0;
	entries[index].max_udp_pkts_tx		= 0;
	entries[index].max_tcp_syn_pkts_tx	= 0;
		
	return 0;
}

int ipt_stat_reset_one(u_int32_t nIp)
{
	int index = find_existed_index(nIp);
	
	return ipt_stat_reset_one_by_index(index);
}

int ipt_stat_reset_all(void)
{
	int i;
	for (i = 0; i < total; i++)
	{
		ipt_stat_reset_one_by_index(i);
	}
	
	return 0;
}


int do_stat_set_ctl(struct sock *sk, int cmd, void __user *user, unsigned int len)
{
	struct ipt_stat_net stat_net;
	u_int32_t entryIp = 0;
	int ret = 0;

	switch(cmd)
	{
		case IPT_STAT_SET_NET:
			if (user == NULL)
			{
				ret = -1;
				break;
			}
			
			copy_from_user(&stat_net, user, len);			
			if ((subnet_ip & subnet_mask) != (stat_net.ip & stat_net.mask))
			{
				spin_lock_bh(&stat_lock);		/* LOCK */
				
				ret = ipt_stat_del_all();
				subnet_ip = stat_net.ip;
				subnet_mask = stat_net.mask;

				spin_unlock_bh(&stat_lock);		/* UNLOCK */
			}			
			break;

		case IPT_STAT_SET_INTERVAL:
			if (user == NULL || len != 4)		/* size of int */
			{
				ret = -1;
				break;
			}
			
			copy_from_user(&interval, user, len);
			break;
			
		case IPT_STAT_RESET_ONE:
			if (user == NULL || len != 4)		/* size of int */
			{
				ret = -1;
				break;
			}
			copy_from_user(&entryIp, user, len);
			spin_lock_bh(&stat_lock);			/* LOCK */
			ret = ipt_stat_reset_one(entryIp);
			spin_unlock_bh(&stat_lock);			/* UNLOCK */
			break;
			
		case IPT_STAT_RESET_ALL:
			spin_lock_bh(&stat_lock);			/* LOCK */
			ret = ipt_stat_reset_all();
			spin_unlock_bh(&stat_lock);			/* UNLOCK */
			break;
			
		case IPT_STAT_DEL_ONE:
			if (user == NULL || len != 4)		/* size of int */
			{
				ret = -1;
				break;
			}
			copy_from_user(&entryIp, user, len);
			
			spin_lock_bh(&stat_lock);			/* LOCK */
			ret = ipt_stat_del_one(entryIp);
			spin_unlock_bh(&stat_lock);			/* UNLOCK */
			break;
			
		case IPT_STAT_DEL_ALL:
			spin_lock_bh(&stat_lock);			/* LOCK */
			ret = ipt_stat_del_all();
			spin_unlock_bh(&stat_lock);			/* UNLOCK */
			break;

		default:
			printk("<1>recv a unknowed ctl command\n");
			ret = -1;
			break;
	}
	
	return ret;
}

int do_stat_get_ctl(struct sock *sk, int cmd, void __user *user, int* len)
{
	switch(cmd)
	{
		case IPT_STAT_GET_ALL:
			
			if (user == NULL || len == NULL)
				return 0;
						
			*len = total;
			copy_to_user(user, entries, total * sizeof(ipt_stat_host_entry_t));

			break;
			
		case IPT_STAT_GET_WAN_STAT:

			if (user == NULL || len == NULL)
				return 0;

			*len = sizeof(wan_statistics);
			copy_to_user(user, &wan_statistics, *len);

			break;
			
		default:
			printk("<1>recv a unknowed ctl command\n");
			break;
	}
	
	return 0;
}

/* netfilter hook */
static struct nf_hook_ops nfho = {
	.hook		= record_stat,		/* hook function */
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31) )
	.hooknum    = NF_INET_FORWARD,
#else
    .hooknum	= NF_IP_FORWARD, 	/* hook in forward */
#endif
    .pf			= PF_INET,
    .priority	= NF_IP_PRI_LAST,	/* last place */
};

static struct nf_sockopt_ops stat_sockopts = {
	.pf			= PF_INET,
	.set_optmin	= IPT_STAT_BASE_CTL,
	.set_optmax	= IPT_STAT_SET_MAX+1,
	.set		= do_stat_set_ctl,
	.get_optmin	= IPT_STAT_BASE_CTL,
	.get_optmax	= IPT_STAT_GET_MAX+1,
	.get		= do_stat_get_ctl,
};



static int __init init(void)
{
	uint32_t ret = 0;
	
    nf_register_hook(&nfho);

	ret = nf_register_sockopt(&stat_sockopts);
	if (ret < 0) {
		printk("<1>Unable to register sockopts.\n");
		return ret;
	}

	total = 0;
	memset(&entries, 0, sizeof(ipt_stat_host_entry_t) * IPT_STAT_ENTRIES_MAX);

	init_timer(&stat_timer);
    stat_timer.data     = (unsigned long)0;
    stat_timer.function = ipt_stat_proc;
    ipt_stat_proc(0);
	
    return 0;
}


static void __exit fini(void)
{
    nf_unregister_hook(&nfho);

	nf_unregister_sockopt(&stat_sockopts);

	del_timer(&stat_timer);
}

module_init(init);
module_exit(fini);


