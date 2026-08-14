#include "sysdeps.h"

#include "C64.h"
#include "1541gcr.h"
#include "CIA.h"
#include "CPU1541.h"
#include "CPUC64.h"
#include "Cartridge.h"
#include "Display.h"
#include "IEC.h"
#include "Prefs.h"
#include "SID.h"
#include "Tape.h"
#include "VIC.h"
#include "CPU1541.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cctype>

#ifdef FRODO_SC
bool IsFrodoSC = true;
#else
bool IsFrodoSC = false;
#endif

#include "Basic_ROM.h"
#include "Kernal_ROM.h"
#include "Char_ROM.h"
#include "1541_ROM.h"

namespace {

constexpr uint8_t SNAPSHOT_MAGIC[16] = {
    'R','G','F','r','o','d','o','S','t','a','t','e',2,0,0,0
};

struct EmbeddedSnapshot {
    uint8_t magic[16];
    uint32_t size;
    uint32_t flags;
    uint32_t cycle_counter;
    uint32_t media_size;
    uint32_t media_hash;
    uint32_t disk_size;
    uint32_t disk_hash;
    uint32_t cartridge_type;
    uint32_t cartridge_state0;
    uint32_t cartridge_state1;
    uint8_t ram[C64_RAM_SIZE];
    uint8_t color[COLOR_RAM_SIZE];
    uint8_t drive_ram[DRIVE_RAM_SIZE];
    MOS6510State cpu;
    MOS6569State vic;
    MOS6581State sid;
    MOS6526State cia1;
    MOS6526State cia2;
    MOS6502State drive_cpu;
    GCRDiskState drive_gcr;
    bool tape_motor;
};

bool hash_file(const std::string &path, uint32_t &size, uint32_t &hash)
{
    FILE *file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) return false;
    uint8_t buffer[1024];
    size = 0;
    hash = 2166136261u;
    size_t count = 0;
    while ((count = std::fread(buffer, 1, sizeof(buffer), file)) != 0) {
        size += count;
        for (size_t i = 0; i < count; ++i) hash = (hash ^ buffer[i]) * 16777619u;
    }
    const bool ok = std::feof(file) != 0;
    std::fclose(file);
    return ok;
}

bool copy_file_bytes(FILE *from, FILE *to, uint32_t size, uint32_t expected_hash)
{
    uint8_t buffer[1024];
    uint32_t remaining = size;
    uint32_t hash = 2166136261u;
    while (remaining > 0) {
        const size_t chunk = std::min<size_t>(sizeof(buffer), remaining);
        if (std::fread(buffer, 1, chunk, from) != chunk ||
            std::fwrite(buffer, 1, chunk, to) != chunk) return false;
        for (size_t i = 0; i < chunk; ++i) hash = (hash ^ buffer[i]) * 16777619u;
        remaining -= chunk;
    }
    return hash == expected_hash;
}

void apply_rom_patch(uint8_t *rom, unsigned offset, const uint8_t *patch, size_t size)
{
    std::memcpy(rom + offset, patch, size);
}

uint8_t joystick_mask(uint8_t input)
{
    uint8_t mask = 0xff;
    if (input & 0x01) mask &= ~0x01;
    if (input & 0x02) mask &= ~0x02;
    if (input & 0x04) mask &= ~0x04;
    if (input & 0x08) mask &= ~0x08;
    if (input & 0x10) mask &= ~0x10;
    return mask;
}

void press_matrix_key(MOS6526_1 *cia, unsigned row, unsigned bit)
{
    cia->KeyMatrix[row] &= static_cast<uint8_t>(~(1u << bit));
    cia->RevMatrix[bit] &= static_cast<uint8_t>(~(1u << row));
}

