#!/usr/bin/env sh

# Source user-specific commands:
if [ -e ./xinitrc.custom ]; then
	. ./xinitrc.custom
fi

# Start WM:
if [ -n "$VALGRIND" ] && [ $VALGRIND = '1' ]; then
	exec valgrind --log-file=valgrind.log \
	              --leak-check=full \
	              --show-leak-kinds=all \
	              --show-reachable=no \
	              --track-origins=yes \
	              -v \
	              ./karuiwm -d
else
	exec ./karuiwm -d
fi
