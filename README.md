MapConv
=======
A cross platform SMF and SMT compiler for SpringRTS http://www.springrts.com

Dependencies
============
Tclap 1.2.1<br>
http://tclap.sourceforge.net/

OpenImageIO 1.2<br>
https://sites.google.com/site/openimageio/home

NVTT 2.0<br>
http://code.google.com/p/nvidia-texture-tools/

Programs
========
<pre>
tileconv [options] image1 ... imageN

Options
 -v --verbose, display extra output
 -q --quiet, supress standard output
 -e --extract, extract tiles from loaded smt
    --slow_dxt1, compress slowly but more accurately
    --cnum <int>, n = -1 no comparison, n = 0 hashtable comparison, n > 0 = tiles to compare to
    --cpet <float>, pixel error threshold 0.0f-1.0f
    --cnet <int>, number of errors threshold
 -i --input <string>, smt to load
 -o --output <string>, prefix for output
 -t --tilemap <string>, 
 -s --stride <int>, number of images horizontally
 -w --width <int>
 -l --length <int>
 -d --decals <.csv>, Decal file with format "filename,x,z"
</pre>
<pre>
mapconv [options]

Options
 -l --load <string>, smf file to load
 -s --save <string>, prefix for output
 -v --verbose, Display extra output
 -q --quiet, Supress standard output
 -d --decompile, Decompile loaded map
 -c --slowcomp, better quality but slower
 -x --width <int>, Width in spring map units
 -z --length <int>, Length in spring map units
 -y --floor <float>, Height of the map in spring map units
 -Y --ceiling <float>, Depth of the map in spring map units
 -a --add-smt <string>
 -i --tileindex <image>
 -e --heightmap <image>
    --lowpass, whether to filter the heightmap with a lowpass filter
    --invert, whether to invert the heightmap values
 -t --typemap <image>
 -m --minimap <image>
 -r --metalmap <image> 
 -g --grassmap <image>
 -f --features <text>
</pre>
