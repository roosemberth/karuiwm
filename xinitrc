#!/bin/sh
# Xinit script for starting stwm.

# check for custom startup actions:
[ -f ./xinitrc.custom ] && . ./xinitrc.custom

# launch stwm:
exec ./stwm

