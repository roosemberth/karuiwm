stwm
====

An attempt to build a minimalist tiling window manager with Xlib; inspired by
and based on [dwm](http://dwm.suckless.org/).


build
-----

This will create the binary <code>stwm</code>:

	make

Alternatively, if you want to add debug output (all the <code>debug()</code>
calls in stwm, additional debug symbols through GCCs <code>-g</code> option),
you may run:

	make dev


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

* <code>Mod</code>+<code>j</code>/<code>k</code>
  set focus to next/previous client

**Layout**

* <code>Mod</code>+<code>l</code>/<code>h</code>
  increase/decrease master area size
* <code>Mod</code>+<code>j</code>/<code>k</code>
  swap client with next/previous client in the layout
* <code>Mod</code>+<code>,</code>/<code>.</code>
  increase/decrease number of clients in the master area
* <code>Mod</code>+<code>Return</code>
  move selected client to master area

**Workspaces**

* <code>Mod</code>+<code>Space</code>
  open workspace dialog
* <code>Mod</code>+<code>Shift</code>+<code>h</code>/<code>j</code>/<code>k</code>/<code>l</code>
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
and destroyed dynamically. A workspace may be considered either *persistent*
(it remains if left) or *temporary* (leaving it destroys it), depending on
whether there is a client on the workspace.

The change from persistent to temporary (or vice versa) happens automatically
upon removing the last client (or placing the first client).

The status bar displays the current position on the "workspace-map"; for a
graphical representation there is the *workspace dialog*; the keys are as
follows:

* <code>Mod</code>+<code>h</code>/<code>j</code>/<code>k</code>/<code>l</code>
  move target to left/below/above/right workspace (currently unable to switch)
* <code>Mod</code>+<code>Space</code> or <code>Esc</code>
  close workspace dialog

