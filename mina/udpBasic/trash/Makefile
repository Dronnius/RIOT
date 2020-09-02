#udpBasic/Makefile
APPLICATION = udpBasic

BOARD ?= avr-rss2
PORT ?= /dev/ttyUSB0

RIOTBASE ?= $(CURDIR)/../..

####
UPLINK ?= ether
# IEEE 802.15.4 configuration
DEFAULT_CHANNEL ?= 25
DEFAULT_PAN_ID ?= 0xFEED
# RPL configuration
RPL_NETIF ?= \"6\"
RPL_DODAG_ADDR ?= \"2001:db8::1/64\"
RPL_INSTANCE_ID ?= 11

#---
USEMODULE += gnrc_dhcpv6_client_6lbr
####

#comment out for release
CFLAGS += -DDEVHELP

#supress compiler invocation lines
QUIET ?= 1

#Included modules:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gnrc_icmpv6_error
USEMODULE += gnrc_sock_udp

########
USEMODULE += gnrc_rpl
USEMODULE += gnrc_rpl_srh
USEMODULE += netstats_rpl
########

include $(RIOTBASE)/Makefile.include

###
CFLAGS += -DRPL_STATIC_NETIF=$(RPL_NETIF) -DRPL_DODAG_ADDR=$(RPL_DODAG_ADDR) 
CFLAGS += -DRPL_INSTANCE_ID=$(RPL_INSTANCE_ID)
###