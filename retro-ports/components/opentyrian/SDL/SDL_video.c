#include "SDL_video.h"
#include "SDL_system.h"
#include <rg_system.h>
#include <rg_surface.h>
#include <rg_display.h>
#include <string.h>
#include <stdlib.h>

#if RG_SCREEN_PIXEL_FORMAT == 0
#define FB_PIXEL_FORMAT RG_PIXEL_PAL565_BE
#else
#define FB_PIXEL_FORMAT RG_PIXEL_PAL565_LE
#endif

#define PRIMARY_SURFACE_COUNT 2
#define GAMEPLAY_WIDTH 264
#define GAMEPLAY_HEIGHT 184

static rg_surface_t *rg_primary_surfaces[PRIMARY_SURFACE_COUNT] = { NULL };
static SDL_Surface *primary_surface = NULL;
static unsigned int draw_surface_index = 0;
static rg_surface_t *last_submitted_surface = NULL;
static uint16_t cached_palette[256];
static bool palette_dirty = false;
static uint32_t next_surface_caps = MEM_SLOW;

static rg_surface_t *get_draw_surface(void)
{
    return rg_primary_surfaces[draw_surface_index];
}

rg_surface_t *get_tyrian_surface(void)
{
    return last_submitted_surface ? last_submitted_surface : get_draw_surface();
}

int SDL_LockSurface(SDL_Surface *surface)
{
    // The primary SDL surface always points to the non-queued Retro-Go surface.
    return 0;
}

void SDL_UnlockSurface(SDL_Surface *surface)
{
}

void SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Sint32 w, Sint32 h)
{
    SDL_Flip(screen);
}

SDL_VideoInfo *SDL_GetVideoInfo(void)
{
    static SDL_VideoInfo info;
    memset(&info, 0, sizeof(info));
    info.video_mem = 4096;
    return &info;
}

char *SDL_VideoDriverName(char *namebuf, int maxlen)
{
    return "Retro-Go Display Driver";
}

SDL_Rect **SDL_ListModes(SDL_PixelFormat *format, Uint32 flags)
{
    static SDL_Rect mode = {0, 0, 320, 200};
    static SDL_Rect *modes[2] = {&mode, NULL};
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

Uint32 SDL_WasInit(Uint32 flags)
{
    if (flags & SDL_INIT_VIDEO)
        return (primary_surface != NULL) ? SDL_INIT_VIDEO : 0;
    return 0;
}

int SDL_InitSubSystem(Uint32 flags)
{
    return 0;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    SDL_Surface *surface = (SDL_Surface *)calloc(1, sizeof(SDL_Surface));
    if (!surface)
        return NULL;

    SDL_Rect rect = {.x = 0, .y = 0, .w = (Uint16)width, .h = (Uint16)height};
    SDL_PixelFormat *pf = (SDL_PixelFormat *)calloc(1, sizeof(SDL_PixelFormat));
    if (!pf)
    {
        free(surface);
        return NULL;
    }
    
    pf->BitsPerPixel = (Uint8)depth;
    pf->BytesPerPixel = (Uint8)(depth / 8);

    surface->flags = flags;
    surface->format = pf;
    surface->w = width;
    surface->h = height;
    surface->pitch = width * (depth / 8);
    surface->clip_rect = rect;
    surface->refcount = 1;

    if (primary_surface == NULL)
    {
        // Display expansion reads these buffers sequentially. Keep them in PSRAM so
        // scarce internal RAM can hold OpenTyrian's randomly accessed render target.
        for (int i = 0; i < PRIMARY_SURFACE_COUNT; ++i)
            rg_primary_surfaces[i] = rg_surface_create(width, height, FB_PIXEL_FORMAT, MEM_SLOW);

        if (!rg_primary_surfaces[0] || !rg_primary_surfaces[1])
        {
            RG_PANIC("Failed to allocate Retro-Go primary surface!");
        }

        draw_surface_index = 0;
        surface->pixels = get_draw_surface()->data;
        primary_surface = surface;
    }
    else
    {
        surface->pixels = rg_alloc(width * height * pf->BytesPerPixel, next_surface_caps);
        next_surface_caps = MEM_SLOW;
        if (!surface->pixels)
        {
            free(pf);
            free(surface);
            return NULL;
        }
    }

    return surface;
}

void SDL_SetNextSurfaceFast(void)
{
    next_surface_caps = MEM_FAST;
}

int SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color)
{
    if (!dst || !dst->pixels)
        return -1;

    if (dstrect != NULL)
    {
        for (int y = dstrect->y; y < dstrect->y + dstrect->h; y++)
        {
            if (y >= 0 && y < dst->h)
            {
                int x1 = dstrect->x < 0 ? 0 : dstrect->x;
                int x2 = (dstrect->x + dstrect->w) > dst->w ? dst->w : (dstrect->x + dstrect->w);
                if (x2 > x1)
                {
                    memset((uint8_t *)dst->pixels + y * dst->pitch + x1, (uint8_t)color, x2 - x1);
                }
            }
        }
    }
    else
    {
        memset(dst->pixels, (uint8_t)color, dst->pitch * dst->h);
    }
    return 0;
}

SDL_Surface *SDL_GetVideoSurface(void)
{
    return primary_surface;
}

Uint32 SDL_MapRGB(SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b)
{
    if (fmt && fmt->BitsPerPixel == 16)
    {
        uint16_t bb = (b >> 3) & 0x1f;
        uint16_t gg = ((g >> 2) & 0x3f) << 5;
        uint16_t rr = ((r >> 3) & 0x1f) << 11;
        return (Uint32)(rr | gg | bb);
    }
    return (Uint32)0;
}

