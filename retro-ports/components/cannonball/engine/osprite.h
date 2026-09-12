/***************************************************************************
    Hardware Sprites.
    
    This class stores sprites in the converted format expected by the
    OutRun graphics hardware.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "stdint.h" 

typedef struct 
{
	uint16_t data[0x7];
	uint32_t scratch;
} osprite;


void osprite_init(osprite* sprite);

uint16_t osprite_get_x(osprite* sprite);
uint16_t osprite_get_y(osprite* sprite);
void osprite_set_x(osprite* sprite, uint16_t);
void osprite_inc_x(osprite* sprite, uint16_t);
void osprite_set_y(osprite* sprite, uint16_t);
void osprite_set_pitch(osprite* sprite, uint8_t);
void osprite_set_vzoom(osprite* sprite, uint16_t);
void osprite_set_hzoom(osprite* sprite, uint16_t);
void osprite_set_priority(osprite* sprite, uint8_t);
void osprite_set_offset(osprite* sprite, uint16_t o);
void osprite_inc_offset(osprite* sprite, uint16_t o);
void osprite_set_render(osprite* sprite, uint8_t b);
void osprite_set_pal(osprite* sprite, uint8_t);
void osprite_set_height(osprite* sprite, uint8_t);
void osprite_sub_height(osprite* sprite, uint8_t);
void osprite_set_bank(osprite* sprite, uint8_t);
void osprite_hide(osprite* sprite);
