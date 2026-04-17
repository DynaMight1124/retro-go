#include "video.h"
#include "hwvideo/hwsprites.h"
#include "globals.h"
#include "frontend/config.h"
#include <stdint.h>
/***************************************************************************
    Video Emulation: OutRun Sprite Rendering Hardware.
    Based on MAME source code.

    Copyright Aaron Giles.
    All rights reserved.
***************************************************************************/

/*******************************************************************************************
*  Out Run/X-Board-style sprites
*
*      Offs  Bits               Usage
*       +0   e------- --------  Signify end of sprite list
*       +0   -h-h---- --------  Hide this sprite if either bit is set
*       +0   ----bbb- --------  Sprite bank
*       +0   -------t tttttttt  Top scanline of sprite + 256
*       +2   oooooooo oooooooo  Offset within selected sprite bank
*       +4   ppppppp- --------  Signed 7-bit pitch value between scanlines
*       +4   -------x xxxxxxxx  X position of sprite (position $BE is screen position 0)
*       +6   -s------ --------  Enable shadows
*       +6   --pp---- --------  Sprite priority, relative to tilemaps
*       +6   ------vv vvvvvvvv  Vertical zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
*       +8   y------- --------  Render from top-to-bottom (1) or bottom-to-top (0) on screen
*       +8   -f------ --------  Horizontal flip: read the data backwards if set
*       +8   --x----- --------  Render from left-to-right (1) or right-to-left (0) on screen
*       +8   ------hh hhhhhhhh  Horizontal zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
*       +E   dddddddd dddddddd  Scratch space for current address
*
*  Out Run only:
*       +A   hhhhhhhh --------  Height in scanlines - 1
*       +A   -------- -ccccccc  Sprite color palette
*
*  X-Board only:
*       +A   ----hhhh hhhhhhhh  Height in scanlines - 1
*       +C   -------- cccccccc  Sprite color palette
*
*  Final bitmap format:
*
*            -s------ --------  Shadow control
*            --pp---- --------  Sprite priority
*            ----cccc cccc----  Sprite color palette
*            -------- ----llll  4-bit pixel data
*
 *******************************************************************************************/

// Clip values.
uint16_t HWSprites_x1, HWSprites_x2;

// 128 sprites, 16 bytes each (0x400)
#define SPRITE_RAM_SIZE (128 * 8)
#define SPRITES_LENGTH (0x100000 >> 2)
#define COLOR_BASE 0x800

uint32_t *sprites = NULL; // Pointer to decoded sprites (reuses ROM buffer)
    
void HWSprites_Create(void)
{
    // Memory is allocated by RomLoader and reused here
}

void HWSprites_Destroy(void)
{
    sprites = NULL;
}
    
// Two halves of RAM
uint16_t ram[SPRITE_RAM_SIZE];
uint16_t ramBuff[SPRITE_RAM_SIZE];

void HWSprites_init(const uint8_t* src_sprites)
{
    uint32_t i;
    HWSprites_reset();

    if (src_sprites)
    {
        // Reuse the ROM buffer. It must NOT be freed by RomLoader_unload.
        sprites = (uint32_t*)src_sprites;

        // Perform in-place conversion (byte swap)
        // src_sprites contains interleaved bytes: f1[0], f2[0], f3[0], f4[0]...
        // We want uint32_t to be: (f4[0] << 24) | (f3[0] << 16) | (f2[0] << 8) | f1[0]
        // This is exactly what a little-endian read of these 4 bytes would give.
        // However, the engine might expect big-endian or specific order.
        // The original code did: (d0 << 24) | (d1 << 16) | (d2 << 8) | d3 where d3, d2, d1, d0 are sequential bytes.
        // That means it was reversing the 4 bytes.
        
        for (i = 0; i < SPRITES_LENGTH; i++)
        {
            uint32_t val = sprites[i];
            // Byte swap 32-bit word
            sprites[i] = ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000) | ((val << 24) & 0xff000000);
        }
    }
}

void HWSprites_reset()
{
    uint16_t i;
    
    // Clear Sprite RAM buffers
    for (i = 0; i < SPRITE_RAM_SIZE; i++)
    {
        ram[i] = 0;
        ramBuff[i] = 0;
    }
}

// Clip areas of the screen in wide-screen mode
void HWSprites_set_x_clip(uint8_t on)
{
    // Clip to central 320 width window.
    if (on)
    {
        HWSprites_x1 = Config_s16_x_off;
        HWSprites_x2 = HWSprites_x1 + S16_WIDTH;

        if (Config_video.hires)
        {
            HWSprites_x1 <<= 1;
            HWSprites_x2 <<= 1;
        }
    }
    // Allow full wide-screen.
    else
    {
        HWSprites_x1 = 0;
        HWSprites_x2 = Config_s16_width;
    }
}


uint8_t HWSprites_read(const uint16_t adr)
{
    uint16_t a = adr >> 1;
    if ((adr & 1) == 1)
        return ram[a] & 0xff;
    else
        return ram[a] >> 8;
}

void HWSprites_write(const uint16_t adr, const uint16_t data)
{
    ram[adr >> 1] = data;
}

// Copy back buffer to main ram, ready for blit
void HWSprites_swap()
{
    uint16_t i;
    uint16_t *src = (uint16_t *)ram;
    uint16_t *dst = (uint16_t *)ramBuff;

    // swap the halves of the road RAM
    for (i = 0; i < SPRITE_RAM_SIZE; i++)
    {
        uint16_t temp = *src;
        *src++ = *dst;
        *dst++ = temp;
    }
}

