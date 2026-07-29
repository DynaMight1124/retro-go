/*
 * level.c - Map, tile collision, and game objects
 *
 * Each object type detects and interacts with the player from within its
 * own update function (the "objects detect player" pattern). This keeps
 * all logic for an entity in one place and eliminates the wide query/trigger
 * API that previously coupled player.c and level.c.
 */

#include "level.h"
#include "player.h"
#include "gfx.h"
#include "vfx.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern void psfx(int id);

#include "map_data.h"

/* === CONSTANTS === */

#define MAX_OBJECTS 150

#define FLAG_SOLID  1
#define FLAG_ICE    2
#define FLAG_SPIKE  4

static const uint8_t tile_flags[128] = {
    /* 00-0F */ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    /* 10-1F */ 4,2,0,0,0,0,0,0, 0,0,0,2,0,0,0,0,
    /* 20-2F */ 3,3,3,3,3,3,3,3, 4,4,4,2,2,0,0,0,
    /* 30-3F */ 3,3,3,3,3,3,3,3, 4,4,4,2,2,2,2,2,
    /* 40-4F */ 0,0,25,25,25,25,2,2, 3,2,2,2,2,2,0,2,
    /* 50-5F */ 0,0,25,25,25,25,2,2, 4,2,2,2,2,2,2,2,
    /* 60-6F */ 0,0,25,25,25,25,0,4, 4,2,2,2,2,2,2,2,
    /* 70-7F */ 0,0,25,25,25,25,0,0, 0,2,2,2,2,2,2,2,
};

/* Tile IDs from the sprite sheet */
#define T_SPAWN       1
#define T_KEY         8
#define T_PLAT_L      11
#define T_PLAT_R      12
#define T_SPIKE_UP    17
#define T_SPRING      18
#define T_CHEST       20
#define T_BALLOON     22
#define T_FALL_FLOOR  23
#define T_STRAW       26
#define T_SPIKE_DOWN  27
#define T_FLY_STRAW   28
#define T_SPIKE_LEFT  43
#define T_SPIKE_RIGHT 59
#define T_FAKE_WALL   64
#define T_BIG_CHEST   96
#define T_FLAG        118

/* === OBJECT SYSTEM === */

typedef enum {
    OBJ_NONE = 0,
    OBJ_PLATFORM, OBJ_SPRING, OBJ_BALLOON, OBJ_FALL_FLOOR,
    OBJ_KEY, OBJ_CHEST, OBJ_STRAWBERRY, OBJ_FLY_STRAWBERRY,
    OBJ_FAKE_WALL, OBJ_BIG_CHEST
} ObjType;

typedef struct {
    ObjType type;
    bool active;
    float x, y;
    float start_x, start_y;
    float vx, vy;
    int timer, state;

    union {
        struct { int dir; } plat;
        struct { bool collected; bool spawned; } berry;
        struct { bool collected; bool escaped; bool flying; float step; } flyer;
        struct { bool broken; } wall;
        struct { bool flip; bool consumed; } key;
        struct { bool opened; } chest;
    } data;
} LevelObject;

static LevelObject objects[MAX_OBJECTS];
static int total_strawberries;
static int collected_strawberries;
static bool has_key_held;
static int anim_frames;

/* === HELPERS === */

static float fsign(float v) {
    return v > 0 ? 1.0f : v < 0 ? -1.0f : 0.0f;
}

static bool overlap(LevelObject *obj, int px, int py, int pw, int ph,
                     int ow, int oh) {
    if (!obj->active) return false;
    return px < obj->x + ow && px + pw > obj->x &&
           py < obj->y + oh && py + ph > obj->y;
}

/* === MAP === */

uint8_t level_get(int x, int y) {
    if (y < 0 || y >= 474) return 0;
    RowInfo r = ROW_INDEX[y];
    int lx = x - r.x_start;
    if ((unsigned)lx >= r.width) return 0;
    return TILE_BLOB[r.blob_ptr + lx];
}

int level_height(void) { return 474 * 8; }

static bool map_is_solid(int x, int y) {
    if (x < 0 || y >= 474) return true;
    if (y <= 14) {
        RowInfo r = ROW_INDEX[y];
        if (x < r.x_start || x >= r.x_start + (int)r.width) return true;
    }
    uint8_t t = level_get(x, y);
    return t < 128 && (tile_flags[t] & FLAG_SOLID);
}

/* === OBJECT POOL === */

