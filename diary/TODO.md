* TODO Benchmark against existing tools - pymapconv
* TODO make OpenImageIO file readers and writers for the SMT and SMF format's
* TODO modernise the code
* TODO re-work the logic
* TODO Implement libfmt formatters for the classes
* TODO Figure out how to get a hash of a pixel region, without a copy - easy, openimageio provides a func for it.
	* `std::string OIIO::ImageBufAlgo::computePixelHashSHA1(const ImageBuf &src, string_view extrainfo = "", [ROI](https://openimageio.readthedocs.io/en/latest/imageioapi.html#_CPPv4N4OIIO3ROIE "OIIO::ROI") roi = {}, int blocksize = 0, int nthreads = 0)`
* TODO Test all the public interfaces
* TODO Pessimise the code

## Testing
TODO: Make sure all tools  output 1 on any errors, eg.
* `14: [error] filemap.cpp:14:addBlock | 'map' clashes with existing block 'eof'`
## Error Accumulator
I've had this idea about adding a sort of error accumulator which holds error messages inside objects for things that are technically OK, but break the logic of things without being showstopping. An object I can just add to a class and can catch errors for use later.

OpenImageIO has this sort of thing, operations can fail, and it adds a message to the error, and uses a `has_error()` function, and `getError()` function 

Image Reconstruction
* Extracting all tiles from an smt
* Reconstructing an image from smt's and a map
* Reconstructing an image from smf, which references smt's