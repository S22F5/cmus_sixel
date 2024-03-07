#!/bin/bash
#config
six_palette=32
six_mult=4
x_offset=4
y_offset=0


#remove runf when exiting
if [[ $2 == "exiting" ]]
then
mv "$HOME"/.config/cmus/.runf "$HOME"/.config/cmus/.old_runf
mv "$HOME"/.config/cmus/.current_song "$HOME"/.config/cmus/.old_current_song
exit
fi

#get music path
music_path="$(echo "$@" | sed 's/.*file\s//; s/\sartist.*//g; s/\salbumartist.*//g')"

if [[ "$music_path" != "$(cat "$HOME"/.config/cmus/.current_song)" ]]
then

#update current_song
printf "%s" "$music_path" > "$HOME"/.config/cmus/.current_song
#get lines & columns (bash -i crashes, tput lines doesnt work, checkwinsize doesnt work)
cmus-remote -C refresh
eval "$( resize )"
#if inital start
if [ ! -f "$HOME"/.config/cmus/.runf ]
then
#get initial $WINDOW $HEIGHT $WIDTH
eval "$(xdotool getactivewindow getwindowgeometry --shell)"
#save $WINDOW to runf
echo "$WINDOW" > "$HOME"/.config/cmus/.runf
else 
#get $WINDOW $HEIGHT $WIDTH
eval "$(xdotool getwindowgeometry --shell "$(cat "$HOME"/.config/cmus/.runf)")"
fi
#get sixel size
six_size=$(("$HEIGHT" / 10 * "$six_mult"))
#extract sixel cover from flac
#libsixel
exiftool "$music_path" -Picture -b | img2sixel -h"$six_size" -p"$six_palette" > "$HOME"/.config/cmus/.cover.six
#imagemagisk (slower)
#exiftool "$music_path" -Picture -b | convert - -geometry "$six_size" +dither -colors "$six_palette" sixel:- > "$HOME"/.config/cmus/.cover.six
#get term
tty=$(printf "/dev/" ;ps hotty $$)
#save cursor position
printf "\e[6n"; read -sdR CURPOS ;CURPOS=${CURPOS#*[}
#display sixel cover
printf "\e[$((LINES - $((six_size / $((HEIGHT / LINES))))-x_offset));$((COLUMNS - $((six_size / $((WIDTH / COLUMNS))))-y_offset))H%s" "$(cat "$HOME"/.config/cmus/.cover.six)" > "$tty"
#restore cursor position
printf "\e["$CURPOS"H"

fi
