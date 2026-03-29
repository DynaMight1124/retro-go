/*
 * main.c - Celeste Classic / Scrolleste SDL2 Clone
 */

#include "gfx.h"
#include "level.h"
#include "vfx.h"
#include "player.h"
#include "audio.h"
#include <stdio.h>
#include <string.h>
#include <rg_system.h>

/* === CONFIGURATION === */
#define TARGET_FPS       30
#define START_LVL_IDX    0
#define SHOW_DEBUG       0

/* Colors */
#define COL_BLACK 0
#define COL_WHITE 7

static int sfx_timer = 0;

void psfx(int id) {
    if (sfx_timer <= 0) {
        audio_sfx_play(id);
        sfx_timer = 2; // Simple debounce/limit
    }
}

/* === DATA STRUCTURES === */

typedef struct {
    int x, y; /* Tile Coordinates */
} Checkpoint;

/* Checkpoints (Bottom to Top) */
static const Checkpoint checkpoints[] = {
    {1, 470}, {16, 457}, {25, 440}, {37, 426}, {39, 409}, {34, 393},
    {43, 380}, {52, 364}, {52, 346}, {56, 329}, {65, 310}, {78, 292},
    {94, 279}, {109, 268}, {116, 250}, {118, 233}, {127, 218}, {135, 204},
    {134, 190}, {146, 170}, {167, 164}, {160, 151}, {172, 135}, {181, 121},
    {184, 104}, {197, 90}, {213, 74}, {231, 53}, {245, 38}, {267, 26},
    {275, 14}
};
#define NUM_CHECKPOINTS (sizeof(checkpoints)/sizeof(Checkpoint))

typedef enum { MODE_TITLE, MODE_PLAY, MODE_WIN } GameMode;

typedef struct {
    GameMode mode;
    int deaths;
    int frames, seconds, minutes;

    /* Camera */
    int cam_x, cam_y;
    int max_y; /* Highest point reached (lowest Y value) */

    /* Logic */
    int respawn_idx;
    bool victory_flag;

    /* Title screen */
    bool start_game;
    int start_game_flash;
} Game;

/* Global Instance */
static Game g;

/* Memorial (typewriter text when player stands near the gravestone) */
#define MEMORIAL_TX  82  /* Tile X (spans 82-83) */
#define MEMORIAL_TY  285 /* Tile Y */
static const char memorial_text[] =
    "-- celeste mountain --#this memorial to those# perished on the climb";
static float memorial_idx;  /* Characters revealed so far */
static int title_frames;    /* Animation counter for title screen */

/* Altitude marker notification */
static int alt_shown;       /* Highest checkpoint index shown so far (-1 = none) */
static int alt_timer;       /* Frames remaining (>0 = visible) */
static float alt_fall_y;    /* Vertical offset (increases when falling off) */
static float alt_fall_spd;  /* Fall speed (accelerates) */
static char alt_text[16];   /* Current text e.g. "1200 m" or "summit" */
#define ALT_HOLD   40       /* Frames to hold before falling */
#define ALT_GRAVITY 0.3f

/* === PLAYER HELPERS === */

static void respawn_player(void) {
    /* Find best checkpoint (highest Y reached) */
    int best = 0;
    for (unsigned int i = 0; i < NUM_CHECKPOINTS; i++) {
        if (checkpoints[i].y * 8 >= g.max_y) {
            if (checkpoints[i].y < checkpoints[best].y) best = i;
        }
    }

    g.respawn_idx = best;
    float px = checkpoints[best].x * 8;
    float py = checkpoints[best].y * 8;
    player_respawn(px, py);
}

static void kill_player(void) {
    g.deaths++;
    level_reset_on_death();  /* Reset fall floors, balloons, keys BEFORE respawn */
    respawn_player();
}

/* === GAME SYSTEM === */

static void game_init(void) {
    g.mode = MODE_TITLE;
    g.deaths = 0;
    g.frames = 0; g.seconds = 0; g.minutes = 0;
    g.victory_flag = false;
    g.start_game = false;
    g.start_game_flash = 0;
    g.max_y = 99999;
    title_frames = 0;
    alt_shown = -1;
    alt_timer = 0;
    audio_music_play(40, 0);
}

static void begin_game(void) {
    g.mode = MODE_PLAY;
    g.frames = 0; g.seconds = 0; g.minutes = 0;
    g.deaths = 0;
    g.start_game = false;

    g.respawn_idx = START_LVL_IDX;
    float start_x = checkpoints[START_LVL_IDX].x * 8;
    float start_y = checkpoints[START_LVL_IDX].y * 8;
    player_init(start_x, start_y, 1);

    g.cam_x = 0;
    g.cam_y = (int)start_y - SCREEN_H/2;
    g.max_y = 99999;

    alt_shown = -1;
    alt_timer = 0;
    audio_music_play(0, 0);
}

