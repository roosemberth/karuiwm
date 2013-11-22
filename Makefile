CC?=gcc
LFLAGS=-lX11
CFLAGS=-Wall -Wpedantic -std=c99

build:
	@[ -f config.h ] || cp config.def.h config.h
	${CC} ${CFLAGS} stwm.c ${LFLAGS} -o stwm

dev: CFLAGS += -g -DDEBUG
dev: build

scan:
	scan-build make dev

all: dev xephyr

run:
	xinit ./xinitrc -- :1

xephyr:
	xinit ./xinitrc -- $(shell which Xephyr) :1

