/* serverpacket.c
 *
 * Construct and send DHCP server packets
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#include "serverpacket.h"
#include "dhcpd.h"
#include "options.h"
#include "common.h"
#include "static_leases.h"
#include "libmsglog.h"

#define DHCPS_HOST_NAME_LEN 32

extern int check_ip(struct dhcpMessage *oldpacket, uint32_t addr);

/* send a packet to giaddr using the kernel ip stack */
static int send_packet_to_relay(struct dhcpMessage *payload)
{
	DEBUG(LOG_INFO, "Forwarding packet to relay");

	return kernel_packet(payload, server_config.server, SERVER_PORT,
			payload->giaddr, SERVER_PORT);
}


/* send a packet to a specific arp address and ip address by creating our own ip packet */
static int send_packet_to_client(struct dhcpMessage *payload, int force_broadcast)
{
	uint8_t *chaddr;
	uint32_t ciaddr;

	if (force_broadcast) {
		DEBUG(LOG_INFO, "broadcasting packet to client (NAK)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else if (payload->ciaddr) {
		DEBUG(LOG_INFO, "unicasting packet to client ciaddr");
		ciaddr = payload->ciaddr;
		chaddr = payload->chaddr;
	} else if (ntohs(payload->flags) & BROADCAST_FLAG) {
		DEBUG(LOG_INFO, "broadcasting packet to client (requested)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else {
		DEBUG(LOG_INFO, "unicasting packet to client yiaddr");
		ciaddr = payload->yiaddr;
		chaddr = payload->chaddr;
	}
	return raw_packet(payload, server_config.server, SERVER_PORT,
			ciaddr, CLIENT_PORT, chaddr, server_config.ifindex);
}


/* send a dhcp packet, if force broadcast is set, the packet will be broadcast to the client */
static int send_packet(struct dhcpMessage *payload, int force_broadcast)
{
	int ret;

	if (payload->giaddr)
		ret = send_packet_to_relay(payload);
	else ret = send_packet_to_client(payload, force_broadcast);
	return ret;
}


static void init_packet(struct dhcpMessage *packet, struct dhcpMessage *oldpacket, char type)
{
	init_header(packet, type);
	packet->xid = oldpacket->xid;
	memcpy(packet->chaddr, oldpacket->chaddr, 16);
	packet->flags = oldpacket->flags;
	packet->giaddr = oldpacket->giaddr;
	packet->ciaddr = oldpacket->ciaddr;
	add_simple_option(packet->options, DHCP_SERVER_ID, server_config.server);
}


/* add in the bootp options */
static void add_bootp_options(struct dhcpMessage *packet)
{
	packet->siaddr = server_config.siaddr;
	if (server_config.sname)
		strncpy(packet->sname, server_config.sname, sizeof(packet->sname) - 1);
	if (server_config.boot_file)
		strncpy(packet->file, server_config.boot_file, sizeof(packet->file) - 1);
}


/* send a DHCP OFFER to a DHCP DISCOVER */
int sendOffer(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct dhcpOfferedAddr *lease = NULL;
	uint32_t req_align, lease_time_align = server_config.lease;
	uint8_t *req, *lease_time;
	struct option_set *curr;
	struct in_addr addr;
	char blank_hostname[] = "Unknown";

#if 1
	uint8_t host_name[DHCPS_HOST_NAME_LEN];
	uint8_t *host_name_start, *host_name_len;
#endif

	uint32_t static_lease_ip;

	init_packet(&packet, oldpacket, DHCPOFFER);

	static_lease_ip = getIpByMac(server_config.static_leases, oldpacket->chaddr);

	/* ADDME: if static, short circuit */
	if(!static_lease_ip)
	{
		/* the client is in our lease/offered table */
		if ((lease = find_lease_by_chaddr(oldpacket->chaddr)))
		{
			if (!lease_expired(lease))
				lease_time_align = lease->expires - uptime()/*time(0)*/;
			packet.yiaddr = lease->yiaddr;

		/* Or the client has a requested ip */
		} 
		else if ((req = get_option(oldpacket, DHCP_REQUESTED_IP)) &&
			/* Don't look here (ugly hackish thing to do) */
			memcpy(&req_align, req, 4) &&

		   	/* and the ip is in the lease range */
		   	ntohl(req_align) >= ntohl(server_config.start) &&
		   	ntohl(req_align) <= ntohl(server_config.end) &&

		#if 0	/* no need to do this in if(!static_lease_ip) -- by lsz 29Sep07 */
			!static_lease_ip &&  /* Check that its not a static lease */
		#endif
			/* And the requested ip is not reserved by static leases */
			!reservedIp(server_config.static_leases, req_align) &&
			
			/* and is not already taken/offered */
		   	((!(lease = find_lease_by_yiaddr(req_align)) ||
		
		   	/* or its taken, but expired */ /* ADDME: or maybe in here */
		   	lease_expired(lease)))) 
		{
			packet.yiaddr = req_align; /* FIXME: oh my, is there a host using this IP? */

			/* check if there is a host using this IP - by lsz 01Sep07 */
			if (check_ip(oldpacket, req_align))
				packet.yiaddr = find_address(oldpacket, 0);
			
			if (!packet.yiaddr)
				packet.yiaddr = find_address(oldpacket, 1);

			/* otherwise, find a free IP */
		}
		else
		{
			/* Is it a static lease? (No, because find_address skips static lease) */
			packet.yiaddr = find_address(oldpacket, 0);
			/* try for an expired lease */
			if (!packet.yiaddr) packet.yiaddr = find_address(oldpacket, 1);
		}

		if(!packet.yiaddr) 
		{
			//LOG(LOG_WARNING, "no IP addresses to give -- OFFER abandoned");
			msglogd(LOG_WARNING, LOGTYPE_DHCP, "DHCPS:no ip addresses to give, OFFER abandoned");
			return -1;
		}

		if (!(host_name_start = get_option(oldpacket, DHCP_HOST_NAME)))
		{
			//LOG(LOG_WARNING, "lease host name not found");
			msglogd(LOG_WARNING, LOGTYPE_DHCP, "DHCPS:lease host name not found");
			//host_name_start = blank_hostname;
			memset(host_name, 0, DHCPS_HOST_NAME_LEN);
			memcpy(host_name, blank_hostname, strlen(blank_hostname));
		}
		else
		{	
			host_name_len = host_name_start - OPT_LEN;
			memset(host_name, 0, DHCPS_HOST_NAME_LEN);
            /* fix host name length bug by tiger 20091208 */
			memcpy(host_name, host_name_start, *host_name_len >= DHCPS_HOST_NAME_LEN ? DHCPS_HOST_NAME_LEN -1 : *host_name_len );
		}
		
		if (!add_lease(DHCPOFFER, host_name, packet.chaddr, packet.yiaddr, server_config.offer_time))
		{
			//LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
			msglogd(LOG_WARNING, LOGTYPE_DHCP, "DHCPS:lease pool is full, OFFER abandoned");
			return -1;
		}

		if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
			memcpy(&lease_time_align, lease_time, 4);
			lease_time_align = ntohl(lease_time_align);
			if (lease_time_align > server_config.lease)
				lease_time_align = server_config.lease;
		}

		/* Make sure we aren't just using the lease time from the previous offer */
		if (lease_time_align < server_config.min_lease)
			lease_time_align = server_config.lease;
	}

	/* ADDME: end of short circuit */
	else
	{
	/* by wdl, 18Apr12, always offer the reserved ip */
	#if 0
		/* static lease, but check it first, lsz 080220 */
		if (check_ip(oldpacket, static_lease_ip))	/* belongs to someone, we choose another ip */
		{
			packet.yiaddr = find_address(oldpacket, 0);
			/* try for an expired lease */
			if (!packet.yiaddr) packet.yiaddr = find_address(oldpacket, 1);

			if(!packet.yiaddr) 
			{
				//LOG(LOG_WARNING, "no IP addresses to give -- OFFER abandoned");
				msglogd(LOG_WARNING, LOGTYPE_DHCP, "DHCPS:no ip addresses to give, OFFER abandoned");
				return -1;
			}
			
			lease_time_align = server_config.lease;

			if (!(host_name_start = get_option(oldpacket, DHCP_HOST_NAME)))
			{
				//LOG(LOG_WARNING, "lease host name not found");
				msglogd(LOG_WARNING, LOGTYPE_DHCP, "DHCPS:lease host name not found");
				//host_name_start = blank_hostname;
				memset(host_name, 0, DHCPS_HOST_NAME_LEN);
				memcpy(host_name, blank_hostname, strlen(blank_hostname));
			}
			else
			{	
				host_name_len = host_name_start - OPT_LEN;
				memset(host_name, 0, DHCPS_HOST_NAME_LEN);
                /* fix host name length bug by tiger 20091208 */
				memcpy(host_name, host_name_start, *host_name_len >= DHCPS_HOST_NAME_LEN ? DHCPS_HOST_NAME_LEN -1 : *host_name_len);
			}
			
			if (!add_lease(DHCPOFFER, host_name, packet.chaddr, packet.yiaddr, server_config.offer_time))
			{
				//LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
				msglogd(LOG_WARNING, LOGTYPE_DHCP, "DHCPS:lease pool is full, OFFER abandoned");
				return -1;
			}
		}
		else
	#endif
		{
			/* It is a static lease... use it */
			packet.yiaddr = static_lease_ip;

			/* Set lease time to INFINITY for static lease -- by lsz 22Aug07. */
			lease_time_align = 0xFFFFFFFF;
		}
	}

	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));

	curr = server_config.options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet);

	addr.s_addr = packet.yiaddr;
	//LOG(LOG_INFO, "sending OFFER of %s", inet_ntoa(addr));
	msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Send OFFER with ip %s", inet_ntoa(addr));
	return send_packet(&packet, 0);
}


