/**
 * Scripted controller input (PORT_INPUT_SCRIPT builds only).
 *
 * The emulator can't be driven from the outside in this environment, so
 * interactive play is verified by replaying a canned button sequence: Start
 * on the title screen, then A through the main menu / character select /
 * course select into a real race, then hold A and steer.  Progress (gamestate,
 * menu, race position) is logged and frames are dumped periodically so the
 * run can be checked after the fact.
 */
#ifdef PORT_INPUT_SCRIPT
#include <ultra64.h>
#include <common_structs.h>
#include <macros.h>
#include "port.h"
#include "main.h"
#ifdef PORT_NET
#include "net/port_net.h"
#endif
#include "menus.h"
#include "code_800029B0.h"
#include "cpu_vehicles_camera_path.h"
#include "course.h"
#include <stdio.h>
#include <math.h>
#ifdef PORT_COURSE_TEST
#include <malloc.h>
#include <pspsysmem.h>
extern int texman_usage_percent(void);
extern size_t gFreeMemorySize; extern uintptr_t gNextFreeMemoryAddress, gFreeMemoryResetAnchor;
#endif

typedef struct {
    u32 from;   // first frame (inclusive)
    u32 to;     // last frame (inclusive)
    u16 button;
    s8 stick_x;
    s8 stick_y;
} ScriptStep;

/* A "tap" is 6 frames held.  Menus react to button *presses* (edges), so
 * every tap must be separated by released frames. */
#define TAP(f, b) { (f), (f) + 5, (b), 0, 0 }

static const ScriptStep sSteps[] = {
#if defined(PORT_NET_FILE) && defined(PORT_NET_RECOUNT_TEST)
    /* docs/adhoc.md "Another number of players": in the 2P session the host
     * backs out of the character select to the game select, picks 3P GAME and
     * confirms.  The session must end on both machines in that frame: the host
     * goes to WAITING FOR 2 MORE PLAYERS, the joiner gets HOST DISCONNECTED,
     * dismisses it and joins the new race (see the joiner's taps below). */
    TAP(660, A_BUTTON),  // the 2P OK: the lobby (the clock stops), then the session
    TAP(780, B_BUTTON),  // character select -> game select (OK)
    TAP(880, B_BUTTON),  // -> class
    TAP(910, B_BUTTON),  // -> mode
    TAP(940, B_BUTTON),  // -> number of players
    TAP(970, R_JPAD),    // 3P GAME
    TAP(1000, A_BUTTON), // mode (VS)
    TAP(1030, A_BUTTON), // class
    TAP(1060, A_BUTTON), // -> OK
    TAP(1120, A_BUTTON), // OK: another number of players
    { 640, 1000000, 0, 0, 0 },
#endif
#if defined(PORT_NET_FILE) && defined(PORT_NET_LEAVE_TEST)
    /* docs/adhoc.md "LEAVE MULTIPLAYER": PORT_NET_LEAVE_TEST=1 the joiner, =2
     * the host pauses the 2P GP race (the START below; the other machine's is
     * masked), goes down to the third line and confirms.  shot1650 is the
     * pause menu, shot1800 the prompt on the machine left behind, whose A at
     * 1900 picks CONTINUE (after the joiner left) / MAIN MENU (the host). */
    TAP(1600, D_JPAD),
    TAP(1630, D_JPAD),
    TAP(1670, A_BUTTON),
    TAP(1900, A_BUTTON),
    { 1566, 1000000, 0, 0, 0 },
#endif
#ifndef PORT_STRAIGHT_RACE
    TAP(1560, START_BUTTON), // pause mid-race (first so it wins over the held-A steps); shot1590/1620 show the pause screen
    { 1566, 1640, 0, 0, 0 }, // ...and release everything: a new A press would pick "CONTINUE GAME"
#endif
    TAP(240, START_BUTTON),  // title screen -> main menu
    // Main menu: 1P -> Mario GP -> 50cc -> OK (defaults; each A advances)
    TAP(400, A_BUTTON),  // leaves the title; the game select appears ~12 frames later
#ifdef PORT_NET_FILE
    TAP(440, R_JPAD),    // 2P GAME (the two-PPSSPP lockstep test: one pad per machine)
#endif
    TAP(470, A_BUTTON),  // (shot450 shows its top level with the L OPTION / R DATA buttons)
    TAP(520, A_BUTTON),
    TAP(580, A_BUTTON),
    // Character select: pick, then OK
    TAP(660, A_BUTTON),
    TAP(720, A_BUTTON),
    // Course select: Mushroom Cup, Luigi Raceway
    TAP(840, A_BUTTON),
    TAP(900, A_BUTTON),
    TAP(960, A_BUTTON),   // skip course intro pan if it is still showing
    TAP(1020, A_BUTTON),
#ifdef PORT_MANUAL_RACE
    // Menus only: the pad is handed over as the race starts (see SCRIPT_END).
};
#define SCRIPT_END 1060
#elif defined(PORT_STRAIGHT_RACE)
    // Issue #10: drive straight ahead from the start (the Rainbow Road kart
    // shadow turns into a solid black box after ~5 s of driving).
    { 1100, 3200, A_BUTTON, 0, 0 },
};
#define SCRIPT_END 3300
#else
    // Race: hold A and keep a gentle right-hand line (Luigi Raceway runs
    // clockwise), with one hop/drift to exercise R.
    { 1100, 1520, A_BUTTON, -80, 0 },  // steer hard left into the rail (issue #10: the kart shadow goes solid there)
    { 1521, 1600, A_BUTTON | R_TRIG, -100, 0 },  // hard drift turn (behind-camera regression check)
    { 1601, 1700, A_BUTTON, 30, 0 },
    { 1301, 1360, A_BUTTON, -40, 0 },
    { 1361, 1700, A_BUTTON, 30, 0 },
    { 1701, 1720, A_BUTTON | R_TRIG, 60, 0 }, // hop/drift
    { 1721, 1900, A_BUTTON, 25, 0 },
    { 1901, 1990, A_BUTTON, -80, 0 },   // sharp left turn away from the wall
    { 1991, 2080, A_BUTTON, 80, 0 },    // sharp right
    { 2081, 3200, A_BUTTON, 25, 0 },
};
#define SCRIPT_END 3300
#endif
#define SHOT_EVERY 120

