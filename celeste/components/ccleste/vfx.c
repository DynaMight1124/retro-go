/*
 * vfx.c - Visual effects (snow, clouds, particles, etc.)
 */

#include "vfx.h"
#include "gfx.h"
#include <stdlib.h>
#include <math.h>

/* Effect toggles */
bool snow_enabled = true;
bool clouds_enabled = true;

/* Freeze/Shake system */
int vfx_freeze = 0;
int vfx_shake = 0;

/* --- Snow particles --- */
#define SNOW_COUNT 50  /* More particles for larger screen */

typedef struct {
    float x, y;      /* position */
    float s;         /* size (0 or 1) */
    float spd;       /* horizontal speed */
    float off;       /* sine offset for vertical oscillation */
    uint8_t c;       /* color (6=light gray, 7=white) */
} SnowParticle;

static SnowParticle snow[SNOW_COUNT];

/* --- Dark background "clouds" for parallax effect --- */
#define CLOUD_COUNT 16

typedef struct {
    float x, y;      /* position */
    float spd;       /* horizontal speed */
    float w;         /* width */
} Cloud;

static Cloud clouds[CLOUD_COUNT];

/* --- Smoke/Puff particles (world-space) --- */
#define SMOKE_COUNT 16

typedef struct {
    bool active;
    float x, y;      /* world position */
    float vx, vy;    /* velocity */
    float spr;       /* sprite frame (29-31, fractional for animation) */
    bool flip_x;
    bool flip_y;
} SmokeParticle;

static SmokeParticle smoke[SMOKE_COUNT];

/* --- Death burst particles --- */
#define DEATH_PARTICLE_COUNT 8

typedef struct {
    bool active;
    float x, y;      /* world position */
    float vx, vy;    /* velocity */
    int t;           /* lifetime */
    uint8_t c;       /* color */
} DeathParticle;

static DeathParticle death_particles[DEATH_PARTICLE_COUNT];

/* --- Dash trail particles --- */
#define DASH_PARTICLE_COUNT 16

typedef struct {
    bool active;
    float x, y;      /* world position */
    int t;           /* lifetime */
    uint8_t c;       /* color */
    int size;        /* particle size */
} DashParticle;

static DashParticle dash_particles[DASH_PARTICLE_COUNT];

/* --- Lifeup floating text ("1000" on strawberry pickup) --- */
#define LIFEUP_COUNT 4

typedef struct {
    bool active;
    float x, y;      /* world position */
    int duration;    /* frames remaining */
    int flash;       /* animation counter for color flash */
} LifeupText;

static LifeupText lifeups[LIFEUP_COUNT];

/* --- Orb effect --- */
typedef struct {
    bool active;
    float x, y;          /* world position */
    float spd_y;         /* vertical speed (floats up then stops) */
    int frame;           /* animation frame counter */
} OrbState;

static OrbState orb = {false, 0, 0, 0, 0};

/* Random float in range [0, max) */
static float rnd(float max) {
    return ((float)rand() / (float)RAND_MAX) * max;
}

/* Floor (round toward negative infinity) */
static float flr(float v) {
    return (float)(int)(v < 0 ? v - 1 : v);
}

void vfx_init(void) {
    /* Initialize snow */
    for (int i = 0; i < SNOW_COUNT; i++) {
        snow[i].x = rnd(SCREEN_W);
        snow[i].y = rnd(SCREEN_H);
        snow[i].s = flr(rnd(5) / 4);  /* mostly 0, occasionally 1 */
        snow[i].spd = 0.25f + rnd(5);
        snow[i].off = rnd(1);
        snow[i].c = 6 + (int)flr(0.5f + rnd(1));  /* 6 or 7 */
    }

    /* Initialize clouds (scaled for 256x150 from 128x128) */
    for (int i = 0; i < CLOUD_COUNT; i++) {
        clouds[i].x = rnd(SCREEN_W);
        clouds[i].y = rnd(SCREEN_H);
        clouds[i].spd = 1 + rnd(4);
        clouds[i].w = 32 + rnd(32);  /* width 32-64 */
    }

    /* Initialize smoke particles as inactive */
    for (int i = 0; i < SMOKE_COUNT; i++) {
        smoke[i].active = false;
    }

    /* Initialize death particles as inactive */
    for (int i = 0; i < DEATH_PARTICLE_COUNT; i++) {
        death_particles[i].active = false;
    }

    /* Initialize dash particles as inactive */
    for (int i = 0; i < DASH_PARTICLE_COUNT; i++) {
        dash_particles[i].active = false;
    }

    /* Initialize lifeup texts as inactive */
    for (int i = 0; i < LIFEUP_COUNT; i++) {
        lifeups[i].active = false;
    }
}

