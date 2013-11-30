stwm
====

An attempt to build a dynamically tiling window manager with Xlib. It is
inspired by [xmonad](http://xmonad.org/) and based on
[dwm](http://dwm.suckless.org/).


build
-----

This will create the binary <code>stwm</code>:

	make

Alternatively, if you want to add debug output (all the <code>debug()</code>
calls in stwm, additional debug symbols through GCCs <code>-g</code> option),
you may run:

	make dev


install
-------

Currently there is no automated way to install stwm on the system.


run
---

Given that stwm is not yet ready to be installed on any system, it allows a way
to invoke it from within the project directory.

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

These are the default settings defined by the <code>keys</code> array:

**Applications**

* <code>Mod</code>+<code>n</code>
  launch xterm
* <code>Mod</code>+<code>p</code>
  launch dmenu
* <code>PrtSc</code>
  launch scrot (screenshot)

**Windows**

* <code>Mod</code>+<code>j</code>|<code>k</code>
  set focus to next/previous client
* <code>Mod</code>+<code>Shift</code>+<code>c</code>
  close selected client

**Layout**

* <code>Mod</code>+<code>l</code>|<code>h</code>
  increase/decrease master area size
* <code>Mod</code>+<code>Shift</code>+<code>j</code>|<code>k</code>
  swap client with next/previous client in the layout
* <code>Mod</code>+<code>,</code>|<code>.</code>
  increase/decrease number of clients in the master area
* <code>Mod</code>+<code>Return</code>
  move selected client to master area

**Workspaces**

* <code>Mod</code>+<code>Ctrl</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  set view to left/below/above/right workspace
* <code>Mod</code>+<code>Ctrl</code>+<code>Shift</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  move and follow client to left/below/above/right workspace
* <code>Mod</code>+<code>Space</code>
  open workspace dialog (see below)

**Session**

* <code>Mod</code>+<code>q</code>
  restart stwm
* <code>Mod</code>+<code>Shift</code>+<code>q</code>
  quit stwm

The <code>Mod</code> key is set to Mod1 (<code>Alt</code>):


workspace dialog
----------------

The workspace dialog is a view that comes with a separately configured set of
keys. It consists of an input bar to change the workspace by entering its name,
and of a "map" to visually select a workspace.

The <code>wsdkeys</code> array defines keys for using the visual selection:

* <code>Mod</code>+<code>Space</code>
  close workspace dialog
* <code>Mod</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  set selection to left/below/above/right workspace
* <code>Mod</code>+<code>Shift</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  swap selected workspace with left/below/above/right workspace
* <code>Return</code>
  switch to the selected workspace (see below)
* <code>Ctrl</code>+<code>Return</code>
  rename current workspace

The input bar allows you to type a name and perform one of the following
actions:

* <code>Return</code>
  change to the workspace matching the specified name
* <code>Ctrl</code>+<code>Return</code>
  rename the *current* (!) workspace to the specified name
* <code>Esc</code>
  close workspace dialog

Besides this, some ANSI control sequences are supported: <code>^A</code> (home),
<code>^B</code> (left), <code>^C</code> (cancel), <code>^D</code> (delete),
<code>^E</code> (end), <code>^F</code> (right), <code>^H</code> (backspace),
<code>^J</code> (return), <code>^K</code> (clear right), <code>^M</code>
(return), <code>^U</code> (clear left).

Note that the name matching will compare the first [n] characters, where [n] is
the name of the specified string. In case the name matches multiple workspaces,
the bahaviour is unspecified (should get fixed in the future).

![screenshot](http://ayekat.ch/img/host/github.com/screen_stwm.png)


appendix A: workspaces
----------------------

Workspaces in stwm are arranged in a two-dimensional grid, and they are created
and destroyed dynamically. A workspace may be considered either *persistent*
(it remains if left) or *temporary* (leaving it destroys it).

Whether a workspaces is temporary or persistent depends on the number of clients
on that workspace; an empty workspace is considered temporary, whereas it
becomes persistent as soon as a client is added.


appendix B: multi-monitor
-------------------------

stwm comes with Xinerama support and is thus capable of handling a multi-monitor
setup. Unlike its parent dwm however (which assigns a separate tag set to each
monitor), stwm uses a unified workspace set for all monitors.

Once a monitor is added, it will attempt to display the next undisplayed
workspace; if there is none, it will create a new workspace nearby and move its
view there. If a monitor moves its view to a workspace that is already displayed
on another monitor, the behaviour is unspecified (should get fixed in the
future).


todo
----

* Complete multi-monitor (switching monitors, avoiding focus collisions, sending
  clients from one monitor to the other, ...)
* Add session restore upon <code>restart()</code> (savefile?).
* Add scratchpad.
* Add mouse support.