#ifdef PORT_COURSE_TEST
s32 gPortForceCourse = -1;
#endif

void port_input_script(OSContPad* pad) {
    static u32 sFrame = 0;

#ifdef PORT_COURSE_TEST
    if (sFrame == 0) {
        FILE* fcp = fopen(port_save_path("testcourse.bin"), "rb");
        if (fcp) { int c = fgetc(fcp); fclose(fcp); gPortForceCourse = c; PORT_LOG("TEST: forcing course %d\n", c); }
    }
    if (gGamestate == RACING && (sFrame % 30) == 0) {
        PORT_LOG("TEST: RACING course %d frame %u\n", gCurrentCourseId, sFrame);
    }
#ifdef PORT_SPARK_DEBUG
    {
        static int sbDone = 0;
        if (gGamestate == RACING && gPlayerOne != NULL && !sbDone && gPlayerOne->speed > 0.1f) {
            gPlayerOne->effects |= 0x2000;      // MUSHROOM_EFFECT (exactly like func_8002A704)
            gPlayerOne->boostTimer = 0x0050;
            PORT_LOG("SBTEST: mimic start-boost (MUSHROOM_EFFECT+boostTimer) at frame %u\n", sFrame);
            sbDone = 1;
        }
    }
#endif

    {
        static int memLogged = 0;
        if (gGamestate == RACING && !memLogged && sFrame > 1150) {
            struct mallinfo mi = mallinfo();
            unsigned poolUsed = (unsigned)(gNextFreeMemoryAddress - gFreeMemoryResetAnchor);
            PORT_LOG("MEM course %d: pool %uKB/%uKB | heap used %uKB free %uKB arena %uKB | maxfree %uKB | tex %d%%\n",
                     gCurrentCourseId, poolUsed/1024u, (unsigned)gFreeMemorySize/1024u,
                     (unsigned)mi.uordblks/1024u, (unsigned)mi.fordblks/1024u, (unsigned)mi.arena/1024u,
                     (unsigned)sceKernelMaxFreeMemSize()/1024u, texman_usage_percent());
            memLogged = 1;
        }
    }
#endif
    static s32 sLastState = -2, sLastMenu = -2;
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sSteps); i++) {
        if (sFrame >= sSteps[i].from && sFrame <= sSteps[i].to) {
            pad->button = sSteps[i].button;
            pad->stick_x = sSteps[i].stick_x;
            pad->stick_y = sSteps[i].stick_y;
            break;
        }
    }
