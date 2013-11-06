ayekat/stwm README
==================

Simple Tiling Window Manager.

It does not Manage any Windows yet. But it *does* Tiling.

And it is Simple.


build & run
-----------

	make

will create a binary <code>stwm</code>.

	make run

will launch *stwm* on the X display **:1** (if you've got already something
running there, modify the Makefile, or launch *stwm* manually on your preferred
X display).


usage
-----

* <code>Space</code> add a new window
* <code>q</code> delete a window
* <code>l</code> increase the master area
* <code>h</code> decrease the master area

*stwm* can be quit by deleting a window when there is no window.

