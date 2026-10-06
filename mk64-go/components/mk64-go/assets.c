#include "assets.h"

#include <rg_system.h>

#include <mbedtls/sha1.h>
#include <esp_heap_caps.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mio0.h"
#include "mio0_index.h"

#define CACHE_DIR RG_BASE_PATH_CACHE "/mk64"
#define SHA1_BUFFER_SIZE 4096
#define MAX_COMPRESSED 76063
#define MAX_UNPACKED 184664

static const uint8_t us_sha1[20] = {
    0x57, 0x9c, 0x48, 0xe2, 0x11, 0xae, 0x95, 0x25, 0x30, 0xff,
    0xc8, 0x73, 0x87, 0x09, 0xf0, 0x78, 0xd5, 0xdd, 0x21, 0x5e,
};

static void log_psram(const char *stage)
{
    RG_LOGI("MK64: PSRAM %s: free %u, largest block %u bytes", stage,
            (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
            (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

static bool validate_sha1(mk64_rom_t *rom)
{
    if (rom->size != 0xc00000u)
        return false;

    uint8_t *buffer = rg_alloc(SHA1_BUFFER_SIZE, MEM_SLOW);
    if (!buffer)
        return false;

    mbedtls_sha1_context context;
    mbedtls_sha1_init(&context);
    int result = mbedtls_sha1_starts(&context);
    for (uint32_t offset = 0; !result && offset < rom->size;
         offset += SHA1_BUFFER_SIZE) {
        if (!mk64_rom_read(rom, offset, buffer, SHA1_BUFFER_SIZE)) {
            result = -1;
            break;
        }
        result = mbedtls_sha1_update(&context, buffer, SHA1_BUFFER_SIZE);
    }
    uint8_t digest[20];
    if (!result)
        result = mbedtls_sha1_finish(&context, digest);
    mbedtls_sha1_free(&context);
    free(buffer);
    return result == 0 && memcmp(digest, us_sha1, sizeof(digest)) == 0;
}

static bool cache_file_valid(const char *path, uint32_t size)
{
    rg_stat_t stat = rg_storage_stat(path);
    return stat.is_file && stat.size == size;
}

static bool write_cache_file(const char *path, const void *data, size_t size)
{
    char temporary[RG_PATH_MAX + 1];
    int length = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    if (length < 0 || length >= (int)sizeof(temporary) ||
        !rg_storage_write_file(temporary, data, size, 0))
        return false;

    if (rg_storage_exists(path) && !rg_storage_delete(path)) {
        rg_storage_delete(temporary);
        return false;
    }
    if (rename(temporary, path) != 0) {
        rg_storage_delete(temporary);
        return false;
    }
    return true;
}

bool mk64_assets_prepare(mk64_rom_t *rom, bool *created,
                         char *error, size_t error_size)
{
    char path[RG_PATH_MAX + 1];
    uint8_t *compressed = NULL;
    uint8_t *unpacked = NULL;
    size_t extracted = 0;
    int64_t began = rg_system_timer();

    if (created)
        *created = false;
    if (!rom || !error || !error_size)
        return false;
    error[0] = '\0';

    log_psram("before extraction");

    RG_LOGI("MK64: verifying USA ROM SHA-1 (%lu bytes)",
            (unsigned long)rom->size);
    if (!validate_sha1(rom)) {
        snprintf(error, error_size, "ROM SHA-1 mismatch; USA revision required");
        return false;
    }
    RG_LOGI("MK64: ROM SHA-1 verified in %u ms",
            (unsigned)((rg_system_timer() - began) / 1000));

    if (!rg_storage_mkdir(CACHE_DIR)) {
        snprintf(error, error_size, "Could not create MK64 cache directory");
        return false;
    }

    for (size_t i = 0; i < sizeof(mk64_mio0_blocks) / sizeof(mk64_mio0_blocks[0]); ++i) {
        const mk64_mio0_block_t *block = &mk64_mio0_blocks[i];
        int length = snprintf(path, sizeof(path), CACHE_DIR "/mio0_%08lx.bin",
                              (unsigned long)block->offset);
        if (length < 0 || length >= (int)sizeof(path)) {
            snprintf(error, error_size, "MK64 cache path is too long");
            goto fail;
        }
        if (cache_file_valid(path, block->output_size))
            continue;

        if (!compressed) {
            compressed = rg_alloc(MAX_COMPRESSED, MEM_SLOW);
            unpacked = rg_alloc(MAX_UNPACKED, MEM_SLOW);
            if (!compressed || !unpacked) {
                snprintf(error, error_size, "Insufficient memory for MK64 extraction");
                goto fail;
            }
            RG_LOGI("MK64: extraction scratch allocated: %u bytes",
                    MAX_COMPRESSED + MAX_UNPACKED);
            log_psram("with scratch allocated");
        }
        if (block->compressed_size > MAX_COMPRESSED ||
            block->output_size > MAX_UNPACKED ||
            !mk64_rom_read(rom, block->offset, compressed, block->compressed_size) ||
            !mk64_mio0_decode(compressed, block->compressed_size,
                              unpacked, block->output_size)) {
            snprintf(error, error_size, "MIO0 decode failed at ROM offset %08lx",
                     (unsigned long)block->offset);
            goto fail;
        }
        if (!write_cache_file(path, unpacked, block->output_size)) {
            snprintf(error, error_size, "Could not write MK64 cache block %u",
                     (unsigned)i);
            goto fail;
        }
        extracted++;
        RG_LOGI("MK64: cached block %u/%u, ROM %08lx, %lu bytes",
                (unsigned)(i + 1), (unsigned)(sizeof(mk64_mio0_blocks) /
                sizeof(mk64_mio0_blocks[0])), (unsigned long)block->offset,
                (unsigned long)block->output_size);
    }

    free(unpacked);
    free(compressed);
    log_psram("after extraction");
    if (created)
        *created = extracted != 0;
    RG_LOGI("MK64: MIO0 cache ready (%u new blocks, %u ms total)",
            (unsigned)extracted,
            (unsigned)((rg_system_timer() - began) / 1000));
    return true;

fail:
    free(unpacked);
    free(compressed);
    log_psram("after extraction failure");
    return false;
}
