include ../Makefile.pkg_common

USEPKG += nanomodbus
USEMODULE += nanomodbus_udp

# IPv6 and UDP, the client and the server talk over the loopback (::1)
USEMODULE += gnrc_ipv6
USEMODULE += gnrc_sock_udp

include $(RIOTBASE)/Makefile.include
