cmus_sixel
==========

![demo](https://github.com/user-attachments/assets/fe159771-f041-4ab3-87ba-6a38fde2d331)

## About
cmus status display program that displays the cover of the current playing song using libsixel or kitty.

## Requirements
- Terminal with [Sixel](https://www.arewesixelyet.com) or [Kitty](https://sw.kovidgoyal.net/kitty/graphics-protocol/) support
- cmus

## Dependencies
- ffmpeg
- libsixel

## Installation

### 1. Install Dependencies

#### Debian/Ubuntu: 
```bash
sudo apt install gcc libsixel-dev libavformat-dev libavutil-dev cmus pkg-config
```

#### Arch Linux:
```bash
sudo pacman -S gcc libsixel ffmpeg cmus pkgconf
```

### 2. Build from Source
```bash
git clone https://github.com/S22F5/cmus_sixel.git && cd cmus_sixel
```
```bash
gcc -O3 main.c -o cmus_sixel $(pkg-config --libs libsixel libavformat libavcodec libswscale libavutil)
```

### 3. Install
```bash
cp cmus_sixel cmus_sixel.conf "$XDG_CONFIG_HOME/cmus/"
```

### 4. Enable in cmus
```bash
cmus-remote -C "set status_display_program=$XDG_CONFIG_HOME/cmus/cmus_sixel"
```
