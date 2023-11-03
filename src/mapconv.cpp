/*
 * re-implementation of pymapconv using smf_tools.
 * the existing implementation that generates a qt gui appears to use a generic tool to create an interface from cli arguments.
 */

#include <spdlog/spdlog.h>
#include "option_args.h"
#include <fmt/core.h>

// TLMCPPOP
enum optionsIndex
{
    UNKNOWN,
    VERBOSE,
    QUIET,
    HELP,
    VERSION,
    MAP_NAME,
    TEXTURE,
    HEIGHT_MAP,
    METAL_MAP,
    NORMALIZE_METAL_MAP,
    MAXIMUM_HEIGHT,
    MINIMUM_HEIGHT,
    GEOVENT_DECAL,

//# {'-c', '--compress', help =  '<compression> How much we should try to compress the texture map. Values between [0;1] lower values make higher quality, larger files. [NOT IMPLEMENTED YET]',  default = 0.0, type = float )
//# {'-i', '--invert', help = 'Flip the height map image upside-down on reading.', default = False, action='store_true' )
//# {'-l', '--lowpass', help = '<int kernelsize> Smoothes the heightmap with a gaussian kernel size specified', default = 0, type=int)

    FEATURE_PLACEMENT_FILE,
    FEATURE_LIST_FILE,
    FEATURE_MAP,
    GRASS_MAP,
    TYPE_MAP,
    OVERRIDE_MINIMAP,
    MAPNORMALS,
    SPECULAR,
    SPLATDISTRIBUTION,
    MULTITHREAD,
    LINUX,
//# {'-s', '--justsmf', help = 'Just create smf file, dont make tile file (for quick recompilations)', default = 0, type=int)
    NVDXT,
    Dirty,
    DECOMPILE
//{'-s', '--skiptexture', help='|DECOMPILE| Skip generating the texture during decompilation', default = False, action = 'store_true')
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
"USAGE:\v  mapconv  \t -o <OUTFILE> -t <INTEX> -a <HEIGHTMAP> [-m METALMAP] [--normalizemetal NORMALIZEMETAL] [-x MAXHEIGHT] [-n MINHEIGHT] [-g GEOVENTFILE] [-k FEATUREPLACEMENT] [-j FEATURELIST]"
"[-f FEATUREMAP] [-r GRASSMAP] [-y TYPEMAP] [-p MINIMAP] [-l MAPNORMALS] [-z SPECULAR] [-w SPLATDISTRIBUTION] [-q NUMTHREADS] [-u] [-v NVDXT_OPTIONS]"
"[--highresheightmapfilter HIGHRESHEIGHTMAPFILTER] [-c] [-d DECOMPILE] [-s] [-v,]"},
    { {},{},"","", Arg::None , 0 }, // column formatting break
    { HELP,    0, "h", "help",    Arg::None,"  -h,  \t--help  \tPrint usage and exit."},
    { VERBOSE, 0, "v", "verbose", Arg::None,"  -v,  \t--verbose  \tMOAR output."},
    { QUIET,   0, "q", "quiet",   Arg::None,"  -q,  \t--quiet  \tSupress Output."},
    { VERSION, 0,  "", "version", Arg::None,"  -q,  \t--quiet  \tSupress Output."},
    { {},{},"","", Arg::None , 0 }, // column formatting break

    { MAP_NAME, 0, "o", "outfile", Arg::Required, "  -o,  \t--outfile <my_new_map.smf>"
"  \tThe name of the created map file. Should end in .smf. A tilefile (extension .smt) is also created, this name may contain spaces" },
    //default='my_new_map.smf' }

    { TEXTURE, 0, "t", "intex", Arg::Required, "  -t,  \t--intex <image filepath>"
"  \tInput bitmap to use for the map. Sizes must be multiples of 1024. Xsize and Ysize are determined from this file; xsize = intex width / 8, ysize = height / 8. Don't use Alpha unless you know what you are doing." },
    // default='', type=str)

    { HEIGHT_MAP, 0, "a", "heightmap", Arg::Required, "  -a,  \t--heightmap <image filepath>"
"  \tInput heightmap to use for the map, this should be 16 bit greyscale PNG image or a 16bit intel byte order single channel .raw image. Must be xsize*64+1 by ysize*64+1" },
    // default='', type=str)

    { METAL_MAP, 0, "m", "metalmap", Arg::Required, "  -m,  \t--metalmap <image filepath>"
"  \tMetal map to use, red channel is amount of metal. Resized to xsize / 2 by ysize / 2." },
    // type=str)

    { NORMALIZE_METAL_MAP, 0, "", "normalizemetal", Arg::Required, "  \t--normalizemetal <float>"
"  \tIf the metal map has noisy spots, specify how many percent of deviation from the median metal map value will be corrected to the median. E.g. 15.0 (%) will correct 1.7 and 2.3 to 2.0" },
    // default=0.0, type=float)

    { MAXIMUM_HEIGHT, 0, "x", "maxheight", Arg::Required, "  -x,  \t--maxheight <float>"
"  \tWhat altitude in spring the max(0xff for 8 bit images or 0xffff for 16bit images) level of the height map represents" },
    // default=100.0, type=float)

    { MINIMUM_HEIGHT, 0, "n", "minheight", Arg::Required, "  -n,  \t--minheight <float>"
"  \tWhat altitude in spring the minimum level (0) of the height map represents" },
    // default=-50.0, type=float)

    { GEOVENT_DECAL, 0, "g", "geoventfile", Arg::None, "  -g,  \t--geoventfile <image filePath>"
"  \t The decal for geothermal vents; appears on the compiled map at each vent. Custom geovent decals should use all white as transparent, clear this if you do not wish to have geovents drawn."},
    // default='./resources/geovent.bmp', type=str)

