/* dhcpc.c
 *
 * udhcp DHCP client
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

#include <sys/time.h>
#include <sys/file.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#include "dhcpd.h"
#include "dhcpc.h"
#include "options.h"
#include "clientpacket.h"
#include "clientsocket.h"
#include "script.h"
#include "socket.h"
#include "common.h"
#include "signalpipe.h"
#include "libmsglog.h"

static int state;
static unsigned long requested_ip; /* = 0 */
static unsigned long server_addr;
static unsigned long timeout;
static int packet_num; /* = 0 */
static int fd = -1;

#define LISTEN_NONE 0
#define LISTEN_KERNEL 1
#define LISTEN_RAW 2
static int listen_mode;

struct client_config_t client_config = {
	/* Default options. */
	abort_if_no_lease: 0,
	foreground: 0,
	quit_after_lease: 0,
	background_if_no_lease: 0,
	interface: "eth0",
	pidfile: NULL,
	script: DEFAULT_SCRIPT,
	clientid: NULL,
	hostname: NULL,
	ifindex: 0,
	arp: "\0\0\0\0\0\0",		/* appease gcc-3.0 */
};

/* 0 means false, tell server to reply in BROADCAST mode */
	
#ifndef IN_BUSYBOX
static void __attribute__ ((noreturn)) show_usage(void)
{
	printf(
"Usage: udhcpc [OPTIONS]\n\n"
"  -c, --clientid=CLIENTID         Set client identifier\n"
"  -C, --clientid-none             Suppress default client identifier\n"
"  -H, --hostname=HOSTNAME         Client hostname\n"
"  -h                              Alias for -H\n"
"  -f, --foreground                Do not fork after getting lease\n"
"  -b, --background                Fork to background if lease cannot be\n"
"                                  immediately negotiated.\n"
"  -i, --interface=INTERFACE       Interface to use (default: eth0)\n"
"  -n, --now                       Exit with failure if lease cannot be\n"
"                                  immediately negotiated.\n"
"  -p, --pidfile=file              Store process ID of daemon in file\n"
"  -q, --quit                      Quit after obtaining lease\n"
"  -r, --request=IP                IP address to request (default: none)\n"
"  -s, --script=file               Run file at dhcp events (default:\n"
"                                  " DEFAULT_SCRIPT ")\n"
"  -v, --version                   Display version\n"
	);
	exit(0);
}
#else

#define show_usage bb_show_usage

extern void show_usage(void) __attribute__ ((noreturn));

#endif

#define PHILIPPINES_DHCPC_SEND_ARP /* by lqt, 2010-2-9, only for PHILIPPINES dhcpc problem */


/* just a little helper */
static void change_mode(int new_mode)
{
	DEBUG(LOG_INFO, "entering %s listen mode",
		new_mode ? (new_mode == 1 ? "kernel" : "raw") : "none");
	if (fd >= 0) close(fd);
	fd = -1;
	listen_mode = new_mode;
}


/* perform a renew */
static void perform_renew(void)
{
	msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC perform a DHCP renew");
	switch (state) {
	case BOUND:
		change_mode(LISTEN_KERNEL);
	case RENEWING:
	case REBINDING:
		state = RENEW_REQUESTED;
		break;
	case RENEW_REQUESTED: /* impatient are we? fine, square 1 */
		run_script(NULL, "deconfig");
	case REQUESTING:
	case RELEASED:
		change_mode(LISTEN_RAW);
		state = INIT_SELECTING;
		break;
	case INIT_SELECTING:
		break;
	}

	/* start things over */
	packet_num = 0;

	/* Kill any timeouts because the user wants this to hurry along */
	timeout = 0;
}


/* perform a release */
static void perform_release(void)
{
	char buffer[16];
	struct in_addr temp_addr;

	/* send release packet */
	if (state == BOUND || state == RENEWING || state == REBINDING) {
		temp_addr.s_addr = server_addr;
		sprintf(buffer, "%s", inet_ntoa(temp_addr));
		temp_addr.s_addr = requested_ip;
		msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC Unicasting a release of %s to %s",
				inet_ntoa(temp_addr), buffer);
		if ( send_release(server_addr, requested_ip) < 0) 
		{
			/*  added by tiger 20090304, sometimes theck return value is important */
			DEBUG(LOG_INFO, LOGTYPE_DHCP, "send_release error\n");
		}
		
		run_script(NULL, "release");
	}
	msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC Entering released state");

	change_mode(LISTEN_NONE);
	state = RELEASED;
	timeout = 0x7fffffff;
}


