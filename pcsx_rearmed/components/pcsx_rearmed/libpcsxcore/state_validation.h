/* Validate the native Retro-Go save format before touching the live core. */
#ifndef PCSX_STATE_VALIDATION_H
#define PCSX_STATE_VALIDATION_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct pcsx_state_layout {
    size_t boolean_size, regs_size, gpu_size, spu_size, spu_size_offset;
    size_t spu_extra_max, tail_size, pad_size, pad_count, pad_size_offset;
};

static inline int pcsx_state_span(size_t *pos, size_t count, size_t length)
{
    if (*pos > length || count > length - *pos)
        return 0;
    *pos += count;
    return 1;
}

static inline uint32_t pcsx_state_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Returns the cache size so Lightrec can skip RV32's optional cache record. */
static inline int pcsx_state_validate(const unsigned char *data, size_t length,
    const struct pcsx_state_layout *layout, uint32_t version, int hle_only,
    size_t *cache_size)
{
    size_t pos = 0, hdr, spu_hdr;
    uint32_t size;
    *cache_size = 0;
    if (!data || layout->gpu_size < 4 ||
        layout->spu_size_offset + 4 > layout->spu_size ||
        layout->pad_size_offset + 2 > layout->pad_size)
        return 0;
    if (length >= 16 && memcmp(data, "RASTATE", 7) == 0)
        pos = 16;
    hdr = pos;
    if (!pcsx_state_span(&pos, 32 + 4 + layout->boolean_size, length) ||
        memcmp(data + hdr, "STv4 PCSX", 9) != 0 ||
        pcsx_state_u32(data + hdr + 32) != version)
        return 0;
    if (layout->boolean_size != 1 && layout->boolean_size != 4)
        return 0;
    uint32_t hle = layout->boolean_size == 1 ? data[hdr + 36] :
        pcsx_state_u32(data + hdr + 36);
    if (hle > 1 || (hle_only && hle != 1))
        return 0;
    if (!pcsx_state_span(&pos, 128 * 96 * 3 + 0x200000 + 0x80000 +
                         0x10000 + layout->regs_size, length))
        return 0;
    hdr = pos;
    if (!pcsx_state_span(&pos, layout->gpu_size + 1024 * 512 * 2, length) ||
        pcsx_state_u32(data + hdr) != 1 ||
        !pcsx_state_span(&pos, 4, length))
        return 0;
    size = pcsx_state_u32(data + pos - 4);
    spu_hdr = pos;
    if (size <= layout->spu_size + 512 * 1024 ||
        size > layout->spu_size + 512 * 1024 + layout->spu_extra_max ||
        !pcsx_state_span(&pos, size, length) ||
        pcsx_state_u32(data + spu_hdr + layout->spu_size_offset) != size ||
        !pcsx_state_span(&pos, layout->tail_size, length))
        return 0;
    if (length - pos >= 8 && memcmp(data + pos, "ariblks\0", 8) == 0) {
        if (!pcsx_state_span(&pos, 12, length))
            return 0;
        size = pcsx_state_u32(data + pos - 4);
        if (!size || size > 16 * 1024 || (size & 7) ||
            !pcsx_state_span(&pos, size, length))
            return 0;
        *cache_size = 12 + size;
    }
    for (size_t i = 0; i < layout->pad_count; ++i) {
        hdr = pos;
        if (!pcsx_state_span(&pos, layout->pad_size, length))
            return 0;
        size = (uint32_t)data[hdr + layout->pad_size_offset] |
               ((uint32_t)data[hdr + layout->pad_size_offset + 1] << 8);
        if (size != layout->pad_size)
            return 0;
    }
    return pos == length;
}
#endif
