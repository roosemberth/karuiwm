# Paths, file names:
PROJECT_ROOT = $(shell pwd)
ifeq (${PROJECT_ROOT}, ${PWD})
	BUILDDIR = ${PROJECT_ROOT}/build
else
	BUILDDIR = ${PWD}
endif
SRCDIR = ${PROJECT_ROOT}
SOURCES = $(shell find ${SRCDIR} -name '*.c')
OBJECTS = $(SOURCES:${SRCDIR}/%.c=${BUILDDIR}/%.o)
DEPENDS = $(OBJECTS:%.o=%.d)

# Module-specific configuration:
-include config.mk

# Global configuration:
-include ../config.mk

# Compiler:
CC ?= gcc

# Application name:
APPNAME ?= karuiwm

# Module name:
MODULENAME ?= $(shell basename "${PROJECT_ROOT}").so

# Installation prefix:
INSTALLDIR ?= /usr/local
INCLUDEDIR ?= ${INSTALLDIR}/include
MODULEDIR ?= ${INSTALLDIR}/share/${APPNAME}/modules

# Compilation flags:
_CFLAGS += -std=c99
_CFLAGS += -W -Wall -Wextra -pedantic
_CFLAGS += -Wcast-align -Wcast-qual -Wconversion -Wwrite-strings -Wfloat-equal
_CFLAGS += -Wlogical-op -Wpointer-arith -Wformat=2
_CFLAGS += -Winit-self -Wuninitialized -Wmaybe-uninitialized -Wshadow
_CFLAGS += -Wstrict-prototypes -Wmissing-declarations -Wmissing-prototypes
_CFLAGS += -I${INCLUDEDIR}
#_CFLAGS += -Wpadded
_CFLAGS += -fPIC

_CFLAGS_ASAN += -fsanitize=address -fno-omit-frame-pointer
_CFLAGS_DEBUG += -Werror -g -O1
_CFLAGS_RELEASE += -O2

# Libraries:
_LIBS +=
_LIBS_ASAN +=
_LIBS_DEBUG +=
_LIBS_RELEASE +=

# Linker flags:
_LDFLAGS += -shared
_LDFLAGS_ASAN += -fsanitize=address
_LDFLAGS_DEBUG +=
_LDFLAGS_RELEASE +=

# Default: Release
all: release

# Release:
release: _CFLAGS += ${_CFLAGS_RELEASE}
release: _LIBS += ${_LIBS_RELEASE}
release: _LDFLAGS += ${_LDFLAGS_RELEASE}
release: build

# Debug:
debug: _CFLAGS += ${_CFLAGS_DEBUG}
debug: _LIBS += ${_LIBS_DEBUG}
debug: _LDFLAGS += ${_LDFLAGS_DEBUG}
debug: build

# Address Sanitizer:
asan: _CFLAGS += ${_CFLAGS_ASAN}
asan: _LIBS += ${_LIBS_ASAN}
asan: _LDFLAGS += ${_LDFLAGS_ASAN}
asan: debug

# Basic actions:
build: $(MODULENAME)
clean:
	rm -rf ${BUILDDIR}
mrproper: clean
	rm -f ${MODULENAME}
install:
	mkdir -p ${MODULEDIR}
	install ${MODULENAME} ${MODULEDIR}/${MODULENAME}
uninstall:
	rm -f ${MODULEDIR}/${MODULENAME}

# Build dependencies:
-include ${DEPENDS}

# Compile:
$(BUILDDIR)/%.o: _CFLAGS += ${CFLAGS}
$(BUILDDIR)/%.o: ${SRCDIR}/%.c
	@printf "compiling \033[1m%s\033[0m ...\n" $@
	mkdir -p "$(shell dirname $@)"
	$(CC) ${_CFLAGS} -c $< -o $@
	$(CC) ${_CFLAGS} -MM -MT $@ $< > ${BUILDDIR}/$*.d

# Link:
$(MODULENAME): _LIBS += ${LIBS}
$(MODULENAME): _LDFLAGS += ${LDFLAGS}
$(MODULENAME): $(OBJECTS)
	@printf "linking \033[1m%s\033[0m ...\n" $@
	$(CC) ${_LDFLAGS} ${OBJECTS} ${_LIBS} -o $@

# Phony targets:
.PHONY: all
.PHONY: release debug asan
.PHONY: build clean mrproper install uninstall