void vfx_draw_bg(void) {
    if (!clouds_enabled) return;

    for (int i = 0; i < CLOUD_COUNT; i++) {
        Cloud* c = &clouds[i];

        /* Update position */
        c->x += c->spd;

        /* Height varies: wider clouds are flatter */
        int h = 4 + (int)((1 - c->w / 64.0f) * 12);
        gfx_rect((int)c->x, (int)c->y, (int)c->w, h, 1);  /* color 1 = dark blue */

        /* Wrap around when off screen */
        if (c->x > SCREEN_W) {
            c->x = -c->w;
            c->y = rnd(SCREEN_H - 8);
        }
    }
}

void vfx_update(void) {
    /* Update freeze/shake counters */
    if (vfx_freeze > 0) vfx_freeze--;
    if (vfx_shake > 0) vfx_shake--;

    /* Update orb */
    vfx_orb_update();

    /* Update smoke particles */
    for (int i = 0; i < SMOKE_COUNT; i++) {
        SmokeParticle* p = &smoke[i];
        if (!p->active) continue;

        p->x += p->vx;
        p->y += p->vy;
        p->spr += 0.2f;

        if (p->spr >= 32.0f) {
            p->active = false;
        }
    }

    /* Update death particles */
    for (int i = 0; i < DEATH_PARTICLE_COUNT; i++) {
        DeathParticle* p = &death_particles[i];
        if (!p->active) continue;

        p->x += p->vx;
        p->y += p->vy;
        p->t--;

        if (p->t <= 0) {
            p->active = false;
        }
    }

    /* Update dash particles */
    for (int i = 0; i < DASH_PARTICLE_COUNT; i++) {
        DashParticle* p = &dash_particles[i];
        if (!p->active) continue;

        p->t--;
        if (p->t <= 0) {
            p->active = false;
        }
    }

    /* Update lifeup texts (float upward) */
    for (int i = 0; i < LIFEUP_COUNT; i++) {
        LifeupText* l = &lifeups[i];
        if (!l->active) continue;

        l->y -= 0.25f;  /* Float upward */
        l->flash++;
        l->duration--;
        if (l->duration <= 0) {
            l->active = false;
        }
    }
}

void vfx_draw_fg(int cam_x, int cam_y) {
    /* Draw snow (screen-relative, ignores camera) */
    if (snow_enabled) {
        for (int i = 0; i < SNOW_COUNT; i++) {
            SnowParticle* p = &snow[i];

            /* Update position */
            p->x += p->spd;
            p->y += p8sin(p->off);
            p->off += (p->spd / 32 < 0.05f) ? p->spd / 32 : 0.05f;

            /* Draw particle (screen-relative, ignores camera) */
            int size = (int)p->s;
            gfx_rect((int)p->x, (int)p->y, size + 1, size + 1, p->c);

            /* Wrap around when off screen */
            if (p->x > SCREEN_W + 4) {
                p->x = -4;
                p->y = rnd(SCREEN_H);
            }
        }
    }

    /* Draw smoke particles (world-space) */
    for (int i = 0; i < SMOKE_COUNT; i++) {
        SmokeParticle* p = &smoke[i];
        if (!p->active) continue;

        int sx = (int)p->x - cam_x;
        int sy = (int)p->y - cam_y;

        /* Skip if off screen */
        if (sx < -8 || sx > SCREEN_W || sy < -8 || sy > SCREEN_H) continue;

        gfx_spr((int)p->spr, sx, sy, p->flip_x, p->flip_y);
    }

    /* Draw death particles (world-space) */
    for (int i = 0; i < DEATH_PARTICLE_COUNT; i++) {
        DeathParticle* p = &death_particles[i];
        if (!p->active) continue;

        int sx = (int)p->x - cam_x;
        int sy = (int)p->y - cam_y;

        /* Draw as small rectangle */
        gfx_rect(sx, sy, 1, 2, p->c);
    }

    /* Draw dash particles (world-space) */
    for (int i = 0; i < DASH_PARTICLE_COUNT; i++) {
        DashParticle* p = &dash_particles[i];
        if (!p->active) continue;

        int sx = (int)p->x - cam_x;
        int sy = (int)p->y - cam_y;

        gfx_rect(sx, sy, p->size, p->size, p->c);
    }

    /* Draw lifeup texts (world-space) */
    for (int i = 0; i < LIFEUP_COUNT; i++) {
        LifeupText* l = &lifeups[i];
        if (!l->active) continue;

        int sx = (int)l->x - cam_x;
        int sy = (int)l->y - cam_y;

        /* Color alternates between white (7) and yellow (10) */
        int color = 7 + (l->flash % 2) * 3;
        gfx_print_num(sx, sy, 1000, color);
    }
}

/* === Particle spawn functions === */

