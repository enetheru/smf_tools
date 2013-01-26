/*
  texcompress code - generates mipmaps & compresses them using
   the compressed_rgba_s3tc_dxt1_ext opengl standard

  this code consists in many parts on code from the DDS GIMP plugin
  while it's also a rewrite of the texcompress utility from mother.
  the new codebase features a gui-less texcompress utility which
  makes script based batch jobs on - console only - linux possible.
  see README-texcompress_nogui.txt for more details.

  code made available by me according to the terms of the GPL.

  Copyright (C) 2007 Joachim Schiele <js@dune2.de>


  --

  DDS GIMP plugin

  Copyright (C) 2004 Shawn Kirst <skirst@fuse.net>,
   with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
      
*/

/*
** To get this code working you need to build the libtxc shared library
** which is included in the DRI svn.
**
** compile libtxc and install it with 'make install' as root
** before using this code
*/

#include <fstream>
#include "Bitmap.h"
#include "texcompress_nogui.h"

using namespace std;

void compressed_rgba_s3tc_dxt1_ext_software(int mipmaps,unsigned char *src, int w, int h, unsigned char *dst) {

    int bpp = 4;

    if (bpp >= 3)
        swap_rb(src, w * h, bpp);

    dxt_compress(dst, src, DDS_COMPRESS_BC1, w, h, bpp, mipmaps);
}

int dxt_compress(unsigned char *dst, unsigned char *src, int format,
                 unsigned int width, unsigned int height, int bpp,
                 int mipmaps) {
    int internal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    int i, size, w, h;
    unsigned int offset;
    unsigned char *tmp;
    unsigned char *s, c;

    if (!(IS_POT(width) && IS_POT(height)))
        return(0);

    size = get_mipmapped_size(width, height, bpp, 0, mipmaps,
                              DDS_COMPRESS_NONE);
//     printf("get_mipmapped_size: %i\n", size);
    tmp = new unsigned char[size];

    generate_mipmaps_software(tmp, src, width, height, bpp, 0, mipmaps);

    /* libtxc_dxtn wants BGRA pixels */
    for (i = 0; i < size; i += bpp) {
        c = tmp[i];
        tmp[i] = tmp[i + 2];
        tmp[i + 2] = c;
    }

    offset = 0;
    w = width;
    h = height;
    s = tmp;

    if (format <= DDS_COMPRESS_BC3N) {
        for (i = 0; i < mipmaps; ++i) {
            compress_dxtn(bpp, w, h, s, internal, dst + offset);
            s += (w * h * bpp);
            printf("get_mipmapped_size[%i] = %i\n",i,get_mipmapped_size(w, h, 0, 0, 1, format));
            offset += get_mipmapped_size(w, h, 0, 0, 1, format);
            if (w > 1) w >>= 1;
            if (h > 1) h >>= 1;
        }
    }

    delete[] tmp;

    return(1);
}

static void swap_rb(unsigned char *pixels, unsigned int n, int bpp) {
    unsigned int  i;
    unsigned char t;

    for (i = 0; i < n; ++i) {
        t = pixels[bpp * i + 0];
        pixels[bpp * i + 0] = pixels[bpp * i + 2];
        pixels[bpp * i + 2] = t;
    }
}

int generate_mipmaps_software(unsigned char *dst, unsigned char *src,
                              unsigned int width, unsigned int height,
                              int bpp, int indexed, int mipmaps) {
    int i;
    unsigned int w, h;
    unsigned int offset;

    memcpy(dst, src, width * height * bpp);
    offset = width * height * bpp;

    for (i = 1; i < mipmaps; ++i) {
        w = width  >> i;
        h = height >> i;
        if (w < 1) w = 1;
        if (h < 1) h = 1;

        scale_image_cubic(dst + offset, w, h, src, width, height, bpp);

        offset += (w * h * bpp);
    }

    return(1);
}

static void scale_image_cubic(unsigned char *dst, int dw, int dh,
                              unsigned char *src, int sw, int sh,
                              int bpp) {
    int n, x, y;
    int ix, iy;
    float fx, fy;
    float dx, dy, val;
    float r0, r1, r2, r3;
    int srowbytes = sw * bpp;
    int drowbytes = dw * bpp;

    #define VAL(x, y, c) \
    (float)src[((y) < 0 ? 0 : (y) >= sh ? sh - 1 : (y)) * srowbytes + \
               (((x) < 0 ? 0 : (x) >= sw ? sw - 1 : (x)) * bpp) + c]

    for (y = 0; y < dh; ++y) {
        fy = ((float)y / (float)dh) * (float)sh;
        iy = (int)fy;
        dy = fy - (float)iy;
        for (x = 0; x < dw; ++x) {
            fx = ((float)x / (float)dw) * (float)sw;
            ix = (int)fx;
            dx = fx - (float)ix;

            for (n = 0; n < bpp; ++n) {
                r0 = cubic_interpolate(VAL(ix - 1, iy - 1, n),
                                       VAL(ix,     iy - 1, n),
                                       VAL(ix + 1, iy - 1, n),
                                       VAL(ix + 2, iy - 1, n), dx);
                r1 = cubic_interpolate(VAL(ix - 1, iy,     n),
                                       VAL(ix,     iy,     n),
                                       VAL(ix + 1, iy,     n),
                                       VAL(ix + 2, iy,     n), dx);
                r2 = cubic_interpolate(VAL(ix - 1, iy + 1, n),
                                       VAL(ix,     iy + 1, n),
                                       VAL(ix + 1, iy + 1, n),
                                       VAL(ix + 2, iy + 1, n), dx);
                r3 = cubic_interpolate(VAL(ix - 1, iy + 2, n),
                                       VAL(ix,     iy + 2, n),
                                       VAL(ix + 1, iy + 2, n),
                                       VAL(ix + 2, iy + 2, n), dx);
                val = cubic_interpolate(r0, r1, r2, r3, dy);
                if (val <   0) val = 0;
                if (val > 255) val = 255;
                dst[y * drowbytes + (x * bpp) + n] = (unsigned char)val;
            }
        }
    }
    #undef VAL
}

