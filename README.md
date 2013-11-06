ayekat/stwm README
==================

Simple Tiling Window Manager.

Does not manage any windows yet. But it *does* tiling. And it is simple.


build & run
-----------

<code>make</code> will create a binary *stwm*.

<code>make run</code> will launch it on the X display :1 (if you've got already
something running there, modify the Makefile, or launch stwm manually on your
preferred X display).


usage
-----

* <code>Space</code>: add a new window
* <code>d</code>: delete a window
* <code>l</code>: increase the master area
* <code>h</code>: decrease the master area

*stwm* can be quit by deleting a window when there is no window.

