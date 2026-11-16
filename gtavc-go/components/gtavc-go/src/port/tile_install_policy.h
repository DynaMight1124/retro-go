#ifndef GTAVC_TILE_INSTALL_POLICY_H
#define GTAVC_TILE_INSTALL_POLICY_H

#include <stdint.h>

/* Finish immediately inside or within the existing 38 m streaming lookahead
   of the tile's bounds. The expanded rectangle is conservative at corners. */
static inline int Tile_installBudgetUs(int i, int j, int tileMm, int ox, int oz,
                                      int playerX, int playerZ) {
    int64_t left = (int64_t)ox + (int64_t)i * tileMm;
    int64_t top = (int64_t)oz + (int64_t)j * tileMm;
    if (tileMm <= 0 ||
        ((int64_t)playerX >= left - 38000 && (int64_t)playerX <= left + tileMm + 38000 &&
         (int64_t)playerZ >= top - 38000 && (int64_t)playerZ <= top + tileMm + 38000))
        return 0;
    return 16000;
}
#endif