//# {'-c', '--compress', help =  '<compression> How much we should try to compress the texture map. Values between [0;1] lower values make higher quality, larger files. [NOT IMPLEMENTED YET]',  default = 0.0, type = float )
//# {'-i', '--invert', help = 'Flip the height map image upside-down on reading.', default = False, action='store_true' )
//# {'-l', '--lowpass', help = '<int kernelsize> Smoothes the heightmap with a gaussian kernel size specified', default = 0, type=int)

    { FEATURE_PLACEMENT_FILE, 0, "k", "featureplacement", Arg::Required, "  -k,  \t--featureplacement <featureplacement.lua>"
"  \tA feature placement text file defining the placement of each feature. (Default: fp.txt). See README.txt for details. The default format specifies it to have each line look like this: \n { name = \'agorm_talltree6\', x = 224, z = 3616, rot = '0' , scale = 1.0} \n the [scale] argument currently does nothing in the engine. "},
    // type=str)

    { FEATURE_LIST_FILE, 0, "j", "featurelist", Arg::Required, "  -j,  \t--featurelist <feature_list_file.txt>"
"  \tA file with the name of one feature on each line. Specifying a number from 32767 to -32768 next to the feature name will tell mapconv how much to rotate the feature. specifying -1 will rotate it randomly." },
    //type=str)

    { FEATURE_MAP, 0, "f", "featuremap", Arg::None, "  -f,  \t--featuremap <image filePath>"
"  \tFeature placement image, xsize by ysize. Green 255 pixels are geo vents, blue is grass, green 201-215 are engine default trees, red 255-0 each correspond to a line in --featurelist" },
    // type=str)

    { GRASS_MAP, 0, "r", "grassmap", Arg::None, "  -r,  \t--grassmap <grassmap.bmp>"
"  \tIf specified, will override the grass specified in the featuremap. Expects an xsize/4 x ysize/4 sized bitmap, all values that are not 0 will result in grass" },
    //type=str)

    { TYPE_MAP, 0, "y", "typemap", Arg::None, "  -y,  \t--typemap <typemap.bmp>"
"  \tType map to use, uses the red channel to define terrain type [0-255]. types are defined in the .smd, if this argument is skipped the entire map will TERRAINTYPE0"},
    // type=str)

    { OVERRIDE_MINIMAP, 0, "p", "minimap", Arg::None, "  -p,  \t--minimap <minimap.bmp>"
"  \tIf specified, will override generating a minimap from the texture file (intex) with the specified file. Must be 1024x1024 size." },
    //type=str)

    { MAPNORMALS, 0, "l", "mapnormals", Arg::None, "  -l,  \t--mapnormals <image filePath"
"  \tCompress and flip the map-wide normals to RGB .dds. Must be <= 16k * 16k sized (Windows only)" },
    // type=str)

    { SPECULAR, 0, "z", "specular", Arg::None, "  -z,  \t--specular <image filePath>"
"  \tCompress and flip the map-wide specular to RGBA .dds. Must be <= 16k * 16k sized (Windows only)" },
    // type=str)

    {SPLATDISTRIBUTION, 0, "w", "splatdistribution", Arg::None, "  -w,  \t--splatdistribution <image filePath>"
"  \tCompress and flip the splat distribution map to RGBA .dds. Must be <= 16k * 16k sized (Windows only)" },
    // type=str)

    { MULTITHREAD, 0, "q", "numthreads", Arg::None, "  -q,  \t--numthreads <integer>"
"  \tHow many threads to use for main compression job (default 4, Windows only)" },
    // default=4, type=int)

    {LINUX, 0, "u", "linux", Arg::None, "  -u,  \t--linux"
"  \tCheck this if you are running linux and wish to use AMD Compressonator instead of nvdxt.exe" },
    //default=(platform.system() == 'Linux'), action='store_true')

