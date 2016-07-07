/*! Copyright(c) 2008-2009 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file		wlanWarnTarget.c
 *\brief	the source file of wlan warn iptables target
 *\details	this target capture the ip header and tcp header of HTTP GET packet, and return 
 *			to user space; it also judge if the GET packet contains configure information for 
 *			the user space program, and return it to user space
 *
 *\author	Zou Boyin
 *\version	1.0.0
 *\date		27Dec09
 *
 *\history  \arg 1.0.0, 27Dec09, Zou Boyin, Create the file.
 */
 
 
/***************************************************************************/
/*						INCLUDE_FILES									   */
/***************************************************************************/
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <linux/completion.h>
#include <linux/version.h>

#include <asm/atomic.h>
#include <linux/byteorder/generic.h>
#include <net/checksum.h>
#include <linux/netfilter_ipv4/ip_tables.h>


#include "wlan_warn.h"

MODULE_LICENSE("GPL");


struct ts_config* 	l_ts_conf;
struct completion 	l_complete;
atomic_t 			l_atomic_flag;
int					l_check;

struct sk_buff*	 	l_pSkb;

/***************************************************************************/
/*						LOCAL_FUNCTIONS									   */
/***************************************************************************/

/*!
 *\fn			static int getConfig(struct iphdr* pIpHeader
										, struct tcphdr* pTcpHeader)
 *
 *\brief		judge if the tcp packet contains configure information
 *	
 *\param[in]	pIpHeader			the pointer of ip header
 *\param[in]	pTcpHeader			the pointer of tcp header
 *
 *\retval 		CHECKED				if user click the check button
 *\retval		NOT_CHECKED			if user doesn't click the check button
 *\retval		CHECK_NONE			if the packet doesn't contains configure information
 */
static int getConfig(struct iphdr* pIpHeader, struct tcphdr* pTcpHeader)
{
	int retVal = CHECK_NONE;
	char* ptr = NULL;
	struct ts_state state;
	int tcpLen = pIpHeader->tot_len - (pIpHeader->ihl >> 2) - (pTcpHeader->doff >> 2);

	int pos = textsearch_find_continuous(l_ts_conf, &state, pTcpHeader, tcpLen);
	if (pos != UINT_MAX)
	{		
		ptr = (char*)pTcpHeader + pos + strlen(CONFIG_PATTERN);

		if (CONFIG_PATTERN_YES == *ptr)
		{
			retVal = CHECKED;
		}
		else if (CONFIG_PATTERH_NO == *ptr)
		{
			retVal = NOT_CHECKED;
		}
		else
		{
			retVal = CHECK_NONE;
		}
	}
	return retVal;
}

/*!
 *\fn			static int set_parm(struct sock *sk
								, int cmd, void __user *user
								, unsigned int len)
 *
 *\brief		this function is call if user space program use a setsockopt
 *				not used in this project
 */
static int set_parm(struct sock *sk, int cmd, void __user *user, unsigned int len)
{
	PDEBUG("setsockopt\n");
	return 0;
}

/*!
 *\fn			static int set_parm(struct sock *sk
								, int cmd, void __user *user
								, unsigned int len)
 *
 *\brief		this function is call if user space program use a getsockopt
 *				this function wait for a HTTP GET packet, and get the ip header and tcp header 
 *				from HTTP GET for it then return to it's caller, 
 *\param[in]	sk			the sock pointer
 *\param[in]	cmd			the command number user space program used on getsockopt()
 *\param[in][out] user		the user space pointer
 *\param[in]	len			the user space length
 
 *\retval		-ERESTARTSYS	if sleep is woken by a signal
 *\retval		-EFAULT			if user space pointer is wrong
 *\retval		0				on success
 */
