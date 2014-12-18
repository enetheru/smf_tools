#!/usr/bin/env bash

add-apt-repository --yes ppa:ubuntu-toolchain-r/test
add-apt-repository --yes ppa:boost-latest/ppa
add-apt-repository --yes ppa:irie/openimageio
apt-get update -qq
apt-get build-dep --yes openimageio

#FIXME force specific version of toolchain for consistency

su vagrant

mkdir -p ~/git/enetheru
cd ~/git/enetheru
git clone https://github.com/enetheru/smf_tools.git

mkdir -p ~/git/OpenImageIO
cd ~/git/OpenImageIO
git clone -b RB-1.4 https://github.com/OpenImageIO/oiio.git
cd oiio
git apply ~/git/enetheru/MapConv/oiio.patch
sudo make USE_QT=0 USE_OPENGL=0 USE_PYTHON=0 USE_FIELD3D=0 USE_OPENJPEG=0 USE_GIF=0 USE_OCIO=0 USE_OPENSSL=0 USE_LIBRAW=0 NOTHREADS=0 OIIO_BUILD_TOOLS=0 OIIO_BUILD_TESTS=0 -e dist_dir= INSTALLDIR=/usr

cd ~/git/enetheru/smf_tools
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX:PATH=/usr
make
sudo make install
