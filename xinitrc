#!/bin/sh

[ -f ~/.Xresources ] && xrdb ~/.Xresources
xterm &
exec ./stwm

