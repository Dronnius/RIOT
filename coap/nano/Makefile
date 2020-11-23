# name of your application
APPLICATION = border_router

# If no BOARD is found in the environment, use this default:
BOARD ?= avr-rss2
PORT ?= /dev/ttyUSB0

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Default to using ether for providing the uplink when not on native
UPLINK ?= ether

# IEEE 802.15.4 configuration
DEFAULT_CHANNEL ?= 25
DEFAULT_PAN_ID ?= 0xFEED
# RPL configuration
RPL_NETIF ?= \"6\"
RPL_DODAG_ADDR ?= \"2001:db8::1/64\"
RPL_INSTANCE_ID ?= 11

# Set the SSID and password of your WiFi network here
WIFI_SSID ?= "Your_WiFi_name"
WIFI_PASS ?= "Your_secure_password"

# Check if the selected Uplink is valid
ifeq (,$(filter ether wifi,$(UPLINK)))
  $(error Supported uplinks are `ether` and `wifi`)
endif

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif

#Inlcude IPv6 stuff
#USEMODULE += gnrc_ipv6_default

#Include UDP stuff
#USEMODULE += gnrc_sock_udp

#Include CoAP stuff
USEMODULE += nanocoap_sock
# include sha256 (used by example blockwise handler)
# USEMODULE += hashes

# Specify the mandatory networking modules for 6LoWPAN border router
USEMODULE += gnrc_sixlowpan_border_router_default
# include DHCPv6 client for 6LoWPAN border router
USEMODULE += gnrc_dhcpv6_client_6lbr

# Activate ICMPv6 error messages
USEMODULE += gnrc_icmpv6_error
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo
# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6
#USEMODULE += gnrc_pktdump

# Optionally include RPL as a routing protocol. When includede gnrc_uhcpc will
# configure the node as a RPL DODAG root when receiving a prefix.
USEMODULE += gnrc_rpl
USEMODULE += gnrc_rpl_srh
USEMODULE += netstats_rpl

# Optionally include DNS support. This includes resolution of names at an
# upstream DNS server and the handling of RDNSS options in Router Advertisements
# to auto-configure that upstream DNS server.
#USEMODULE += sock_dns

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# MODULE DEPENDENT CONFIGURATION IMPORT
# =====================================
# use ethos (ethernet over serial) or SLIP (serial-line IP) for network
# communication and stdio over UART, but not on native, as native has a tap
# interface towards the host.
ifeq (wifi,$(UPLINK))
  # SSID and Password need to be configured
  include $(CURDIR)/Makefile.wifi.conf
endif

# As there is an 'Kconfig' we want to explicitly disable Kconfig by setting
# the variable to empty
SHOULD_RUN_KCONFIG ?=

include $(RIOTBASE)/Makefile.include

# For RPL border router
#
CFLAGS += -DRPL_STATIC_NETIF=$(RPL_NETIF) -DRPL_DODAG_ADDR=$(RPL_DODAG_ADDR) 
CFLAGS += -DRPL_INSTANCE_ID=$(RPL_INSTANCE_ID)

# Set a custom channel if needed
include $(RIOTMAKE)/default-radio-settings.inc.mk
