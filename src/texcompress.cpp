#include <SDL.h>
#include <SDL_main.h>
#include <GL/glew.h> /* Goes before GL headers. */
#include <GL/gl.h>
#include <GL/glu.h>
#include <fstream>
#include "Bitmap.h"

/* Size of the window that pops up during conversion. */
#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 512
/* Define this if you want texcompress to pause at each stage.
   With focus on the window, press space to continue, or q to quit.
 */
#undef INTERACTIVE

using namespace std;

void show_gl_error(const char *place)
{
    GLenum errorCode = glGetError();
    if (errorCode != GL_NO_ERROR) {
        printf("%s: GL error %s\n", place, gluErrorString(errorCode));
    }
}



void _display_image(int minimapTex)
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the given image
    glColor4f(1, 1, 1, 1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, minimapTex);
    show_gl_error("bind minimapTex");
    glBegin(GL_QUADS);
    {
        int bottom = 0;
        int left = 0;
        int right = WINDOW_WIDTH;
        int top = WINDOW_HEIGHT;
        glTexCoord2f(0, 1); glVertex2f(left, bottom);
        glTexCoord2f(0, 0); glVertex2f(left, top);
        glTexCoord2f(1, 0); glVertex2f(right, top);
        glTexCoord2f(1, 1); glVertex2f(right, bottom);
    }
    glEnd();
    show_gl_error("show minimapTex");
    glDisable(GL_TEXTURE_2D);

    // Swap buffers
    SDL_GL_SwapBuffers();
}

/* Returns true if the user asks to stop conversion. */
bool display_image(int minimapTex)
{
#ifndef INTERACTIVE
    _display_image(minimapTex);
    return false;
#else
    /*
     * Draw image, and wait for a keypress.
     */

    bool must_quit = false;

    while (!must_quit) {
        _display_image(minimapTex);

        SDL_Event e;

        // go through all available events; block if there's none available (no keypress)
        while (SDL_WaitEvent(&e)) {
            //printf("PollEvent came back\n");

            // we use a switch to determine the event type
            switch (e.type) {
                // key down
            case SDL_KEYDOWN:

                // as for the character it's a litte more complicated. we'll use for translated unicode value.
                // this is described in more detail below.
                if ((e.key.keysym.unicode & 0xFF80) == 0) {
                    if (e.key.keysym.unicode == 'q') {
                        return true;
                    } else if (e.key.keysym.unicode == ' ') {
                        must_quit = true;
                    }
                }
                break;
            }
        }
    }

    return false;
#endif
}

bool preview(CBitmap * picData)
{
    GLuint minimapTex;

    glGenTextures(1, &minimapTex);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, minimapTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
        picData->xsize, picData->ysize, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, picData->mem);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    bool ret = display_image(minimapTex);

    glDeleteTextures(1, &minimapTex);

    if (ret) return false;
    return true;
}

/* If the current texture were mipmapped, how many levels would there be? */
int mipmap_levels(void)
{
    GLint xsize, ysize;

    /* Get the size of the full-size image, er.

       gluBuild2DMipmaps may have rescaled the original image to
       side lengths of powers of 2. */
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &xsize);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &ysize);
    show_gl_error("Fetch base texture size");

    /* The image's dimensions get cut in half until they're both 1.
       Hence, we worry about the biggest side. */
    int size_left = (xsize > ysize) ? xsize : ysize;

    /* Slow but readable algorithm: */

    int levels = 1;

    while (size_left > 1) {
        size_left = size_left / 2;
        levels += 1;
    }

    return levels;
}