static void update_camera(void) {
    const Player* p = player_get();
    int target_x = (int)p->x - SCREEN_W/2 + 4;
    int target_y = (int)p->y - SCREEN_H/2 + 4;

    if (target_x < 0) target_x = 0;
    if (target_y < 0) target_y = 0;

    int max_map_y = level_height() - SCREEN_H;
    if (target_y > max_map_y) target_y = max_map_y;

    /* Smooth Lerp */
    g.cam_x += (target_x - g.cam_x) / 8;
    g.cam_y += (target_y - g.cam_y) / 8;
}

static void update_game(void) {
    /* Title screen logic */
    if (g.mode == MODE_TITLE) {
        title_frames++;
        if (!g.start_game && (input_jump() || input_dash())) {
            g.start_game = true;
            g.start_game_flash = 50;
        }
        if (g.start_game) {
            g.start_game_flash--;
            if (g.start_game_flash <= -30) {
                begin_game();
            }
        }
        return;
    }

    const Player* p = player_get();

    /* Input Debug */
    static bool p_teleport = false;
    bool k_teleport = input_teleport();

    if (k_teleport && !p_teleport) {
        g.respawn_idx = (g.respawn_idx + 1) % NUM_CHECKPOINTS;
        float px = checkpoints[g.respawn_idx].x * 8;
        float py = checkpoints[g.respawn_idx].y * 8;
        level_reset_on_death();
        player_respawn(px, py);
        g.max_y = (int)py;
    }
    p_teleport = k_teleport;

    if (g.mode == MODE_PLAY) {
        /* Timer (freeze once flag is reached) */
        if (!g.victory_flag) {
            g.frames++;
            if (g.frames >= 30) {
                g.frames = 0; g.seconds++;
                if (g.seconds >= 60) { g.seconds = 0; g.minutes++; }
            }
        }

        level_update(player_get_mut());

        bool died = player_update(g.frames);
        if (died) {
            kill_player();
            return;
        }

        /* Refresh player pointer after update */
        p = player_get();

        /* Stats */
        if ((int)p->y < g.max_y) g.max_y = (int)p->y;

        /* Altitude markers: check if player passed a new checkpoint Y threshold */
        for (int i = alt_shown + 1; i < (int)NUM_CHECKPOINTS; i++) {
            if (g.max_y <= checkpoints[i].y * 8) {
                alt_shown = i;
                alt_timer = ALT_HOLD;
                alt_fall_y = 0;
                alt_fall_spd = 0;
                if (i == (int)NUM_CHECKPOINTS - 1) {
                    snprintf(alt_text, sizeof(alt_text), "summit");
                } else {
                    snprintf(alt_text, sizeof(alt_text), "%d m", (i + 1) * 100);
                }
            } else {
                break;
            }
        }

        /* Summit Flag Check (Tile 118 @ 282, 6) */
        if (!g.victory_flag && p->x > 282*8 && p->x < 283*8 && p->y < 8*8) {
            g.victory_flag = true;
            vfx_shake = 10;
        }

        /* Restart from victory */
        if (g.victory_flag) {
            static bool p_restart = false;
            bool k_restart = input_restart();
            if (k_restart && !p_restart) {
                level_init();
                vfx_init();
                game_init();
                memorial_idx = 0;
                begin_game();
                p_restart = false;
                return;
            }
            p_restart = k_restart;
        }
    }
}

/* === DRAWING === */

/* Draw time in HH:MM:SS format */
static void draw_time(int x, int y) {
    int s = g.seconds;
    int m = g.minutes % 60;
    int h = g.minutes / 60;

    /* Format: HH:MM:SS */
    char str[16];
    /* Draw background */
    gfx_rectfill(x, y, x + 32, y + 6, 0);

    /* Manual formatting for 2-digit display */
    str[0] = '0' + (h / 10) % 10;
    str[1] = '0' + h % 10;
    str[2] = ':';
    str[3] = '0' + m / 10;
    str[4] = '0' + m % 10;
    str[5] = ':';
    str[6] = '0' + s / 10;
    str[7] = '0' + s % 10;
    str[8] = '\0';

    gfx_print(x + 1, y + 1, str, 7);
}

