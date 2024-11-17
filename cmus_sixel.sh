#!/bin/sh
#config
six_palette=64
six_mult=35
x_offset=3
y_offset=0

#remove runf when exiting
if [ "$2" = "exiting" ]
then
  mv "$HOME"/.config/cmus/.runf "$HOME"/.config/cmus/.old_runf
  mv "$HOME"/.config/cmus/.pid "$HOME"/.config/cmus/.old_pid
  mv "$HOME"/.config/cmus/.current_song "$HOME"/.config/cmus/.old_current_song
  exit
fi
#get music path
music_path="$(expr "$*" : '[^0]*file \(.*\) artist \| albumartist ')"
#check for song change
if [ "$music_path" != "$(cat "$HOME"/.config/cmus/.current_song)" ]
then
  #update current_song
  printf "%s" "$music_path" > "$HOME"/.config/cmus/.current_song
  kill $(cat "$HOME"/.config/cmus/.pid)
  printf "%s" "$$" > "$HOME"/.config/cmus/.pid
  #clear last cover
  cmus-remote -C refresh
  #get lines & columns (bash -i crashes, tput lines doesnt work, checkwinsize doesnt work, resize crashes)
  term_size=$(stty size -F /dev/tty || printf "75 310")
  LINES=${term_size%% *}
  COLUMNS=${term_size##* }
  #if inital start
  if [ ! -f "$HOME"/.config/cmus/.runf ]
  then
    #get initial $WINDOW $HEIGHT $WIDTH
    eval "$(xdotool getactivewindow getwindowgeometry --shell)"
    #save $WINDOW to runf
    printf "%s" "$WINDOW" > "$HOME"/.config/cmus/.runf
  else
    #get $WINDOW $HEIGHT $WIDTH
    eval "$(xdotool getwindowgeometry --shell "$(cat "$HOME"/.config/cmus/.runf)")"
  fi
  #get sixel size
  six_size=$(("$HEIGHT" * "$six_mult / 100"))
  #extract sixel cover from flac using libsixel
  exiftool "$music_path" -Picture -b | img2sixel -h"$six_size" -p"$six_palette" > "$HOME"/.config/cmus/.cover.six
  #display sixel cover
  printf "\e[$((LINES - $((six_size / $((HEIGHT / LINES))))-x_offset));$((COLUMNS - $((six_size / $((WIDTH / COLUMNS))))-y_offset))H%s" "$(cat "$HOME"/.config/cmus/.cover.six)" >> /dev/tty &&
  exit 0
fi
