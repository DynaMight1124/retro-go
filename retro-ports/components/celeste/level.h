/*
 * level.h - Level map and object system
 *
 * Objects detect and interact with the player directly in their update
 * functions. No query/trigger API pairs needed.
 */

#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct Player; /* forward decl (defined in player.h) */

#define MAP_W 160
#define MAP_H 520
#define TILE_SIZE 8

void level_init(void);

/* Update all objects: movement, timers, and player interactions.
 * Objects detect and modify the player directly. */
void level_update(struct Player *p);

void level_draw(int cam_x, int cam_y);

/* Tile queries */
uint8_t level_get(int x, int y);
bool level_solid(int x, int y);
bool level_collide(int x, int y, int w, int h);
bool level_spikes_at(float x, float y, int w, int h, float xspd, float yspd);
int  level_height(void);

/* Platform collision (used by player movement code) */
bool level_platform_solid(int px, int py, int w, int h, int ox, int oy);

/* State queries */
bool level_has_key(void);
bool level_big_chest_opened(void);
int  level_strawberry_total(void);
int  level_strawberry_collected(void);

/* Reset on death (fall floors, balloons, keys) */
void level_reset_on_death(void);
void level_reset_key(void);

/* Save State Support */
size_t level_get_state_size(void);
void level_get_state(void *dest);
void level_set_state(const void *src);

#endif /* LEVEL_H */
