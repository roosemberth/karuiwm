stwm
====

Simple Tiling Window Manager.


build & run
-----------

In order to build stwm, run

	make

This will create the binary <code>stwm</code>.

In order to run stwm correctly, you will need to launch it on a new X display:

	make run

This will launch a new X session on display **:1** with stwm as the window
manager and an xterm window opened. In case the display is occupied, you might
need to adapt the according rule in the Makefile.

Alternatively, if you have got Xephyr installed, you may also launch

	make xephyr

to launch stwm inside a window.


usage
-----

* <code>Mod4</code>+<code>q</code> quit stdwm
* <code>Mod4</code>+<code>l</code> increase the master area
* <code>Mod4</code>+<code>h</code> decrease the master area

Note that there is currently no way to launch an application from *bare* stwm,
so closing the last terminal will effectively render stwm useless, and the only
thing you can do is terminate it.

