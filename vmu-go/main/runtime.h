#pragma once
#include "vmu.h"
#include <ctype.h>
#include <string.h>

inline int vmu_content_type(const char *name)
{
    const char *ext = strrchr(name, '.');
    if (!ext || strlen(ext) != 4) return -1;
    char lower[5];
    for (int i = 0; i < 5; ++i) lower[i] = (char)tolower((unsigned char)ext[i]);
    if (!strcmp(lower, ".bin")) return 0;
    if (!strcmp(lower, ".vms")) return 1;
    if (!strcmp(lower, ".dci")) return 2;
    return -1;
}

struct VmuZipInfo { size_t size; uint32_t crc; int type; };
inline uint32_t vmu_le32(const byte *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
inline bool vmu_zip_info(const byte *header, size_t size, size_t fileSize, VmuZipInfo &info)
{
    // Match Retro-Go's first-member unzip behavior, but bound allocation first.
    if (size < 30 || vmu_le32(header) != 0x04034B50 || header[6] || header[7] ||
        header[8] != 8 || header[9]) return false;
    size_t nameSize = header[26] | (header[27] << 8);
    size_t extra = header[28] | (header[29] << 8);
    if (!nameSize || nameSize > 225 || size < 30 + nameSize) return false;
    char name[226]; memcpy(name,header+30,nameSize); name[nameSize]=0;
    if (strlen(name) != nameSize) return false;
    info.type = vmu_content_type(name);
    info.size = vmu_le32(header+22);
    info.crc = vmu_le32(header+14);
    size_t compressed = vmu_le32(header+18);
    size_t offset = 30 + nameSize + extra;
    return info.type >= 0 && info.size > 0 && info.size <= 0x20000 + 32 &&
        compressed > 0 && offset <= fileSize && compressed <= fileSize - offset;
}

// Common oscillator timebase: lcm(600000, 32768). A 60 Hz frame is exact.
// Carry the last cycle's overshoot into the next frame, including clock changes.
struct VmuRuntime
{
    int frameDebt = 0;
    void runFrame(VMU &machine)
    {
        frameDebt += 5120000;
        while (frameDebt > 0) {
            byte ocr = machine.ram->readByte_RAW(OCR);
            int ticks = (ocr & 32) ? 56250 : 3072;
            if (!(ocr & 128)) ticks *= 2;
            machine.runCycle();
            frameDebt -= ticks;
        }
    }
    void input(VMU &machine, byte pressed)
    {
        if (!machine.hasBIOS()) pressed &= 0x3F;
        if (!machine.cpu->P3_taken) return;
        machine.ram->writeByte_RAW(P3, (byte)~pressed);
        if (pressed) {
            machine.ram->writeByte_RAW(P3INT,
                machine.ram->readByte_RAW(P3INT) | 2);
            machine.intHandler->setP3();
            machine.cpu->P3_taken = false;
        }
    }
};

// CRC32 for content/BIOS identity and on-disk state corruption detection.
inline uint32_t vmu_crc32(const void *data, size_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    const byte *p = (const byte *)data;
    while (size--) {
        crc ^= *p++;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1)));
    }
    return ~crc;
}
