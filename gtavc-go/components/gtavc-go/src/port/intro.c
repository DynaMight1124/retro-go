#include "intro.h"
#include "assets.h"
#include <stdlib.h>

void Intro_start(Intro* intro, int W, int H) {
    (void)W;
    (void)H;
    if (!intro) return;
    int len = 0;
    uint8_t* data = Assets_readAll("/intro.3gp", &len);
    if (!data) {
        intro->done = true;
    } else {
        free(data);
        intro->done = true;
    }
}

void Intro_stop(Intro* intro) {
    if (!intro) return;
    intro->done = true;
}
