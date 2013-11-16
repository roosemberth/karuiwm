stwm
====

An attempt to build a minimalist tiling window manager with Xlib; inspired by
and based on [dwm](http://dwm.suckless.org/).


configure
---------

<code>config.h</code> holds the configuration of stwm and gets included by
<code>stwm.c</code> upon compilation. It defines look and behaviour related
things and special keys.


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


usage
-----

Applications:

* <code>Mod</code>+<code>n</code> launch xterm
* <code>Mod</code>+<code>p</code> launch dmenu

Windows:

* <code>Mod</code>+<code>l</code>/<code>h</code>
  increase/decrease master area size
* <code>Mod</code>+<code>j</code>/<code>k</code>
  set focus to next/previous window

Layout:

* <code>Mod</code>+<code>j</code>/<code>k</code>
  swap client with next/previous client in the layout
* <code>Mod</code>+<code>,</code>/<code>.</code>
  increase/decrease number of clients in the master area
* <code>Mod</code>+<code>Return</code>
  move selected client to master area

Workspaces:
* <code>Mod</code>+<code>h<code>/<code>j</code>/<code>k</code>/<code>l</code>
  switch to left/below/above/right workspace

Session:
* <code>Mod</code>+<code>q</code>
  restart stwm
* <code>Mod</code>+<code>Shift</code>+<code>q</code>
  quit stwm

The <code>Mod</code> key is set to Mod1 (Alt). Note that these are just the
default settings; key combinations can be modified in <code>config.h</code>.


workspaces
----------

Workspaces in stwm are arranged in a two-dimensional grid, and they are created
and destroyed dynamically. A workspace may either be considered *persistent* (if
there is at least one window placed in it) or *temporary* (if the workspace is
empty).

If a persistent workspace is left, it gets destroyed, whereas a persistent
workspace remains (as the name suggests).

Once a window is placed inside a temporary workspace, it is automatically turned
into a persistent workspace; the same way a persistent workspace is
automatically turned into a temporary one once its last window is removed.

It is possible to move to a temporary workspace as long as there is at least one
adjacent persistent workspace, for example:

	             +---+                           +   +
	X = focus    |   | = persistent workspace          = temporary workspace
	             +---+                           +   +
	
	
	+---+                         +---+   +
	| X |      ====moveright===>  |   | X      OK, move to temporary workspace
	+---+                         +---+   +
	
	+---+   +                     +---+   +
	|   | X    ====moveright===>  |   | X      not OK, stay at current workspace
	+---+   +                     +---+   +
	
	+---+   +                     +---+---+
	|   | X    ==createwindow==>  |   | X |    OK, make workspace persistent
	+---+   +                     +---+---+

