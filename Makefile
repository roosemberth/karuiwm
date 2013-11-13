CC=gcc
LDLIBS=-lX11
LDFLAGS=
CFLAGS=-Wall -Wpedantic -std=c99
SOURCES=stwm.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=stwm

build: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDLIBS) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $? -o $@

all: clean build xephyr

clean:
	rm -f $(OBJECTS)

run:
	xinit ./xinitrc -- :1

xephyr:
	xinit ./xinitrc -- $(shell which Xephyr) :1

