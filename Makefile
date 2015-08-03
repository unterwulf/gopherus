#
# Copyright (c) 2015 Vitaly Sinilin
#
# Requires GNU make 3.80 or newer
#

BUILD ?= linux
HOST ?= linux

srcdir := .
objdir := build/$(HOST)
pkgdir := pkg/$(HOST)
config := make/local/$(BUILD)/$(HOST).mk


all:

-include $(config)
include make/common.mk
include make/host/$(HOST).mk

progname := gopherus$(exeext)
prog := $(objdir)/$(progname)
objs := $(addprefix $(objdir)/,$(objs))
pkgfiles := $(addprefix $(pkgdir)/,$(progname) $(distfiles))
depfiles := $(objs:.o=.d)

include make/rules.mk
include make/build/$(BUILD).mk
-include $(depfiles)

all:
pkg:
clean:

.PHONY: all pkg clean
