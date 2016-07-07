#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

#define WSC_UDP_ADDR   "127.0.0.1"
#define WSC_EVENT_PORT  38100
#define WSC_EAP_DATA_MAX_LENGTH 2048
#define WSC_RECVBUF_SIZE    40


int main(int argc, char ** argv)
{
    int    new_sock; 
    struct sockaddr_in to;
    char   buf[128] = {0};
    char   cmdbuf[128] = {0};

    if (argc <= 2) {
        if (!strcasecmp(argv[1], "pin")) {
       		printf("Usage: wsc_option [pin]\n exapmle: wsc_cfg pin 12345678\n");
       		return 0;
    	}
    	else if (!strcasecmp(argv[1], "passphrase")) {
       		printf("Usage: wsc_option [passphrase]\n exapmle: wsc_cfg passphrase 12345678\n");
       		return 0;
    	}
    	else if (!strcasecmp(argv[1], "ssid")) {
       		printf("Usage: wsc_option [ssid]\n exapmle: wsc_cfg ssid 12345678\n");
       		return 0;
    	}
    	else if (!strcasecmp(argv[1], "authentication")) {
       		printf("Usage: wsc_option [authentication]\n exapmle: wsc_cfg authentication wpa\n");
       		return 0;
    	}
    	else if (!strcasecmp(argv[1], "cipher")) {
       		printf("Usage: wsc_option [cipher]\n exapmle: wsc_cfg cipher TKIP\n");
       		return 0;
    	}
    }
   
    if (!strcasecmp(argv[1], "pin")) {
       if ( argc == 3) {
          if(strlen(argv[2])!=8)
          {
              printf("Invalid pin entered, pin length must be 8!\n");
              return 0;
          }
       }
    }
    
    if (!strcasecmp(argv[1], "passphrase")) {
       if ( argc == 3) {
          if(strlen(argv[2]) < 8)
          {
              printf("Invalid passphrase entered, passphrase length must be at least 8!\n");
              return 0;
          }
       }
    }
    sprintf(buf, "%s %s", argv[1], argv[2]);
    
    new_sock = (int) socket(AF_INET, SOCK_DGRAM, 0); 

    bzero(&to,sizeof(to));

    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr(WSC_UDP_ADDR);
    to.sin_port = htons(WSC_EVENT_PORT);
    
    if(sendto(new_sock, buf, strlen(buf), 0,
              (struct sockaddr *) &to, sizeof(to))<0)
    {
        printf("Failed to send WSC config message: %s\n", buf);
        return -1;
    }
    
    /*
	The reason to put command operation here is because we at least ensure command was sendto wsccmd apps.
	We try to sync the wsc_cfg operation with madwifi driver operation.  It is make sense both did at the 
	same time. --hshao
    */
    if (!strcasecmp(argv[1], "ssid")) {
       if ( argc == 3) {
		strcpy(cmdbuf,"set ssid ");
		strcat(cmdbuf, argv[2]);
		strcat(cmdbuf, ";commit;get ssid"); 
		system(cmdbuf);
       }
    }
    else if (!strcasecmp(argv[1], "authentication")) {
       if ( argc == 3) {
		strcpy(cmdbuf,"set authentication ");
		strcat(cmdbuf, argv[2]);
		strcat(cmdbuf, ";commit;get authentication"); 
		system(cmdbuf);
       }
    }
    else if (!strcasecmp(argv[1], "cipher")) {
       if ( argc == 3) {
		strcpy(cmdbuf,"set cipher ");
		strcat(cmdbuf, argv[2]);
		strcat(cmdbuf, ";commit;get cipher"); 
		system(cmdbuf);
       }
    }
    else if (!strcasecmp(argv[1], "passphrase")) {
       if ( argc == 3) {
		strcpy(cmdbuf,"set passphrase ");
		strcat(cmdbuf, argv[2]);
		strcat(cmdbuf, ";commit;get passphrase"); 
		system(cmdbuf);
       }
    }

}

