/* First-run asset generation: rebuild the asset region from the player's own
 * Mario Kart 64 ROM by executing the recipe table (port_assets.h), then cache
 * the result as assets.bin next to the EBOOT.  See tools/psp/derive_recipes.py
 * for how the recipes are found. */
#include <PR/ultratypes.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "port.h"
#include "port_assets.h"

extern u8 __assets_start[], __assets_end[];
extern u8 _ftext[];
extern u8 gPortMemoryPool[]; /* 3 MB game pool, free until the game starts: our scratch */
extern void mio0decode(u8* in, u8* out);
extern void displaylist_unpack(uintptr_t* data, uintptr_t finalDisplaylistOffset, u32 arg2);
extern uintptr_t gHeapEndPtr;
static const PortAssetPattern* sPatterns; /* set by blob_parse */

/* ------------------------------------------------------------------ ROM */
static FILE* sRom;
static int sRomMode; /* 0 = z64 (big-endian), 1 = v64 (byte-swapped pairs), 2 = n64 (little-endian words) */

static void fix_order(u8* b, u32 n) {
    u32 i;
    if (sRomMode == 1) {
        for (i = 0; i + 1 < n; i += 2) { u8 t = b[i]; b[i] = b[i + 1]; b[i + 1] = t; }
    } else if (sRomMode == 2) {
        for (i = 0; i + 3 < n; i += 4) { u8 t = b[i]; b[i] = b[i + 3]; b[i + 3] = t; t = b[i + 1]; b[i + 1] = b[i + 2]; b[i + 2] = t; }
    }
}

/* Read `len` bytes at ROM offset `off` (any alignment) in big-endian order. */
static int rom_read(u32 off, u8* dst, u32 len) {
    u32 a = off & ~3u, e = (off + len + 3) & ~3u;
    static u8 tmp[4096];
    u32 pos = a;
    while (pos < e) {
        u32 n = e - pos; if (n > sizeof(tmp)) n = sizeof(tmp);
        fseek(sRom, pos, SEEK_SET);
        if (fread(tmp, 1, n, sRom) != n) return 0;
        fix_order(tmp, n);
        {
            u32 s = pos < off ? off - pos : 0;
            u32 c = pos + n > off + len ? off + len - (pos + s) : n - s;
            memcpy(dst + (pos + s - off), tmp + s, c);
        }
        pos += n;
    }
    return 1;
}

int port_rom_open(const char* path, char* err, u32 errlen) {
    u8 hdr[0x40];
    sRom = fopen(path, "rb");
    if (sRom == NULL) { snprintf(err, errlen, "cannot open"); return 0; }
    sRomMode = 0;
    if (fread(hdr, 1, 4, sRom) != 4) { snprintf(err, errlen, "unreadable"); fclose(sRom); sRom = NULL; return 0; }
    if (hdr[0] == 0x80 && hdr[1] == 0x37) sRomMode = 0;
    else if (hdr[0] == 0x37 && hdr[1] == 0x80) sRomMode = 1;
    else if (hdr[0] == 0x40 && hdr[1] == 0x12) sRomMode = 2;
    else { snprintf(err, errlen, "not an N64 ROM"); fclose(sRom); sRom = NULL; return 0; }
    rom_read(0, hdr, sizeof(hdr));
    if (memcmp(hdr + 0x20, "MARIOKART64", 11) != 0) { snprintf(err, errlen, "not Mario Kart 64"); fclose(sRom); sRom = NULL; return 0; }
    if (hdr[0x3E] != 'E') { snprintf(err, errlen, "not the US (NTSC) version (region '%c')", hdr[0x3E]); fclose(sRom); sRom = NULL; return 0; }
    return 1;
}

