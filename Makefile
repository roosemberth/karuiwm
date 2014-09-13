# Makefile for karuiwm

# Compiler:
CC ?= gcc

# Installation prefix:
PREFIX = /usr/local

# Application name:
APPNAME = karuiwm

# Flags:
CFLAGS  = -W -Wall -Wextra -pedantic -g
CFLAGS += -Wcast-align -Wcast-qual -Wconversion -Wwrite-strings -Wfloat-equal
CFLAGS += -Wlogical-op -Wpointer-arith -Wformat=2
CFLAGS += -Winit-self -Wuninitialized -Wmaybe-uninitialized
CFLAGS += -Wstrict-prototypes -Wmissing-declarations -Wmissing-prototypes
CFLAGS += -Wshadow #-Wpadded
CFLAGS += -std=c99

LIBS  = $(shell pkg-config --libs xinerama x11)

# Default: Build application
all: $(APPNAME)

# Handy actions:
clean:
mrproper: clean
	rm -f ${APPNAME}
install:
	install ${APPNAME} ${PREFIX}/bin/${APPNAME}
uninstall:
	rm -f ${PREFIX}/bin/${APPNAME}
xephyr:
	xinit ./karuiwm -- $(shell which Xephyr) :1

# Compile & Link:
$(APPNAME):
	@[ -f config.h ] || cp config.def.h config.h
	$(CC) ${CFLAGS} karuiwm.c ${LIBS} -o $@

# Phony targets:
.PHONY: all
.PHONY: clean
.PHONY: mrproper
.PHONY: install
.PHONY: uninstall
.PHONY: xephyr
