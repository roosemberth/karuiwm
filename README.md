karuiwm
=======

karuiwm is an attempt to write a lightweight, dynamically tiling window manager
for X. It is inspired by dwm's simplicity, replacing its tag system by desktops
and workspaces.

It is a complete rewrite from an earlier version, which can be tested at commit
[#31a8063](https://github.com/ayekat/karuiwm/tree/31a8063da9f3960e268514952b92d9c9ce5719ee).

**karuiwm is currently under heavy development and not usable.** See the
[issues](https://github.com/ayekat/karuiwm/issues) or the list below for what's
there and what's to come.

* **!** Tiling Layouts (not modular yet)
* **✓** Desktops
* **!** Workspaces (not controllable yet, needs API)
* **✗** Scratchpads
* **✗** Session Restore
* **✗** API + Unix Socket
* **✗** Complete EWMH Compliance
* **✗** Configuration through Xresources


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


run
---

	make xephyr

launches karuiwm inside Xephyr (X server in a window). Alternatively, `make run`
will launch karuiwm normally, however it is discouraged to run from within an
existing X session, as it may cause an X hickup.


bugs
----

Although crafted with utmost care, some bugs may have slipped my notice. Please
feel free to file bug reports. I'm a hobby developer. I won't bite.