/* ------------------------------------------------------------------ transforms */
static void xf_sw16(u8* b, u32 n) { u32 i; for (i = 0; i + 1 < n; i += 2) { u8 t = b[i]; b[i] = b[i + 1]; b[i + 1] = t; } }
static void xf_sw32(u8* b, u32 n) { u32 i; for (i = 0; i + 3 < n; i += 4) { u8 t = b[i]; b[i] = b[i + 3]; b[i + 3] = t; t = b[i + 1]; b[i + 1] = b[i + 2]; b[i + 2] = t; } }
static void xf_pattern(u8* b, u32 n, const PortAssetPattern* p) {
    u32 w, nw = n / 4;
    for (w = 0; w < nw; w++) {
        u8* q = b + w * 4; u8 t;
        switch (p->shapes[w % p->nwords]) {
            case PA_SHAPE_4:    t = q[0]; q[0] = q[3]; q[3] = t; t = q[1]; q[1] = q[2]; q[2] = t; break;
            case PA_SHAPE_22:   t = q[0]; q[0] = q[1]; q[1] = t; t = q[2]; q[2] = q[3]; q[3] = t; break;
            case PA_SHAPE_211:  t = q[0]; q[0] = q[1]; q[1] = t; break;
            case PA_SHAPE_112:  t = q[2]; q[2] = q[3]; q[3] = t; break;
            default: break;
        }
    }
}
static void apply_xform(u8* b, u32 n, u16 xf) {
    if (xf == PA_XF_SW16) xf_sw16(b, n);
    else if (xf == PA_XF_SW32) xf_sw32(b, n);
    else if (xf >= PA_XF_PATTERN) xf_pattern(b, n, &sPatterns[xf - PA_XF_PATTERN]);
}
/* The recipes were matched against whole transformed streams, so a symbol
 * whose size is not a multiple of the transform's word gets its last partial
 * word from the transformed neighbourhood: transform a rounded-up range in a
 * temp buffer and keep `size` bytes. */
static u8* sXfTmp;
static void emit_xformed(u8* dst, const u8* src, u32 size, u16 xf) {
    u32 n = (size + 3) & ~3u;
    if (xf == PA_XF_ID) { memcpy(dst, src, size); return; }
    memcpy(sXfTmp, src, n);
    apply_xform(sXfTmp, n, xf);
    memcpy(dst, sXfTmp, size);
}

