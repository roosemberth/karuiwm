CC?=gcc
LFLAGS=-lX11 -lXinerama
CFLAGS=-Wall -Wpedantic -std=c99
INSTALLDIR=/usr/local

build:
	@[ -f config.h ] || cp config.def.h config.h
	${CC} ${CFLAGS} stwm.c ${LFLAGS} -o stwm

debug: CFLAGS += -g -DDEBUG
debug: build

scan:
	scan-build make debug

install:
	install stwm ${INSTALLDIR}/bin/stwm

uninstall:
	rm ${INSTALLDIR}/bin/stwm

dev: debug
	xinit ./stwm -- $(shell which Xephyr) :1

