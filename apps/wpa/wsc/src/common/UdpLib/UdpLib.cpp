/*============================================================================
//
// Copyright(c) 2006 Intel Corporation. All rights reserved.
//   All rights reserved.
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in
//       the documentation and/or other materials provided with the
//       distribution.
//     * Neither the name of Intel Corporation nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//  File Name: UdpLib.c
//  Description: Provides a simple UDP socket for communication between
//               different components.
//
****************************************************************************/

#ifdef _WIN32
//#include <windows.h>
#include <winsock2.h>
#endif // _WIN32
#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif // ___linux__

#include "tutrace.h"
#include "UdpLib.h"


/******************************************************************************
 * udp_open()
 *
 * This function opens a UDP socket and returns the socket to the calling
 * application. The application will use this socket in any subsequent calls.
 *
 * Returns socket handle on success or -1 if there is an error.
 ****************************************************************************/
int udp_open()
{
    int        new_sock;     // temporary socket handle
#ifdef _WIN32
    int           retval;
    WSADATA       wsa_data;
#endif // _WIN32

    TUTRACE((TUTRACE_DBG, "Entered udp_open\n"));

#ifdef _WIN32
    retval = WSAStartup(MAKEWORD(2, 0), &wsa_data);
    if (retval != 0)
    {
        TUTRACE((TUTRACE_ERR, "WSAStartup call failed.\n"));
        return -1;
    }
#endif // _WIN32

    // create INTERNET, udp datagram socket
    new_sock = (int) socket(AF_INET, SOCK_DGRAM, 0);
    if ( new_sock < 0 ) {
        TUTRACE((TUTRACE_ERR, "socket call failed.\n"));
        return -1;
    }

    TUTRACE((TUTRACE_DBG, "Socket open successful, sd: %d\n", new_sock));

    return new_sock;
}


/******************************************************************************
 * udp_bind(int fd, int portno)
 *
 * This call is used typically by the server application to establish a
 * well known port.
 *
 * Returns 1 if succeeds and returns -1 in case of an error.
 ****************************************************************************/
int udp_bind(int fd, int portno)
{
    struct sockaddr_in binder;

    TUTRACE((TUTRACE_DBG, "Entered udp_bind\n"));

    memset(&binder, 0, sizeof(binder));

    binder.sin_family = AF_INET;
    binder.sin_addr.s_addr = INADDR_ANY;
    binder.sin_port = htons(portno);
    // bind protocol to socket
    if (0 != bind(fd, (struct sockaddr *)&binder, sizeof(binder)))
    {
        TUTRACE((TUTRACE_ERR, strerror(errno)));
        TUTRACE((TUTRACE_ERR, "bind call for socket [%d] failed.\n", fd));
        return -1;
    }

    TUTRACE((TUTRACE_DBG, "Binding successful for socket [%d] with port %d\n", fd,
                        portno));

    return 1;
}


/******************************************************************************
 * udp_close(int fd)
 *
 * Closes a UDP session. Closes the socket and returns.
 ****************************************************************************/
void udp_close(int fd)
{
    TUTRACE((TUTRACE_DBG, "Entered udp_close\n"));

#ifdef _WIN32
    WSACleanup();
    closesocket(fd);
#endif // _WIN32

#ifdef __linux__
    close(fd);
#endif // __linux__

    return;
}


/******************************************************************************
 * udp_write(int fd, char * buf, int len, struct sockaddr_in * to)
 *
 * This function is called by the application to send data.
 * fd - socket handle
 * buf - data buffer
 * len - byte count of data in buf
 * to - socket address structure that contains remote ip address and port
 *      where the data is to be sent
 *
 * Returns bytesSent if succeeded. If there is an error -1 is returned
 ****************************************************************************/
int udp_write(int fd, char * buf, int len, struct sockaddr_in * to)
{
    int            bytes_sent;

    TUTRACE((TUTRACE_DBG, "Entered udp_write\n"));
    bytes_sent = sendto(fd, buf, len, 0,
                (struct sockaddr *)to,
                sizeof (struct sockaddr_in));
    if (bytes_sent < 0)
    {
        TUTRACE((TUTRACE_ERR, "sendto on socket %d failed\n", fd));
        return -1;
    }

    return bytes_sent;
}


