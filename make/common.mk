CC := $(CROSS_COMPILE)gcc
WINDRES := $(CROSS_COMPILE)windres
UPX := upx
CFLAGS += -O3 -pedantic

objs := \
	common.o \
	dnscache.o \
	embdpage.o \
	gopherus.o \
	history.o \
	int2str.o \
	menuview.o \
	parseurl.o \
	textview.o \
	wordwrap.o

libs :=

distfiles := \
	gopherus.txt \
	license.txt \
	history.txt
