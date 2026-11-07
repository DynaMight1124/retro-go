#include "t386_firmware.h"
#include "t386_file.h"
#include <limits.h>
#include <string.h>

bool t386_firmware_load(void *memory,size_t capacity,const char *path,
                       uint32_t address,bool backward,int *loaded)
{
    *loaded=0;
    if(!path) return false;
    size_t size;
    const unsigned char *blob;
    if(!strcmp(path,T386_BIOS_BUILTIN)) blob=t386_embedded_bios(&size);
    else if(!strcmp(path,T386_VGA_BUILTIN)) blob=t386_embedded_vga(&size);
    else return t386_file_load_rom(memory,capacity,path,address,backward,loaded);

    size_t start=address;
    if(!blob || !size || size>INT_MAX || (backward && size>start)) return false;
    if(backward) start-=size;
    if(start>capacity || size>capacity-start) return false;
    /* Flash-mapped const bytes copy directly into already allocated guest RAM. */
    memcpy((unsigned char*)memory+start,blob,size);
    *loaded=(int)size;
    return true;
}