static void client_background(void)
{
	background(client_config.pidfile);
	client_config.foreground = 1; /* Do not fork again. */
	client_config.background_if_no_lease = 0;
}

/* 070707, LiShaozhang add the unicast opt */
#ifdef COMBINED_BINARY
int udhcpc_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	uint8_t *temp, *message;
	unsigned long t1 = 0, t2 = 0, xid = 0;
	unsigned long start = 0, lease;
	fd_set rfds;
	int retval;
	struct timeval tv;
	int c, len;
	struct dhcpMessage packet;
	struct in_addr temp_addr;
	long now;
	int max_fd;
	int sig;
	int no_clientid = 0;
	int fail_times = 0;		/* how many times that we fail to find a dhcp server */
	int server_unicast = 0;		


	static const struct option arg_options[] = {
		{"clientid",	required_argument,	0, 'c'},
		{"clientid-none", no_argument,		0, 'C'},
		{"foreground",	no_argument,		0, 'f'},
		{"background",	no_argument,		0, 'b'},
		{"hostname",	required_argument,	0, 'H'},
		{"hostname",    required_argument,      0, 'h'},
		{"interface",	required_argument,	0, 'i'},
		{"now", 	no_argument,		0, 'n'},
		{"pidfile",	required_argument,	0, 'p'},
		{"quit",	no_argument,		0, 'q'},
		{"request",	required_argument,	0, 'r'},
		{"script",	required_argument,	0, 's'},
		{"unicast",	no_argument,		0, 'u'},	/* unicast flag */
		{"version",	no_argument,		0, 'v'},
		{0, 0, 0, 0}
	};

	/* get options */
	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "c:CfbH:h:i:np:qr:s:uv", arg_options, &option_index);
		if (c == -1) break;

		switch (c) {
		case 'c':
			if (no_clientid) show_usage();
			len = strlen(optarg) > 255 ? 255 : strlen(optarg);
			if (client_config.clientid) free(client_config.clientid);
			client_config.clientid = xmalloc(len + 2);
			client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
			client_config.clientid[OPT_LEN] = len;
			client_config.clientid[OPT_DATA] = '\0';
			strncpy(client_config.clientid + OPT_DATA, optarg, len);
			break;
		case 'C':
			if (client_config.clientid) show_usage();
			no_clientid = 1;
			break;
		case 'f':
			client_config.foreground = 1;
			break;
		case 'b':
			client_config.background_if_no_lease = 1;
			break;
		case 'h':
		case 'H':
			len = strlen(optarg) > 255 ? 255 : strlen(optarg);
			if (client_config.hostname) free(client_config.hostname);
			client_config.hostname = xmalloc(len + 2);
			client_config.hostname[OPT_CODE] = DHCP_HOST_NAME;
			client_config.hostname[OPT_LEN] = len;
			strncpy(client_config.hostname + 2, optarg, len);
			break;
		case 'i':
			client_config.interface =  optarg;
			break;
		case 'n':
			client_config.abort_if_no_lease = 1;
			break;
		case 'p':
			client_config.pidfile = optarg;
			break;
		case 'q':
			client_config.quit_after_lease = 1;
			break;
		case 'r':
			requested_ip = inet_addr(optarg);
			break;
		case 's':
			client_config.script = optarg;
			break;
		case 'u':
			server_unicast = 1;
			break;
		case 'v':
			printf("udhcpcd, version %s\n\n", VERSION);
			return 0;
			break;
		default:
			show_usage();
		}
	}

	/* Start the log, sanitize fd's, and write a pid file */
	start_log_and_pid("udhcpc", client_config.pidfile);

	if (read_interface(client_config.interface, &client_config.ifindex,
			   NULL, client_config.arp) < 0)
		return 1;

	/* if not set, and not suppressed, setup the default client ID */
	if (!client_config.clientid && !no_clientid) {
		client_config.clientid = xmalloc(6 + 3);
		client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
		client_config.clientid[OPT_LEN] = 7;
		client_config.clientid[OPT_DATA] = 1;
		memcpy(client_config.clientid + 3, client_config.arp, 6);
	}

	/* changed by lsz 070621 */
	client_background();
	
	/* setup the signal pipe */
	udhcp_sp_setup();

	//if (dhcpc_shm_init() != 0)
	//	return -1;
	#include "msgq.h"
	dhcp_ipc_fork(DHCPC);

	state = INIT_SELECTING;
	run_script(NULL, "deconfig");
	change_mode(LISTEN_RAW);

	for (;;) {
		tv.tv_sec = timeout - uptime();
		tv.tv_usec = 0;

		if (listen_mode != LISTEN_NONE && fd < 0) {
			if (listen_mode == LISTEN_KERNEL)
				fd = listen_socket(INADDR_ANY, CLIENT_PORT, client_config.interface);
			else
				fd = raw_socket(client_config.ifindex);
			if (fd < 0) {
				LOG(LOG_ERR, "FATAL: couldn't listen on socket, %m");
				return 0;
			}
		}
		
        /* 
         * select don't return when timeout value is larger than 1.5 hours
         * we just wait multiple times
         * added by tiger 090819, should fix later
         */
        struct timeval tp_timeout;
        #define TP_TIMEOUT_MAX  (30*60)
        
		if (tv.tv_sec > 0) 
		{
            do
            {
                max_fd = udhcp_sp_fd_set(&rfds, fd);
                
                tp_timeout.tv_sec = (tv.tv_sec > TP_TIMEOUT_MAX) ? TP_TIMEOUT_MAX : tv.tv_sec;
                tv.tv_sec -= tp_timeout.tv_sec;                                
                tp_timeout.tv_usec = 0;

                retval = select(max_fd + 1, &rfds, NULL, NULL, &tp_timeout);                
                
            } while (tv.tv_sec > 0 && retval == 0);
		}
		else
		{
			retval = 0; /* If we already timed out, fall through */
		}
        
		now = uptime();
		if (retval == 0) {
			/* timeout dropped to zero */
			switch (state) {
			case INIT_SELECTING:
               
#define     DISCOVER_RETRY_TIMES    5
#define     DISCOVER_INVERT_TIMES   3

				if (packet_num < DISCOVER_RETRY_TIMES) {
					if (packet_num == 0)
					{
						xid = random_xid();
                        /* use user config dhcp flags when first discover, added by tiger 20090821 */
                        if (server_unicast)
                        {
                            set_runtime_dhcp_flags(DHCP_FLAGS_UNICAST);
                        }
                        else
                        {
                            set_runtime_dhcp_flags(DHCP_FLAGS_BROADCAST);                            
                        }
					}

                    /* change runtime dhcp flags when exceed DISCOVER_INVERT_TIMES added by tiger 20090819 apply 11G and XP's option */
                    if (DISCOVER_INVERT_TIMES == packet_num)
                    {                        
                        invert_runtime_dhcp_flags();
                    }
					/* modified for enable Requested IP (option50) in DHCP Discover.wuchao@2013-5-2 */
                    requested_ip = dhcpc_get_last_ip();
					/* end modify, wuchao@2013-5-2 */
					/* send discover packet */
					//send_discover(xid, requested_ip, server_unicast); /* broadcast */
					/* modified by tiger 20090304, reply mode's setting way changed */
					send_discover(xid, requested_ip);
					msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC Send DISCOVER with request ip %X and unicast flag %d", requested_ip, get_runtime_dhcp_flags());                    
                    
					timeout = now + ((packet_num == 2) ? 4 : 2);
					packet_num++;
				} else {
					run_script(NULL, "leasefail");
					msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC DHCP Service unavailable, recv no OFFER");
					if (client_config.background_if_no_lease) {
						LOG(LOG_INFO, "No lease, forking to background.");
						client_background();
					} else if (client_config.abort_if_no_lease) {
						LOG(LOG_INFO, "No lease, failing.");
						return 1;
				  	}
					/* wait to try again */
					packet_num = 0;
					
					timeout = now + 10 + (fail_times ++) * 30;	
					/* 60->6000, we dont need to try again -- lsz, 080722 */
					/* 6000->30*fail_times -- lsz, 081008 */
				}
				break;
			case RENEW_REQUESTED:
			case REQUESTING:
				if (packet_num < 3) {
					/* send request packet */
					if (state == RENEW_REQUESTED)
					{
						send_renew(xid, server_addr, requested_ip); /* unicast */
					}
					else
					{
						send_selecting(xid, server_addr, requested_ip); /* broadcast */
					}

					msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC Send REQUEST to server %x with request ip %x", server_addr, requested_ip);

					timeout = now + ((packet_num == 2) ? 10 : 2);
					packet_num++;
				} else {
					/* timed out, go back to init state */
					if (state == RENEW_REQUESTED) run_script(NULL, "deconfig");
					state = INIT_SELECTING;
					timeout = now;
					packet_num = 0;
					change_mode(LISTEN_RAW);
				}
				break;
			case BOUND:
				/* Lease is starting to run out, time to enter renewing state */
				state = RENEWING;
				change_mode(LISTEN_KERNEL);
				DEBUG(LOG_INFO, "Entering renew state");
				/* fall right through */
			case RENEWING:
				/* Either set a new T1, or enter REBINDING state */
				if ((t2 - t1) <= (lease / 14400 + 1)) {
					/* timed out, enter rebinding state */
					state = REBINDING;
					timeout = now + (t2 - t1);
					DEBUG(LOG_INFO, "Entering rebinding state");
				} else {
					/* send a request packet */				
					send_renew(xid, server_addr, requested_ip); /* unicast */

					msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC Send REQUEST to server %x with request ip %x", server_addr, requested_ip);
              
					t1 = (t2 - t1) / 2 + t1;
					timeout = t1 + start;
				}
				break;
			case REBINDING:
				/* Either set a new T2, or enter INIT state */
				if ((lease - t2) <= (lease / 14400 + 1)) {
					/* timed out, enter init state */
					state = INIT_SELECTING;
					LOG(LOG_INFO, "Lease lost, entering init state");
					run_script(NULL, "deconfig");
					timeout = now;
					packet_num = 0;
					change_mode(LISTEN_RAW);
				} else {
					/* send a request packet */
					send_renew(xid, 0, requested_ip); /* broadcast */

					
					msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC Broadcast REQUEST with request ip %x", requested_ip);
					t2 = (lease - t2) / 2 + t2;
					timeout = t2 + start;
				}
				break;
			case RELEASED:
				/* yah, I know, *you* say it would never happen */
				timeout = 0x7fffffff;
				break;
			}
		}
		else if (retval > 0 && listen_mode != LISTEN_NONE && FD_ISSET(fd, &rfds)) {
			/* a packet is ready, read it */
			if (listen_mode == LISTEN_KERNEL)
				len = get_packet(&packet, fd);
			else len = get_raw_packet(&packet, fd);

			if (len == -1 && errno != EINTR) {
				DEBUG(LOG_INFO, "error on read, %m, reopening socket");
				change_mode(listen_mode); /* just close and reopen */
			}
			if (len < 0) continue;

			if (packet.xid != xid) {
				DEBUG(LOG_INFO, "Ignoring XID %lx (our xid is %lx)",
					(unsigned long) packet.xid, xid);
				continue;
			}
			/* Ignore packets that aren't for us */
			if (memcmp(packet.chaddr, client_config.arp, 6)) {
				DEBUG(LOG_INFO, "packet does not have our chaddr -- ignoring");
				continue;
			}

			if ((message = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL) {
				DEBUG(LOG_ERR, "couldnt get option from packet -- ignoring");
				continue;
			}

			switch (state) {
			case INIT_SELECTING:
				/* Must be a DHCPOFFER to one of our xid's */
				if (*message == DHCPOFFER) {
					if ((temp = get_option(&packet, DHCP_SERVER_ID))) {
						memcpy(&server_addr, temp, 4);
						xid = packet.xid;
						requested_ip = packet.yiaddr;

						msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC Recv OFFER from server %x with ip %x", server_addr, requested_ip);

						/* enter requesting state */
						state = REQUESTING;
						timeout = now;
						packet_num = 0;
					} else {
						DEBUG(LOG_ERR, "No server ID in message");
					}
				}
				break;
			case RENEW_REQUESTED:
			case REQUESTING:
			case RENEWING:
			case REBINDING:
				if (*message == DHCPACK) {
					if (!(temp = get_option(&packet, DHCP_LEASE_TIME))) {
						LOG(LOG_ERR, "No lease time with ACK, using 1 hour lease");
						lease = 60 * 60;
					} else {
						memcpy(&lease, temp, 4);
						lease = ntohl(lease);
					}

/* RFC 2131 3.1 paragraph 5:
 * "The client receives the DHCPACK message with configuration
 * parameters. The client SHOULD perform a final check on the
 * parameters (e.g., ARP for allocated network address), and notes
 * the duration of the lease specified in the DHCPACK message. At this
 * point, the client is configured. If the client detects that the
 * address is already in use (e.g., through the use of ARP),
 * the client MUST send a DHCPDECLINE message to the server and restarts
 * the configuration process..." 
 * added by tiger 20090827
 */                    
					if (!arpping(packet.yiaddr,
						    (uint32_t) 0,
						    packet.yiaddr,
						    client_config.arp,
						    client_config.interface)
					) {
					
						msglogd (LOG_INFO, LOGTYPE_DHCP, "DHCPC: offered address is in use "
							"(got ARP reply), Send decline");
						send_decline(xid, server_addr, packet.yiaddr);

						if (state != REQUESTING)
							run_script(NULL, "deconfig");
						change_mode(LISTEN_RAW);
						state = INIT_SELECTING;
						requested_ip = 0;
						timeout = now + 12;
						packet_num = 0;
						continue; /* back to main loop */
					}
                    
					/* enter bound state */
					t1 = lease / 2;

					/* little fixed point for n * .875 */
					t2 = (lease * 0x7) >> 3;
					temp_addr.s_addr = packet.yiaddr;
					LOG(LOG_INFO, "Lease of %s obtained, lease time %ld",
						inet_ntoa(temp_addr), lease);
					start = now;
					timeout = t1 + start;
					requested_ip = packet.yiaddr;

					if ((temp = get_option(&packet, DHCP_SERVER_ID))) 
						memcpy(&server_addr, temp, 4);
					msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC Recv ACK from server %x with ip %x lease time %ld", 
						server_addr, requested_ip, lease);
					
					run_script(&packet,
						   ((state == RENEWING || state == REBINDING) ? "renew" : "bound"));

					fail_times = 0;		/* clear the retry counter */
					state = BOUND;
					change_mode(LISTEN_NONE);
					if (client_config.quit_after_lease)
						return 0;
					if (!client_config.foreground)
						client_background();

				} else if (*message == DHCPNAK) {
					/* return to init state */
					LOG(LOG_INFO, "Received DHCP NAK");

					if ((temp = get_option(&packet, DHCP_SERVER_ID))) 
						memcpy(&server_addr, temp, 4);
					msglogd(LOG_INFO, LOGTYPE_DHCP, "DHCPC Recv NAK from server %x with ip %x", server_addr, requested_ip);

					run_script(&packet, "nak");
					if (state != REQUESTING)
						run_script(NULL, "deconfig");
					state = INIT_SELECTING;
					timeout = now + 3;	/* change by lsz 080905, without this 3 seconds,
										 * the udhcpc will keep on trying and the release
										 * msg cant be recved by udhcpc, even if we are
										 * wan static ip now, the udhcpc is still sending 
										 * discover pkts. 
										 */
					requested_ip = 0;
					packet_num = 0;
					change_mode(LISTEN_RAW);
					//sleep(3); /* avoid excessive network traffic */
				}
				break;
			/* case BOUND, RELEASED: - ignore all packets */
			}
		} 
		else if (retval > 0 && (sig = udhcp_sp_read(&rfds))) {
			printf("sig:%d\n", sig);
			switch (sig) {
			case DHCPC_RENEW://case SIGUSR1:
				printf("renew\n");
				perform_renew();
				break;
			case DHCPC_RELEASE://SIGUSR2:
				printf("release\n");
				perform_release();
				break;

			/* added by tiger 20090304, for dhcpc change to unicast or broadcast mode without reboot */
			case DHCPC_UNICAST_RENEW: /* for unicast dhcp */
				printf ("unicast renew\n");
				server_unicast = 1;
				perform_renew();
				break;
			case DHCPC_BROADCAST_RENEW: /* for broadcast */
				printf ("broadcast renew\n");
				server_unicast = 0;
				perform_renew();
				break;

            case DHCPC_MAC_CLONE: /* WAN MAC clone */
                perform_release();
                /* so ugly */
                if (read_interface(client_config.interface, &client_config.ifindex,
			                        NULL, client_config.arp) < 0)
                {
                    msglogd (LOG_INFO, LOGTYPE_DHCP, "DHCPC: Error after mac clone, can't read WAN interface information");
                }
                else
                {
                    msglogd (LOG_INFO, LOGTYPE_DHCP, "DHCPC: mac clone, re-read WAN interface information");
                }
                break;

			case SIGTERM:
				LOG(LOG_INFO, "Received SIGTERM");
				return 0;
			}
		} else if (retval == -1 && errno == EINTR) {
			/* a signal was caught */
		} else {
			/* An error occured */
			DEBUG(LOG_ERR, "Error on select");
		}

	}
	return 0;
}
