core
====

The `core` module adds the basic functionality for handling windows, desktops
and workspaces through mouse and keyboard input; it is therefore strongly
recommended to enable the `core` module (or a module that provides equivalent
functionality).

The actions provided by `core` are:

* `core.killclient`: Close the currently focused client's window.
* `core.mousemove`: Detach client under mouse from tiled layout and move it
  around freely.
* `core.mouseresize`: Detach client under mouse from tiled layout and resize it
  freely.
* `core.restart`: Stop karuiwm and start it again.
* `core.setmfact[FLOAT]`: Increase/decrease master area size by factor `FLOAT`.
* `core.setnmaster[INT]`: Increase/decrease number of clients in master area by
  `INT` clients.
* `core.shiftclient[prev|next]`: Swap client with previous/next client in
  layout.
* `core.spawn[STRING]`: Fork and execute a program `STRING`.
* `core.stepclient[prev|next]`: Set focus to previous/next client in layout.
* `core.stepdesktop[up|right|down|left]`: Set focus to upper/right/lower/left
  desktop.
* `core.steplayout[prev|next]`: Arrange tiled clients according to next/previous
  layout.
* `core.stop`: Stop karuiwm.
* `core.togglefloat`: Toggle floating state for focus client.
* `core.zoom`: Swap client with first client in the master area, or swap with
  next client if already first master.
