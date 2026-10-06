#include "resident.h"

#include <rg_system.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "recipe_index.h"
#include "mio0_index.h"
#include "course_relocs.h"

#define CACHE_DIR RG_BASE_PATH_CACHE "/mk64"
#define TRANSFORM_SCRATCH 65536u
#define MAX_MIO0_OUTPUT 184664u
#define MAX_PACKED_COURSE 131072u
#define MAX_UNPACKED_COURSE (51008u + 64u)

extern uintptr_t gHeapEndPtr;
extern int32_t gIsMirrorMode;
extern int32_t sGfxSeekPosition;
extern void displaylist_unpack(uintptr_t *packed, uintptr_t output_size,
                               uint32_t arg2);

static bool transform(uint8_t *data, size_t length, unsigned xform,
                      size_t first_word)
{
    if (xform == 0)
        return true;
    if (xform >= 16 && xform - 16 >= mk64_pattern_count)
        return false;

    const mk64_pattern_t *pattern = xform >= 16 ?
                                    &mk64_patterns[xform - 16] : NULL;
    for (size_t pos = 0; pos + 3 < length; pos += 4) {
        uint8_t *p = data + pos;
        uint8_t temp;
        unsigned shape = xform;
        if (pattern)
            shape = 16 + pattern->shapes[(first_word + pos / 4) % pattern->nwords];
        switch (shape) {
            case 1: /* sw16 */
            case 17: /* 22 */
                temp = p[0]; p[0] = p[1]; p[1] = temp;
                temp = p[2]; p[2] = p[3]; p[3] = temp;
                break;
            case 2: /* sw32 */
            case 16: /* 4 */
                temp = p[0]; p[0] = p[3]; p[3] = temp;
                temp = p[1]; p[1] = p[2]; p[2] = temp;
                break;
            case 18: /* 211 */
                temp = p[0]; p[0] = p[1]; p[1] = temp;
                break;
            case 19: /* 112 */
                temp = p[2]; p[2] = p[3]; p[3] = temp;
                break;
            case 20: /* 1111 */
                break;
            default:
                return false;
        }
    }
    return true;
}

static size_t next_chunk(size_t remaining, unsigned xform)
{
    size_t chunk = remaining < TRANSFORM_SCRATCH - 4 ?
                   remaining : TRANSFORM_SCRATCH - 4;
    if (chunk < remaining) {
        size_t period = 4;
        if (xform >= 16 && xform - 16 < mk64_pattern_count)
            period *= mk64_patterns[xform - 16].nwords;
        chunk -= chunk % period;
    }
    return chunk;
}

static bool emit_raw(mk64_rom_t *rom, const mk64_recipe_t *recipe,
                     uint8_t *scratch)
{
    uint8_t *dst = gMk64AssetRegion + recipe->dst;
    if (recipe->xform == 0)
        return mk64_rom_read(rom, recipe->src, dst, recipe->size);

    for (size_t done = 0; done < recipe->size;) {
        size_t chunk = next_chunk(recipe->size - done, recipe->xform);
        size_t padded = (chunk + 3) & ~3u;
        if (!chunk || !mk64_rom_read(rom, recipe->src + done, scratch, padded) ||
            !transform(scratch, padded, recipe->xform, done / 4))
            return false;
        memcpy(dst + done, scratch, chunk);
        done += chunk;
    }
    return true;
}

static bool emit_mio0(const mk64_recipe_t *recipe, const uint8_t *block,
                      size_t block_size, uint8_t *scratch)
{
    if ((uint64_t)recipe->extra + recipe->size > block_size)
        return false;
    uint8_t *dst = gMk64AssetRegion + recipe->dst;
    const uint8_t *source = block + recipe->extra;
    if (recipe->xform == 0) {
        memcpy(dst, source, recipe->size);
        return true;
    }
    for (size_t done = 0; done < recipe->size;) {
        size_t chunk = next_chunk(recipe->size - done, recipe->xform);
        size_t padded = (chunk + 3) & ~3u;
        size_t available = block_size - recipe->extra - done;
        size_t copy = padded < available ? padded : available;
        if (!chunk)
            return false;
        memcpy(scratch, source + done, copy);
        if (copy < padded)
            memset(scratch + copy, 0, padded - copy);
        if (!transform(scratch, padded, recipe->xform, done / 4))
            return false;
        memcpy(dst + done, scratch, chunk);
        done += chunk;
    }
    return true;
}

