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
CFLAGS += -Wshadow -Werror #-Wpadded
CFLAGS += -std=c99 -O2
CFLAGS += $(shell pkg-config --cflags xinerama x11)
LIBS  = $(shell pkg-config --libs xinerama x11)

# File names:
SRCDIR = src
BUILDDIR = build
SOURCES = $(wildcard ${SRCDIR}/*.c ${SRCDIR}/layouts/*.c)
OBJECTS = $(SOURCES:${SRCDIR}/%.c=${BUILDDIR}/%.o)
DEPENDS = $(OBJECTS:%.o=%.d)
XINITRC = xinitrc

-include "config.mk"

# Default: Build application
all: $(APPNAME)

# Alternative build targets:
asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: LIBS += -fsanitize=address
asan: all

# Handy actions:
clean:
	rm -rf ${BUILDDIR}
mrproper: clean
	rm -f ${APPNAME}
install:
	install ${APPNAME} ${PREFIX}/bin/${APPNAME}
uninstall:
	rm -f ${PREFIX}/bin/${APPNAME}

# Build dependencies:
-include ${DEPENDS}

# Compile & Link:
$(BUILDDIR)/%.o: ${SRCDIR}/%.c
	@if [ ! -d ${BUILDDIR} ]; then mkdir ${BUILDDIR}; fi
	if [ ! -d $(shell dirname $@) ]; then mkdir $(shell dirname $@); fi
	@printf "compiling \033[1m%s\033[0m ...\n" $@
	$(CC) ${CFLAGS} -c $< -o $@
	$(CC) ${CFLAGS} -MM -MT $@ $< > ${BUILDDIR}/$*.d
$(APPNAME): $(OBJECTS)
	@printf "linking \033[1m%s\033[0m ...\n" $@
	$(CC) ${LIBS} ${OBJECTS} -o $@

xephyr:
	xinit ${XINITRC} -- $(shell which Xephyr) :1
valphyr:
	VALGRIND=1 xinit ${XINITRC} -- $(shell which Xephyr) :1
run:
	startx ${XINITRC} > log

# Phony targets:
.PHONY: all
.PHONY: asan
.PHONY: clean
.PHONY: mrproper
.PHONY: install
.PHONY: uninstall
.PHONY: xephyr
