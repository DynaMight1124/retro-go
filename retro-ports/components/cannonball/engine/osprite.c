/***************************************************************************
    Hardware Sprites.
    
    This class stores sprites in the converted format expected by the
    OutRun graphics hardware.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/osprite.h"

//  Out Run/X-Board-style sprites
//
//      Offs  Bits               Usage
//       +0   e------- --------  Signify end of sprite list
//       +0   -h-h---- --------  Hide this sprite if either bit is set
//       +0   ----bbb- --------  Sprite bank
//       +0   -------t tttttttt  Top scanline of sprite + 256
//       +2   oooooooo oooooooo  Offset within selected sprite bank
//       +4   ppppppp- --------  Signed 7-bit pitch value between scanlines
//       +4   -------x xxxxxxxx  X position of sprite (position $BE is screen position 0)
//       +6   -s------ --------  Enable shadows
//       +6   --pp---- --------  Sprite priority, relative to tilemaps
//       +6   ------vv vvvvvvvv  Vertical zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
//       +8   y------- --------  Render from top-to-bottom (1) or bottom-to-top (0) on screen
//       +8   -f------ --------  Horizontal flip: read the data backwards if set
//       +8   --x----- --------  Render from left-to-right (1) or right-to-left (0) on screen
//       +8   ------hh hhhhhhhh  Horizontal zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
//       +E   dddddddd dddddddd  Scratch space for current address
//
//    Out Run only:
//       +A   hhhhhhhh --------  Height in scanlines - 1
//       +A   -------- -ccccccc  Sprite color palette


void osprite_init(osprite* sprite)
{
    sprite->data[0] = 0;
    sprite->data[1] = 0;
    sprite->data[2] = 0;
    sprite->data[3] = 0;
    sprite->data[4] = 0;
    sprite->data[5] = 0;
    sprite->data[6] = 0;
    sprite->scratch = 0;
}

// X is now stored separately (not in the original data structure)
// This is to support wide-screen mode
uint16_t osprite_get_x(osprite* sprite)
{
    return sprite->data[0x6];
}

uint16_t osprite_get_y(osprite* sprite)
{
    return sprite->data[0x0]; // returning y uses whole value
}

void osprite_set_x(osprite* sprite, uint16_t x)
{
    sprite->data[0x6] = x;
}

void osprite_set_pitch(osprite* sprite, uint8_t p)
{
   sprite-> data[0x02] = (sprite->data[0x2] & 0x1FF) | ((p & 0xFE) << 8);
}

void osprite_inc_x(osprite* sprite, uint16_t v)
{
    sprite->data[0x6] += v;
}

void osprite_set_y(osprite* sprite, uint16_t y)
{
    sprite->data[0x0] = y; // setting y wipes entire value
}

void osprite_set_vzoom(osprite* sprite, uint16_t z)
{
    sprite->data[0x03] = z;
}

void osprite_set_hzoom(osprite* sprite, uint16_t z)
{
    sprite->data[0x4] = z;
}

void osprite_set_priority(osprite* sprite, uint8_t p)
{
    sprite->data[0x03] |= (p << 8);
}

void osprite_set_offset(osprite* sprite, uint16_t o)
{
    sprite->data[0x1] = o;
}

void osprite_inc_offset(osprite* sprite, uint16_t o)
{
    sprite->data[0x1] += o;
}

void osprite_set_render(osprite* sprite, uint8_t bits)
{
    sprite->data[0x4] |= ((bits & 0xE0) << 8);
}

void osprite_set_pal(osprite* sprite, uint8_t pal)
{
    sprite->data[0x5] = (sprite->data[0x5] & 0xFF00) + pal;
}

void osprite_set_height(osprite* sprite, uint8_t h)
{
    sprite->data[0x5] = (sprite->data[0x5] & 0xFF) + (h << 8);
}

void osprite_sub_height(osprite* sprite, uint8_t h)
{
    uint8_t height = ((sprite->data[0x05] >> 8) - h) & 0xFF;
    sprite->data[0x5] = (sprite->data[0x5] & 0xFF) + (height << 8);
}

void osprite_set_bank(osprite* sprite, uint8_t bank)
{
    sprite->data[0x0] |= (bank << 8);
}

void osprite_hide(osprite* sprite)
{
    sprite->data[0x0] |= 0x4000;
    sprite->data[0x0] &= ~0x8000; // denote sprite list not ended
}