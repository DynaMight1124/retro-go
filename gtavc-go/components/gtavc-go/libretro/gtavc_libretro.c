/*
 * libretro core wrapper for the GTA Vice City J2ME C port (SF2000 / GB300).
 * The game itself is the direct C translation in src/port; this file only
 * adapts input, video and the frame loop to the froggy multicore frontend.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "libretro.h"
#include "game.h"
#include "world.h"
#include "anim_data.h"
#include "snd.h"
#include "j2me.h"

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_sample_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static uint16_t g_video[320 * 240];
static int16_t g_audio[4410 * 2];
static bool g_loaded = false;
static bool g_prev[16];
static uint32_t g_lastTick;
static bool g_stateLogged;
static int g_lastState = -1;
static int g_loadLogFrame;
static int g_missionState = -1;
static int g_tileSeq = 0;
static int g_installSeq = 0;

extern int Tile_lastSeq;
extern int Tile_lastMs;
extern int Tile_lastCache;
extern int Tile_lastI;
extern int Tile_lastJ;
extern int Tile_installSeq;
extern int Tile_installMs;
extern int Tile_installTex;
extern int Tile_installI;
extern int Tile_installJ;

static char g_dir[512];
static char g_logpath[600];

static void gtavc_log(const char* s) {
#ifdef SF2000
    if (!g_logpath[0]) return;
    FILE* f = fopen(g_logpath, "a");
    if (!f) return;
    fputs(s, f);
    fputc('\n', f);
    fclose(f);
#else
    (void)s;
#endif
}

static int file_ok(const char* dir, const char* name) {
    char p[700];
    snprintf(p, sizeof(p), "%s/%s", dir, name);
    FILE* f = fopen(p, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static void make_dir(const char* path) {
    if (!path || !*path) { g_dir[0] = 0; return; }
    snprintf(g_dir, sizeof(g_dir), "%s", path);
    char* slash = strrchr(g_dir, '/');
    if (slash) *slash = 0;
    else g_dir[0] = 0;

    /* The launcher may hand us the stub in ROMS/ instead of the real
       ROMS/gtavc/ directory.  Resolve the asset directory by looking for
       mapidx.bin next to the file, then in a gtavc/ subdirectory and one
       level up. */
    if (!g_dir[0]) {
        if (file_ok("ROMS/gtavc", "mapidx.bin")) snprintf(g_dir, sizeof(g_dir), "ROMS/gtavc");
        else if (file_ok("/ROMS/gtavc", "mapidx.bin")) snprintf(g_dir, sizeof(g_dir), "/ROMS/gtavc");
    }
    if (g_dir[0] && !file_ok(g_dir, "mapidx.bin")) {
        char cand[700];
        snprintf(cand, sizeof(cand), "%s/gtavc", g_dir);
        if (file_ok(cand, "mapidx.bin")) {
            snprintf(g_dir, sizeof(g_dir), "%s", cand);
        } else {
            const char* tries[6];
            char c1[700], c2[700], c3[700], c4[700];
            snprintf(c1, sizeof(c1), "%s/../gtavc", g_dir);
            snprintf(c2, sizeof(c2), "%s/../../ROMS/gtavc", g_dir);
            snprintf(c3, sizeof(c3), "%s/../../../ROMS/gtavc", g_dir);
            snprintf(c4, sizeof(c4), "%s/../../../../ROMS/gtavc", g_dir);
            tries[0] = c1; tries[1] = c2; tries[2] = c3; tries[3] = c4;
            tries[4] = "ROMS/gtavc"; tries[5] = "/ROMS/gtavc";
            for (int ti = 0; ti < 6; ti++) {
                if (file_ok(tries[ti], "mapidx.bin")) {
                    snprintf(g_dir, sizeof(g_dir), "%s", tries[ti]);
                    break;
                }
            }
        }
    }
    snprintf(g_logpath, sizeof(g_logpath), "%s/gtavc.log", g_dir);
}

void retro_set_environment(retro_environment_t cb) { environ_cb = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_sample_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

unsigned retro_api_version(void) { return RETRO_API_VERSION; }
unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }
void* retro_get_memory_data(unsigned id) { (void)id; return NULL; }
size_t retro_get_memory_size(unsigned id) { (void)id; return 0; }

