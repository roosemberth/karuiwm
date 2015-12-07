# Compiler:
CC ?= gcc

# Application name:
APPNAME = karuiwm

# Files:
SRCDIR = src
BUILDDIR = build
MODULEDIR = modules
SOURCES = $(shell find ${SRCDIR} -name '*.c')
OBJECTS = $(SOURCES:${SRCDIR}/%.c=${BUILDDIR}/%.o)
DEPENDS = $(OBJECTS:%.o=%.d)
XINITRC = xinitrc

# Installation:
INSTALLDIR ?= /usr/local
BINDIR ?= ${INSTALLDIR}/bin
INCLUDEDIR ?= ${INSTALLDIR}/include/${APPNAME}

# ==============================================================================

# Compilation flags:
_CFLAGS = -std=c99
_CFLAGS += -W -Wall -Wextra -pedantic
_CFLAGS += -Wcast-align -Wcast-qual -Wconversion -Wwrite-strings -Wfloat-equal
_CFLAGS += -Wlogical-op -Wpointer-arith -Wformat=2
_CFLAGS += -Winit-self -Wuninitialized -Wmaybe-uninitialized -Wshadow
_CFLAGS += -Wstrict-prototypes -Wmissing-declarations -Wmissing-prototypes
#_CFLAGS += -Wpadded
_CFLAGS += $(shell pkg-config --cflags x11)
_CFLAGS += -I${HOME}/.local/include

_CFLAGS_ASAN = -fsanitize=address -fno-omit-frame-pointer
_CFLAGS_DEBUG = -Werror -g -O1 -DMODE_DEBUG
_CFLAGS_RELEASE = -O2
_CFLAGS_XINERAMA = $(shell pkg-config --cflags xinerama) -DXINERAMA

# Libraries:
_LIBS = $(shell pkg-config --libs x11) -ldl
_LIBS_ASAN =
_LIBS_DEBUG =
_LIBS_RELEASE =
_LIBS_XINERAMA = $(shell pkg-config --libs xinerama)

# Linker flags:
_LDFLAGS = -Wl,--export-dynamic
_LDFLAGS_ASAN = -fsanitize=address
_LDFLAGS_DEBUG =
_LDFLAGS_RELEASE =
_LDFLAGS_XINERAMA =

# ==============================================================================

# Configuration:
-include config.mk

# ==============================================================================

# Default: Release + Xinerama
.PHONY: all
all: release_xinerama

# Release:
.PHONY: release
release: _CFLAGS += ${_CFLAGS_RELEASE}
release: _LIBS += ${_LIBS_RELEASE}
release: _LDFLAGS += ${_LDFLAGS_RELEASE}
release: $(APPNAME)

# Release + Xinerama:
.PHONY: release_xinerama
release_xinerama: _CFLAGS += ${_CFLAGS_XINERAMA}
release_xinerama: _LIBS += ${_LIBS_XINERAMA}
release_xinerama: _LDFLAGS += ${_LDFLAGS_XINERAMA}
release_xinerama: release

# Debug:
.PHONY: debug
debug: _CFLAGS += ${_CFLAGS_DEBUG}
debug: _LIBS += ${_LIBS_DEBUG}
debug: _LDFLAGS += ${_LDFLAGS_DEBUG}
debug: $(APPNAME)

# Debug + Xinerama:
.PHONY: debug_xinerama
debug_xinerama: _CFLAGS += ${_CFLAGS_XINERAMA}
debug_xinerama: _LIBS += ${_LIBS_XINERAMA}
debug_xinerama: _LDFLAGS += ${_LDFLAGS_XINERAMA}
debug_xinerama: debug

# Address Sanitizer:
.PHONY: asan
asan: _CFLAGS += ${_CFLAGS_ASAN}
asan: _LIBS += ${_LIBS_ASAN}
asan: _LDFLAGS += ${_LDFLAGS_ASAN}
asan: debug

# Address Sanitizer + Xinerama:
.PHONY: asan_xinerama
asan_xinerama: _CFLAGS += ${_CFLAGS_XINERAMA}
asan_xinerama: _LIBS += ${_LIBS_XINERAMA}
asan_xinerama: _LDFLAGS += ${_LDFLAGS_XINERAMA}
asan_xinerama: asan

# ==============================================================================

# Build-dependencies:
-include ${DEPENDS}

# Compile:
$(BUILDDIR)/%.o: _CFLAGS += ${CFLAGS}
$(BUILDDIR)/%.o: ${SRCDIR}/%.c
	@printf "compiling \033[1m%s\033[0m ...\n" $@
	mkdir -p "$(shell dirname $@)"
	$(CC) ${_CFLAGS} -c $< -o $@
	$(CC) ${_CFLAGS} -MM -MT $@ $< > ${BUILDDIR}/$*.d

# Link:
$(APPNAME): _LIBS += ${LIBS}
$(APPNAME): _LDFLAGS += ${LDFLAGS}
$(APPNAME): $(OBJECTS)
	@printf "linking \033[1m%s\033[0m ...\n" $@
	$(CC) ${_LDFLAGS} ${OBJECTS} ${_LIBS} -o $@

# ==============================================================================

.PHONY: modules
modules:
	for m in ${MODULEDIR}/*/; do cd "$$m"; make; cd -; done

modules-install:
	for m in ${MODULEDIR}/*/; do cd "$$m"; make install; cd -; done

# ==============================================================================

# Generic actions:
.PHONY: clean mrproper install uninstall
clean:
	rm -rf ${BUILDDIR}
mrproper: clean
	rm -f ${APPNAME}
install:
	install ${APPNAME} ${BINDIR}/${APPNAME}
install-headers:
	mkdir -p ${INCLUDEDIR}
	install -m 644 ${SRCDIR}/*.h ${INCLUDEDIR}/
uninstall:
	rm -f ${BINDIR}/${APPNAME}
	rm -rf ${INCLUDEDIR}

# Ctags:
ctags:
	rm -f tags
	find ${SRCDIR} -name '*.[ch]' | ctags --append -L -

# ==============================================================================

# karuiwm-specific actions:
.PHONY: run xephyr valphyr
run:
	startx ${XINITRC} -- :1
xephyr:
	xinit ${XINITRC} -- $(shell which Xephyr) :1
valphyr:
	VALGRIND=1 xinit ${XINITRC} -- $(shell which Xephyr) :1
