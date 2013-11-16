LFLAGS=-lX11
CFLAGS=-Wall -Wpedantic -std=c99

all: stwm.c config.h
	gcc ${LFLAGS} stwm.c ${CFLAGS} -o stwm

run:
	xinit ./xinitrc -- :1

