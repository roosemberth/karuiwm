stwm
====

An attempt to build a minimalist tiling window manager with Xlib; inspired by
and based on [dwm](http://dwm.suckless.org/).


build
-----

This will create the binary <code>stwm</code>:

	make


run
---

This will launch a new X session on display **:1** (modify the Makefile to
change) with stwm as the window manager:

	make run

Create a <code>xinitrc.custom</code> file to customise startup actions, like
launching a systray or setting the keyboard layout.


configure
---------

<code>config.h</code> holds the configuration of stwm and gets included by
<code>stwm.c</code> upon compilation.

<code>config.def.h</code> holds the default configuration of stwm. When running
<code>make</code>, this file will be used to generate a <code>config.h</code> if
that one doesn't exist yet. This protects your configuration from being
overwritten by the default upon checking out the git repository.

The default configuration specifies the following keys:

**Applications**

* <code>Mod</code>+<code>n</code>
  launch xterm
* <code>Mod</code>+<code>p</code>
  launch dmenu

**Windows**

* <code>Mod</code>+<code>l</code>/<code>h</code>
  increase/decrease master area size
* <code>Mod</code>+<code>j</code>/<code>k</code>
  set focus to next/previous client

**Layout**

* <code>Mod</code>+<code>j</code>/<code>k</code>
  swap client with next/previous client in the layout
* <code>Mod</code>+<code>,</code>/<code>.</code>
  increase/decrease number of clients in the master area
* <code>Mod</code>+<code>Return</code>
  move selected client to master area

**Workspaces**

* <code>Mod</code>+<code>h</code>/<code>j</code>/<code>k</code>/<code>l</code>
  switch to left/below/above/right workspace

**Session**

* <code>Mod</code>+<code>q</code>
  restart stwm
* <code>Mod</code>+<code>Shift</code>+<code>q</code>
  quit stwm

The <code>Mod</code> key is set to Mod1 (<code>Alt</code>).


workspaces
----------

Workspaces in stwm are arranged in a two-dimensional grid, and they are created
and destroyed dynamically. A workspace may either be considered *persistent* (if
there is at least one client placed in it) or *temporary* (if the workspace is
empty).

If a temporary workspace is left, it is destroyed, whereas a persistent
workspace remains (as the name suggests).

Once a client is placed inside a temporary workspace, it is automatically turned
into a persistent workspace; the same way a persistent workspace is
automatically turned into a temporary one once its last client is removed.

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
	|   | X    ==createclient==>  |   | X |    OK, make workspace persistent
	+---+   +                     +---+---+

Remember that this works in two dimensions.

