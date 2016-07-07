#ifndef _SCRIPT_H
#define _SCRIPT_H

void run_script(struct dhcpMessage *packet, const char *name);
/* modified for enable Requested IP (option50) in DHCP Discover.wuchao@2013-5-2 */
unsigned long dhcpc_get_last_ip(void);
/* end modified by wuchao @2013-5-2 */
#endif
