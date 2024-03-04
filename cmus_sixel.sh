#!/bin/bash
#config
six_palette=32
six_mult=7
x_offset=4
y_offset=0

#get music path
music_path="$(echo "$@" | sed 's/.*file\s//; s/\sartist.*//g; s/\salbumartist.*//g')"

if [[ "$music_path" != "$(cat "$HOME"/.config/cmus/current_song)" ]]
then
#update current_song
printf "$music_path" > "$HOME"/.config/cmus/current_song
#get lines & columns (bash -i crashes, tput lines doesnt work, checkwinsize doesnt work)
cmus-remote -C refresh
eval "$( resize )"
six_width=$(("$LINES" * "$six_mult"))
#extract sixel cover from flac
#libsixel
exiftool "$music_path" -Picture -b | img2sixel -w"$six_width" -p"$six_palette" > "$HOME"/.config/cmus/cover.six
#imagemagisk (slower)
#exiftool "$music_path" -Picture -b | convert - -geometry "$width" +dither -colors "$six_palette" sixel:- > "$HOME"/.config/cmus/cover.six
#get term
tty=$(printf "/dev/" ;ps hotty $$)
#get $HEIGHT and $WIDTH
eval "$(xdotool getactivewindow getwindowgeometry --shell)"
#save cursor position
printf "\e[6n"; read -sdR CURPOS ;CURPOS=${CURPOS#*[}
#display sixel cover
printf "\e[$((LINES - $((six_width / $((HEIGHT / LINES))))-x_offset));$((COLUMNS - $((six_width / $((WIDTH / COLUMNS))))-y_offset))H%s" "$(cat "$HOME"/.config/cmus/cover.six)" > "$tty"
#restore cursor position
printf "\e["$CURPOS"H"
fi