bool compress_one(const char * in_filename)
{
    GLuint minimapTex;
    printf("Converting %s\n", in_filename);

    CBitmap picData(in_filename);

    printf("%d x %d\n", picData.xsize, picData.ysize);

    /* Show the uncompressed image, so we can see if we've got it the right way up and so forth. */
    if (!preview(&picData))
        return false;

    /*
     * Upload the minimap.
     */
    glGenTextures(1, &minimapTex);
    glBindTexture(GL_TEXTURE_2D, minimapTex);

    show_gl_error("Fetch texture name");

    printf("Having glu build mipmaps...\n");

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
        picData.xsize, picData.ysize, GL_RGBA, GL_UNSIGNED_BYTE, picData.mem);

    printf("Mipmaps built.\n");

    show_gl_error("Upload texture");

    /* Visual inspection of the compressed version, too. */
    if (display_image(minimapTex))
        return false;

    /*
     * Download and save the compressed minimap.
     */
    glBindTexture(GL_TEXTURE_2D, minimapTex);

    FILE * outfp;
    char outname[100];
    snprintf(outname, 100, "%s.raw", in_filename);
    outfp = fopen(outname, "wb");
    if (outfp == NULL)
    {
        perror("fopen");
        return false;
    }

    printf("Writing to %s:\n", outname);

    char * dst = NULL;
    int dst_len = 0;
    int max_mipmap = mipmap_levels();

    for (int mipmapLevel = 0; mipmapLevel < max_mipmap; mipmapLevel++) {
        printf("Mipmap %d/%d ", mipmapLevel + 1, max_mipmap);

        /* Great. Magic numbers. (TODO Where'd I get these from?) */
        int mipxsize = picData.xsize >> mipmapLevel;
        int mipysize = picData.ysize >> mipmapLevel;
        int expectedSize = ((mipxsize + 3) / 4) * ((mipysize + 3) / 4) * 8;
        int size;

        glGetTexLevelParameteriv(GL_TEXTURE_2D, mipmapLevel,
            GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &size);
        show_gl_error("Fetch size");

        printf(": %d octets", size);

        if (size != expectedSize) {
            printf(" (Should be %d?)", expectedSize);
        }

        if (dst_len < size)
        {
            if (dst != NULL)
                delete[] dst;

            dst = new char[size];
            dst_len = size;
        }

        glGetCompressedTexImage(GL_TEXTURE_2D, mipmapLevel, dst);
        show_gl_error("Fetch data");

        if (fwrite(dst, size, 1, outfp) != 1) {
            perror("Couldn't write minimap\n");
        }

        printf(".\n");
    }

    if (dst != NULL)
        delete[] dst;

    fclose(outfp);

    printf("Done.\n");

    glDeleteTextures(1, &minimapTex);

    return true;
}

int main(int argc, char **argv)
{
    /* TODO An --interactive option.
       TODO Describe what it does on --help.
     */

    if (argc < 2) {
        printf("Usage: %s image.png\n", argv[0]);
        return 1;
    }

    /*
     * Set up. (There's rather a lot of it.)
     */

    if ((SDL_Init(SDL_INIT_VIDEO) == -1)) {
        printf("%s\n", "Could not initialize SDL.");
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface *screen =
        SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 0, SDL_OPENGL);
    if (!screen) {
        printf("%s\n", "Could not set video mode");
        return 1;
    }

    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.1f);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    /* Use 2D projection, or something. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);

    /* Reset modelview matrix */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
        SDL_DEFAULT_REPEAT_INTERVAL);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapBuffers();

    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);

    SDL_EnableUNICODE(1);

    /* Fetch glCompressedTexImage2DARB. */
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return 2;
    }
    fprintf(stdout, "Status: Using GLEW %s\n",
            glewGetString(GLEW_VERSION));

    if (!(GLEW_ARB_texture_compression)) {
        printf
            ("Erk, GLEW can't find the generic texture compression extension.\n");
        return 3;
    }

    /* TODO Check for the specific form of compression. */

    show_gl_error("HACK Flush");

    /*
     * For each file on the command line, make a converted version of it.
     */

    for (int i = 1; i < argc; i++) {
        if (!compress_one(argv[i]))
            break;
    }

    return 0;
}
