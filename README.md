cmus_sixel
==========

![demo](https://github.com/user-attachments/assets/fe159771-f041-4ab3-87ba-6a38fde2d331)

## About
cmus status display program that displays the cover of the current playing song using libsixel.

## Requirements
- Terminal with [Sixel support](https://www.arewesixelyet.com)
- cmus
- ffmpeg
- libsixel

## Installation

### 1. Install Dependencies

#### Debian/Ubuntu: 
```bash
sudo apt install libsixel-dev libavformat-dev libavutil-dev cmus
```

#### Arch Linux:
```bash
sudo pacman -S libsixel ffmpeg cmus
```

### 2. Build from Source
```bash
git clone https://github.com/S22F5/cmus_sixel.git && cd cmus_sixel
gcc -O3 main.c -o cmus_sixel $(pkg-config --libs libsixel libavformat libavutil)
```

### 3. Install
```bash
cp cmus_sixel cmus_sixel.conf "$XDG_CONFIG_HOME/cmus/"
```

### 4. Enable in cmus
```bash
cmus-remote -C "set status_display_program=$XDG_CONFIG_HOME/cmus/cmus_sixel"
```
