/* Boot-time asset region setup.  The EBOOT carries none of the game's data:
 * it comes from assets.bin (a cache written on the first run) or, when that
 * is missing or stale, from the player's own ROM next to the EBOOT via the
 * recipe table (assets_gen.c).  See tools/psp/make_assets.py. */
#include <PR/ultratypes.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "port.h"

extern u8 __assets_start[], __assets_end[]; /* linker script: the .assetdata region */
extern u8 _ftext[];                          /* link address 0 -> runtime module base */
extern u8 gPortMemoryPool[];                 /* 3 MB game pool: free scratch until the game starts (the heap is nearly full on a PSP-1000) */
extern int port_assets_generate(const char* dir, const char* rom_path, void (*progress)(const char*, int), char* err, u32 errlen);
extern int port_assets_write_cache(const char* path, char* err, u32 errlen);

struct AssetHeader {
    char magic[8];
    u32 version, region_addr, region_size, reloc_count, sym_count, names_size, build_id, data_crc, pad;
};
extern u32 port_crc32(u32 crc, const void* b, u32 n);

static int try_load(const char* path, char* err, u32 errlen) {
    struct AssetHeader h;
    u32 base, hdrlen, i, done;
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        snprintf(err, errlen, "not found");
        return 0;
    }
    if (fread(&h, 1, sizeof(h), f) != sizeof(h) || memcmp(h.magic, "MK64ASST", 8) != 0 || h.version != 1) {
        snprintf(err, errlen, "not an MK64 asset file");
        fclose(f);
        return 0;
    }
    base = (u32) _ftext; // PRX links at 0, so the module base is _ftext's runtime address
    if ((u32) __assets_start - h.region_addr != base || (u32) (__assets_end - __assets_start) != h.region_size) {
        snprintf(err, errlen, "from another version of the EBOOT");
        fclose(f);
        return 0;
    }
    {
        u32* rl = (u32*) gPortMemoryPool;
        if (h.reloc_count * 4 > 1024 * 1024) {
            snprintf(err, errlen, "too many relocs (%u)", (unsigned) h.reloc_count);
            fclose(f);
            return 0;
        }
        if (fread(rl, 4, h.reloc_count, f) != h.reloc_count) {
            snprintf(err, errlen, "truncated (relocs)");
            fclose(f);
            return 0;
        }
        /* Every relocated word must lie inside the region, 4-byte aligned. */
        for (i = 0; i < h.reloc_count; i++) {
            if ((rl[i] & 3) != 0 || rl[i] + 4 > h.region_size) {
                snprintf(err, errlen, "corrupt (reloc %u at %08X)", (unsigned) i, (unsigned) rl[i]);
                fclose(f);
                return 0;
            }
        }
        hdrlen = sizeof(h) + h.reloc_count * 4 + h.sym_count * 16 + h.names_size;
        hdrlen = (hdrlen + 15) & ~15u;
        fseek(f, hdrlen, SEEK_SET);
        {
            u32 crc = 0;
            for (done = 0; done < h.region_size;) {
                u32 want = h.region_size - done;
                size_t got;
                if (want > 512 * 1024) want = 512 * 1024;
                got = fread(__assets_start + done, 1, want, f);
                if (got == 0) break;
                if (h.data_crc != 0) crc = port_crc32(crc, __assets_start + done, (u32) got);
                done += got;
            }
            fclose(f);
            if (done != h.region_size) {
                snprintf(err, errlen, "truncated (%u of %u bytes)", (unsigned) done, (unsigned) h.region_size);
                return 0;
            }
            /* data_crc == 0: a cache from a build that did not record it */
            if (h.data_crc != 0 && crc != h.data_crc) {
                snprintf(err, errlen, "corrupt (crc %08X, expected %08X)", (unsigned) crc, (unsigned) h.data_crc);
                return 0;
            }
        }
        for (i = 0; i < h.reloc_count; i++) {
            *(u32*) (__assets_start + rl[i]) += base;
        }
    }
    PORT_LOG("assets: %s loaded (%u bytes, %u relocs, base %08X)\n", path, (unsigned) h.region_size, (unsigned) h.reloc_count, (unsigned) base);
    return 1;
}

/* ------------------------------------------------------------------ UI
 * First-start / error screens: the XMB background art (PIC1, shipped as
 * RGB565 in the EBOOT's PSAR) darkened, with drop-shadowed white text and a
 * progress bar, drawn straight into the debug screen's 8888 VRAM buffer. */
extern u32 port_psar_read(const char* dir, const char* tag, u8* buf, u32 cap, char* err, u32 errlen);
#define VRAM32 ((volatile u32*) 0x44000000)
#define SCR_W 480
#define SCR_H 272
#define SCR_STRIDE 512
static const u16* sSplashImg; /* RGB565, or NULL */
static char sSplashDir[192];

