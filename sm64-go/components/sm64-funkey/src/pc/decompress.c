#include <stdint.h>
#include <string.h>

/* Standalone MIO0 Decompressor for ESP32 */

#define MIO0_HEADER_LENGTH 16

typedef struct {
    uint32_t dest_size;
    uint32_t comp_offset;
    uint32_t uncomp_offset;
} mio0_header_t;

static uint32_t read_u32_be(const uint8_t *buf) {
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static int mio0_decode_header(const uint8_t *buf, mio0_header_t *head) {
    if (memcmp(buf, "MIO0", 4) == 0) {
        head->dest_size = read_u32_be(&buf[4]);
        head->comp_offset = read_u32_be(&buf[8]);
        head->uncomp_offset = read_u32_be(&buf[12]);
        return 1;
    }
    return 0;
}

void decompress(const uint8_t *in, uint8_t *out) {
    mio0_header_t head;
    uint32_t bytes_written = 0;
    int bit_idx = 0;
    int comp_idx = 0;
    int uncomp_idx = 0;

    if (!mio0_decode_header(in, &head)) {
        return;
    }

    while (bytes_written < head.dest_size) {
        uint8_t bit_byte = in[MIO0_HEADER_LENGTH + (bit_idx / 8)];
        int bit = (bit_byte & (1 << (7 - (bit_idx % 8)))) != 0;

        if (bit) {
            // 1 - pull uncompressed data
            out[bytes_written] = in[head.uncomp_offset + uncomp_idx];
            bytes_written++;
            uncomp_idx++;
        } else {
            // 0 - read compressed data
            const uint8_t *vals = &in[head.comp_offset + comp_idx];
            comp_idx += 2;
            int length = ((vals[0] & 0xF0) >> 4) + 3;
            int idx = ((vals[0] & 0x0F) << 8) + vals[1] + 1;
            for (int i = 0; i < length; i++) {
                out[bytes_written] = out[bytes_written - idx];
                bytes_written++;
            }
        }
        bit_idx++;
    }
}