#ifdef PORT_FINISH_TEST
    if (sFrame >= 1100 && sFrame < 1700) { pad->button = A_BUTTON; pad->stick_x = 0; pad->stick_y = 0; } /* straight over the line */
    else if (sFrame >= 1700) { pad->button = 0; pad->stick_x = 0; pad->stick_y = 0; }                  /* then hands off: no pause, no menu presses */
#endif
#ifdef PORT_NET
    /* Both machines run this script and both navigate their own menus to the
     * same race before the lobby; the lobby waits without input (netrole.bin
     * picks HOST/JOIN) and the script's clock stops with it.  Once in a
     * session the menu navigation is the host's alone -- the game counts a
     * Right on two pads as two presses. */
    if (port_net_lobby_active()) {
        pad->button = 0; pad->stick_x = pad->stick_y = 0;
#ifdef PORT_NET_RECOUNT_TEST
        { static int n; if (sFrame >= 700 && ++n == 120) port_screenshot(7100); } /* the second lobby */
#endif
        return;
    }
#ifdef PORT_NET_RECOUNT_TEST
    {
        static int sJoiner, sCalls;
        if (port_net_active() && port_net_local_slot() != 0) sJoiner = 1;
        if (sJoiner && sFrame >= 670) {
            pad->button = 0; pad->stick_x = pad->stick_y = 0;
            if (port_net_modal_active() || !port_net_active()) { /* the prompt, then the game select it leaves */
                sCalls++;
                if (sCalls == 60) port_screenshot(7000);
                if (sCalls > 90 && (sCalls % 40) < 6) pad->button = A_BUTTON;
            }
        }
    }
#endif
    if (port_net_active() && port_net_local_slot() != 0) {
        pad->button &= ~(R_JPAD | L_JPAD);
    }
#ifdef PORT_NET_LEAVE_TEST
    {
        static int sSlot = -1;
        if (port_net_active()) sSlot = port_net_local_slot();
        if (sSlot != (PORT_NET_LEAVE_TEST == 1 ? 1 : 0) && sFrame < 1700) pad->button &= ~START_BUTTON; /* the leaver pauses */
        if (sFrame > 1680 && sFrame < 1900 && (sFrame % 20) == 0 && (sFrame % SHOT_EVERY) != 0) { /* two dumps in one frame: the second is black */ port_screenshot((int) sFrame); PORT_LOG("script f%u: shot, gamestate %d paused %d modal %d\n", sFrame, gGamestate, gIsGamePaused, port_net_modal_active()); }
    }
#endif
#endif

