#include "rom.h"

#include <limits.h>
#include <string.h>

static void normalize_bytes(uint8_t *data, size_t length, uint8_t byte_order)
{
    if (byte_order == 1) {
        for (size_t i = 0; i < length; i += 2) {
            uint8_t temp = data[i];
            data[i] = data[i + 1];
            data[i + 1] = temp;
        }
    } else if (byte_order == 2) {
        for (size_t i = 0; i < length; i += 4) {
            uint8_t temp = data[i];
            data[i] = data[i + 3];
            data[i + 3] = temp;
            temp = data[i + 1];
            data[i + 1] = data[i + 2];
            data[i + 2] = temp;
        }
    }
}

bool mk64_rom_read(mk64_rom_t *rom, uint32_t offset, void *dst, size_t length)
{
    uint8_t scratch[512];
    uint8_t *output = dst;
    uint64_t end = (uint64_t)offset + length;

    if (!rom || !rom->file || (!dst && length) || end > rom->size)
        return false;

    /* Native .z64 bytes need no word alignment or conversion. A single
     * transfer also lets stdio/FatFS batch the SD read instead of issuing
     * hundreds of 512-byte application reads for one menu asset. */
    if (rom->byte_order == 0) {
        if (!length) return true;
        return fseek(rom->file, (long)offset, SEEK_SET) == 0 &&
               fread(dst, 1, length, rom->file) == length;
    }

    if (length && fseek(rom->file, (long)(offset & ~3u), SEEK_SET) != 0)
        return false;

    while (length) {
        uint32_t aligned = offset & ~3u;
        size_t skip = offset - aligned;
        size_t count = length < sizeof(scratch) - skip ? length : sizeof(scratch) - skip;
        size_t read_size = (skip + count + 3) & ~3u;

        /* The last ROM read can be shorter than a whole word. */
        if ((uint64_t)aligned + read_size > rom->size)
            return false;
        if (fread(scratch, 1, read_size, rom->file) != read_size)
            return false;

        normalize_bytes(scratch, read_size, rom->byte_order);
        memcpy(output, scratch + skip, count);
        output += count;
        offset += count;
        length -= count;
    }
    return true;
}

mk64_rom_result_t mk64_rom_open(mk64_rom_t *rom, const char *path)
{
    uint8_t magic[4];
    uint8_t header[0x40];
    long file_size;

    if (!rom || !path || !*path)
        return MK64_ROM_NOT_FOUND;
    memset(rom, 0, sizeof(*rom));
    rom->file = fopen(path, "rb");
    if (!rom->file)
        return MK64_ROM_NOT_FOUND;

    if (fseek(rom->file, 0, SEEK_END) != 0 ||
        (file_size = ftell(rom->file)) < (long)sizeof(header) ||
        file_size > INT32_MAX ||
        fseek(rom->file, 0, SEEK_SET) != 0 ||
        fread(magic, 1, sizeof(magic), rom->file) != sizeof(magic)) {
        mk64_rom_close(rom);
        return MK64_ROM_IO_ERROR;
    }
    rom->size = (uint32_t)file_size;

    if (!memcmp(magic, "\x80\x37\x12\x40", 4))
        rom->byte_order = 0; /* .z64 */
    else if (!memcmp(magic, "\x37\x80\x40\x12", 4))
        rom->byte_order = 1; /* .v64 */
    else if (!memcmp(magic, "\x40\x12\x37\x80", 4))
        rom->byte_order = 2; /* .n64 */
    else {
        mk64_rom_close(rom);
        return MK64_ROM_INVALID;
    }

    if (!mk64_rom_read(rom, 0, header, sizeof(header)) ||
        memcmp(header + 0x20, "MARIOKART64", 11) != 0) {
        mk64_rom_close(rom);
        return MK64_ROM_INVALID;
    }
    if (header[0x3e] != 'E') {
        mk64_rom_close(rom);
        return MK64_ROM_UNSUPPORTED_REGION;
    }
    return MK64_ROM_OK;
}

void mk64_rom_close(mk64_rom_t *rom)
{
    if (!rom)
        return;
    if (rom->file)
        fclose(rom->file);
    memset(rom, 0, sizeof(*rom));
}
