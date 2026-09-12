/*
 * player.h - Player state and physics
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include <stddef.h>

/* Hitbox (relative to sprite position) */
#define PLAYER_HIT_X 1
#define PLAYER_HIT_Y 3
#define PLAYER_HIT_W 6
#define PLAYER_HIT_H 5

/* Hair segment */
typedef struct {
    float x, y;
    int size;
} Hair;

/* Player state */
typedef struct Player {
    /* Position & Physics */
    float x, y;
    float vx, vy;
    float rx, ry; /* Subpixel remainder */
    bool flip_x;
    bool grounded;

    /* State */
    int facing;   /* -1 or 1 */
    int anim;     /* Current sprite ID */
    float run_anim_timer;

    /* Capabilities */
    int dashes;
    int max_dashes;
    int dash_timer; /* Frames remaining in dash */
    int dash_effect_time; /* Frames remaining where dash can break walls */
    float dash_tx, dash_ty; /* Dash target velocity */

    /* Buffers */
    int jump_buffer; /* Frames since jump press */
    int grace_timer; /* Coyote time frames */

    /* Hair */
    Hair hair[5];
} Player;

/* Get player instance (read-only for game/drawing code) */
const Player* player_get(void);

/* Get mutable player pointer (for object interactions in level.c) */
Player* player_get_mut(void);

/* Initialize player at position */
void player_init(float x, float y, int max_dashes);

/* Respawn player at position (resets velocity, dashes, hair) */
void player_respawn(float x, float y);

/* Update player physics. Returns true if player died this frame. */
bool player_update(int frame_count);

/* Draw player and hair */
void player_draw(int cam_x, int cam_y, int frame_count);

/* Set max dashes (for big chest powerup) */
void player_set_max_dashes(int max_dashes);

/* Get hair color based on dash state and frame */
int player_hair_color(int frame_count);

/* Synchronize edge-triggered controls after a reset or state load. */
void player_sync_input_latches(void);

/* Save State Support */
size_t player_get_state_size(void);
void player_get_state(void *dest);
void player_set_state(const void *src);

#endif /* PLAYER_H */
