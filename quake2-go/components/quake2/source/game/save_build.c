#include "save_build.h"
#include <string.h>

extern unsigned QG_SaveVersion(void);
static const unsigned char magic[8] = {'Q', '2', 'R', 'G', 'S', 'V', '0', '2'};

qboolean Q2_SaveWriteHeader(FILE *file)
{
    unsigned version = QG_SaveVersion();
    unsigned char bytes[4] = {(unsigned char)version, (unsigned char)(version >> 8),
                              (unsigned char)(version >> 16), (unsigned char)(version >> 24)};
    return fwrite(magic, sizeof(magic), 1, file) == 1 &&
           fwrite(bytes, sizeof(bytes), 1, file) == 1;
}

qboolean Q2_SaveReadHeader(FILE *file)
{
    unsigned char saved_magic[8], bytes[4];
    if (fread(saved_magic, sizeof(saved_magic), 1, file) != 1 ||
        memcmp(saved_magic, magic, sizeof(magic)) ||
        fread(bytes, sizeof(bytes), 1, file) != 1) return false;
    unsigned version = (unsigned)bytes[0] | ((unsigned)bytes[1] << 8) |
                       ((unsigned)bytes[2] << 16) | ((unsigned)bytes[3] << 24);
    return version == QG_SaveVersion();
}

qboolean Q2_SaveCheckBuild(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    qboolean compatible = Q2_SaveReadHeader(file);
    fclose(file);
    return compatible;
}
