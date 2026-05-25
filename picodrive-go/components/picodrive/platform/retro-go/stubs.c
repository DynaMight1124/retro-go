#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <pico/pico_int.h>

#ifdef ESP_PLATFORM
#include <esp_attr.h>
#else
#define EXT_RAM_BSS_ATTR
#endif

// Sega CD stubs
#ifdef NO_CD
unsigned int pcd_event_times[4];
mcd_state *Pico_mcd;
void *Pico_msd;
u32 pcd_base_address;
void PicoMemSetupCD(void) {}
void PicoInitMCD(void) {}
int  PicoResetMCD(void) { return 0; }
void PicoPowerMCD(void) {}
void PicoFrameMCD(void) {}
void PicoCreateMCD(unsigned char *bios_data, int bios_size) {}
void pcd_pcm_update(s32 *buffer, int length, int stereo) {}
int cdd_load(const char *filename, int type) { return 0; }
int cdd_unload(void) { return 0; }
void PicoMCDPrepare(void) {}

u32 PicoRead8_mcd_io(u32 a) { return 0; }
u32 PicoRead16_mcd_io(u32 a) { return 0; }
void PicoWrite8_mcd_io(u32 a, u32 d) {}
void PicoWrite16_mcd_io(u32 a, u32 d) {}

// void s68k_read8_map(void) {}
// void s68k_read16_map(void) {}
void DmaSlowCell(u32 source, u32 a, int len, unsigned char inc) {}

// CD parse stubs
void *cue_parse(const char *fname) { return NULL; }
void *chd_parse(const char *fname) { return NULL; }
void cdparse_destroy(void *data) {}
#endif

// CD Audio format stubs (Always stubbed for now as we don't have the libs)
void mp3_update(s32 *buffer, int length, int stereo) {}
void ogg_update(s32 *buffer, int length, int stereo) {}
void mp3_start_play(void *f, int pos) {}
void ogg_start_play(void *f, int pos) {}
void ogg_stop_play(void) {}
int mp3_get_bitrate(void *f, int size) { return 0; }
int ogg_get_length(void *f) { return 0; }

// 32X stubs
#ifndef p32x_pwm_update
void p32x_pwm_update(s32 *buf32, int length, int stereo) {}
#endif
#ifndef PicoPower32x
void PicoPower32x(void) {}
#endif
#ifndef PicoReset32x
void PicoReset32x(void) {}
#endif
#ifndef PicoFrame32x
void PicoFrame32x(void) {}
#endif
#ifndef Pico32xInit
void Pico32xInit(void) {}
#endif
#ifndef PicoUnload32x
void PicoUnload32x(void) {}
#endif

unsigned int p32x_event_times[4];
void PicoDrawSetOutBuf32X(void *dest, int increment) {}
void PicoDrawSetOutFormat32x(pdso_t which, int use_32x_line_mode) {}
u32  PicoRead8_32x(u32 a) { return 0; }
u32  PicoRead16_32x(u32 a) { return 0; }
void PicoWrite8_32x(u32 a, u32 d) {}
void PicoWrite16_32x(u32 a, u32 d) {}
void Pico32xPrepare(void) {}
void PicoPrepareMS(void) {}

// Missing scanline pointers
int (*PicoScan32xBegin)(unsigned int num) = NULL;
int (*PicoScan32xEnd)(unsigned int num) = NULL;

// Sega Pico educational console stubs
void PicoInitPico(void) {}
void PicoMemSetupPico(void) {}
void PicoReratePico(void) {}
int PicoPicoIrqAck(int level) { return 0; }
void PicoPicoPCMUpdate(short *buffer, int length, int stereo) {}

// VGM stubs
void vgm_reset(void) {}
void vgm_finish(void) {}
void vgm_load(void) {}
void vgm_frame(void) {}

// Sega Pico stubs for state.c
picohw_state PicoPicohw;
int PicoPicoPCMSave(void *buffer, int length) { return 0; }
void PicoPicoPCMLoad(void *buffer, int length) {}

// YM2413 stubs for state.c
size_t ym2413_pack_state(void *buf_, size_t size) { return 0; }
void ym2413_unpack_state(const void *buf_, size_t size) {}

// Sega CD stubs for state.c (needed when NO_CD is defined)
#ifdef NO_CD
int  cdd_context_save(unsigned char *dest) { return 0; }
int  cdd_context_load(unsigned char *src) { return 0; }
int  cdd_context_load_old(unsigned char *src) { return 0; }
int  cdc_context_save(unsigned char *dest) { return 0; }
int  cdc_context_load(unsigned char *src) { return 0; }
int  cdc_context_load_old(unsigned char *src) { return 0; }
int  gfx_context_save(unsigned char *dest) { return 0; }
int  gfx_context_load(const unsigned char *src) { return 0; }
void wram_2M_to_1M(unsigned char *data) {}
void wram_1M_to_2M(unsigned char *data) {}
#endif

// CPU contexts
extern M68K_CONTEXT PicoCpuFM68k;
#if defined(NO_CD) && !defined(EMU_F68K)
M68K_CONTEXT PicoCpuFS68k;
#endif

// YM2413 stubs
void *opll = NULL;
int16_t OPLL_calc(void *opll) { return 0; }
void *OPLL_new(void) { return NULL; }
void OPLL_setChipType(void) {}
void OPLL_reset(void *opll) {}
void OPLL_setRate(void *opll, int rate) {}

// Gz stubs
void *gzopen(const char *path, const char *mode) { return NULL; }
int gzwrite(void *file, const void *buf, unsigned int len) { return 0; }
int gzread(void *file, void *buf, unsigned int len) { return 0; }
int gzclose(void *file) { return 0; }
int gzseek(void *file, long offset, int whence) { return 0; }
int gzeof(void *file) { return 1; }
int gzsetparams(void *file, int level, int strategy) { return 0; }