static LevelObject* spawn_object(ObjType type, float x, float y) {
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].active) {
            LevelObject *o = &objects[i];
            *o = (LevelObject){
                .type = type, .active = true,
                .x = x, .y = y, .start_x = x, .start_y = y
            };
            return o;
        }
    }
    return NULL;
}

/* === INIT === */

void level_init(void) {
    anim_frames = 0;
    total_strawberries = 0;
    collected_strawberries = 0;
    has_key_held = false;

    for (int i = 0; i < MAX_OBJECTS; i++)
        objects[i].active = false;

    for (int y = 0; y < 474; y++) {
        RowInfo r = ROW_INDEX[y];
        for (int lx = 0; lx < r.width; lx++) {
            uint8_t tile = TILE_BLOB[r.blob_ptr + lx];
            float px = (r.x_start + lx) * 8;
            float py = y * 8;
            LevelObject *o;

            switch (tile) {
            case T_PLAT_L: case T_PLAT_R:
                o = spawn_object(OBJ_PLATFORM, px - 4, py);
                if (o) o->data.plat.dir = (tile == T_PLAT_L) ? -1 : 1;
                break;
            case T_KEY:     spawn_object(OBJ_KEY, px, py); break;
            case T_CHEST:
                o = spawn_object(OBJ_CHEST, px - 4, py);
                if (o) o->timer = 20;
                break;
            case T_BIG_CHEST: spawn_object(OBJ_BIG_CHEST, px, py); break;
            case T_STRAW:
                if (spawn_object(OBJ_STRAWBERRY, px, py)) total_strawberries++;
                break;
            case T_FLY_STRAW:
                o = spawn_object(OBJ_FLY_STRAWBERRY, px, py);
                if (o) { o->data.flyer.step = 0.5f; total_strawberries++; }
                break;
            case T_FAKE_WALL:  spawn_object(OBJ_FAKE_WALL, px, py); break;
            case T_FALL_FLOOR: spawn_object(OBJ_FALL_FLOOR, px, py); break;
            case T_BALLOON:    spawn_object(OBJ_BALLOON, px, py); break;
            case T_SPRING:     spawn_object(OBJ_SPRING, px, py); break;
            }
        }
    }
}

/* === OBJECT UPDATE (with player interaction) === */

