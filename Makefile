# Compiler:
CC ?= gcc

# Installation prefix:
PREFIX = /usr/local

# Application name:
APPNAME = karuiwm

# Flags:
KWM_CFLAGS  = -W -Wall -Wextra -pedantic -g
KWM_CFLAGS += -Wcast-align -Wcast-qual -Wconversion -Wwrite-strings -Wfloat-equal
KWM_CFLAGS += -Wlogical-op -Wpointer-arith -Wformat=2
KWM_CFLAGS += -Winit-self -Wuninitialized -Wmaybe-uninitialized
KWM_CFLAGS += -Wstrict-prototypes -Wmissing-declarations -Wmissing-prototypes
KWM_CFLAGS += -Wshadow -Werror #-Wpadded
KWM_CFLAGS += -std=c99 -O2
KWM_CFLAGS += $(shell pkg-config --cflags xinerama x11)
KWM_LIBS  = $(shell pkg-config --libs xinerama x11)
KWM_LDFLAGS =

# File names:
SRCDIR = src
BUILDDIR = build
SOURCES = $(wildcard ${SRCDIR}/*.c ${SRCDIR}/layouts/*.c)
OBJECTS = $(SOURCES:${SRCDIR}/%.c=${BUILDDIR}/%.o)
DEPENDS = $(OBJECTS:%.o=%.d)
XINITRC = xinitrc

-include config.mk

# Default: Build application
all: $(APPNAME)

# Alternative build targets:
asan: KWM_CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: KWM_LDFLAGS += -fsanitize=address
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

# Compile:
$(BUILDDIR)/%.o: KWM_CFLAGS += ${CFLAGS}
$(BUILDDIR)/%.o: ${SRCDIR}/%.c
	@if [ ! -d ${BUILDDIR} ]; then mkdir ${BUILDDIR}; fi
	if [ ! -d $(shell dirname $@) ]; then mkdir $(shell dirname $@); fi
	@printf "compiling \033[1m%s\033[0m ...\n" $@
	$(CC) ${KWM_CFLAGS} -c $< -o $@
	$(CC) ${KWM_CFLAGS} -MM -MT $@ $< > ${BUILDDIR}/$*.d

# Link:
$(APPNAME): KWM_LIBS += ${LIBS}
$(APPNAME): KWM_LDFLAGS += ${LDFLAGS}
$(APPNAME): $(OBJECTS)
	@printf "linking \033[1m%s\033[0m ...\n" $@
	$(CC) ${KWM_LDFLAGS} ${OBJECTS} ${KWM_LIBS} -o $@

xephyr:
	xinit ${XINITRC} -- $(shell which Xephyr) :1
valphyr:
	VALGRIND=1 xinit ${XINITRC} -- $(shell which Xephyr) :1

# Phony targets:
.PHONY: all
.PHONY: asan
.PHONY: clean
.PHONY: mrproper
.PHONY: install
.PHONY: uninstall
.PHONY: xephyr
