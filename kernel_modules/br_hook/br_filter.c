/* bridge filter
 *
 * (C) 2007 TP-LINK Technologies Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * 080613 Li Shaozhang <lishaozhang@tp-link.net>
 */
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/module.h>
#include <linux/netfilter_bridge.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/version.h>


#include "br_filter.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Li Shaozhang <lishaozhang@tp-link.net>");
MODULE_DESCRIPTION("Linux Netfilter module for bridge packet filtering");

#define IEEE80211_ADDR_LEN	6
/*
 * generic definitions for IEEE 802.11 frames
 */
struct ieee80211_frame {
	u_int8_t	i_fc[2];
	u_int8_t	i_dur[2];
	u_int8_t	i_addr1[IEEE80211_ADDR_LEN];
	u_int8_t	i_addr2[IEEE80211_ADDR_LEN];
	u_int8_t	i_addr3[IEEE80211_ADDR_LEN];
	u_int8_t	i_seq[2];
	/* possibly followed by addr4[IEEE80211_ADDR_LEN]; */
	/* see below */
} __packed;

#ifdef BR_FILTER_DEBUG
static void dumpHexData(unsigned char *head, int len)
{
    int i;
    unsigned char *curPtr = head;

    for (i = 0; i < len; ++i) {
		if (!i)
			;
		else if ((i & 0xF) == 0)
            printk("\n"); 
		else if ((i & 0x7) == 0)
            printk("- ");
        printk("%02X ", *curPtr++);
    }
    printk("\n");
}
#endif	/* BR_FILTER_DEBUG */


/* our hook function */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
unsigned int block_eapol(unsigned int hooknum,
                       struct sk_buff *skb,
                       const struct net_device *in,
                       const struct net_device *out,
                       int (*okfn)(struct sk_buff *))
{
	struct sk_buff *pskb = skb;
	struct ieee80211_frame *wlanHdr = (struct ieee80211_frame *)skb_mac_header(pskb);
#else
unsigned int block_eapol(unsigned int hooknum,
                       struct sk_buff **skb,
                       const struct net_device *in,
                       const struct net_device *out,
                       int (*okfn)(struct sk_buff *))
{
	struct sk_buff *pskb = *skb;
	struct ieee80211_frame *wlanHdr = (struct ieee80211_frame *)(pskb->mac.raw);
#endif

	#ifdef BR_FILTER_DEBUG
	printk("data %#x raw %#x\n\n", pskb->data, pskb->mac.raw);
	dumpHexData(pskb->data, pskb->len);
	#endif
	
	if (ntohs(wlanHdr->i_fc[0]) == 0x40 && 
		wlanHdr->i_addr1[0] == 0xff &&
		wlanHdr->i_addr1[1] == 0xff &&
		wlanHdr->i_addr1[2] == 0xff &&
		wlanHdr->i_addr1[3] == 0xff &&
		wlanHdr->i_addr1[4] == 0xff &&
		wlanHdr->i_addr1[5] == 0xff)
	{
		return NF_DROP;
	}

	return NF_ACCEPT;
}


/* netfilter hook */
static struct nf_hook_ops nfho = {
	.hook		= block_eapol,		/* hook function */
    .hooknum	= NF_BR_FORWARD, 	/* hook in forward */
    .pf			= PF_BRIDGE,
    .priority	= NF_BR_PRI_LAST,	/* last place */
};


static int __init init(void)
{
	
    return nf_register_hook(&nfho);

}


static void __exit fini(void)
{
    nf_unregister_hook(&nfho);
}

module_init(init);
module_exit(fini);


