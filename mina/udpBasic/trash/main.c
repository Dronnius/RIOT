//udpBasic/main.c
//MISSING THE BLOODY EHTERNET INTERFACE, DO SOMETHING ABOUT IT!
#include <stdio.h>
#include <string.h>
#include "msg.h"
#include "shell.h"
#include "net/gnrc/rpl.h"


#define _IPV6_DEFAULT_PREFIX_LEN 64 //sssssss

#define MSG_Q_SIZE 8
static msg_t msgQ[MSG_Q_SIZE];

int udp_listen(int argc, char **argv);

static const shell_command_t shell_commands[] = 
{
	{ "listen", "prints received udp packets", udp_listen},
	{NULL, NULL, NULL}
};

////////////////////////
static void start_rpl(char *if_name) {
    kernel_pid_t if_pid = atoi(if_name);
    gnrc_rpl_init(if_pid);
    printf("started RPL on interface %d\n", if_pid);
}


static int add_iface_prefix(char *if_name, ipv6_addr_t *addr, uint8_t prefix_len) {
    netif_t *iface = netif_get_by_name(if_name);
    uint16_t flags = GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID;
    flags |= (prefix_len << 8U);
    if (netif_set_opt(iface, NETOPT_IPV6_ADDR, flags, addr, sizeof(*addr)) < 0) {
      printf("error: unable to add IPv6 address\n");
      return 1;
    }
    return 0;
}


static int add_rpl_root(ipv6_addr_t *addr, uint8_t instance_id) {
    gnrc_rpl_instance_t *inst = gnrc_rpl_root_init(instance_id, addr, false, false);
    if (inst == NULL) {
      printf("error: could not create rpl root\n");
        return 1;
    }
    return 0;
}
////////////////////////////

int main(void)
{
	msg_init_queue(msgQ, MSG_Q_SIZE);
	printf("message queue initiated\n");
	
	//TEST BEGIN
	start_rpl(RPL_STATIC_NETIF);

   
     //Parse prefix and split into IPv6 address and prefix length 

    char ip6prefix[] = RPL_DODAG_ADDR;
    ipv6_addr_t addr;
    uint8_t prefix_len;

    prefix_len = ipv6_addr_split_int(ip6prefix, '/', _IPV6_DEFAULT_PREFIX_LEN);
    if (ipv6_addr_from_str(&addr, ip6prefix) == NULL) {
      printf("error: DODAG ID address error: %s", ip6prefix);
        return 1;
    }
    add_iface_prefix(RPL_STATIC_NETIF, &addr, prefix_len);
    add_rpl_root(&addr, RPL_INSTANCE_ID);
	//TEST END
	
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
	
	return 0;
}