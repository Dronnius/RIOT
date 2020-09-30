//udpBasic/udpListener.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "net/sock/udp.h"
#include "net/ipv6/addr.h"
#include "thread.h"

#define SRV_MSG_Q_SIZE 8
#define SRV_BUF_SIZE 64

static bool serving = false;
static sock_udp_t socket;
static char srv_buf[SRV_BUF_SIZE];
static char srv_stck[THREAD_STACKSIZE_DEFAULT];
static msg_t srv_msgQ[SRV_MSG_Q_SIZE];

static int srv_pid;

void mirror (char buffer[], int len)
{
	char swap;
	int j = len;
	for (int i = 0; i < (len >> 1); i++)
	{
		j--;
		swap = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = swap;
	}
	return;
}

void* udpServer(void *args)
{
	sock_udp_ep_t server = {.port = atoi(args), .family = AF_INET6};
	msg_init_queue(srv_msgQ, SRV_MSG_Q_SIZE);
	
	if(sock_udp_create(&socket, &server, NULL, 0) < 0)
	{
		puts("socket creation failed");
		return NULL;
	}
	
	serving = true;
	printf("Success: started udp listener on port %u\n", server.port);
	
	int res;
	sock_udp_ep_t remote;
	while(1) //INFINITE LOOP
	{
		if((res = sock_udp_recv(&socket, srv_buf, sizeof(srv_buf) - 1, SOCK_NO_TIMEOUT, &remote)) < 0)
			puts("Error receiving message");
		else if (res == 0)
			puts("Data received: (Empty)");
		else
		{
			srv_buf[res] = '\0';
			puts("Data received:");
			puts(srv_buf);
			puts("Hexadecimal format:");
			for(int i = 0; i < res; i++)
				printf("%x", srv_buf[i]);
			/*mirror(srv_buf, res);
			if(sock_udp_send(&socket, srv_buf, sizeof(srv_buf), &remote) < 0)
			{
				puts("Error sending reply");
			}*/
		}
	}
}

int udp_reflect(int argc, char **argv)
{
	if(argc != 2)
	{
		puts("Usage: listen <port>");
		return -1;
	}
	
	if(serving == false)
		if((srv_pid = thread_create(srv_stck, sizeof(srv_stck), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, udpServer, argv[1], "UDP Listener") ) <= KERNEL_PID_UNDEF)
				return -1;
	return 0;
}