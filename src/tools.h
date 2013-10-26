#ifndef __TOOLS_H
#define __TOOLS_H
template <class T> T *
map_channels(T *in_d, //input data
	int width, int length, // input width, length
   	int in_c, int out_c, // channels in,out
   	int *map, int *fill) // channel map, fill values
{
	T *out_d = new T[width * length * out_c];

	for (int y = 0; y < length; ++y ) // rows
		for ( int x = 0; x < width; ++x ) // columns
			for ( int c = 0; c < out_c; ++c ) { // output channels
				if( map[ c ] >= in_c )
					out_d[ (y * width + x) * out_c + c ] = fill[ c ];
				else
					out_d[ (y * width + x) * out_c + c ] =
						in_d[ (y * width + x) * in_c + map[ c ] ];
	}

	return out_d;
}

template <class T> T *
interpolate_nearest(T *in_d, //input data
	int in_w, int in_l, int chans, // input width, length, channels
	int out_w, int out_l) // output width and length
{
	int in_x, in_y; // origin coordinates

	T *out_d = new T[out_w * out_l * chans];

	for (int y = 0; y < out_l; ++y ) // output rows
		for ( int x = 0; x < out_w; ++x ) { // output columns
			in_x = (float)x / out_w * in_w; //calculate the input coordinates
			in_y = (float)y / out_l * in_l;

			for ( int c = 0; c < chans; ++c ) // output channels
				out_d[ (y * out_w + x) * chans + c ] =
					in_d[ (in_y * in_w + in_x) * chans + c ];
	}

	return out_d;
}

template <class T> T *
interpolate_bilinear(T *in_d, //input data
	int in_w, int in_l, int chans, // input width, length, channels
	int out_w, int out_l) // output width and length
{
	float in_x, in_y; // sub pixel coordinates on source map.
	// the values of the four bounding texels + two interpolated ones.
	T p1,p2,p3,p4,p5,p6,p7; 
	int x1,x2,y1,y2;

	T *out_d = new T[out_w * out_l * chans];

	for (int y = 0; y < out_l; ++y ) // output rows
		for ( int x = 0; x < out_w; ++x ) { // output columns
			in_x = (float)x / (out_w-1) * (in_w-1); //calculate the input coordinates
			in_y = (float)y / (out_l-1) * (in_l-1);

			x1 = floor(in_x);
			x2 = ceil(in_x);
			y1 = floor(in_y);
			y2 = ceil(in_y);
			for ( int c = 0; c < chans; ++c ) { // output channels
				p1 = in_d[ (y1 * in_w + x1) * chans + c ];
				p2 = in_d[ (y1 * in_w + x2) * chans + c ];
				p3 = in_d[ (y2 * in_w + x1) * chans + c ];
				p4 = in_d[ (y2 * in_w + x2) * chans + c ];
				p5 = p2 * (in_x - x1) + p1 * (x2 - in_x);
				p6 = p4 * (in_x - x1) + p3 * (x2 - in_x);
				if(x1 == x2){ p5 = p1; p6 = p3; }
				p7 = p6 * (in_y - y1) + p5 * (y2 - in_y);
				if(y1 == y2) p7 = p5;
				out_d[ (y * out_l + x) * chans + c] = p7;

			}
	}

	return out_d;
}

#define RM	0x0000F800
#define GM  0x000007E0
#define BM  0x0000001F

#define RED_RGB565(x) ((x&RM)>>11)
#define GREEN_RGB565(x) ((x&GM)>>5)
#define BLUE_RGB565(x) (x&BM)
#define PACKRGB(r, g, b) (((r<<11)&RM) | ((g << 5)&GM) | (b&BM) )

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

/* FIXME update
static void
HeightMapFilter( int mapx, int mapy)
{
	float *heightmap2;
	int x,y,x2,y2,dx,dy;
	float h,tmod,mod;

	printf( "Applying lowpass filter to height map\n" );
	heightmap2 = heightmap;
	heightmap = new float[ mapx * mapy ];
	for ( y = 0; y < mapy; ++y )
		for ( x = 0; x < mapx; ++x ) {
			h = 0;
			tmod = 0;
			for ( y2 = max( 0, y - 2 ); y2 < min( mapy, y + 3 ); ++y2 ) {
				dy = y2 - y;
				for ( x2 = max( 0, x - 2 ); x2 < min( mapx, x + 3 ); ++x2 ) {
					dx = x2 - x;
					mod = max( 0.0f,
						1.0f - 0.4f * sqrtf( float( dx * dx + dy * dy ) ) );
					tmod += mod;
					h += heightmap2[ y2 * mapx + x2 ] * mod;
				}
			}
			heightmap[ y * mapx + x ] = h / tmod;
		}
	delete[] heightmap2;
}

*/


#endif //__TOOLS_H