bool character_matrix_key(uint8_t character, unsigned &row, unsigned &bit,
                          bool &shifted)
{
    shifted = character >= 'A' && character <= 'Z';
    if (shifted) character = static_cast<uint8_t>(character - 'A' + 'a');

    static constexpr uint8_t letter_keys[26] = {
        0x0a, 0x1c, 0x14, 0x12, 0x0e, 0x15, 0x1a, 0x1d, 0x21,
        0x22, 0x25, 0x2a, 0x24, 0x27, 0x26, 0x29, 0x3e, 0x11,
        0x0d, 0x16, 0x1e, 0x1f, 0x09, 0x17, 0x19, 0x0c,
    };
    static constexpr uint8_t digit_keys[10] = {
        0x23, 0x38, 0x3b, 0x08, 0x0b, 0x10, 0x13, 0x18, 0x1b, 0x20,
    };

    uint8_t key = 0;
    if (character >= 'a' && character <= 'z') {
        key = letter_keys[character - 'a'];
    } else if (character >= '0' && character <= '9') {
        key = digit_keys[character - '0'];
    } else {
        switch (character) {
            case ' ': key = 0x3c; break;
            case ',': key = 0x2f; break;
            case '.': key = 0x2c; break;
            case '?': key = 0x37; shifted = true; break;
            default: return false;
        }
    }
    row = key >> 3;
    bit = key & 7;
    return true;
}

bool has_extension(const char *path, const char *extension)
{
    if (path == nullptr) return false;
    const char *dot = std::strrchr(path, '.');
    if (dot == nullptr) return false;
    ++dot;
    while (*dot != '\0' && *extension != '\0') {
        if (std::tolower(static_cast<unsigned char>(*dot++)) != *extension++) return false;
    }
    return *dot == '\0' && *extension == '\0';
}

} // namespace

C64::C64(const char *content_path, bool full_1541, uint32_t audio_sample_rate)
    : RAM(new uint8_t[C64_RAM_SIZE]),
      Basic(new uint8_t[BASIC_ROM_SIZE]),
      Kernal(new uint8_t[KERNAL_ROM_SIZE]),
      Char(new uint8_t[CHAR_ROM_SIZE]),
      Color(new uint8_t[COLOR_RAM_SIZE]),
      RAM1541(new uint8_t[DRIVE_RAM_SIZE]),
      ROM1541(new uint8_t[DRIVE_ROM_SIZE]),
      TheDisplay(nullptr), TheCPU(nullptr), TheVIC(nullptr), TheSID(nullptr),
      TheCIA1(nullptr), TheCIA2(nullptr), TheIEC(nullptr), TheCart(nullptr),
      TheCPU1541(nullptr), TheGCRDisk(nullptr), TheTape(nullptr),
      cycle_counter(0), quit_requested(false),
      cartridge_media(has_extension(content_path, "crt")),
      content_path(content_path == nullptr ? "" : content_path),
      source_media_size(0), source_media_hash(0), source_media_valid(false)
{
    source_media_valid = hash_file(this->content_path, source_media_size,
                                   source_media_hash);
    std::memcpy(Basic, BuiltinBasicROM, BASIC_ROM_SIZE);
    std::memcpy(Kernal, BuiltinKernalROM, KERNAL_ROM_SIZE);
    std::memcpy(Char, BuiltinCharROM, CHAR_ROM_SIZE);
    std::memcpy(ROM1541, BuiltinDriveROM, DRIVE_ROM_SIZE);

    ThePrefs.DrivePath[0].clear();
    int media_type = 0;
    const bool disk_media = content_path != nullptr &&
                            IsMountableFile(content_path, media_type);
    ThePrefs.Emul1541Proc = full_1541 && disk_media;
    if (disk_media) {
        ThePrefs.DrivePath[0] = content_path;
    } else if (has_extension(content_path, "prg") ||
               has_extension(content_path, "t64")) {
        program_path = content_path;
    }
    ThePrefs.SIDType = SIDTYPE_NONE;

    initialize_memory();
    patch_roms(full_1541);

    TheDisplay = new Display(this);
    TheCPU = new MOS6510(this, RAM, Basic, Kernal, Char, Color);
    TheGCRDisk = new GCRDisk(RAM1541);
    TheCPU1541 = new MOS6502_1541(this, TheGCRDisk, RAM1541, ROM1541);
    TheGCRDisk->SetCPU(TheCPU1541);
    TheVIC = new MOS6569(this, TheDisplay, TheCPU, RAM, Char, Color);
    TheSID = new MOS6581(audio_sample_rate);
    TheCIA1 = new MOS6526_1(TheCPU, TheVIC);
    TheCIA2 = new MOS6526_2(TheCPU, TheVIC, TheCPU1541);
    TheCPU1541->TheCIA2 = TheCIA2;
    TheIEC = new IEC(this);
    TheTape = new Tape(TheCIA1);
    if (has_extension(content_path, "crt")) {
        TheCart = Cartridge::FromFile(content_path, startup_error);
    }
    if (TheCart == nullptr) TheCart = new NoCartridge;

    TheCPU->SetChips(TheVIC, TheSID, TheCIA1, TheCIA2, TheCart, TheIEC, TheTape);

    TheCPU->Reset();
    TheSID->Reset();
    TheCIA1->Reset();
    TheCIA2->Reset();
    TheCPU1541->Reset();
    TheGCRDisk->Reset();
    TheTape->Reset();
}