void retro_get_system_info(struct retro_system_info* info) {
    memset(info, 0, sizeof(*info));
    info->library_name = "GTA Vice City";
    info->library_version = "c-port";
    info->valid_extensions = "gb3k|sf2k";
    info->need_fullpath = true;
    info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info* info) {
    memset(info, 0, sizeof(*info));
    info->geometry.base_width = 320;
    info->geometry.base_height = 240;
    info->geometry.max_width = 320;
    info->geometry.max_height = 240;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
    info->timing.fps = 60.0;
    info->timing.sample_rate = 44100.0;
}

void retro_init(void) {}
void retro_deinit(void) {
    if (GTA_game) {
        Game_stop(GTA_game);
        Game_free(GTA_game);
    }
    g_loaded = false;
}

bool retro_load_game(const struct retro_game_info* game) {
    if (!game || !game->path) return false;
    make_dir(game->path);
    gtavc_log("load_game");
    {
        char b[800];
        snprintf(b, sizeof(b), "assets=%s mapidx=%d p_player=%d anim=%d",
                 g_dir[0] ? g_dir : "assets",
                 file_ok(g_dir[0] ? g_dir : "assets", "mapidx.bin"),
                 file_ok(g_dir[0] ? g_dir : "assets", "p_player.bin"),
                 file_ok(g_dir[0] ? g_dir : "assets", "anim.bin"));
        gtavc_log(b);
    }
    platform_set_assets_dir(g_dir[0] ? g_dir : "assets");
    platform_set_data_dir(g_dir[0] ? g_dir : ".");
    platform_fast_mode = 1;
    platform_frame_limit = 1;

    GTA_game = Game_new();
    if (!GTA_game) { gtavc_log("Game_new failed"); return false; }
    Game_start(GTA_game);

    if (environ_cb) {
        enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
        environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);
    }
    memset(g_prev, 0, sizeof(g_prev));
    g_lastTick = (uint32_t)System_currentTimeMillis();
    g_stateLogged = false;
    g_lastState = -1;
    g_loadLogFrame = 0;
    g_loaded = true;
    return true;
}

void retro_unload_game(void) {
    if (GTA_game) {
        Game_stop(GTA_game);
        Game_free(GTA_game);
    }
    g_loaded = false;
}
void retro_reset(void) { }
void retro_set_controller_port_device(unsigned port, unsigned device) { (void)port; (void)device; }
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void* data, size_t size) { (void)data; (void)size; return false; }
bool retro_unserialize(const void* data, size_t size) { (void)data; (void)size; return false; }
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char* code) { (void)index; (void)enabled; (void)code; }
bool retro_load_game_special(unsigned type, const struct retro_game_info* info, size_t num) {
    (void)type; (void)info; (void)num; return false;
}

static uint16_t argb_to_565(uint32_t c) {
    return (uint16_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F));
}

static int key_for_id(unsigned id) {
    switch (id) {
    case RETRO_DEVICE_ID_JOYPAD_UP:     return KEY_UP;
    case RETRO_DEVICE_ID_JOYPAD_DOWN:   return KEY_DOWN;
    case RETRO_DEVICE_ID_JOYPAD_LEFT:   return KEY_LEFT;
    case RETRO_DEVICE_ID_JOYPAD_RIGHT:  return KEY_RIGHT;
    /* Switch-style layout: A sprint/accelerate, B attack/brake,
       X jump/handbrake, Y enter/exit.  L/R are the shoulder buttons:
       camera on foot, handbrake/look-behind while driving. */
    case RETRO_DEVICE_ID_JOYPAD_A:      return KEY_NUM1;    /* sprint / accelerate */
    case RETRO_DEVICE_ID_JOYPAD_B:      return KEY_NUM5;    /* attack / brake */
    case RETRO_DEVICE_ID_JOYPAD_X:      return KEY_NUM0;    /* jump / handbrake */
    case RETRO_DEVICE_ID_JOYPAD_Y:      return KEY_STAR;    /* enter / exit car */
    case RETRO_DEVICE_ID_JOYPAD_L:      return KEY_NUM4;    /* camera left / handbrake */
    case RETRO_DEVICE_ID_JOYPAD_R:      return KEY_NUM6;    /* camera right / look behind */
    case RETRO_DEVICE_ID_JOYPAD_SELECT: return KEY_NUM3;    /* switch weapon */
    case RETRO_DEVICE_ID_JOYPAD_START:  return KEY_POUND;   /* pause */
    default: return 0;
    }
}

