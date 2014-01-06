CC?=gcc
LFLAGS=-lX11 -lXinerama
CFLAGS=-Wall -Wpedantic -std=c99
INSTALLDIR=/usr/local
DEPS=dmenu

build:
	@[ -f config.h ] || cp config.def.h config.h
	${CC} ${CFLAGS} karuiwm.c ${LFLAGS} -o karuiwm

scan:
	scan-build make debug

install:
	@for i in ${DEPS}; do \
		printf "checking whether %s is installed... " $$i; \
		which $$i >/dev/null 2>&1 && echo 'OK' || { echo 'FAIL'; exit 1; }; \
	done
	install karuiwm ${INSTALLDIR}/bin/karuiwm

uninstall:
	rm ${INSTALLDIR}/bin/karuiwm

# for ayekat, for developing
devbuild: CFLAGS+=-g -DDEBUG
devbuild: build
devrun:
	PATH="${PWD}:${PATH}" xinit ./karuiwm -- :1
dev: devbuild devrun

