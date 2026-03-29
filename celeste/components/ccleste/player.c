/*
 * player.c - Player physics and rendering
 */

#include "player.h"
#include "gfx.h"
#include "level.h"
#include "vfx.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern void psfx(int id);

/* Physics Constants */
#define PHY_GRAVITY      0.21f
#define PHY_MAX_FALL     2.0f
#define PHY_ACCEL        0.6f
#define PHY_FRICTION     0.4f
#define PHY_MAX_RUN      1.0f
#define PHY_JUMP_SPD     -2.0f
#define PHY_WALL_SLIDE   0.4f
#define PHY_DASH_SPD     5.0f
#define PHY_DASH_DIAG    3.5355f /* 5 / sqrt(2) */

/* Sprites */
#define SPR_STAND   1
#define SPR_JUMP    3
#define SPR_WALL    5
#define SPR_CROUCH  6
#define SPR_LOOKUP  7

/* Global player instance */
static Player p;

/* === MATH UTILS === */

static float appr(float val, float target, float amount) {
    return (val > target) ?
           ((val - amount < target) ? target : val - amount) :
           ((val + amount > target) ? target : val + amount);
}

static float sign(float v) {
    return (v > 0) ? 1.0f : (v < 0) ? -1.0f : 0.0f;
}

static float absf(float v) {
    return (v < 0) ? -v : v;
}

/* === COLLISION === */

static bool check_solid(int ox, int oy) {
    /* Check Level Tiles */
    if (level_collide(p.x + PLAYER_HIT_X + ox, p.y + PLAYER_HIT_Y + oy, PLAYER_HIT_W, PLAYER_HIT_H))
        return true;

    /* Check Platforms */
    if (level_platform_solid(p.x + PLAYER_HIT_X, p.y + PLAYER_HIT_Y, PLAYER_HIT_W, PLAYER_HIT_H, ox, oy))
        return true;

    return false;
}

static void move_x(float amount) {
    p.rx += amount;
    int move = (int)roundf(p.rx);
    p.rx -= move;

    int step = sign(move);
    if (step == 0) return;

    /* GOTCHA: Using <= abs(move) to replicate original N+1 physics loop.
       If move is 1, loop runs for 0 and 1 (2 pixels). */
    int count = abs(move);
    for (int i = 0; i <= count; i++) {
        if (!check_solid(step, 0)) {
            p.x += step;
        } else {
            p.vx = 0;
            p.rx = 0;
            break;
        }
    }
}

static void move_y(float amount) {
    p.ry += amount;
    int move = (int)roundf(p.ry);
    p.ry -= move;

    int step = sign(move);
    if (step == 0) return;

    /* GOTCHA: Using <= abs(move) to replicate original N+1 physics loop. */
    int count = abs(move);
    for (int i = 0; i <= count; i++) {
        if (!check_solid(0, step)) {
            p.y += step;
        } else {
            p.vy = 0;
            p.ry = 0;
            break;
        }
    }
}

/* === HAIR === */

static void hair_init(void) {
    for (int i = 0; i < 5; i++) {
        p.hair[i].x = p.x;
        p.hair[i].y = p.y;
        p.hair[i].size = (i < 3) ? 2 : 1;
    }
}

int player_hair_color(int frame_count) {
    if (p.dashes >= 2) return ((frame_count / 3) % 2 == 0) ? 7 : 11; /* Flash White/Pink */
    return (p.dashes == 1) ? 8 : 12; /* Red or Blue */
}

/* === PUBLIC API === */

const Player* player_get(void) { return &p; }
Player* player_get_mut(void) { return &p; }

void player_init(float x, float y, int max_dashes) {
    p.x = x;
    p.y = y;
    p.vx = 0;
    p.vy = 0;
    p.rx = 0;
    p.ry = 0;
    p.flip_x = false;
    p.grounded = false;
    p.facing = 1;
    p.anim = SPR_STAND;
    p.run_anim_timer = 0;
    p.dashes = max_dashes;
    p.max_dashes = max_dashes;
    p.dash_timer = 0;
    p.dash_effect_time = 0;
    p.dash_tx = 0;
    p.dash_ty = 0;
    p.jump_buffer = 0;
    p.grace_timer = 0;
    hair_init();
}

