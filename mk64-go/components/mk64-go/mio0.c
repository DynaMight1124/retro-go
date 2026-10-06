#include "mio0.h"

#include <string.h>

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

bool mk64_mio0_decode(const uint8_t *source, size_t source_size,
                     uint8_t *output, size_t output_size)
{
    if (!source || !output || source_size < 16 ||
        memcmp(source, "MIO0", 4) != 0 || read_be32(source + 4) != output_size)
        return false;

    size_t compressed = read_be32(source + 8);
    size_t literal = read_be32(source + 12);
    if (compressed < 16 || literal < compressed || literal > source_size)
        return false;
    const size_t literal_start = literal;

    size_t written = 0;
    size_t command = 16;
    unsigned bit = 0;
    while (written < output_size) {
        if (command >= compressed)
            return false;
        bool is_literal = (source[command] & (0x80u >> bit)) != 0;
        if (++bit == 8) {
            bit = 0;
            command++;
        }
        if (is_literal) {
            if (literal >= source_size)
                return false;
            output[written++] = source[literal++];
        } else {
            if (compressed + 2 > literal_start)
                return false;
            uint8_t hi = source[compressed++];
            uint8_t lo = source[compressed++];
            size_t count = (hi >> 4) + 3;
            size_t distance = (((size_t)hi & 0x0f) << 8) + lo + 1;
            if (distance > written || count > output_size - written)
                return false;
            for (size_t i = 0; i < count; ++i) {
                output[written] = output[written - distance];
                written++;
            }
        }
    }
    return true;
}