static u32 dark_pixel(u16 p) { /* 565 -> 8888 (ABGR) at ~40% */
    u32 r = (p & 0x1F) << 3, g = ((p >> 5) & 0x3F) << 2, b = ((p >> 11) & 0x1F) << 3;
    r = r * 2 / 5; g = g * 2 / 5; b = b * 2 / 5;
    return 0xFF000000u | (b << 16) | (g << 8) | r;
}
static void splash_band(int y0, int y1) { /* repaint rows [y0,y1) from the art (or black) */
    int x, y;
    for (y = y0; y < y1; y++) {
        for (x = 0; x < SCR_W; x++) {
            VRAM32[y * SCR_STRIDE + x] = sSplashImg ? dark_pixel(sSplashImg[y * SCR_W + x]) : 0xFF000000u;
        }
    }
}
static void splash_text(int x, int y, const char* t) { /* 8x8 debug font with a 1px shadow */
    for (; *t; t++, x += 8) {
        pspDebugScreenPutChar(x + 1, y + 1, 0xFF000000u, (u8) *t);
        pspDebugScreenPutChar(x, y, 0xFFFFFFFFu, (u8) *t);
    }
}
static void splash_text_center(int y, const char* t) { splash_text((SCR_W - 8 * (int) strlen(t)) / 2, y, t); }
extern unsigned char msx[]; /* libpspdebug's 8x8 font, 8 bytes per glyph */
static void splash_glyph(int x, int y, u8 ch, int scale, u32 color, int italic) {
    int row, col, sy, sx;
    for (row = 0; row < 8; row++) {
        u8 bits = msx[ch * 8 + row];
        int shear = italic ? (7 - row) * scale / 4 : 0; /* slant: top rows shifted right */
        for (col = 0; col < 8; col++) {
            if (!(bits & (0x80 >> col))) continue;
            for (sy = 0; sy < scale; sy++)
                for (sx = 0; sx < scale; sx++) {
                    int px = x + col * scale + sx + shear, py = y + row * scale + sy;
                    if (px >= 0 && px < SCR_W && py >= 0 && py < SCR_H) VRAM32[py * SCR_STRIDE + px] = color;
                }
        }
    }
}
static void splash_big_center(int y, const char* t, int scale, u32 color, int italic) {
    int x = (SCR_W - 8 * scale * (int) strlen(t)) / 2;
    for (; *t; t++, x += 8 * scale) {
        splash_glyph(x + scale, y + scale, (u8) *t, scale, 0xFF000000u, italic); /* shadow */
        splash_glyph(x, y, (u8) *t, scale, color, italic);
    }
}
static void splash_begin(const char* dir) {
    static char err[64];
    u8* img = gPortMemoryPool + (2700 * 1024); /* 261 KB, above every other scratch user */
    pspDebugScreenInit();
    pspDebugScreenEnableBackColor(0);
    pspDebugScreenSetTextColor(0xFFFFFFFFu);
    sSplashImg = NULL;
    if (port_psar_read(dir, "PIC1", img, SCR_W * SCR_H * 2, err, sizeof(err)) == SCR_W * SCR_H * 2) sSplashImg = (const u16*) img;
    splash_band(0, SCR_H);
    splash_big_center(14, "MK64", 5, 0xFFFFFFFFu, 0);          /* heading: 40 px tall */
    splash_big_center(58, "PORTABLE", 2, 0xFFB0B0B0u, 1);      /* subtitle: small, gray, italic */
}
static void splash_bar(int pct) {
    int x0 = 60, x1 = 420, y0 = 196, y1 = 208, x, y, fill = x0 + (x1 - x0) * pct / 100;
    for (y = y0; y < y1; y++)
        for (x = x0; x < x1; x++)
            VRAM32[y * SCR_STRIDE + x] = (x < fill) ? 0xFF00D6FFu /* gold */ : 0xFF404040u;
}
static void screen_progress(const char* what, int pct) {
    splash_band(150, 230);
    splash_text_center(160, what);
    splash_bar(pct);
    {
        char p[8];
        snprintf(p, sizeof(p), "%d%%", pct);
        splash_text_center(216, p);
    }
#ifdef PORT_SPLASH_DUMP
    { static int dumped; if (!dumped && pct >= 40) { FILE* f = fopen(port_save_path("splash.ppm"), "wb"); int x, y; dumped = 1;
        if (f) { fprintf(f, "P6\n480 272\n255\n"); for (y = 0; y < SCR_H; y++) for (x = 0; x < SCR_W; x++) { u32 v = VRAM32[y * SCR_STRIDE + x]; u8 px[3] = { v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF }; fwrite(px, 1, 3, f); } fclose(f); } } }
#endif
}