C64::~C64()
{
    delete TheCart;
    delete TheTape;
    delete TheIEC;
    delete TheCIA2;
    delete TheCIA1;
    delete TheSID;
    delete TheVIC;
    delete TheCPU1541;
    delete TheGCRDisk;
    delete TheCPU;
    delete TheDisplay;

    delete[] ROM1541;
    delete[] RAM1541;
    delete[] Color;
    delete[] Char;
    delete[] Kernal;
    delete[] Basic;
    delete[] RAM;
}

void C64::initialize_memory()
{
    uint8_t *p = RAM;
    for (unsigned i = 0; i < 512; ++i) {
        for (unsigned j = 0; j < 64; ++j) {
            *p++ = (j == 32 || j == 57 || j == 58) ? 0xff : 0x00;
        }
        for (unsigned j = 0; j < 64; ++j) {
            *p++ = (j == 36) ? 0xfb : 0xff;
        }
    }
    for (unsigned i = 0; i < COLOR_RAM_SIZE; ++i) {
        Color[i] = static_cast<uint8_t>(i & 0x0f);
    }
    std::memset(RAM1541, 0, DRIVE_RAM_SIZE);
}

void C64::patch_roms(bool full_1541)
{
    if (!full_1541) {
        static constexpr uint8_t patches[][2] = {
            {0xf2, 0x00}, {0xf2, 0x01}, {0xf2, 0x02}, {0xf2, 0x03},
            {0xf2, 0x04}, {0xf2, 0x05}, {0xf2, 0x06}, {0xf2, 0x07},
        };
        static constexpr unsigned offsets[] = {
            0x0d40, 0x0d23, 0x0d36, 0x0e13, 0x0def, 0x0dbe, 0x0dcc, 0x0e03,
        };
        for (unsigned i = 0; i < 8; ++i) {
            apply_rom_patch(Kernal, offsets[i], patches[i], sizeof(patches[i]));
        }
    }

    // Enter the normal BASIC input loop before invoking our one-shot loader.
    // CPUC64 already implements Frodo's $f2/10 host opcode.
    if (!ThePrefs.DrivePath[0].empty() || !program_path.empty()) {
        static constexpr uint8_t auto_start_patch[] = {0xf2, 0x10};
        apply_rom_patch(Basic, 0x0560, auto_start_patch, sizeof(auto_start_patch));
    }

    static constexpr uint8_t checksum_patch[] = {0xea, 0xea};
    static constexpr uint8_t idle_patch[] = {0xf2, 0x00};
    static constexpr uint8_t write_patch[] = {0x20, 0xf2, 0xf5, 0xf2, 0x01};
    static constexpr uint8_t format_patch[] = {0xf2, 0x02};
    apply_rom_patch(ROM1541, 0x2ae4, checksum_patch, sizeof(checksum_patch));
    apply_rom_patch(ROM1541, 0x2ae8, checksum_patch, sizeof(checksum_patch));
    apply_rom_patch(ROM1541, 0x2c9b, idle_patch, sizeof(idle_patch));
    apply_rom_patch(ROM1541, 0x3594, write_patch, sizeof(write_patch));
    apply_rom_patch(ROM1541, 0x3b0c, format_patch, sizeof(format_patch));
}

