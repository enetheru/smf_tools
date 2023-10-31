* make OpenImageIO file readers and writers for the SMT and SMF format's
* modernise the code
* re-work the logic
* Implement libfmt formatters for the classes
* Benchmark against existing tools - pymapconv
* Figure out how to get a hash of a pixel region, without a copy - easy, openimageio provides a func for it.
	* `std::string OIIO::ImageBufAlgo::computePixelHashSHA1(const ImageBuf &src, string_view extrainfo = "", [ROI](https://openimageio.readthedocs.io/en/latest/imageioapi.html#_CPPv4N4OIIO3ROIE "OIIO::ROI") roi = {}, int blocksize = 0, int nthreads = 0)`
* Test all the public interfaces
* Pessimise the code