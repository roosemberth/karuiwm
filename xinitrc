#!/bin/sh

[ -f ~/.Xresources ] && xrdb ~/.Xresources
xterm &
[ $USER = 'ayekat' ] && setxkbmap -layout ch   # *sigh*
exec ./stwm