bool C64::RunFrame(uint8_t joystick1, uint8_t joystick2, uint16_t keyboard,
                   uint8_t character, bool render_video)
{
    TheCIA1->Joystick1 = joystick_mask(joystick1);
    TheCIA1->Joystick2 = joystick_mask(joystick2);

    // The handheld supplies a small virtual-keyboard subset. Matrix bits are
    // active low; RevMatrix mirrors the same switch for reverse CIA scans.
    for (unsigned i = 0; i < 8; ++i) {
        TheCIA1->KeyMatrix[i] = 0xff;
        TheCIA1->RevMatrix[i] = 0xff;
    }
    if (keyboard & 0x01) { // F1: row 0, column/bit 4
        TheCIA1->KeyMatrix[0] &= ~0x10;
        TheCIA1->RevMatrix[4] &= ~0x01;
    }
    if (keyboard & 0x02) { // Space: row 7, column/bit 4
        TheCIA1->KeyMatrix[7] &= ~0x10;
        TheCIA1->RevMatrix[4] &= ~0x80;
    }
    if (keyboard & 0x04) { // Return: row 0, column/bit 1
        TheCIA1->KeyMatrix[0] &= ~0x02;
        TheCIA1->RevMatrix[1] &= ~0x01;
    }
    if (keyboard & 0x08) { // Y: row 3, column/bit 1
        TheCIA1->KeyMatrix[3] &= ~0x02;
        TheCIA1->RevMatrix[1] &= ~0x08;
    }
    if (keyboard & 0x10) { // N: row 4, column/bit 7
        TheCIA1->KeyMatrix[4] &= ~0x80;
        TheCIA1->RevMatrix[7] &= ~0x10;
    }
    if (keyboard & 0x20) { // RUN/STOP: row 7, column/bit 7
        TheCIA1->KeyMatrix[7] &= ~0x80;
        TheCIA1->RevMatrix[7] &= ~0x80;
    }
    unsigned character_row = 0;
    unsigned character_bit = 0;
    bool character_shifted = false;
    if (character && character_matrix_key(character, character_row,
                                          character_bit, character_shifted)) {
        press_matrix_key(TheCIA1, character_row, character_bit);
        if (character_shifted) press_matrix_key(TheCIA1, 1, 7);
    }

    TheVIC->SetRenderEnabled(render_video);

    TheSID->BeginFrame();
    bool new_frame = false;
    while (!new_frame && !quit_requested) {
#ifdef FRODO_SC
        const unsigned flags = TheVIC->EmulateCycle();
        if (flags & VIC_HBLANK) {
            TheSID->EmulateLine();
        }
        TheCIA1->EmulateCycle();
        TheCIA2->EmulateCycle();
        TheCPU->EmulateCycle();
        TheTape->EmulateCycle();

        if (ThePrefs.Emul1541Proc) {
            TheCPU1541->EmulateVIACycle();
            if (!TheCPU1541->Idle) {
                TheCPU1541->EmulateCPUCycle();
            }
        }

        ++cycle_counter;
        new_frame = (flags & VIC_VBLANK) != 0;
#else
        int cycles = 0;
        const unsigned flags = TheVIC->EmulateLine(cycles);
        new_frame = (flags & VIC_VBLANK) != 0;

        TheSID->EmulateLine();

        if (ThePrefs.Emul1541Proc) {
            int drive_cycles = ThePrefs.FloppyCycles;
            TheCPU1541->CountVIATimers(drive_cycles);
            if (!TheCPU1541->Idle) {
                while (cycles >= 0 || drive_cycles >= 0) {
                    if (cycles > drive_cycles) {
                        cycles -= TheCPU->EmulateLine(1);
                    } else {
                        const int used = TheCPU1541->EmulateLine(1);
                        drive_cycles -= used;
                        cycle_counter += used;
                    }
                }
            } else {
                TheCPU->EmulateLine(cycles);
                cycle_counter += CYCLES_PER_LINE;
            }
        } else {
            TheCPU->EmulateLine(cycles);
            cycle_counter += CYCLES_PER_LINE;
        }
#endif
    }

    TheCIA1->CountTOD();
    TheCIA2->CountTOD();
    return !quit_requested;
}