static void fail_screen(const char* line1, const char* line2, const char* line3) {
    splash_begin(sSplashDir);
    splash_text_center(120, line1);
    if (line2) splash_text_center(136, line2);
    if (line3) splash_text_center(152, line3);
    splash_text_center(232, "Press any button to return to the XMB.");
    PORT_LOG("assets: %s | %s | %s\n", line1, line2 ? line2 : "", line3 ? line3 : "");
    {
        SceCtrlData pad;
        sceCtrlSetSamplingCycle(0);
        sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
        for (;;) { /* wait for a press (after any button held from the launch is released) */
            sceCtrlReadBufferPositive(&pad, 1);
            if (pad.Buttons == 0) break;
        }
        for (;;) {
            sceCtrlReadBufferPositive(&pad, 1);
            if (pad.Buttons & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_SQUARE | PSP_CTRL_TRIANGLE | PSP_CTRL_START | PSP_CTRL_SELECT |
                               PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT)) break;
        }
    }
    sceKernelExitGame();
}

/* Look for a ROM in `dir`: any *.z64 / *.n64 / *.v64 (case-insensitive). */
static int find_rom(const char* dir, char* out, u32 outlen) {
    SceUID d = sceIoDopen(dir);
    SceIoDirent ent;
    int found = 0;
    if (d < 0) return 0;
    memset(&ent, 0, sizeof(ent));
    while (sceIoDread(d, &ent) > 0) {
        const char* n = ent.d_name;
        size_t l = strlen(n);
        if (n[0] == '.') { memset(&ent, 0, sizeof(ent)); continue; } /* macOS "._foo.z64" resource forks etc. */
        if (l > 4 && n[l - 4] == '.') {
            char e1 = n[l - 3] | 0x20, e2 = n[l - 2] | 0x20, e3 = n[l - 1] | 0x20;
            if ((e1 == 'z' || e1 == 'n' || e1 == 'v') && e2 == '6' && e3 == '4') {
                snprintf(out, outlen, "%s%s", dir, n);
                found = 1;
                if (e1 == 'z') break; // prefer .z64
            }
        }
        memset(&ent, 0, sizeof(ent));
    }
    sceIoDclose(d);
    return found;
}

#ifdef PORT_ASSETS_VERIFY
/* Compare the generated region against the build-time archive (with its symbol table). */
static void verify_against(const char* path) {
    struct AssetHeader h;
    u32 *rl, hdrlen, i, bad = 0, base = (u32) _ftext;
    FILE* f = fopen(path, "rb");
    if (f == NULL) { PORT_LOG("verify: %s missing\n", path); return; }
    PORT_LOG("verify: start\n");
    fread(&h, 1, sizeof(h), f);
    PORT_LOG("verify: header syms %u relocs %u names %u\n", (unsigned) h.sym_count, (unsigned) h.reloc_count, (unsigned) h.names_size);
    rl = (u32*) gPortMemoryPool; fread(rl, 4, h.reloc_count, f); /* the pool is free again once the cache is written */
    hdrlen = sizeof(h) + h.reloc_count * 4;
    {
        u8* symtab = gPortMemoryPool + 512 * 1024;
        u32 hdr2 = (sizeof(h) + h.reloc_count * 4 + h.sym_count * 16 + h.names_size + 15) & ~15u;
        fread(symtab, 1, h.sym_count * 16 + h.names_size, f);
        (void) hdrlen;
        for (i = 0; i < h.sym_count; i++) {
            u32 off = *(u32*) (symtab + i * 16), sz = *(u32*) (symtab + i * 16 + 4), nm = *(u32*) (symtab + i * 16 + 12);
            const char* name = (const char*) symtab + h.sym_count * 16 + nm;
            u32 firstbad = 0xFFFFFFFF, nbad = 0;
            u32 a0 = off & ~3u, a1 = (off + sz + 3) & ~3u, a;   /* aligned word grid: relocs are 4-aligned */
            for (a = a0; a < a1; a += 4) {
                u32 fv, mv, isrel, lo = 0, hi = h.reloc_count, k;
                u8 fb[4];
                fseek(f, hdr2 + a, SEEK_SET); fread(fb, 1, 4, f);
                memcpy(&fv, fb, 4); memcpy(&mv, __assets_start + a, 4);
                while (lo < hi) { u32 mid = (lo + hi) / 2; if (rl[mid] < a) lo = mid + 1; else hi = mid; }
                isrel = lo < h.reloc_count && rl[lo] == a;
                if (isrel) { if (fv + base != mv) { if (firstbad == 0xFFFFFFFF) firstbad = a - off; nbad++; } continue; }
                for (k = 0; k < 4; k++) { /* bytes of this word that belong to the symbol */
                    if (a + k < off || a + k >= off + sz) continue;
                    if (fb[k] != __assets_start[a + k]) { if (firstbad == 0xFFFFFFFF) firstbad = a + k - off; nbad++; break; }
                }
            }
            if (nbad) {
                static u32 shown_dl;
                bad++;
                if (bad <= 40) PORT_LOG("verify MISMATCH %s size %u first@%u (%u words)\n", name, (unsigned) sz, (unsigned) firstbad, (unsigned) nbad);
                if ((strstr(name, "packed_dl") ? shown_dl < 3 : shown_dl < 14) && shown_dl++ < 14) {
                    off += firstbad & ~3u; /* dump around the first bad word */
                    u8 fb[16]; fseek(f, hdr2 + off, SEEK_SET); fread(fb, 1, 16, f);
                    PORT_LOG("  DL %s expect %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X  got %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X\n", name,
                             fb[0],fb[1],fb[2],fb[3],fb[4],fb[5],fb[6],fb[7],fb[8],fb[9],fb[10],fb[11],fb[12],fb[13],fb[14],fb[15],
                             __assets_start[off],__assets_start[off+1],__assets_start[off+2],__assets_start[off+3],__assets_start[off+4],__assets_start[off+5],__assets_start[off+6],__assets_start[off+7],
                             __assets_start[off+8],__assets_start[off+9],__assets_start[off+10],__assets_start[off+11],__assets_start[off+12],__assets_start[off+13],__assets_start[off+14],__assets_start[off+15]);
                    off -= firstbad & ~3u;
                }
            }
            if ((i % 1000) == 0) PORT_LOG("verify: %u/%u\n", (unsigned) i, (unsigned) h.sym_count);
        }
        PORT_LOG("verify: %u of %u symbols mismatch\n", (unsigned) bad, (unsigned) h.sym_count);
    }
    fclose(f);
}
#endif

