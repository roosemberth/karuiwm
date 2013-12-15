stwm
====

stwm is a not so small, not so fast, and not so dynamic window manager for X.

It is inspired by [xmonad](http://xmonad.org/) and based on
[dwm](http://dwm.suckless.org/).


build
-----

stwm requires Xlib and Xinerama to be installed on the system.

This will create the binary <code>stwm</code>:

	make

Alternatively, if you want to add debug output (all the <code>debug()</code>
calls in stwm, additional debug symbols through GCCs <code>-g</code> option),
you may run:

	make dev


install
-------

As the project is still in early development, there is currently no automated
way to install stwm on the system.


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
  launch dmenu\_run
* <code>PrtSc</code>
  launch scrot (screenshot)

**Clients**

* <code>Mod</code>+<code>j</code>|<code>k</code>
  set focus to next/previous client
* <code>Mod</code>+<code>t</code>
  toggle floating for selected client
* <code>Mod</code>+<code>Shift</code>+<code>c</code>
  close selected client

**Layout**

* <code>Mod</code>+<code>l</code>|<code>h</code>
  increase/decrease master area size
* <code>Mod</code>+<code>Shift</code>+<code>j</code>|<code>k</code>
  swap client with next/previous client in the layout
* <code>Mod</code>+<code>,</code>|<code>.</code>
  increase/decrease number of clients in the master area
* <code>Mod</code>(+<code>Shift</code>)+<code>Space</code>
  select next (previous) layout
* <code>Mod</code>+<code>Return</code>
  move selected client to master area

**Workspaces**

* <code>Mod</code>+<code>Ctrl</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  set view to left/below/above/right workspace
* <code>Mod</code>+<code>Ctrl</code>+<code>Shift</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  move and follow client to left/below/above/right workspace
* <code>Mod</code>+<code>i</code>
  switch to a workspace by name using dmenu
* <code>Mod</code>+<code>Shift</code>+<code>i</code>
  rename current workspace using dmenu
* <code>Mod</code>+<code>o</code>
  open workspace map (see appendix A)

**Scratchpad** (see appendix C)

* <code>Mod</code>+<code>Tab</code>
  toggle scratchpad
* <code>Mod</code>+<code>Shift</code>+<code>Tab</code>
  set focused client as scratchpad, or unset scratchpad if it is focused

**Monitors** (see appendix B)

* <code>Mod</code>+<code>m</code>
  set focus to next monitor

**Session**

* <code>Mod</code>+<code>q</code>
  restart stwm
* <code>Mod</code>+<code>Shift</code>+<code>q</code>
  quit stwm

The <code>Mod</code> key is set to Mod1 (<code>Alt</code>):

The mouse can be configured via the <code>buttons</code> array and currently
support following actions:

* <code>Mod</code>+<code>Button1</code>
  grab and move window, make it floating
* <code>Mod</code>+<code>Button3</code>
  resize window, make it floating


appendix A: workspaces
----------------------

Workspaces in stwm are arranged in a two-dimensional grid, and they are created
and destroyed dynamically. A workspace may be considered either *persistent*
(it remains if left) or *temporary* (leaving it destroys it).

Whether a workspaces is temporary or persistent depends on the number of clients
on that workspace; an empty workspace is considered temporary, whereas it
becomes persistent as soon as a client is added.

The workspace map (accessible by <code>Mod</code>+<code>Space</code>, see above)
is a visual representation of the workspaces and allows to perform actions on
them with a separately configured set of keys (the <code>wsmkeys</code> array):

* <code>Mod</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  set selection to left/below/above/right workspace
* <code>Mod</code>+<code>Shift</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  swap selected workspace with left/below/above/right workspace
* <code>Return</code>
  switch to the selected workspace
* <code>Esc</code>
  close workspace map without selecting a workspace

![screenshot](http://ayekat.ch/img/host/github.com/screen_stwm.png)

Each workspace is assigned a unique name that can be used to switch workspaces
by name. If no or an empty name is assigned to a workspace, it automatically
takes the workspace structure's pointer value, preceded by <code>\*</code>.


appendix B: Xinerama (aka multi-monitor)
----------------------------------------

stwm comes with Xinerama support and is thus capable of handling a multi-monitor
setup. Unlike its parent dwm however (which assigns a separate tag set to each
monitor), stwm uses a unified workspace set for all monitors.

Once a monitor is added, it will attempt to display the next undisplayed
workspace; if there is none, it will create a new workspace nearby and move its
view there. If a monitor moves its view to a workspace that is already displayed
on another monitor, the workspace view is swapped (except for floating windows,
they behave a bit buggy yet).


appendic C: scratchpad
----------------------

A scratchpad is a client that can easily be displayed and dismissed, typically a
terminal emulator to spontaneously type a command. In stwm, the scratchpad can
be toggled by hitting <code>Mod</code>+<code>Tab</code>.

At startup, no client is defined as the scratchpad (thus toggling it won't have
any effect); one first needs to define a window as the scratchpad by hitting
<code>Mod</code>+<code>Shift</code>+<code>Tab</code>.

