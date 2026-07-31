/***************************************************************************
    Shared Sound Commands.
    Used by both the ported 68K and Z80 program code.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

    // ----------------------------------------------------------------------------
    // Commands to send from main program code
    // ----------------------------------------------------------------------------

enum
{
    sound_FM_RESET = 0,            // Reset FM Chip (Stop Music etc.)
    sound_RESET  = 0x80,           // Reset sound code
    SOUND_MUSIC_BREEZE = 0x81,     // Music: Passing Breeze
    SOUND_MUSIC_SPLASH = 0x82,     // Music: Splash Wave
    sound_COIN_IN = 0x84,          // Coin IN Effect
    SOUND_MUSIC_MAGICAL = 0x85,    // Music: Magical Sound Shower
    sound_YM_CHECKPOINT = 0x86,    // YM: Checkpoint Ding
    sound_INIT_SLIP = 0x8A,        // Slip (Looped)
    sound_STOP_SLIP = 0x8B,
    sound_INIT_CHEERS = 0x8D,
    sound_STOP_CHEERS = 0x8E,
    sound_CRASH1 = 0x8F,
    sound_REBOUND = 0x90,
    sound_CRASH2 = 0x92,
    sound_NEW_COMMAND = 0x93,
    sound_SIGNAL1 = 0x94,
    sound_SIGNAL2 = 0x95,
    sound_INIT_WEIRD = 0x96,
    sound_STOP_WEIRD = 0x97,
    sound_REVS = 0x98,             // New: Added to support revs during WAV playback
    sound_BEEP1 = 0x99,            // YM Beep
    sound_UFO = 0x9A,              // Unused sound. Note that the z80 code to play this is not implemented in this conversion.
    sound_BEEP2 = 0x9B,            // YM Double Beep
    sound_INIT_CHEERS2     = 0x9C, // Cheers (Looped)   
    sound_VOICE_CHECKPOINT = 0x9D, // Voice: Checkpoint
    sound_VOICE_CONGRATS   = 0x9E, // Voice: Congratulations
    sound_VOICE_GETREADY   = 0x9F, // Voice: Get Ready
    sound_INIT_SAFETYZONE  = 0xA0,
    sound_STOP_SAFETYZONE  = 0xA1,
    sound_YM_SET_LEVELS    = 0xA2,
    // 0xA3 Unused - Should be voice 4, but isn't hooked up
    sound_PCM_WAVE         = 0xA4, // Wave Sample
    SOUND_MUSIC_LASTWAVE   = 0xA5, // Music: Last Wave
    SOUND_MUSIC_BREEZE2    = 0xB0, // Enhancement: Play new music in-game without triggering rev effect
    SOUND_MUSIC_MAGICAL2   = 0xB1,
    SOUND_MUSIC_SPLASH2    = 0xB2,
};

// ----------------------------------------------------------------------------
// Engine Commands to send from main program code
// ----------------------------------------------------------------------------

enum
{
    sound_UNUSED,
    sound_ENGINE_PITCH_H,
    sound_ENGINE_PITCH_L,
    sound_ENGINE_VOL,
    sound_TRAFFIC1,
    sound_TRAFFIC2,
    sound_TRAFFIC3,
    sound_TRAFFIC4,
};
