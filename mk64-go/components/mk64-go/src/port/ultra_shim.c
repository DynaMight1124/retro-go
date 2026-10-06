/**
 * libultra replacement for the PSP port.
 *
 * The game runs single-threaded: the port's main loop calls the game loop
 * directly, so message queues never block (osRecvMesg returns -1 when empty),
 * DMAs are memcpys (the "ROM" is linked into the executable), the VI/AI/SP
 * calls are no-ops or hooks into the port backends, and EEPROM is a file.
 */
#include <ultra64.h>
#include <PR/os.h>
#include <macros.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "port.h"
#ifdef PORT_NET
#include "net/port_net.h"
#endif

#ifdef TARGET_PSP
#include <pspkernel.h>
#include <psprtc.h>
#else
#include <rg_system.h>
#include <rg_storage.h>
#include <rg_utils.h>
#include <esp_memory_utils.h>
#include "ultra_bridge.h"
#ifdef RETRO_GO
#include "port/rg/menu_timing.h"
#endif
#endif

/* ------------------------------------------------------------------------- */
/* Globals libultra normally provides                                          */
/* ------------------------------------------------------------------------- */

u64 osClockRate = 62500000;
u32 osTvType = 1; // OS_TV_TYPE_NTSC
u32 osResetType = 0; // cold boot
s32 osAppNmiBuffer[16];
OSViMode osViModeTable[32];

/* Referenced by src/os/math (sinf/cosf). */
typedef union {
    int i;
    float f;
} port_fu;
#undef NAN
const port_fu NAN = { 0x7f810000 };

/* ------------------------------------------------------------------------- */
/* Init / threads / timers                                                      */
/* ------------------------------------------------------------------------- */

void osInitialize(void) {
}

void osCreateThread(UNUSED OSThread* thread, UNUSED OSId id, UNUSED void (*entry)(void*), UNUSED void* arg,
                    UNUSED void* sp, UNUSED OSPri pri) {
}
void osStartThread(UNUSED OSThread* thread) {
}
void osDestroyThread(UNUSED OSThread* thread) {
}
void osYieldThread(void) {
}
void osSetThreadPri(UNUSED OSThread* thread, UNUSED OSPri pri) {
}
OSPri osGetThreadPri(UNUSED OSThread* thread) {
    return 0;
}
OSThread* __osGetCurrFaultedThread(void) {
    return NULL;
}

/* osGetTime counts at osClockRate (62.5 MHz); derive it from the system
 * microsecond clock so time deltas stay meaningful. */
static u64 sTimeBase;

static u64 port_clock_us(void) {
#ifdef TARGET_PSP
    return sceKernelGetSystemTimeWide();
#else
    return (u64) rg_system_timer();
#endif
}

OSTime osGetTime(void) {
    u64 now = port_clock_us(); // microseconds
    return (OSTime) ((now - sTimeBase) * 125 / 2);
}

void osSetTime(OSTime time) {
    sTimeBase = port_clock_us() - (u64) time * 2 / 125;
}

u32 osGetCount(void) {
    // 46.875 MHz on the N64; system clock is 1 MHz, scale to keep ratios sane.
#ifdef TARGET_PSP
    return (u32) (port_clock_us() * 46);
#else
    return (u32) (port_clock_us() * 375 / 8);
#endif
}

u32 osSetTimer(UNUSED OSTimer* timer, UNUSED OSTime countdown, UNUSED OSTime interval, UNUSED OSMesgQueue* mq,
               UNUSED OSMesg msg) {
    return 0;
}

void osSetEventMesg(UNUSED OSEvent e, UNUSED OSMesgQueue* mq, UNUSED OSMesg msg) {
}

/* ------------------------------------------------------------------------- */
/* Message queues (never block)                                               */
/* ------------------------------------------------------------------------- */

void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msgBuf, s32 count) {
    mq->validCount = 0;
    mq->first = 0;
    mq->msgCount = count;
    mq->msg = msgBuf;
}

s32 osSendMesg(OSMesgQueue* mq, OSMesg msg, UNUSED s32 flag) {
    s32 index;
    if (mq->validCount >= mq->msgCount) {
        return -1;
    }
    index = (mq->first + mq->validCount) % mq->msgCount;
    mq->msg[index] = msg;
    mq->validCount++;
    return 0;
}

s32 osJamMesg(OSMesgQueue* mq, OSMesg msg, UNUSED s32 flag) {
    if (mq->validCount >= mq->msgCount) {
        return -1;
    }
    mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
    mq->msg[mq->first] = msg;
    mq->validCount++;
    return 0;
}

