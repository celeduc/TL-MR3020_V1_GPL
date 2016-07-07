
/*
 * Cone NAT target for IP connection.
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Revison History:
 *  8/11/2010, initial version for ap99, by yingming.yu@atheros.com
 */

#include <linux/types.h>
#include <linux/ip.h>
#include <linux/udp.h>
//#include <linux/timer.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/module.h>
#include <net/protocol.h>
#include <net/checksum.h>
#include <linux/tcp.h>

#include <net/netfilter/nf_nat_rule.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <linux/netfilter/x_tables.h>

#include "ipt_NATTYPE.h"
#include "lockhelp.h"
#include "listhelp.h"

static int DebugMode = 0;
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Netfilter target module to support Cone NAT");
module_param(DebugMode, int, 0400);
MODULE_PARM_DESC(DebugMode, "Debug Mode control. Set 1 to enable this mode. Throughput might be really poor with this mode");


struct ipt_nattype {
    struct list_head list;      
    struct timer_list timeout;  
    unsigned int dst_addr;
    unsigned short nat_port;
    unsigned short proto;
    struct nf_nat_multi_range_compat mr;
};
#define DEBUGP(format, args...)

LIST_HEAD(nattype_list);

static unsigned int
del_nattype_rule(struct ipt_nattype *nattype)
{
    if(!nattype) return 1;
    write_lock_bh(&nf_conntrack_lock);
    list_del(&nattype->list);
    write_unlock_bh(&nf_conntrack_lock);
    kfree(nattype);
    return 0;
}

static void 
refresh_timer(struct ipt_nattype *nattype)
{
    if(!nattype) return;
    write_lock_bh(&nf_conntrack_lock);
    if (del_timer(&nattype->timeout)) {
        nattype->timeout.expires = jiffies + NATTYPE_TIMEOUT * HZ;
        add_timer(&nattype->timeout);
    }
    write_unlock_bh(&nf_conntrack_lock);
    return;
}

static void timer_timeout(unsigned long in_nattype)
{
    struct ipt_nattype *nattype= (void *) in_nattype;
    write_lock_bh(&nf_conntrack_lock);
    del_nattype_rule(nattype);
    write_unlock_bh(&nf_conntrack_lock);
    return;
}


static inline int 
packet_in_match(
    const struct ipt_nattype *nattype, 
    struct iphdr *iph,
    const struct ipt_nattype_info *info)
{
    struct tcphdr *tcph = (void *)iph + iph->ihl*4;
    struct udphdr *udph = (void *)iph + iph->ihl*4;
    u_int16_t dst_port;
    int ret = 0;
    __be32 daddr = iph->saddr; 

        //ipv4_is_loopback(daddr)||
    if(ipv4_is_multicast(daddr)||
        ipv4_is_local_multicast(daddr)||
        ipv4_is_lbcast(daddr)){
        return 0;
    }

    if( info->type == TYPE_ENDPOINT_INDEPEND) {
        if( iph->protocol == IPPROTO_TCP)
            dst_port = tcph->dest;
        else if( iph->protocol == IPPROTO_UDP)
            dst_port = udph->dest;
        else
            return 0;
                
        ret = ( (nattype->proto==iph->protocol) && (nattype->nat_port == dst_port) );

    } else if( info->type == TYPE_ADDRESS_RESTRICT) {
        ret = (nattype->dst_addr== iph->saddr);
    }
    
    if(ret == 1){
        if( info->type == TYPE_ENDPOINT_INDEPEND)
            DEBUGP("packet_in_match: nat port :%d\n", ntohs(nattype->nat_port));
        else
            DEBUGP("packet_in_match: nattpe->dst_addr :%pI4\n", &nattype->dst_addr);
    }else{
        if( info->type == TYPE_ENDPOINT_INDEPEND)
            DEBUGP("packet_in_match fail: nat port :%d, dest_port: %d\n",
            ntohs(nattype->nat_port), ntohs(dst_port));   
        else
            DEBUGP("packet_in_match fail: nattype->dst_addr:%pI4,"
            "iph->saddr: %pI4\n", &nattype->dst_addr, &iph->saddr);   
    }
    return ret;
}

static inline int 
packet_out_match(
        const struct ipt_nattype *nattype,
        struct iphdr *iph,
        const u_int16_t nat_port,
        const struct ipt_nattype_info *info)
{
    struct tcphdr *tcph = (void *)iph + iph->ihl*4;
    struct udphdr *udph = (void *)iph + iph->ihl*4;
    u_int16_t src_port;
    int ret;

    if( info->type == TYPE_ENDPOINT_INDEPEND) {
        if( iph->protocol == IPPROTO_TCP)
            src_port = tcph->source;
        else if( iph->protocol == IPPROTO_UDP)
            src_port = udph->source;
        else
            return 0;
        ret = ( (nattype->proto==iph->protocol) &&
            (nattype->mr.range[0].min.all == src_port) &&
            (nattype->nat_port == nat_port));
    } else if( info->type == TYPE_ADDRESS_RESTRICT) {
        ret = (nattype->dst_addr == iph->daddr );
    }

    if(ret == 1)
        DEBUGP("packet_out_match: nat port :%d, mr.port:%d\n",
        ntohs(nattype->nat_port), ntohs(nattype->mr.range[0].min.all));

    return ret;
}


