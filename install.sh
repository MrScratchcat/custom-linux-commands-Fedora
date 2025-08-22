#!/bin/bash
if command -v git &> /dev/null; then
    echo "Git is already installed"
else
    echo "Git is not installed. Installing..."
    sudo dnf install git -y
fi
if command -v flatpak &> /dev/null
then
    echo "flatpak is installed"
    flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
else
    sudo dnf install flatpak -y
    flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
fi
git clone https://github.com/MrScratchcat/custom-linux-commands-Fedora
cd custom-linux-commands-Fedora
chmod +x *
sudo cp * /bin/
sudo chmod +x /bin/*
cd ..
rm -rf custom-linux-commands-Fedora