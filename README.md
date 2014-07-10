MapConv
=======
A cross platform SMF and SMT compiler for SpringRTS http://www.springrts.com

Dependencies
============
The Lean Mean C++ Option Parser<br>
http://optionparser.sourceforge.net/index.html

OpenImageIO 1.4<br>
https://sites.google.com/site/openimageio/home<br>

NVTT 2.0<br>
http://code.google.com/p/nvidia-texture-tools/

Boost<br>
Boost_system<br>

Building
========

### Tested on linux mint 17

    # sudo add-apt-repository ppa:irie/openimageio
    # sudo apt-get install libopenimageio-dev libnvtt-dev libboost-dev libboost-system-dev
    # git clone https://github.com/enetheru/MapConv.git
    # mkdir mapconv-build
    # cd mapconv-build
    # cmake ../MapConv
    # make
    
### Windows 7 with Msys2
 * https://github.com/enetheru/MapConv/wiki/Building-with-msys2
