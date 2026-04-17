/***************************************************************************
    Palette Control

    Sky, Ground & Road Palettes. Including palette cycling code.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.h"

// Palette Manipulation Control
//
// 0 = Palette manipulation deactivated
//
// Bit 0 = Set to enable/disable palette manipulation
// Bit 1 = Set when fade difference calculated, and fade in progress
// Bit 2 = Set when memory areas have been setup for fade
extern uint8_t OPalette_pal_manip_ctrl;

void OPalette_init();
void OPalette_setup_sky_palette();	
void OPalette_setup_sky_change();
void OPalette_setup_sky_cycle();
void OPalette_cycle_sky_palette();
void OPalette_fade_palette();
void OPalette_setup_ground_color();
void OPalette_setup_road_centre();
void OPalette_setup_road_stripes();
void OPalette_setup_road_side();
void OPalette_setup_road_colour();