void player_respawn(float x, float y) {
    p.x = x;
    p.y = y;
    p.vx = 0;
    p.vy = 0;
    p.rx = 0;
    p.ry = 0;
    p.dashes = p.max_dashes;
    p.dash_timer = 0;
    p.dash_effect_time = 0;
    p.facing = 1;
    hair_init();
}

void player_set_max_dashes(int max_dashes) {
    p.max_dashes = max_dashes;
    p.dashes = max_dashes;
}

bool player_update(int frame_count) {
    /* Dash effect timer (10-frame window for breaking fake walls) */
    if (p.dash_effect_time > 0) p.dash_effect_time--;

    /* --- Phase 1: Input --- */
    bool k_left  = input_left();
    bool k_right = input_right();
    bool k_up    = input_up();
    bool k_down  = input_down();
    bool k_jump  = input_jump();
    bool k_dash  = input_dash();

    /* Input buffering */
    static bool p_jump = false;
    static bool p_dash = false;
    bool jump_pressed = k_jump && !p_jump;
    bool dash_pressed = k_dash && !p_dash;
    p_jump = k_jump;
    p_dash = k_dash;

    int dir_x = (k_right ? 1 : 0) - (k_left ? 1 : 0);
    int dir_y = (k_down ? 1 : 0) - (k_up ? 1 : 0);

    /* --- Phase 2: Movement (X/Y) --- */
    /* Platform carrying is handled by the platform object in level.c */

    move_x(p.vx);
    move_y(p.vy);

    /* --- Phase 3: Status Checks --- */

    /* Hazards */
    if (p.y > level_height() + 16) {
        psfx(4);
        vfx_death_burst(p.x, p.y);
        return true; /* Player died */
    }
    if (level_spikes_at(p.x + PLAYER_HIT_X, p.y + PLAYER_HIT_Y, PLAYER_HIT_W, PLAYER_HIT_H, p.vx, p.vy)) {
        psfx(4);
        vfx_death_burst(p.x, p.y);
        return true; /* Player died */
    }

    /* Ground check */
    bool was_grounded = p.grounded;
    p.grounded = check_solid(0, 1);

    if (p.grounded) {
        if (!was_grounded) psfx(5); // Land
        p.grace_timer = 6;
        if (p.dashes < p.max_dashes) {
            psfx(54); // Refill
            p.dashes = p.max_dashes;
        }
    } else if (p.grace_timer > 0) {
        p.grace_timer--;
    }

    /* Jump Buffer */
    if (jump_pressed) p.jump_buffer = 4;
    else if (p.jump_buffer > 0) p.jump_buffer--;

    /* --- Phase 4: Velocity Update --- */

    if (p.dash_timer > 0) {
        /* Dashing */
        p.dash_timer--;
        /* Approach dash target speed (high acceleration) */
        float ax = 1.5f;
        float ay = 1.5f;
        if (p.vy != 0) ax *= 0.707f;
        if (p.vx != 0) ay *= 0.707f;

        p.vx = appr(p.vx, p.dash_tx, ax);
        p.vy = appr(p.vy, p.dash_ty, ay);

        /* Dash particles */
        vfx_dash_particle(p.x + 4, p.y + 4, player_hair_color(frame_count));
    } else {
        /* Normal Movement */

        /* X-Axis */
        float target = dir_x * PHY_MAX_RUN;
        float accel = p.grounded ? PHY_ACCEL : PHY_ACCEL * 0.65f; /* Less control in air */

        if (absf(p.vx) > PHY_MAX_RUN && sign(p.vx) == dir_x) {
            p.vx = appr(p.vx, target, PHY_FRICTION * 0.5f); /* Conserve momentum */
        } else {
            p.vx = appr(p.vx, target, accel);
        }

        /* Y-Axis (Gravity) */
        float grav = PHY_GRAVITY;
        float max_fall = PHY_MAX_FALL;

        if (absf(p.vy) < 0.2f && !k_jump) grav *= 0.5f; /* Peak hang time */

        /* Wall Slide friction */
        if (dir_x != 0 && check_solid(dir_x, 0) && p.vy > 0) {
            max_fall = PHY_WALL_SLIDE;
            /* Wall jump smoke */
            if (frame_count % 5 == 0) vfx_smoke(p.x + (dir_x > 0 ? 8 : -4), p.y + 4);
        }

        if (!p.grounded) {
            p.vy = appr(p.vy, max_fall, grav);
        }

        /* Jumping */
        if (p.jump_buffer > 0) {
            if (p.grace_timer > 0) {
                /* Ground Jump */
                psfx(1);
                p.vy = PHY_JUMP_SPD;
                p.jump_buffer = 0;
                p.grace_timer = 0;
                vfx_smoke(p.x, p.y + 8);
            } else {
                /* Wall Jump */
                int wall_dir = 0;
                if (check_solid(-3, 0)) wall_dir = -1;
                else if (check_solid(3, 0)) wall_dir = 1;

                if (wall_dir != 0) {
                    psfx(2);
                    p.vy = PHY_JUMP_SPD;
                    p.vx = -wall_dir * (PHY_MAX_RUN + 1.0f); /* Kick off wall */
                    p.jump_buffer = 0;
                    vfx_smoke(p.x + (wall_dir > 0 ? 8 : -4), p.y);
                }
            }
        }

        /* Dash Start */
        if (dash_pressed) {
            if (p.dashes > 0) {
                psfx(3);
                p.dashes--;
                p.dash_timer = 4;
                p.dash_effect_time = 10;
                vfx_shake = 5;
                vfx_smoke(p.x, p.y);

                float dx = 0, dy = 0;
                if (dir_x != 0 || dir_y != 0) {
                    dx = dir_x; dy = dir_y;
                } else {
                    dx = (p.facing == 1) ? 1 : -1;
                }

                if (dx != 0 && dy != 0) {
                    p.vx = dx * PHY_DASH_DIAG;
                    p.vy = dy * PHY_DASH_DIAG;
                } else {
                    p.vx = dx * PHY_DASH_SPD;
                    p.vy = dy * PHY_DASH_SPD;
                }

                p.dash_tx = 2.0f * sign(p.vx);
                p.dash_ty = 2.0f * sign(p.vy);
                if (p.vy < 0) p.dash_ty *= 0.75f; /* Dashing up slows down faster */
            } else {
                psfx(9); // No dashes left
            }
        }
    }

    /* --- Phase 5: Animation State --- */
    /* Object interactions (springs, balloons, etc.) handled by level_update() */
    if (p.vx != 0) p.facing = (p.vx > 0) ? 1 : -1;
    p.flip_x = (p.facing < 0);

    if (!p.grounded) {
        bool wall_sliding = (dir_x != 0 && check_solid(dir_x, 0) && p.vy > 0);
        p.anim = wall_sliding ? SPR_WALL : SPR_JUMP;
    } else {
        if (k_down) p.anim = SPR_CROUCH;
        else if (k_up) p.anim = SPR_LOOKUP;
        else if (p.vx != 0) {
            p.run_anim_timer += 0.25f;
            p.anim = SPR_STAND + ((int)p.run_anim_timer % 4);
        } else {
            p.anim = SPR_STAND;
            p.run_anim_timer = 0;
        }
    }

    return false; /* Player alive */
}

