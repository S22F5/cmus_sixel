#!/bin/bash
#config
six_palette=32
six_mult=7
x_offset=4
y_offset=0

#get lines & columns (bash -i crashes, tput lines doesnt work, checkwinsize doesnt work)
cmus-remote -C refresh
eval "$( resize )"
six_width=$(("$LINES" * "$six_mult"))
#get music path
music_path="$(echo "$@" | sed 's/.*file\s//; s/\sartist.*//g; s/\salbumartist.*//g')"
#extract sixel cover from flac
#libsixel
exiftool "$music_path" -Picture -b | img2sixel -w"$six_width" -p"$six_palette" > "$HOME"/.config/cmus/cover.six
#imagemagisk (slower)
#exiftool "$music_path" -Picture -b | convert - -geometry "$width" +dither -colors "$six_palette" sixel:- > "$HOME"/.config/cmus/cover.six
#get term
tty=$(printf "/dev/" ;ps hotty $$)
#get $HEIGHT and $WIDTH
eval "$(xdotool getactivewindow getwindowgeometry --shell)"
#display sixel cover
printf "\e[$((LINES - $((six_width / $((HEIGHT / LINES))))-x_offset));$((COLUMNS - $((six_width / $((WIDTH / COLUMNS))))-y_offset))H%s" "$(cat "$HOME"/.config/cmus/cover.six)" > "$tty"



##debug
#debug: terminfo
printf "coverCursor: %s,%s\n" "$((LINES - $((six_width / $((HEIGHT / LINES))))-4))" "$((COLUMNS - $((six_width / $((WIDTH / COLUMNS))))))" > /home/seefs/csix.log
printf "TerminalInfo: %s %s %s %s\n" "$TERM" "$(whoami)" "$0" "$(pwd)" >> "$HOME"/csix.log
printf "Input: %s\n" "$*" >> "$HOME"/csix.log
#debug: line and columns
printf "LinCol: %s,%s\n" "$LINES" "$COLUMNS" >> "$HOME"/csix.log
printf "SixWidthPix: %s\n" "$six_width" >> "$HOME"/csix.log
printf "SixWidthCol: %s\n" "$((six_width/$((HEIGHT / COLUMNS))))" >> "$HOME"/csix.log
#debug: term size
printf "termsize: %s,%s\n" "$HEIGHT" "$WIDTH" >> "$HOME"/csix.log
printf "pixels_perline: %s\n" "$((HEIGHT / LINES))" >> "$HOME"/csix.log
printf "pixels_percolumn: %s\n" "$((WIDTH / COLUMNS))" >> "$HOME"/csix.log
#debug: term
printf "term: %s\n" "$tty" >> "$HOME"/csix.log
#debug: home dir
printf "home: %s\n" "$HOME" >> "$HOME"/csix.log

#unused
##getuniqsig
# exiftool "$music_path"  -MD5Signature -b
##get lyrics
#exiftool  "$music_path"  -Lyrics -b >> $HOME/.config/cmus/cover.six
