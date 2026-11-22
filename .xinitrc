#!/bin/sh

feh --bg-fill ~/wallpapers/flowers.png
xset r rate 200 50 &

pipewire &
sleep 1
wireplumber &
pipewire-pulse &

/usr/libexec/xfce-polkit &

exec dbus-run-session bspwm
