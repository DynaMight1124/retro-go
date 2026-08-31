/* gpSP ESP32-P4 / ESP32-S31 platform glue */
#include "gpsp_esp.h"
#include "common.h"
#include "cpu.h"
#include "gba_memory.h"
#include "gpsp_config.h"
#include "main.h"
#include "sound.h"
#include "video.h"
#include "input.h"
#include "savestate.h"

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_log.h"
#if defined(HAVE_DYNAREC) && defined(MMAP_JIT_CACHE)
#include "esp_mmu_map.h"
#include "hal/mmu_types.h"
#include "esp_cache.h"
#endif
static const char *TAG = "gpsp";
#endif

/* Variables normally provided by libretro.c */
#ifdef HAVE_DYNAREC
int dynarec_enable = 1;
#else
int dynarec_enable = 0;
#endif
boot_mode selected_boot_mode = boot_game;
int sprite_limit = 1;
u32 idle_loop_target_pc = 0xFFFFFFFF;
u32 translation_gate_target_pc[MAX_TRANSLATION_GATES];
u32 translation_gate_targets = 0;
u32 skip_next_frame = 0;
u32 num_skipped_frames = 0;
u32 netplay_num_clients = 0;
u32 netplay_client_id = 0;

/* Button state set by the host */
static uint16_t gpsp_buttons = 0;

void gpsp_set_buttons(uint16_t buttons)
{
    gpsp_buttons = buttons;
}

void gpsp_set_framebuffer(uint16_t *fb)
{
    gba_screen_pixels = (u16 *)fb;
}

size_t gpsp_state_size(void)
{
    return GBA_STATE_MEM_SIZE;
}

/* Override update_input to use our button state directly */
u32 update_input(void)
{
    /* GBA P1 register is active-low */
    write_ioreg(REG_P1, (~gpsp_buttons) & 0x3FF);

    /* Check for key interrupt */
    u16 p1cnt = read_ioreg(REG_P1CNT);
    if (p1cnt & 0x4000) {
        u16 key_mask = p1cnt & 0x3FF;
        u16 pressed = gpsp_buttons & key_mask;
        if (p1cnt & 0x8000) {
            /* AND mode: all specified keys must be pressed */
            if (pressed == key_mask)
                flag_interrupt(IRQ_KEYPAD);
        } else {
            /* OR mode: any specified key */
            if (pressed)
                flag_interrupt(IRQ_KEYPAD);
        }
    }
    return 0;
}

bool gpsp_init(void)
{
#ifdef HAVE_DYNAREC
    /* Verify register file layout matches assembly stub offsets */
    {
        extern u32 spsr[];
        extern u32 reg_mode[][7];
        ptrdiff_t spsr_off = (u8*)spsr - (u8*)reg;
        ptrdiff_t regmode_off = (u8*)reg_mode - (u8*)reg;
        ESP_LOGI(TAG, "Layout: spsr=+0x%x reg_mode=+0x%x", (unsigned)spsr_off, (unsigned)regmode_off);
        assert(spsr_off == 0x100 && regmode_off == 0x118);
    }
#endif
#if defined(HAVE_DYNAREC) && defined(MMAP_JIT_CACHE)
    /* Allocate JIT translation caches in executable PSRAM via MMU map.
     * heap_caps_malloc gives data-only PSRAM; JIT code needs execute permission.
     * esp_mmu_map with MMU_TARGET_PSRAM0 + MMU_MEM_CAP_EXEC maps PSRAM pages
     * with instruction bus enabled. */
    extern u8 *rom_translation_cache;
    extern u8 *ram_translation_cache;
    {
        /* Total JIT size, rounded up to MMU page boundary */
        size_t total = ROM_TRANSLATION_CACHE_SIZE + RAM_TRANSLATION_CACHE_SIZE;
        size_t page_size = CONFIG_MMU_PAGE_SIZE;
        total = (total + page_size - 1) & ~(page_size - 1);

        /* Allocate JIT buffer from PSRAM heap, page-aligned for MMU mapping */
        u8 *jit_data = (u8 *)heap_caps_aligned_alloc(page_size, total, MALLOC_CAP_SPIRAM);
        if (!jit_data) {
            ESP_LOGE(TAG, "Failed to allocate JIT buffer (%u bytes)", (unsigned)total);
            return false;
        }

        /* Get physical address of the heap allocation */
        esp_paddr_t paddr = 0;
        mmu_target_t paddr_target = 0;
        esp_err_t err = esp_mmu_vaddr_to_paddr(jit_data, &paddr, &paddr_target);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_mmu_vaddr_to_paddr failed: %s", esp_err_to_name(err));
            free(jit_data);
            return false;
        }
        ESP_LOGI(TAG, "JIT data ptr=%p, paddr=0x%08x, size=%u",
                 jit_data, (unsigned)paddr, (unsigned)total);

        /* Create executable mapping of the same physical PSRAM pages.
         * Request only MMU_MEM_CAP_EXEC (caps check rejects EXEC+WRITE combo).
         * The PSRAM region has IBUS1+DBUS1, so the mapping is actually R/W/X. */
        void *jit_base = NULL;
        err = esp_mmu_map(paddr, total, MMU_TARGET_PSRAM0,
            MMU_MEM_CAP_EXEC,
            ESP_MMU_MMAP_FLAG_PADDR_SHARED, &jit_base);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_mmu_map for JIT failed (%d bytes): %s",
                     (int)total, esp_err_to_name(err));
            free(jit_data);
            return false;
        }
        ESP_LOGI(TAG, "JIT exec mapping at %p (data at %p, %u bytes)",
                 jit_base, jit_data, (unsigned)total);

        /* Exec mapping has IBUS1+DBUS1, so it's readable, writable, and executable.
         * All JIT code is written and executed through this pointer. */
        rom_translation_cache = (u8 *)jit_base;
        ram_translation_cache = (u8 *)jit_base + ROM_TRANSLATION_CACHE_SIZE;
    }
