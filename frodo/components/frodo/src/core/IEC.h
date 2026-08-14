#pragma once

#include <cstdint>
#include <string>

enum {
    FILE_DISK_IMAGE,
    FILE_GCR_IMAGE,
    FILE_TAPE_IMAGE,
    FILE_ARCH,
};

// 1541 DOS error indices. The GCR drive uses these internally while
// converting D64 error-byte information into controller behaviour.
enum {
    ERR_OK,
    ERR_SCRATCHED,
    ERR_UNIMPLEMENTED,
    ERR_READ20,
    ERR_READ21,
    ERR_READ22,
    ERR_READ23,
    ERR_READ24,
    ERR_WRITE25,
    ERR_WRITEPROTECT,
    ERR_READ27,
    ERR_WRITE28,
    ERR_DISKID,
    ERR_SYNTAX30,
    ERR_SYNTAX31,
    ERR_SYNTAX32,
    ERR_SYNTAX33,
    ERR_SYNTAX34,
    ERR_WRITEFILEOPEN,
    ERR_FILENOTOPEN,
    ERR_FILENOTFOUND,
    ERR_FILEEXISTS,
    ERR_FILETYPE,
    ERR_NOBLOCK,
    ERR_ILLEGALTS,
    ERR_NOCHANNEL,
    ERR_DIRERROR,
    ERR_DISKFULL,
    ERR_STARTUP,
    ERR_NOTREADY,
};

enum {
    DRVLED_OFF,
    DRVLED_ON,
    DRVLED_ERROR_OFF,
    DRVLED_ERROR_ON,
    DRVLED_ERROR_FLASH,
};

class C64;
class Prefs;

class IEC {
public:
    explicit IEC(C64 *c64);
    void Reset();
    void NewPrefs(const Prefs *prefs);

    uint8_t Out(uint8_t byte, bool eoi);
    uint8_t OutATN(uint8_t byte);
    uint8_t OutSec(uint8_t byte);
    uint8_t In(uint8_t &byte);
    void SetATN();
    void RelATN();
    void Turnaround();
    void Release();

private:
    C64 *the_c64;
};

bool IsMountableFile(const std::string &path, int &type);
