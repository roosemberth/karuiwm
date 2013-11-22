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


keys (default configuration)
----------------------------

The <code>Mod</code> key is set to Mod1 (<code>Alt</code>):

**Applications**

* <code>Mod</code>+<code>n</code>
  launch xterm
* <code>Mod</code>+<code>p</code>
  launch dmenu

**Windows**

* <code>Mod</code>+<code>j</code>|<code>k</code>
  set focus to next/previous client
* <code>Mod</code>+<code>c</code>
  close selected client

**Layout**

* <code>Mod</code>+<code>l</code>|<code>h</code>
  increase/decrease master area size
* <code>Mod</code>+<code>j</code>|<code>k</code>
  swap client with next/previous client in the layout
* <code>Mod</code>+<code>,</code>|<code>.</code>
  increase/decrease number of clients in the master area
* <code>Mod</code>+<code>Return</code>
  move selected client to master area

**Workspaces**

* <code>Mod</code>+<code>Shift</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  switch to left/below/above/right workspace
* <code>Mod</code>+<code>Space</code>
  open workspace dialog (see below)

**Session**

* <code>Mod</code>+<code>q</code>
  restart stwm
* <code>Mod</code>+<code>Shift</code>+<code>q</code>
  quit stwm


workspaces
----------

Workspaces in stwm are arranged in a two-dimensional grid, and they are created
and destroyed dynamically. A workspace may be considered either *persistent*
(it remains if left) or *temporary* (leaving it destroys it), depending on
whether there is a client on the workspace.

The change from persistent to temporary (or vice versa) happens automatically
upon removing the last client (or placing the first client).


workspace dialog (default configuration)
----------------------------------------

The workspace dialog allows you to change the workspace either by name or by
selection on a "map":

* <code>Mod</code>+<code>Space</code> or <code>Esc</code>
  close workspace dialog
* <code>Mod</code>+<code>Shift</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  move target to left/below/above/right workspace
* <code>Return</code>
  switch to workspace by name, or to target workspace if name is not matched
* <code>Ctrl</code>+<code>Return</code>
  rename current workspace

Note that the name matching will compare the first [n] characters, where [n] is
the name of the specified string. In case the name matches multiple workspaces,
the bahaviour is unspecified (should get fixed in the future).

![screenshot](http://ayekat.ch/img/host/github.com/screen_stwm.png)

