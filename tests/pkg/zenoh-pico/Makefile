include ../Makefile.pkg_common

USEPKG += zenoh-pico

USEMODULE += shell
USEMODULE += shell_cmds_default
USEMODULE += ps

# IPv6 and TCP over the network interface of the board (tap on native)
USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gnrc_sock_tcp
USEMODULE += gnrc_tcp

# z_open() runs in the shell, on the stack of the main thread: 3 KiB more
# than the default (4 KiB on Cortex-M, more on native)
CFLAGS += -DTHREAD_STACKSIZE_MAIN=\(THREAD_STACKSIZE_DEFAULT+3072\)

include $(RIOTBASE)/Makefile.include
