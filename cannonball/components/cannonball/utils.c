/***************************************************************************
    General C++ Helper Functions

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "utils.h"
#include <stdio.h>
#include "setup.h"
#include "engine/outrun.h"

char stringConvert[128];

// Convert value to string
const char* Utils_int_to_string(int i)
{
    sprintf(stringConvert, "%d", i);
    return stringConvert;
}

// Convert value to string
const char* Utils_char_to_string(char c)
{
    sprintf(stringConvert, "%c", c);
    return stringConvert;
}

// Convert value to hex string
const char* Utils_int_to_hex_string(int i)
{
    sprintf(stringConvert, "%x", i);
    return stringConvert;
}

// Convert hex string to unsigned int
uint32_t Utils_from_hex_string(const char* string)
{
    uint32_t x;
    sscanf(string, "%lx", &x);
    return x;
   
}

const char* getScoresFilename()
{
    if (Outrun_cannonball_mode == OUTRUN_MODE_ORIGINAL)
    {
        return !Config_engine.jap ? FILENAME_SCORES : FILENAME_SCORES_JAPAN;
    }

    return !Config_engine.jap ?  FILENAME_CONT : FILENAME_CONT_JAPAN;
}
