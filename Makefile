# Compiler:
CC ?= gcc

# Library/configuration prefix:
PREFIX ?= /usr/local

# Installation prefix:
INSTALLDIR ?= /usr/local

# Application name:
APPNAME = karuiwm

# Compilation flags:
KWM_CFLAGS = -std=c99
KWM_CFLAGS += -W -Wall -Wextra -pedantic
KWM_CFLAGS += -Wcast-align -Wcast-qual -Wconversion -Wwrite-strings -Wfloat-equal
KWM_CFLAGS += -Wlogical-op -Wpointer-arith -Wformat=2
KWM_CFLAGS += -Winit-self -Wuninitialized -Wmaybe-uninitialized -Wshadow
KWM_CFLAGS += -Wstrict-prototypes -Wmissing-declarations -Wmissing-prototypes
#KWM_CFLAGS += -Wpadded
KWM_CFLAGS += $(shell pkg-config --cflags x11)

KWM_CFLAGS_ASAN = -fsanitize=address -fno-omit-frame-pointer
KWM_CFLAGS_DEBUG = -Werror -g -O1 -DMODE_DEBUG
KWM_CFLAGS_RELEASE = -O2
KWM_CFLAGS_XINERAMA = $(shell pkg-config --cflags xinerama) -DXINERAMA

# Libraries:
KWM_LIBS = $(shell pkg-config --libs x11)
KWM_LIBS_ASAN =
KWM_LIBS_DEBUG =
KWM_LIBS_RELEASE =
KWM_LIBS_XINERAMA = $(shell pkg-config --libs xinerama)

# Linker flags:
KWM_LDFLAGS =
KWM_LDFLAGS_ASAN = -fsanitize=address
KWM_LDFLAGS_DEBUG =
KWM_LDFLAGS_RELEASE =
KWM_LDFLAGS_XINERAMA =

# File names:
SRCDIR = src
BUILDDIR = build
SOURCES = $(wildcard ${SRCDIR}/*.c ${SRCDIR}/layouts/*.c)
OBJECTS = $(SOURCES:${SRCDIR}/%.c=${BUILDDIR}/%.o)
DEPENDS = $(OBJECTS:%.o=%.d)
XINITRC = xinitrc

-include config.mk

# Default: Release + Xinerama
all: release_xinerama

# Release:
release: KWM_CFLAGS += ${KWM_CFLAGS_RELEASE}
release: KWM_LIBS += ${KWM_LIBS_RELEASE}
release: KWM_LDFLAGS += ${KWM_LDFLAGS_RELEASE}
release: build

# Release + Xinerama:
release_xinerama: KWM_CFLAGS += ${KWM_CFLAGS_XINERAMA}
release_xinerama: KWM_LIBS += ${KWM_LIBS_XINERAMA}
release_xinerama: KWM_LDFLAGS += ${KWM_LDFLAGS_XINERAMA}
release_xinerama: release

# Debug:
debug: KWM_CFLAGS += ${KWM_CFLAGS_DEBUG}
debug: KWM_LIBS += ${KWM_LIBS_DEBUG}
debug: KWM_LDFLAGS += ${KWM_LDFLAGS_DEBUG}
debug: build

# Debug + Xinerama:
debug_xinerama: KWM_CFLAGS += ${KWM_CFLAGS_XINERAMA}
debug_xinerama: KWM_LIBS += ${KWM_LIBS_XINERAMA}
debug_xinerama: KWM_LDFLAGS += ${KWM_LDFLAGS_XINERAMA}
debug_xinerama: debug

# Address Sanitizer:
asan: KWM_CFLAGS += ${KWM_CFLAGS_ASAN}
asan: KWM_LIBS += ${KWM_LIBS_ASAN}
asan: KWM_LDFLAGS += ${KWM_LDFLAGS_ASAN}
asan: debug

# Address Sanitizer + Xinerama:
asan_xinerama: KWM_CFLAGS += ${KWM_CFLAGS_XINERAMA}
asan_xinerama: KWM_LIBS += ${KWM_LIBS_XINERAMA}
asan_xinerama: KWM_LDFLAGS += ${KWM_LDFLAGS_XINERAMA}
asan_xinerama: asan

# Basic actions:
build: $(APPNAME)
clean:
	rm -rf ${BUILDDIR}
mrproper: clean
	rm -f ${APPNAME}
install:
	install ${APPNAME} ${INSTALLDIR}/bin/${APPNAME}
uninstall:
	rm -f ${INSTALLDIR}/bin/${APPNAME}

# Build dependencies:
-include ${DEPENDS}

# Compile:
$(BUILDDIR)/%.o: KWM_CFLAGS += ${CFLAGS}
$(BUILDDIR)/%.o: ${SRCDIR}/%.c
	@printf "compiling \033[1m%s\033[0m ...\n" $@
	mkdir -p "$(shell dirname $@)"
	$(CC) ${KWM_CFLAGS} -c $< -o $@
	$(CC) ${KWM_CFLAGS} -MM -MT $@ $< > ${BUILDDIR}/$*.d

# Link:
$(APPNAME): KWM_LIBS += ${LIBS}
$(APPNAME): KWM_LDFLAGS += ${LDFLAGS}
$(APPNAME): $(OBJECTS)
	@printf "linking \033[1m%s\033[0m ...\n" $@
	$(CC) ${KWM_LDFLAGS} ${OBJECTS} ${KWM_LIBS} -o $@

# karuiwm-specific actions:
run:
	startx ${XINITRC} -- :1
xephyr:
	xinit ${XINITRC} -- $(shell which Xephyr) :1
valphyr:
	VALGRIND=1 xinit ${XINITRC} -- $(shell which Xephyr) :1

# Phony targets:
.PHONY: all
.PHONY: release release_xinerama debug debug_xinerama asan asan_xinerama
.PHONY: build clean mrproper install uninstall
.PHONY: run xephyr valphyr
