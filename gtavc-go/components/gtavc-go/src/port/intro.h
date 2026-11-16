#ifndef GTAVC_INTRO_H
#define GTAVC_INTRO_H

#include <stdbool.h>

typedef struct Intro {
    bool done;
} Intro;

void Intro_start(Intro* intro, int W, int H);
void Intro_stop(Intro* intro);

#endif /* GTAVC_INTRO_H */
