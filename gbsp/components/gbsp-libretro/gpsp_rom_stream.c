/* P4 ROM read transport. One bounded scratch buffer for the application's
 * serialized ROM loading, page misses and state/menu cache restoration. */
#include "streams/file_stream.h"
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include <unistd.h>
#include "esp_heap_caps.h"
#include "gpsp_log.h"

static uint8_t *read_stage;
static size_t read_stage_size;
static bool read_stage_tried;

static void prepare_read_stage(void)
{
    if (read_stage_tried) return;
    read_stage_tried = true;
    const size_t sizes[] = {32 * 1024, 16 * 1024, 8 * 1024};
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        /* Unlike rg_alloc's location preferences, this scratch must actually
         * be internal and byte-accessible for SD staging to be useful/safe. */
        read_stage = heap_caps_malloc(sizes[i], MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!read_stage)
            read_stage = heap_caps_malloc(sizes[i], MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (read_stage) {
            read_stage_size = sizes[i];
            RG_LOGI("ROM I/O: %u KB reusable internal read buffer", (unsigned)(read_stage_size / 1024));
            return;
        }
    }
    RG_LOGW("ROM I/O: no internal read buffer; using direct reads");
}
#endif

void filestream_read_cleanup(void)
{
#ifdef ESP_PLATFORM
    free(read_stage);
    read_stage = NULL;
    read_stage_size = 0;
    read_stage_tried = false;
#endif
}

int64_t filestream_read(RFILE *f, void *buf, int64_t len)
{
    if (!f || len < 0 || (uint64_t)len > SIZE_MAX || (!buf && len)) {
        errno = EINVAL;
        return -1;
    }
    if (!len) return 0;
#ifdef ESP_PLATFORM
    prepare_read_stage();
    if (read_stage) {
        /* Preserve the existing descriptor/stdio synchronization. The checked
         * ROM boundary seeks before each read and rejects incomplete pages. */
        int fd = fileno(f);
        long fpos = ftell(f);
        if (fd < 0 || fpos < 0 || lseek(fd, fpos, SEEK_SET) < 0) return -1;
        uint8_t *dst = buf;
        int64_t remaining = len, total = 0;
        while (remaining > 0) {
            size_t chunk = remaining > (int64_t)read_stage_size ? read_stage_size : (size_t)remaining;
            ssize_t got = read(fd, read_stage, chunk);
            if (got < 0) { total = -1; break; }
            if (got == 0) break;
            memcpy(dst, read_stage, (size_t)got);
            dst += got;
            remaining -= got;
            total += got;
        }
        if (total >= 0 && fseek(f, fpos + total, SEEK_SET)) total = -1;
        return total;
    }
#endif
    size_t got = fread(buf, 1, (size_t)len, f);
    return ferror(f) ? -1 : (int64_t)got;
}
