#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define T386_BIOS_BUILTIN "@tiny386/bios"
#define T386_VGA_BUILTIN "@tiny386/vgabios"
const unsigned char *t386_embedded_bios(size_t *size);
const unsigned char *t386_embedded_vga(size_t *size);
bool t386_firmware_load(void *memory,size_t capacity,const char *path,
                       uint32_t address,bool backward,int *loaded);
