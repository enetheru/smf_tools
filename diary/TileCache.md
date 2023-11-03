The tile cache holds references to the source of the tiles, an image, or smt file, and which tiles it represents.

## Future Plans
* TODO don't resize tiles in the tilecache, make that an external problem, tilecache should just spit out tiles.


### TileCache Sources
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