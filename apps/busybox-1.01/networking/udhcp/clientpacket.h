#ifndef _CLIENTPACKET_H
#define _CLIENTPACKET_H

#include "packet.h"

/*
 * addeb by tiger 20090304, for reply mode setting 
 */
typedef enum _dhcp_flags_t
{
    DHCP_FLAGS_BROADCAST = 0,
    DHCP_FLAGS_UNICAST
} dhcp_flags_t;

void set_runtime_dhcp_flags (dhcp_flags_t flags);
int get_runtime_dhcp_flags (void);
void invert_runtime_dhcp_flags (void);

unsigned long random_xid(void);
int send_discover(unsigned long xid, unsigned long requested);
int send_selecting(unsigned long xid, unsigned long server, unsigned long requested);
int send_renew(unsigned long xid, unsigned long server, unsigned long ciaddr);
int send_release(unsigned long server, unsigned long ciaddr);
int send_decline(uint32_t xid, uint32_t server, uint32_t requested);
int get_raw_packet(struct dhcpMessage *payload, int fd);

#endif

