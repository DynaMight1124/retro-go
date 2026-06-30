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
/*
	generate the tms9995 emulator
*/

#include "tms9900.h"

#define TMS99XX_MODEL TMS9995_ID

#include "99xxcore.h"

#ifdef __cplusplus
}
#endif