#ifdef PORT_STRAIGHT_RACE
    /* Issue #10 neon signs: drop the kart onto the track a little before a
     * sign, facing along the path (yaw: forward is (sin(-yaw), cos(-yaw)),
     * 0x8000 = -z as on the start line).  The screenshots every 15 frames
     * then show the sign as it is passed. */
    if (gGamestate == RACING && gPlayerOne != NULL) {
        static const struct { u32 frame; f32 x, y, z, dx, dz; const char* sign; } sWarps[] = {
            { 1400, -3333.0f, 743.0f, 2211.0f, -0.18f, 0.98f, "DK" },
            { 1500, -3310.0f, 743.0f, 2705.0f, 0.80f, 0.61f, "Yoshi" },
            { 1600, 1184.0f, 999.0f, -5074.0f, 0.31f, 0.95f, "Luigi" },
            { 1700, 1825.0f, 791.0f, 2149.0f, 0.61f, 0.79f, "Toad" },
        };
        u32 w;
        for (w = 0; w < ARRAY_COUNT(sWarps); w++) {
            if (sFrame == sWarps[w].frame) {
                extern f32 get_surface_height(f32 posX, f32 posY, f32 posZ);
                Player* p = gPlayerOne;
                p->pos[0] = p->oldPos[0] = sWarps[w].x;
                p->pos[2] = p->oldPos[2] = sWarps[w].z;
                /* as spawn_player: land on the surface.  (Do not reset the collision mesh
                 * indices to 5000 here: the kart then hangs the collision search.  The kart
                 * usually still falls off after a warp -- the sign is visible either way.) */
                p->pos[1] = p->oldPos[1] = get_surface_height(sWarps[w].x, sWarps[w].y + 50.0f, sWarps[w].z) + p->boundingBoxSize;
                p->velocity[0] = p->velocity[1] = p->velocity[2] = 0.0f;
                p->rotation[1] = (s16) (u16) (s32) (atan2f(-sWarps[w].dx, sWarps[w].dz) * (65536.0f / 6.2831853f));
                PORT_LOG("script f%u: warp to %s sign approach (%.0f %.0f %.0f) yaw %d\n", sFrame, sWarps[w].sign, p->pos[0], p->pos[1], p->pos[2], p->rotation[1]);
            }
        }
    }
#endif
    if (gGamestate != sLastState || gMenuSelection != sLastMenu) {
        PORT_LOG("script f%u: gamestate %d menu %d (course %d)\n", sFrame, gGamestate, gMenuSelection,
                 gCurrentCourseId);
        sLastState = gGamestate;
        sLastMenu = gMenuSelection;
    }
    if (((sFrame % SHOT_EVERY) == 0 && sFrame >= 240) || sFrame == 450 /* top-level game select: OPTION/DATA */ || sFrame == 1442 /* the frame traced at 1441 */ || sFrame == 1352 ||
        (sFrame >= 1380 && sFrame <= 1700 && (sFrame % 30) == 0)
#ifdef PORT_STRAIGHT_RACE
        || (sFrame >= 1290 && sFrame <= 1760 && (sFrame % 15) == 0) || sFrame == 1331 || sFrame == 1601
#endif
#ifdef PORT_COURSE_TEST
        || (gPortForceCourse == 9 && sFrame > 1700 && sFrame <= 3000 && (sFrame % 30) == 0) /* the finish and the results screen */
#endif
        ) {
        port_screenshot((int) sFrame);
        if (gGamestate == RACING && gPlayerOne != NULL) {
            PORT_LOG("script f%u: p1 pos %.1f %.1f %.1f speed %.2f lap %d\n", sFrame, gPlayerOne->pos[0],
                     gPlayerOne->pos[1], gPlayerOne->pos[2], gPlayerOne->speed, gLapCountByPlayerId[0]);
        }
    }
    if (sFrame == 1320) {
        // Dump the live player-1 kart palette for comparison with the ROM copy.
        extern u8 gPlayerPalettesList[];
        FILE* fp = fopen(port_save_path("pal_p1.bin"), "wb");
        if (fp) { fwrite(&gPlayerPalettesList[0], 1, 0x200, fp); fclose(fp); }
        PORT_LOG("script: palette dumped, gPlayerPalettesList at %p\n", &gPlayerPalettesList[0]);
    }
    if (sFrame == 480) {
        extern u16* gMenuTextureBuffer;
        FILE* fp = fopen(port_save_path("menubuf.bin"), "wb");
        if (fp) { fwrite(gMenuTextureBuffer, 1, 320 * 240 * 2, fp); fclose(fp); }
        PORT_LOG("script: menu buffer dumped from %p\n", gMenuTextureBuffer);
    }