//# {'-s', '--justsmf', help = 'Just create smf file, dont make tile file (for quick recompilations)', default = 0, type=int)

    {NVDXT, 0, "v", "nvdxt_options", Arg::None, "  -v,  \t--nvdxt_options  \tcompression options " },
    //, default='-Sinc -quality_highest')

//        {'--highresheightmapfilter',
//        help='Which filter to use when downsampling highres heightmap: [lanczos, bilinear, nearest, median, histogram] ',
//        default="nearest", type = str)

    {Dirty, 0, "c", "dirty", Arg::None, "  -c,  \t--dirty  \tKeep temp directory after compilation" },
        //default=False, action='store_true')

        //# {'-q', '--quick', help='|FAST| Quick compilation (lower texture quality)', action='store_true') //not implemented yet

    {DECOMPILE, 0, "d", "decompile", Arg::None, "  -d,  \t--decompile  \tDecompiles a map to everything you need to recompile it"},

        // type=str)

//        {'-s', '--skiptexture', help='|DECOMPILE| Skip generating the texture during decompilation', default = False, action = 'store_true')
//        {'-v,', '--version', action='version', version=__VERSION__)
//        parser.description = 'Spring RTS SMF map compiler/decompiler by Beherith (mysterme@gmail.com). You must select at least a texture and a heightmap for compilation'
//        parser.epilog = 'Save your settings before exiting! Scroll down for more options!'
        { 0, 0, nullptr, nullptr, nullptr, nullptr }
};

static void shutdown( int code ){
    OIIO::shutdown();
    exit( code );
}

int
main( int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing
    // ==============
    bool fail = false;
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    auto* options = new option::Option[ stats.options_max ];
    auto* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );
    int term_columns = getenv("COLUMNS" ) ? atoi(getenv("COLUMNS" ) ) : 80;

    // No arguments
    if( argc == 0 ){
        option::printUsage(std::cout, usage, term_columns, 60, 80 );
        shutdown( 1 );
    }
    // Help Message
    if( options[ HELP ] ) {
        option::printUsage(std::cout, usage, term_columns, 60, 80);
        shutdown( 0 );
    }

    // setup logging level.
    spdlog::set_level(spdlog::level::warn);
    if( options[ VERBOSE ] )
        spdlog::set_level(spdlog::level::info);
    if( options[ QUIET ] )
        spdlog::set_level(spdlog::level::off);

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        SPDLOG_WARN( "Unknown option: {}", std::string( opt->name,opt->namelen ) );
        fail = true;
    }

    // non options
    if( parse.nonOptionsCount() == 0 ){
        SPDLOG_WARN( "Missing input filename" );
        fail = true;
    }

    for( int i = 1; i < parse.nonOptionsCount(); ++i ){
        SPDLOG_WARN( "Superfluous Argument: {}", parse.nonOption( i ) );
        fail = true;
    }

    if( fail || parse.error() ){
        SPDLOG_ERROR( "Options parsing" );
        shutdown( 1 );
    }
    // end of options parsing


    OIIO::shutdown();
    return 0;
}


