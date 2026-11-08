BOARD ?= nrf52840dk

include ../Makefile.pkg_common

USEPKG += openthread
USEMODULE += openthread-rcp

# the radio of the board, a ZEP socket on native
ifneq (,$(filter native native32 native64,$(BOARD)))
  USEMODULE += socket_zep
else
  USEMODULE += netdev_default
endif

# the UART of stdio carries the spinel link
USEMODULE += stdio_null

# baudrate of the spinel link, e.g. 500000 for the A8-M3 of IoT-LAB
OPENTHREAD_RCP_UART_BAUDRATE ?= 115200
CFLAGS += -DOPENTHREAD_RCP_UART_BAUDRATE=$(OPENTHREAD_RCP_UART_BAUDRATE)

include $(RIOTBASE)/Makefile.include
