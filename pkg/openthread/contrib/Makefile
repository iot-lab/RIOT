MODULE := openthread_contrib

SRC := $(wildcard *.c)
# spinel over HDLC, only in the Radio Co-Processor
ifeq (,$(filter openthread-rcp,$(USEMODULE)))
  SRC := $(filter-out rcp.c,$(SRC))
endif
# border router over ethos
ifeq (,$(filter openthread_border_router,$(USEMODULE)))
  SRC := $(filter-out border_router.c,$(SRC))
endif

include $(RIOTBASE)/Makefile.base
