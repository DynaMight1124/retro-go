/**
 * C versions of the hand-written N64 assembly decoders (asm/mio0_decode.s,
 * asm/tkmk00_decode.s).  The data formats are big-endian and are read byte
 * by byte, so this is endian neutral.
 */
#include <ultra64.h>
#include <string.h>
#include <stdio.h>
#include "port.h"
#ifdef RETRO_GO
#include <rg_system.h>
#include "port/rg/menu_timing.h"
extern unsigned int port_time_us(void);
#endif

static inline u32 read_be32(const u8* p) {
    return ((u32) p[0] << 24) | ((u32) p[1] << 16) | ((u32) p[2] << 8) | p[3];
}

/* ------------------------------------------------------------------------- */
/* MIO0                                                                        */
/* ------------------------------------------------------------------------- */

void mio0decode(u8* in, u8* out) {
#ifdef RETRO_GO
    u32 started = port_time_us();
#endif
    u32 dest_size = read_be32(in + 4);
    u32 comp_offset = read_be32(in + 8);
    u32 uncomp_offset = read_be32(in + 12);
    const u8* bits = in + 16;
    u32 bit_idx = 0;
    u32 written = 0;

    if (in[0] != 'M' || in[1] != 'I' || in[2] != 'O' || in[3] != '0') {
#ifdef RETRO_GO
        RG_LOGE("mio0decode: bad header at %p\n", in);
#else
        PORT_LOG("mio0decode: bad header at %p\n", in);
#endif
        return;
    }
    while (written < dest_size) {
        if (bits[bit_idx >> 3] & (0x80 >> (bit_idx & 7))) {
            out[written++] = in[uncomp_offset++];
        } else {
            const u8* v = in + comp_offset;
            u32 length = (v[0] >> 4) + 3;
            u32 idx = ((v[0] & 0xF) << 8) + v[1] + 1;
            u32 i;
            comp_offset += 2;
            for (i = 0; i < length; i++) {
                out[written] = out[written - idx];
                written++;
            }
        }
        bit_idx++;
    }
#ifdef RETRO_GO
    mk64_menu_timing_add(MK64_MENU_MIO0, port_time_us() - started, dest_size);
#endif
}

/* asm/unused_mio0_decode.s: an older copy of the same decoder. */
void func_80040030(u8* in, u8* out) {
    mio0decode(in, out);
}

/* asm/mio0_decode.s: ghost-data helpers used by replays.c.  func_80040174
 * prepares the input for mio0encode; mio0encode returns the compressed size.
 * Ghost saving needs a Controller Pak, which the port does not emulate yet,
 * so these are inert for now. */
s32 func_80040174(void* src, s32 size, s32 dst) {
    (void) src;
    (void) size;
    (void) dst;
    return 0;
}

s32 mio0encode(s32 input, s32 size, s32 output) {
    (void) input;
    (void) size;
    (void) output;
    return 0;
}

/* ------------------------------------------------------------------------- */
/* TKMK00 (menu backgrounds): the decomp ships a C decoder for its host tools  */
/* (tools/libtkmk00.c); it is plain C, so it is compiled in as-is.           */
/* ------------------------------------------------------------------------- */

#include "libtkmk00.h"

static const char *sDecoderError;

const char *port_decoder_error(void) {
    return sDecoderError;
}

void tkmk00decode(u8* tkmk, u8* tmp_buf, u8* rgba16, s32 alpha_color) {
    if (sDecoderError) return;
#ifdef RETRO_GO
    u32 started = port_time_us();
#endif
    unsigned width = ((unsigned)tkmk[8] << 8) | tkmk[9];
    unsigned height = ((unsigned)tkmk[10] << 8) | tkmk[11];
#if MK64_RG_PROFILE
    PORT_LOG("TKMK00 input %p -> %p scratch %p: magic %02x%02x%02x%02x%02x%02x "
             "%ux%u stream0 %08x flags %02x\n", tkmk, rgba16, tmp_buf,
             tkmk[0], tkmk[1], tkmk[2], tkmk[3], tkmk[4], tkmk[5],
             width, height, (unsigned)read_be32(tkmk + 0x0c), tkmk[6]);
#endif
    if (width > 320 || height > 240 ||
        tkmk00_decode(tkmk, tmp_buf, rgba16, alpha_color) != 0) {
        sDecoderError = "Invalid TKMK00 menu image. See serial log.";
#ifdef RETRO_GO
        RG_LOGE("TKMK00 decode rejected: header or color tree invalid\n");
#else
        PORT_LOG("TKMK00 decode rejected: header or color tree invalid\n");
#endif
    }
#ifdef RETRO_GO
    mk64_menu_timing_add(MK64_MENU_TKMK, port_time_us() - started, width * height * 2);
#endif
#ifdef PORT_INPUT_SCRIPT
    {
        // Debug: keep input and output so the decode can be checked on the host.
        static int n;
        char name[64];
        FILE* fp;
        u32 w = ((u32) tkmk[8] << 8) | tkmk[9];
        u32 h = ((u32) tkmk[10] << 8) | tkmk[11];
        if (n < 16) {
            snprintf(name, sizeof(name), "%s" "tkmk%02d_%ux%u_a%d.in", port_save_dir(), n, (unsigned) w, (unsigned) h, (int) alpha_color);
            fp = fopen(name, "wb");
            if (fp) { fwrite(tkmk, 1, 0xCE00, fp); fclose(fp); }
            snprintf(name, sizeof(name), "%s" "tkmk%02d_%ux%u_a%d.out", port_save_dir(), n, (unsigned) w, (unsigned) h, (int) alpha_color);
            fp = fopen(name, "wb");
            if (fp) { fwrite(rgba16, 1, w * h * 2, fp); fclose(fp); }
            PORT_LOG("tkmk00 %d: %ux%u alpha %d in %p out %p tmp %p\n", n, (unsigned) w, (unsigned) h, (int) alpha_color, tkmk, rgba16, tmp_buf);
            n++;
        }
    }
#endif
}
