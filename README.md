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
Tested on linux mint 17
<pre>sudo add-apt-repository ppa:irie/openimageio
sudo apt-get install libopenimageio-dev libnvtt-dev libboost-dev libboost-system-dev
git clone https://github.com/enetheru/MapConv.git
mkdir mapconv-build
cd mapconv-build
cmake ../MapConv
make</pre>

Programs
========
<pre>USAGE: mapconv [options]

OPTIONS:
  -h,  --help       Print usage and exit.
  -v,  --verbose    Print extra information.
  -q,  --quiet      Supress output.
  -i,  --input      SMF filename to load.
  -o,  --output     Output prefix used when saving.
  -e,  --extract    Extract images from loaded SMF.
       --slow_dxt1  Use slower but better analytics when compressing DXT1
                    textures
  -x,  --width      Width of the map in spring map units, Must be multiples of
                    two.
  -z,  --length     Length of the map in spring map units, Must be multiples of
                    two.
  -y,  --floor      Minimum height of the map.
  -Y,  --ceiling    Maximum height of the map.
       --smt        list of SMT files referenced.
       --tilemap    Image to use for tilemap.
       --height     Image to use for heightmap.
       --invert     Invert the heightmap.
       --type       Image to use for typemap.
       --mini       Image to use for minimap.
       --metal      Image to use for metalmap.
       --grass      Image to use for grassmap.
       --features   List of features.

EXAMPLES:
  mapconv -x 8 -z 8 -y -10 -Y 256 --height height.tif --metal metal.png -smts
tiles.smt -tilemap tilemap.tif -o mymap.smf
  mapconv -i oldmap.smf -o prefix
</pre>

<pre>USAGE: tileconv [options]

OPTIONS:
  -h,  --help       Print usage and exit.
  -v,  --verbose    Print extra information.
  -q,  --quiet      Supress output.
  -i,  --input      SMT filename to load.
  -o,  --output     Output prefix used when saving.
       --extract    Extract images from loaded SMT.
       --slow_dxt1  Use slower but better analytics when compressing DXT1
                    textures
  -x,  --width      Width of the map in spring map units, Must be multiples of
                    two.
  -z,  --length     Length of the map in spring map units, Must be multiples of
                    two.
       --cnum       Number of tiles to compare; n=-1, no comparison; n=0,
                    hashtable exact comparison; n > 0, numeric comparison
                    between n tiles
       --cpet       Pixel error threshold. 0.0f-1.0f
       --cnet       Errors threshold 0-1024.
       --tilemap    Image to use for tilemap.
       --stride     Number of image horizontally.
       --decals     File to parse when pasting decals.
       --sources    Source files to use for tiles

EXAMPLES:
  tileconv ...
</pre>
