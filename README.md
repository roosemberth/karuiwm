karuiwm
=======

karuiwm is an attempt to write a lightweight, dynamically tiling window manager
for X.

**karuiwm is currently under heavy development and not usable.** It is a
complete rewrite of the currently used (but rather buggy) [legacy
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


configuration
-------------

The configuration happens through X resources. Here is a sample X resources
configuration snippet that can be used and modified:

``` Xresources
! Use windows key as the principal modifier ('M'):
karuiwm.border.width       : 2
karuiwm.border.colour      : #FF0000
karuiwm.border.colour_focus: #00FF00

karuiwm.modifier           : W

karuiwm.keysym.M-k         : stepclient:prev
karuiwm.keysym.M-j         : stepclient:next
karuiwm.keysym.M-S-k       : shiftclient:prev
karuiwm.keysym.M-S-j       : shiftclient:next
karuiwm.keysym.M-Return    : zoom
karuiwm.keysym.M-S-c       : killclient

karuiwm.keysym.M-C-h       : stepdesktop:left
karuiwm.keysym.M-C-j       : stepdesktop:down
karuiwm.keysym.M-C-k       : stepdesktop:up
karuiwm.keysym.M-C-l       : stepdesktop:right

karuiwm.keysym.M-q         : restart
karuiwm.keysym.M-S-q       : stop

karuiwm.keysym.M-n         : spawn:urxvt

karuiwm.button.M-1         : mousemove
karuiwm.button.M-3         : mouseresize
```

Documentation will follow.


bugs
----

Although karuiwm has been crafted with utmost care and love, some bugs may have
slipped my notice. Please feel free to file bug reports.

I won't bite.
