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

//#include <stdio.h>
//#include <string.h>

//#include "shell.h"

#include "net/gnrc.h"
#include "net/gnrc/rpl.h"
#include "net/netif.h"
#include "handler.h"


#define _IPV6_DEFAULT_PREFIX_LEN 64

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

const netopt_unit custom_netopt_units[] = {
	{"auto_ack", NETOPT_AUTOACK, 0},	//example of boolean variable, (1 or 0)
	{"channel", NETOPT_CHANNEL, 2},		//u16_setget},
	{"hop_limit", NETOPT_HOP_LIMIT, 1},	//u8_setget}
	{"random", NETOPT_RANDOM, 4},		//doesn't seem to be in use
};

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

    //set_extra_commands(shell_commands);

    //SUPER-SICKO-DATATYPE-TEST:
    //unsigned long lol;
    //unsigned short* kek = (unsigned short*)&lol;
    //*kek = 0x3;
    //printf("value of lol:\t0x%x\nvalue of kek:\t0x%x\n", lol, *kek);
    //CONCLUSION: I CANNOT ASSIGN A LESSER-SIZED INTEGER TO A GREATER-SIZED VARIABLE (bytes aren't aligned properly)

	//initiate CoAP server
    start_coap_server(custom_netopt_units, 4);
    thread_yield(); //why did I put this here?

        ///*DEBUG, print ipv6 address:
        ipv6_addr_t ipv6_addrs[2];
            netif_t *iface = netif_get_by_name("7");
            if (!iface) {
                puts("error: invalid interface given\n");
                return 1;
            }
int res = netif_get_opt(iface, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
                      sizeof(ipv6_addrs));
if (res >= 0) {
    //printf("debug-debugging-message: \tres = %i \tsize-of-ipv6 = %i \tloops = %i\n", res, sizeof(ipv6_addr_t), res / sizeof(ipv6_addr_t));    //DEBUG
    uint8_t ipv6_addrs_flags[2];
//printf("debugdebug-debugging-message: A\n");     //DEBUG

    memset(ipv6_addrs_flags, 0, sizeof(ipv6_addrs_flags));
//printf("debugdebug-debugging-message: B\n");     //DEBUG
    // assume it to succeed (otherwise array will stay 0)
    netif_get_opt(iface, NETOPT_IPV6_ADDR_FLAGS, 0, ipv6_addrs_flags,
                    sizeof(ipv6_addrs_flags));
    //printf("debugdebug-debugging-message: C\n");     //DEBUG
    for (unsigned i = 0; i < (res / sizeof(ipv6_addr_t)); i++) {
            char addr_str[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")];
                //printf("debugdebug-debugging-message: D\n");     //DEBUG

            printf("inet6 addr: ");
            ipv6_addr_to_str(addr_str, &ipv6_addrs[i], sizeof(addr_str));
                //printf("debugdebug-debugging-message: E\n");     //DEBUG
            printf("%s  scope: ", addr_str);
            if (ipv6_addr_is_link_local(&ipv6_addrs[i])) {
                printf("link");
            }
            else if (ipv6_addr_is_site_local(&ipv6_addrs[i])) {
                printf("site");
            }
            else if (ipv6_addr_is_global(&ipv6_addrs[i])) {
                printf("global");
            }
            else {
                printf("unknown");
            }
                //printf("debugdebug-debugging-message: F\n");     //DEBUG
            printf("\n");   //flush
    }
} else printf("debug FAIL\n");

//DEBUG*/
    return 0;
}
