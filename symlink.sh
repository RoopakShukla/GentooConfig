#!/bin/sh

echo "Okayyyy letss gooo"

ln -sf "$HOME/GentooConfig/helix" "$HOME/.config/helix"
ln -sf "$HOME/GentooConfig/tmux" "$HOME/.config/tmux"
ln -sf "$HOME/GentooConfig/.xinitrc" "$HOME/.xinitrc"
ln -sf "$HOME/GentooConfig/st" "$HOME/.config/st"
ln -sf "$HOME/GentooConfig/wallpapers" "$HOME/wallpapers"

ln -sf "$HOME/GentooConfig/vis" "$HOME/.config/vis"

ln -sf "$HOME/GentooConfig/bspwm/bspwm" "$HOME/.config/bspwm"
ln -sf "$HOME/GentooConfig/bspwm/sxhkd" "$HOME/.config/sxhkd"
ln -sf "$HOME/GentooConfig/polybar" "$HOME/.config/polybar"

echo "All Done :D"

