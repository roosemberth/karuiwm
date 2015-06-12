karuiwm ACTIONS
===============


action
------

An *action* is a a sequence of operations on the WM that keep its state
consistent, e.g. "detach client from desktop" is *not* an action, however it may
be part of the action "move client from desktop A to desktop B".

Each component (desktop, workspace, ...) provide a list of functions that take a
`union arg *` as argument, and perform certain things.


event
-----

An *event* is to be *triggered*, i.e. at the end of an action, we must specify
a list of all events that are caused by that action. In particular, this means
creating new (concrete) events and adding them to the scheduler.

Each event takes a property ID and a value (e.g. moving the focus to a desktop
called "example" emits the `DESKTOP` property with `example` as a string value
attached to all subscribed clients).
