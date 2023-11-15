The purpose of smt_convert is:
* Take an image file and convert it into an tilefile.smt file and a tilemap.csv
* Take an image file and split it into tiles
* take a series of image files and turn them into tiles.
* Create an image using tile sources, and an optional tilemap

Creation of a tile_file.smt from an image file, or set of image files.

I can see myself getting confused with how things are structured, can i mix tiles from images? can I use an image file as if it was multiple tiles? I am unsure as I cant remember how I implemented the code.

Either way the tool is a mess, it doesn't do what it intends with any clarity.

I would rather have an image-to-smt, and an smt-to-image tool.

A problem I see is that the smt convert should be able to take a bunch of tile files, each with their own properties separated, the tilecache can spit out the originals.