static void update_object(LevelObject *obj, Player *p) {
    int px = (int)p->x + PLAYER_HIT_X;
    int py = (int)p->y + PLAYER_HIT_Y;
    int pw = PLAYER_HIT_W, ph = PLAYER_HIT_H;

    switch (obj->type) {

    case OBJ_SPRING:
        if (obj->timer > 0) { obj->timer--; break; }
        if (p->vy >= 0 && overlap(obj, px, py, pw, ph, 8, 8)) {
            psfx(1);
            p->y = obj->y - 4;
            p->vy = -3.0f;
            p->vx *= 0.2f;
            p->dashes = p->max_dashes;
            obj->timer = 10;
        }
        break;

    case OBJ_BALLOON:
        if (obj->timer > 0) { obj->timer--; break; }
        if (p->dashes < p->max_dashes &&
            overlap(obj, px, py, pw, ph, 8, 8)) {
            psfx(22);
            p->dashes = p->max_dashes;
            obj->timer = 60;
        }
        break;

    case OBJ_FALL_FLOOR:
        if (obj->state == 1) {
            obj->timer--;
            if (obj->timer <= 0) { obj->state = 2; obj->timer = 60; }
        } else if (obj->state == 2) {
            obj->timer--;
            if (obj->timer <= 0 && !overlap(obj, px, py, pw, ph, 8, 8))
                obj->state = 0;
        } else {
            /* Trigger when player stands on top or touches from the side */
            bool on_top = p->grounded && p->vy >= 0 &&
                          overlap(obj, px, py + 1, pw, ph, 8, 8);
            bool beside = overlap(obj, px - 1, py, pw, ph, 8, 8) ||
                          overlap(obj, px + 1, py, pw, ph, 8, 8);
            if (on_top || beside) {
                obj->state = 1;
                obj->timer = 15;
            }
        }
        break;

    case OBJ_KEY:
        if (overlap(obj, px, py, pw, ph, 8, 8)) {
            psfx(23);
            obj->active = false;
            has_key_held = true;
        }
        break;

    case OBJ_CHEST:
        if (obj->data.chest.opened) break;
        {
            bool near = px < obj->start_x + 32 && px + pw > obj->start_x - 24 &&
                        py < obj->y + 32 && py + ph > obj->y - 24;
            if (has_key_held && near) {
                obj->timer--;
                obj->x = obj->start_x - 1 + ((anim_frames + (int)obj->y) % 3);
                if (obj->timer <= 0) {
                    psfx(38);
                    obj->data.chest.opened = true;
                    has_key_held = false;
                    /* Mark key as permanently consumed */
                    for (int k = 0; k < MAX_OBJECTS; k++) {
                        if (objects[k].type == OBJ_KEY && !objects[k].active &&
                            !objects[k].data.key.consumed) {
                            objects[k].data.key.consumed = true;
                            break;
                        }
                    }
                    LevelObject *b = spawn_object(OBJ_STRAWBERRY,
                                                  obj->x + 4, obj->y - 4);
                    if (b) { b->data.berry.spawned = true; total_strawberries++; }
                }
            } else {
                obj->x = obj->start_x;
                obj->timer = 20;
            }
        }
        break;

    case OBJ_STRAWBERRY:
        if (!obj->data.berry.collected &&
            overlap(obj, px, py, pw, ph, 8, 8)) {
            psfx(14);
            obj->data.berry.collected = true;
            collected_strawberries++;
            vfx_lifeup(obj->x, obj->y);
            p->dashes = p->max_dashes;
        }
        break;

    case OBJ_FLY_STRAWBERRY:
        if (obj->data.flyer.collected || obj->data.flyer.escaped) break;

        /* Trigger flying on the first frame of a new dash nearby */
        if (!obj->data.flyer.flying && p->dash_effect_time == 10) {
            int dx = (int)obj->x - (int)p->x;
            int dy = (int)obj->y - (int)p->y;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if (dx <= 80 && dy <= 80) {
                psfx(15); // Bird/flyer start
                obj->data.flyer.flying = true;
            }
        }

        if (obj->data.flyer.flying) {
            if (obj->vy > -3.5f) {
                obj->vy -= 0.25f;
                if (obj->vy < -3.5f) obj->vy = -3.5f;
            }
            obj->y += obj->vy;
            obj->data.flyer.step += 0.05f;
            if (obj->y < -16) { obj->data.flyer.escaped = true; break; }
        } else {
            obj->data.flyer.step += 0.05f;
            obj->y = obj->start_y + p8sin(obj->data.flyer.step) * 0.5f;
        }

        /* Collect on overlap */
        if (overlap(obj, px, py, pw, ph, 8, 8)) {
            psfx(14);
            obj->data.flyer.collected = true;
            collected_strawberries++;
            vfx_lifeup(obj->x, obj->y);
            p->dashes = p->max_dashes;
        }
        break;

    case OBJ_FAKE_WALL:
        if (obj->data.wall.broken) break;
        if (p->dash_effect_time > 0 &&
            px < obj->x + 17 && px + pw > obj->x - 1 &&
            py < obj->y + 17 && py + ph > obj->y - 1) {
            psfx(13); // Wall break
            obj->data.wall.broken = true;
            p->vx = -fsign(p->vx) * 1.5f;
            p->vy = -1.5f;
            p->dash_timer = 0;
            p->dash_effect_time = 0;
            /* Spawn hidden strawberry */
            LevelObject *b = spawn_object(OBJ_STRAWBERRY,
                                          obj->x + 4, obj->y + 4);
            if (b) { b->data.berry.spawned = true; total_strawberries++; }
            /* Smoke at corners */
            float ox = obj->x, oy = obj->y;
            vfx_smoke(ox, oy);      vfx_smoke(ox + 16, oy);
            vfx_smoke(ox, oy + 16); vfx_smoke(ox + 16, oy + 16);
        }
        break;

    case OBJ_BIG_CHEST:
        if (!obj->data.chest.opened && p->grounded &&
            overlap(obj, px, py, pw, ph, 16, 16)) {
            psfx(37);
            obj->data.chest.opened = true;
            vfx_orb_init(p->x, p->y - 4);
        }
        break;

    case OBJ_PLATFORM: {
        int old_ix = (int)obj->x;
        obj->x += obj->data.plat.dir * 0.65f;

        /* Wrap at row boundaries */
        int row = (int)(obj->y / 8);
        if (row >= 0 && row < 474) {
            RowInfo r = ROW_INDEX[row];
            int left  = (r.x_start - 2) * 8;
            int right = (r.x_start + r.width + 1) * 8;
            if (obj->x < left - 16) obj->x = right;
            else if (obj->x > right) obj->x = left - 16;
        }

        /* Carry player if standing on top */
        int carry = (int)obj->x - old_ix;
        if (carry != 0) {
            bool h_over = px < (int)obj->x + 16 && px + pw > (int)obj->x;
            bool on_top = py + ph >= (int)obj->y && py + ph <= (int)obj->y + 1;
            if (h_over && on_top) {
                int step = (carry > 0) ? 1 : -1;
                for (int s = 0; s < (carry < 0 ? -carry : carry); s++) {
                    if (!level_collide((int)p->x + PLAYER_HIT_X + step,
                                       (int)p->y + PLAYER_HIT_Y,
                                       PLAYER_HIT_W, PLAYER_HIT_H))
                        p->x += step;
                }
            }
        }
        break;
    }

    default: break;
    }
}

