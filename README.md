karuiwm
=======

karuiwm is an attempt to write a lightweight, dynamically tiling window manager
for X.

**karuiwm is currently under heavy development and not usable.** It is a
complete rewrite of the currently used, but rather buggy [legacy
version](https://github.com/ayekat/karuiwm/tree/legacy).


build
-----

	make

will build karuiwm with the default settings. The targets `release_xinerama`
(default), `release`, `debug_xinerama`, `debug`, `asan_xinerama` and `asan` will
build karuiwm for release mode, debug mode, and debug mode with address
sanitizer, each with and without Xinerama support.


install
-------

	make install

will install karuiwm to /usr/local. Pass `INSTALLDIR=...` to install karuiwm to
a different location.


for developers
--------------

For testing purposes, karuiwm can also be launched via Xephyr (X server in a
window):

	make xephyr

To run it with valgrind in Xephyr, you can use the `valphyr` target:

	make valphyr

Alternatively, `make run` will launch karuiwm normally, however it is
discouraged to run from within an existing X session, as it may cause an X
hickup.

See the [doc](doc) folder for the documentation.


bugs
----

Although karuiwm has been crafted with utmost care and love, some bugs may have
slipped my notice. Please feel free to file bug reports.

I won't bite.
