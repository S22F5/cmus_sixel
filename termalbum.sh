#!/bin/sh
file=$(cmus-remote -Q 2>/dev/null | sed  -n 's/^file //p')
[ -n "$file" ] || exit 0
cmus-remote -C refresh 2>/dev/null
ffmpeg -v error -i "$file" -map v:0 -frames:v 1 -f image2pipe -c:v png - 2>/dev/null | img2sixel > /dev/tty