void C64::Reset(bool clear_memory)
{
    if (clear_memory) initialize_memory();
    TheCPU->AsyncReset();
    TheCPU1541->AsyncReset();
    TheGCRDisk->Reset();
    TheTape->Reset();
    TheSID->Reset();
    TheCIA1->Reset();
    TheCIA2->Reset();
    TheIEC->Reset();
    TheCart->Reset();
}

void C64::DisableAutoStart()
{
    std::memcpy(Basic + 0x0560, BuiltinBasicROM + 0x0560, 2);
    program_path.clear();
}

void C64::NMI() { TheCPU->AsyncNMI(); }
void C64::RequestQuit(int) { quit_requested = true; }
void C64::ShowNotification(std::string message) { std::printf("Frodo: %s\n", message.c_str()); }
void C64::AutoStartOp()
{
    // Restore the instruction replaced by the one-shot host opcode so the
    // BASIC input loop behaves normally after this call.
    std::memcpy(Basic + 0x0560, BuiltinBasicROM + 0x0560, 2);

    if (!program_path.empty()) {
        if (dma_load_program(program_path)) {
            static constexpr uint8_t keys[] = {'R', 'U', 'N', '\r'};
            std::memcpy(RAM + 0x0277, keys, sizeof(keys));
            RAM[0x00c6] = sizeof(keys);
        }
        return;
    }

    if (ThePrefs.DrivePath[0].empty()) {
        return;
    }

    // Put LOAD"*",8,1 on the current BASIC input line. Keeping the command
    // on screen leaves enough of the ten-byte KERNAL keyboard buffer for
    // RETURN, RUN, RETURN, matching upstream Frodo's autostart mechanism.
    uint16_t screen = RAM[0xd1] | (uint16_t(RAM[0xd2]) << 8);
    static constexpr char command[] = "load\"*\",8,1";
    for (char c : command) {
        if (c == '\0') break;
        uint8_t screen_code = static_cast<uint8_t>(c);
        if (screen_code >= 'a' && screen_code <= 'z') screen_code ^= 0x60;
        RAM[screen++] = screen_code;
    }

    static constexpr uint8_t keys[] = {'\r', 'R', 'U', 'N', '\r'};
    std::memcpy(RAM + 0x0277, keys, sizeof(keys));
    RAM[0x00c6] = sizeof(keys);
}

