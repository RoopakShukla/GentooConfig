#!/bin/sh

feh --bg-fill ~/wallpapers/void.png
xset r rate 200 50 &

exec bspwm
