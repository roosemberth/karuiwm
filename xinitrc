#!/bin/sh

xrdb ~/.Xresources
for t in xfce4-terminal roxterm gnome-terminal; do
	if $(which "$t" 2>&1 1>/dev/null); then
		"$t" &
		found=1
		break
	fi
done
[ $found ] || xterm &
exec ./stwm

