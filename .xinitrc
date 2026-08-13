#!/bin/sh

slstatus &
feh --bg-fill ~/.config/suckless/dwm/wallpapers/main.png
xset r rate 200 50 &

exec dwm