void vfx_smoke(float x, float y) {
    /* Find inactive smoke slot */
    for (int i = 0; i < SMOKE_COUNT; i++) {
        if (!smoke[i].active) {
            smoke[i].active = true;
            smoke[i].x = x - 1 + rnd(2);
            smoke[i].y = y - 1 + rnd(2);
            smoke[i].vx = 0.3f + rnd(0.2f);
            smoke[i].vy = -0.1f;
            smoke[i].spr = 29.0f;
            smoke[i].flip_x = rnd(2) > 1;
            smoke[i].flip_y = rnd(2) > 1;
            return;
        }
    }
}

void vfx_death_burst(float x, float y) {
    /* Spawn 8 particles in a circle */
    for (int i = 0; i < DEATH_PARTICLE_COUNT; i++) {
        float angle = (float)i / 8.0f;
        death_particles[i].active = true;
        death_particles[i].x = x + 4;
        death_particles[i].y = y + 4;
        death_particles[i].vx = p8sin(angle) * 3.0f;
        death_particles[i].vy = -p8sin(angle + 0.25f) * 3.0f;  /* cos = -sin(x+0.25) */
        death_particles[i].t = 10;
        death_particles[i].c = (i % 2 == 0) ? 8 : 14;  /* red and pink */
    }
}

void vfx_dash_particle(float x, float y, int color) {
    /* Find inactive dash particle slot */
    for (int i = 0; i < DASH_PARTICLE_COUNT; i++) {
        if (!dash_particles[i].active) {
            dash_particles[i].active = true;
            dash_particles[i].x = x + rnd(6) - 3;
            dash_particles[i].y = y + rnd(6) - 3;
            dash_particles[i].t = 4 + (int)rnd(4);
            dash_particles[i].c = color;
            dash_particles[i].size = 1 + (int)rnd(2);
            return;
        }
    }
}

void vfx_lifeup(float x, float y) {
    /* Find inactive lifeup slot */
    for (int i = 0; i < LIFEUP_COUNT; i++) {
        if (!lifeups[i].active) {
            lifeups[i].active = true;
            lifeups[i].x = x - 2;   /* Offset */
            lifeups[i].y = y - 4;
            lifeups[i].duration = 30;
            lifeups[i].flash = 0;
            return;
        }
    }
    /* If all slots full, overwrite slot 0 */
    lifeups[0].active = true;
    lifeups[0].x = x - 2;
    lifeups[0].y = y - 4;
    lifeups[0].duration = 30;
    lifeups[0].flash = 0;
}

/* === Freeze/Shake Functions === */

void vfx_get_shake_offset(int *ox, int *oy) {
    if (vfx_shake > 0) {
        /* Random offset -2 to +2 */
        *ox = (rand() % 5) - 2;
        *oy = (rand() % 5) - 2;
    } else {
        *ox = 0;
        *oy = 0;
    }
}

/* === Orb Functions === */

void vfx_orb_init(float x, float y) {
    orb.active = true;
    orb.x = x;
    orb.y = y;
    orb.spd_y = -4.0f;  /* Float upward initially */
    orb.frame = 0;
}

void vfx_orb_update(void) {
    if (!orb.active) return;

    /* Decelerate upward motion: appr(spd_y, 0, 0.5) */
    if (orb.spd_y < 0) {
        orb.spd_y += 0.5f;
        if (orb.spd_y > 0) orb.spd_y = 0;
    }
    orb.y += orb.spd_y;
    orb.frame++;
}

void vfx_orb_draw(int cam_x, int cam_y) {
    if (!orb.active) return;

    int sx = (int)orb.x - cam_x;
    int sy = (int)orb.y - cam_y;

    /* Draw orb sprite (sprite 102) */
    gfx_spr(102, sx, sy, false, false);

    /* Draw 8 rotating particles in a circle */
    float off = (float)orb.frame / 30.0f;
    for (int i = 0; i < 8; i++) {
        float angle = off + (float)i / 8.0f;
        int px = sx + 4 + (int)(p8sin(angle + 0.25f) * 8.0f);  /* cos = sin(x+0.25) in p8 */
        int py = sy + 4 + (int)(p8sin(angle) * 8.0f);
        /* Draw filled circle radius 1 (3x3 rect approximation) */
        gfx_rect(px - 1, py - 1, 3, 3, 7);  /* white */
    }
}

bool vfx_orb_active(void) {
    return orb.active && orb.spd_y == 0;  /* Only touchable when stopped */
}

bool vfx_orb_check_player(int px, int py, int pw, int ph) {
    if (!orb.active || orb.spd_y != 0) return false;

    /* Orb hitbox: 8x8 at orb position */
    int ox = (int)orb.x;
    int oy = (int)orb.y;

    bool hit = (px < ox + 8 && px + pw > ox &&
                py < oy + 8 && py + ph > oy);

    if (hit) {
        orb.active = false;  /* Consumed */
    }
    return hit;
}
