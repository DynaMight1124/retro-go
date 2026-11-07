#ifndef VGA_H
#define VGA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "t386_fill.h"

typedef struct FBDevice FBDevice;

typedef void SimpleFBDrawFunc(void *opaque,
                              int x, int y, int w, int h);

typedef struct VGAState VGAState;
VGAState *vga_init(char *vga_ram, int vga_ram_size,
                   uint8_t *fb, int width, int height);
void vga_set_force_8dm(VGAState *s, int v);
void vga_get_viewport(VGAState *s,int *x,int *y,int *w,int *h);
int vga_get_render_stride(VGAState *s);
/* Caller-owned optional native320x200 RGB565 buffer. Configure before refresh;
 * fallback modes always retain the full-size buffer passed to vga_init. */
bool vga_set_native_buffer(VGAState *s,uint8_t *buffer,size_t size);
const uint8_t *vga_get_render_data(VGAState *s);
/* Optional caller-owned native indices/palette. Neither may be queued to a
 * display directly. Configure before refresh; unsupported modes remain RGB565.
 * Palette has256 RGB565 host-order entries, separate from live VGA DAC data. */
bool vga_set_native_indexed_buffer(VGAState *s,uint8_t *buffer,size_t size,
                                   uint16_t *palette,size_t entries);
const uint16_t *vga_get_render_palette(VGAState *s);

#if defined(RETRO_GO) && defined(ESP_PLATFORM)
enum {T386_VGA_BLANK,T386_VGA_TEXT,T386_VGA_NATIVE_PLANAR,T386_VGA_NATIVE_INDEXED,
      T386_VGA_FALLBACK,T386_VGA_RENDER_KINDS};
/* Last refresh, single-VM diagnostic only. Native palette/draw exclude redraw;
 * fallback draw includes its existing palette/callback work. Nested in VGA time. */
typedef struct {uint64_t clear_us,palette_us,draw_us; unsigned kind,clears;} T386VGARenderProfile;
const T386VGARenderProfile *vga_render_profile_get(void);
#endif

int vga_step(VGAState *vga);
void vga_refresh(VGAState *s,
                 SimpleFBDrawFunc *redraw_func, void *opaque, int full_update);

void vga_ioport_write(VGAState *s, uint32_t addr, uint32_t val);
uint32_t vga_ioport_read(VGAState *s, uint32_t addr);

void vbe_write(VGAState *s, uint32_t offset, uint32_t val);
uint32_t vbe_read(VGAState *s, uint32_t offset);

void vga_mem_write(VGAState *s, uint32_t addr, uint8_t val);
uint8_t vga_mem_read(VGAState *s, uint32_t addr);
void vga_mem_write16(VGAState *s, uint32_t addr, uint16_t val);
void vga_mem_write32(VGAState *s, uint32_t addr, uint32_t val);
bool vga_mem_write_string(VGAState *s, uint32_t addr, uint8_t *buf, int len);
/* Optional planar fill of zero CPU words (modes 1/3 repeat latch data), or
 * character/attribute word fill in eligible odd/even text mappings.
 * False leaves guest/VGA state untouched;
 * a non-NULL reject receives the current eligibility reasons and registers. */
bool vga_mem_fill16(VGAState *s,uint32_t addr,uint16_t value,unsigned count,T386FillReject *reject);

typedef struct PCIDevice PCIDevice;
typedef struct PCIBus PCIBus;
PCIDevice *vga_pci_init(VGAState *s, PCIBus *bus,
                        void *o, void (*set_bar)(void *, int, uint32_t, bool));

#ifndef BPP
#define BPP 32
#endif

typedef struct VGATextOps {
    void (*set_cursor)(void *o, int y, int x);
    void (*put_char2)(void *o, int y, int x, int ch, int attr);
    void (*refresh)(void *o, const uint8_t *ram, int w, int h);
} VGATextOps;

void vga_set_text_ops(VGAState *s, VGATextOps *ops, void *o);

#endif /* VGA_H */
