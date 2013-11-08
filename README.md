Simple Tiling Window Manager
============================

**stwm** is an attempt to build a minimalist tiling window manager with Xlib.

It is inspired by and based on [dwm](http://dwm.suckless.org/).


build
-----

This will create the binary <code>stwm</code>:

	make


run
---

This will launch a new X session on display **:1** with stwm as window manager
and an xterm window already opened:

	make run

If you want to open other application at startup, modify <code>xinitrc</code>.

If you want to launch stwm on another display than **:1**, modify the
<code>run</code> target in the Makefile.

If you've got Xephyr installed and want to run stwm inside Xephyr:

	make xephyr


usage
-----

* <code>Mod4</code>+<code>l</code> increase the master area
* <code>Mod4</code>+<code>h</code> decrease the master area
* <code>Mod4</code>+<code>j</code> set focus to next window
* <code>Mod4</code>+<code>k</code> set focus to previous window
* <code>Mod4</code>+<code>q</code> quit stwm

