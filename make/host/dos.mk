CFLAGS += -D__DJGPP__
exeext := .exe

ifeq ($(WATT_ROOT),)
$(error WATT_ROOT is not set)
endif

CFLAGS += -I $(WATT_ROOT)/inc
libs += $(WATT_ROOT)/lib/libwatt.a

objs += ui.o net.o

distfiles += gopherus.ico
