#ifndef RG_MAME_CORE_H
#define RG_MAME_CORE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "retrogo/rg_psram.h"
#include "driver.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************

sndhrdw\jedi.c

***************************************************************************/

#include "driver.h"

/* Misc sound code */

WRITE_HANDLER( jedi_speech_w )
{
    static unsigned char speech_write_buffer;

    if(offset<0xff)
    {
        speech_write_buffer = data;
    }
    else if (offset<0x1ff)
    {
        tms5220_data_w(0,speech_write_buffer);
    }
}

READ_HANDLER( jedi_speech_ready_r )
{
    return (!tms5220_ready_r())<<7;
}

#ifdef __cplusplus
}
#endif
