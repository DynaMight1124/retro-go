#pragma once

#include <string>

enum {
    SIDTYPE_NONE,
    SIDTYPE_DIGITAL_6581,
    SIDTYPE_DIGITAL_8580,
    SIDTYPE_SIDCARD,
};

class Prefs {
public:
    Prefs();

    int NormalCycles;
    int BadLineCycles;
    int CIACycles;
    int FloppyCycles;
    int SIDType;

    bool SpriteCollisions;
    bool CIAIRQHack;
    bool Emul1541Proc;
    bool MapSlash;
    bool TestBench;

    std::string DrivePath[4];
    std::string TapePath;
};

extern Prefs ThePrefs;
