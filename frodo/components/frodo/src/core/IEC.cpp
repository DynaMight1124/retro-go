#include "IEC.h"

#include "Prefs.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

IEC::IEC(C64 *c64) : the_c64(c64)
{
}

void IEC::Reset() {}
void IEC::NewPrefs(const Prefs *) {}

uint8_t IEC::Out(uint8_t, bool) { return 0x80; }
uint8_t IEC::OutATN(uint8_t) { return 0x80; }
uint8_t IEC::OutSec(uint8_t) { return 0x80; }
uint8_t IEC::In(uint8_t &byte) { byte = 0; return 0x80; }
void IEC::SetATN() {}
void IEC::RelATN() {}
void IEC::Turnaround() {}
void IEC::Release() {}

bool IsMountableFile(const std::string &path, int &type)
{
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return false;
    }

    std::string extension = path.substr(dot + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (extension == "d64" || extension == "x64") {
        type = FILE_DISK_IMAGE;
        return true;
    }
    if (extension == "g64") {
        type = FILE_GCR_IMAGE;
        return true;
    }
    return false;
}
