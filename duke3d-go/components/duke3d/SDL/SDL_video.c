#include "SDL_video.h"
#include "rg_display.h"
#include "rg_system.h"
#include "build.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

static rg_surface_t screen_surface;

SDL_Surface* primary_surface;

int SDL_LockSurface(SDL_Surface *surface)
{
    return 0;
}

void SDL_UnlockSurface(SDL_Surface* surface)
{

}

void SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Sint32 w, Sint32 h)
{
    SDL_Flip(screen);
}

SDL_VideoInfo *SDL_GetVideoInfo(void)
{
    static SDL_VideoInfo info;
    static SDL_PixelFormat vfmt;
    info.vfmt = &vfmt;
    info.vfmt->BitsPerPixel = 8;
    return &info;
}

char *SDL_VideoDriverName(char *namebuf, int maxlen)
{
    return "RETRO-GO";
}


SDL_Rect **SDL_ListModes(SDL_PixelFormat *format, Uint32 flags)
{
    static SDL_Rect mode = {0, 0, 320, 240};
    static SDL_Rect *modes[2];
    modes[0] = &mode;
    modes[1] = NULL;
    return modes;
}

void SDL_WM_SetCaption(const char *title, const char *icon)
{

}

char *SDL_GetKeyName(SDLKey key)
{
    return (char *)"";
}

SDL_Keymod SDL_GetModState(void)
{
    return (SDL_Keymod)0;
}

IRAM_ATTR Uint32 SDL_GetTicks(void)
{
    return esp_timer_get_time() / 1000;
}

Uint32 SDL_WasInit(Uint32 flags)
{
	return 0;
}

#define MAX_SUBMITTED_SURFACES 2
typedef struct {
    rg_surface_t surface;
    uint8_t *pixels;
} fb_t;

static fb_t fb_pool[MAX_SUBMITTED_SURFACES];
static int current_fb_idx = 0;

int SDL_InitSubSystem(Uint32 flags)
{
    if(flags & SDL_INIT_VIDEO)
    {
        for (int i = 0; i < MAX_SUBMITTED_SURFACES; i++) {
            fb_pool[i].pixels = rg_alloc(INTERNAL_RES_W * INTERNAL_RES_H, MEM_SLOW);
            fb_pool[i].surface.palette = rg_alloc(256 * 2, MEM_SLOW);
            if (!fb_pool[i].pixels || !fb_pool[i].surface.palette) {
                RG_LOGE("Failed to pre-allocate FB %d", i);
                return -1;
            }
        }
        SDL_CreateRGBSurface(0, INTERNAL_RES_W, INTERNAL_RES_H, 8, 0,0,0,0);
    }
    if(flags & SDL_INIT_AUDIO)
    {

    }
    return 0; // 0 = OK, -1 = Error
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    SDL_Surface *surface = (SDL_Surface *)rg_alloc(sizeof(SDL_Surface), MEM_SLOW);
    SDL_Rect rect = { .x=0, .y=0, .w=width, .h=height};
    
    SDL_PixelFormat* pf = (SDL_PixelFormat*)rg_alloc(sizeof(SDL_PixelFormat), MEM_SLOW);
    pf->palette = (SDL_Palette *)rg_alloc(sizeof(SDL_Palette), MEM_SLOW);
    pf->palette->ncolors = 256;
    pf->palette->colors = (SDL_Color *)rg_alloc(256 * sizeof(SDL_Color), MEM_SLOW);
    
	pf->BitsPerPixel = depth;
	pf->BytesPerPixel = depth / 8;
	pf->Rloss = 0; pf->Gloss = 0; pf->Bloss = 0; pf->Aloss = 0,
	pf->Rshift = 0; pf->Gshift = 0; pf->Bshift = 0; pf->Ashift = 0;
	pf->Rmask = Rmask; pf->Gmask = Gmask; pf->Bmask = Bmask; pf->Amask = Amask;
	pf->colorkey = 0;
	pf->alpha = 255;

    surface->flags = flags;
    surface->format = pf;
    surface->w = width;
    surface->h = height;
    surface->pitch = width*(depth/8);
    surface->clip_rect = rect;
    surface->refcount = 1;
    surface->pixels = rg_alloc(width*height*(depth/8), MEM_SLOW);
    if(primary_surface == NULL)
    	primary_surface = surface;
    return surface;
}

int SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color)
{
    if(dst)//|| dst->sprite == NULL)
    {
    	if(dstrect != NULL)
    	{
			for(int y = dstrect->y; y < dstrect->y + dstrect->h;y++)
				memset((unsigned char *)dst->pixels + y*dst->w + dstrect->x, (unsigned char)color, dstrect->w);
    	} else {
    		memset(dst->pixels, (unsigned char)color, dst->pitch*dst->h);
    	}
    }
    return 0;
}

SDL_Surface *SDL_GetVideoSurface(void)
{
    return primary_surface;
}

