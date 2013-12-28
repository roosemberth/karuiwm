stwm
====

stwm is a small, dynamically tiling window manager for X.

It is based on [dwm](http://dwm.suckless.org/) and inspired by
[xmonad](http://xmonad.org/).


build
-----

stwm requires Xlib and Xinerama to be installed on the system. It is also highly
recommended to install dmenu, since some workspace-related actions rely on it.

This will create the binary <code>stwm</code>:

	make

If you want to add debug output (all the <code>debug()</code> calls in stwm,
additional debug symbols through GCCs <code>-g</code> option), you may run:

	make devbuild

This is especially useful when developing stwm; for starting stwm directly in
the current directory without installing it, run

	make devrun

This will launch the created stwm binary on the X display :1 (modify the
Makefile to change). To speed up things a little,

	make dev

runs both <code>devbuild</code> and <code>devrun</code>.


install
-------

	make install

as root will install stwm to <code>/usr/local/bin</code> by default; modify the
Makefile to change the installation directory.


uninstall
---------

	make uninstall

as root will remove stwm from your system. Alternatively, you may simply delete
the <code>stwm</code> binary from <code>/usr/local/bin</code> (or where ever you
installed it to).


run
---

Follow the standard procedure for launching a WM; add an <code>.xinitrc</code>
to your home directory, then launch <code>xinit</code>.

Here is an example <code>.xinitrc</code>:

	#!/bin/sh
	# xinitrc to launch stwm
	
	# set sane keyboard layout:
	setxkbmap -layout ch -option 'caps:swapescape' &
	
	# launch stwm:
	exec stwm

You may add other actions to launch the WM, but make sure that
<code>exec stwm</code> comes at last (since everything after that won't get
executed).


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

**Monitors** (see appendix D)

* <code>Mod</code>+<code>m</code>
  set focus to next monitor

**Session**

* <code>Mod</code>+<code>q</code>
  restart stwm
* <code>Mod</code>+<code>Shift</code>+<code>q</code>
  quit stwm

By default, the <code>Mod</code> key is set to Mod1 (<code>Alt</code>).

The mouse can be configured via the <code>buttons</code> array and currently
supports following actions:

* <code>Mod</code>+<code>Button1</code>
  grab and move client, make it floating
* <code>Mod</code>+<code>Button3</code>
  resize client, make it floating


appendix A: layouts
-------------------

stwm is a *dynamically tiling window manager*, this means that it automatically
arranges windows following certain dynamically changable rules, called
*layouts*. This allows the user to focus on the workflow instead of having to
arrange windows manually.

You can cycle through the layouts with <code>Mod</code>+<code>Space</code>; the
windows will get arranged automatically.

The layouts are defined in <code>layout.h</code>; the array at the end of the
file contains the layouts that will be actually used in your personal stwm
setup. Every layout below the <code>NULL</code> entry will not be accessible
through the usual layout cycling, but will need an separate setup.

This may be useful with a monocle layout (only displays one maximised window):
As you cycle through your layouts, you seldom need the monocle layout; however
in case you *need* the monocle layout, you may not want to cycle through all
your other layouts. In this case it is more appropriate to define a keyboard
shortcut that toggles the monocle layout.

*(Note: monocle layout and layout toggling has not yet been implemented)*

Every layout is assigned a bitfield that describes an icon that will get
displayed in the statusbar to indicate the current layout. Every hexadecimal
<code>long</code> entry in the array corresponds to a row in the icon (thus an
icon cannot be larger than <code>sizeof(long)\*8</code>, but that shouldn't be a
problem).


appendix B: workspaces
----------------------

Workspaces in stwm are arranged in a two-dimensional grid, and they are created
and destroyed dynamically. A workspace may be considered either *persistent*
(it remains if left) or *temporary* (leaving it destroys it).

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

Renaming and switching workspaces happens through the key combinations
<code>Mod</code>(+<code>Shift</code>)+<code>i</code> (see above).


appendix C: scratchpad
----------------------

A scratchpad is a window that can easily be displayed and dismissed, typically a
terminal emulator to spontaneously type a command. In stwm, the scratchpad can
be toggled by hitting <code>Mod</code>+<code>Tab</code>.

At startup, no client is defined as the scratchpad (thus toggling it won't have
any effect); one first needs to define a client as the scratchpad by hitting
<code>Mod</code>+<code>Shift</code>+<code>Tab</code>.


appendix D: Xinerama (aka multi-monitor)
----------------------------------------

stwm comes with Xinerama support and is thus capable of handling a multi-monitor
setup. Unlike its parent dwm however (which assigns a separate tag set to each
monitor), stwm uses a unified workspace set for all monitors.

Once a monitor is added, it will attempt to display the next undisplayed
workspace; if there is none, it will create a new workspace nearby and move its
view there. If a monitor moves its view to a workspace that is already displayed
on another monitor, the workspace view is swapped.


meta
----

Bug reports are welcome, especially since this is still in a quite early
development state.

Feature requests are welcome too, as long as they do not require me to add a
thousand lines of code or change the programming language.

In the unlikely case there is any need for support, visit
<code>irc.freenode.net#stwm</code>.