/******************************************************************************
 * udp_read(int fd, char * buf, int len, struct sockaddr_in * from)
 *
 * This function is called by the application to receive data.
 * fd - socket handle
 * buf - data buffer in which the received data will be put
 * len - size of buf in bytes
 * from - socket address structure that contains remote ip address and port
 *        from where the data is received
 *
 * Returns bytes received if succeeded. If there is an error -1 is returned
 ****************************************************************************/
int udp_read(int fd, char * buf, int len, struct sockaddr_in * from)
{
    int bytes_recd = 0;
#ifdef WIN32
    int  fromlen = 0;
#endif
#ifdef __linux__
    socklen_t  fromlen = 0;
#endif
    fd_set        fdvar;
    int            sel_ret;
    TUTRACE((TUTRACE_DBG, "Entered udp_read\n"));

    FD_ZERO(&fdvar);
    // we have to wait for only one descriptor
    FD_SET(fd, &fdvar);


    sel_ret = select(fd + 1, &fdvar, (fd_set *) 0, (fd_set *) 0, NULL);

    // if select returns negative value, system error
    if (sel_ret < 0)
    {
        TUTRACE((TUTRACE_ERR, "select call failed; system error\n"));
        return -1;
    }

    // Otherwise Read notification might has come, since we are
    // waiting for only one fd we need not check the bit. Go ahead
    // and read the packet
    fromlen = sizeof (struct sockaddr_in);
    bytes_recd = recvfrom(fd, buf, len, 0,
                (struct sockaddr *)from, &fromlen);
    if (bytes_recd < 0)
    {
        TUTRACE((TUTRACE_ERR, "recvfrom on socket %d failed\n", fd));
        return -1;
    }
    return bytes_recd;
}
/******************************************************************************
 * udp_read_timed(int fd, char * buf, int len,
 *                  struct sockaddr_in * from, int timeout)
 *
 * This function is called by the application to receive data.
 * fd - socket handleTUTRACE((TUTRACE_DBG, "Entered udp_read\n"));


 * buf - data buffer in which the received data will be put
 * len - size of buf in bytes
 * from - socket address structure that contains remote ip address and port
 *        from where the data is received
 * timeout - wait time in seconds
 *
 * Returns bytes received if succeeded. If there is an error -1 is returned
 ****************************************************************************/
int udp_read_timed(int fd, char * buf, int len,
            struct sockaddr_in * from, int timeout)
{
    int bytes_recd = 0;
    unsigned int fromlen = 0;
    fd_set        fdvar;
    int            sel_ret;
    struct timeval tv;

    //TUTRACE((TUTRACE_DBG, "Entered udp_read\n"));

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&fdvar);
    // we have to wait for only one descriptor
    FD_SET(fd, &fdvar);

    sel_ret = select(fd + 1, &fdvar, (fd_set *) 0, (fd_set *) 0, &tv);

    // if select returns negative value, system error
    if (sel_ret < 0)
    {
        TUTRACE((TUTRACE_ERR, "select call failed; system error\n"));
        return -1;
    }
    else if (sel_ret == 0)
    {
        //TUTRACE((TUTRACE_ERR, "select call timeout; system error\n"));
        return -2;
    }

    // Otherwise Read notification might has come, since we are
    // waiting for only one fd we need not check the bit. Go ahead
    // and read the packet
    fromlen = sizeof (struct sockaddr_in);
    bytes_recd = recvfrom(fd, buf, len, 0,
                (struct sockaddr *)from, &fromlen);
    if (bytes_recd < 0)
    {
        TUTRACE((TUTRACE_ERR, "recvfrom on socket %d failed\n", fd));
        return -1;
    }

    return bytes_recd;
}

int get_mac_address(char *ifname, char *mac)
{
#ifdef __linux__
	int fd, rtn;
	struct ifreq ifr;

	if( !ifname || !mac ) {
        	TUTRACE((TUTRACE_ERR, "get_mac_address: NULL inputs\n"));
		return -1;
	}
	fd = socket(AF_INET, SOCK_DGRAM, 0 );
    	if ( fd < 0 ) {
        	TUTRACE((TUTRACE_ERR, "socket call failed.\n"));
        	return -1;
    	}
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ - 1 );

	if ( (rtn = ioctl(fd, SIOCGIFHWADDR, &ifr) ) == 0 )
		memcpy(	mac, (unsigned char *)ifr.ifr_hwaddr.sa_data, 6);
	close(fd);
	return rtn;
#endif
}
