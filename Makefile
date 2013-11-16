LFLAGS=-lX11
CFLAGS=-Wall -Wpedantic -std=c99

all:
	@[ -f config.h ] || cp config.def.h config.h
	gcc ${LFLAGS} stwm.c ${CFLAGS} -o stwm

run:
	xinit ./xinitrc -- :1

