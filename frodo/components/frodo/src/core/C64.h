#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

constexpr unsigned C64_RAM_SIZE = 0x10000;
constexpr unsigned COLOR_RAM_SIZE = 0x400;
constexpr unsigned BASIC_ROM_SIZE = 0x2000;
constexpr unsigned KERNAL_ROM_SIZE = 0x2000;
constexpr unsigned CHAR_ROM_SIZE = 0x1000;
constexpr unsigned DRIVE_RAM_SIZE = 0x800;
constexpr unsigned DRIVE_ROM_SIZE = 0x4000;

constexpr unsigned SCREEN_FREQ = 50;
constexpr unsigned CYCLES_PER_LINE = 63;

extern bool IsFrodoSC;

class Display;
class MOS6510;
class MOS6569;
class MOS6581;
class MOS6526_1;
class MOS6526_2;
class IEC;
class Cartridge;
class MOS6502_1541;
class GCRDisk;
class Tape;

class C64 {
public:
    C64(const char *content_path, bool full_1541, uint32_t audio_sample_rate);
    ~C64();

    bool RunFrame(uint8_t joystick1, uint8_t joystick2, uint16_t keyboard,
                  uint8_t character, bool render_video);
    void Reset(bool clear_memory = false);
    void NMI();

    uint32_t CycleCounter() const { return cycle_counter; }

    void RequestQuit(int exit_code = 0);
    void ShowNotification(std::string message);
    void AutoStartOp();
    void SetDriveLEDs(int l0, int l1, int l2, int l3);

    const uint8_t *VideoPixels() const;
    const uint32_t *VideoPalette() const;
    const int16_t *AudioSamples(size_t &stereo_frame_count) const;
    const std::string &StartupError() const { return startup_error; }
    bool SaveSnapshot(const char *filename);
    bool LoadSnapshot(const char *filename);
    void DisableAutoStart();

    uint8_t *RAM;
    uint8_t *Basic;
    uint8_t *Kernal;
    uint8_t *Char;
    uint8_t *Color;
    uint8_t *RAM1541;
    uint8_t *ROM1541;

    Display *TheDisplay;
    MOS6510 *TheCPU;
    MOS6569 *TheVIC;
    MOS6581 *TheSID;
    MOS6526_1 *TheCIA1;
    MOS6526_2 *TheCIA2;
    IEC *TheIEC;
    Cartridge *TheCart;
    MOS6502_1541 *TheCPU1541;
    GCRDisk *TheGCRDisk;
    Tape *TheTape;

    static const uint8_t BuiltinBasicROM[BASIC_ROM_SIZE];
    static const uint8_t BuiltinKernalROM[KERNAL_ROM_SIZE];
    static const uint8_t BuiltinCharROM[CHAR_ROM_SIZE];
    static const uint8_t BuiltinDriveROM[DRIVE_ROM_SIZE];

private:
    void initialize_memory();
    void patch_roms(bool full_1541);
    bool dma_load_program(const std::string &path);

    uint32_t cycle_counter;
    bool quit_requested;
    bool cartridge_media;
    std::string content_path;
    uint32_t source_media_size;
    uint32_t source_media_hash;
    bool source_media_valid;
    std::string program_path;
    std::string startup_error;
};