bool C64::dma_load_program(const std::string &path)
{
    FILE *file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        ShowNotification("Unable to open PRG file");
        return false;
    }

    uint16_t load_address = 0;
    size_t requested_size = 0;

    if (has_extension(path.c_str(), "t64")) {
        uint8_t header[64] = {};
        if (std::fread(header, 1, sizeof(header), file) != sizeof(header) ||
            std::memcmp(header, "C64", 3) != 0) {
            std::fclose(file);
            ShowNotification("Invalid T64 file");
            return false;
        }

        const unsigned max_entries = unsigned(header[0x22]) |
                                     (unsigned(header[0x23]) << 8);
        bool found = false;
        for (unsigned i = 0; i < max_entries; ++i) {
            uint8_t entry[32] = {};
            if (std::fread(entry, 1, sizeof(entry), file) != sizeof(entry)) break;
            if (entry[0] == 0) continue;

            load_address = uint16_t(entry[2]) | (uint16_t(entry[3]) << 8);
            const uint16_t end_address = uint16_t(entry[4]) |
                                         (uint16_t(entry[5]) << 8);
            const uint32_t data_offset = uint32_t(entry[8]) |
                                         (uint32_t(entry[9]) << 8) |
                                         (uint32_t(entry[10]) << 16) |
                                         (uint32_t(entry[11]) << 24);
            if (data_offset < 64 || end_address <= load_address ||
                std::fseek(file, static_cast<long>(data_offset), SEEK_SET) != 0) {
                break;
            }
            requested_size = size_t(end_address - load_address);
            found = true;
            break;
        }
        if (!found) {
            std::fclose(file);
            ShowNotification("T64 has no loadable program");
            return false;
        }
    } else {
        uint8_t header[2];
        if (std::fread(header, 1, sizeof(header), file) != sizeof(header)) {
            std::fclose(file);
            ShowNotification("Invalid PRG file");
            return false;
        }
        load_address = uint16_t(header[0]) | (uint16_t(header[1]) << 8);
        requested_size = C64_RAM_SIZE - load_address;
    }

    const size_t capacity = C64_RAM_SIZE - load_address;
    if (requested_size > capacity) requested_size = capacity;
    const size_t byte_count = std::fread(RAM + load_address, 1,
                                         requested_size, file);
    std::fclose(file);
    const uint16_t end_address = static_cast<uint16_t>(load_address + byte_count);

    RAM[0x90] = 0x40;
    RAM[0xba] = 8;
    RAM[0xae] = end_address & 0xff;
    RAM[0xaf] = end_address >> 8;
    if (load_address == 0x0801) {
        RAM[0x2d] = end_address & 0xff;
        RAM[0x2e] = end_address >> 8;
        RAM[0x2f] = RAM[0x31] = RAM[0x2d];
        RAM[0x30] = RAM[0x32] = RAM[0x2e];
        RAM[0x33] = RAM[0x37];
        RAM[0x34] = RAM[0x38];
        RAM[0x7a] = (load_address - 1) & 0xff;
        RAM[0x7b] = (load_address - 1) >> 8;
        RAM[0x41] = RAM[0x7a];
        RAM[0x42] = RAM[0x7b];
    }
    return byte_count != 0;
}
void C64::SetDriveLEDs(int, int, int, int) {}
const uint8_t *C64::VideoPixels() const { return TheDisplay->Pixels(); }
const uint32_t *C64::VideoPalette() const { return TheDisplay->Palette(); }
const int16_t *C64::AudioSamples(size_t &stereo_frame_count) const
{
    return TheSID->AudioSamples(stereo_frame_count);
}

bool C64::SaveSnapshot(const char *filename)
{
    if (filename == nullptr || !TheGCRDisk->FlushImage()) return false;
    auto *state = new (std::nothrow) EmbeddedSnapshot{};
    if (state == nullptr) return false;

    std::memcpy(state->magic, SNAPSHOT_MAGIC, sizeof(state->magic));
    state->size = sizeof(*state);
#ifdef FRODO_SC
    state->flags |= 2;
#endif
    if (ThePrefs.Emul1541Proc) state->flags |= 1;
    if (!source_media_valid) {
        delete state;
        return false;
    }
    state->media_size = source_media_size;
    state->media_hash = source_media_hash;
    const bool writable_disk = ThePrefs.Emul1541Proc &&
        (has_extension(content_path.c_str(), "d64") ||
         has_extension(content_path.c_str(), "x64"));
    if (writable_disk) {
        if (!hash_file(content_path, state->disk_size, state->disk_hash)) {
            delete state;
            return false;
        }
    }
    state->cartridge_type = TheCart->SnapshotType();
    TheCart->GetSnapshotState(state->cartridge_state0,
                              state->cartridge_state1);
    state->cycle_counter = cycle_counter;
    std::memcpy(state->ram, RAM, sizeof(state->ram));
    std::memcpy(state->color, Color, sizeof(state->color));
    std::memcpy(state->drive_ram, RAM1541, sizeof(state->drive_ram));
    TheCPU->GetState(&state->cpu);
    TheVIC->GetState(&state->vic);
    TheSID->GetState(&state->sid);
    TheCIA1->GetState(&state->cia1);
    TheCIA2->GetState(&state->cia2);
    TheCPU1541->GetState(&state->drive_cpu);
    TheGCRDisk->GetState(&state->drive_gcr);
    state->tape_motor = TheTape->MotorOn();

    FILE *file = std::fopen(filename, "wb");
    bool ok = file != nullptr && std::fwrite(state, sizeof(*state), 1, file) == 1;
    if (ok && state->disk_size != 0) {
        FILE *disk = std::fopen(content_path.c_str(), "rb");
        ok = disk != nullptr && copy_file_bytes(disk, file, state->disk_size,
                                                state->disk_hash);
        if (disk != nullptr) std::fclose(disk);
    }
    ok = ok && std::fflush(file) == 0;
    if (file != nullptr) std::fclose(file);
    delete state;
    return ok;
}

