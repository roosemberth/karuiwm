CC=gcc
LDLIBS=-lX11
LDFLAGS=
CFLAGS=-DHOST_$(shell hostname) -Wall -Wpedantic -ansi -g
SOURCES=$(shell find src -name *.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=stwm

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDLIBS) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $? -o $@

clean:
	rm -f $(OBJECTS)

run: $(EXECUTABLE)
	xinit ./xinitrc -- :1