/* === PUBLIC UPDATE === */

void level_update(Player *p) {
    anim_frames = (anim_frames + 1) % 60;

    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].active)
            update_object(&objects[i], p);
    }

    /* Orb interaction (spawned by big chest, grants double dash) */
    if (vfx_orb_active()) {
        int px = (int)p->x + PLAYER_HIT_X;
        int py = (int)p->y + PLAYER_HIT_Y;
        if (vfx_orb_check_player(px, py, PLAYER_HIT_W, PLAYER_HIT_H)) {
            p->max_dashes = 2;
            p->dashes = 2;
            vfx_freeze = 10;
            vfx_shake = 10;
        }
    }
}

/* === TILE COLLISION === */

bool level_solid(int x, int y) {
    if (map_is_solid(x, y)) return true;

    for (int i = 0; i < MAX_OBJECTS; i++) {
        LevelObject *o = &objects[i];
        if (!o->active) continue;
        int ox = (int)(o->x / 8), oy = (int)(o->y / 8);

        if (o->type == OBJ_FALL_FLOOR) {
            if (ox == x && oy == y) return o->state != 2;
        } else if (o->type == OBJ_FAKE_WALL) {
            if (!o->data.wall.broken &&
                x >= ox && x < ox + 2 && y >= oy && y < oy + 2)
                return true;
        }
    }
    return false;
}

bool level_collide(int px, int py, int w, int h) {
    int x1 = px / 8, y1 = py / 8;
    int x2 = (px + w - 1) / 8, y2 = (py + h - 1) / 8;
    for (int ty = y1; ty <= y2; ty++)
        for (int tx = x1; tx <= x2; tx++)
            if (level_solid(tx, ty)) return true;
    return false;
}

bool level_spikes_at(float x, float y, int w, int h, float xspd, float yspd) {
    int x1 = (int)x / 8, x2 = (int)(x + w - 1) / 8;
    int y1 = (int)y / 8, y2 = (int)(y + h - 1) / 8;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;

    for (int ty = y1; ty <= y2; ty++) {
        for (int tx = x1; tx <= x2; tx++) {
            uint8_t t = level_get(tx, ty);
            if (t == T_SPIKE_UP    && yspd >= 0 && (int)(y+h-1) % 8 >= 6) return true;
            if (t == T_SPIKE_DOWN  && yspd <= 0 && (int)y % 8 <= 2) return true;
            if (t == T_SPIKE_LEFT  && xspd <= 0 && (int)x % 8 <= 2) return true;
            if (t == T_SPIKE_RIGHT && xspd >= 0 && (int)(x+w-1) % 8 >= 6) return true;
        }
    }
    return false;
}

bool level_platform_solid(int px, int py, int w, int h, int ox, int oy) {
    if (oy <= 0) return false;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        LevelObject *o = &objects[i];
        if (!o->active || o->type != OBJ_PLATFORM) continue;
        /* Skip if currently overlapping */
        if (px < o->x + 16 && px + w > o->x &&
            py < o->y + 8 && py + h > o->y) continue;
        /* Check at offset */
        if (px + ox < o->x + 16 && px + ox + w > o->x &&
            py + oy < o->y + 8 && py + oy + h > o->y) return true;
    }
    return false;
}

/* === STATE === */

bool level_has_key(void)  { return has_key_held; }
void level_reset_key(void) { has_key_held = false; }

bool level_big_chest_opened(void) {
    for (int i = 0; i < MAX_OBJECTS; i++)
        if (objects[i].active && objects[i].type == OBJ_BIG_CHEST)
            return objects[i].data.chest.opened;
    return false;
}

int level_strawberry_total(void)     { return total_strawberries; }
int level_strawberry_collected(void) { return collected_strawberries; }

