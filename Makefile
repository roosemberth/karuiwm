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

all: build run

run:
	xinit ./xinitrc -- :1

