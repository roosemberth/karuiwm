karuiwm
=======

karuiwm is a lightweight, dynamically tiling window manager for X.

It is based on [dwm](http://dwm.suckless.org/) and inspired by
[xmonad](http://xmonad.org/).


build
-----

karuiwm requires Xlib and Xinerama to be installed on the system.

This will create the binary <code>karuiwm</code>:

	make


install
-------

karuiwm requires dmenu to run correctly. If dmenu is missing, the behaviour is
unspecified yet; probably it will freeze the moment there is an interaction with
dmenu.

Running the following command as root will install karuiwm to
<code>/usr/local/bin</code> by default:

	make install

Modify the Makefile to change the installation directory.


uninstall
---------

	make uninstall

as root will remove karuiwm from your system. Alternatively, you may simply
delete the <code>karuiwm</code> binary from <code>/usr/local/bin</code> (or
where ever you installed it to).


run
---

Follow the standard procedure for launching a WM; add an <code>.xinitrc</code>
to your home directory, then launch <code>xinit</code>.

Here is an example <code>.xinitrc</code>:

	#!/bin/sh
	# xinitrc to launch karuiwm
	
	# set sane keyboard layout:
	setxkbmap -layout ch -option 'caps:swapescape' &
	
	# launch karuiwm:
	exec karuiwm

You may add other actions to <code>.xinitrc</code>, but make sure that
<code>exec karuiwm</code> comes at last (since everything after that won't get
executed).


configure
---------

<code>config.h</code> holds the configuration of karuiwm and gets included by
<code>karuiwm.c</code> upon compilation.

<code>config.def.h</code> holds the default configuration of karuiwm. When
running <code>make</code>, this file will be used to generate a
<code>config.h</code> if that one doesn't exist yet. This protects your
configuration from being overwritten by the default upon checking out the git
repository.

These are the default settings defined by the <code>keys</code> array;
<code>Mod</code> is set to the Mod1 (<code>Alt</code>) key:

**Applications**

* <code>Mod</code>+<code>n</code>
  launch xterm
* <code>Mod</code>+<code>p</code>
  launch dmenu\_run
* <code>PrtSc</code>
  launch scrot (screenshot)

**Hardware**

* <code>VolUp</code>|<code>VolDown</code>
  increase/decrease ALSA's Master sound level by 2%
* <code>VolMute</code>
  toggle ALSA's Master sound (mute/unmute)

**Clients**

* <code>Mod</code>+<code>j</code>|<code>k</code>
  set focus to next/previous client
* <code>Mod</code>+<code>t</code>
  toggle floating for selected client
* <code>Mod</code>+<code>Shift</code>+<code>c</code>
  close selected client
* <code>Mod</code>+<code>u</code>
  select client by name

**Layout** (see appendix A)

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

**Workspaces** (see appendix B)

* <code>Mod</code>+<code>Ctrl</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  set view to left/below/above/right workspace
* <code>Mod</code>+<code>Ctrl</code>+<code>Shift</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  move and follow client to left/below/above/right workspace
* <code>Mod</code>+<code>i</code>
  switch to a workspace by name
* <code>Mod</code>+<code>Shift</code>+<code>i</code>
  send selected client to a workspace by name
* <code>Mod</code>+<code>Shift</code>+<code>Ctrl</code>+<code>i</code>
  send and follow selected client to a workspace by name
* <code>Mod</code>+<code>r</code>
  rename current workspace
* <code>Mod</code>+<code>o</code>
  open workspace map

**Scratchpad** (see appendix C)

* <code>Mod</code>+<code>Tab</code>
  toggle scratchpad
* <code>Mod</code>+<code>Shift</code>+<code>Tab</code>
  set focused client as scratchpad, or unset scratchpad if it is focused

**Monitors** (see appendix D)

* <code>Mod</code>+<code>m</code>
  set focus to next monitor

**Session**

* <code>Mod</code>+<code>q</code>
  restart karuiwm
* <code>Mod</code>+<code>Shift</code>+<code>q</code>
  quit karuiwm

The mouse can be configured via the <code>buttons</code> array and currently
supports following actions:

* <code>Mod</code>+<code>Button1</code>
  grab and move client, make it floating
* <code>Mod</code>+<code>Button3</code>
  resize client, make it floating


appendix A: layouts
-------------------

karuiwm is a *dynamically tiling window manager*, this means that it
automatically arranges windows following certain dynamically changable rules,
called *layouts*. This allows the user to focus on the workflow instead of
having to arrange windows manually.

You can cycle through the layouts with <code>Mod</code>+<code>Space</code>; the
windows will get arranged automatically.

The layouts are defined in <code>layout.h</code>; the array at the end of the
file contains the layouts that will be actually used in your personal karuiwm
setup. Every layout below the <code>NULL</code> entry will not be accessible
through the usual layout cycling.

Every layout is assigned a bitfield that describes an icon that will get
displayed in the statusbar to indicate the current layout. Every hexadecimal
<code>long</code> entry in the array corresponds to a row in the icon (thus an
icon cannot be wider than <code>sizeof(long)\*8</code> pixels, but that
shouldn't be a problem).


appendix B: workspaces
----------------------

Workspaces in karuiwm are arranged in a two-dimensional grid, and they are
created and destroyed dynamically. A workspace may be considered either
*persistent* (it remains if left) or *temporary* (leaving it destroys it).

Whether a workspaces is temporary or persistent depends on the number of clients
on that workspace; an empty workspace is considered temporary, whereas it
becomes persistent as soon as a client is added.

The workspace map (accessible by <code>Mod</code>+<code>o</code>, see above) is
a visual representation of the workspaces and allows to perform actions on them
with a separately configured set of keys (the <code>wsmkeys</code> array):

* <code>Mod</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  set selection to left/below/above/right workspace
* <code>Mod</code>+<code>Shift</code>+<code>h</code>|<code>j</code>|<code>k</code>|<code>l</code>
  swap selected workspace with left/below/above/right workspace
* <code>Return</code>
  switch to the selected workspace
* <code>Esc</code>
  close workspace map without selecting a workspace

Each workspace is assigned a unique name that can be used to switch workspaces
by name using dmenu (see the dmenu manpage for further information how to use
it). If no or an empty name is assigned to a workspace, it automatically
takes the workspace structure's pointer value, preceded by <code>\*</code>.


appendix C: scratchpad
----------------------

A scratchpad is a window that can easily be displayed and dismissed, typically a
terminal emulator to spontaneously type a command. In karuiwm, the scratchpad
can be toggled by hitting <code>Mod</code>+<code>Tab</code>.

At startup, no client is defined as the scratchpad (thus toggling it won't have
any effect); one first needs to define a client as the scratchpad by hitting
<code>Mod</code>+<code>Shift</code>+<code>Tab</code>.


appendix D: Xinerama (aka multi-monitor)
----------------------------------------

karuiwm comes with Xinerama support and is thus capable of handling a
multi-monitor setup. Unlike its parent dwm however (which assigns a separate tag
set to each monitor), karuiwm uses a unified workspace set for all monitors.

Once a monitor is added, it will attempt to display the next undisplayed
workspace; if there is none, it will create a new workspace nearby and move its
view there. If a monitor moves its view to a workspace that is already displayed
on another monitor, the workspace view is swapped.

