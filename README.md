karuiwm
=======

karuiwm is an attempt to write a lightweight, dynamically tiling window manager
for X.

It is currently under heavy development and rather unusable.


build
-----

karuiwm requires Xlib and Xinerama to be installed on the system.

This will create the binary `karuiwm`:

	make


install
-------

karuiwm requires dmenu to run correctly.

Running the following command as root will install karuiwm to
`/usr/local/bin` by default:

	make install

Modify the Makefile to change the installation directory.


run
---

Follow the standard procedure for launching a WM: add an `.xinitrc`
to your home directory, then launch `xinit`.

Here is an example `.xinitrc`:

	#!/bin/sh
	# xinitrc to launch karuiwm
	
	# set sane keyboard layout:
	setxkbmap -layout ch -option 'caps:swapescape' &
	
	# launch karuiwm:
	exec karuiwm

These are currently the key bindings and settings for karuiwm (`Mod` is set to
`Alt`). Since the project is currently at an early development stage, this is
hardcoded and can only be changed by modifying the source code directly:

**Applications**

* `Mod`+`n`: launch urxvt
* `Mod`+`p`: launch dmenu\_run
* `Mod`+`PrtSc`: launch the
  [`prtscr`](https://github.com/ayekat/dotfiles/blob/master/.local/bin/prtscr)
  script to take a screenshot

**Hardware**

* `VolUp`|`VolDown`
  increase/decrease ALSA's Master sound level by 2%
* `VolMute`
  toggle ALSA's Master sound (mute/unmute)

**Clients**

* `Mod`+`j`|`k`
  set focus to next/previous client
* `Mod`+`t`
  toggle floating for selected client
* `Mod`+`Shift`+`c`
  close selected client
* `Mod`+`u`
  select client by name

**Layouts**

* `Mod`+`l`|`h`
  increase/decrease master area size
* `Mod`+`Shift`+`j`|`k`
  swap client with next/previous client in the layout
* `Mod`+`,`|`.`
  increase/decrease number of clients in the master area
* `Mod`(+`Shift`)+`Space`
  select next (previous) layout
* `Mod`+`Return`
  move selected client to master area

**Workspaces**

* `Mod`+`Ctrl`+`h`|`j`|`k`|`l`
  set view to left/below/above/right workspace
* `Mod`+`Ctrl`+`Shift`+`h`|`j`|`k`|`l`
  move and follow client to left/below/above/right workspace
* `Mod`+`i`
  switch to a workspace by name
* `Mod`+`Shift`+`i`
  send selected client to a workspace by name
* `Mod`+`Shift`+`Ctrl`+`i`
  send and follow selected client to a workspace by name
* `Mod`+`r`
  rename current workspace
* `Mod`+`o`
  open workspace map

**Monitors**

* `Mod`+`m`
  set focus to next monitor

**Session**

* `Mod`+`Shift`+`q`
  restart karuiwm
* `Mod`+`Shift`+`Ctrl`+`q`
  quit karuiwm

**Mouse**

* `Mod`+`Button1`
  grab and move client, make it floating
* `Mod`+`Button3`
  resize client, make it floating
