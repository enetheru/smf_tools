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

<pre>USAGE: tileconv [options] [source files] 
  eg. 'tileconv -v --mapsize 16x8 -o mysmt.smt diffuse.jpg'

GENERAL OPTIONS:
  -h,  --help                     Print usage and exit.
  -v,  --verbose                  Print extra information.
  -q,  --quiet                    Supress output.
  -f,  --force                    overwrite existing files.

SPECIFICATIONS:
       --mapsize=XxY              Width and length of map, in spring map units
                                  eg. '--mapsize=4x4', must be multiples of two.
       --tileres=X                XY resolution of tiles to save, eg.
                                  '--tileres=32'.
       --imagesize=XxY            Scale the resultant extraction to this size,
                                  eg. '--imagesize=1024x768'.
       --stride=N                 Number of source tiles horizontally before
                                  wrapping.

CREATION:
       --filter=[1,2,n,1-n]       Append only these tiles
  -o,  --output=filename.smt      filename to output.

COMPRESSION OPTIONS:
       --slow_dxt1                Use slower but better analytics when
                                  compressing DXT1 textures
       --cnum=[-1,0,N]            Number of tiles to compare; n=-1, no
                                  comparison; n=0, hashtable exact comparison; n
                                  > 0, numeric comparison between n tiles
       --cpet                     Pixel error threshold. 0.0f-1.0f
       --cnet=[0-N]               Errors threshold 0-1024.

DECONSTRUCTION OPTIONS:
       --separate                 Split the source files into individual images.
       --collate                  Collate the extracted tiles.
       --reconstruct=tilemap.exr  Reconstruct the extracted tiles using a
                                  tilemap.

EXAMPLES:
  tileconv -v --mapsize 8x8 -o mysmt.smt segment_{0..7}_{0..7}.jpg
  tileconv -v --reconstruct tilemap.exr --imagesize 1024x1024 source.smt

</pre>
