I need an overview, like a TOC for me to properly organise my thoughts.
* What are you reading?
* How things appear to work
	* File Formats
		* SMF
		* SMT
	* Code Review
* Why does this annoy me?
* What do I think they should do?
	* Drop SMF, completely unnecessary.
	* Either drop, or embrace SMT

I would like to get this thing out of my head and onto paper
because I would like to let it go, I don't want to think about it anymore.

I started thinking about this ten years ago when I was at a dead end job doing barely anything, wanting to get into open source game development, I found a project I liked and wanted to contribute, so I started editing the wiki and making things easier for myself to understand.

During the process I came to understand the file formats, and how it is currently being used, and my brain doing what my brain does, thinking within the limits of what is currently possible how it might be utilised. 

I just want it to stop, I want to let it go.

I have to explain firstly what it is that I am looking at, which is maps for a game engine known as springrts, and recently forked as recoil, it powers the beyond all reason game, which I enjoy playing sometimes, and enjoy watching commentary on. I've always been a big fan of total annihilation.

Anyway, the map format has evolved quite a bit, but the core of it remains this old chunk of binary formats which make no sense to maintain anymore, and I am surprised it still exists.

There are two binary formats which I am concerned with, the rest of the lua code, and the pictures, which feed into the shaders which make maps so much prettier than basic colour I am not overly concerned with. 

The two files are the SMF and the SMT file.
The SMF file is a very simple format, though it has the potential to be more, it just isn't because nobody updates the core engine code with additions. either because its entirely unnecessary to keep using, in which case why keep it around, or because, fuck I don't know. its just dumb.

the smf file holds basic data about the map, and contains some image data for other bits and pieces.

the smt file contains thousands of dxt1 compressed blocks, for use in reconstruction of an image file, again dumb, because its entirely unused .

the way the code is organised, it loads up the information from the struct, pulls the relevant images, and then re-builds 1024x1024 textures from the tiles based on the tilemap.

In reality, none of this is actually done for any good reason other than to eat up cycles, the textures used in maps are usually unique, generated in worldmachine, there is very little reason to use a tile based approach, especially one that uses 32x32 pixel tiles, its like from the dark ages.

But there is potential in there for something more, if it were possible to get someone to actually work on it.

Making the map refer to both height and texture would be a good start, being able to compile them separately, etc. but its all moot, its just not how its done anymore.

I would prefer if they dump the old map format, have everything in lua or json, reference actual image files that can be easily identified, it would make it much more flexible into the future.
Compilation of the images can be done offline for anything.

The only reason to maintain a tile based format would be if they had any plans for procedural generation of maps, but I have never seen any indication of this.

blah, rant is ranty.