#define HWSprites_draw_pixel()                                                                                  \
{                                                                                                     \
    if (x >= HWSprites_x1 && x < HWSprites_x2 && pix != 0 && pix != 15)                                                   \
    {                                                                                                 \
        if (shadow && pix == 0xa)                                                                     \
        {                                                                                             \
            pPixel[x] &= 0xfff;                                                                       \
            pPixel[x] += ((S16_PALETTE_ENTRIES * 2) - ((Video_read_pal16(pPixel[x]) & 0x8000) >> 3)); \
        }                                                                                             \
        else                                                                                          \
        {                                                                                             \
            pPixel[x] = (pix | color);                                                                \
        }                                                                                             \
    }                                                                                                 \
}

void HWSprites_render(const uint8_t priority)
{
    uint16_t data;
    const uint32_t numbanks = SPRITES_LENGTH / 0x10000;

    for (data = 0; data < SPRITE_RAM_SIZE; data += 8) 
    {
        // stop when we hit the end of sprite list
        if ((ramBuff[data+0] & 0x8000) != 0) break;

        uint32_t sprpri  = 1 << ((ramBuff[data+3] >> 12) & 3);
        if (sprpri != priority) continue;

        // if hidden, or top greater than/equal to bottom, or invalid bank, punt
        int16_t hide    = (ramBuff[data+0] & 0x5000);
        int32_t height  = (ramBuff[data+5] >> 8) + 1;       
        if (hide != 0 || height == 0) continue;
        
        int16_t bank    = (ramBuff[data+0] >> 9) & 7;
        int32_t top     = (ramBuff[data+0] & 0x1ff) - 0x100;
        uint32_t addr    = ramBuff[data+1];
        int32_t pitch  = ((ramBuff[data+2] >> 1) | ((ramBuff[data+4] & 0x1000) << 3)) >> 8;
        int32_t xpos    =  ramBuff[data+6]; // moved from original structure to accomodate widescreen
        uint8_t shadow  = (ramBuff[data+3] >> 14) & 1;
        int32_t vzoom    = ramBuff[data+3] & 0x7ff;
        int32_t ydelta = ((ramBuff[data+4] & 0x8000) != 0) ? 1 : -1;
        int32_t flip   = (~ramBuff[data+4] >> 14) & 1;
        int32_t xdelta = ((ramBuff[data+4] & 0x2000) != 0) ? 1 : -1;
        int32_t hzoom    = ramBuff[data+4] & 0x7ff;     
        int32_t color   = COLOR_BASE + ((ramBuff[data+5] & 0x7f) << 4);
        int32_t x, y, ytarget, yacc = 0, pix;
            
        // adjust X coordinate
        // note: the threshhold below is a guess. If it is too high, rachero will draw garbage
        // If it is too low, smgp won't draw the bottom part of the road
        if (xpos < 0x80 && xdelta < 0)
            xpos += 0x200;
        xpos -= 0xbe;

        // initialize the end address to the start address
        ramBuff[data+7] = addr;

        // clamp to within the memory region size
        if (numbanks)
            bank %= numbanks;

        const uint32_t* spritedata = sprites + 0x10000 * bank;

        // clamp to a maximum of 8x (not 100% confirmed)
        if (vzoom < 0x40) vzoom = 0x40;
        if (hzoom < 0x40) hzoom = 0x40;

        // loop from top to bottom
        ytarget = top + ydelta * height;

        // Adjust for widescreen mode
        xpos += Config_s16_x_off;

        // Adjust for hi-res mode
        if (Config_video.hires)
        {
            xpos <<= 1;
            top <<= 1;
            ytarget <<= 1;
            hzoom >>= 1;
            vzoom >>= 1;
        }

        for (y = top; y != ytarget; y += ydelta)
        {
            // skip drawing if not within the cliprect
            if (y >= 0 && y < Config_s16_height)
            {
                uint16_t* pPixel = &Video_pixels[y * Config_s16_width];
                int32_t xacc = 0;

                // non-flipped case
                if (flip == 0)
                {
                    // start at the word before because we preincrement below
                    ramBuff[data+7] = (addr - 1);

                    for (x = xpos; (xdelta > 0 && x < Config_s16_width) || (xdelta < 0 && x >= 0); )
                    {
                        uint32_t pixels = spritedata[++ramBuff[data+7]]; // Add to base sprite data the vzoom value

                        // draw four pixels
                        pix = (pixels >> 28) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 24) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 20) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 16) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 12) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  8) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  4) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  0) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;

                        // stop if the second-to-last pixel in the group was 0xf
                        if ((pixels & 0x000000f0) == 0x000000f0)
                            break;
                    }
                }
                // flipped case
                else
                {
                    // start at the word after because we predecrement below
                    ramBuff[data+7] = (addr + 1);

                    for (x = xpos; (xdelta > 0 && x < Config_s16_width) || (xdelta < 0 && x >= 0); )
                    {
                        uint32_t pixels = spritedata[--ramBuff[data+7]];

                        // draw four pixels
                        pix = (pixels >>  0) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  4) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  8) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 12) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 16) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 20) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 24) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 28) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixel(); x += xdelta; xacc += hzoom; } xacc -= 0x200;

                        // stop if the second-to-last pixel in the group was 0xf
                        if ((pixels & 0x0f000000) == 0x0f000000)
                            break;
                    }
                }
            }
            // accumulate zoom factors; if we carry into the high bit, skip an extra row
            yacc += vzoom; 
            addr += pitch * (yacc >> 9);
            yacc &= 0x1ff;
        }
    }
}
