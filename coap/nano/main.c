/*
 * Copyright (C) 2021 Peter Sjödin, KTH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/*
 * 6LoWPAN border router 
 * 
 * Assign static IPv6 prefix to WPAN interface
 * and become DODAG root on RPL instance
 */

#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "msg.h"

#include "net/gnrc.h"
#include "net/gnrc/rpl.h"

#include <net/nanocoap_sock.h>

#define _IPV6_DEFAULT_PREFIX_LEN 64

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
char thread_stack[2048]; //THREAD_STACKSIZE_MAIN];


kernel_pid_t coap_pid;

static const shell_command_t shell_commands[] =
{
	{NULL, NULL, NULL}
};

/*
 * Initiate RPL on an interface given by name (a number)
 */
static void start_rpl(char *if_name) {
    kernel_pid_t if_pid = atoi(if_name);
    gnrc_rpl_init(if_pid);
    printf("started RPL on interface %d\n", if_pid);
}

/*
 * Assign an IPv6 address and a prefix length on an interface
 * given by name (a number)
 */
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

/*
 * Become RPL root on a given instance 
 */
static int add_rpl_root(ipv6_addr_t *addr, uint8_t instance_id) {
    gnrc_rpl_instance_t *inst = gnrc_rpl_root_init(instance_id, addr, false, false);
    if (inst == NULL) {
      printf("error: could not create rpl root\n");
        return 1;
    }
    return 0;
}

void* coaperator(void* arg)
{
	//printf("PID = %i", coap_pid);
	printf("starting \"coaperator\"\n");
	(void) arg;
	uint8_t buf[512];
	sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
	if(nanocoap_server(&local, buf, sizeof(buf)) == -1)
		puts("Error binding to local, or UDP reception failed");
	return NULL;
}

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

#ifdef RPL_STATIC_NETIF
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
#endif // RPL_STATIC_NETIF 

    /* start shell */
    puts("Border router running");

	//initiate CoAP server
	coap_pid = thread_create(thread_stack, sizeof(thread_stack), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, coaperator, NULL, "CoAP server thread");
	//coaperator(NULL);

    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