/* ------------------------------------------------------------------ crc */
/* Standard CRC-32 (same as zlib's crc32(), incrementally: start from 0). */
static u32 crc_table[256];
static int crc_ready;
static void crc_init(void) { u32 i, j; if (crc_ready) return; crc_ready = 1; for (i = 0; i < 256; i++) { u32 c = i; for (j = 0; j < 8; j++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1; crc_table[i] = c; } }
static u32 crc_update(u32 crc, const u8* b, u32 n) { u32 i; crc = ~crc; for (i = 0; i < n; i++) crc = crc_table[(crc ^ b[i]) & 0xFF] ^ (crc >> 8); return ~crc; }
u32 port_crc32(u32 crc, const void* b, u32 n) { crc_init(); return crc_update(crc, (const u8*) b, n); }

/* ------------------------------------------------------------------ recipe blob */
static const PortAssetBlobHeader* sH;
static const PortAssetRecipe* sRecipes;
static const u8* sLiterals;
static const PortAssetPattern* sPatterns;
static const PortAssetCourseDl* sCourses;
static const PortAssetBlock* sBlocks;
static const PortAssetReloc* sRelocs;

static u32 read_le32(const u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }

/* Read one entry of our EBOOT's DATA.PSAR container (tools/psp/make_psar.py):
 * MK64PSAR | count | {tag, off, size}... | payloads.  A bare MK64RCP1 blob is
 * accepted as the RCP1 entry.  Returns the size read, 0 if absent. */
u32 port_psar_read(const char* dir, const char* tag, u8* buf, u32 cap, char* err, u32 errlen) {
    char path[256];
    const char* cands[2];
    u32 n = 0, i;
    if (dir && dir[0]) { snprintf(path, sizeof(path), "%sEBOOT.PBP", dir); cands[n++] = path; }
    cands[n++] = "EBOOT.PBP";
    for (i = 0; i < n; i++) {
        FILE* f = fopen(cands[i], "rb");
        u8 head[0x28];
        u32 psar, end;
        if (f == NULL) continue;
        if (fread(head, 1, sizeof(head), f) == sizeof(head) && memcmp(head, "\0PBP", 4) == 0) {
            psar = read_le32(head + 0x24);
            fseek(f, 0, SEEK_END); end = (u32) ftell(f);
            if (psar != 0 && psar < end) {
                u8 ch[12];
                fseek(f, psar, SEEK_SET);
                if (fread(ch, 1, 12, f) == 12 && memcmp(ch, "MK64PSAR", 8) == 0) {
                    u32 count = read_le32(ch + 8), k;
                    for (k = 0; k < count; k++) {
                        u8 e[12];
                        fseek(f, psar + 12 + k * 12, SEEK_SET);
                        if (fread(e, 1, 12, f) != 12) break;
                        if (memcmp(e, tag, 4) == 0) {
                            u32 off = read_le32(e + 4), size = read_le32(e + 8);
                            if (size > cap) { snprintf(err, errlen, "%s too large (%u)", tag, (unsigned) size); fclose(f); return 0; }
                            fseek(f, psar + off, SEEK_SET);
                            if (fread(buf, 1, size, f) == size) { fclose(f); return size; }
                        }
                    }
                } else if (memcmp(ch, "MK64RCP1", 8) == 0 && memcmp(tag, "RCP1", 4) == 0) {
                    u32 len = end - psar;
                    if (len <= cap) { fseek(f, psar, SEEK_SET); if (fread(buf, 1, len, f) == len) { fclose(f); return len; } }
                }
            }
        }
        fclose(f);
    }
    snprintf(err, errlen, "%s not in EBOOT.PBP", tag);
    return 0;
}

/* Load the recipe blob into `buf` (limit `cap`): from the EBOOT's PSAR, or a
 * loose recipes.bin next to the EBOOT or in its data/ folder. */
static u32 blob_load(const char* dir, u8* buf, u32 cap, char* err, u32 errlen) {
    u32 len = port_psar_read(dir, "RCP1", buf, cap, err, errlen), i;
    if (len && memcmp(buf, "MK64RCP1", 8) == 0) return len;
    {
        static const char* loose[2] = { NULL, NULL };
        char p2[256];
        snprintf(p2, sizeof(p2), "%srecipes.bin", dir && dir[0] ? dir : port_save_dir());
        loose[0] = p2; loose[1] = port_save_path("recipes.bin");
        for (i = 0; i < 2; i++) {
            FILE* f = fopen(loose[i], "rb");
            if (f == NULL) continue;
            fseek(f, 0, SEEK_END); len = (u32) ftell(f); fseek(f, 0, SEEK_SET);
            if (len <= cap && fread(buf, 1, len, f) == len && memcmp(buf, "MK64RCP1", 8) == 0) { fclose(f); return len; }
            fclose(f);
        }
    }
    snprintf(err, errlen, "recipe table missing (EBOOT.PBP has no DATA.PSAR)");
    return 0;
}

static int blob_parse(u8* buf, u32 len) {
    const u8* p = buf;
    sH = (const PortAssetBlobHeader*) p; p += sizeof(*sH);
    sRecipes = (const PortAssetRecipe*) p; p += sH->recipe_count * sizeof(PortAssetRecipe);
    sLiterals = p; p += (sH->literal_size + 3) & ~3u;
    sPatterns = (const PortAssetPattern*) p; p += sH->pattern_count * sizeof(PortAssetPattern);
    sCourses = (const PortAssetCourseDl*) p; p += sH->course_count * sizeof(PortAssetCourseDl);
    sBlocks = (const PortAssetBlock*) p; p += sH->block_count * sizeof(PortAssetBlock);
    sRelocs = (const PortAssetReloc*) p; p += sH->reloc_count * sizeof(PortAssetReloc);
    return (u32) (p - buf) <= len;
}

/* ------------------------------------------------------------------ generate */
static u8* sBlockBuf;  /* decompressed mio0 block */
static u32 sBlockRom = 0xFFFFFFFF;
static u8* sCompBuf;   /* compressed block read from the ROM */
static u8* sUnpackBuf; /* displaylist_unpack output */

static const PortAssetBlock* find_block(u32 rom_off) {
    u32 i;
    for (i = 0; i < sH->block_count; i++) if (sBlocks[i].rom_off == rom_off) return &sBlocks[i];
    return NULL;
}

static int load_block(u32 rom_off, char* err, u32 errlen) {
    const PortAssetBlock* bk;
    if (sBlockRom == rom_off) return 1;
    bk = find_block(rom_off);
    if (bk == NULL) { snprintf(err, errlen, "unknown mio0 block %08X", (unsigned) rom_off); return 0; }
    if (!rom_read(rom_off, sCompBuf, bk->rom_len)) { snprintf(err, errlen, "ROM read failed at %08X", (unsigned) rom_off); return 0; }
    if (memcmp(sCompBuf, "MIO0", 4) != 0) { snprintf(err, errlen, "no MIO0 block at %08X (wrong ROM?)", (unsigned) rom_off); return 0; }
    mio0decode(sCompBuf, sBlockBuf);
    sBlockRom = rom_off;
    return 1;
}

static int sCrcFailed;
int port_assets_crc_failed(void) { return sCrcFailed; }
#define POOL_BYTES (3 * 1024 * 1024)
int port_assets_generate(const char* dir, const char* rom_path, void (*progress)(const char*, int), char* err, u32 errlen) {
    u32 i, base = (u32) _ftext, pending = 0, blob_len, scratch;
    u32 region_size = (u32) (__assets_end - __assets_start);
    u8* region = __assets_start;
    uintptr_t saved_heap_end;
    u32 t0 = sceKernelGetSystemTimeLow();
    /* scratch inside the (still unused) 3 MB game pool: blob | block | compressed | unpack */
    blob_len = blob_load(dir, gPortMemoryPool, POOL_BYTES / 2, err, errlen);
    if (blob_len == 0) return 0;
    if (!blob_parse(gPortMemoryPool, blob_len)) { snprintf(err, errlen, "recipe table is corrupt"); return 0; }
    if (region_size != sH->region_size) { snprintf(err, errlen, "recipe table is for another build"); return 0; }
    scratch = (blob_len + 63) & ~63u;
    if (scratch + 2 * ((sH->max_block + 63) & ~63u) + sH->max_unpacked + 64 * 1024 + 256 > POOL_BYTES) { snprintf(err, errlen, "not enough scratch memory"); return 0; }
    if (!port_rom_open(rom_path, err, errlen)) return 0;
    sBlockBuf = gPortMemoryPool + scratch;
    sCompBuf = sBlockBuf + ((sH->max_block + 63) & ~63u);
    sUnpackBuf = sCompBuf + ((sH->max_block + 63) & ~63u);   /* compressed <= decompressed */
    sXfTmp = sUnpackBuf + ((sH->max_unpacked + 63) & ~63u); /* 64 KB + 4 */
    sBlockRom = 0xFFFFFFFF;
    memset(region, 0, region_size);

    for (i = 0; i < sH->recipe_count; i++) {
        const PortAssetRecipe* r = &sRecipes[i];
        u8* dst = region + r->dst;
        switch (r->kind) {
            case PA_RAW:
                if (r->xform == PA_XF_ID) {
                    if (!rom_read(r->src, dst, r->size)) { snprintf(err, errlen, "ROM read failed at %08X", (unsigned) r->src); fclose(sRom); return 0; }
                } else {
                    u32 done = 0;
                    while (done < r->size) { /* through the temp buffer, in bounded pieces */
                        u32 n = r->size - done; if (n > 64 * 1024) n = 64 * 1024;
                        if (!rom_read(r->src + done, sCompBuf, (n + 3) & ~3u)) { snprintf(err, errlen, "ROM read failed at %08X", (unsigned) r->src); fclose(sRom); return 0; }
                        emit_xformed(dst + done, sCompBuf, n, r->xform);
                        done += n;
                    }
                }
                break;
            case PA_MIO0:
                if (!load_block(r->src, err, errlen)) { fclose(sRom); return 0; }
                emit_xformed(dst, sBlockBuf + r->extra, r->size, r->xform);
                break;
            case PA_LITERAL:
                memcpy(dst, sLiterals + r->src, r->size);
                break;
            case PA_RELOCS:
            case PA_UNPACK:
                pending++;
                break;
        }
        if (progress && (i & 255) == 0) progress("Extracting assets from your ROM", (int) (i * 70 / sH->recipe_count));
    }

    /* Packed course display lists: what load_course does on the N64. */
    saved_heap_end = gHeapEndPtr;
    for (i = 0; i < sH->course_count; i++) {
        const PortAssetCourseDl* c = &sCourses[i];
        u32 k;
        /* the packed stream is raw ROM data right after the course's vertex mio0 block */
        if (c->rom_len > ((sH->max_block + 63) & ~63u) || !rom_read(c->rom_off, sCompBuf, c->rom_len)) {
            snprintf(err, errlen, "ROM read failed at %08X", (unsigned) c->rom_off); fclose(sRom); gHeapEndPtr = saved_heap_end; return 0;
        }
        sBlockRom = 0xFFFFFFFF; /* sCompBuf reused */
        gHeapEndPtr = (uintptr_t) sUnpackBuf + ((c->unpacked_len + 15) & ~15u) + 8; /* unpack writes at gHeapEndPtr - (ALIGN16(len)+8) */
        memset(sUnpackBuf, 0, c->unpacked_len + 64);
        displaylist_unpack((uintptr_t*) (sCompBuf + c->packed_off), c->unpacked_len, 0);
#ifdef PORT_ASSETS_VERIFY
        if (i == 0) { FILE* df = fopen(port_save_path("unpack0.bin"), "wb"); if (df) { fwrite(sUnpackBuf, 1, c->unpacked_len + 64, df); fclose(df); }
                      df = fopen(port_save_path("packed0.bin"), "wb"); if (df) { fwrite(sCompBuf, 1, c->rom_len, df); fclose(df); } }
#endif
        for (k = 0; k < sH->recipe_count; k++) {
            const PortAssetRecipe* r = &sRecipes[k];
            if (r->kind == PA_UNPACK && r->src == c->course) memcpy(region + r->dst, sUnpackBuf + r->extra, r->size);
        }
        if (progress) progress("Unpacking course display lists", 70 + (int) (i * 25 / sH->course_count));
    }
    gHeapEndPtr = saved_heap_end;
    fclose(sRom); sRom = NULL;

    /* Verify before pointers go in: crc of the region with pointer words zero
     * (literal recipes carry their link-time pointer values: clear them). */
    for (i = 0; i < sH->reloc_count; i++) *(u32*) (region + (sRelocs[i].off & 0x7FFFFFFFu)) = 0;
    {
        u32 crc;
        crc_init();
        crc = crc_update(0, region, region_size);
        if (crc != sH->data_crc) {
            snprintf(err, errlen, "extracted data does not match (crc %08X, expected %08X) - wrong ROM?", (unsigned) crc, (unsigned) sH->data_crc);
#ifdef PORT_ASSETS_VERIFY
            PORT_LOG("assets: CRC MISMATCH %s (continuing for verification)\n", err);
            sCrcFailed = 1;
#else
            return 0;
#endif
        }
    }
    for (i = 0; i < sH->reloc_count; i++) {
        const PortAssetReloc* rl = &sRelocs[i];
        u32 v = (rl->off & 0x80000000u) ? base + rl->target : (u32) region + rl->target; /* flag on the offset */
        *(u32*) (region + (rl->off & 0x7FFFFFFFu)) = v;
    }
    PORT_LOG("assets: generated from %s in %u ms\n", rom_path, (unsigned) ((sceKernelGetSystemTimeLow() - t0) / 1000));
    if (progress) progress("Saving assets.bin", 96);
    return 1;
}

/* Write the cache in assets_load.c's format: pointer words hold link-time
 * values, and the header carries the CRC-32 of the data (checked on load). */
int port_assets_write_cache(const char* path, char* err, u32 errlen) {
    u32 base = (u32) _ftext, region_size = (u32) (__assets_end - __assets_start);
    u32 hdr[11]; u32 i, off = 0, ri = 0, hdrlen, crc = 0;
    FILE* f = fopen(path, "wb");
    static u8 chunk[64 * 1024];
    if (f == NULL) { snprintf(err, errlen, "cannot create %s", path); return 0; }
    memcpy(hdr, "MK64ASST", 8);
    hdr[2] = 1; hdr[3] = (u32) __assets_start - base; hdr[4] = region_size; hdr[5] = sH->reloc_count;
    hdr[6] = 0; hdr[7] = 0; hdr[8] = 0; hdr[9] = 0; hdr[10] = 0; /* data_crc (hdr[9]) is patched in at the end */
    fwrite(hdr, 1, 44, f);
    for (i = 0; i < sH->reloc_count; i++) { u32 o = sRelocs[i].off & 0x7FFFFFFFu; fwrite(&o, 4, 1, f); }
    hdrlen = 44 + sH->reloc_count * 4;
    while (hdrlen & 15) { fputc(0, f); hdrlen++; }
    while (off < region_size) {
        u32 n = region_size - off; if (n > sizeof(chunk)) n = sizeof(chunk);
        memcpy(chunk, __assets_start + off, n);
        while (ri < sH->reloc_count && (sRelocs[ri].off & 0x7FFFFFFFu) < off + n) {
            u32 o = (sRelocs[ri].off & 0x7FFFFFFFu) - off;
            u32 v = *(u32*) (chunk + o) - base; /* runtime pointer -> link-time value */
            memcpy(chunk + o, &v, 4);
            ri++;
        }
        crc = port_crc32(crc, chunk, n);
        if (fwrite(chunk, 1, n, f) != n) { snprintf(err, errlen, "write failed (memory stick full?)"); fclose(f); remove(path); return 0; }
        off += n;
    }
    fseek(f, 9 * 4, SEEK_SET);
    fwrite(&crc, 4, 1, f);
    if (fclose(f) != 0) { snprintf(err, errlen, "write failed (memory stick full?)"); remove(path); return 0; }
    return 1;
}
