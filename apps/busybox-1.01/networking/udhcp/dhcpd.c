/* dhcpd.c
 *
 * udhcp Server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
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

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>

#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "options.h"
#include "files.h"
#include "serverpacket.h"
#include "common.h"
#include "signalpipe.h"
#include "static_leases.h"
#include "libmsglog.h"

#define DHCPS_SHARED_MEM_SIZE		(16 * 1024)	/* 16KB for 253 clients */
#define DHCPS_SHARED_MEM_KEY_ID		0x2F

/* globals */
struct dhcpOfferedAddr *leases;
struct dhcpOfferedAddr *leases_shm; 
struct server_config_t server_config;


int dhcps_shm_init()
{
	int shmid;
	char* shmbuf;
   
	if ((shmid = shmget(DHCPS_SHARED_MEM_KEY_ID, DHCPS_SHARED_MEM_SIZE, 0)) < 0)
	{
		perror("shmget");
		return -1;
	}
   
	if ((shmbuf = shmat(shmid, 0, 0)) < (char*)0)
	{
		perror("shmat");
		return -1;
	}

	if ((server_config.max_leases * sizeof(struct dhcpOfferedAddr))
		> DHCPS_SHARED_MEM_SIZE)
	{
		printf("two many clients\n");
		return -1;
	}
	
	//memset(shmbuf, 0, DHCPS_SHARED_MEM_SIZE);
	leases_shm = (struct dhcpOfferedAddr*) shmbuf;

	return 0;
}

int check_and_ack(struct dhcpMessage* packet, uint32_t ip)
{
	uint32_t static_ip = 0;
	
	/* There is an Static IP for this guy, whether it is requested or not */
	if ((static_ip = getIpByMac(server_config.static_leases, packet->chaddr)) != 0)
	{
		msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:REQUEST ip %x, static ip %x", ip, static_ip);
		return sendACK(packet, static_ip);
	}

	/* requested ip is reserved by a static lease -- if it is himself, situation above match */
	if (reservedIp(server_config.static_leases, ip))
	{
		msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:REQUEST ip %x is reserved as a static ip", ip);
		return sendNAK(packet);
	}
	
	/* if some one reserve it */
	if ( ip != packet->ciaddr && check_ip(packet, ip) )
	{
		msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:REQUEST ip %x already reserved by someone", ip);
		return sendNAK(packet);
	}

	if (ntohl(ip) < ntohl(server_config.start) || ntohl(ip) > ntohl(server_config.end))
	{
		msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:REQUEST ip %x is not in the address pool", ip);
		return sendNAK(packet);
	}
	
	return sendACK(packet, ip);
}

#ifdef COMBINED_BINARY
int udhcpd_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	fd_set rfds;
	struct timeval tv;
	int server_socket = -1;
	int bytes, retval;
	struct dhcpMessage packet;
	uint8_t *state;
	uint8_t *server_id, *requested;
	uint32_t server_id_align, requested_align;
	unsigned long timeout_end;
	struct option_set *option;
	struct dhcpOfferedAddr *lease;
	struct dhcpOfferedAddr static_lease;
	int max_sock;
	unsigned long num_ips;

	uint32_t static_lease_ip;
	
	memset(&server_config, 0, sizeof(struct server_config_t));
	read_config(argc < 2 ? DHCPD_CONF_FILE : argv[1]);

	/* Start the log, sanitize fd's, and write a pid file */
	start_log_and_pid("udhcpd", server_config.pidfile);

	if ((option = find_option(server_config.options, DHCP_LEASE_TIME))) {
		memcpy(&server_config.lease, option->data + 2, 4);
		server_config.lease = ntohl(server_config.lease);
	}
	else server_config.lease = LEASE_TIME;

	/* it is not impossible */
	if (server_config.end < server_config.start)
	{
		uint32_t tmp_ip = server_config.start;
		server_config.start = server_config.end ;
		server_config.end = tmp_ip;
	}
	
	/* Sanity check */
	num_ips = ntohl(server_config.end) - ntohl(server_config.start) + 1;
	if (server_config.max_leases > num_ips) {
		LOG(LOG_ERR, "max_leases value (%lu) not sane, "
			"setting to %lu instead",
			server_config.max_leases, num_ips);
		server_config.max_leases = num_ips;
	}

	if (dhcps_shm_init() != 0)
		return -1;
	
	leases = xcalloc(server_config.max_leases, sizeof(struct dhcpOfferedAddr));
	memset(leases, 0, server_config.max_leases * sizeof(struct dhcpOfferedAddr));

	//read_leases(server_config.lease_file);

	if (read_interface(server_config.interface, &server_config.ifindex,
			   &server_config.server, server_config.arp) < 0)
		return 1;

