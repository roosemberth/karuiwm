#!/bin/sh

xrdb ~/.Xresources
xterm &
exec ./stwm