bool C64::LoadSnapshot(const char *filename)
{
    if (filename == nullptr) return false;
    auto *state = new (std::nothrow) EmbeddedSnapshot{};
    if (state == nullptr) return false;
    FILE *file = std::fopen(filename, "rb");
    const bool read_ok = file != nullptr &&
        std::fread(state, sizeof(*state), 1, file) == 1;
    long snapshot_size = -1;
    if (read_ok && std::fseek(file, 0, SEEK_END) == 0) {
        snapshot_size = std::ftell(file);
        std::fseek(file, sizeof(*state), SEEK_SET);
    }

#ifdef FRODO_SC
    constexpr uint32_t engine_flag = 2;
#else
    constexpr uint32_t engine_flag = 0;
#endif
    const bool valid = read_ok &&
        std::memcmp(state->magic, SNAPSHOT_MAGIC, sizeof(state->magic)) == 0 &&
        state->size == sizeof(*state) && (state->flags & 2) == engine_flag &&
        bool(state->flags & 1) == ThePrefs.Emul1541Proc &&
        state->cartridge_type == TheCart->SnapshotType() &&
        source_media_valid && state->media_size == source_media_size &&
        state->media_hash == source_media_hash &&
        state->disk_size <= 256 * 1024 &&
        snapshot_size == static_cast<long>(sizeof(*state) + state->disk_size) &&
        (state->disk_size == 0 ||
         ((has_extension(content_path.c_str(), "d64") ||
           has_extension(content_path.c_str(), "x64")) &&
          state->disk_size != 0));
    if (!valid) {
        if (file != nullptr) std::fclose(file);
        delete state;
        return false;
    }

    if (state->disk_size != 0) {
        auto *disk_data = new (std::nothrow) uint8_t[state->disk_size];
        bool disk_ok = disk_data != nullptr &&
            std::fread(disk_data, 1, state->disk_size, file) == state->disk_size;
        uint32_t hash = 2166136261u;
        if (disk_ok) {
            for (uint32_t i = 0; i < state->disk_size; ++i)
                hash = (hash ^ disk_data[i]) * 16777619u;
            disk_ok = hash == state->disk_hash;
        }
        if (file != nullptr) std::fclose(file);
        file = nullptr;
        if (disk_ok) {
            TheGCRDisk->ReloadImage("");
            FILE *disk = std::fopen(content_path.c_str(), "wb");
            disk_ok = disk != nullptr &&
                std::fwrite(disk_data, 1, state->disk_size, disk) == state->disk_size &&
                std::fflush(disk) == 0;
            if (disk != nullptr) std::fclose(disk);
            TheGCRDisk->ReloadImage(content_path);
        }
        delete[] disk_data;
        if (!disk_ok) {
            delete state;
            return false;
        }
    }
    if (file != nullptr) std::fclose(file);

    if (!TheCart->SetSnapshotState(state->cartridge_state0,
                                   state->cartridge_state1)) {
        delete state;
        return false;
    }

    std::memcpy(RAM, state->ram, sizeof(state->ram));
    std::memcpy(Color, state->color, sizeof(state->color));
    std::memcpy(RAM1541, state->drive_ram, sizeof(state->drive_ram));
    cycle_counter = state->cycle_counter;
    TheCPU->SetState(&state->cpu);
    TheVIC->SetState(&state->vic);
    TheSID->SetState(&state->sid);
    TheCIA1->SetState(&state->cia1);
    TheCIA2->SetState(&state->cia2);
    TheCPU1541->SetState(&state->drive_cpu);
    TheGCRDisk->SetState(&state->drive_gcr);
    TheTape->SetState(state->tape_motor);
    delete state;
    return true;
}
