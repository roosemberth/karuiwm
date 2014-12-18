#!/usr/bin/env sh

xrdb $XDG_CONFIG_HOME/X11/Xresources

nitrogen --restore

if [ $VALGRIND -eq 1 ]; then
	exec valgrind --log-file=valgrind.log --leak-check=full --show-leak-kinds=all -v ./karuiwm
else
	exec ./karuiwm
fi
