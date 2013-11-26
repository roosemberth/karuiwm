CC?=gcc
LFLAGS=-lX11 -lXinerama
CFLAGS=-Wall -Wpedantic -std=c99

build:
	@[ -f config.h ] || cp config.def.h config.h
	${CC} ${CFLAGS} stwm.c ${LFLAGS} -o stwm

dev: CFLAGS += -g -DDEBUG
dev: build

scan:
	scan-build make dev

run:
	xinit ./xinitrc -- :1

xephyr:
	xinit ./xinitrc -- $(shell which Xephyr) :1

all: dev xephyr

