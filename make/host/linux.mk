CFLAGS += -std=gnu89 -Wall -Wextra
CPPFLAGS += -DHAVE_SNPRINTF
exeext :=

objs += net-lin.o ui-sdl.o

libs += -lSDL

distfiles += gopherus.svg