int sendNAK(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;

	init_packet(&packet, oldpacket, DHCPNAK);

	//DEBUG(LOG_INFO, "sending NAK");
	msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Send NAK");
	return send_packet(&packet, 1);
}


int sendACK(struct dhcpMessage *oldpacket, uint32_t yiaddr)
{
	struct dhcpMessage packet;
	struct option_set *curr;
	uint8_t *lease_time;
	uint32_t lease_time_align = server_config.lease;
	struct in_addr addr;
	//uint8_t blank_host_name[] = {[0 ... 31] = 0};
	char blank_hostname[] = "Unknown";
	
	uint8_t host_name[DHCPS_HOST_NAME_LEN];
	uint8_t *host_name_start, *host_name_len;

	init_packet(&packet, oldpacket, DHCPACK);
	packet.yiaddr = yiaddr;

	/* Set lease time to INFINITY for static lease -- by lsz 22Aug07. */
	if (reservedIp(server_config.static_leases, yiaddr))
	{
		lease_time_align = 0xFFFFFFFF;
	}
	else if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME)))
	{
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
		
		if (lease_time_align < server_config.min_lease || 
			lease_time_align > server_config.lease)
			lease_time_align = server_config.lease;
	}

	
	
	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));

	curr = server_config.options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet);

	addr.s_addr = packet.yiaddr;
	//LOG(LOG_INFO, "sending ACK to %s", inet_ntoa(addr));
	msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Send ACK to %s", inet_ntoa(addr));

	if (send_packet(&packet, 0) < 0)
		return -1;

	if (!(host_name_start = get_option(oldpacket, DHCP_HOST_NAME)))
	{
		LOG(LOG_WARNING, "lease host name not found");
		//host_name_start = blank_hostname;
		memset(host_name, 0, DHCPS_HOST_NAME_LEN);
		memcpy(host_name, blank_hostname, strlen(blank_hostname));
	}
	else
	{
		host_name_len = host_name_start - OPT_LEN;
		memset(host_name, 0, DHCPS_HOST_NAME_LEN);
        /* fix host name length bug by tiger 20091208 */
		memcpy(host_name, host_name_start, *host_name_len >= DHCPS_HOST_NAME_LEN ? DHCPS_HOST_NAME_LEN -1 : *host_name_len);
	}

	add_lease(DHCPACK, host_name, packet.chaddr, packet.yiaddr, lease_time_align);

	return 0;
}


int send_inform(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct option_set *curr;

	init_packet(&packet, oldpacket, DHCPACK);

	curr = server_config.options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet);

	return send_packet(&packet, 0);
}
