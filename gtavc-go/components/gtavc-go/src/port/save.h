#ifndef GTAVC_SAVE_H
#define GTAVC_SAVE_H

#include <stdbool.h>

struct World;
struct Game;

bool Save_exists(void);
bool Save_write(struct World* w);
bool Save_load(struct World* w);
/* Native progress checkpoints in the existing RMS format, not full snapshots. */
bool Save_writePath(struct World* w, const char* path);
bool Save_loadPath(struct World* w, const char* path);
bool Save_writeOptions(struct Game* g);
bool Save_loadOptions(struct Game* g);

#endif /* GTAVC_SAVE_H */
