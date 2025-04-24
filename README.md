cmus_sixel
==========

https://github.com/S22F5/cmus_sixel/assets/51321684/8a04ddd2-9b5b-4c59-bbe9-4c02fa97ce24

## About
cmus status display program that displays the cover of the current playing song using libsixel

## Installation
1. compile using ```gcc -O3 main.c -o cmus_sixel $(libsixel-config --libs) -lX11  -lavformat```
2. copy **cmus_sixel** and **cmus_sixel.conf** to /home/USERNAME/.config/cmus/
3. run ```:set status_display_program=/home/USERNAME/.config/cmus/cmus_sixel``` inside cmus

## Requirements
- ffmpeg
- libsixel