static unsigned int
add_nattype_rule(struct ipt_nattype *nattype)
{
    struct ipt_nattype *rule;

    write_lock_bh(&nf_conntrack_lock);
    rule = (struct ipt_nattype *)kmalloc(sizeof(struct ipt_nattype), GFP_ATOMIC);

    if (!rule) {
        write_unlock_bh(&nf_conntrack_lock);
        return -ENOMEM;
    }
    
    memset(rule, 0, sizeof(*nattype));
    INIT_LIST_HEAD(&rule->list);
    memcpy(rule, nattype, sizeof(*nattype));
    list_prepend(&nattype_list, &rule->list);   
    init_timer(&rule->timeout);
    rule->timeout.data = (unsigned long)rule;
    rule->timeout.function = timer_timeout;
    rule->timeout.expires = jiffies + (NATTYPE_TIMEOUT  * HZ);
    add_timer(&rule->timeout);  
    write_unlock_bh(&nf_conntrack_lock);
    return 0;
}


static unsigned int
nattype_nat(struct sk_buff **pskb,
          const struct net_device *in,
          const struct net_device *out,
          unsigned int hooknum,
          const void *targinfo)
{
    struct nf_conn *ct;
    enum ip_conntrack_info ctinfo;
    const struct iphdr *iph = ip_hdr(*pskb);
    struct nf_nat_range newrange;
    struct ipt_nattype *found;
    u_int32_t newdst;
    enum nf_nat_manip_type maniptype;
    //IP_NF_ASSERT(hooknum == NF_IP_PRE_ROUTING);
    found = LIST_FIND(&nattype_list, packet_in_match,
        struct ipt_nattype *, iph, targinfo);

    if( !found )
    {
        DEBUGP("nattype_nat: not found\n");
        return IPT_CONTINUE;
    }   

    DEBUGP("NAT nat port :%d, mr.port:%d\n",
        ntohs(found->nat_port), ntohs(found->mr.range[0].min.all));

    ct = nf_ct_get(*pskb, &ctinfo);
    
    newrange = ((struct nf_nat_range)
        { found->mr.range[0].flags | IP_NAT_RANGE_MAP_IPS,
          found->mr.range[0].min_ip, found->mr.range[0].min_ip,
          found->mr.range[0].min, found->mr.range[0].max });         
    maniptype = HOOK2MANIP(hooknum);
    return nf_nat_setup_info(ct, &newrange, IP_NAT_MANIP_DST/*maniptype*/); 
}


static unsigned int
nattype_forward(struct sk_buff **pskb,
          unsigned int hooknum,
          const struct net_device *in,
          const struct net_device *out,
          const void *targinfo,
          int mode)
{
    const struct iphdr *iph = ip_hdr(*pskb);
    void *protoh = (void *)iph + iph->ihl * 4;
    struct ipt_nattype nattype, *found;
    struct nf_conn *ct;
    enum ip_conntrack_info ctinfo;

    //IP_NF_ASSERT(hooknum == NF_IP_FORWARD);
    switch(mode)
    {
        case MODE_FORWARD_IN:
            found = LIST_FIND(&nattype_list, 
                packet_in_match, struct ipt_nattype *, iph, targinfo);
            if (found) {
                refresh_timer(found);
                DEBUGP("FORWARD_IN matched, refresh timer\n");
                return NF_ACCEPT;
            }
            else
                DEBUGP("FORWARD_IN FAIL\n");
            break;

        /* MODE_FORWARD_OUT */
        case MODE_FORWARD_OUT:
            ct = nf_ct_get(*pskb, &ctinfo);
            if(!ct || !&ctinfo ||
                (ctinfo != IP_CT_NEW && ctinfo != IP_CT_RELATED))
                return IPT_CONTINUE; /*Sanity check & release CPU load*/
            
            found = LIST_FIND(&nattype_list, packet_out_match,
                                struct ipt_nattype *, iph, 
                                ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all,
                                targinfo);
            if (found) {
                refresh_timer(found);
            } else {
                DEBUGP("FORWARD_OUT ADD\n");
                if( iph->daddr == iph->saddr)
                    return IPT_CONTINUE;
                if (!iph->daddr || !iph->saddr || !iph->protocol)
                    return IPT_CONTINUE;
                memset(&nattype, 0, sizeof(nattype));
                nattype.mr.rangesize = 1;
                nattype.mr.range[0].flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
                nattype.dst_addr = iph->daddr;
                nattype.mr.range[0].min_ip = iph->saddr;
                nattype.proto = iph->protocol;
                
                if( iph->protocol == IPPROTO_TCP) {
                    nattype.nat_port = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.tcp.port;
                    nattype.mr.range[0].max.tcp.port = 
                        nattype.mr.range[0].min.tcp.port =((struct tcphdr *) protoh)->source ;
                    DEBUGP("ADD: TCP nat port: %d\n", ntohs(nattype.nat_port));
                    DEBUGP("ADD: TCP Source Port: %d\n", ntohs(nattype.mr.range[0].min.tcp.port));
                } else if( iph->protocol == IPPROTO_UDP) {
                    nattype.nat_port = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port;
                    nattype.mr.range[0].max.udp.port = 
                        nattype.mr.range[0].min.udp.port = ((struct udphdr *) protoh)->source;
                    DEBUGP("ADD: UDP nat port: %d\n", ntohs(nattype.nat_port));
                    DEBUGP("ADD: UDP IP_CT_DIR_ORIGINAL dst port: %d\n",
                        ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port));
                    DEBUGP("ADD: UDP Source Port: %d\n", ntohs(nattype.mr.range[0].min.udp.port));
                } else
                    return IPT_CONTINUE;

                add_nattype_rule(&nattype);
            }
            break;
    }

    return IPT_CONTINUE;
}

