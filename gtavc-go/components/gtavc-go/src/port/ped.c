#include "ped.h"
#include <string.h>

void Ped_init(Ped* p) {
    if (!p) return;
    memset(p, 0, sizeof(Ped));
    p->hp = 100;
    p->clip = -1;
    p->clipNext = -1;
}
