/***************************************************************************
    Ported Z80 Sound Playing Code.
    Controls Sega PCM and YM2151 Chips.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <stdint.h>
#include <string.h>
#include "engine/audio/osound.h"
#include <stdio.h>

uint8_t OSound_command_input;

uint8_t OSound_engine_data[8];

#define PCM_RAM_SIZE  0x100
#define CHAN_RAM_SIZE 0x800

// Internal channel format
uint8_t chan_ram[CHAN_RAM_SIZE];

// Size of each internal channel entry
const static uint8_t CHAN_SIZE = 0x20;

// Reference to 0xFF bytes of PCM Chip RAM
uint8_t* pcm_ram;

// Bit 0: Set denotes car stationary do rev sample when revs high enough
// Bit 1: Set to denote PCM sound effect triggered.
uint8_t sound_props;

// Stored Command
uint8_t command_index;

// F810 - F813
uint8_t counter1, counter2, counter3, counter4;

// Position in sequence [de]
uint16_t pos;

// Store last command to assist program flow
uint8_t cmd_prev;

// Store last chan ID
uint16_t chanid_prev;

// PCM Channel Commands in RAM to send
const static uint16_t CH09_CMDS1 = 0x570; // 0xFD70;
const static uint16_t CH09_CMDS2 = 0x578;
const static uint16_t CH11_CMDS1 = 0x580; // 0xFD80;
const static uint16_t CH11_CMDS2 = 0x588;

// Panning flags
#define PAN_LEFT  0x40
#define PAN_RIGHT 0x80
#define PAN_CENTRE (PAN_LEFT | PAN_RIGHT)

// ------------------------------------------------------------------------
// ENGINE TONE CODE
// ------------------------------------------------------------------------
// Used to skip the engine code 1/2 times
uint8_t engine_counter;

// Engine Channel: Selects Channel at offset 0xF800 for engine tones
uint8_t engine_channel;

uint8_t OSound_pcm_r(uint16_t adr);
void    OSound_pcm_w(uint16_t adr, uint8_t v);
uint16_t OSound_r16(uint8_t* adr);
void     OSound_w16(uint8_t* adr, uint16_t v);
void OSound_process_command();
void OSound_pcm_backup();
void OSound_check_fm_mapping();
void OSound_process_channels();
void OSound_process_channel(uint16_t chan_id);
void OSound_process_section(uint8_t* chan);
void OSound_calc_end_marker(uint8_t* chan);
void OSound_do_command(uint8_t* chan, uint8_t cmd);
void OSound_new_command();
void OSound_play_pcm_index(uint8_t* chan, uint8_t cmd);
void OSound_setvol(uint8_t* chan);
void OSound_pcm_setpitch(uint8_t* chan);
void OSound_set_loop_adr();
void OSound_do_loop(uint8_t* chan);
void OSound_pcm_finalize(uint8_t* chan);
void OSound_init_sound(uint8_t cmd, uint16_t src, uint16_t dst);
void OSound_process_pcm(uint8_t* chan);
void OSound_pcm_send_cmds(uint8_t* chan, uint16_t pcm_adr, uint8_t channel_pair);
void OSound_fm_dotimera();
void OSound_fm_reset();
void OSound_fm_write_reg_c(uint8_t ix0, uint8_t reg, uint8_t value);
void OSound_fm_write_reg(uint8_t reg, uint8_t value);
void OSound_fm_write_block(uint8_t ix0, uint16_t adr, uint8_t chan);
void OSound_ym_set_levels();
void OSound_ym_set_block(uint8_t* chan);
uint16_t OSound_ym_lookup_data(uint8_t cmd, uint8_t offset, uint8_t block);
void OSound_ym_set_connect(uint8_t* chan, uint8_t pan);
void OSound_ym_finalize(uint8_t* chan);
void OSound_read_mod_table(uint8_t* chan);
void OSound_write_seq_adr(uint8_t* chan);

// ------------------------------------------------------------------------
// ENGINE TONE FUNCTIONS
// ------------------------------------------------------------------------
void OSound_engine_process();
void OSound_engine_process_chan(uint8_t* chan, uint8_t* pcm);
void OSound_vol_thicken(uint16_t* pos, uint8_t* chan, uint8_t* pcm);
uint8_t OSound_get_adjusted_vol(uint16_t* pos, uint8_t* chan);
void OSound_engine_set_pitch(uint16_t* pos, uint8_t* pcm);
void OSound_engine_mute_channel(uint8_t* chan, uint8_t* pcm, uint8_t do_check);
void OSound_unk78c7(uint8_t* chan, uint8_t* pcm);
void OSound_ferrari_vol_pan(uint8_t* chan, uint8_t* pcm);
uint16_t OSound_engine_get_table_adr(uint8_t* chan, uint8_t* pcm);
void OSound_engine_adjust_volume(uint8_t* chan);
uint16_t OSound_engine_set_adr(uint16_t* pos, uint8_t* chan, uint8_t* pcm);
void OSound_engine_set_adr_end(uint16_t* pos, uint16_t loop_adr, uint8_t* chan, uint8_t* pcm);
void OSound_engine_set_pan(uint16_t* pos, uint8_t* chan, uint8_t* pcm);
void OSound_engine_read_data(uint8_t* chan, uint8_t* pcm);

// ----------------------------------------------------------------------------
//                               PASSING TRAFFIC FX 
// ----------------------------------------------------------------------------
void OSound_traffic_process();
void OSound_traffic_process_chan(uint8_t* pcm);
void OSound_traffic_process_entry(uint8_t* pcm);
void OSound_traffic_disable(uint8_t* pcm);
void OSound_traffic_set_vol(uint8_t* pcm);
void OSound_traffic_set_pan(uint8_t* pcm);
uint8_t OSound_traffic_get_vol(uint16_t pos, uint8_t* pcm);
void OSound_traffic_note_changes(uint8_t new_vol, uint8_t* pcm);
void OSound_traffic_read_data(uint8_t* pcm);


// Use YM2151 Timing
#define TIMER_CODE 1

void OSound_init(uint8_t* _pcm_ram)
{
    uint16_t i;
    int8_t j;
    pcm_ram = _pcm_ram;

    OSound_command_input = 0;
    sound_props   = 0;
    pos           = 0;
    counter1      = 0;
    counter2      = 0;
    counter3      = 0;
    counter4      = 0;

    engine_counter= 0;

    // Clear AM RAM 0xF800 - 0xFFFF
    for (i = 0; i < CHAN_RAM_SIZE; i++)
        chan_ram[i] = 0;

    // Enable all PCM channels by default
    if (pcm_ram) {
        for (j = 0; j < 16; j++)
            pcm_ram[0x86 + (j * 8)] = 1; // Channel Active
    }

    OSound_init_fm_chip();
}

// Initialize FM Chip. Initalize and start Timer A.
// Source: 0x79
void OSound_init_fm_chip()
{
   OSound_command_input = sound_RESET;

   // Initialize the FM Chip with the set of default commands
   OSound_fm_write_block(0, z80_adr_YM_INIT_CMDS, 0);

   // Start Timer A & enable its IRQ, and do an IRQ reset (%00110101)
   OSound_fm_write_reg(0x14, 0x35);
}

void OSound_tick()
{
    OSound_fm_dotimera();          // FM: Process Timer A. Stop Timer B
    OSound_process_command();      // Process Command sent by main program code (originally the main 68k processor)
    OSound_process_channels();     // Run logic on individual sound channel (both YM & PCM channels)
    OSound_engine_process();       // Ferrari Engine Tone & Traffic Noise
    OSound_traffic_process();      // Traffic Volume/Panning & Pitch
}

// PCM RAM Read/Write Helper Functions
uint8_t OSound_pcm_r(uint16_t adr)
{
    if (!pcm_ram) return 0;
    return pcm_ram[adr & 0xFF];
}

void OSound_pcm_w(uint16_t adr, uint8_t v)
{
    if (pcm_ram) pcm_ram[adr & 0xFF] = v;
}

// RAM Read/Write Helper Functions
uint16_t OSound_r16(uint8_t* adr)
{
    return ((*(adr+1) << 8) | *adr);
}

void OSound_w16(uint8_t* adr, uint16_t v)
{
    *adr     = v & 0xFF;
    *(adr+1) = v >> 8;
}

void OSound_process_command()
{
    if (OSound_command_input == sound_RESET)
        return;
    else if (OSound_command_input < 0x80 || OSound_command_input >= 0xFF)
    {
        if (OSound_command_input == sound_FM_RESET || OSound_command_input == 0xFF)
            OSound_fm_reset();

        OSound_command_input = sound_RESET;
        OSound_new_command();
    }
    else
    {
        uint8_t cmd   = OSound_command_input;
        OSound_command_input = sound_RESET;

        switch (cmd)
        {
            case sound_RESET:
                break;

            case SOUND_MUSIC_BREEZE:
                sound_props |= BIT_0; // Trigger rev effect
            case SOUND_MUSIC_BREEZE2:
                cmd = SOUND_MUSIC_BREEZE;
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_BREEZE, channel_YM1);
                break;

            case SOUND_MUSIC_SPLASH:
                sound_props |= BIT_0; // Trigger rev effect
            case SOUND_MUSIC_SPLASH2:
                cmd = SOUND_MUSIC_SPLASH;
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_SPLASH, channel_YM1);
                break;

            case sound_COIN_IN:
                OSound_init_sound(cmd, z80_adr_DATA_COININ, channel_YM_FX1);
                break;

            case SOUND_MUSIC_MAGICAL:
                sound_props |= BIT_0; // Trigger rev effect
            case SOUND_MUSIC_MAGICAL2:
                cmd = SOUND_MUSIC_MAGICAL;
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_MAGICAL, channel_YM1);
                break;

            case sound_YM_CHECKPOINT:
                OSound_init_sound(cmd, z80_adr_DATA_CHECKPOINT, channel_YM_FX1);
                break;

            case sound_INIT_SLIP:
                OSound_init_sound(cmd, z80_adr_DATA_SLIP, channel_PCM_FX3);
                break;

            case sound_INIT_CHEERS:
                OSound_init_sound(cmd, z80_adr_DATA_CHEERS, channel_PCM_FX1);
                break;

            case sound_STOP_CHEERS:
                chan_ram[channel_PCM_FX1] = 0;
                chan_ram[channel_PCM_FX2] = 0;
                OSound_pcm_w(0xF08E, 1); // Set inactive flag on channels
                OSound_pcm_w(0xF09E, 1);
                break;

            case sound_CRASH1:
                OSound_init_sound(cmd, z80_adr_DATA_CRASH1, channel_PCM_FX5);
                break;

            case sound_REBOUND:
                OSound_init_sound(cmd, z80_adr_DATA_REBOUND, channel_PCM_FX5);
                break;

            case sound_CRASH2:
                OSound_init_sound(cmd, z80_adr_DATA_CRASH2, channel_PCM_FX5);
                break;

            case sound_NEW_COMMAND:
                OSound_new_command();
                break;

            case sound_SIGNAL1:
                OSound_init_sound(cmd, z80_adr_DATA_SIGNAL1, channel_YM_FX1);
                break;

            case sound_SIGNAL2:
                sound_props &= ~BIT_0; // Clear rev effect
                OSound_init_sound(cmd, z80_adr_DATA_SIGNAL2, channel_YM_FX1);
                break;

            case sound_INIT_WEIRD:
                OSound_init_sound(cmd, z80_adr_DATA_WEIRD, channel_PCM_FX5);
                break;

            case sound_STOP_WEIRD:
                chan_ram[channel_PCM_FX5] = 0;
                chan_ram[channel_PCM_FX6] = 0;
                OSound_pcm_w(0xF0CE, 1); // Set inactive flag on channels
                OSound_pcm_w(0xF0DE, 1);
                break;

            case sound_REVS:
                OSound_fm_reset();
                sound_props |= BIT_0; // Trigger rev effect
                break;

            case sound_BEEP1:
                OSound_init_sound(cmd, z80_adr_DATA_BEEP1, channel_YM_FX1);
                break;

            case sound_UFO:
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_UFO, channel_YM_FX1);
                break;

            case sound_BEEP2:
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_BEEP2, channel_YM1);
                break;

            case sound_INIT_CHEERS2:
                OSound_init_sound(cmd, z80_adr_DATA_CHEERS2, channel_PCM_FX1);
                break;

            case sound_VOICE_CHECKPOINT:
                OSound_pcm_backup();
                OSound_init_sound(cmd, z80_adr_DATA_VOICE1, channel_PCM_FX7);
                break;

            case sound_VOICE_CONGRATS:
                OSound_pcm_backup();
                OSound_init_sound(cmd, z80_adr_DATA_VOICE2, channel_PCM_FX7);
                break;

            case sound_VOICE_GETREADY:
                OSound_pcm_backup();
                OSound_init_sound(cmd, z80_adr_DATA_VOICE3, channel_PCM_FX7);
                break;

            case sound_INIT_SAFETYZONE:
                OSound_init_sound(cmd, z80_adr_DATA_SAFETY, channel_PCM_FX3);
                break;

            case sound_STOP_SLIP:
            case sound_STOP_SAFETYZONE:
                chan_ram[channel_PCM_FX3] = 0;
                chan_ram[channel_PCM_FX4] = 0;
                OSound_pcm_w(0xF0AE, 1); // Set inactive flag on channels
                OSound_pcm_w(0xF0BE, 1);
                break;

            case sound_YM_SET_LEVELS:
                OSound_ym_set_levels();
                break;

            case sound_PCM_WAVE:
                OSound_init_sound(cmd, z80_adr_DATA_WAVE, channel_PCM_FX1);
                break;

            case SOUND_MUSIC_LASTWAVE:
                OSound_init_sound(cmd, z80_adr_DATA_LASTWAVE, channel_YM1);
                break;
        }
    }
}

void OSound_new_command()
{
    uint16_t i;
    if (chan_ram[channel_YM_FX1] & BIT_7)
    {
        chan_ram[channel_YM_FX1] = 0;
        uint16_t adr = z80_adr_YM_LEVEL_CMDS2;
        for (i = 0; i < 4; i++)
        {
            uint8_t reg = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
            uint8_t val = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
            OSound_fm_write_reg(reg, val);
        }
    }

    for (i = channel_PCM_FX1; i < channel_YM_FX1; i++)
        chan_ram[i] = 0;

    uint16_t pcm_enable = 0xF088 + 6;
    for (i = 0; i < 6; i++)
    {
        OSound_pcm_w(pcm_enable, OSound_pcm_r(pcm_enable) | BIT_0); 
        pcm_enable += 0x10; 
    }
}

void OSound_pcm_backup()
{
    if (sound_props & BIT_1)
        return;
    if (pcm_ram) {
        memcpy(chan_ram + CH09_CMDS1, pcm_ram + 0x40, 8); 
        memcpy(chan_ram + CH09_CMDS2, pcm_ram + 0xC0, 8);
        memcpy(chan_ram + CH11_CMDS1, pcm_ram + 0x50, 8); 
        memcpy(chan_ram + CH11_CMDS1, pcm_ram + 0xD0, 8);
    }
    sound_props |= BIT_1; 
}

void OSound_check_fm_mapping()
{
    uint8_t c;
    uint16_t chan_id = channel_MAP1;
    for (c = 0; c < 8; c++)
    {
        if (chan_ram[chan_id] & BIT_7)    
            chan_ram[chan_id - 0x2C0] |= BIT_2;
        chan_id += CHAN_SIZE;
    }
}

void OSound_process_channels()
{
    uint8_t c;
    OSound_check_fm_mapping();
    uint16_t chan_id = channel_YM1;
    for (c = 0; c < 30; c++)
    {
        if (chan_ram[chan_id] & BIT_7)
            OSound_process_channel(chan_id);
        chan_id += CHAN_SIZE; 
    }
}

void OSound_process_channel(uint16_t chan_id)
{
    chanid_prev = chan_id;
    uint8_t* chan = &chan_ram[chan_id];
    pos = OSound_r16(&chan[ch_SEQ_POS]) + 1;
    OSound_w16(&chan[ch_SEQ_POS], pos);
    uint16_t seq_end = OSound_r16(&chan[ch_SEQ_END]);

    if (pos == seq_end)
    {
        pos = OSound_r16(&chan[ch_SEQ_CMD]);
        OSound_process_section(chan);
        if (cmd_prev == 0x84 || cmd_prev == 0x99) 
            return;
    }

    if (chan[ch_FM_FLAGS] & BIT_6) return;

    uint8_t reg;
    uint8_t chan_index = chan[ch_FM_FLAGS] & 7;

    if (chan[ch_FM_PHASETBL])
    {
        OSound_read_mod_table(chan);
    }

    if (chan[ch_FM_NOTE] == 0xFF)
    {
        OSound_fm_write_reg_c(chan[ch_FLAGS], 8, chan_index);
        return;
    }

    if (chan[ch_FLAGS] & BIT_1)
    {
        reg = 0xF; 
    }
    else
    {
        OSound_fm_write_reg_c(chan[ch_FLAGS], 0x30 + chan_index, chan[ch_FM_PHASETBL] ? chan[ch_FM_PHASE_AMP] : 0);
        reg = 0x28 + chan_index;
    }

    OSound_fm_write_reg_c(chan[ch_FLAGS], reg, chan[ch_FM_NOTE]);

    if (OSound_r16(&chan[ch_SEQ_POS])) return;

    OSound_fm_write_reg_c(chan[ch_FLAGS], 8, chan_index);
    OSound_fm_write_reg_c(chan[ch_FLAGS], 8, chan_index | 0x78);
}

void OSound_process_section(uint8_t* chan)
{
    uint8_t cmd = RomLoader_read8IncP_addr16(&Roms_z80, &pos);
    cmd_prev = cmd;
    if (cmd >= 0x80)
    {
        OSound_do_command(chan, cmd);
        return;
    }

    if (cmd)
    {
        uint16_t adr = z80_adr_YM_NOTE_OCTAVE + (cmd - 1 + (int8_t) chan[ch_NOTE_OFFSET]);
        chan[ch_FM_NOTE] = RomLoader_read8_addr16(&Roms_z80, adr);
    }
    else if (!(chan[ch_FM_FLAGS] & BIT_6))
    {
        chan[ch_FM_NOTE] = 0xFF;
    }

    OSound_calc_end_marker(chan);
}

void OSound_calc_end_marker(uint8_t* chan)
{
    uint16_t end_marker = RomLoader_read8_addr16(&Roms_z80, pos);
    
    if (chan[ch_FM_MARKER] & BIT_1)
    {
        if (chan[ch_FM_MARKER] & BIT_0)
        {
            chan[ch_FM_MARKER] &= ~BIT_0;
            end_marker += (RomLoader_read8_addr16(&Roms_z80, ++pos) << 8); 
        }
    }
    else
    {
        end_marker = chan[ch_END_MARKER] * end_marker;
    }
  
    OSound_w16(&chan[ch_SEQ_END], end_marker); 
    OSound_w16(&chan[ch_SEQ_CMD], ++pos);      
    OSound_w16(&chan[ch_UNKNOWN], 0);
    OSound_w16(&chan[ch_SEQ_POS], 0);
}

void OSound_do_command(uint8_t* chan, uint8_t cmd)
{
    if (cmd >= 0xBF)
    {
        OSound_play_pcm_index(chan, cmd);
        return;
    }

    switch (cmd & 0x3F)
    {
        case 0x02:
            OSound_setvol(chan);
            break;
        case 0x04:
            OSound_ym_finalize(chan);
            return;
        case 0x07:
            chan[ch_FM_PHASETBL] = RomLoader_read8_addr16(&Roms_z80, pos);
            break;
        case 0x08:
            OSound_write_seq_adr(chan);
            break;
        case 0x09:
            pos = OSound_r16(&chan[chan[ch_MEM_OFFSET]]);
            chan[ch_MEM_OFFSET] += 2;
            break;
        case 0x0A:
            OSound_set_loop_adr();
            break;
        case 0x0B:
            chan[ch_NOTE_OFFSET] += RomLoader_read8_addr16(&Roms_z80, pos);
            break;
        case 0x0C:
            OSound_do_loop(chan);
            break;
        case 0x11:
            OSound_ym_set_block(chan);
            break;
        case 0x13:
            OSound_pcm_setpitch(chan);
            break;
        case 0x14:
            chan[ch_FM_MARKER] |= BIT_1;
            pos--;
            break;
        case 0x15:
            chan[ch_FM_MARKER] |= BIT_0;
            pos--;
            break;
        case 0x16:
            OSound_ym_set_connect(chan, PAN_RIGHT);
            break;
        case 0x17:
            OSound_ym_set_connect(chan, PAN_LEFT);
            break;
        case 0x18:
            OSound_ym_set_connect(chan, PAN_CENTRE);
            break;
        case 0x19:
            OSound_pcm_finalize(chan);
            return;
    }

    pos++;
    OSound_process_section(chan);
}

void OSound_setvol(uint8_t* chan)
{  
    uint8_t vol_l = RomLoader_read8_addr16(&Roms_z80, pos);
    if (chan[ch_FM_FLAGS] & BIT_6)
    {
        const uint8_t VOL_MAX = 0x40;
        chan[ch_VOL_L] = vol_l > VOL_MAX ? 0 : vol_l;
        uint8_t vol_r = RomLoader_read8_addr16(&Roms_z80, ++pos);
        chan[ch_VOL_R] = vol_r > VOL_MAX ? 0 : vol_r;
    }
    else
    {
        chan[ch_FM_MARKER] = vol_l;
    }
}

void OSound_pcm_setpitch(uint8_t* chan)
{
    if (chan[ch_FM_FLAGS] & BIT_6)
        chan[ch_PCM_PITCH] = RomLoader_read8_addr16(&Roms_z80, pos);
}

void OSound_set_loop_adr()
{
    pos = RomLoader_read16_addr16(&Roms_z80, pos) - 1;
}

void OSound_do_loop(uint8_t* chan)
{
    uint8_t offset = RomLoader_read8_addr16(&Roms_z80, pos++) + 0x18;
    uint8_t a = chan[offset];
    if (a == 0)
    {
        chan[offset] = RomLoader_read8_addr16(&Roms_z80, pos);
    }
    pos++;
    if (--chan[offset] != 0)
        OSound_set_loop_adr();
    else
        pos++;
}

void OSound_pcm_finalize(uint8_t* chan)
{
    sound_props &= ~BIT_1; 
    if (pcm_ram) {
        memcpy(pcm_ram + 0x40, chan_ram + CH09_CMDS1, 8); 
        memcpy(pcm_ram + 0xC0, chan_ram + CH09_CMDS2, 8);
        memcpy(pcm_ram + 0x50, chan_ram + CH11_CMDS1, 8); 
        memcpy(pcm_ram + 0xD0, chan_ram + CH11_CMDS1, 8);
    }
    OSound_ym_finalize(chan);
}

const static uint16_t PCM_PERCUSSION [] =
{
    0x17C0, 0x42, 0x84, 0xD6,
    0x302F, 0x3A, 0x84, 0xC6,
    0x0090, 0x0B, 0x84, 0xC2,
    0x00F0, 0x08, 0x84, 0xD2,
    0x49DE, 0x4C, 0x84, 0xD2,
    0x437C, 0x48, 0x84, 0xD2,
    0x29AB, 0x2E, 0x84, 0xC2,
    0x1C03, 0x28, 0x84, 0xC2,
    0x0951, 0x16, 0x84, 0xD2,
    0x3BE6, 0x7F, 0x90, 0xC2,
    0x5830, 0x5F, 0x84, 0xD2,
    0x4DA0, 0x57, 0x84, 0xD2,
    0x0C1D, 0x1B, 0x78, 0xC2,
    0x6002, 0x7F, 0x84, 0xD2,
    0x0C1D, 0x1B, 0x40, 0xC2,
};

void OSound_play_pcm_index(uint8_t* chan, uint8_t cmd)
{
    if (cmd == 0)
    {
        OSound_calc_end_marker(chan);
        return;
    }

    if (cmd >= 0xD0)
    {
        uint16_t pcm_index = z80_adr_PCM_INFO + ((cmd - 0xD0) << 2);
        chan[ch_PCM_ADR1L] = RomLoader_read8IncP_addr16(&Roms_z80, &pcm_index);           
        chan[ch_PCM_ADR1H] = RomLoader_read8IncP_addr16(&Roms_z80, &pcm_index);
        chan[ch_PCM_ADR2]  = RomLoader_read8IncP_addr16(&Roms_z80, &pcm_index);           
        chan[ch_CTRL]      = RomLoader_read8IncP_addr16(&Roms_z80, &pcm_index);           
    }
    else
    {
        uint16_t pcm_index = (cmd - 0xC0) << 2;
        OSound_w16(&chan[ch_PCM_ADR1L], PCM_PERCUSSION[pcm_index]);        
        chan[ch_PCM_ADR2]  = (uint8_t) PCM_PERCUSSION[pcm_index+1]; 
        chan[ch_PCM_PITCH] = (uint8_t) PCM_PERCUSSION[pcm_index+2]; 
        chan[ch_CTRL]      = (uint8_t) PCM_PERCUSSION[pcm_index+3]; 
    }
    OSound_process_pcm(chan);
}

void OSound_init_sound(uint8_t cmd, uint16_t src, uint16_t dst)
{
    int ch, i;
    command_index = cmd - 0x81;
    src = RomLoader_read16_addr16(&Roms_z80, src);
    uint8_t channels = RomLoader_read8IncP_addr16(&Roms_z80, &src);
    for ( ch = 0; ch < channels; ch++)
    {
        uint16_t adr = RomLoader_read16IncP_addr16(&Roms_z80, &src);
        for (i = 0; i < 0xE; i++)
            chan_ram[dst++] = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
        chan_ram[dst++] = command_index;
        for (i = 0xF; i < CHAN_SIZE; i++)
            chan_ram[dst++] = 0;
    }
}

void OSound_process_pcm(uint8_t* chan)
{
    int i;
    if (chan[ch_CTRL] & BIT_7)
    {
        const uint16_t BASE_ADR = 0xF088; 
        const uint16_t CHAN_SIZE = 0x10;  
        uint16_t adr = BASE_ADR; 
        if (chan[ch_CTRL] & BIT_2)
        {
            for (i = 0; i < 6; i++)
            {
                uint8_t channel_pair = OSound_pcm_r(adr + 6);
                if ((channel_pair & 0x84) == 0x84 && (channel_pair & BIT_0) == 0)
                {
                    OSound_pcm_send_cmds(chan, adr, channel_pair);
                    return;
                }
                adr += CHAN_SIZE; 
            }
            adr = BASE_ADR;
        }
        for (i = 0; i < 6; i++)
        {
            uint8_t channel_pair = OSound_pcm_r(adr + 6);
            if (channel_pair & BIT_0)
            {
                OSound_pcm_send_cmds(chan, adr, channel_pair);
                return;
            }
            adr += CHAN_SIZE; 
        }
        adr = BASE_ADR;
        for (i = 0; i < 6; i++)
        {
            uint8_t channel_pair = OSound_pcm_r(adr + 6);
            if (channel_pair & BIT_7)
            {
                OSound_pcm_send_cmds(chan, adr, channel_pair);
                return;
            }
            adr += CHAN_SIZE; 
        }
        OSound_calc_end_marker(chan);
    }
    else
    {
        uint8_t channel_pair = chan[ch_CTRL] & 0xC;
        uint8_t selected = 0;
        if (channel_pair < 4)
        {
            sound_props |= BIT_1;
            if (++counter1 & BIT_0)
            {
                selected = 10;      
                OSound_pcm_w(0xF0D6, channel_pair = 1); 
            }
            else
            {
                selected = 8;
                OSound_pcm_w(0xF0C6, channel_pair = 1); 
            }
        }
        else if (channel_pair == 4)
        {
            if (++counter2 & BIT_0) selected = 3;      
            else selected = 1;
        }
        else if (channel_pair == 8)
        {
            if (++counter3 & BIT_0) selected = 11;      
            else selected = 9;
        }
        else
        {
            if (++counter4 & BIT_0) selected = 7;      
            else selected = 5;
        }
        uint16_t pcm_adr = 0xF080 + (selected * 8);
        OSound_pcm_send_cmds(chan, pcm_adr, channel_pair);
    }
}

void OSound_pcm_send_cmds(uint8_t* chan, uint16_t pcm_adr, uint8_t channel_pair)
{
    OSound_pcm_w(pcm_adr + 0x80, channel_pair);        
    OSound_pcm_w(pcm_adr + 0x82, chan[ch_VOL_L]);     
    OSound_pcm_w(pcm_adr + 0x83, chan[ch_VOL_R]);     
    OSound_pcm_w(pcm_adr + 0x84, chan[ch_PCM_ADR1L]); 
    OSound_pcm_w(pcm_adr + 0x4,  chan[ch_PCM_ADR1L]); 
    OSound_pcm_w(pcm_adr + 0x85, chan[ch_PCM_ADR1H]); 
    OSound_pcm_w(pcm_adr + 0x5,  chan[ch_PCM_ADR1H]); 
    OSound_pcm_w(pcm_adr + 0x86, chan[ch_PCM_ADR2]);  
    OSound_pcm_w(pcm_adr + 0x87, chan[ch_PCM_PITCH]); 
    OSound_pcm_w(pcm_adr + 0x6,  chan[ch_CTRL]);      
    OSound_calc_end_marker(chan);
}

void OSound_fm_dotimera()
{
    #ifdef TIMER_CODE
    if (!(YM_read_status() & BIT_0))
        return;
    #endif
    YM_write_reg(0x14, 0x15); 
}

void OSound_fm_reset()
{
    uint16_t i;
    for (i = channel_YM1; i < channel_PCM_FX1; i++)
        chan_ram[i] = 0;
    OSound_fm_write_block(0, z80_adr_YM_INIT_CMDS, 0);
}

void OSound_fm_write_reg_c(uint8_t ix0, uint8_t reg, uint8_t value)
{
    if (ix0 & BIT_2)
        return;
    OSound_fm_write_reg(reg, value);
}

void OSound_fm_write_reg(uint8_t reg, uint8_t value)
{
    #ifdef TIMER_CODE
    // Return if YM2151 is busy
    if (YM_read_status() & BIT_7)
        return;
    #endif
    YM_write_reg(reg, value);
}
void OSound_fm_write_block(uint8_t ix0, uint16_t adr, uint8_t chan)
{
    if (Roms_z80.rom == NULL) return;
    while (1)
    {
        uint8_t cmd = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
        if (cmd == 2) return;
        if (cmd == 3)
        {
            adr = RomLoader_read16_addr16(&Roms_z80, adr);
        }
        else
        {
            uint8_t reg = cmd + chan;
            uint8_t val = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
            OSound_fm_write_reg_c(0, reg, val);
        }
    }
}

void OSound_ym_set_levels()
{
    uint16_t i;
    for (i = channel_YM1; i < channel_PCM_FX1; i++)
        chan_ram[i] = 0;
    uint8_t entries = (chan_ram[channel_YM_FX1] & BIT_7) ? 28 : 32;
    uint16_t adr = z80_adr_YM_LEVEL_CMDS1;
    for (i = 0; i < entries; i++)
    {
        uint8_t reg = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
        uint8_t val = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
        OSound_fm_write_reg(reg, val);
    }
}

const static uint16_t FM_DATA_TABLE[] =
{
    z80_adr_DATA_BREEZE,
    z80_adr_DATA_SPLASH,
    0,
    z80_adr_DATA_COININ,
    z80_adr_DATA_MAGICAL,
    z80_adr_DATA_CHECKPOINT,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    z80_adr_DATA_SIGNAL1,
    z80_adr_DATA_SIGNAL2,
    0, 0, 0,
    z80_adr_DATA_BEEP1,
    z80_adr_DATA_UFO,
    z80_adr_DATA_BEEP2,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    z80_adr_DATA_LASTWAVE,
};

void OSound_ym_set_block(uint8_t* chan)
{
    chan[ch_FM_BLOCK] = RomLoader_read8_addr16(&Roms_z80, pos);
    if (!chan[ch_FM_BLOCK])
        return;
    uint16_t adr = OSound_ym_lookup_data(chan[ch_COMMAND], 3, chan[ch_FM_BLOCK]); 
    OSound_fm_write_block(chan[ch_FLAGS], adr, chan[ch_FM_FLAGS] & 7);
}

uint16_t OSound_ym_lookup_data(uint8_t cmd, uint8_t offset, uint8_t block)
{
    block = (block - 1) << 1;
    uint16_t adr = RomLoader_read16_addr16(&Roms_z80, (uint16_t) (FM_DATA_TABLE[cmd] + (offset << 1)));
    return RomLoader_read16_addr16(&Roms_z80, (uint16_t) (adr + block));
}

void OSound_ym_set_connect(uint8_t* chan, uint8_t pan)
{
    uint8_t block = chan[ch_FM_BLOCK];                          
    uint16_t adr = OSound_ym_lookup_data(chan[ch_COMMAND], 3, block);  
    adr += 0x33;
    uint8_t chan_ctrl_reg = (chan[ch_FM_FLAGS] & 7) + 0x20;
    uint8_t reg_value = (RomLoader_read8_addr16(&Roms_z80, adr) & 0x3F) | pan;
    pos--;
    OSound_fm_write_reg_c(chan[ch_FLAGS], chan_ctrl_reg, reg_value);
}

void OSound_ym_finalize(uint8_t* chan)
{
    uint8_t chan_index = chan[ch_FM_FLAGS] & 7;
    OSound_fm_write_block(chan[ch_FLAGS], z80_adr_YM_RELEASE_RATE, chan_index);
    OSound_fm_write_reg(0x8, chan_index);
    OSound_fm_write_reg(0xf, 0);
    chan[ch_FLAGS] = 0;
    if (chanid_prev < channel_MAP1) return;
    chan -= 0x2C0; 
    if (!(chan[ch_FLAGS] & BIT_7)) return;
    chan[ch_FLAGS] &= ~BIT_2;
    uint8_t block = chan[ch_FM_BLOCK];
    if (!block) return;
    uint16_t adr = OSound_ym_lookup_data(chan[ch_COMMAND], 3, block);
    OSound_fm_write_block(chan[ch_FLAGS], adr, chan[ch_FM_FLAGS] & 7);
}

void OSound_read_mod_table(uint8_t* chan)
{
    uint16_t adr = OSound_ym_lookup_data(chan[ch_COMMAND], 2, chan[ch_FM_PHASETBL]); 
    while (1)
    {
        uint16_t offset = chan[ch_FM_PHASEOFF];
        uint8_t table_entry = RomLoader_read8_addr16(&Roms_z80, (uint16_t) (adr + offset));
        if (table_entry == 0xFD) chan[ch_FM_PHASEOFF] = 0;
        else if (table_entry == 0xFE) chan[ch_FM_PHASEOFF]--;
        else
        {
            chan[ch_FM_PHASEOFF]++;
            uint8_t carry = (table_entry < 0xFC) ? 2 : 0;
            chan[ch_FM_PHASE_AMP] = ((table_entry << 2) + carry) + ((table_entry & 0x80) >> 7);
            return;
        }
    }
}

void OSound_write_seq_adr(uint8_t* chan)
{
    uint16_t value = RomLoader_read16_addr16(&Roms_z80, pos);
    pos++;
    chan[ch_MEM_OFFSET]--;
    uint8_t offset = chan[ch_MEM_OFFSET];
    chan[ch_MEM_OFFSET]--;
    chan[offset]   = pos >> 8;
    chan[offset-1] = pos & 0xFF;
    pos = value - 1;
}

void OSound_engine_process()
{
    if ((++engine_counter & 1) == 0) return;
    uint16_t ix = 0;                    
    uint16_t iy = channel_ENGINE_CH1;  
    for (engine_channel = 6; engine_channel > 0; engine_channel--)
    {
        OSound_engine_process_chan(&chan_ram[iy], &pcm_ram[ix]);
        ix += 0x10;
        iy += CHAN_SIZE;
    }
}

void OSound_engine_process_chan(uint8_t* chan, uint8_t* pcm)
{
    if (engine_channel < 3) { if (sound_props & BIT_1) return; }
    OSound_engine_read_data(chan, pcm);
    if (sound_props & BIT_0)
    {
        uint16_t revs = OSound_r16(pcm);
        if (revs == 0) { OSound_engine_mute_channel(chan, pcm, 1); return; }
        if (revs >= 0xFA)
        {
            if (!(pcm[0x86] & BIT_0)) return;
            if (engine_channel < 3) { pcm[2] = 0x20; pcm[3] = 0; pcm[7] = 0x41; }
            else if (engine_channel < 5) { pcm[2] = 0x10; pcm[3] = 0x10; pcm[7] = 0x42; }
            else { pcm[2] = 0x20; pcm[3] = 0; pcm[7] = 0x40; }
            pcm[4] = pcm[0x84] = 0; pcm[5] = pcm[0x85] = 0x36; pcm[6] = 0x55; pcm[0x86] = 2;
            return;
        }
    }
    if (engine_channel & BIT_0) OSound_init_fm_chip(); // Was OSound_unk78c7, changed to match original intent
    if (!chan[ch_engines_VOL0]) { OSound_engine_mute_channel(chan, pcm, 1); return; }
    if (chan[ch_engines_VOL0] == chan[ch_engines_VOL1]) chan[ch_engines_FLAGS] |= BIT_1; 
    else { chan[ch_engines_FLAGS] &= ~BIT_1; chan[ch_engines_VOL1] = chan[ch_engines_VOL0]; }
    uint16_t revs = OSound_r16(pcm);
    if (revs == 0) { OSound_engine_mute_channel(chan, pcm, 1); return; }
    uint16_t old_revs = OSound_r16(pcm + 0x80);
    if (revs == old_revs) chan[ch_engines_FLAGS] |= BIT_0;
    else 
    {
        if (revs - old_revs < 0) chan[ch_engines_FLAGS] &= ~BIT_2;
        else chan[ch_engines_FLAGS] |= BIT_2;
        chan[ch_engines_FLAGS] &= ~BIT_0; 
        OSound_w16(pcm + 0x80, revs);
    }
    chan[ch_engines_ACTIVE] &= ~BIT_0; 
    if ((chan[ch_engines_FLAGS] & BIT_0) && chan[ch_engines_FLAGS] & BIT_1) return;
    if (engine_channel >= 5)
    {
        int16_t off = OSound_r16(pcm) - 0x30;
        if (off >= 0) {
            if (chan[ch_engines_FLAGS] & BIT_4) { chan[ch_engines_FLAGS] &= ~BIT_3; chan[ch_engines_FLAGS] &= ~BIT_4; }
            OSound_ferrari_vol_pan(chan, pcm); return;
        } else {
            if (!(chan[ch_engines_FLAGS] & BIT_4)) { chan[ch_engines_FLAGS] &= ~BIT_3; chan[ch_engines_FLAGS] |=  BIT_4; }
        }
    }
    uint16_t engine_pos = OSound_engine_get_table_adr(chan, pcm); 
    if (chan[ch_engines_FLAGS] & BIT_5) { OSound_engine_mute_channel(chan, pcm, 0); return; }
    if (chan[ch_engines_FLAGS] & BIT_0) engine_pos += 2;
    else { uint16_t start_adr = OSound_engine_set_adr(&engine_pos, chan, pcm); OSound_engine_set_adr_end(&engine_pos, start_adr, chan, pcm); }
    OSound_vol_thicken(&engine_pos, chan, pcm); OSound_engine_set_pitch(&engine_pos, pcm); pcm[0x86] = 0; 
}

void OSound_unk78c7(uint8_t* chan, uint8_t* pcm)
{
    uint16_t adr = (engine_channel == 1) ? 0xFD10 : (engine_channel == 3) ? 0xFD30 : 0xFD50;
    uint16_t adr_offset = (adr + (chan[ch_engines_OFFSET] * 3)) & 0x7FF;
    chan_ram[adr_offset++] = pcm[0x0]; chan_ram[adr_offset++] = pcm[0x1]; chan_ram[adr_offset++] = chan[ch_engines_VOL0];
    if (++chan[ch_engines_OFFSET] >= 8) { chan[ch_engines_OFFSET] = 0; adr_offset = adr & 0x7FF; }
    pcm[0x0] = chan_ram[adr_offset++]; pcm[0x1] = chan_ram[adr_offset++]; chan[ch_engines_VOL0] = chan_ram[adr_offset++];
}

void OSound_ferrari_vol_pan(uint8_t* chan, uint8_t* pcm)
{
    OSound_engine_adjust_volume(chan);
    int16_t pitch_table_index = OSound_r16(pcm + 0x80) - 0x30;
    if (pitch_table_index < 0) { OSound_engine_mute_channel(chan, pcm, 0); return; }
    OSound_w16(chan + ch_engines_PITCH_L, pitch_table_index);
    uint16_t pos = z80_adr_ENGINE_ADR_TABLE;
    OSound_engine_set_adr(&pos, chan, pcm);
    pcm[0x6] = RomLoader_read8_addr16(&Roms_z80, ++pos);
    OSound_engine_set_pan(&pos, chan, pcm);
    pos = z80_adr_ENGINE_ADR_TABLE + 4;
    uint16_t pitch = RomLoader_read8_addr16(&Roms_z80, pos); 
    pitch += OSound_r16(chan + ch_engines_PITCH_L) >> 1;
    if (pitch > 0xFF) pitch = 0xFF;
    if (engine_channel & BIT_0) pitch -= 2;
    pcm[0x7] = (uint8_t) pitch; pcm[0x86] = 0x10; 
}

uint16_t OSound_engine_get_table_adr(uint8_t* chan, uint8_t* pcm)
{
    int16_t off = OSound_r16(pcm + 0x80) - 0x52;
    int16_t table_offset;
    if (off < 0) { chan[ch_engines_FLAGS] &= ~BIT_5; table_offset = OSound_r16(pcm + 0x80); OSound_w16(pcm + 0x82, 0); }
    else { chan[ch_engines_FLAGS] |= BIT_5; table_offset = 1; OSound_w16(pcm + 0x82, off); }
    table_offset--;
    return (z80_adr_ENGINE_ADR_TABLE + 5) + (table_offset * 5); 
}

uint16_t OSound_engine_set_adr(uint16_t* pos, uint8_t* chan, uint8_t* pcm)
{
    uint16_t start_adr = RomLoader_read16_addr16(&Roms_z80, (*pos)++);
    OSound_w16(pcm + 0x4, start_adr);
    if (engine_channel < 5)
    {
        if (chan[ch_engines_FLAGS] & BIT_5) {
            if (chan[ch_engines_FLAGS] & BIT_6) {
                chan[ch_engines_FLAGS] |= BIT_6; chan[ch_engines_FLAGS] |= BIT_3; OSound_w16(pcm + 0x84, start_adr); return start_adr;
            }
        } else chan[ch_engines_FLAGS] &= ~BIT_6;
    }
    if (chan[ch_engines_FLAGS] & BIT_3) return start_adr;
    chan[ch_engines_FLAGS] |= BIT_3; OSound_w16(pcm + 0x84, start_adr); return start_adr;
}

void OSound_engine_set_adr_end(uint16_t* pos, uint16_t loop_adr, uint8_t* chan, uint8_t* pcm)
{
    pcm[0x6] = RomLoader_read8_addr16(&Roms_z80, ++(*pos));
    if (chan[ch_engines_FLAGS] & BIT_2) return;
    if (pcm[0x6] >= pcm[0x85]) return;
    OSound_w16(pcm + 0x84, loop_adr);
}

void OSound_vol_thicken(uint16_t* pos, uint8_t* chan, uint8_t* pcm)
{
    (*pos)++;
    if (engine_channel & BIT_0) { pcm[0x2] = pcm[0x82] & BIT_5 ? 0 : OSound_get_adjusted_vol(pos, chan); pcm[0x3] = 0; }
    else { pcm[0x2] = 0; pcm[0x3] = pcm[0x83] & BIT_5 ? 0 : OSound_get_adjusted_vol(pos, chan); }
}

uint8_t OSound_get_adjusted_vol(uint16_t* pos, uint8_t* chan)
{
    uint8_t multiply =  RomLoader_read8IncP_addr16(&Roms_z80, pos);
    uint16_t vol = (chan[ch_engines_VOL1] * multiply) >> 6;
    if (vol > 0x3F) vol = 0x3F;
    return (uint8_t) vol;
}

void OSound_engine_set_pitch(uint16_t* pos, uint8_t* pcm)
{
    (*pos)++;
    uint16_t bc = OSound_r16(pcm + 0x82);
    bc >>= 2;
    if (bc & 0xFF00) bc = (bc & 0xFF00) | 0xFF;
    uint16_t pitch = RomLoader_read8IncP_addr16(&Roms_z80, pos);
    if (bc) { pitch += (bc & 0xFF); if (pitch > 0xFF) pitch = 0xFC; }
    if (engine_channel & BIT_0) pcm[0x7] = (uint8_t) pitch;
    else pcm[0x7] = (uint8_t) pitch + 3;
}

void OSound_engine_mute_channel(uint8_t* chan, uint8_t* pcm, uint8_t do_check)
{
    if (do_check && (chan[ch_engines_ACTIVE] & BIT_0)) return;
    chan[ch_engines_ACTIVE] |= BIT_0;
    pcm[0x02] = pcm[0x03] = pcm[0x07] = 0; pcm[0x86] |= BIT_0; 
    chan[ch_engines_VOL0] = chan[ch_engines_VOL1] = chan[ch_engines_FLAGS] = 0;
    chan[ch_engines_PITCH_L] = chan[ch_engines_PITCH_H] = chan[ch_engines_VOL6] = 0;
}

void OSound_engine_adjust_volume(uint8_t* chan)
{
    uint16_t vol = (chan[ch_engines_VOL1] * 0x18) >> 6;
    if (vol > 0x3F) vol = 0x3F;
    chan[ch_engines_VOL6] = (uint8_t) vol;
}   

void OSound_engine_set_pan(uint16_t* pos, uint8_t* chan, uint8_t* pcm)
{
    uint16_t pitch = OSound_r16(chan + ch_engines_PITCH_L) >> 1;
    pitch += RomLoader_read8_addr16(&Roms_z80, ++(*pos));
    uint16_t vol = (chan[ch_engines_VOL1] * pitch) >> 6;
    if (vol > 0x3F) vol = 0x3F;
    if (vol >= chan[ch_engines_VOL6]) vol = chan[ch_engines_VOL6];
    if (engine_channel & BIT_0) { pcm[0x2] = (uint8_t) vol; pcm[0x3] = (uint8_t) vol >> 1; }
    else { pcm[0x2] = (uint8_t) vol >> 1; pcm[0x3] = (uint8_t) vol; }
}

void OSound_engine_read_data(uint8_t* chan, uint8_t* pcm)
{
    uint16_t pitch = (OSound_engine_data[sound_ENGINE_PITCH_H] << 8) + OSound_engine_data[sound_ENGINE_PITCH_L];
    pitch = (pitch >> 5) & 0x1FF;
    pcm[0x0] = pitch & 0xFF; pcm[0x1] = (pitch >> 8) & 0xFF;
    chan[ch_engines_VOL0] = OSound_engine_data[sound_ENGINE_VOL];
}

void OSound_traffic_process()
{
    if ((engine_counter & 1) == 0) return;
    uint16_t pcm_adr = 0x60; 
    for (engine_channel = 4; engine_channel > 0; engine_channel--)
    {
        OSound_traffic_process_chan(&pcm_ram[pcm_adr]);
        pcm_adr += 0x8; 
    }
}

void OSound_traffic_process_chan(uint8_t* pcm)
{
    if (!(pcm[0x82] & BIT_4))
    {
        OSound_traffic_read_data(pcm); 
        uint8_t vol = pcm[0x00];
        if (vol) {
            OSound_traffic_note_changes(vol, pcm); 
            uint8_t flags = pcm[0x82];
            if (!(flags & BIT_0) || !(flags & BIT_1)) { OSound_traffic_process_entry(pcm); return; }
            else if (flags & BIT_2) return;
            OSound_traffic_process_entry(pcm); return;
        } else {
            if (!(pcm[0x82] & BIT_3)) { OSound_traffic_disable(pcm); return; }
            pcm[0x82] |= BIT_4; 
            if (pcm[0x07] < 0x81) pcm[0x07] -= 4; else pcm[0x07] -= 6;
        }
    }
    if (pcm[0x03]) pcm[0x03]--;
    if (pcm[0x02]) pcm[0x02]--;
    if (!pcm[0x02] && !pcm[0x03]) OSound_traffic_disable(pcm);
}

const uint8_t TRAFFIC_PITCH_H[] = { 0, 2, 4, 4, 0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8 };

void OSound_traffic_process_entry(uint8_t* pcm)
{
    if (!(pcm[0x82] & BIT_2)) { pcm[0x82] |= BIT_2; pcm[0x04] = pcm[0x84] = 0x82; pcm[0x05] = pcm[0x85] = 0x00; pcm[0x06] = 0x6; }
    OSound_traffic_set_vol(pcm); OSound_traffic_set_pan(pcm);
    int8_t vol_boost = pcm[0x80] - 0x16;  
    uint8_t pitch = (vol_boost >= 0) ? TRAFFIC_PITCH_H[vol_boost] : 0;
    pitch += (engine_channel & 1) ? 0x60 : 0x80;
    pcm[0x07] = pitch; pcm[0x86] = 0x10; 
}

void OSound_traffic_disable(uint8_t* pcm)
{
    pcm[0x86] |= BIT_0; pcm[0x82] = 0; pcm[0x02] = pcm[0x03] = pcm[0x07] = pcm[0x00] = pcm[0x80] = pcm[0x01] = pcm[0x81] = 0;
}

void OSound_traffic_set_vol(uint8_t* pcm)
{
    uint8_t vol_entry = pcm[0x80];
    if (!vol_entry) return;
    uint16_t multiply = z80_adr_TRAFFIC_VOL_MULTIPLY + vol_entry - 1;
    pcm[0x83] = RomLoader_read8_addr16(&Roms_z80, multiply);
    if (pcm[0x83] < 0x10) pcm[0x82] &= ~BIT_3; else pcm[0x82] |= BIT_3;
}

const uint8_t TRAFFIC_PANNING[] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x08, 0x0D, 0x10, 0x0D, 0x08, 0x00, 0x10, 0x10, 0x10, 0x10 };

void OSound_traffic_set_pan(uint8_t* pcm)
{
    pcm[0x03] = OSound_traffic_get_vol(pcm[0x81] + 0, pcm); pcm[0x02] = OSound_traffic_get_vol(pcm[0x81] + 8, pcm);
}

uint8_t OSound_traffic_get_vol(uint16_t pos, uint8_t* pcm)
{
    return (TRAFFIC_PANNING[pos] * pcm[0x83]) >> 4;
}

void OSound_traffic_note_changes(uint8_t new_vol, uint8_t* pcm)
{
    if (new_vol == pcm[0x80]) pcm[0x82] |= BIT_0;
    else { pcm[0x82] &= ~BIT_0; pcm[0x80] = new_vol; }
    if (pcm[0x01] == pcm[0x81]) pcm[0x82] |= BIT_1;
    else { pcm[0x82] &= ~BIT_1; pcm[0x81] = pcm[0x01]; }
}

void OSound_traffic_read_data(uint8_t* pcm)
{
    uint8_t vol = OSound_engine_data[sound_ENGINE_VOL + engine_channel];
    pcm[0x01] = vol & 7; pcm[0x00] = vol >> 3;
}
