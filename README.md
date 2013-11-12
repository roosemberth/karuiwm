stwm
====

An attempt to build a minimalist tiling window manager with Xlib; inspired by
and based on [dwm](http://dwm.suckless.org/).


configure
---------

<code>config.h</code> holds values to configure the behaviour and look of stwm.
It is included from <code>dwm.c</code> upon compilation.


build
-----

This will create the binary <code>stwm</code>:

	make


run
---

This will launch a new X session on display **:1** (modify Makefile to change)
with stwm as window manager and an xterm window already opened:

	make run

If you want to open other application at startup, modify <code>xinitrc</code>.
Note that it is currently not possible to launch an application from *within*
stwm, so once you close your last terminal, all you can do is shut down stwm.

In case you've got Xephyr installed and want to run stwm inside Xephyr:

	make xephyr


usage
-----

* <code>Mod4</code>+<code>l</code> increase the master area
* <code>Mod4</code>+<code>h</code> decrease the master area
* <code>Mod4</code>+<code>j</code> set focus to next window
* <code>Mod4</code>+<code>k</code> set focus to previous window
* <code>Mod4</code>+<code>r</code> restart stwm
* <code>Mod4</code>+<code>q</code> quit stwm

