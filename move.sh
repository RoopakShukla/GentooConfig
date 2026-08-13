#!/bin/sh

echo "Okkaayyyyy lets go"

mv .vim ~/.config/.vim
mv bfetch ~/.config/bfetch
mv dunst ~/.config/dunst 
mv suckless ~/.config/suckless
mv fastfetch ~/.config/fastfetch 
mv nvim ~/.config/nvim 
mv rofi ~/.config/rofi 

mv .xinitrc ~/.xinitrc
mv .vimrc ~/.vimrc 


echo "Yayyyy, we done moved everything successfully"
sleep 1
echo "Now you just have to go and build dwm, slstatus & st"
sleep 0.5
echo "(im not doing allat.)"
