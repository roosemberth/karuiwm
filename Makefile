CC=gcc
LDLIBS=-lX11
LDFLAGS=
CFLAGS=-DHOST_$(shell hostname) -Wall -Wpedantic -std=c99 -g
SOURCES=$(shell find src -name *.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=stwm

build: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDLIBS) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $? -o $@

all: build run

clean:
	rm -f $(OBJECTS)

run:
	xinit ./xinitrc -- :1

xephyr:
	xinit ./xinitrc -- $(shell which Xephyr) :1