float cubic_interpolate(float a, float b, float c, float d, float x) {
    float v0, v1, v2, v3, x2;

    x2 = x * x;
    v0 = d - c - a + b;
    v1 = a - b - v0;
    v2 = c - a;
    v3 = b;

    return(v0 * x * x2 + v1 * x2 + v2 * x + v3);
}


unsigned int get_mipmapped_size(int width, int height, int bpp,
                                int level, int num, int format) {
    int w, h, n = 0;
    unsigned int size = 0;

    w = width >> level;
    h = height >> level;
    if (w == 0) w = 1;
    if (h == 0) h = 1;
    w <<= 1;
    h <<= 1;

    while (n < num && (w != 1 || h != 1)) {
        if (w > 1) w >>= 1;
        if (h > 1) h >>= 1;
        if (format == DDS_COMPRESS_NONE)
            size += (w * h);
        else
            size += ((w + 3) >> 2) * ((h + 3) >> 2);
        ++n;
    }

    size *= 8;

    return(size);
}

int get_num_mipmaps(int width, int height) {
    int w = width << 1;
    int h = height << 1;
    int n = 0;

    while (w != 1 || h != 1) {
        if (w > 1) w >>= 1;
        if (h > 1) h >>= 1;
        ++n;
    }

    return(n);
}

bool compress_one(const char * in_filename) {
    int w, h;
    int bpp=4;
    unsigned char* dst;
    printf("Converting %s\n", in_filename);

    CBitmap picData(in_filename);

    w=picData.xsize;
    h=picData.ysize;

    printf("Source image xsize: %d\n", picData.xsize);
    printf("Source image ysize: %d\n", picData.ysize);

    if (w < 512) {
        printf("ERROR, xsize too small, must be at least 512 pixels\n");
        exit(1);
    }

    if (h < 512) {
        printf("ERROR, ysize too small, must be at least 512 pixels\n");
        exit(1);
    }


    if (!(IS_POT(w) && IS_POT(h))) {
        if (!IS_POT(w))
            printf("ERROR, ERROR xsize is not n^2\n");
        if (!IS_POT(h))
            printf("ERROR, ERROR ysize is not n^2\n");
        exit(1);
    }

    int mipmaps = get_num_mipmaps(w,h);
    int size = get_mipmapped_size(w, h, bpp, 0, mipmaps, DDS_COMPRESS_BC1);

    dst = new unsigned char[size];

    compressed_rgba_s3tc_dxt1_ext_software(mipmaps, picData.mem, w, h, dst);

    printf("Mipmaps built.\n");

    /*
     * Download and save the compressed minimap.
     */

    FILE * outfp;
    char outname[100];
    snprintf(outname, 100, "%s.raw", in_filename);
    outfp = fopen(outname, "wb");
    if (outfp == NULL) {
        perror("fopen");
        return false;
    }

    printf("Writing to %s:\n", outname);

    int max_mipmap;
    max_mipmap = get_num_mipmaps(w,h);

    // write every compressed mipmap into outfp
    printf("Writing %d octets for %i mipmaps to %s", size, max_mipmap, outname);
    if (fwrite(dst, size, 1, outfp) != 1) {
        perror("Couldn't write mipmap\n");
        exit(0);
    }

    printf(".\n");

    if (dst != NULL)
        delete[] dst;

    fclose(outfp);

    printf("Done.\n");

    return true;
}

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("Usage: %s image.png\n", argv[0]);
        return 1;
    }

    /*
     * Load the shared library for dxt compression
     */

    hdxtn = dlopen(DXTN_DLL, RTLD_LAZY | RTLD_GLOBAL);
    if (hdxtn == NULL) {
        printf("Unable to load library %s\n", DXTN_DLL);
        printf("If the library libtxc isn't installed. DO THIS NOW ;-)\n");
        printf("Maybe you find it here: http://homepage.hispeed.ch/rscheidegger/dri_experimental/libtxc_dxtn060508.tar.gz\n");
        printf("try to set: the LD_LIBRARY_PATH to the directory containing libtxc_dxtn.so\n example: \"export LD_LIBRARY_PATH=directory_containing_lib/\"\n");
        return 1;
    }
    printf("Library %s found and loaded with success\n", DXTN_DLL);

    compress_dxtn = (void (*)(int, int, int, const unsigned char*, int, unsigned char*))dlsym(hdxtn, "tx_compress_dxtn");

    if (compress_dxtn == NULL) {
        dlclose(hdxtn);
        printf("Missing symbol `tx_compress_dxtn' in %s\n", DXTN_DLL);
        return 1;
    }

    /*
     * For each file on the command line, make a converted version of it.
     */

    for (int i = 1; i < argc; i++) {
        if (!compress_one(argv[i])) {
            printf("ERROR, couldn't compress_one(%s)\n", argv[i]);
            return 1;
        }
    }

    return 0;
}
