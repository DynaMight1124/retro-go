/***************************************************************************
    Open GL Video Rendering.

    Useful References:
    http://www.sdltutorials.com/sdl-opengl-tutorial-basics
    http://www.opengl.org/wiki/Common_Mistakes
    http://open.gl/textures

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "rendergl.h"
#include "frontend/config.h"
#include <stdint.h>
#include "../globals.h"
#include "../setup.h"
#include <SDL.h>
#include <SDL_opengl.h>

const static uint32_t SCANLINE_TEXTURE[] = { 0x00000000, 0xff000000 }; // BGRA 8-8-8-8-REV

uint32_t my_min(uint32_t a, uint32_t b) { return a < b ? a : b; }


// Texture IDs
const static int RENDER_SCREEN = 0;
const static int RENDER_SCANLN = 1;

GLuint Render_textures[2];
GLuint Render_dlist; // GL display list


SDL_Surface *Render_surface = NULL;

// Palette Lookup
uint32_t Render_rgb[S16_PALETTE_ENTRIES * 3];    // Extended to hold shadow/hilight colours

uint32_t *Render_screen_pixels = NULL;

// Original Screen Width & Height
uint16_t Render_orig_width = 0;
uint16_t Render_orig_height = 0;

// --------------------------------------------------------------------------------------------
// Screen setup properties. Example below: 
// ________________________
// |  |                |  | <- screen size      (e.g. 1280 x 720)
// |  |                |  |
// |  |                |<-|--- destination size (e.g. 1027 x 720) to maintain aspect ratio
// |  |                |  | 
// |  |                |  |    source size      (e.g. 320  x 224) System 16 proportions
// |__|________________|__|
//
// --------------------------------------------------------------------------------------------

// Source texture / pixel array that we are going to manipulate
int Render_src_width, Render_src_height;

// Destination window width and height
int Render_dst_width, Render_dst_height;

// Screen width and height 
int Render_scn_width, Render_scn_height;

// Full-Screen, Stretch, Window
int Render_video_mode;

// Scanline density. 0 = Off, 1 = Full
int Render_scanlines;

// Offsets (for full-screen mode, where x/y resolution isn't a multiple of the original height)
uint32_t Render_screen_xoff, Render_screen_yoff;

// SDL Pixel Format Codes. These differ between platforms.
uint8_t  Render_Rshift, Render_Gshift, Render_Bshift;
uint32_t Render_Rmask, Render_Gmask, Render_Bmask;

uint8_t Render_sdl_screen_size();


uint8_t Render_init(int src_width, int src_height,
                    int scale,
                    int video_mode,
                    int scanlines)
{
    Render_src_width  = src_width;
    Render_src_height = src_height;
    Render_video_mode = video_mode;
    Render_scanlines  = scanlines;

    // Setup SDL Screen size
    if (!Render_sdl_screen_size())
        return 0;

    int flags = SDL_OPENGL;

    // Full Screen Mode
    if (video_mode == VIDEO_MODE_FULL)
    {
        // Calculate how much to scale screen from its original resolution
        uint32_t w = (Render_scn_width  << 16)  / src_width;
        uint32_t h = (Render_scn_height << 16)  / src_height;
        Render_dst_width  = (Render_src_width  * my_min(w, h)) >> 16;
        Render_dst_height = (Render_src_height * my_min(w, h)) >> 16;
        //flags |= SDL_FULLSCREEN; // Set SDL flag
        SDL_ShowCursor(0);   // Don't show mouse cursor in full-screen mode
    }
    // Stretch screen. Lose original proportions
    else if (video_mode == VIDEO_MODE_STRETCH)
    {
        Render_dst_width  = Render_scn_width;
        Render_dst_height = Render_scn_height;
        //flags |= SDL_FULLSCREEN; // Set SDL flag
        SDL_ShowCursor(0);   // Don't show mouse cursor in full-screen mode
    }
    // Window Mode
    else
    {
        Render_scn_width  = Render_dst_width  = src_width  * scale;
        Render_scn_height = Render_dst_height = src_height * scale;
        SDL_ShowCursor(1);
    }

    // If we're not stretching the screen, centre the image
    if (video_mode != VIDEO_MODE_STRETCH)
    {
        Render_screen_xoff = Render_scn_width - Render_dst_width;
        if (Render_screen_xoff)
            Render_screen_xoff = (Render_screen_xoff / 2);

        Render_screen_yoff = Render_scn_height - Render_dst_height;
        if (Render_screen_yoff)
            Render_screen_yoff = (Render_screen_yoff / 2);
    }
    // Otherwise set to the top-left corner
    else
    {
        Render_screen_xoff = 0;
        Render_screen_yoff = 0;
    }

    //int bpp = info->vfmt->BitsPerPixel;
    const int bpp = 32;
    const int available = SDL_VideoModeOK(Render_scn_width, Render_scn_height, bpp, flags);

    // Frees (Deletes) existing surface
    if (Render_surface)
        SDL_FreeSurface(Render_surface);

    // Set the video mode
    Render_surface = SDL_SetVideoMode(Render_scn_width, Render_scn_height, bpp, flags);

    if (!Render_surface || !available)
    {
        //std::cerr << "Video mode set failed: " << SDL_GetError() << std::endl;
        return 0;
    }

    if (Render_screen_pixels)
        free(Render_screen_pixels);
    Render_screen_pixels = (uint32_t*)malloc(src_width * src_height * sizeof(uint32_t));

    // SDL Pixel Format Information
    Render_Rshift = Render_surface->format->Rshift;
    Render_Gshift = Render_surface->format->Gshift;
    Render_Bshift = Render_surface->format->Bshift;

    // This hack is necessary to fix an Apple OpenGL with SDL issue
    #ifdef __APPLE__
      #if SDL_BYTEORDER == SDL_LIL_ENDIAN
        Rmask = 0x000000FF;
        Gmask = 0x0000FF00;
        Bmask = 0x00FF0000;
        Render_Rshift += 8;
        Gshift -= 8;
        Bshift += 8;
      #else
        Rmask = 0xFF000000;
        Gmask = 0x00FF0000;
        Bmask = 0x0000FF00;
      #endif
    #else
        Render_Rmask  = Render_surface->format->Rmask;
        Render_Gmask  = Render_surface->format->Gmask;
        Render_Bmask  = Render_surface->format->Bmask;
    #endif

    // --------------------------------------------------------------------------------------------
    // Initalize Open GL
    // --------------------------------------------------------------------------------------------

    // Disable dithering
    glDisable(GL_DITHER);
    // Disable anti-aliasing
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    // Disable depth buffer
    glDisable(GL_DEPTH_TEST);

    // V-Sync
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    glClearColor(0, 0, 0, 0); // Black background
    glShadeModel(GL_FLAT);

    glViewport(0, 0, Render_scn_width, Render_scn_height);

    // Initalize Texture ID
    glGenTextures(scanlines ? 2 : 1, Render_textures);

    // Screen Texture Setup
    const GLint param = Config_video.filtering ? GL_LINEAR : GL_NEAREST;
    glBindTexture(GL_TEXTURE_2D, Render_textures[RENDER_SCREEN]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                src_width, src_height, 0,                // texture width, texture height
                GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,    // Data format in pixel array
                NULL);

    // Scanline Texture Setup
    if (scanlines)
    {
        glBindTexture(GL_TEXTURE_2D, Render_textures[RENDER_SCANLN]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     1, 2, 0,
                     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                     SCANLINE_TEXTURE);
    }

    // Initalize D-List
    Render_dlist = glGenLists(1);
    glNewList(Render_dlist, GL_COMPILE);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, Render_scn_width, Render_scn_height, 0, 0, 1);         // left, right, bottom, top, near, far
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear screen and depth buffer
    glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Render_textures[RENDER_SCREEN]);     // Select Screen Texture
        glBegin(GL_QUADS);
            glTexCoord2i(0, 1);
            glVertex2i  (Render_screen_xoff,             Render_screen_yoff + Render_dst_height);  // lower left
            glTexCoord2i(0, 0);
            glVertex2i  (Render_screen_xoff,             Render_screen_yoff);               // upper left
            glTexCoord2i(1, 0);
            glVertex2i  (Render_screen_xoff + Render_dst_width, Render_screen_yoff);               // upper right
            glTexCoord2i(1, 1);
            glVertex2i  (Render_screen_xoff + Render_dst_width, Render_screen_yoff + Render_dst_height);  // lower right
        glEnd();

        if (scanlines)
        {
            glEnable(GL_BLEND);
                glColor4ub(255, 255, 255, ((scanlines - 1) << 8) / 100);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glBindTexture(GL_TEXTURE_2D, Render_textures[RENDER_SCANLN]);
                glBegin(GL_QUADS);
                    glTexCoord2i(0, S16_HEIGHT);
                    glVertex2i  (Render_screen_xoff,             Render_screen_yoff + Render_dst_height);  // lower left
                    glTexCoord2i(0, 0);
                    glVertex2i  (Render_screen_xoff,             Render_screen_yoff);               // upper left
                    glTexCoord2i(src_width, 0);
                    glVertex2i  (Render_screen_xoff + Render_dst_width, Render_screen_yoff);               // upper right
                    glTexCoord2i(src_width, S16_HEIGHT);
                    glVertex2i  (Render_screen_xoff + Render_dst_width, Render_screen_yoff + Render_dst_height);  // lower right
                glEnd();
            glDisable(GL_BLEND);
        }
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
    glEndList();

    return 1;
}

void Render_disable()
{
    glDeleteLists(Render_dlist, 1);
    glDeleteTextures(Render_scanlines ? 2 : 1, Render_textures);
}

uint8_t Render_start_frame()
{
    return !(SDL_MUSTLOCK(Render_surface) && SDL_LockSurface(Render_surface) < 0);
}

uint8_t Render_finalize_frame()
{
    if (SDL_MUSTLOCK(Render_surface))
        SDL_UnlockSurface(Render_surface);

    return 1;
}

void Render_draw_frame(uint16_t* pixels)
{
    uint32_t* spix = Render_screen_pixels;

    // Lookup real RGB value from rgb array for backbuffer
    for (int i = 0; i < (Render_src_width * Render_src_height); i++)
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];

    glBindTexture(GL_TEXTURE_2D, Render_textures[RENDER_SCREEN]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,            // target, LOD, xoff, yoff
            Render_src_width, Render_src_height,                     // texture width, texture height
            GL_BGRA,                                   // format of pixel data
            GL_UNSIGNED_INT_8_8_8_8_REV,               // data type of pixel data
            Render_screen_pixels);                            // pointer in image memory

    glCallList(Render_dlist);
    //glFinish();
    SDL_GL_SwapBuffers();
}


// Setup screen size
uint8_t Render_sdl_screen_size()
{
    if (Render_orig_width == 0 || Render_orig_height == 0)
    {
        const SDL_VideoInfo* info = SDL_GetVideoInfo();

        if (!info)
        {
            //std::cerr << "Video query failed: " << SDL_GetError() << std::endl;
            return 0;
        }
        
        Render_orig_width  = info->current_w; 
        Render_orig_height = info->current_h;
    }

    Render_scn_width  = Render_orig_width;
    Render_scn_height = Render_orig_height;

    return 1;
}

// See: SDL_PixelFormat
#define CURRENT_RGB() (r << Render_Rshift) | (g << Render_Gshift) | (b << Render_Bshift);

void Render_convert_palette(uint32_t adr, uint32_t r, uint32_t g, uint32_t b)
{
    adr >>= 1;

    r = r * 255 / 31;
    g = g * 255 / 31;
    b = b * 255 / 31;

    Render_rgb[adr] = CURRENT_RGB();
      
    // Create shadow / highlight colours at end of RGB array
    // The resultant values are the same as MAME
    r = r * 202 / 256;
    g = g * 202 / 256;
    b = b * 202 / 256;
        
    Render_rgb[adr + S16_PALETTE_ENTRIES] =
    Render_rgb[adr + (S16_PALETTE_ENTRIES * 2)] = CURRENT_RGB();
}
