#!/usr/bin/env bash

add-apt-repository -y ppa:irie/openimageio
apt-get update
apt-get dist-upgrade -y
apt-get build-dep -y openimageio
apt-get install libgif-dev libraw-dev
apt-get install -y git cmake

mkdir -p ~/git/OpenImageIO
cd ~/git/OpenImageIO
git clone https://github.com/OpenImageIO/oiio.git
cd oiio
for branch in `git branch -a | grep remotes | grep -v HEAD | grep -v master`; do
    git branch --track ${branch##*/} $branch
done
git pull --all
#git checkout RB-1.4
make -e INSTALLDIR=/usr dist_dir=

mkdir -p ~/git/enetheru
cd ~/git/enetheru
git clone https://github.com/enetheru/MapConv.git
mkdir MapConv-build
cd MapConv-build
cmake ../MapConv -DCMAKE_INSTALL_PREFIX:PATH=/usr
make
make install
