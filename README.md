stwm
====

An attempt to build a minimalist tiling window manager with Xlib; inspired by
and based on [dwm](http://dwm.suckless.org/).


configure
---------

<code>config.h</code> holds the configuration of stwm and gets included by
<code>stwm.c</code> upon compilation. It defines look and behaviour related
things and defines special keys.


build
-----

This will create the binary <code>stwm</code>:

	make


run
---

This will launch a new X session on display **:1** (modify the Makefile to
change) with stwm as window manager and an open xterm window:

	make run

If you want to open other applications at startup, modify <code>xinitrc</code>.

In case you've got Xephyr installed and want to run stwm inside Xephyr:

	make xephyr


usage
-----

Applications:
* <code>Mod</code>+<code>n</code> launch xterm
* <code>Mod</code>+<code>p</code> launch dmenu

Windows:
* <code>Mod</code>+<code>l</code> increase master area size
* <code>Mod</code>+<code>h</code> decrease master area size
* <code>Mod</code>+<code>j</code> set focus to next window
* <code>Mod</code>+<code>k</code> set focus to previous window
* <code>Mod</code>+<code>Return</code> move selected window to master area

Session:
* <code>Mod</code>+<code>q</code> restart stwm
* <code>Mod</code>+<code>Shift</code>+<code>q</code> quit stwm

The <code>Mod</code> key is set to Mod4 (*Windows* key). Note that these are
just the default settings; key combinations can be modified in
<code>config.h</code>.

