#pragma once

#include <stdbool.h>

/* Selects the course whose segments 6 and 7 are active; -1 clears them. */
void mk64_segment_set_course(int course);
void mk64_segment_set_logo(bool active);
void mk64_segment_set_ceremony(bool active);