#ifdef PORT_COURSE_TEST
    if (sFrame == 1443 && gPortForceCourse >= 0) {
        // Dump the kart shadow textures as the GE sees them (issue #10): the
        // two 64x32 8888 halves are texture ids 303/304 on Rainbow Road.
        extern unsigned char* texman_get_tex_data(unsigned int num);
        extern unsigned char texman_get_tex_type(unsigned int num);
        unsigned int id;
        for (id = 300; id <= 306; id++) {
            char name[48];
            FILE* fp;
            const unsigned char* d = texman_get_tex_data(id);
            snprintf(name, sizeof(name), "texmem%u_type%u.bin", id, texman_get_tex_type(id));
            fp = fopen(port_save_path(name), "wb");
            if (fp) { if (d) fwrite(d, 1, 8192, fp); fclose(fp); }
        }
    }
#ifdef PORT_STRAIGHT_RACE
    if ((sFrame == 1330 || sFrame == 1600) && gPortForceCourse >= 0) {
        // Two traced race frames: early (shadow expected fine) and late (the box).
        extern int gfx_debug_frame, gfx_trace_frames, gfx_colorflush;
        gfx_debug_frame = 1;
        gfx_trace_frames = 1;
        gfx_colorflush = (sFrame == 1600); /* shot1601 shows every batch in its own colour */
    }
#endif
    /* Debug warps (frame 1400): drop the kart onto the track facing a feature,
     * then hold A with the stick centred so the 1410.. screenshots show it.
     *   14 Wario Stadium / 8 Luigi Raceway: the stadium TV screen (issue #11)
     *   7 Royal Raceway: the dash pad ramp (issue #13)
     *   9 Moo Moo Farm: the last stretch with two laps already counted, so the
     *     race ends within seconds and the results screen follows (issue #14)
     * yaw = atan2(-dx, dz) * 65536 / 2pi, precomputed (0x8000 = -z as on the start line). */
    {
        static const struct { s32 course; f32 x, y, z; s16 yaw; u32 holdA; s32 laps; } sWarps[] = {
            { 14, -1356.0f, -69.0f, 345.0f, 0, 1470, -1 },
            { 8, -1242.0f, -53.0f, -1879.0f, -10423, 1470, -1 },
            { 7, 1417.0f, 0.0f, -2387.0f, 11429, 1600, -1 },
            { 9, 147.0f, 0.0f, 782.0f, 28537, 1700, 2 },
        };
        u32 w;
        for (w = 0; w < ARRAY_COUNT(sWarps); w++) {
            if (sWarps[w].course != gPortForceCourse || gGamestate != RACING || gPlayerOne == NULL) continue;
            if (sFrame == 1400) {
                extern f32 get_surface_height(f32 posX, f32 posY, f32 posZ);
                Player* p = gPlayerOne;
                p->pos[0] = p->oldPos[0] = sWarps[w].x;
                p->pos[2] = p->oldPos[2] = sWarps[w].z;
                p->pos[1] = p->oldPos[1] = get_surface_height(sWarps[w].x, sWarps[w].y + 50.0f, sWarps[w].z) + p->boundingBoxSize;
                p->velocity[0] = p->velocity[1] = p->velocity[2] = 0.0f;
                p->rotation[1] = sWarps[w].yaw;
                if (sWarps[w].laps >= 0) gLapCountByPlayerId[0] = sWarps[w].laps;
                PORT_LOG("script f%u: warp (%.0f %.0f %.0f) yaw %d laps %d\n", sFrame, p->pos[0], p->pos[1], p->pos[2], p->rotation[1], gLapCountByPlayerId[0]);
            }
            if (sFrame >= 1400 && sFrame <= 1700) {
                pad->button = (sFrame <= sWarps[w].holdA) ? A_BUTTON : 0;
                pad->stick_x = pad->stick_y = 0;
            }
        }
    }
    /* Issue #15: on Mario Raceway (0) give the kart a star at frame 1450 and
     * trace frame 1500 (the kart tint combiner, the sparkles). */
    if (gPortForceCourse == 0 && gGamestate == RACING && gPlayerOne != NULL) {
        if (sFrame == 1450) {
            gPlayerOne->triggers |= STAR_TRIGGER;
            PORT_LOG("script f%u: star\n", sFrame);
        }
        if (sFrame == 1500) {
            extern int gfx_debug_frame, gfx_trace_frames;
            gfx_debug_frame = 1;
            gfx_trace_frames = 1;
        }
    }
    if (sFrame == 1441 && gPortForceCourse >= 0) {
        // One traced race frame on the forced course (issue #1: Moo Moo Farm road patches).
        extern int gfx_debug_frame, gfx_trace_frames;
        gfx_debug_frame = 1;
        gfx_trace_frames = 1;
    }