#endif

    init_gamepak_buffer();
    init_sound();

    if (!gba_screen_pixels) {
#ifdef ESP_PLATFORM
        /* Cache-line aligned for PPA DMA access */
        gba_screen_pixels = (uint16_t *)heap_caps_aligned_alloc(
            64, GBA_SCREEN_BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        if (!gba_screen_pixels)
            gba_screen_pixels = (uint16_t *)heap_caps_aligned_alloc(
                64, GBA_SCREEN_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
#else
        gba_screen_pixels = (uint16_t *)malloc(GBA_SCREEN_BUFFER_SIZE);
#endif
    }

    if (!gba_screen_pixels) {
#ifdef ESP_PLATFORM
        ESP_LOGE(TAG, "Failed to allocate framebuffer");
#endif
        return false;
    }

    return true;
}

int gpsp_load_rom(const char *rom_path)
{
    /* Use built-in BIOS */
    extern u8 open_gba_bios_rom[];
    extern u8 bios_rom[];
    memcpy(bios_rom, open_gba_bios_rom, 16 * 1024);

    memset(gamepak_backup, 0xFF, sizeof(gamepak_backup));

    ESP_LOGI(TAG, "Loading gamepak: %s", rom_path);
    if (load_gamepak(NULL, rom_path, FEAT_AUTODETECT, FEAT_DISABLE, SERIAL_MODE_AUTO) != 0) {
#ifdef ESP_PLATFORM
        ESP_LOGE(TAG, "Failed to load ROM: %s", rom_path);
#endif
        return -1;
    }
    ESP_LOGI(TAG, "Gamepak loaded, calling reset_gba()");

    ESP_LOGI(TAG, "selected_boot_mode=%d (boot_game=%d)", selected_boot_mode, boot_game);
    reset_gba();
    ESP_LOGI(TAG, "reset_gba() done, dynarec=%d PC=0x%08x CPSR=0x%08x mode=%u",
             dynarec_enable, reg[REG_PC], reg[REG_CPSR], reg[CPU_MODE]);
    return 0;
}

uint16_t *gpsp_run_frame(void)
{
    /* update_input writes button state to IO register */
    update_input();

    /* execute_arm runs until frame completion (calls update_gba internally,
       returns when update_gba sets frame_complete flag) */
    clear_gamepak_stickybits();
#ifdef HAVE_DYNAREC
    execute_arm_translate(execute_cycles);
#else
    execute_arm(execute_cycles);
#endif

    return gba_screen_pixels;
}

unsigned gpsp_get_audio(int16_t *out, unsigned max_frames)
{
    return sound_read_samples(out, max_frames);
}

int gpsp_load_save(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fread(gamepak_backup, 1, sizeof(gamepak_backup), f);
    fclose(f);
    return 0;
}

int gpsp_write_save(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    /* Determine save size from backup type */
    u32 size = sizeof(gamepak_backup);
    fwrite(gamepak_backup, 1, size, f);
    fclose(f);
    return 0;
}

void gpsp_save_state_buf(void *buf)
{
    gba_save_state(buf);
}

bool gpsp_load_state_buf(const void *buf)
{
    return gba_load_state(buf);
}

void gpsp_reset(void)
{
    reset_gba();
#ifdef HAVE_DYNAREC
    if (dynarec_enable)
        flush_dynarec_caches();
#endif
}

void gpsp_deinit(void)
{
    memory_term();
    if (gba_screen_pixels) {
        free(gba_screen_pixels);
        gba_screen_pixels = NULL;
    }
}