Uint32 SDL_MapRGB(SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b)
{
    if(fmt->BitsPerPixel == 16)
    {
        uint16_t bb = (b >> 3) & 0x1f;
        uint16_t gg = ((g >> 2) & 0x3f) << 5;
        uint16_t rr = ((r >> 3) & 0x1f) << 11;
        return (Uint32) (rr | gg | bb);
    }
    return (Uint32)0;
}

int SDL_SetColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors)
{
    if (surface == NULL) {
        printf("SDL_SetColors: surface is NULL!\n");
        return 0;
    }
    if (!screen_surface.palette) {
        screen_surface.palette = rg_alloc(256 * 2, MEM_SLOW);
    }
    if (!screen_surface.palette) return 0;

	for(int i = firstcolor; i < firstcolor+ncolors; i++)
	{
        if (i >= 256) break;
        uint16_t r = colors[i].r;
        uint16_t g = colors[i].g;
        uint16_t b = colors[i].b;
        uint16_t v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
#if RG_SCREEN_PIXEL_FORMAT == 0 /* 565_BE */
        v = (v >> 8) | (v << 8);
#endif
		screen_surface.palette[i] = v;
    }
    return 1;
}

SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
    if (primary_surface) {
        if (primary_surface->w == width && primary_surface->h == height)
            return primary_surface;
    }
	return SDL_GetVideoSurface();
}

void SDL_FreeSurface(SDL_Surface *surface)
{
    if (surface) {
        if (surface->pixels) free(surface->pixels);
        if (surface->format) {
            if (surface->format->palette) {
                if (surface->format->palette->colors) free(surface->format->palette->colors);
                free(surface->format->palette);
            }
            free(surface->format);
        }
        free(surface);
    }
}

void SDL_QuitSubSystem(Uint32 flags)
{

}

static void limit_fps(int fps)
{
    static uint32_t last_tick = 0;
    uint32_t now = SDL_GetTicks();
    uint32_t frame_time = 1000 / fps;

    if (last_tick > 0)
    {
        uint32_t elapsed = now - last_tick;
        if (elapsed < frame_time)
        {
            vTaskDelay(pdMS_TO_TICKS(frame_time - elapsed));
        }
    }
    last_tick = SDL_GetTicks();
}

int SDL_Flip(SDL_Surface *screen)
{
    if (!screen || !screen->pixels) {
        printf("SDL_Flip: screen or pixels is NULL!\n");
        return -1;
    }

    fb_t *fb = &fb_pool[current_fb_idx];
    current_fb_idx = (current_fb_idx + 1) % MAX_SUBMITTED_SURFACES;

    if (!fb->pixels || !fb->surface.palette) {
        RG_LOGE("SDL_Flip: FB not allocated!");
        return -1;
    }

    fb->surface.width = screen->w;
    fb->surface.height = screen->h;
    fb->surface.stride = screen->w;
    fb->surface.format = RG_PIXEL_PAL565_BE;
    fb->surface.data = fb->pixels;

    if (screen->w == INTERNAL_RES_W && screen->h == INTERNAL_RES_H) {
        if (rg_display_get_scaling() != RG_DISPLAY_SCALING_FULL) {
            rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);
        }
    }

    // printf("SDL_Flip: memcpy pixels\n");
    // Copy pixels to stable buffer
    memcpy(fb->pixels, screen->pixels, screen->w * screen->h);

    // printf("SDL_Flip: memcpy palette\n");
    // Copy palette to stable buffer
    if (screen_surface.palette) {
        memcpy(fb->surface.palette, screen_surface.palette, 256 * 2);
    }
    else {
        // printf("SDL_Flip: screen_surface.palette is NULL\n");
    }

    // printf("SDL_Flip: rg_display_submit\n");
    rg_display_submit(&fb->surface, 0);
    rg_system_tick(0);

    limit_fps(60);

	return 0;
}

int SDL_VideoModeOK(int width, int height, int bpp, Uint32 flags)
{
	if(bpp == 8)
		return 1;
	return 0;
}

SemaphoreHandle_t display_mutex = NULL;

void SDL_LockDisplay()
{
    // if (display_mutex == NULL)
    // {
    //     printf("Creating display mutex.\n");
    //     display_mutex = xSemaphoreCreateMutex();
    //     if (!display_mutex)
    //         abort();
    //     //xSemaphoreGive(display_mutex);
    // }

    // if (!xSemaphoreTake(display_mutex, 60000 / portTICK_RATE_MS))
    // {
    //     printf("Timeout waiting for display lock.\n");
    //     abort();
    // }
    //printf("L");
    //taskYIELD();
}

void SDL_UnlockDisplay()
{
    // if (!display_mutex)
    //     abort();
    // if (!xSemaphoreGive(display_mutex))
    //     abort();

    //printf("U ");
    //taskYIELD();
}