void retro_run(void) {
    if (!g_loaded || !GTA_game) return;
    if (input_poll_cb) input_poll_cb();
    if (input_state_cb) {
        for (unsigned id = 0; id < 16; ++id) {
            bool down = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, id) != 0;
            if (down == g_prev[id]) continue;
            g_prev[id] = down;
            int key = key_for_id(id);
            if (!key) continue;
            if (down) Game_keyPressed(GTA_game, key);
            else Game_keyReleased(GTA_game, key);
        }
    }

    uint32_t now = (uint32_t)System_currentTimeMillis();
    int dt = (int)(now - g_lastTick);
    g_lastTick = now;
    /* Audio must follow real time, not a fixed 735 samples per frame.  The
       console often runs below 60 fps; a fixed chunk underruns the sound
       driver and produces the crackle/pops. */
    int audioDt = dt;
    if (audioDt < 10) audioDt = 10;
    if (audioDt > 100) audioDt = 100;
    int audioFrames = audioDt * 44100 / 1000;
    if (audioFrames < 735) audioFrames = 735;
    if (audioFrames > 4410) audioFrames = 4410;
    /* The stock tick is in milliseconds; a minimum keeps the loading screen
       moving even if the tick is not initialised yet. */
    if (dt < 16) dt = 16;
    if (dt > 100) dt = 100;
    Game_frame(GTA_game, dt);
    if (GTA_game->state != g_lastState) {
        char sb[96];
        snprintf(sb, sizeof(sb), "state -> %d (loadI=%d/%d err=%d)", GTA_game->state,
                 GTA_game->loadI, GTA_game->total, GTA_game->loadErr);
        gtavc_log(sb);
        g_lastState = GTA_game->state;
        g_loadLogFrame = 0;
    }
    if (GTA_game->state == GAME_S_LOAD && (++g_loadLogFrame % 30) == 0) {
        char sb[96];
        snprintf(sb, sizeof(sb), "loading %d/%d ms=%d err=%d", GTA_game->loadI,
                 GTA_game->total, GTA_game->loadMs, GTA_game->loadErr);
        gtavc_log(sb);
    }
    if (GTA_game->world) {
        World* w = GTA_game->world;
        if (w->missionState != g_missionState) {
            char mb[192];
            snprintf(mb, sizeof(mb), "mission -> state=%d done=%d text=%s marker=%d,%d player=%d,%d",
                     w->missionState, w->missionDone, w->missionText,
                     w->missionX, w->missionZ, w->player->x, w->player->z);
            gtavc_log(mb);
            g_missionState = w->missionState;
        }
    }
    if (Tile_lastSeq != g_tileSeq) {
        char tb[128];
        snprintf(tb, sizeof(tb), "tile %d_%d cache=%d ms=%d", Tile_lastI, Tile_lastJ,
                 Tile_lastCache, Tile_lastMs);
        gtavc_log(tb);
        g_tileSeq = Tile_lastSeq;
    }
    if (Tile_installSeq != g_installSeq) {
        char ib[128];
        snprintf(ib, sizeof(ib), "install %d_%d tex=%d ms=%d", Tile_installI, Tile_installJ,
                 Tile_installTex, Tile_installMs);
        gtavc_log(ib);
        g_installSeq = Tile_installSeq;
    }
    if (!g_stateLogged && GTA_game->state == GAME_S_PLAY && GTA_game->world) {
        World* w = GTA_game->world;
        int tex = w->playerTex;
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "state: legacy=%d anim=%d pm0=%p playerTex=%d tex0=%p npool=%d nped=%d tiles=%d",
                 GTA_game->legacy, AnimData_n, (void*)w->pm0, w->playerTex,
                 (tex >= 0 && tex < R3D_MAX_TEX) ? (void*)GTA_game->r.T[tex] : (void*)0,
                 w->npool, w->nped, w->tnx * w->tnz);
        gtavc_log(buf);
        g_stateLogged = true;
    }

    int n = GTA_game->W * GTA_game->H;
    if (n > 320 * 240) n = 320 * 240;
    const int* src = GTA_game->scr;
    if (src) {
        for (int i = 0; i < n; ++i) g_video[i] = argb_to_565((uint32_t)src[i]);
    }
    if (video_cb) video_cb(g_video, 320, 240, 320 * sizeof(uint16_t));
    Snd_mix(g_audio, audioFrames, 44100);
    if (audio_batch_cb) {
        for (int off = 0; off < audioFrames; off += 735) {
            int n = audioFrames - off;
            if (n > 735) n = 735;
            audio_batch_cb(g_audio + off * 2, (size_t)n);
        }
    } else if (audio_sample_cb) {
        for (int i = 0; i < audioFrames; ++i) audio_sample_cb(g_audio[i * 2], g_audio[i * 2 + 1]);
    }
}
