The TileMap holds an array of indexes to tiles. It is defined as two dimensional, thought the data is unidimensional. This is just a nice convenience to getting indexes using 2d coordinates.

### TODO
There is no reason this cant be a sparse object, holding only references that it has. allowing fully transparent, or placeholder sections of the image to exist. But it's not entirely important as in reality most images are fully filled.