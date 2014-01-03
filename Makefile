CC?=gcc
LFLAGS=-lX11 -lXinerama
CFLAGS=-Wall -Wpedantic -std=c99
INSTALLDIR=/usr/local

build:
	@[ -f config.h ] || cp config.def.h config.h
	${CC} ${CFLAGS} karuiwm.c ${LFLAGS} -o karuiwm

scan:
	scan-build make debug

install:
	install karuiwm ${INSTALLDIR}/bin/karuiwm

uninstall:
	rm ${INSTALLDIR}/bin/karuiwm

# for ayekat, for developing
devbuild: CFLAGS += -g -DDEBUG
devbuild: build
devrun:
	PATH="${PWD}:${PATH}" xinit ./karuiwm -- :1
dev: devbuild devrun

