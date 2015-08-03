CFLAGS += -std=gnu89 -Wall -Wextra -mwindows
exeext := .exe

objs += \
	net-win.o \
	ui-sdl.o \
	gopherus.res

libs += -lSDL -lws2_32

distfiles += SDL.dll
