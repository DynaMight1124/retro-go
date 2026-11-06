#pragma once
#include <string.h>
#include "gpsp_log.h"

static inline void gpsp_log_rom_info(const unsigned char *header)
{
    char title[13], code[5], maker[3];
    memcpy(title, header + 0xA0, 12); title[12] = '\0';
    memcpy(code, header + 0xAC, 4); code[4] = '\0';
    memcpy(maker, header + 0xB0, 2); maker[2] = '\0';
    RG_LOGI("gamepak title: %s", title);
    RG_LOGI("gamepak code : %s", code);
    RG_LOGI("gamepak maker: %s", maker);
}