/*
 * This is the default help output.
 * usage: pymapconv.py [-h] [-o OUTFILE] [-t INTEX] [-a HEIGHTMAP] [-m METALMAP] [--normalizemetal NORMALIZEMETAL] [-x MAXHEIGHT] [-n MINHEIGHT] [-g GEOVENTFILE] [-k FEATUREPLACEMENT] [-j FEATURELIST]
                    [-f FEATUREMAP] [-r GRASSMAP] [-y TYPEMAP] [-p MINIMAP] [-l MAPNORMALS] [-z SPECULAR] [-w SPLATDISTRIBUTION] [-q NUMTHREADS] [-u] [-v NVDXT_OPTIONS]
                    [--highresheightmapfilter HIGHRESHEIGHTMAPFILTER] [-c] [-d DECOMPILE] [-s] [-v,]
 */

/* // This appears to be a direct copy from the original mapconv, which pymapconv replaces.
 * '''
   -k <feature placement file>,  --featureplacement <feature placement
      file>
     (value required)  A special text file defining the placement of each
     feature. (Default: fp.txt). See README.txt for details. The default
     format specifies it to have each line look like this (spacing is
     strict)

     { name = 'agorm_talltree6', x = 224, z = 3616, rot = "0" }

   -j <feature list file>,  --featurelist <feature list file>
     (value required)  A file with the name of one feature on each line.
     (Default: fs.txt). See README.txt for details.

     Specifying a number from 32767 to -32768 next to the feature name will
     tell mapconv how much to rotate the feature. specifying -1 will rotate
     it randomly.

     Example line from fs.txt

     btreeblo_1 -1

     btreeblo 16000

   -f <featuremap image>,  --featuremap <featuremap image>
     (value required)  Feature placement file, xsize by ysize. See
     README.txt for details. Green 255 pixels are geo vents, blue is grass,
     green 201-215 is default trees, red 255-0 correspont each to a line in
     fs.txt.

   -r <randomrotate>,  --randomrotate <randomrotate>
     (value required)  rotate features randomly, the first r features in
     featurelist (fs.txt) get random rotation, default 0

   -s,  --justsmf
     Just create smf file, dont make smt

   -l,  --lowpass
     Lowpass filters the heightmap

   -q,  --use_nvcompress
     Use NVCOMPRESS.EXE tool for ultra fast CUDA compression. Needs Geforce
     8 or higher nvidia card.

   -i,  --invert
     Flip the height map image upside-down on reading.

   -z <texcompress program>,  --texcompress <texcompress program>
     (value required)  Name of companion program texcompress from current
     working directory.

   -c <compression>,  --compress <compression>
     (value required)  How much we should try to compress the texture map.
     Default 0.8, lower -> higher quality, larger files.

   -x <max height>,  --maxheight <max height>
     (required)  (value required)  What altitude in spring the max(0xff or
     0xffff) level of the height map represents.

   -n <min height>,  --minheight <min height>
     (required)  (value required)  What altitude in spring the min(0) level
     of the height map represents.

   -o <output .smf>,  --outfile <output .smf>
     (value required)  The name of the created map file. Should end in
     .smf. A tilefile (extension .smt) is also created.

   -e <tile file>,  --externaltilefile <tile file>
     (value required)  External tile file that will be used for finding
     tiles. Tiles not found in this will be saved in a new tile file.

   -g <Geovent image>,  --geoventfile <Geovent image>
     (value required)  The decal for geothermal vents; appears on the
     compiled map at each vent. (Default: geovent.bmp).

   -y <typemap image>,  --typemap <typemap image>
     (value required)  Type map to use, uses the red channel to decide
     type. types are defined in the .smd, if this argument is skipped the
     map will be all type 0

   -m <metalmap image>,  --metalmap <metalmap image>
     (required)  (value required)  Metal map to use, red channel is amount
     of metal. Resized to xsize / 2 by ysize / 2.

   -a <heightmap file>,  --heightmap <heightmap file>
     (required)  (value required)  Input heightmap to use for the map, this
     should be in 16 bit raw format (.raw extension) or an image file. Must
     be xsize*64+1 by ysize*64+1.

   -t <texturemap file>,  --intex <texturemap file>
     (required)  (value required)  Input bitmap to use for the map. Sides
     must be multiple of 1024 long. xsize, ysize determined from this file:
     xsize = intex width / 8, ysize = height / 8.

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   -v,  --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.


   Converts a series of image files to a Spring map. This just creates the
   .smf and .smt files. You also need to write a .smd file using a text
   editor.'''
 */

