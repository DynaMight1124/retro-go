#ifndef Q2_SAVE_BUILD_H
#define Q2_SAVE_BUILD_H
#include <stdio.h>
#include "q_shared.h"

// Native saves use stable symbol IDs and a manually maintained schema version.
qboolean Q2_SaveWriteHeader(FILE *file);
qboolean Q2_SaveReadHeader(FILE *file);
qboolean Q2_SaveCheckBuild(const char *path);
void QG_SaveNotice(const char *message);
#endif
