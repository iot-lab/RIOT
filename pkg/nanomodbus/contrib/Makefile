MODULE = nanomodbus_contrib

SRC :=

ifneq (,$(filter nanomodbus_tcp,$(USEMODULE)))
  SRC += nanomodbus_tcp.c
endif

ifneq (,$(filter nanomodbus_uart,$(USEMODULE)))
  SRC += nanomodbus_uart.c
endif

ifneq (,$(filter nanomodbus_udp,$(USEMODULE)))
  SRC += nanomodbus_udp.c
endif

include $(RIOTBASE)/Makefile.base
