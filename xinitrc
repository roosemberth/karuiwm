#!/bin/sh

[ -f ~/.Xresources ] && xrdb ~/.Xresources

# AYEKAT FLAVOUR ---------------------------------------------------------------
# Simplifies life for the developer.

if [ $USER = 'ayekat' ]; then
	setxkbmap -layout ch &
	nitrogen --restore &
	lcdadjust start &
fi

# ------------------------------------------------------------------------------

exec ./stwm