s32 osRecvMesg(OSMesgQueue* mq, OSMesg* msg, UNUSED s32 flag) {
    if (mq->validCount == 0) {
        return -1;
    }
    if (msg != NULL) {
        *msg = mq->msg[mq->first];
    }
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Cache / memory                                                             */
/* ------------------------------------------------------------------------- */

void osInvalDCache(UNUSED void* a, UNUSED size_t b) {
}
void osInvalICache(UNUSED void* a, UNUSED size_t b) {
}
void osWritebackDCache(UNUSED void* a, UNUSED size_t b) {
}
void osWritebackDCacheAll(void) {
}
uintptr_t osVirtualToPhysical(void* addr) {
    return (uintptr_t) addr;
}

/* ------------------------------------------------------------------------- */
/* "PI" DMA: the cartridge is in memory                                       */
/* ------------------------------------------------------------------------- */

#ifndef TARGET_PSP
static mk64_rom_t *sPortRom;

/* MenuTexture descriptors extracted with sw16 retain segmented texture
 * addresses with their two 16-bit halves exchanged. The USA ROM's 0A/0B
 * texture segment bases are recorded in mk64.ld. */
static bool texture_rom_offset(uintptr_t address, uint32_t *offset) {
    uint32_t segmented = (uint32_t)address;
    unsigned segment = segmented >> 24;
    if (segment != 0x0a && segment != 0x0b && segment != 0x0f) {
        segmented = (segmented << 16) | (segmented >> 16);
        segment = segmented >> 24;
    }
    if (segment != 0x0a && segment != 0x0b && segment != 0x0f)
        return false;
    *offset = (segment == 0x0a ? 0x729a30u :
               segment == 0x0b ? 0x7fa3c0u : 0x641f70u) +
              (segmented & 0x00ffffffu);
    return true;
}

void mk64_ultra_set_rom(mk64_rom_t *rom) {
    sPortRom = rom;
}
#endif

void osCreatePiManager(UNUSED OSPri pri, UNUSED OSMesgQueue* cmdQ, UNUSED OSMesg* cmdBuf, UNUSED s32 cmdMsgCnt) {
}

s32 osPiStartDma(UNUSED OSIoMesg* mb, UNUSED s32 priority, UNUSED s32 direction, uintptr_t devAddr, void* vAddr,
                 size_t nbytes, OSMesgQueue* mq) {
    if (nbytes != 0) {
#ifdef TARGET_PSP
        memcpy(vAddr, (const void*) devAddr, nbytes);
#else
        uint32_t rom_offset;
        /* A halfword-swapped 0x0B000000 descriptor is only 0x00000B00.
         * Recognize texture segments before treating small values as ROM offsets. */
        if (devAddr <= UINTPTR_MAX - (nbytes - 1) &&
            esp_ptr_byte_accessible((const void *)devAddr) &&
            esp_ptr_byte_accessible((const void *)(devAddr + nbytes - 1))) {
            memcpy(vAddr, (const void*) devAddr, nbytes);
        } else if (sPortRom && texture_rom_offset(devAddr, &rom_offset)) {
#if MK64_RG_PROFILE
            static unsigned texture_reads;
            if (texture_reads++ < 16)
                do { RG_LOGD("MK64 texture DMA %p -> ROM %08lx (%u bytes)",
                        (const void *)devAddr, (unsigned long)rom_offset,
                        (unsigned)nbytes); } while (0);
#endif
#ifdef RETRO_GO
            uint32_t read_started = (uint32_t)rg_system_timer();
#endif
            bool read_ok = mk64_rom_read(sPortRom, rom_offset, vAddr, nbytes);
#ifdef RETRO_GO
            mk64_menu_timing_add(MK64_MENU_ROM, (uint32_t)rg_system_timer() - read_started, nbytes);
#endif
            if (!read_ok)
                return -1;
        } else if (sPortRom && devAddr < sPortRom->size) {
#ifdef RETRO_GO
            uint32_t read_started = (uint32_t)rg_system_timer();
#endif
            bool read_ok = mk64_rom_read(sPortRom, (uint32_t) devAddr, vAddr, nbytes);
#ifdef RETRO_GO
            mk64_menu_timing_add(MK64_MENU_ROM, (uint32_t)rg_system_timer() - read_started, nbytes);
#endif
            if (!read_ok)
                return -1;
        } else {
            RG_LOGE("MK64 invalid PI DMA source %p (%u bytes)",
                    (const void *)devAddr, (unsigned)nbytes);
            return -1;
        }
#endif
    }
    if (mq != NULL) {
        osSendMesg(mq, NULL, OS_MESG_NOBLOCK); // completion message
    }
    return 0;
}

s32 osEPiStartDma(UNUSED OSPiHandle* pihandle, OSIoMesg* mb, s32 direction) {
    return osPiStartDma(mb, 0, direction, mb->devAddr, mb->dramAddr, mb->size, mb->hdr.retQueue);
}

OSPiHandle* osCartRomInit(void) {
    static OSPiHandle handle;
    return &handle;
}

/* ------------------------------------------------------------------------- */
/* VI                                                                         */
/* ------------------------------------------------------------------------- */

void osCreateViManager(UNUSED OSPri pri) {
}
void osViSetMode(UNUSED OSViMode* mode) {
}
void osViSetEvent(UNUSED OSMesgQueue* mq, UNUSED OSMesg msg, UNUSED u32 retraceCount) {
}
void osViBlack(UNUSED u8 active) {
}
void osViSetSpecialFeatures(UNUSED u32 func) {
}
void osViSwapBuffer(UNUSED void* vaddr) {
}

/* ------------------------------------------------------------------------- */
/* SP tasks: graphics go through port_gfx_run(), audio through the mixer      */
/* ------------------------------------------------------------------------- */

void osSpTaskLoad(UNUSED OSTask* task) {
}
void osSpTaskStartGo(UNUSED OSTask* task) {
}
void osSpTaskYield(void) {
}
OSYieldResult osSpTaskYielded(UNUSED OSTask* task) {
    return 0;
}

/* ------------------------------------------------------------------------- */
/* AI                                                                         */
/* ------------------------------------------------------------------------- */

extern void port_audio_out_push(const s16* samples, u32 bytes);
extern u32 port_audio_out_queued_bytes(void);

s32 osAiSetFrequency(u32 freq) {
    extern void port_audio_out_set_rate(u32 freq);
    port_audio_out_set_rate(freq); // the output stage resamples to the PSP rate
    return (s32) freq;
}
s32 osAiSetNextBuffer(void* buf, u32 size) {
    port_audio_out_push((const s16*) buf, size);
    return 0;
}
u32 osAiGetLength(void) {
    return port_audio_out_queued_bytes();
}

/* ------------------------------------------------------------------------- */
/* Controllers                                                                */
/* ------------------------------------------------------------------------- */

#ifdef TARGET_PSP
extern void controller_psp_init(void);
extern void controller_psp_read(OSContPad* pad);
#define port_controller_init controller_psp_init
#define port_controller_read controller_psp_read
#else
extern void controller_rg_init(void);
extern void controller_rg_read(OSContPad* pad);
#define port_controller_init controller_rg_init
#define port_controller_read controller_rg_read
#endif

s32 osContInit(UNUSED OSMesgQueue* mq, u8* bitpattern, OSContStatus* status) {
    int i, plugged = 1;
    port_controller_init();
#ifdef PORT_NET
    if (port_net_active()) plugged = port_net_players(); // one pad per machine in the session
#endif
    *bitpattern = (u8) ((1 << plugged) - 1);
    for (i = 0; i < 4; i++) {
        status[i].type = CONT_TYPE_NORMAL;
        status[i].status = 0;
        status[i].errnum = i < plugged ? 0 : CONT_NO_RESPONSE_ERROR;
    }
    return 0;
}

s32 osContStartReadData(OSMesgQueue* mq) {
    osSendMesg(mq, NULL, OS_MESG_NOBLOCK);
    return 0;
}

/* The local pad as the game reads it: the PSP controls, plus the scripted
 * input of debug builds.  In a network session this feeds the local slot. */
void port_local_pad(OSContPad* pad) {
    port_controller_read(pad);
#ifdef PORT_INPUT_SCRIPT
    {
        extern void port_input_script(OSContPad* pad);
        port_input_script(pad);
    }
#endif
}

void osContGetReadData(OSContPad* pad) {
#ifdef PORT_NET
    if (port_net_active()) {
        port_net_pads(pad); // this frame's inputs for every slot, ours included
        return;
    }
#endif
    port_local_pad(&pad[0]);
    pad[1].button = pad[2].button = pad[3].button = 0;
    pad[1].stick_x = pad[2].stick_x = pad[3].stick_x = 0;
    pad[1].stick_y = pad[2].stick_y = pad[3].stick_y = 0;
    pad[1].errno = pad[2].errno = pad[3].errno = CONT_NO_RESPONSE_ERROR;
}

/* ------------------------------------------------------------------------- */
/* EEPROM: a 512 byte file next to the EBOOT                                  */
/* ------------------------------------------------------------------------- */

/* The save is two files, each the whole 512-byte image, written one after
 * the other.  "wb" truncates before it writes, so a power-off, a pulled
 * stick or HOME in the middle of a save leaves one file short -- and the other
 * one whole: the backup still holds the previous save while it is the primary
 * that is torn, and already holds the new one when the primary's turn comes.
 * (No rename: sceIoRename is unreliable across firmwares, and FAT cannot
 * rename over an existing file anyway.) */
#ifdef TARGET_PSP
#define EEPROM_FILE port_save_path("eeprom.bin")
#define EEPROM_BACKUP_FILE port_save_path("eeprom.bak")
#else
static char *sEepromFile, *sEepromBackupFile;
#define EEPROM_FILE sEepromFile
#define EEPROM_BACKUP_FILE sEepromBackupFile
#endif
#define EEPROM_SIZE 512

static u8 sEeprom[EEPROM_SIZE];
static u8 sEepromLoaded;
#ifndef TARGET_PSP
static bool sEepromDirty;
#endif
static s32 eeprom_save(void);

/* A whole image or nothing: a short file is a torn save. */
static int eeprom_read_file(const char* path) {
    u8 image[EEPROM_SIZE];
    size_t n = 0;
    FILE* fp = fopen(path, "rb");
    if (fp != NULL) {
        n = fread(image, 1, sizeof(image), fp);
        if (n == sizeof(image) && fgetc(fp) != EOF) n = 0;
        fclose(fp);
    }
    if (n != sizeof(image)) {
        return 0;
    }
    memcpy(sEeprom, image, sizeof(sEeprom));
    return 1;
}

static s32 eeprom_write_file(const char* path) {
    FILE* fp = fopen(path, "wb");
    size_t n;
    if (fp == NULL) {
        return -1;
    }
    n = fwrite(sEeprom, 1, sizeof(sEeprom), fp);
    if (fclose(fp) != 0 || n != sizeof(sEeprom)) {
        return -1;
    }
    return 0;
}

static void eeprom_load(void) {
    if (sEepromLoaded) {
        return;
    }
    sEepromLoaded = 1;
#ifndef TARGET_PSP
    if (!sEepromFile) {
        sEepromFile = rg_emu_get_path(RG_PATH_SAVE_SRAM, rg_system_get_app()->romPath);
        sEepromBackupFile = malloc(strlen(sEepromFile) + 5);
        if (!sEepromBackupFile) RG_PANIC("MK64 save path allocation failed");
        sprintf(sEepromBackupFile, "%s.bak", sEepromFile);
        rg_storage_mkdir(rg_dirname(sEepromFile));
    }
#endif
    if (eeprom_read_file(EEPROM_FILE)) {
        return;
    }
    if (eeprom_read_file(EEPROM_BACKUP_FILE)) {
#ifdef TARGET_PSP
        PORT_LOG("save: eeprom.bin missing or short, restored from eeprom.bak\n");
#else
        RG_LOGW("save: eeprom.bin missing or short, restored from eeprom.bak\n");
#endif
#ifdef TARGET_PSP
        eeprom_write_file(EEPROM_FILE); /* make the pair whole again */
#else
        sEepromDirty = eeprom_write_file(EEPROM_FILE) != 0;
        rg_storage_commit();
#endif
        return;
    }
#ifndef TARGET_PSP
    /* Import the previous shared path only if this ROM has no save files.
     * Leave the originals intact; a damaged new pair must not roll back to
     * a stale legacy image. */
    if (!rg_storage_exists(EEPROM_FILE) && !rg_storage_exists(EEPROM_BACKUP_FILE) &&
        (eeprom_read_file(RG_BASE_PATH_SAVES "/mk64/eeprom.bin") ||
         eeprom_read_file(RG_BASE_PATH_SAVES "/mk64/eeprom.bak"))) {
        sEepromDirty = true;
        eeprom_save();
        PORT_LOG("save: migrated native EEPROM to %s\n", EEPROM_FILE);
        return;
    }
#endif
    memset(sEeprom, 0, sizeof(sEeprom)); /* no save yet: a blank EEPROM */
}

static s32 eeprom_save(void) {
    /* The backup first: see the note at EEPROM_FILE.  A save the stick has no
     * room for is reported to the game instead of passing for written. */
    s32 backup = eeprom_write_file(EEPROM_BACKUP_FILE);
    s32 primary = eeprom_write_file(EEPROM_FILE);
    if (backup != 0 || primary != 0) {
#ifdef TARGET_PSP
        PORT_LOG("save: write failed (eeprom.bak %d, eeprom.bin %d)\n", (int) backup, (int) primary);
#else
        RG_LOGE("save: write failed (eeprom.bak %d, eeprom.bin %d)\n", (int) backup, (int) primary);
#endif
    }
#ifndef TARGET_PSP
    sEepromDirty = primary != 0 || backup != 0;
    rg_storage_commit();
    return sEepromDirty ? -1 : 0;
#else
    return primary;
#endif
}
#ifndef TARGET_PSP
bool mk64_eeprom_flush(void) {
    return !sEepromLoaded || !sEepromDirty || eeprom_save() == 0;
}
#endif

s32 osEepromProbe(UNUSED OSMesgQueue* mq) {
    return EEPROM_TYPE_4K;
}

s32 osEepromLongRead(UNUSED OSMesgQueue* mq, u8 address, u8* buffer, int nbytes) {
    eeprom_load();
    if (!buffer || nbytes < 0 || nbytes > EEPROM_SIZE - address * 8) {
        return -1;
    }
    memcpy(buffer, sEeprom + address * 8, nbytes);
    return 0;
}

s32 osEepromLongWrite(UNUSED OSMesgQueue* mq, u8 address, u8* buffer, int nbytes) {
    eeprom_load();
    if (!buffer || nbytes < 0 || nbytes > EEPROM_SIZE - address * 8) {
        return -1;
    }
    memcpy(sEeprom + address * 8, buffer, nbytes);
#ifndef TARGET_PSP
    sEepromDirty = true;
#endif
    return eeprom_save();
}

s32 osEepromRead(OSMesgQueue* mq, u8 address, u8* buffer) {
    return osEepromLongRead(mq, address, buffer, 8);
}

s32 osEepromWrite(OSMesgQueue* mq, u8 address, u8* buffer) {
    return osEepromLongWrite(mq, address, buffer, 8);
}

/* ------------------------------------------------------------------------- */
/* Controller Pak (ghost data): not present                                   */
/* ------------------------------------------------------------------------- */

s32 osPfsInit(UNUSED OSMesgQueue* mq, UNUSED OSPfs* pfs, UNUSED int channel) {
    return PFS_ERR_NOPACK;
}
s32 osPfsIsPlug(UNUSED OSMesgQueue* mq, u8* pattern) {
    *pattern = 0;
    return 0;
}
s32 osPfsFindFile(UNUSED OSPfs* pfs, UNUSED u16 company, UNUSED u32 game, UNUSED u8* gameName, UNUSED u8* extName,
                  UNUSED s32* fileNo) {
    return PFS_ERR_NOPACK;
}
s32 osPfsAllocateFile(UNUSED OSPfs* pfs, UNUSED u16 company, UNUSED u32 game, UNUSED u8* gameName,
                      UNUSED u8* extName, UNUSED int length, UNUSED s32* fileNo) {
    return PFS_ERR_NOPACK;
}
s32 osPfsDeleteFile(UNUSED OSPfs* pfs, UNUSED u16 company, UNUSED u32 game, UNUSED u8* gameName,
                    UNUSED u8* extName) {
    return PFS_ERR_NOPACK;
}
s32 osPfsReadWriteFile(UNUSED OSPfs* pfs, UNUSED s32 fileNo, UNUSED u8 flag, UNUSED int offset, UNUSED int nbytes,
                       UNUSED u8* data) {
    return PFS_ERR_NOPACK;
}
s32 osPfsFileState(UNUSED OSPfs* pfs, UNUSED s32 fileNo, UNUSED OSPfsState* state) {
    return PFS_ERR_NOPACK;
}
s32 osPfsFreeBlocks(UNUSED OSPfs* pfs, s32* bytes) {
    *bytes = 0;
    return PFS_ERR_NOPACK;
}
s32 osPfsNumFiles(UNUSED OSPfs* pfs, s32* max, s32* used) {
    *max = 0;
    *used = 0;
    return PFS_ERR_NOPACK;
}

/* ------------------------------------------------------------------------- */
/* Debug output                                                               */
/* ------------------------------------------------------------------------- */

void rmonPrintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void osSyncPrintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
