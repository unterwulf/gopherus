CFLAGS += -std=gnu89 -Wall -Wextra -mwindows
exeext := .exe

objs += \
	net-win.o \
	ui-sdl.o \
	gopherus.res

libs += -Wl,-Bstatic -lSDLmain -lSDL -Wl,-Bdynamic -lm -luser32 -lgdi32 -lwinmm -ldxguid -lws2_32
