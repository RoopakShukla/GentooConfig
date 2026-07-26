#!/bin/sh
export PATH="$PATH:/usr/local/bin"

xrdb merge ~/.Xresources 
xbacklight -set 10 &
feh --bg-fill --randomize ~/.wallpapers/*
xset r rate 200 50 &
# picom &
"$HOME"/.config/picom/build/src/./picom -b --animations --animation-window-mass 0.5 --animation-for-open-window zoom --animation-stiffness-in-tag 200 --animation-stiffness-tag-change 200 --animation-dampening 10 &


#Helper for keyboard layout
setxkbmap -model pc105 -layout us,ru -option grp:alt_shift_toggle

dash ~/.config/chadwm/scripts/bar.sh &
while type chadwm >/dev/null; do chadwm && continue || break; done
