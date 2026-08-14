#include "Prefs.h"

Prefs ThePrefs;

Prefs::Prefs()
    : NormalCycles(63),
      BadLineCycles(23),
      CIACycles(63),
      FloppyCycles(64),
      SIDType(SIDTYPE_NONE),
      SpriteCollisions(true),
      CIAIRQHack(false),
      Emul1541Proc(false),
      MapSlash(true),
      TestBench(false)
{
}
