#ifndef _IPT_STAT_H_
#define _IPT_STAT_H_

#include <linux/if_ether.h>

#define IPT_STAT_BASE_CTL			128	/* base for firewall socket options */

#define IPT_STAT_SET_NET			(IPT_STAT_BASE_CTL)
#define IPT_STAT_SET_INTERVAL		(IPT_STAT_BASE_CTL + 1)
#define IPT_STAT_RESET_ONE			(IPT_STAT_BASE_CTL + 2)
#define IPT_STAT_RESET_ALL			(IPT_STAT_BASE_CTL + 3)
#define IPT_STAT_DEL_ONE			(IPT_STAT_BASE_CTL + 4)
#define IPT_STAT_DEL_ALL			(IPT_STAT_BASE_CTL + 5)
#define IPT_STAT_SET_MAX			IPT_STAT_DEL_ALL

#define IPT_STAT_GET_ALL			IPT_STAT_BASE_CTL
#define IPT_STAT_GET_WAN_STAT		(IPT_STAT_BASE_CTL + 1)
#define IPT_STAT_GET_MAX			IPT_STAT_GET_WAN_STAT

//#define IPT_STAT_CHAINS_MAX		0x100		/* 256  */
#define IPT_STAT_ENTRIES_MAX		0x400		/* 1024 */

#define ETHER_PAYLOAD_LEN			18	/* 14 Bytes ethernet header + 4 Bytes CRC checksum */


/* an ipt_stat_net struct indicates a subnet from which every pkts will be recorded */
typedef struct ipt_stat_net
{
	u_int32_t ip;
	u_int32_t mask;
}ipt_stat_net_t;

typedef struct ipt_stat_statistic
{
	u_int32_t	all_pkts;
	u_int32_t	all_bytes;
	
	u_int32_t	icmp_pkts_tx;
	u_int32_t	udp_pkts_tx;
	u_int32_t   tcp_syn_pkts_tx;
	u_int32_t	tcp_pkts_tx;
}ipt_stat_statistic_t;

typedef struct ipt_stat_reference
{
	u_int32_t	all_pkts;
	u_int32_t	all_bytes;
}ipt_stat_reference_t;

/* the statistics kept for each host in the subnet */
typedef struct ipt_stat_host_entry
{
	u_int32_t ip;
	
	u_int16_t used;
	u_int8_t  mac[ETH_ALEN];

	/* total stat. */
	ipt_stat_statistic_t total;

	/* stat. in last interval time */
	ipt_stat_statistic_t recent;

	/* old stat. for reference */
	ipt_stat_reference_t refer;

	/* max tx */
	u_int32_t max_icmp_pkts_tx;
	u_int32_t max_udp_pkts_tx;
	u_int32_t max_tcp_syn_pkts_tx;
	
}ipt_stat_host_entry_t;


#endif /* _IPT_STAT_H_ */

