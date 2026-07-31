/***************************************************************************
    SDL Software Video Rendering.  
    
    Known Bugs:
    - Scanlines don't work when Endian changed?

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "rendersw.h"
#include "frontend/config.h"
#include <stdint.h>
#include "../globals.h"
#include "../setup.h"

#include "font/font_drawing.h"
#include <SDL/SDL.h>

/* alekmaul's scaler taken from mame4all */
void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t* restrict src, uint16_t* restrict dst)
{
    uint32_t W,H,ix,iy,x,y;
    x=startx<<16;
    y=starty<<16;
    W=newwidth;
    H=newheight;
    ix=(viswidth<<16)/W;
    iy=(visheight<<16)/H;

    do 
    {
        uint16_t* restrict buffer_mem=&src[(y>>16)*pitchsrc];
        W=newwidth; x=startx<<16;
        do 
        {
            *dst++=buffer_mem[x>>16];
            x+=ix;
        } while (--W);
        dst+=pitchdest;
        y+=iy;
    } while (--H);
}


uint32_t my_min(uint32_t a, uint32_t b) { return a < b ? a : b; }

#ifdef RETRO_GO
uint16_t Render_rgb[S16_PALETTE_ENTRIES * 3];
#else
uint32_t Render_rgb[S16_PALETTE_ENTRIES * 3];
#endif

#ifndef RETRO_GO
SDL_Surface *Render_surface, *real_screen;
#endif

uint16_t *Render_screen_pixels;

// Original Screen Width & Height
uint16_t Render_orig_width, Render_orig_height;

// SDL Screen Width & Height
uint16_t Render_scn_width, Render_scn_height;

int Render_src_width, Render_src_height;

// Video mode: 0 = Windowed, 1 = Fullscreen
int Render_video_mode;

uint8_t  Render_Rshift, Render_Gshift, Render_Bshift;
uint32_t Render_Rmask, Render_Gmask, Render_Bmask;

uint8_t Render_sdl_screen_size();

#ifndef RETRO_GO
uint8_t Render_init(int src_width, int src_height, 
                    int scale,
                    int video_mode,
                    int scanlines)
{
    Render_src_width  = src_width;
    Render_src_height = src_height;
    Render_video_mode = video_mode;

    // Setup SDL Screen size
    if (!Render_sdl_screen_size())
        return 0;

    int bpp = 16;
    int32_t flags = SDL_FLAGS;

    if (Render_video_mode == 0)
        flags |= SDL_FULLSCREEN;

#ifdef RS90_PORT
    real_screen = SDL_SetVideoMode(Render_scn_width, Render_scn_height, bpp, flags);
    Render_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 224, 16, 0, 0, 0, 0);
#else
    // Set the video mode
	Render_surface = SDL_SetVideoMode(Render_scn_width, Render_scn_height, bpp, flags);
#endif

    if (!Render_surface)
    {
        fprintf(stderr, "Video mode set failed: %d.\n", SDL_GetError());
        return 0;
    }

    // Convert the SDL pixel surface to 16 bit.
    // This is potentially a larger surface area than the internal pixel array.
    Render_screen_pixels = (uint16_t*)Render_surface->pixels;
    
    // SDL Pixel Format Information
    Render_Rshift = Render_surface->format->Rshift;
    Render_Gshift = Render_surface->format->Gshift;
    Render_Bshift = Render_surface->format->Bshift;
    Render_Rmask  = Render_surface->format->Rmask;
    Render_Gmask  = Render_surface->format->Bmask;
    Render_Bmask  = Render_surface->format->Bmask;

    return 1;
}

void Render_disable()
{
	if (Render_surface)
		SDL_FreeSurface(Render_surface);
}

uint8_t Render_start_frame()
{
	return !(SDL_MUSTLOCK(Render_surface) && SDL_LockSurface(Render_surface) < 0);
}

uint8_t Render_finalize_frame()
{
	if (SDL_MUSTLOCK(Render_surface))
		SDL_UnlockSurface(Render_surface);
#ifdef RS90_PORT
	bitmap_scale(0, 0, 320, 224, real_screen->w, real_screen->h, 320, 0, Render_surface->pixels, real_screen->pixels);
	SDL_Flip(real_screen);
#else
	SDL_Flip(Render_surface);
#endif
	
    return 1;
}

void Render_draw_frame(uint16_t* pixels)
{
	uint32_t i = 0;
#ifdef CENTER_240
    uint16_t* spix = Render_screen_pixels + (1280 + 320);
#else
    uint16_t* spix = Render_screen_pixels;
#endif
    for (i = 0; i < (320 * 224); i++)
    {
		*(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)]; 
	}
}
#endif

// Setup screen size
uint8_t Render_sdl_screen_size()
{
    if (Render_orig_width == 0 || Render_orig_height == 0)
    {
		Render_orig_width  = 320; 
#ifdef CENTER_240
		Render_orig_height = 240;
#else
		Render_orig_height = 224;
#endif
    }

    Render_scn_width  = Render_orig_width;
    Render_scn_height = Render_orig_height;

    return 1;
}

#ifndef RETRO_GO
void Render_convert_palette(uint32_t adr, uint32_t r, uint32_t g, uint32_t b)
{
    adr >>= 1;

    r = (r) * (255 / 31);
    g = (g) * (255 / 31);
    b = (b) * (255 / 31);
    
	Render_rgb[adr] = SDL_MapRGB(Render_surface->format, r , g, b);
      
    // Create shadow / highlight colours at end of RGB array
    // The resultant values are the same as MAME
    // 79105
    r = (r * 202) / 256;
    g = (g * 202) / 256;
    b = (b * 202) / 256;
    
    Render_rgb[adr + S16_PALETTE_ENTRIES] = Render_rgb[adr + (S16_PALETTE_ENTRIES * 2)] = SDL_MapRGB(Render_surface->format, r , g, b);
}
#endif