static int get_parm(struct sock *sk, int cmd, void __user *user, int *len)
{
	struct tcphdr* 		l_tcph;
	struct iphdr*		l_iph;
	struct tcphdr 		_tcph;
	WLAN_WARN_PARAM 	wlanWarnParam;

	
	INIT_COMPLETION(l_complete);

	atomic_set(&l_atomic_flag, CMP_NEED_PARM);

	if (wait_for_completion_interruptible(&l_complete) != 0)
	{
		return -ERESTARTSYS;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
	l_iph = ip_hdr(l_pSkb);
#else
	l_iph = l_pSkb->nh.iph;
#endif
	
	l_tcph = skb_header_pointer(l_pSkb, l_iph->ihl << 2, sizeof(_tcph), &_tcph);

	if (copy_from_user(&wlanWarnParam, user, sizeof(WLAN_WARN_PARAM)))
	{
		return -EFAULT;
	}

	memcpy(&wlanWarnParam.ipHeader, l_iph, sizeof(struct iphdr));
	memcpy(&wlanWarnParam.tcpHeader,l_tcph, sizeof(struct tcphdr));
	wlanWarnParam.checked = l_check;

	if (copy_to_user(user, &wlanWarnParam, sizeof(WLAN_WARN_PARAM)))
	{
		return -EFAULT;
	}

	kfree_skb(l_pSkb);

	return 0;
}

/*!
 *\fn			static unsigned int ipt_wlan_warn_target(
						  struct sk_buff **pskb
						, const struct net_device *in
						, const struct net_device *out, unsigned int hooknum
						, const void *targinfo
						, void *userinfo)
 *
 *\brief		this function is call by netfilter framwork
 *
 *
 *\param[in]	pskb		the pointer of pointer of skb data
 *\param[in]	in			the in device of the packet
 *\param[in]	out			the out device of the packet
 *\param[in]	hooknum		the hook number
 *\param[in]	targinfo	target infomation
 *\param[in]	userinfo	user infomation
 
 *\retval		IPT_CONTINUE	if the packet contains a configure information
 *\retval		NF_DROP			if the packet doesn't contains a configure information
 *\retval		0				on success
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
static unsigned int ipt_wlan_warn_target(
								  struct sk_buff **pskb
								, const struct xt_target_param *param)
#else
static unsigned int ipt_wlan_warn_target(
						  struct sk_buff **pskb
						, const struct net_device *in
						, const struct net_device *out, unsigned int hooknum
						, const void *targinfo
						, void *userinfo)
#endif						
{
	int ret_val = IPT_CONTINUE;
	struct tcphdr* 		l_tcph;
	struct iphdr*		l_iph;
	struct tcphdr 		_tcph;

	if (atomic_read(&l_atomic_flag) == CMP_NEED_PARM)
	{	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
		l_iph = ip_hdr(*pskb);
#else
		l_iph = (*pskb)->nh.iph;
#endif		
		l_tcph = skb_header_pointer((*pskb), l_iph->ihl << 2, sizeof(_tcph), &_tcph);
		
		l_check = CHECK_NONE;
		l_check = getConfig(l_iph, l_tcph);
		
		if (CHECK_NONE == l_check)
		{
			ret_val = NF_DROP;
		}
		l_pSkb = pskb_copy(*pskb, GFP_KERNEL);	
		
		atomic_set(&l_atomic_flag, CMP_NONE);
		complete(&l_complete);
	}	
	return ret_val;
}


/*!
 *\struct 	nf_sockopt_ops
 *\brief	the sturcut to register to netfilter sockopt
 */
static struct nf_sockopt_ops ipt_sockopts = 
{
	.pf     	= PF_INET,
	.set_optmin = WLAN_WARN_SO_SET_MIN,
	.set_optmax = WLAN_WARN_SO_SET_MAX,
	.set        = set_parm,
	.get_optmin = WLAN_WARN_SO_GET_MIN,
	.get_optmax = WLAN_WARN_SO_GET_MAX,
	.get        = get_parm,
};

/*!
 *\struct 	ipt_target
 *\brief	the structure of target info to register to iptables framework
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
	static struct xt_target ipt_wlan_warn = { 	
#else
	static struct ipt_target ipt_wlan_warn = { 		
#endif
	.name 		= "wlanwarn",
	.target 	= ipt_wlan_warn_target, 
	.targetsize = XT_ALIGN(sizeof(struct ipt_wlan_warn_info)),
	.me 		= THIS_MODULE,
};

/*!
 *\fn			static int __init init(void)
 *
 *\brief		init function of this kernel module, call when insmod
 *
 *\retval		0				on success
 */
static int __init init(void)
{
	l_ts_conf = textsearch_prepare("kmp", CONFIG_PATTERN, strlen(CONFIG_PATTERN), 
					GFP_KERNEL, TS_AUTOLOAD);
	if (IS_ERR(l_ts_conf))
	{
		return PTR_ERR(l_ts_conf);
	}

	init_completion(&l_complete);


	nf_register_sockopt(&ipt_sockopts);
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
	return xt_register_target(&ipt_wlan_warn);	
#else
	return ipt_register_target(&ipt_wlan_warn);	
#endif
	
}

/*!
 *\fn			static void __exit fini(void)
 *
 *\brief		exit function of this kernel module, call when rmmod
 *
 *\retval		0				on success
 */
static void __exit fini(void)
{
	textsearch_destroy(l_ts_conf);
	nf_unregister_sockopt(&ipt_sockopts);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
	xt_unregister_target(&ipt_wlan_warn);
#else
	ipt_unregister_target(&ipt_wlan_warn);
#endif
}

module_init(init);
module_exit(fini);
