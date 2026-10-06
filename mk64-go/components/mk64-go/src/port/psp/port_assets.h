/* Recipe blob: how the PSP rebuilds the asset region from the player's ROM.
 * Written by tools/psp/emit_recipes.py into the EBOOT's DATA.PSAR (numbers
 * only -- no game data) and parsed by assets_gen.c in scratch memory. */
#ifndef PORT_ASSETS_H
#define PORT_ASSETS_H

enum { PA_RAW = 1, PA_MIO0 = 2, PA_LITERAL = 3, PA_RELOCS = 4, PA_UNPACK = 5 };
enum { PA_XF_ID = 0, PA_XF_SW16 = 1, PA_XF_SW32 = 2, PA_XF_PATTERN = 16 }; /* >= 16: pattern index + 16 */
enum { PA_SHAPE_4 = 0, PA_SHAPE_22 = 1, PA_SHAPE_211 = 2, PA_SHAPE_112 = 3, PA_SHAPE_1111 = 4 };

typedef struct {
    char magic[8]; /* MK64RCP1 */
    unsigned int region_size, data_crc, recipe_count, literal_size, pattern_count, course_count, block_count, reloc_count,
        max_block, max_unpacked;
} PortAssetBlobHeader;

typedef struct {
    unsigned int dst, size;
    unsigned short kind, xform;
    unsigned int src;   /* RAW: ROM offset. MIO0: ROM offset of the block. LITERAL: offset in the literal blob. UNPACK: course index */
    unsigned int extra; /* MIO0: offset in the decompressed block. UNPACK: offset in the unpacked stream */
} PortAssetRecipe;

typedef struct { unsigned int nwords; unsigned char shapes[16]; } PortAssetPattern;
typedef struct { unsigned int course, unpacked_len, rom_off, rom_len, packed_off; } PortAssetCourseDl;
typedef struct { unsigned int rom_off, rom_len, decomp_len; } PortAssetBlock;
typedef struct { unsigned int off, target; } PortAssetReloc; /* off bit 31 set: target is a module-relative link address; else region-relative */

#endif