static const mk64_mio0_block_t *find_block(uint32_t offset)
{
    for (size_t i = 0; i < sizeof(mk64_mio0_blocks) /
                            sizeof(mk64_mio0_blocks[0]); ++i)
        if (mk64_mio0_blocks[i].offset == offset)
            return &mk64_mio0_blocks[i];
    return NULL;
}

static bool read_cached_block(uint32_t offset, uint8_t *buffer,
                              size_t *output_size)
{
    const mk64_mio0_block_t *block = find_block(offset);
    if (!block || block->output_size > MAX_MIO0_OUTPUT)
        return false;
    char path[RG_PATH_MAX + 1];
    int length = snprintf(path, sizeof(path), CACHE_DIR "/mio0_%08lx.bin",
                          (unsigned long)offset);
    if (length < 0 || length >= (int)sizeof(path))
        return false;
    FILE *file = fopen(path, "rb");
    if (!file)
        return false;
    bool okay = fread(buffer, 1, block->output_size, file) == block->output_size &&
                fgetc(file) == EOF;
    fclose(file);
    *output_size = block->output_size;
    return okay;
}

static bool unpack_course(mk64_rom_t *rom, size_t course_id,
                          uint8_t *packed, uint8_t *unpacked,
                          unsigned *count, uint32_t *bytes)
{
    const mk64_course_dl_t *course = &mk64_course_displaylists[course_id];
    if (course->rom_len > MAX_PACKED_COURSE ||
        (uint64_t)course->unpacked_len + 64 > MAX_UNPACKED_COURSE ||
        course->packed_off >= course->rom_len ||
        !mk64_rom_read(rom, course->rom_off, packed, course->rom_len))
        return false;

    memset(unpacked, 0, course->unpacked_len + 64);
    uintptr_t saved_heap_end = gHeapEndPtr;
    int32_t saved_mirror = gIsMirrorMode;
    gIsMirrorMode = 0;
    gHeapEndPtr = (uintptr_t)unpacked +
                  ((course->unpacked_len + 15u) & ~15u) + 8u;
    displaylist_unpack((uintptr_t *)(packed + course->packed_off),
                       course->unpacked_len, 0);
    gHeapEndPtr = saved_heap_end;
    gIsMirrorMode = saved_mirror;
    if (sGfxSeekPosition < 0 || (uint64_t)sGfxSeekPosition * 8 >
                                  course->unpacked_len + 64)
        return false;

    for (size_t i = 0; i < mk64_recipe_count; ++i) {
        const mk64_recipe_t *recipe = &mk64_recipes[i];
        if (recipe->kind != 5 || recipe->src != course_id)
            continue;
        if ((uint64_t)recipe->extra + recipe->size > course->unpacked_len)
            return false;
        memcpy(gMk64AssetRegion + recipe->dst, unpacked + recipe->extra,
               recipe->size);
        (*count)++;
        *bytes += recipe->size;
    }
    RG_LOGI("MK64: unpacked course %u/%u (%u bytes)",
            (unsigned)(course_id + 1),
            (unsigned)mk64_course_displaylist_count,
            (unsigned)course->unpacked_len);
    return true;
}