static void draw_hud(void) {
    if (g.victory_flag) {
        /* Victory overlay (player keeps moving) */
        int box_x = SCREEN_W/2 - 30, box_y = 0;
        gfx_rectfill(box_x, box_y, box_x + 60, box_y + 28, 0);

        /* Strawberries */
        gfx_spr(26, box_x + 12, box_y + 2, false, false);
        gfx_print(box_x + 21, box_y + 4, "x", 7);
        gfx_print_num(box_x + 25, box_y + 4, level_strawberry_collected(), 7);

        draw_time(box_x + 12, box_y + 12);

        gfx_print(box_x + 13, box_y + 22, "DEATHS:", 7);
        gfx_print_num(box_x + 42, box_y + 22, g.deaths, 7);

        /* Hint at bottom of screen */
        gfx_print(SCREEN_W/2 - 36, SCREEN_H - 8, "Q=QUIT; R=RESTART", 5);
    }
}

/* Mountain logo: 7x4 sprites from the PICO-8 sheet (rows 4-7, cols 9-15) */
static const uint8_t logo_sprites[4][7] = {
    { 73,  74,  75,  76,  77,  78,  79},
    { 89,  90,  91,  92,  93,  94,  95},
    {105, 106, 107, 108, 109, 110, 111},
    {121, 122, 123, 124, 125, 126, 127},
};

static void draw_title(void) {
    gfx_clear(0);

    /* Start-game flash: palette fade */
    if (g.start_game) {
        int c = -1; /* -1 = no remap */
        if (g.start_game_flash > 10) {
            if (title_frames % 10 < 5) c = 7; /* white flash */
        } else if (g.start_game_flash > 5) {
            c = 2;  /* dark blue */
        } else if (g.start_game_flash > 0) {
            c = 1;  /* dark navy */
        } else {
            c = 0;  /* black */
        }
        if (c >= 0) {
            gfx_pal(6, c); gfx_pal(12, c); gfx_pal(13, c);
            gfx_pal(5, c); gfx_pal(1, c);  gfx_pal(7, c);
        }
    }

    /* Clouds (dark blue shapes drifting across the full width) */
    vfx_draw_bg();

    /* Mountain logo centered on 256-wide screen */
    int logo_x = (SCREEN_W - 7 * 8) / 2 - 4; /* -4 offset */
    int logo_y = 24;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 7; col++) {
            gfx_spr(logo_sprites[row][col],
                     logo_x + col * 8, logo_y + row * 8, false, false);
        }
    }

    /* Snow */
    vfx_draw_fg(0, 0);

    /* Credits text (centered on 256-wide screen) */
    if (!g.start_game || g.start_game_flash > 0) {
        gfx_print(SCREEN_W/2 - 8,  80, "z+x", 5);
        gfx_print(SCREEN_W/2 - 27, 96, "matt thorson", 5);
        gfx_print(SCREEN_W/2 - 22, 102, "noel berry", 5);
    }

    gfx_pal_reset();
    gfx_flip();
}