void level_reset_on_death(void) {
    has_key_held = false;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        LevelObject *o = &objects[i];
        switch (o->type) {
        case OBJ_FALL_FLOOR: o->state = 0; o->timer = 0; break;
        case OBJ_BALLOON:    o->timer = 0; break;
        case OBJ_KEY:
            if (!o->active && !o->data.key.consumed) o->active = true;
            break;
        default: break;
        }
    }
}

/* === DRAWING === */

static bool is_dynamic_tile(uint8_t t) {
    return t == T_PLAT_L || t == T_PLAT_R || t == T_SPRING || t == T_BALLOON ||
           t == T_FALL_FLOOR || t == T_STRAW || t == T_FLY_STRAW ||
           t == T_FAKE_WALL || t == T_KEY || t == T_CHEST ||
           t == T_BIG_CHEST || t == T_BIG_CHEST + 1 || t == T_SPAWN;
}

static void draw_object(LevelObject *obj, int cx, int cy) {
    int sx = (int)obj->x - cx, sy = (int)obj->y - cy;
    if (sx < -16 || sx > SCREEN_W || sy < -16 || sy > SCREEN_H) return;

    switch (obj->type) {
    case OBJ_PLATFORM:
        gfx_spr(11, sx, sy, false, false);
        gfx_spr(12, sx + 8, sy, false, false);
        break;

    case OBJ_SPRING:
        gfx_spr(obj->timer > 0 ? 19 : 18, sx, sy, false, false);
        break;

    case OBJ_BALLOON:
        if (obj->timer == 0) {
            float off = anim_frames * 0.01f;
            int bob = (int)(p8sin(off) * 2.0f);
            gfx_spr(13 + ((int)(off * 8.0f) % 3), sx, sy + 6 + bob, false, false);
            gfx_spr(22, sx, sy + bob, false, false);
        }
        break;

    case OBJ_FALL_FLOOR:
        if (obj->state != 2) {
            int f = (obj->state == 1) ? (15 - obj->timer) / 5 : 0;
            gfx_spr(23 + f, sx, sy, false, false);
        }
        break;

    case OBJ_KEY:
        if (!obj->active) break;
        {
            float t = (anim_frames % 60) / 30.0f;
            int spr = (int)(9.0f + (p8sin(t) + 0.5f));
            if (spr < 8)  spr = 8;
            if (spr > 10) spr = 10;
            bool flip = (anim_frames % 60 >= 17 && anim_frames % 60 < 47);
            gfx_spr(spr, sx, sy, flip, false);
        }
        break;

    case OBJ_CHEST:
        if (!obj->data.chest.opened) gfx_spr(20, sx, sy, false, false);
        break;

    case OBJ_BIG_CHEST:
        if (!obj->data.chest.opened) {
            gfx_spr(96, sx, sy, false, false);
            gfx_spr(97, sx + 8, sy, false, false);
        }
        gfx_spr(112, sx, sy + 8, false, false);
        gfx_spr(113, sx + 8, sy + 8, false, false);
        break;

    case OBJ_FAKE_WALL:
        if (!obj->data.wall.broken) {
            gfx_spr(64, sx, sy, false, false);
            gfx_spr(65, sx + 8, sy, false, false);
            gfx_spr(80, sx, sy + 8, false, false);
            gfx_spr(81, sx + 8, sy + 8, false, false);
        }
        break;

    case OBJ_STRAWBERRY:
        if (!obj->data.berry.collected) {
            int bob = (int)(p8sin(anim_frames / 40.0f) * 2.5f);
            gfx_spr(26, sx, sy + bob, false, false);
        }
        break;

    case OBJ_FLY_STRAWBERRY:
        if (!obj->data.flyer.collected && !obj->data.flyer.escaped) {
            float step = obj->data.flyer.step;
            float dir = p8sin(step);
            int wing = 0;
            if (obj->data.flyer.flying) {
                wing = ((int)(step * 4)) % 3;
            } else {
                if (dir < 0) { wing = 1; if (obj->y < obj->start_y) wing = 2; }
            }
            gfx_spr(45 + wing, sx - 6, sy - 2, true, false);
            gfx_spr(28, sx, sy, false, false);
            gfx_spr(45 + wing, sx + 6, sy - 2, false, false);
        }
        break;

    default: break;
    }
}