#endif
    if (sFrame == 482) { extern int gPortTraceArm; gPortTraceArm = 4; }
#ifdef PORT_FINISH_TEST
    /* Any course: two laps already counted as the race starts, so the first
     * crossing of the line (the karts start just behind it, and the pad is held
     * straight above) ends the race and the
     * results screen follows.  Screenshots every 30 frames from there. */
    {
        static int sLapsSet;
        if (!sLapsSet && gGamestate == RACING && gPlayerOne != NULL && gPlayerOne->speed > 1.0f) { /* rolling: the race is on */
            sLapsSet = 1;
            gLapCountByPlayerId[0] = 2;
            PORT_LOG("script f%u: finish test, laps set to 2\n", sFrame);
        }
    }
    if (sFrame > 1300 && sFrame <= 3200 && (sFrame % 30) == 0 && (sFrame % SHOT_EVERY) != 0) port_screenshot((int) sFrame);
#endif
    if (sFrame == 481) {
        // One fully traced frame of the game-select screen (issue #5: the
        // GAME SELECT banner and the OPTION/DATA buttons do not draw).
        extern int gfx_debug_frame, gfx_trace_frames;
        gfx_debug_frame = 1;
        gfx_trace_frames = 1;
    }
    if (sFrame == SCRIPT_END) {
        PORT_LOG("script done\n");
    }
    if (sFrame > SCRIPT_END) {
        // Manual capture: after the script ends the pad is live; a Triangle
        // (C-up) press traces this frame, screenshots it next frame and dumps
        // the kart shadow textures (issue #10).
        static int armed = 1, shotNext = 0;
        if (shotNext) {
            port_screenshot(9000 + shotNext);
            shotNext = 0;
        }
        if ((pad->button & U_CBUTTONS) && armed) {
            extern int gfx_debug_frame, gfx_trace_frames;
            extern unsigned char* texman_get_tex_data(unsigned int num);
            extern unsigned char texman_get_tex_type(unsigned int num);
            static int n;
            unsigned int id;
            gfx_debug_frame = 1;
            gfx_trace_frames = 1;
            for (id = 1; id < 512; id++) {
                const unsigned char* d = texman_get_tex_data(id);
                if (d && texman_get_tex_type(id) == 3) { /* every 8888 texture (the shadow halves are 64x32 8888) */
                    char name[48]; FILE* fp;
                    snprintf(name, sizeof(name), "cap%d_tex%u.bin", n, id);
                    fp = fopen(port_save_path(name), "wb");
                    if (fp) { fwrite(d, 1, 8192, fp); fclose(fp); }
                }
            }
            PORT_LOG("manual capture %d at frame %u\n", n, sFrame);
            shotNext = ++n;
            armed = 0;
        }
        if (!(pad->button & U_CBUTTONS)) armed = 1;
    }
    sFrame++;
}
#endif