static void draw_game(void) {
    const Player* p = player_get();

    gfx_clear(COL_BLACK);

    int shake_x = 0, shake_y = 0;
    vfx_get_shake_offset(&shake_x, &shake_y);
    int dx = g.cam_x + shake_x;
    int dy = g.cam_y + shake_y;

    vfx_draw_bg();
    level_draw(dx, dy);
    vfx_orb_draw(dx, dy);

    /* Draw Player */
    player_draw(dx, dy, g.frames);

    /* Draw trailing Key */
    if (level_has_key()) {
        int kx = (int)p->x + (p->flip_x ? 6 : -6) - dx;
        int ky = (int)p->y - 8 - dy + (int)(p8sin(g.frames/15.0f)*3.0f);
        gfx_spr(8, kx, ky, false, false);
    }

    vfx_draw_fg(dx, dy);

    /* Memorial text overlay */
    {
        int mpx = MEMORIAL_TX * 8;  /* Memorial pixel coords */
        int mpy = MEMORIAL_TY * 8;
        bool near = (p->x > mpx - 12 && p->x < mpx + 20 &&
                     p->y > mpy - 8  && p->y < mpy + 8);
        if (near) {
            if (memorial_idx < (int)sizeof(memorial_text) - 1)
                memorial_idx += 0.5f;
            int cx = 8, cy = 96;
            for (int i = 0; i < (int)memorial_idx && memorial_text[i]; i++) {
                if (memorial_text[i] == '#') {
                    cx = 8; cy += 7;
                } else {
                    gfx_rectfill(cx - 2, cy - 2, cx + 7, cy + 6, 7);
                    char ch[2] = { memorial_text[i], 0 };
                    gfx_print(cx, cy, ch, 0);
                    cx += 5;
                }
            }
        } else {
            memorial_idx = 0;
        }
    }

    /* Altitude marker notification (bottom-left, falls off screen) */
    if (alt_timer > 0 || alt_fall_y < SCREEN_H) {
        if (alt_timer > 0) {
            alt_timer--;
        } else {
            alt_fall_spd += ALT_GRAVITY;
            alt_fall_y += alt_fall_spd;
        }
        int ay = SCREEN_H - 14 + (int)alt_fall_y;
        if (ay < SCREEN_H) {
            int tw = gfx_print(0, -20, alt_text, 7); /* measure width off-screen */
            gfx_rectfill(2, ay, 6 + tw, ay + 9, 0);
            gfx_print(4, ay + 2, alt_text, 7);
        }
    }

    draw_hud();

    /* Summit narrowing effect: black bars grow as player approaches the flag.
     * Adapted from ccleste's level_index()==30 camera effect. */
    {
        float flag_y  = 6.0f * 8;   /* Flag tile Y in pixels */
        float start_y = 26.0f * 8;  /* Begin narrowing ~20 tiles below flag */
        if (p->y < start_y) {
            float t = (start_y - p->y) / (start_y - flag_y);
            if (t > 1.0f) t = 1.0f;
            int bar_w = (int)(t * 48); /* max 48px per side (~19% of 256) */
            if (bar_w > 0) {
                gfx_rectfill(0, 0, bar_w - 1, SCREEN_H - 1, 0);
                gfx_rectfill(SCREEN_W - bar_w, 0, SCREEN_W - 1, SCREEN_H - 1, 0);
            }
        }
    }

#if SHOW_DEBUG
    gfx_rect(0,0,60,10,0);
    gfx_print_num(2,2, (int)p->x/8, 6);
    gfx_print_num(30,2, (int)p->y/8, 6);
#endif

    gfx_flip();
}

int celeste_game_main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (gfx_init() != 0) return 1;
    audio_init(22050);
    if (gfx_load_sprites("data/gfx.bmp") != 0) { audio_stop_all(); gfx_quit(); return 1; }

    level_init();
    vfx_init();
    game_init();

    rg_app_t *app = rg_system_get_app();
    if (app && (app->bootFlags & RG_BOOT_RESUME)) {
        rg_emu_load_state(app->saveSlot);
    }

    int64_t last_tick = rg_system_timer();
    const int frame_time_us = 1000000 / TARGET_FPS;

    extern bool celeste_is_running(void);

    while (celeste_is_running()) {
        int64_t frame_start = rg_system_timer();
        rg_system_tick(frame_start - last_tick);
        last_tick = frame_start;

        input_update();
        if (sfx_timer > 0) sfx_timer--;

        /* Freeze Frame Logic */
        vfx_update();
        if (vfx_freeze > 0) {
            draw_game();
        } else {
            update_game();
            if (g.mode == MODE_TITLE) {
                draw_title();
            } else {
                update_camera();
                draw_game();
            }
        }

        int64_t frame_end = rg_system_timer();
        int64_t elapsed = frame_end - frame_start;
        if (elapsed < frame_time_us) {
            rg_task_delay((frame_time_us - elapsed) / 1000);
        }
    }

    audio_deinit();
    gfx_quit();
    return 0;
}

/* === SAVE STATE SUPPORT === */

typedef struct {
    Game g;
    float memorial_idx;
    int title_frames;
    int alt_shown;
    int alt_timer;
    float alt_fall_y;
    float alt_fall_spd;
    char alt_text[16];
} GameState;

size_t game_get_state_size(void) {
    return sizeof(GameState);
}

void game_get_state(void *dest) {
    GameState *s = (GameState *)dest;
    s->g = g;
    s->memorial_idx = memorial_idx;
    s->title_frames = title_frames;
    s->alt_shown = alt_shown;
    s->alt_timer = alt_timer;
    s->alt_fall_y = alt_fall_y;
    s->alt_fall_spd = alt_fall_spd;
    memcpy(s->alt_text, alt_text, sizeof(alt_text));
}

void game_set_state(const void *src) {
    GameState *s = (GameState *)src;
    g = s->g;
    memorial_idx = s->memorial_idx;
    title_frames = s->title_frames;
    alt_shown = s->alt_shown;
    alt_timer = s->alt_timer;
    alt_fall_y = s->alt_fall_y;
    alt_fall_spd = s->alt_fall_spd;
    memcpy(alt_text, s->alt_text, sizeof(alt_text));
}
