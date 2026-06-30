#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "retrogo/rg_psram.h"
#include "rg_memory.h"
#include "rg_system.h"

extern "C" {

void *mame_alloc_fast(size_t size)
{
    return rg_alloc(size, MEM_FAST);
}

}
