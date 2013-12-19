CC?=gcc
LFLAGS=-lX11 -lXinerama
CFLAGS=-Wall -Wpedantic -std=c99
INSTALLDIR=/usr/local/bin

build:
	@[ -f config.h ] || cp config.def.h config.h
	${CC} ${CFLAGS} stwm.c ${LFLAGS} -o stwm

dev: CFLAGS += -g -DDEBUG
dev: build

scan:
	scan-build make dev

install:
	install stwm ${INSTALLDIR}/stwm

uninstall:
	rm ${INSTALLDIR}/stwm

