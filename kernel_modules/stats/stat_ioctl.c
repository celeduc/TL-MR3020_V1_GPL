#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ipt_STAT.h"

static u_int32_t ip = 0xC0A80164;

int total = 0;
struct ipt_stat_host_entry entries[IPT_STAT_ENTRIES_MAX];

int print_entries(void)
{
	int i = 0;
	for (i = 0; i < total; i ++)
	{
		if (entries[i].used == 1)
		{
			printf("index:%d\n", i);
			printf("\tip:%#X  all_pkts:%d all_bytes:%d\n"  
					"\ttx_pkts_icmp:%d tx_pkts_udp:%d tx_pkts_tcp:%d tx_pkts_tcp_syn:%d\n"
					"\tcur_all_pkts:%d cur_all_bytes:%d cur_icmp:%d cur_udp:%d cur_tcp_syn:%d\n"
					"\tmax_icmp:%d max_udp:%d max_tcp_syn:%d\n",
					entries[i].ip, entries[i].total.all_pkts, 
					entries[i].total.all_bytes,
					entries[i].total.icmp_pkts_tx, entries[i].total.udp_pkts_tx,
					entries[i].total.tcp_pkts_tx, entries[i].total.tcp_syn_pkts_tx,
					entries[i].recent.all_pkts, entries[i].recent.all_bytes, 
					entries[i].recent.icmp_pkts_tx, entries[i].recent.udp_pkts_tx,
					entries[i].recent.tcp_syn_pkts_tx, entries[i].max_icmp_pkts_tx, 
					entries[i].max_udp_pkts_tx, entries[i].max_tcp_syn_pkts_tx);
		}
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int sockfd = 0;
	int ret = 0;
	struct ipt_stat_net stat_net;
	int tmp = 0;
	
	int len = 0;
	stat_net.ip = 0xC0A80164;
	stat_net.mask = 0xFFFFFF00;

	sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
	if (sockfd < 0)
	{
		printf("socket create failed\n");
		return sockfd;
	}

	if (argc == 1)
	{
		ret = setsockopt(sockfd, IPPROTO_IP, IPT_STAT_SET_NET, &stat_net, sizeof(stat_net));
		if (ret < 0)
			return ret;
	}
	else 
	{
		if (strcmp(argv[1], "get") == 0)
		{
			memset(entries, 0, IPT_STAT_ENTRIES_MAX * sizeof(ipt_stat_host_entry_t));
			ret = getsockopt(sockfd, IPPROTO_IP, IPT_STAT_GET_ALL, entries, &total);
			if (ret < 0)
				return ret;
			
			print_entries();		
		}
		if (strcmp(argv[1], "set_interval") == 0)
		{
			tmp = 5;
			ret = setsockopt(sockfd, IPPROTO_IP, IPT_STAT_SET_INTERVAL, &tmp, sizeof(int));
			if (ret < 0)
				return ret;
		}
		if (strcmp(argv[1], "reset_one") == 0)
		{
			tmp = 1;
			ret = setsockopt(sockfd, IPPROTO_IP, IPT_STAT_RESET_ONE, &tmp, sizeof(int));
			if (ret < 0)
				return ret;
			print_entries();
		}
		if (strcmp(argv[1], "reset_all") == 0)
		{
			ret = setsockopt(sockfd, IPPROTO_IP, IPT_STAT_RESET_ALL, NULL, 0);
			if (ret < 0)
				return ret;
			print_entries();
		}
		if (strcmp(argv[1], "del_one") == 0)
		{
			tmp = 1;
			ret = setsockopt(sockfd, IPPROTO_IP, IPT_STAT_DEL_ONE, &tmp, sizeof(int));
			if (ret < 0)
				return ret;
			print_entries();
		}
		if (strcmp(argv[1], "del_all") == 0)
		{
			ret = setsockopt(sockfd, IPPROTO_IP, IPT_STAT_DEL_ALL, NULL, 0);
			if (ret < 0)
				return ret;
			print_entries();
		}
	}
	
	return 1;	
}

