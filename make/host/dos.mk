CFLAGS += -D__DJGPP__
exeext := .exe

ifeq ($(NO_NET),)
ifeq ($(WATT_ROOT),)
$(error WATT_ROOT is not set)
endif

CFLAGS += -I $(WATT_ROOT)/inc
libs += $(WATT_ROOT)/lib/libwatt.a
objs += net.o
else
objs += net-stub.o
endif

objs += ui.o snprintf.o

distfiles += gopherus.ico
