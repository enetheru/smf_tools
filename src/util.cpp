#include "util.h"

#include <fstream>
#include <OpenImageIO/imageio.h>

OIIO_NAMESPACE_USING
using namespace std;

bool
isSMT( string FN )
{
    bool status = false;
    char magic[ 16 ];
    if(! FN.compare( "" ) ) return false;

    ifstream file( FN, ifstream::in );
    if( file.good() ){
        file.read( magic, 16 );
        if(! strcmp( magic, "spring tilefile" ) ) status = true;
    }

    file.close();
    return status;
}

bool
isSMF( string FN )
{
    bool status = false;
    char magic[ 16 ];
    if(! FN.compare( "" ) ) return false;

    ifstream file( FN, ifstream::in );
    if( file.good() ){
        file.read( magic, 16 );
        if(! strcmp( magic, "spring map file" ) ) status = true;
    }

    file.close();
    return status;
}

bool
isImage( string FN )
{
    ImageInput *in = ImageInput::open( FN );
    if(! in ) return false;

    in->close();
    return true;
}
