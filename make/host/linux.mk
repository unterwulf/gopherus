CFLAGS += -std=gnu89 -Wall -Wextra
exeext :=

objs += net-lin.o ui-sdl.o

libs += -lSDL

distfiles += gopherus.svg
