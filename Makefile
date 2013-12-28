CC?=gcc
LFLAGS=-lX11 -lXinerama
CFLAGS=-Wall -Wpedantic -std=c99
INSTALLDIR=/usr/local

build:
	@[ -f config.h ] || cp config.def.h config.h
	${CC} ${CFLAGS} stwm.c ${LFLAGS} -o stwm

scan:
	scan-build make debug

install:
	install stwm ${INSTALLDIR}/bin/stwm

uninstall:
	rm ${INSTALLDIR}/bin/stwm

# for ayekat, for developing
devbuild: CFLAGS += -g -DDEBUG
devbuild: build
devrun:
	PATH="${PWD}:${PATH}" xinit ./stwm -- :1
dev: devbuild devrun