/* argv0: the EBOOT path from main(), used to look next to the executable. */
void port_assets_load(const char* argv0) {
    char dir[192] = "", path[256], rom[256], err[128];
    if (argv0 != NULL) {
        const char* slash = strrchr(argv0, '/');
        if (slash != NULL && (size_t) (slash + 1 - argv0) < sizeof(dir)) {
            memcpy(dir, argv0, slash + 1 - argv0);
            dir[slash + 1 - argv0] = 0;
        }
    }
    PORT_LOG("assets: argv0 %s dir %s\n", argv0 ? argv0 : "(null)", dir);
    strncpy(sSplashDir, dir, sizeof(sSplashDir) - 1);
    // 1. a cache from an earlier run (data/ next to the EBOOT)
    if (try_load(port_save_path("assets.bin"), err, sizeof(err))) return;
    PORT_LOG("assets: %sassets.bin: %s\n", port_save_dir(), err);
    // 2. the player's ROM next to the EBOOT
    if (!(dir[0] && find_rom(dir, rom, sizeof(rom))) && !find_rom(port_eboot_dir(), rom, sizeof(rom))) {
        fail_screen("No Mario Kart 64 ROM found.", "Copy your Mario Kart 64 (USA) .z64 file into the", "same folder as this EBOOT and start the game again.");
        return;
    }
    splash_begin(dir);
    splash_text_center(96, "First start: building the game data from your ROM");
    {
        const char* base = strrchr(rom, '/'); base = base ? base + 1 : rom;
        splash_text_center(112, base);
    }
    screen_progress("Reading the ROM", 0);
    if (!port_assets_generate(dir, rom, screen_progress, err, sizeof(err))) {
        fail_screen("Could not build the game data from the ROM:", err, "A Mario Kart 64 (USA) .z64 ROM is required.");
        return;
    }
    /* Cache right away: the recipe blob lives in the scratch pool and must not
     * be disturbed (the verifier below uses the pool's upper part). */
    snprintf(path, sizeof(path), "%s", port_save_path("assets.bin"));
    {
        extern int port_assets_crc_failed(void);
        if (port_assets_crc_failed()) {
            PORT_LOG("assets: not caching a mismatched extraction\n");
        } else if (!port_assets_write_cache(path, err, sizeof(err))) {
            PORT_LOG("assets: cache not written: %s\n", err); // the game still runs; next start extracts again
        } else {
            PORT_LOG("assets: cache written to %s\n", path);
        }
    }
#ifdef PORT_ASSETS_VERIFY
    verify_against(port_save_path("assets_ref.bin"));
#endif
    screen_progress("Done", 100);
    sceKernelDelayThread(400 * 1000);
}
