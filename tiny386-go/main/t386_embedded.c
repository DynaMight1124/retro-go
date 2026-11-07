#include "t386_firmware.h"
extern const unsigned char bios_start[] asm("_binary_bios_bin_start");
extern const unsigned char bios_end[] asm("_binary_bios_bin_end");
extern const unsigned char vga_start[] asm("_binary_vgabios_bin_start");
extern const unsigned char vga_end[] asm("_binary_vgabios_bin_end");
const unsigned char *t386_embedded_bios(size_t *size)
{
    *size=(size_t)(bios_end-bios_start);
    return bios_start;
}
const unsigned char *t386_embedded_vga(size_t *size)
{
    *size=(size_t)(vga_end-vga_start);
    return vga_start;
}
