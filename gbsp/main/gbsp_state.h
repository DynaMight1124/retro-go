#pragma once
#include "gpsp_state.h"
#include "gbsp_state_file.h"
#ifndef GP_STATE_WRITE
#define GP_STATE_WRITE(filename, data, size) gp_state_write(filename, data, size)
#endif

static size_t state_buffer_size(void)
{
#if defined(RISCV_ARCH)
    return gpsp_state_size();
#else
    return GBA_STATE_MEM_SIZE;
#endif
}

/* Retro-Go presents four slots. Repair its interrupted rename/failure path
 * before the next menu scan or cold resume can treat an old slot as missing. */
static void gp_state_recover_slots(void)
{
    for (unsigned slot = 0; slot < 4; slot++) {
        char *path = rg_emu_get_path(RG_PATH_SAVE_STATE + slot, app->romPath);
        if (path) {
            if (gp_state_recover(path, state_buffer_size()))
                RG_LOGI("State: recovered backup slot %u", slot);
            free(path);
        }
    }
}

static void *state_buffer_get(size_t size, bool *borrowed)
{
    void *buffer = malloc(size);
    *borrowed = false;
    if (!buffer && size <= 1024u * 1024u) {
        buffer = gamepak_borrow_block();
        *borrowed = buffer != NULL;
        if (buffer) RG_LOGD("State: using ROM-cache buffer (%u bytes)", (unsigned)size);
    }
    if (!buffer) RG_LOGE("State: cannot obtain %u-byte buffer", (unsigned)size);
    return buffer;
}

static void state_buffer_put(void *buffer, bool borrowed)
{
    if (borrowed) {
        /* Never resume a CPU that could read overwritten ROM-cache contents. */
        if (!gamepak_return_block()) RG_PANIC("Could not restore ROM cache after state operation");
    } else {
        free(buffer);
    }
}

static bool save_state_handler(const char *filename)
{
    gp_display_quiesce();
    size_t size = state_buffer_size();
    bool borrowed;
    void *buffer = state_buffer_get(size, &borrowed);
    if (!buffer) return false;
#if defined(RISCV_ARCH)
    gpsp_save_state_buf(buffer);
#else
    gba_save_state(buffer);
#endif
    bool success = GP_STATE_WRITE(filename, buffer, size);
    state_buffer_put(buffer, borrowed);
    return success;
}

static bool load_state_handler(const char *filename)
{
    gp_display_quiesce();
    const size_t capacity = state_buffer_size();
    rg_stat_t file = rg_storage_stat(filename);
    char backup[320];
    if (!file.is_file) {
        int n = snprintf(backup, sizeof(backup), "%s.bak", filename);
        if (n < 0 || (size_t)n >= sizeof(backup)) return false;
        file = rg_storage_stat(backup);
        filename = backup; /* Wrapper may have stopped between slot renames. */
    }
    if (!file.is_file || file.size != capacity) {
        RG_LOGE("State: missing file or invalid size: %s", filename);
        return false;
    }
    size_t size = capacity;
    bool borrowed;
    void *buffer = state_buffer_get(capacity, &borrowed);
    if (!buffer) return false;
    bool success = rg_storage_read_file(filename, &buffer, &size, RG_FILE_USER_BUFFER) && size == capacity;
    if (success) {
#if defined(RISCV_ARCH)
        success = gpsp_load_state_buf(buffer);
#else
        success = gba_load_state(buffer);
#endif
    }
    state_buffer_put(buffer, borrowed);
    if (success) {
        gpsp_save_mark_dirty();
        gp_save_flush_sync();
    }
    return success;
}
