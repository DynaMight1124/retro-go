#pragma once
#include <stdbool.h>
void q2_world_init(void);
void q2_world_shutdown(void);
void QG_WorldDraw(void (*draw)(void *, int), void *arg, bool parallel);
