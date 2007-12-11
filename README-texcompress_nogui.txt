== what is this good for ==
 if you plan to do script based batch processing of maps on linux
 this modified tool is very handy because it doesn't relay on X
 or any 3d graphic card for mipmapping and compression in a way
 that all graphic vendors support.

== requirements ==
 you need to install libtxc_dxtn.so into /lib
 libtxc can be found in the codebase of the DRI project 

== how to use ==
 simply use texcompress_nogui instead of texcompress

== example ==
 assuming you use the samples:

 modify the Makefile:
    MAPCONV=../../mapconv
    TEXCOMP=../../texcompress_nogui

 assuming you call mapconv directly use:
   -z <texcompress program> 
 (same as you would use normal texcompress, just call the other binary)

have fun!
 Joachim
