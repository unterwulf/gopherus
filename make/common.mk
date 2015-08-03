CC := $(CROSS_COMPILE)gcc
WINDRES := $(CROSS_COMPILE)windres
UPX := upx
CFLAGS += -O3 -pedantic

objs := \
	dnscache.o \
	gopherus.o \
	history.o \
	int2str.o \
	parseurl.o \
	startpg.o \
	wordwrap.o

libs :=

distfiles := \
	gopherus.txt \
	license.txt \
	history.txt