/* This is the python argument parser code from pymapconv
	parser = argparse.ArgumentParser()
	parser.add_argument('-o', '--outfile',
						help='|MAP NAME| <output mapname.smf> (required) The name of the created map file. Should end in .smf. A tilefile (extension .smt) is also created, this name may contain spaces',
						default='my_new_map.smf', type=str)

	parser.add_argument('-t', '--intex',
						help='|TEXTURE| <texturemap.bmp> (required) Input bitmap to use for the map. Sizes must be multiples of 1024. Xsize and Ysize are determined from this file; xsize = intex width / 8, ysize = height / 8. Don\'t use Alpha unless you know what you are doing.',
						default='', type=str)
	parser.add_argument('-a', '--heightmap',
						help='|HEIGHT MAP| <heightmap file> (required) Input heightmap to use for the map, this should be 16 bit greyscale PNG image or a 16bit intel byte order single channel .raw image. Must be xsize*64+1 by ysize*64+1',
						default='', type=str)
	parser.add_argument('-m', '--metalmap',
						help='|METAL MAP| <metalmap.bmp> Metal map to use, red channel is amount of metal. Resized to xsize / 2 by ysize / 2.',
						type=str)
	parser.add_argument('--normalizemetal',
						help='|NORMALIZE METAL MAP| <percent spread> (optional) If the metal map has noisy spots, specify how many percent of deviation from the median metal map value will be corrected to the median. E.g. 15.0 (%) will correct 1.7 and 2.3 to 2.0',
						default=0.0, type=float)
	parser.add_argument('-x', '--maxheight',
						help='|MAXIMUM HEIGHT| <max height> (required) What altitude in spring the max(0xff for 8 bit images or 0xffff for 16bit images) level of the height map represents',
						default=100.0, type=float)
	parser.add_argument('-n', '--minheight',
						help='|MINIMUM HEIGHT| <min height> (required) What altitude in spring the minimum level (0) of the height map represents',
						default=-50.0, type=float)
	parser.add_argument('-g', '--geoventfile',
						help='|GEOVENT DECAL| <geovent.bmp> The decal for geothermal vents; appears on the compiled map at each vent. Custom geovent decals should use all white as transparent, clear this if you do not wish to have geovents drawn.',
						default='./resources/geovent.bmp', type=str)
	# parser.add_argument('-c', '--compress', help =  '<compression> How much we should try to compress the texture map. Values between [0;1] lower values make higher quality, larger files. [NOT IMPLEMENTED YET]',  default = 0.0, type = float )

	# parser.add_argument('-i', '--invert', help = 'Flip the height map image upside-down on reading.', default = False, action='store_true' )
	# parser.add_argument('-l', '--lowpass', help = '<int kernelsize> Smoothes the heightmap with a gaussian kernel size specified', default = 0, type=int)


	parser.add_argument('-k', '--featureplacement',
						help='|FEATURE PLACEMENT FILE| <featureplacement.lua> A feature placement text file defining the placement of each feature. (Default: fp.txt). See README.txt for details. The default format specifies it to have each line look like this: \n { name = \'agorm_talltree6\', x = 224, z = 3616, rot = "0" , scale = 1.0} \n the [scale] argument currently does nothing in the engine. ',
						type=str)
	parser.add_argument('-j', '--featurelist',
						help='|FEATURE LIST FILE| <feature_list_file.txt> (required if featuremap image is specified) A file with the name of one feature on each line. Specifying a number from 32767 to -32768 next to the feature name will tell mapconv how much to rotate the feature. specifying -1 will rotate it randomly.',
						type=str)
	parser.add_argument('-f', '--featuremap',
						help='|FEATURE MAP| <featuremap.bmp> Feature placement image, xsize by ysize. Green 255 pixels are geo vents, blue is grass, green 201-215 are engine default trees, red 255-0 each correspond to a line in --featurelist',
						type=str)
	parser.add_argument('-r', '--grassmap',
						help='|GRASS MAP| <grassmap.bmp> If specified, will override the grass specified in the featuremap. Expects an xsize/4 x ysize/4 sized bitmap, all values that are not 0 will result in grass',
						type=str)
	parser.add_argument('-y', '--typemap',
						help='|TYPE MAP| <typemap.bmp> Type map to use, uses the red channel to define terrain type [0-255]. types are defined in the .smd, if this argument is skipped the entire map will TERRAINTYPE0',
						type=str)
	parser.add_argument('-p', '--minimap',
						help='|OVERRIDE MINIMAP| <minimap.bmp> If specified, will override generating a minimap from the texture file (intex) with the specified file. Must be 1024x1024 size.',
						type=str)

	parser.add_argument('-l', '--mapnormals',
						help='|MAPNORMALS| Compress and flip the map-wide normals to RGB .dds. Must be <= 16k * 16k sized (Windows only)',
						type=str)

	parser.add_argument('-z', '--specular',
						help='|SPECULAR| Compress and flip the map-wide specular to RGBA .dds. Must be <= 16k * 16k sized (Windows only)',
						type=str)

	parser.add_argument('-w', '--splatdistribution',
						help='|SPLATDISTRIBUTION| Compress and flip the splat distribution map to RGBA .dds. Must be <= 16k * 16k sized (Windows only)',
						type=str)

	parser.add_argument('-q', '--numthreads',
						help='|MULTITHREAD| <numthreads> How many threads to use for main compression job (default 4, Windows only)',
						default=4, type=int)

	parser.add_argument('-u', '--linux',
						help='|LINUX| Check this if you are running linux and wish to use AMD Compressonator instead of nvdxt.exe',
						default=(platform.system() == 'Linux'), action='store_true')

	# parser.add_argument('-s', '--justsmf', help = 'Just create smf file, dont make tile file (for quick recompilations)', default = 0, type=int)
	parser.add_argument('-v', '--nvdxt_options', help='|NVDXT| compression options ', default='-Sinc -quality_highest')

	parser.add_argument('--highresheightmapfilter',
						help='Which filter to use when downsampling highres heightmap: [lanczos, bilinear, nearest, median, histogram] ',
						default="nearest", type = str)

	parser.add_argument('-c', '--dirty',
						help='|Dirty| Keep temp directory after compilation',
						default=False, action='store_true')
	# parser.add_argument('-q', '--quick', help='|FAST| Quick compilation (lower texture quality)', action='store_true') //not implemented yet

	parser.add_argument('-d', '--decompile', help='|DECOMPILE| Decompiles a map to everything you need to recompile it', type=str)
	parser.add_argument('-s', '--skiptexture', help='|DECOMPILE| Skip generating the texture during decompilation', default = False, action = 'store_true')
	parser.add_argument('-v,', '--version', action='version', version=__VERSION__)
	parser.description = 'Spring RTS SMF map compiler/decompiler by Beherith (mysterme@gmail.com). You must select at least a texture and a heightmap for compilation'
	parser.epilog = 'Save your settings before exiting! Scroll down for more options!'

 */