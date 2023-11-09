The tile cache holds references to the source of the tiles, an image, or smt file, and which tiles it represents.
The Tilecache is simply a retrieval mechanism, to fetch tiles based on indexes found in the TileMap.
### TileCache Sources
Tiles within a single source are assumed to have a homogeneous specification. i.e. width, height, channels, depth, compression etc. Fetching a tile fetches the original, which is then upto the receiver to resize and manipulate.
#### SMT Files
smt files are a very simple file format, specifying how many tiles they hold, the tile sizes and the compression format. They have only ever been implemented with DXT1 compression which is a fixed size for each tile, making random access easy.
#### Image Files
Image files can either hold a single tile per file, or alternatively, hold many tiles in the same image.

In the case of holding many tiles, more information is needed to be provided to properly extract the individual tiles, like:
* Tile sizes, width and height
* Tile amounts x and y
* Borders
* Overlap
### API
Is very simple: 
* addSource(...)
* getTile(...)