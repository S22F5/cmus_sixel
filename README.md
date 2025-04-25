cmus_sixel
==========

![demo](https://github.com/user-attachments/assets/fe159771-f041-4ab3-87ba-6a38fde2d331)

## About
cmus status display program that displays the cover of the current playing song using libsixel

## Installation
1. compile using ```gcc -O3 main.c -o cmus_sixel $(libsixel-config --libs) -lX11 -lavformat -lavutil```
2. copy **cmus_sixel** and **cmus_sixel.conf** to **/home/USERNAME/.config/cmus/**
3. run ```:set status_display_program=/home/USERNAME/.config/cmus/cmus_sixel``` inside cmus

## Requirements
- ffmpeg
- libsixel