bool mk64_resident_load(mk64_rom_t *rom, char *error, size_t error_size)
{
    if (!rom || !error || !error_size)
        return false;
    error[0] = '\0';
    uint8_t *scratch = rg_alloc(TRANSFORM_SCRATCH, MEM_SLOW);
    uint8_t *block = rg_alloc(MAX_MIO0_OUTPUT, MEM_SLOW);
    uint8_t *packed = NULL;
    uint8_t *unpacked = NULL;
    if (!scratch || !block) {
        snprintf(error, error_size, "Insufficient RAM for asset recipes");
        free(block);
        free(scratch);
        return false;
    }

    int64_t began = rg_system_timer();
    memset(gMk64AssetRegion, 0, sizeof(gMk64AssetRegion));
    uint32_t loaded_block = UINT32_MAX;
    size_t block_size = 0;
    unsigned raw = 0, mio0 = 0, unpack_count = 0, deferred = 0;
    uint32_t raw_bytes = 0, mio0_bytes = 0, unpack_bytes = 0, deferred_bytes = 0;
    for (size_t i = 0; i < mk64_recipe_count; ++i) {
        const mk64_recipe_t *recipe = &mk64_recipes[i];
        if ((uint64_t)recipe->dst + recipe->size > sizeof(gMk64AssetRegion)) {
            snprintf(error, error_size, "Asset recipe %u exceeds region", (unsigned)i);
            goto fail;
        }
        if (recipe->kind == 1) {
            if (!emit_raw(rom, recipe, scratch)) {
                snprintf(error, error_size, "RAW recipe %u failed", (unsigned)i);
                goto fail;
            }
            raw++;
            raw_bytes += recipe->size;
        } else if (recipe->kind == 2) {
            if (recipe->src != loaded_block) {
                if (!read_cached_block(recipe->src, block, &block_size)) {
                    snprintf(error, error_size, "MIO0 cache read failed at %08lx",
                             (unsigned long)recipe->src);
                    goto fail;
                }
                loaded_block = recipe->src;
            }
            if (!emit_mio0(recipe, block, block_size, scratch)) {
                snprintf(error, error_size, "MIO0 recipe %u failed", (unsigned)i);
                goto fail;
            }
            mio0++;
            mio0_bytes += recipe->size;
        } else if (recipe->kind == 4) {
            deferred++;
            deferred_bytes += recipe->size;
        } else if (recipe->kind != 5) {
            snprintf(error, error_size, "Unknown asset recipe %u", (unsigned)i);
            goto fail;
        }
        if ((i & 2047u) == 0) {
            RG_LOGI("MK64: resident recipes %u/%u", (unsigned)i,
                    (unsigned)mk64_recipe_count);
            vTaskDelay(1);
        }
    }
    free(block);
    free(scratch);
    block = scratch = NULL;
    packed = heap_caps_aligned_alloc(16, MAX_PACKED_COURSE,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    unpacked = heap_caps_aligned_alloc(16, MAX_UNPACKED_COURSE,
                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!packed || !unpacked) {
        snprintf(error, error_size, "Insufficient RAM for course unpacking");
        goto fail;
    }
    for (size_t course = 0; course < mk64_course_displaylist_count; ++course) {
        if (!unpack_course(rom, course, packed, unpacked,
                           &unpack_count, &unpack_bytes)) {
            snprintf(error, error_size, "Course unpack failed at %u",
                     (unsigned)course);
            goto fail;
        }
    }
    mk64_course_relocs_apply(gMk64AssetRegion);
    free(unpacked);
    free(packed);
    unpacked = packed = NULL;
    RG_LOGI("MK64: resident assets: RAW %u/%u bytes, MIO0 %u/%u bytes, UNPACK %u/%u bytes, reloc %u/%u bytes, deferred %u/%u bytes, %u ms",
            raw, (unsigned)raw_bytes, mio0, (unsigned)mio0_bytes,
            unpack_count, (unsigned)unpack_bytes,
            (unsigned)MK64_RELOC_TABLE_COUNT, (unsigned)MK64_RELOC_POINTER_BYTES,
            deferred - MK64_RELOC_TABLE_COUNT,
            (unsigned)(deferred_bytes - MK64_RELOC_RECIPE_BYTES),
            (unsigned)((rg_system_timer() - began) / 1000));
    RG_LOGI("MK64: PSRAM after resident load: free %u, largest %u bytes",
            (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
            (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    return true;

fail:
    free(unpacked);
    free(packed);
    free(block);
    free(scratch);
    return false;
}
