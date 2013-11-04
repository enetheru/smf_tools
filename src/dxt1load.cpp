#include <cstring>

#include "dxt1load.h"

unsigned char *
dxt1_load(unsigned char* data, int width, int height)
{
	unsigned char *out_d = new unsigned char[width * height * 4];
	memset( out_d, 0, width * height * 4 );

	int numblocks =  width * height / 16;

	for ( int i = 0; i < numblocks; i++ ) {

		unsigned short color0 = *(unsigned short *)&data[0];
		unsigned short color1 = *(unsigned short *)&data[2];

		int r0 = RED_RGB565( color0 ) << 3;
		int g0 = GREEN_RGB565( color0 ) << 2;
		int b0 = BLUE_RGB565( color0 ) << 3;

		int r1 = RED_RGB565( color1 ) << 3;
		int g1 = GREEN_RGB565( color1 ) << 2;
		int b1 = BLUE_RGB565( color1 ) << 3;

		unsigned int bits = *(unsigned int*)&data[4];

		for ( int a = 0; a < 4; a++ ) {
			for ( int b = 0; b < 4; b++ ) {
				// get pixel location in final image.
				int x = 4 * (i % (width / 4)) + b;
				int y = 4 * (i / (width / 4)) + a;
				int pixelno = (y * width + x) * 4;

				unsigned char code = bits & 0x3; // get code
				bits >>= 2; // shift for next code.

				if ( color0 > color1 ) { //no alpha channel
					if ( code == 0 ) {
						out_d[ pixelno + 0 ] = r0;
						out_d[ pixelno + 1 ] = g0;
						out_d[ pixelno + 2 ] = b0;
						out_d[ pixelno + 3 ] = 255;
					}
					else if ( code == 1 ) {
						out_d[ pixelno + 0 ] = r1;
						out_d[ pixelno + 1 ] = g1;
						out_d[ pixelno + 2 ] = b1;
						out_d[ pixelno + 3 ] = 255;
					}
					else if ( code == 2 ) {
						out_d[ pixelno + 0 ] = (r0 * 2 + r1) / 3;
						out_d[ pixelno + 1 ] = (g0 * 2 + g1) / 3;
						out_d[ pixelno + 2 ] = (b0 * 2 + b1) / 3;
						out_d[ pixelno + 3 ] = 255;
					} else {	
						out_d[ pixelno + 0 ] = (r0 + r1 * 2) / 3;
						out_d[ pixelno + 1 ] = (g0 + g1 * 2) / 3;
						out_d[ pixelno + 2 ] = (b0 + b1 * 2) / 3;
						out_d[ pixelno + 3 ] = 255;
					}
				} else { // with alpha bit
					if ( code == 0 ) {
						out_d[ pixelno + 0 ] = r0;
						out_d[ pixelno + 1 ] = g0;
						out_d[ pixelno + 2 ] = b0;
						out_d[ pixelno + 3 ] = 255;
					}
					else if ( code == 1 ) {
						out_d[ pixelno + 0 ] = r1;
						out_d[ pixelno + 1 ] = g1;
						out_d[ pixelno + 2 ] = b1;
						out_d[ pixelno + 3 ] = 255;
					}
					else if ( code == 2 ) {
						out_d[ pixelno + 0 ] = (r0 + r1) / 2;
						out_d[ pixelno + 1 ] = (g0 + g1) / 2;
						out_d[ pixelno + 2 ] = (b0 + b1) / 2;
						out_d[ pixelno + 3 ] = 255;
					} else {
						out_d[ pixelno + 0 ] = 0;
						out_d[ pixelno + 1 ] = 0;
						out_d[ pixelno + 2 ] = 0;
						out_d[ pixelno + 3 ] = 0;
					}
				}
			}
		}
		data += 8;
	}
	return out_d;
}
