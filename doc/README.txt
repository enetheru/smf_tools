MapConv.exe:

As of Mar 2006:

* Images can be any format DevIL supports. 
  That should include every image format you've used before, 
  and adds, say, PNG. (No more huge BMPs!)

* It shouldn't go berserk about geothermal vents.

Texture map:
=============

Size: A multiple of 1024 by a (possibly different) multiple of 1024.

The sizes of the other images are based on the size of the texture
map:

 xsize = Texture map width / 8
 ysize = Texture map height / 8

Metal map:
===========

Size: xsize / 2 by ysize / 2. 

Red: Metal
(Greyscale works correctly.)

Height map:
============

Size: exactly (xsize + 1) by (ysize + 1).

1. Ordinary image

Red: Height. 

2. '.raw'

The file is assumed to have (xsize + 1) * (ysize + 1) *pairs* of octets in
it.

Feature map: 
============

Size: xsize by ysize.

Green:
255 	- geovent
200-215 - trees (types 1-16)

Blue:
0-255   - no grass -> full grass

Red:
255	- feature of type on first  line of fs.txt
254	- feature of type on second line of fs.txt
253     - ...

ie.  fs.txt has armjeth_dead on the first line, so a pixel of 255 red places an armjeth_dead !

Format is one feature type per line. 
These features are read from the .tdf files in the mods/<modname.sd7>/features filesystem.

Type map:
==========

Size: xsize / 2 by ysize / 2

Red: Terrain type.


Good luck and have fun!
