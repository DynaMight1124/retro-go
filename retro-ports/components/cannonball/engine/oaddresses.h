/***************************************************************************
    68000 Program Code Addresses 
    Addresses to data within the Master and Sub CPU Program ROMs.
    
    These are typically large blocks of data that we don't want to include
    in the codebase. 
    
    Where needed addresses from the separate Japanese version are also
    included. These are denoted by entries ending _J
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

// ----------------------------------------------------------------------------
// Text Structures
// ----------------------------------------------------------------------------

// Text1 = Use BlitText1 routine | Text2 = Use BlitText2 routine

// Text: Credits
#define TEXT1_CREDIT  0x6D38
#define TEXT1_CREDITS 0x6D48
#define TEXT1_CLEAR_CREDITS 0x6D58
#define TEXT1_FREEPLAY 0x6D6C

// Text: Course Map
#define TEXT2_COURSEMAP 0xBBC2

// Text: Press Start
#define TEXT1_PRESS_START 0xBBD0
#define TEXT1_CLEAR_START 0xBBEC

// Text: Insert Coins
#define TEXT1_INSERT_COINS 0xBC08
#define TEXT1_CLEAR_COINS  0xBC1E

// Text: Game Over
#define TEXT2_GAMEOVER 0xBCB0

// Text: Select Music By Steering
#define TEXT2_SELECT_MUSIC 0xBCBE

// Text: 1986 Sega
#define TEXT1_1986_SEGA 0xBCF2

// Text: Copyright Symbol
#define TEXT1_COPYRIGHT 0xBD04

// Text: Magical Sound Shower
#define TEXT2_MAGICAL 0xCE04

// Text: Passing Breeze
#define TEXT2_BREEZE 0xCE1E

// Text: Splash Wave
#define TEXT2_SPLASH 0xCE38

// Text: Your Score
#define TEXT1_YOURSCORE 0xD5E0

// Text: Best OutRunners
#define TEXT2_BEST_OR 0xD5F2

// Text: Score, Name, Route, Record
#define TEXT1_SCORE_ETC 0xD606

// Text: ABCDEFGHIJKLMNOPQRSTUVWXYZ
#define TEXT2_ALPHABET 0xD5C2

// Text: Extend Time
#define TEXT1_EXTEND1 0x90DC
#define TEXT1_EXTEND2 0x90F6

// Text: Clear Extend Time
#define TEXT1_EXTEND_CLEAR1 0x9110
#define TEXT1_EXTEND_CLEAR2 0x912A

// Text: Laptime
#define TEXT1_LAPTIME1 0x9144
#define TEXT1_LAPTIME2 0x9150

// Text: Clear Laptime
#define TEXT1_LAPTIME_CLEAR1 0x915C
#define TEXT1_LAPTIME_CLEAR2 0x917A

// Text: Easter Egg
#define TEXT1_EASTER       0x91B4
#define TEXT1_EASTER_CLEAR 0x91D6

// Text: Bonus Points Section
#define TEXT2_BONUS_POINTS 0x9C0C
#define TEXT1_BONUS_STOP   0x9C1C
#define TEXT1_BONUS_SEC    0x9C26
#define TEXT1_BONUS_X      0x9C34
#define TEXT1_BONUS_PTS    0x9C3E
#define TEXT1_BONUS_100K   0x9C4A
#define TEXT2_BONUS_CLEAR1 0x9C52
#define TEXT2_BONUS_CLEAR2 0x9C64
#define TEXT2_BONUS_CLEAR3 0x9C78

// ----------------------------------------------------------------------------
// HUD
// ----------------------------------------------------------------------------

// SCORE Graphic (2 Lines)
#define HUD_SCORE1 0xBC3E
#define HUD_SCORE2 0xBC4C

// TIME Graphic (2 Lines)
#define HUD_TIME1 0xBC5A
#define HUD_TIME2 0xBC66

// KPH (2 Lines)
#define HUD_KPH1 0xBC72
#define HUD_KPH2 0xBC7E

// STAGE (2 Lines)
#define HUD_STAGE1 0xBC8A
#define HUD_STAGE2 0xBC98

// Number "1" to appear after stage
#define HUD_ONE 0xBCA6

// LAP (2 Lines)
#define HUD_LAP1 0xBCDA
#define HUD_LAP2 0xBCE6

// ----------------------------------------------------------------------------
// Tilemaps
// ----------------------------------------------------------------------------

// Tilemap hardware addresses
#define HW_FG_PSEL       0xE80
#define HW_BG_PSEL       0xE82
#define HW_BG_HSCROLL    0xE9A
#define HW_FG_VSCROLL    0xE90
#define HW_BG_VSCROLL    0xE92
#define HW_FG_HSCROLL    0xE98

// In-Game Tilemap Defaults
#define TILES_PAGE_FG1     0x17E4C
#define TILES_PAGE_BG1     0x17E5C
#define TILES_PAGE_FG2     0x17E68  // Used for road split
#define TILES_PAGE_BG2     0x17E78  // Used for road split
#define TILES_DEF_LOOKUP   0x17E84  // Tilemap default lookup indexes, for values in table below
#define TILES_DEF_LOOKUP_J 0x178E4
#define TILES_TABLE        0x17EAC  // Stage Tilemap Default Values
#define TILES_TABLE_J      0x1790c

#define TILES_MINIMAP    0x8C04

// Table of h_scroll offsets (words) for road split
// Note the h_scroll is set manually during the road split from the actual road position
#define H_SCROLL_TABLE   0x30B00

// Tilemap: Music Selection Screen
#define TILEMAP_MUSIC_SELECT 0x383F2

// ----------------------------------------------------------------------------
// Palettes
// ----------------------------------------------------------------------------

// In-Game Tilemap Palettes
#define TILEMAP_PALS 0xDF9C\

// Palette: Music Select Screen
#define PAL_MUSIC_SELECT 0x175CC

// Palette Data. Stored in blocks of 32 bytes.
#define PAL_DATA 0x14ED8

// Table of long addresses of ground colours
#define PAL_GND_TABLE 0x17350

// Table of palette addresses
#define PAL_SKY_TABLE 0x17590

// Palette Data: Best Outrunners Name Entry
#define PAL_BESTOR 0x17DCC

// ----------------------------------------------------------------------------
// Sprites
// ----------------------------------------------------------------------------

// Sprite Animation Sequences For Crashes
//
// +00 [Long] Sprite Data Frame Address
// +04 [Byte] Bit 7: Set to H-Flip Sprite
//            Bit 0: Set Sprite to Sprite Priority Higher (Unused so far?)
// +05 [Byte] Sprite Colour Palette
// +06 [Byte] Passenger Frame
//            OR FOR Passenger Sprites: X Offset
// +07 [Byte] Set to denote end of frame sequence
//            OR FOR Passenger Sprites: Y Offset

#define SPRITE_CRASH_SPIN1        0x2294
#define SPRITE_CRASH_SPIN2        0x22D4
#define SPRITE_BUMP_DATA1         0x2314
#define SPRITE_BUMP_DATA2         0x232C
#define SPRITE_CRASH_MAN1         0x2344
#define SPRITE_CRASH_GIRL1        0x23B4
#define SPRITE_CRASH_FLIP         0x2424 // Flip: Car
#define SPRITE_CRASH_FLIP_MAN1    0x2464 // Flip: Man
#define SPRITE_CRASH_FLIP_GIRL1   0x255C // Flip: Girl
#define SPRITE_CRASH_FLIP_MAN2    0x24DC // Post Flip: Man
#define SPRITE_CRASH_FLIP_GIRL2   0x25D4 // Post Flip: Girl
#define SPRITE_CRASH_MAN2         0x2604
#define SPRITE_CRASH_GIRL2        0x2660

#define SPRITE_CRASH_SPIN1_J      (SPRITE_CRASH_SPIN1 - 12)
#define SPRITE_CRASH_SPIN2_J      (SPRITE_CRASH_SPIN2 - 12)
#define SPRITE_BUMP_DATA1_J       (SPRITE_BUMP_DATA1  - 12)
#define SPRITE_BUMP_DATA2_J       (SPRITE_BUMP_DATA2  - 12)
#define SPRITE_CRASH_MAN1_J       (SPRITE_CRASH_MAN1  - 12)
#define SPRITE_CRASH_GIRL1_J      (SPRITE_CRASH_GIRL1 - 12)
#define SPRITE_CRASH_FLIP_J       (SPRITE_CRASH_FLIP  - 12)      // Flip: Car
#define SPRITE_CRASH_FLIP_MAN1_J  (SPRITE_CRASH_FLIP_MAN1 - 12)  // Flip: Man
#define SPRITE_CRASH_FLIP_GIRL1_J (SPRITE_CRASH_FLIP_GIRL1 - 12) // Flip: Girl
#define SPRITE_CRASH_FLIP_MAN2_J  (SPRITE_CRASH_FLIP_MAN2 - 12)  // Post Flip: Man
#define SPRITE_CRASH_FLIP_GIRL2_J (SPRITE_CRASH_FLIP_GIRL2 - 12) // Post Flip: Girl
#define SPRITE_CRASH_MAN2_J       (SPRITE_CRASH_MAN2 - 12)
#define SPRITE_CRASH_GIRL2_J      (SPRITE_CRASH_GIRL2 - 12)

// Sprite Default Properties
//
// +0: Sprite Properties
// +1: Draw Properties
// +2: Sprite Priority
// +3: Sprite Palette
// +4: Sprite Type
// +6: Sprite X World
// +8: Sprite Y World
// +A: Sprite Z
// +C: Routine Address
#define SPRITE_DEF_PROPS1   0x2B70
#define SPRITE_DEF_PROPS1_J 0x2B64

// Best OutRunners Sprites
#define SPRITE_DEF_PROPS2   0x2FB2
#define SPRITE_DEF_PROPS2_J 0x2FA6

// Sprite: Cloud Frames
#define SPRITE_CLOUD_FRAMES   0x4246
#define SPRITE_CLOUD_FRAMES_J 0x423A

// Sprite: Mini Tree Frames
#define SPRITE_MINITREE_FRAMES   0x435C
#define SPRITE_MINITREE_FRAMES_J 0x4350

// Sprite: Grass Frames (Vary in thickness. Closer to the camera = Need Thicker Sprite.)
#define SPRITE_GRASS_FRAMES   0x4548
#define SPRITE_GRASS_FRAMES_J 0x453C

// Sprite: Sand Frames
#define SPRITE_SAND_FRAMES   0x4588
#define SPRITE_SAND_FRAMES_J 0x457C

// Sprite: Stone Frames
#define SPRITE_STONE_FRAMES   0x45C8
#define SPRITE_STONE_FRAMES_J 0x45BC

// Sprite: Water Frames (Vary in thickness. Closer to the camera = Need Thicker Sprite.)
#define SPRITE_WATER_FRAMES   0x4608
#define SPRITE_WATER_FRAMES_J 0x45FC

// Sprite: Shadow Frames
#define SPRITE_SHDW_FRAMES   0x7862
#define SPRITE_SHDW_FRAMES_J 0x7794

// Sprite: Ferrari Frames, Offsets
#define SPRITE_FERRARI_FRAMES   0x9ECC
#define SPRITE_FERRARI_FRAMES_J 0x9CF2

// Sprite: Frame Data For Ferrari Skid
#define SPRITE_SKID_FRAMES   0x9F1C
#define SPRITE_SKID_FRAMES_J 0x9D3A

// Sprite: Passenger Frames (2 Frames for each, hair up and hair down)
#define SPRITE_PASS_FRAMES   0xA6EC
#define SPRITE_PASS_FRAMES_J 0xA512

// Table of smoke data from wheels
#define SMOKE_DATA   0xACC6
#define SMOKE_DATA_J 0xAAEC

// Table of spray data from wheels
#define SPRAY_DATA   0xAD06
#define SPRAY_DATA_J 0xAB2C

// Sprite: Shadow Data
#define SPRITE_SPRITES_SHADOW_DATA   0x103B6
#define SPRITE_SPRITES_SHADOW_DATA_J 0xFE16

// Sprite: Passenger Skid Frames
#define SPRITE_PASS1_SKIDL   0x1107C
#define SPRITE_PASS1_SKIDR   0x110C2
#define SPRITE_PASS2_SKIDL   0x110CC
#define SPRITE_PASS2_SKIDR   0x11112
#define SPRITE_PASS1_SKIDL_J 0x10ADC
#define SPRITE_PASS1_SKIDR_J 0x10B22
#define SPRITE_PASS2_SKIDL_J 0x10B2C
#define SPRITE_PASS2_SKIDR_J 0x10B72

// Long addresses of sprite data for hardware
#define SPRITE_TYPE_TABLE   0x11ED2
#define SPRITE_TYPE_TABLE_J 0x11932

// Master Sprite Table
//
// Each one of the following addresses contains the following:
//
// [+0] Sprite Frequency Value Bitmask [Word]
// [+2] Reload Value For Sprite Info Offset [Word]
// [+4] Start of table with x,y,type,palette etc.
#define SPRITE_MASTER_TABLE   0x1A43C
#define SPRITE_MASTER_TABLE_J 0x19C96

// OutRun logo data
#define SPRITE_LOGO_BG      0x11162
#define SPRITE_LOGO_CAR     0x1128E
#define SPRITE_LOGO_BIRD1   0x112C0
#define SPRITE_LOGO_BIRD2   0x112F2
#define SPRITE_LOGO_BASE    0x1125C
#define SPRITE_LOGO_TEXT    0x11194
#define SPRITE_LOGO_PALM1   0x111C6
#define SPRITE_LOGO_PALM2   0x111F8
#define SPRITE_LOGO_PALM3   0x1122A
#define SPRITE_LOGO_BG_J    (SPRITE_LOGO_BG - 0x5A0)
#define SPRITE_LOGO_CAR_J   (SPRITE_LOGO_CAR - 0x5A0)
#define SPRITE_LOGO_BIRD1_J (SPRITE_LOGO_BIRD1 - 0x5A0)
#define SPRITE_LOGO_BIRD2_J (SPRITE_LOGO_BIRD2 - 0x5A0)
#define SPRITE_LOGO_BASE_J  (SPRITE_LOGO_BASE - 0x5A0)
#define SPRITE_LOGO_TEXT_J  (SPRITE_LOGO_TEXT - 0x5A0)
#define SPRITE_LOGO_PALM1_J (SPRITE_LOGO_PALM1 - 0x5A0)
#define SPRITE_LOGO_PALM2_J (SPRITE_LOGO_PALM2 - 0x5A0)
#define SPRITE_LOGO_PALM3_J (SPRITE_LOGO_PALM3 - 0x5A0)

// Music Selection Screen - Sprite Data
#define SPRITE_FM_LEFT       0x11892
#define SPRITE_FM_CENTRE     0x1189C
#define SPRITE_FM_RIGHT      0x118A6
#define SPRITE_DIAL_LEFT     0x118B0
#define SPRITE_DIAL_CENTRE   0x118BA
#define SPRITE_DIAL_RIGHT    0x118C4
#define SPRITE_EQ            0x118CE // EQ Sprite, Part of Radio
#define SPRITE_RADIO         0x118D8
#define SPRITE_HAND_LEFT     0x118E2
#define SPRITE_HAND_CENTRE   0x118EC
#define SPRITE_HAND_RIGHT    0x118F6

#define SPRITE_FM_LEFT_J     (SPRITE_FM_LEFT - 0x5A0)
#define SPRITE_FM_CENTRE_J   (SPRITE_FM_CENTRE - 0x5A0)
#define SPRITE_FM_RIGHT_J    (SPRITE_FM_RIGHT - 0x5A0)
#define SPRITE_DIAL_LEFT_J   (SPRITE_DIAL_LEFT - 0x5A0)
#define SPRITE_DIAL_CENTRE_J (SPRITE_DIAL_CENTRE - 0x5A0)
#define SPRITE_DIAL_RIGHT_J  (SPRITE_DIAL_RIGHT - 0x5A0)
#define SPRITE_EQ_J          (SPRITE_EQ - 0x5A0)
#define SPRITE_RADIO_J       (SPRITE_RADIO - 0x5A0)
#define SPRITE_HAND_LEFT_J   (SPRITE_HAND_LEFT - 0x5A0)
#define SPRITE_HAND_CENTRE_J (SPRITE_HAND_CENTRE - 0x5A0)
#define SPRITE_HAND_RIGHT_J  (SPRITE_HAND_RIGHT - 0x5A0)

// Shadow data
#define SPRITE_SHDW_SMALL   0x1193C
#define SPRITE_SHDW_SMALL_J 0x1139C

// Sprite Collision X Offsets [Signed]
// Table is indexed with the type of sprite. 
//
// Format:
// Word 1: X-Left Offset
// Word 2: X-Right Offset
#define SPRITE_X_OFFS 0x1212A

// Sprite Zoom Lookup Table.
// Table Of Longs that represent X & Y Zoom Value
#define SPRITE_ZOOM_LOOKUP 0x28000

#define MOVEMENT_LOOKUP_Z 0x30900

// Table to alter sprite based on its y position.
//
// Input = Y Position
//
// Format:
//
// +0: Frame Number To Use
// +2: Entry In Zoom Lookup Table
#define MAP_Y_TO_FRAME 0x30A00

// ----------------------------------------------------------------------------
// Road
// ----------------------------------------------------------------------------

// Road Data Lookup Table
// Long Addresses where road data is looked up from.
// Note this only contains the road data itself. Namely the x position, and the length of each segment.
//
// The height and width are fetched from elsewhere.
// The sprites etc are fetched from elsewhere.
//
// Same location in both Jap and USA Roms.
//
// Note: Although 1AD92 might not be used, this seems to be a section of road with road split / checkpoint sign
#define ROAD_DATA_LOOKUP 0x1224

// Lookup table of road height information.
// Each entry is a long address into this rom.
#define ROAD_HEIGHT_LOOKUP   0x220A
#define ROAD_HEIGHT_LOOKUP_J 0x21F6

// Data for Road Split (Same Address for Jap and USA Roms)
#define ROAD_DATA_SPLIT 0x3A33E

// Data for Road Bonus (Same Address for Jap and USA Roms)
#define ROAD_DATA_BONUS 0x3ACA0

// Data for Road Background Colour (Same Address for Jap and USA Roms)
#define ROAD_BGCOLOR 0x109EE

// Segment data for End Sequences (Master CPU Code)
#define ROAD_SEG_TABLE_END   0xE514
#define ROAD_SEG_TABLE_END_J 0xE1FE

// Road Segment Table Information (Master CPU Code)
#define ROAD_SEG_TABLE   0xE528
#define ROAD_SEG_TABLE_J 0xE212

// Segment data for Road Split (Master CPU Code)
#define ROAD_SEG_TABLE_SPLIT   0x1DFA4
#define ROAD_SEG_TABLE_SPLIT_J 0x1D5A6


// ----------------------------------------------------------------------------
// Traffic Data
// ----------------------------------------------------------------------------

// Traffic Property Table
#define TRAFFIC_PROPS   0x4CFA
#define TRAFFIC_PROPS_J 0x4CDE

// There are six types of traffic in OutRun:
// Lorry, Pickup, Beetle, BMW, Corvette, Porsche.
//
// Each has 5 directional frames, including horizontal flipping.
// Some vehicles have different frames for inclines.
//
// Format is [0x20 byte boundaries]:
//
// [+0] Straight Frame
// [+4] Straight Frame (same as above)
// [+8] Right Frame
// [+C] Rightmost Frame 
//
// [+10] Straight Frame  [uphill version]
// [+14] Straight Frame  [uphill version] (same as above)
// [+18] Right Frame     [uphill version]
// [+1C] Rightmost Frame [uphill version]

#define TRAFFIC_DATA     0x5424
#define TRAFFIC_DATA_J   0x5362

#define SPRITE_PORSCHE   0xF290
#define SPRITE_PORSCHE_J 0xED22

// ----------------------------------------------------------------------------
// Animation Sequences
// ----------------------------------------------------------------------------

// Flag Waver
#define ANIM_SEQ_FLAG   0x12382
#define ANIM_SEQ_FLAG_J (ANIM_SEQ_FLAG - 0x5A0)

// Ferrari Drive-In Sequence
#define ANIM_FERRARI_CURR   0x12970
#define ANIM_FERRARI_NEXT   0x129C0
#define ANIM_FERRARI_CURR_J (ANIM_FERRARI_CURR - 0x5A0)
#define ANIM_FERRARI_NEXT_J (ANIM_FERRARI_NEXT - 0x5A0)

#define ANIM_PASS1_CURR   0x129C8
#define ANIM_PASS1_NEXT   0x12A18
#define ANIM_PASS1_CURR_J (ANIM_PASS1_CURR - 0x5A0)
#define ANIM_PASS1_NEXT_J (ANIM_PASS1_NEXT - 0x5A0)

#define ANIM_PASS2_CURR   0x12A20
#define ANIM_PASS2_NEXT   0x12A70
#define ANIM_PASS2_CURR_J (ANIM_PASS2_CURR - 0x5A0)
#define ANIM_PASS2_NEXT_J (ANIM_PASS2_NEXT - 0x5A0)

// ----------------------------------------------------------------------------
// End Sequence / Bonus Mode
// ----------------------------------------------------------------------------

// Ferrari Sprite Frame Data For Bonus Mode
//
// +0 [Long]: Address of frame
// +4 [Byte]: Passenger Offset (always 0!)
// +5 [Byte]: Ferrari X Change
// +6 [Byte]: Sprite Colour Palette
// +7 [Byte]: H-Flip
#define ANIM_FERRARI_FRAMES   0xA2F0
#define ANIM_FERRARI_FRAMES_J 0xA116

// Note each table is used by every end sequence animation.
// And contains the variance for that particular animation.
#define ANIM_ENDSEQ_OBJ1      0x124B0 // Ferrari Door Opening
#define ANIM_ENDSEQ_OBJ2      0x124D8 // Ferrari Interior
#define ANIM_ENDSEQ_OBJ3      0x12500 // Car Shadow & Man Sprite
#define ANIM_ENDSEQ_OBJ4      0x12528 // Man Shadow & Female Sprite
#define ANIM_ENDSEQ_OBJ5      0x12550 // Female Shadow & Person Presenting Trophy
#define ANIM_ENDSEQ_OBJ6      0x12578
#define ANIM_ENDSEQ_OBJ7      0x125A0
#define ANIM_ENDSEQ_OBJ8      0x125C8 // Animation Data Sequence For Trophy Person
#define ANIM_ENDSEQ_OBJA      0x125F0
#define ANIM_ENDSEQ_OBJB      0x12618
#define ANIM_END_TABLE        0x123A2 // Start/End Animation Sequence Markers (Only End Markers Appear To Be Used)
#define ANIM_ENDSEQ_OBJ1_J    (ANIM_ENDSEQ_OBJ1 - 0x5A0)
#define ANIM_ENDSEQ_OBJ2_J    (ANIM_ENDSEQ_OBJ2 - 0x5A0)
#define ANIM_ENDSEQ_OBJ3_J    (ANIM_ENDSEQ_OBJ3 - 0x5A0)
#define ANIM_ENDSEQ_OBJ4_J    (ANIM_ENDSEQ_OBJ4 - 0x5A0)
#define ANIM_ENDSEQ_OBJ5_J    (ANIM_ENDSEQ_OBJ5 - 0x5A0)
#define ANIM_ENDSEQ_OBJ6_J    (ANIM_ENDSEQ_OBJ6 - 0x5A0)
#define ANIM_ENDSEQ_OBJ7_J    (ANIM_ENDSEQ_OBJ7 - 0x5A0)
#define ANIM_ENDSEQ_OBJ8_J    (ANIM_ENDSEQ_OBJ8 - 0x5A0)
#define ANIM_ENDSEQ_OBJA_J    (ANIM_ENDSEQ_OBJA - 0x5A0)
#define ANIM_ENDSEQ_OBJB_J    (ANIM_ENDSEQ_OBJB - 0x5A0)
#define ANIM_END_TABLE_J      (ANIM_END_TABLE   - 0x5A0)
// ----------------------------------------------------------------------------
// Course Map Data
// ----------------------------------------------------------------------------

// Course Map Sprite Data
#define SPRITE_COURSEMAP   0x26BE
#define SPRITE_COURSEMAP_J 0x26B2 // Japanese ROMs

// Coursemap Sprite Components
#define SPRITE_COURSEMAP_TOP   0x3784
#define SPRITE_COURSEMAP_BOT   0x386C
#define SPRITE_COURSEMAP_END   0x3954
#define SPRITE_COURSEMAP_TOP_J (SPRITE_COURSEMAP_TOP - 12)
#define SPRITE_COURSEMAP_BOT_J (SPRITE_COURSEMAP_BOT - 12)
#define SPRITE_COURSEMAP_END_J (SPRITE_COURSEMAP_END - 12)

// Minicar
#define SPRITE_MINICAR_RIGHT   0x10C58
#define SPRITE_MINICAR_UP      0x10C62
#define SPRITE_MINICAR_DOWN    0x10C6C
#define SPRITE_MINICAR_RIGHT_J (SPRITE_MINICAR_RIGHT - 0x5A0)
#define SPRITE_MINICAR_UP_J    (SPRITE_MINICAR_UP - 0x5A0)
#define SPRITE_MINICAR_DOWN_J  (SPRITE_MINICAR_DOWN - 0x5A0)

// Convert route information to piece index to colour
#define MAP_ROUTE_LOOKUP 0x3636

// X and Y Movement table for Minicar
#define MAP_MOVEMENT_LEFT 0x3A34
#define MAP_MOVEMENT_RIGHT 0x3AB4

// ----------------------------------------------------------------------------
// Best OutRunners Data
// ----------------------------------------------------------------------------

// Tiles for minicars that run across the text layer
#define TILES_MINICARS1 0xD62A
#define TILES_MINICARS2 0xD670 // smoke tiles

// Default Score Data
#define DEFAULT_SCORES  0xD676

// Alphabet characters for initial entry
#define TILES_ALPHABET  0xD5A4

// ----------------------------------------------------------------------------
// Music Selection Data
// ----------------------------------------------------------------------------

// Palette to cycle the graphical equalizer on the radio
#define MUSIC_EQ_PAL 0xCCAA

// ----------------------------------------------------------------------------
// Data
// ----------------------------------------------------------------------------

// X,Y Offsets of passenger 1 (Man) from car
//
// Format 
//
// Word 1: Frame 1 X Offset
// Word 2: Frame 1 Y Offset 
// Word 3: Frame 1 X Offset H-FLIP
// Word 4: Frame 1 Y Offset H-FLIP
//
// Word 5: Frame 2 X Offset
// Word 6: Frame 2 Y Offset
//
// etc.
#define PASS1_OFFSET 0xA6FC
#define PASS2_OFFSET 0xA75C

// Map stage ID to number 0-14
#define MAP_STAGE_ID 0xDDB4

// Width / Height Table for sprites
#define WH_TABLE 0x20000

// Movement Lookup Table
// Used by:
// - Logo Bird Animation In Attract Mode
// - Car Flip In Crash Scenario
//
// Entries 
// 0     - 0xFF : X Position
// 0x100 - 0x1FF: Y Position
#define DATA_MOVEMENT 0x30800