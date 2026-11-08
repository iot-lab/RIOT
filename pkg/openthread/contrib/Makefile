MODULE := openthread_contrib

# spinel over HDLC, only in the Radio Co-Processor
ifeq (,$(filter openthread-rcp,$(USEMODULE)))
  SRC := $(filter-out rcp.c,$(wildcard *.c))
endif

include $(RIOTBASE)/Makefile.base
