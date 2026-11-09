PKG_NAME = nanomodbus
PKG_URL = https://github.com/debevv/nanoMODBUS
# v1.23.0
PKG_VERSION = 91d6782930ee263bc760f27b0cbc5b82773c5f0d
PKG_LICENSE = MIT

include $(RIOTBASE)/pkg/pkg.mk

# only the file record functions access registers through uint16_t pointers
# into the message buffer, see the package documentation
CFLAGS += -Wno-cast-align
# loop counters are int, compared to uint16_t quantities (16 bit int on AVR)
CFLAGS += -Wno-sign-compare

all:
	$(QQ)"$(MAKE)" -C $(PKG_SOURCE_DIR) -f $(RIOTBASE)/Makefile.base \
	  MODULE=$(PKG_NAME) SRC=nanomodbus.c