static unsigned int
nattype_target(struct sk_buff *pskb,
            const struct xt_target_param *par)
{
    const struct ipt_nattype_info *info = par->targinfo;
    const struct iphdr *iph = ip_hdr(pskb);

    const struct net_device *in = par->in;
    const struct net_device *out = par->out;
    unsigned int hooknum = par->hooknum;
    const void *targinfo = par->targinfo;
    
    if ((iph->protocol != IPPROTO_TCP) && (iph->protocol != IPPROTO_UDP))
        return IPT_CONTINUE;
    
    DEBUGP("nattype_target, info->mode=%d, info->type=%d\n", info->mode, info->type);   

    if (info->mode == MODE_DNAT)
    {
        DEBUGP("nattype_target: MODE_DNAT\n");
        return nattype_nat(&pskb, in, out, hooknum, targinfo);
    }   
    else if (info->mode == MODE_FORWARD_OUT)
    {
        DEBUGP("nattype_target: MODE_FORWARD_OUT\n");
        if (in == NULL || out == NULL || targinfo == NULL) 
            return IPT_CONTINUE;
        return nattype_forward(&pskb, hooknum, in, out, targinfo, MODE_FORWARD_OUT);
    }   
    else if (info->mode == MODE_FORWARD_IN)
    {
        DEBUGP("nattype_target: MODE_FORWARD_IN\n");
        return nattype_forward(&pskb, hooknum, in, out, targinfo, MODE_FORWARD_IN);
    }
    return IPT_CONTINUE;
}

static bool 
nattype_check(const struct xt_tgchk_param *par)
{
    unsigned int hook_mask = par->hook_mask;
    const struct ipt_nattype_info *info = par->targinfo;
    struct list_head *cur, *tmp;
    
    if( info->type == TYPE_PORT_ADDRESS_RESTRICT) {
        DEBUGP("For Port and Address Restricted. this rule is needless\n");
        return 0;
    }
    else if( info->type == TYPE_ENDPOINT_INDEPEND)
        DEBUGP("NAT_Tyep = TYPE_ENDPOINT_INDEPEND.\n");
    else if( info->type == TYPE_ADDRESS_RESTRICT)
        DEBUGP("NAT_Tyep = TYPE_ADDRESS_RESTRICT.\n");  
        
    if ( info->mode != MODE_DNAT &&
        info->mode != MODE_FORWARD_IN &&
        info->mode != MODE_FORWARD_OUT) {
        DEBUGP("nattype_check: bad nat mode.\n");
        return 0;
    }   
    
    if (hook_mask & ~((1 << NF_INET_PRE_ROUTING) | (1 << NF_INET_FORWARD))) {
        DEBUGP("nattype_check: bad hooks %x.\n", hook_mask);
        return 0;
    }

    list_for_each_safe(cur, tmp, &nattype_list) {
        struct ipt_nattype *nattype = (void *)cur;
        del_timer(&nattype->timeout);
        del_nattype_rule(nattype);
    }

    return 1;
}

static struct xt_target nattype = {
     .family    = AF_INET,
     .name      = "NATTYPE",
     .target    = nattype_target,
     .targetsize = sizeof(struct ipt_nattype_info),
     .checkentry = nattype_check,
     .me        = THIS_MODULE,
};

static int __init init(void)
{
    return xt_register_target(&nattype);
}

static void __exit fini(void)
{
    xt_unregister_target(&nattype);
}

module_init(init);
module_exit(fini);