void level_draw(int cam_x, int cam_y) {
    int tx_start = cam_x / 8;
    int ty_start = cam_y / 8;
    int tx_end = tx_start + (SCREEN_W / 8) + 1;
    int ty_end = ty_start + (SCREEN_H / 8) + 1;
    int off_x = cam_x % 8;
    int off_y = cam_y % 8;

    /* Layer 1: Background tiles (spike flag) */
    for (int ty = ty_start; ty <= ty_end; ty++) {
        if (ty < 0 || ty >= 474) continue;
        RowInfo r = ROW_INDEX[ty];
        for (int tx = tx_start; tx <= tx_end; tx++) {
            if (tx < r.x_start || tx >= r.x_start + (int)r.width) continue;
            uint8_t t = TILE_BLOB[r.blob_ptr + (tx - r.x_start)];
            if (t && !is_dynamic_tile(t) && (tile_flags[t] & 4))
                gfx_spr(t, tx * 8 - cam_x, ty * 8 - cam_y, false, false);
        }
    }

    /* Layer 2: Back objects */
    for (int i = 0; i < MAX_OBJECTS; i++) {
        LevelObject *o = &objects[i];
        if (!o->active) continue;
        ObjType t = o->type;
        if (t == OBJ_PLATFORM || t == OBJ_FAKE_WALL || t == OBJ_BIG_CHEST ||
            t == OBJ_CHEST || t == OBJ_BALLOON)
            draw_object(o, cam_x, cam_y);
    }

    /* Layer 3: Ice borders */
    for (int ty = 0; ty <= (SCREEN_H / 8) + 1; ty++) {
        int map_y = ty_start + ty;
        int sy = ty * 8 - off_y;
        if (map_y < 0) continue;
        if (map_y >= 474) { gfx_rect(0, sy, SCREEN_W, 8, 12); continue; }
        if (map_y <= 15) continue;

        RowInfo r = ROW_INDEX[map_y];
        if (tx_start < r.x_start) {
            int w = (r.x_start - tx_start) * 8;
            if (w > SCREEN_W) w = SCREEN_W;
            gfx_rect(0 - off_x, sy, w, 8, 12);
        }
        int row_end = r.x_start + r.width;
        if (tx_end > row_end) {
            int start = row_end - tx_start;
            if (start < 0) start = 0;
            int x = start * 8 - off_x;
            if (x < SCREEN_W) gfx_rect(x, sy, SCREEN_W - x, 8, 12);
        }
    }

    /* Layer 4: Foreground tiles */
    for (int ty = ty_start; ty <= ty_end; ty++) {
        if (ty < 0 || ty >= 474) continue;
        RowInfo r = ROW_INDEX[ty];
        for (int tx = tx_start; tx <= tx_end; tx++) {
            if (tx < r.x_start || tx >= r.x_start + (int)r.width) continue;
            uint8_t t = TILE_BLOB[r.blob_ptr + (tx - r.x_start)];
            if (t && !is_dynamic_tile(t) && !(tile_flags[t] & 4)) {
                if (t == T_FLAG) {
                    int at = t + (anim_frames / 5) % 3;
                    gfx_spr(at, tx * 8 - cam_x + 5, ty * 8 - cam_y, false, false);
                } else {
                    gfx_spr(t, tx * 8 - cam_x, ty * 8 - cam_y, false, false);
                }
            }
        }
    }

    /* Layer 5: Front objects */
    for (int i = 0; i < MAX_OBJECTS; i++) {
        LevelObject *o = &objects[i];
        if (!o->active) continue;
        ObjType t = o->type;
        if (t == OBJ_FALL_FLOOR || t == OBJ_SPRING || t == OBJ_KEY ||
            t == OBJ_STRAWBERRY || t == OBJ_FLY_STRAWBERRY)
            draw_object(o, cam_x, cam_y);
    }
}

/* === SAVE STATE SUPPORT === */

typedef struct {
    LevelObject objects[MAX_OBJECTS];
    int total_strawberries;
    int collected_strawberries;
    bool has_key_held;
    int anim_frames;
} LevelState;

size_t level_get_state_size(void) {
    return sizeof(LevelState);
}

void level_get_state(void *dest) {
    LevelState *s = (LevelState *)dest;
    memcpy(s->objects, objects, sizeof(objects));
    s->total_strawberries = total_strawberries;
    s->collected_strawberries = collected_strawberries;
    s->has_key_held = has_key_held;
    s->anim_frames = anim_frames;
}

void level_set_state(const void *src) {
    const LevelState *s = (const LevelState *)src;
    memcpy(objects, s->objects, sizeof(objects));
    total_strawberries = s->total_strawberries;
    collected_strawberries = s->collected_strawberries;
    has_key_held = s->has_key_held;
    anim_frames = s->anim_frames;
}