void player_draw(int cam_x, int cam_y, int frame_count) {
    int color = player_hair_color(frame_count);

    /* Hair follows player with lag */
    float target_x = p.x + 4 - (p.facing * 2);
    float target_y = p.y + (p.anim == SPR_CROUCH ? 4 : 3);

    gfx_pal(8, color); /* Pal swap 8 -> hair color */

    for (int i = 0; i < 5; i++) {
        Hair* h = &p.hair[i];
        h->x += (target_x - h->x) / 1.5f;
        h->y += (target_y + 0.5f - h->y) / 1.5f;

        gfx_rect((int)h->x - cam_x - h->size, (int)h->y - cam_y - h->size,
                 h->size*2+1, h->size*2+1, color);

        target_x = h->x; target_y = h->y;
    }

    /* Draw sprite with hair color palette swap */
    gfx_spr(p.anim, (int)p.x - cam_x, (int)p.y - cam_y, p.flip_x, false);
    gfx_pal_reset();
}

/* === SAVE STATE SUPPORT === */

size_t player_get_state_size(void) {
    return sizeof(Player);
}

void player_get_state(void *dest) {
    memcpy(dest, &p, sizeof(Player));
}

void player_set_state(const void *src) {
    memcpy(&p, src, sizeof(Player));
}