int SDL_SetColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors)
{
    for (int i = firstcolor; i < firstcolor + ncolors && i < 256; i++)
    {
        uint8_t r = colors[i - firstcolor].r;
        uint8_t g = colors[i - firstcolor].g;
        uint8_t b = colors[i - firstcolor].b;

#if RG_SCREEN_PIXEL_FORMAT == 0
        uint16_t v = (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
        cached_palette[i] = (v >> 8) | (v << 8);
#else
        cached_palette[i] = (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
#endif
    }
    palette_dirty = true;
    return 1;
}

SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
    return SDL_GetVideoSurface();
}

void SDL_FreeSurface(SDL_Surface *surface)
{
    if (!surface)
        return;

    if (surface == primary_surface)
    {
        while (rg_display_is_busy())
            rg_task_yield();

        for (int i = 0; i < PRIMARY_SURFACE_COUNT; ++i)
        {
            if (rg_primary_surfaces[i])
            {
                rg_surface_free(rg_primary_surfaces[i]);
                rg_primary_surfaces[i] = NULL;
            }
        }
        last_submitted_surface = NULL;
        draw_surface_index = 0;
        primary_surface = NULL;
    }
    else
    {
        if (surface->pixels)
            free(surface->pixels);
    }
    if (surface->format)
        free(surface->format);
    free(surface);
}

void SDL_QuitSubSystem(Uint32 flags)
{
}

static int64_t last_flip_time = 0;
static bool async_flip = false;

static void copy_surface_region(rg_surface_t *dst, const rg_surface_t *src,
                                int x, int y, int width, int height)
{
    for (int row = y; row < y + height; ++row)
    {
        memcpy(dst->data + row * dst->stride + x,
               src->data + row * src->stride + x,
               width);
    }
}

static void seed_next_surface(rg_surface_t *next, const rg_surface_t *submitted)
{
    if (async_flip)
    {
        // JE_starShowVGA overwrites the 264x184 playfield every gameplay tick.
        // Preserve only the HUD/sidebar and bottom status area between buffers.
        copy_surface_region(next, submitted, GAMEPLAY_WIDTH, 0,
                            submitted->width - GAMEPLAY_WIDTH, GAMEPLAY_HEIGHT);
        copy_surface_region(next, submitted, 0, GAMEPLAY_HEIGHT,
                            submitted->width, submitted->height - GAMEPLAY_HEIGHT);
    }
    else
    {
        copy_surface_region(next, submitted, 0, 0,
                            submitted->width, submitted->height);
    }
    memcpy(next->palette, submitted->palette, sizeof(cached_palette));
}

void SDL_SetAsyncFlip(bool enable)
{
    if (async_flip && !enable && last_submitted_surface && primary_surface)
    {
        // A menu may use the current frame as a background. Complete its
        // playfield from the most recently submitted frame before direct draws.
        while (rg_display_is_busy())
            rg_task_yield();
        copy_surface_region(get_draw_surface(), last_submitted_surface, 0, 0,
                            GAMEPLAY_WIDTH, GAMEPLAY_HEIGHT);
    }
    async_flip = enable;
}

void SDL_ResetFrameTime(void)
{
    last_flip_time = rg_system_timer();
    SDL_GetAndResetWaitTime();
}

int SDL_Flip(SDL_Surface *screen)
{
    rg_surface_t *draw_surface = get_draw_surface();
    if (draw_surface)
    {
        // 1. Calculate and report actual core busy time for this frame tick.
        // If more than 50ms elapsed, a level/file load occurred; reset so we don't spike monitor.
        int64_t now = rg_system_timer();
        int64_t busy_us = 0;
        if (last_flip_time > 0 && (now - last_flip_time) < 50000)
        {
            int64_t elapsed = now - last_flip_time;
            int64_t wait = SDL_GetAndResetWaitTime();
            busy_us = (elapsed > wait) ? (elapsed - wait) : 0;
        }
        else
        {
            SDL_GetAndResetWaitTime();
        }
        rg_system_tick(busy_us);

        // Keep each queued surface's palette immutable until Retro-Go releases it.
        if (palette_dirty)
        {
            memcpy(draw_surface->palette, cached_palette, sizeof(cached_palette));
            palette_dirty = false;
        }

        // Submission blocks only until the prior queue entry is consumed. At that
        // point the alternate surface is free, while this source is read-only.
        rg_display_submit(draw_surface, 0);
        last_submitted_surface = draw_surface;

        // Menus and transitions are intentionally synchronous. Gameplay draws into
        // the alternate surface while Retro-Go consumes the submitted one.
        if (!async_flip)
        {
            while (rg_display_is_busy())
            {
                rg_task_yield();
            }
        }
        else
        {
            draw_surface_index ^= 1;
            rg_surface_t *next_surface = get_draw_surface();
            seed_next_surface(next_surface, draw_surface);
            primary_surface->pixels = next_surface->data;
        }

        last_flip_time = rg_system_timer();
    }
    return 0;
}

void SDL_SkipFlip(void)
{
    int64_t now = rg_system_timer();
    if (last_flip_time > 0 && (now - last_flip_time) < 50000)
    {
        int64_t elapsed = now - last_flip_time;
        int64_t wait = SDL_GetAndResetWaitTime();
        int64_t busy_us = (elapsed > wait) ? (elapsed - wait) : 0;
        rg_system_tick(busy_us);
    }
    else
    {
        SDL_GetAndResetWaitTime();
        rg_system_tick(0);
    }
    last_flip_time = rg_system_timer();
}

int SDL_VideoModeOK(int width, int height, int bpp, Uint32 flags)
{
    return (bpp == 8) ? 1 : 0;
}

void SDL_LockDisplay(void)
{
}

void SDL_UnlockDisplay(void)
{
}