#ifndef UDHCP_DEBUG
	background(server_config.pidfile); /* hold lock during fork. */
#endif

	/* Setup the signal pipe */
	udhcp_sp_setup();

	timeout_end = uptime()/*time(0)*/ + server_config.auto_time;
	while(1) { /* loop until universe collapses */		
		if (server_socket < 0)
			if ((server_socket = listen_socket(INADDR_ANY, SERVER_PORT, server_config.interface)) < 0) {
				LOG(LOG_ERR, "FATAL: couldn't create server socket, %m");
				return 2;
			}

		max_sock = udhcp_sp_fd_set(&rfds, server_socket);
		if (server_config.auto_time) {
			tv.tv_sec = timeout_end - uptime()/*time(0)*/;
			tv.tv_usec = 0;
		}
		if (!server_config.auto_time || tv.tv_sec > 0) {
			retval = select(max_sock + 1, &rfds, NULL, NULL,
					server_config.auto_time ? &tv : NULL);
		} else retval = 0; /* If we already timed out, fall through */

		if (retval == 0) {
			write_leases();
			timeout_end = uptime()/*time(0)*/ + server_config.auto_time;
			continue;
		} else if (retval < 0 && errno != EINTR) {
			DEBUG(LOG_INFO, "error on select");
			continue;
		}

		switch (udhcp_sp_read(&rfds)) {
		case SIGUSR1:
			LOG(LOG_INFO, "Received a SIGUSR1");
			
			write_leases();
			/* why not just reset the timeout, eh */
			timeout_end = uptime()/*time(0)*/ + server_config.auto_time;
			continue;
		case SIGUSR2:
			LOG(LOG_INFO, "Received a SIGUSR2, now DHCP SERVER restart");
			
			if (execv(argv[0], argv) == -1)
				printf("errno:%d\n", errno);
			/* NEVER reach here except execv failed! */			
			break;
		case SIGTERM:
			LOG(LOG_INFO, "Received a SIGTERM");
			
			return 0;
		case 0: break;		/* no signal */
		default: continue;	/* signal or error (probably EINTR) */
		}

		if ((bytes = get_packet(&packet, server_socket)) < 0) { /* this waits for a packet - idle */
			if (bytes == -1 && errno != EINTR) {
				DEBUG(LOG_INFO, "error on read, %m, reopening socket");
				close(server_socket);
				server_socket = -1;
			}
			continue;
		}

		if ((state = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL) {
			DEBUG(LOG_ERR, "couldn't get option from packet, ignoring");
			continue;
		}

		//printBuf(&packet, sizeof(struct dhcpMessage));
		
		lease = find_lease_by_chaddr(packet.chaddr);

		if (!lease)
		{
			/* Look for a static lease */
			static_lease_ip = getIpByMac(server_config.static_leases, &packet.chaddr);

			/* found */
			if(static_lease_ip)
			{
				memcpy(&static_lease.chaddr, &packet.chaddr, 16);
				static_lease.yiaddr = static_lease_ip;
				static_lease.expires = 0;

				lease = &static_lease;
			}
			/*
			else
			{
				lease = find_lease_by_chaddr(packet.chaddr);
			}
			*/
		}
		
		
		switch (state[0]) {
		case DHCPDISCOVER:
			//DEBUG(LOG_INFO,"received DISCOVER");
			msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Recv DISCOVER from %02X:%02X:%02X:%02X:%02X:%02X", packet.chaddr[0],
				packet.chaddr[1], packet.chaddr[2], packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);

			if (sendOffer(&packet) < 0) {
				LOG(LOG_ERR, "send OFFER failed");
			}
			break;
 		case DHCPREQUEST:
			//DEBUG(LOG_INFO, "received REQUEST");			
			msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Recv REQUEST from %02X:%02X:%02X:%02X:%02X:%02X", packet.chaddr[0],
							packet.chaddr[1], packet.chaddr[2], packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);

			requested = get_option(&packet, DHCP_REQUESTED_IP);
			server_id = get_option(&packet, DHCP_SERVER_ID);

			if (requested) memcpy(&requested_align, requested, 4);
			if (server_id) memcpy(&server_id_align, server_id, 4);

			if (lease) {
				if (server_id) {
					/* SELECTING State */
					DEBUG(LOG_INFO, "server_id = %08x", ntohl(server_id_align));
					if (server_id_align == server_config.server && requested &&
					    requested_align == lease->yiaddr) {
						sendACK(&packet, lease->yiaddr);
					}
					else 
					{
						msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Wrong Server id or request an invalid ip");
						sendNAK(&packet);
					}
				} else {
					if (requested) {
						/* INIT-REBOOT State */
						if (lease->yiaddr == requested_align)
							sendACK(&packet, lease->yiaddr);
						else 
						{
							msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Server id not found and request an invalid ip");
							sendNAK(&packet);
						}
					} else {
						/* RENEWING or REBINDING State */
						if (lease->yiaddr == packet.ciaddr)
							sendACK(&packet, lease->yiaddr);
						else {
							/* don't know what to do!!!! */
							msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Server id and requested ip not found");
							msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:%x %x", lease->yiaddr, packet.ciaddr);
							sendNAK(&packet);
						}
					}
				}

			/* what to do if we have no record of the client */
			} else if (server_id) {
				/* SELECTING State */

			}
			else if (requested) 
			{
				/* INIT-REBOOT State */
				if ((lease = find_lease_by_yiaddr(requested_align))) 
				{
					/* Requested IP already reserved by other one */
					
					if (lease_expired(lease))
					{
						/* probably best if we drop this lease */
						memset(lease->chaddr, 0, 16);

						check_and_ack(&packet, requested_align);
					/* make some contention for this address */
					} 
					else	/* still reserved by someone */
					{
						msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:REQUEST ip %x already reserved by %02X:%02X:%02X:%02X:%02X:%02X",
							requested_align, lease->chaddr[0], lease->chaddr[1], lease->chaddr[2],
							lease->chaddr[3], lease->chaddr[4], lease->chaddr[5]);
						sendNAK(&packet);
					}
				}
				else /*if (requested_align < server_config.start ||
					   requested_align > server_config.end)*/ 
				{
					check_and_ack(&packet, requested_align);
				} /* else remain silent */
			} 
			else 
			{
                /* error state, just reply NAK modified by tiger 20090927 */
                 sendNAK(&packet);
			}
			break;
		case DHCPDECLINE:
			//DEBUG(LOG_INFO,"received DECLINE");
			msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Recv DECLINE from %02X:%02X:%02X:%02X:%02X:%02X", packet.chaddr[0],
										packet.chaddr[1], packet.chaddr[2], packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);
			
			if (lease) {
				memset(lease->chaddr, 0, 16);
				lease->expires = uptime()/*time(0)*/ + server_config.decline_time;
			}
			break;
		case DHCPRELEASE:
			//DEBUG(LOG_INFO,"received RELEASE");			
			msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Recv RELEASE from %02X:%02X:%02X:%02X:%02X:%02X", packet.chaddr[0],
										packet.chaddr[1], packet.chaddr[2], packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);
			if (lease)
			{
				/* Delete the lease, lsz 080221 */
				#if 1
				memset(lease, 0, sizeof(struct dhcpOfferedAddr));
				#else
				lease->expires = uptime()/*time(0)*/;
				#endif
			}
			
			break;
		case DHCPINFORM:
			//DEBUG(LOG_INFO,"received INFORM");
			msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPS:Recv INFORM from %02X:%02X:%02X:%02X:%02X:%02X", packet.chaddr[0],
										packet.chaddr[1], packet.chaddr[2], packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);
			send_inform(&packet);
			break;
		default:
			LOG(LOG_WARNING, "unsupported DHCP message (%02x) -- ignoring", state[0]);
		}
	}

	return 0;
}

