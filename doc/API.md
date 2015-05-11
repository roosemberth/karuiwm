karuiwm API
===========

The API is accessible via a Unix socket in /var/run/karuiwm.sock.
It accepts messages in binary form. A command line utility (`karuictl`) will be
made available once the binary API is stable enough. However, the binary
messages and arguments are referred to by their command line utility's
equivalent string variant, for the sake of readability.


messages
--------

* **0x00**, `get`
* **0x01**, `set`

Messages can be sent both by the server and the client, although some messages
will be ignored on the server side (and some should be ignored on the client
side).


properties
----------

`get` and `set` are used to read property values and assigning new values to
them. The structure is as follows:

	get PROPERTY
	set PROPERTY [INTEGER ARGUMENTS ...] [STRING ARGUMENTS ...]

The number of arguments for a `set` message depends on the property, which is
defined through a 1-byte ID. The argument can either be a 32-bit little endian
integer, or a null-terminated string.

The list of properties (and their arguments for `set`) is as follows:

* **0x00**, `monitor`: Focused monitor
  * `INTEGER`: monitor index
* **0x01**, `workspace`: Focused workspace
  * `INTEGER`: monitor index
  * `STRING`: workspace name
* **0x02**, `desktop`: Focused desktop
  * `INTEGER`: monitor index
  * `STRING`: desktop name
* **0x03**, `layout`: Active layout
  * `INTEGER`: monitor index
  * `STRING`: layout name
* **0x04**, `nmaster`: Number of clients in master area
  * `INTEGER`: monitor index
  * `INTEGER`: number of clients in master area
* **0x05**, `urgent-workspace`: Workspace that contains a client with the
  `URGENT` hint (server-only)
  * `STRING`: workspace name
* **0x06**, `urgent-desktop`: Desktop that contains a client with the `URGENT`
  hint (server-only)
  * `STRING`: desktop name
* **0x07**, `subscription`: Events to which the client is subscribed
  * `INTEGER`: mode (0=set, 1=add, 2=delete, 3=toggle)
  * `INTEGER`: 32-bit (flags), see list of events below

Setting the monitor index to -1 (`0xFFFFFFFF`) will be interpreted as "the
currently focused monitor".


events
------

Clients may subscribe to different types of events in order to be notified when
something happens. It is done so through setting the bitfield/mask for the
`subscription` property.

* **0x01**, `monitor`: Events concerning the monitor focus
  Received messages: `focus`
* **0x02**, `workspace`: Events concerning workspaces
  Received messages: `workspace`, `urgent-workspace`
* **0x04**, `desktop`: Events concerning desktops
  Received messages: `desktop`, `urgent-desktop`
* **0x08**, `layout`: Events concerning desktops' layout
  Received messages: `layout`, `nmaster`
* **0x10**, `urgent`: Events concerning `URGENT` hints
  Received messages: `urgent-workspace`, `urgent-desktop*