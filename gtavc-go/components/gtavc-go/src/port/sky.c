#include "sky.h"
#include "assets.h"
#include "world.h"
#include "fx.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int* Sky_flash = NULL; int* Sky_smoke = NULL; int* Sky_fire = NULL; int* Sky_explo = NULL; int* Sky_tgt = NULL;
int Sky_sFlash = 0, Sky_sSmoke = 0, Sky_sFire = 0, Sky_sExplo = 0, Sky_sTgt = 0;

static int *cloud1 = NULL, *cloud2 = NULL, *cloud3 = NULL, *hilit = NULL;
static int *corona = NULL, *star = NULL, *moon = NULL, *ringa = NULL, *hex_tex = NULL;
static int *circle = NULL, *reflect = NULL, *streek = NULL;

static int sCloud = 0, sCorona = 0, sStar = 0, sMoon = 0, sRing = 0, sHex = 0;
static int sCircle = 0, sReflect = 0, sStreek = 0;

int Sky_sunX = 0, Sky_sunY = 0, Sky_sunZ = 0;

#define NSTAR 70
#define NCLOUD 11

static int starA[NSTAR], starE[NSTAR], starB[NSTAR];
static int cloA[NCLOUD], cloE[NCLOUD], cloS[NCLOUD], cloK[NCLOUD];
static int sunSX = -9999, sunSY = 0, sunVis = 0;

bool Sky_load(void) {
    int dm[2];
    cloud1 = Assets_intensity("/fx_cloud1.png", dm); sCloud = dm[0];
    cloud2 = Assets_intensity("/fx_cloud2.png", dm);
    cloud3 = Assets_intensity("/fx_cloud3.png", dm);
    hilit  = Assets_intensity("/fx_cloudhilit.png", dm);
    corona = Assets_intensity("/fx_corona.png", dm); sCorona = dm[0];
    star   = Assets_intensity("/fx_star.png", dm);   sStar = dm[0];
    moon   = Assets_intensity("/fx_moon.png", dm);   sMoon = dm[0];
    ringa  = Assets_intensity("/fx_ringa.png", dm);  sRing = dm[0];
    hex_tex= Assets_intensity("/fx_hex.png", dm);    sHex = dm[0];
    circle = Assets_intensity("/fx_circle.png", dm); sCircle = dm[0];
    reflect= Assets_intensity("/fx_reflect.png", dm);sReflect = dm[0];
    streek = Assets_intensity("/fx_streek.png", dm); sStreek = dm[0];
    Sky_flash = Assets_intensity("/fx_flash.png", dm); Sky_sFlash = dm[0];
    Sky_smoke = Assets_intensity("/fx_smoke.png", dm); Sky_sSmoke = dm[0];
    Sky_fire  = Assets_intensity("/fx_fire.png", dm);  Sky_sFire = dm[0];
    Sky_explo = Assets_intensity("/fx_explo.png", dm); Sky_sExplo = dm[0];
    Sky_tgt   = Assets_intensity("/fx_target.png", dm);Sky_sTgt = dm[0];

    int seed = 1234567;
    for (int i = 0; i < NSTAR; i++) {
        seed = (int)((uint32_t)seed * 1103515245u + 12345u);
        starA[i] = ((uint32_t)seed >> 8) & 1023;
        seed = (int)((uint32_t)seed * 1103515245u + 12345u);
        starE[i] = 20 + (((uint32_t)seed >> 8) % 210);
        seed = (int)((uint32_t)seed * 1103515245u + 12345u);
        starB[i] = 90 + (((uint32_t)seed >> 8) % 160);
    }
    for (int i = 0; i < NCLOUD; i++) {
        seed = (int)((uint32_t)seed * 1103515245u + 12345u);
        cloA[i] = i * 1024 / NCLOUD + (((uint32_t)seed >> 8) % 50);
        seed = (int)((uint32_t)seed * 1103515245u + 12345u);
        cloE[i] = 8 + (((uint32_t)seed >> 8) % 34);
        seed = (int)((uint32_t)seed * 1103515245u + 12345u);
        cloS[i] = 70 + (((uint32_t)seed >> 8) % 70);
        cloK[i] = i % 3;
    }
    return true;
}

void Sky_sunDir(World* w) {
    int t = w->minutes;
    int th = (t - 360) * 512 / 840;
    Sky_sunX = (Fx_cos(th) * 14500) >> 14;
    Sky_sunY = (Fx_sin(th) * 14500) >> 14;
    Sky_sunZ = -6500;
}

static int s_projX, s_projY, s_projZ;

static IRAM_ATTR bool proj(R3D* r, int dx, int dy, int dz) {
    const int* m = r->mv;
    int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14;
    int vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14;
    int vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
    if (vz < 1200) return false;
    s_projX = r->cx + vx * r->focal / vz;
    s_projY = r->cy - vy * r->focal / vz;
    s_projZ = vz;
    return true;
}

static IRAM_ATTR bool projAngle(R3D* r, int az, int el) {
    int ce = Fx_cos(el);
    return proj(r, (Fx_sin(az) * ce) >> 14, Fx_sin(el), (Fx_cos(az) * ce) >> 14);
}

IRAM_ATTR void Sky_spr(int* fb, int W, int H, const int* tex, int ts, int cx, int cy, int w, int h, int col, int mode, int mul) {
    if (!tex || w < 2 || h < 2 || mul <= 0) return;
    int x0 = cx - w / 2, y0 = cy - h / 2;
    int xs = x0 < 0 ? 0 : x0, ys = y0 < 0 ? 0 : y0;
    int xe = x0 + w > W ? W : x0 + w, ye = y0 + h > H ? H : y0 + h;
    if (xs >= xe || ys >= ye) return;

    int cr = (col >> 16) & 255, cg = (col >> 8) & 255, cb = col & 255;
    int stepX = (ts << 16) / w, stepY = (ts << 16) / h;

    for (int y = ys; y < ye; y++) {
        int ty = ((y - y0) * stepY) >> 16;
        int row = ty * ts, i = y * W + xs, u = (xs - x0) * stepX;
        for (int x = xs; x < xe; x++, i++, u += stepX) {
            int a = tex[row + (u >> 16)];
            if (a < 6) continue;
            a = (a * mul) >> 8;
            int d = fb[i];
            if (mode == 0) {
                int rr = ((d >> 16) & 255) + ((cr * a) >> 8);
                int gg = ((d >> 8) & 255) + ((cg * a) >> 8);
                int bb = (d & 255) + ((cb * a) >> 8);
                fb[i] = ((rr > 255 ? 255 : rr) << 16) |
                        ((gg > 255 ? 255 : gg) << 8) |
                        (bb > 255 ? 255 : bb);
            } else {
                if (a > 255) a = 255;
                fb[i] = Ui_blend(d, col, a);
            }
        }
    }
}

void Sky_flashEffect(R3D* r, int x, int y, int z) {
    int dx = x - r->camX, dy = y - r->camY, dz = z - r->camZ;
    const int* m = r->mv;
    int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14;
    int vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14;
    int vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
    if (vz < 64) return;
    int s = r->focal * 40 / vz + 4;
    Sky_spr(r->fb, r->W, r->H, Sky_flash, Sky_sFlash, r->cx + vx * r->focal / vz, r->cy - vy * r->focal / vz, s, s, 0xFFD890, 0, 256);
}

void Sky_particle(R3D* r, int type, int x, int y, int z, int size, int fade) {
    int dx = x - r->camX, dy = y - r->camY, dz = z - r->camZ;
    const int* m = r->mv;
    int vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
    if (vz < 80 || vz > r->farZ) return;
    int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14;
    int vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14;
    int sx = r->cx + vx * r->focal / vz, sy = r->cy - vy * r->focal / vz;
    int s = size * r->focal / vz;
    if (s < 2) return;
    if (sx >= 0 && sy >= 0 && sx < r->W && sy < r->H && r->zb[sy * r->W + sx] > ((1 << 24) / vz) * 260) return;
    if (type == 0) Sky_spr(r->fb, r->W, r->H, Sky_smoke, Sky_sSmoke, sx, sy, s, s, 0x606060, 1, fade * 3 / 4);
    else if (type == 1) Sky_spr(r->fb, r->W, r->H, Sky_fire, Sky_sFire, sx, sy, s, s * 5 / 4, 0xFF9030, 0, fade);
    else Sky_spr(r->fb, r->W, r->H, Sky_explo, Sky_sExplo, sx, sy, s, s, 0xFFB050, 0, fade);
}

void Sky_target(R3D* r, int x, int y, int z, int hp, int maxHp, int time) {
    int dx = x - r->camX, dy = y - r->camY, dz = z - r->camZ;
    const int* m = r->mv;
    int vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
    if (vz < 80) return;
    int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14;
    int vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14;
    int sx = r->cx + vx * r->focal / vz, sy = r->cy - vy * r->focal / vz;
    int s = r->focal * 70 / vz + 10 + ((Fx_sin(time / 2) * 3) >> 14);
    int denom = maxHp > 1 ? maxHp : 1;
    int f = hp * 255 / denom;
    if (f < 0) f = 0;
    if (f > 255) f = 255;
    int col = f > 128 ? (((255 - f) * 2) << 16) | 0xFF00 : 0xFF0000 | ((f * 2) << 8);
    Sky_spr(r->fb, r->W, r->H, Sky_tgt, Sky_sTgt, sx, sy, s, s, col, 1, 900);
    Sky_spr(r->fb, r->W, r->H, Sky_tgt, Sky_sTgt, sx, sy, s, s, col, 0, 256);
}

IRAM_ATTR int Sky_mix(int a, int b, int t) {
    int ia = 256 - t;
    return (((((a >> 16) & 255) * ia + ((b >> 16) & 255) * t) >> 8) << 16) |
           (((((a >> 8) & 255) * ia + ((b >> 8) & 255) * t) >> 8) << 8) |
           ((((a & 255) * ia + (b & 255) * t) >> 8));
}

void Sky_drawSky(World* w, R3D* r) {
    int W = r->W, H = r->H, f = r->focal;
    int* fb = r->fb;
    int mins = w->minutes;
    bool night = w->night;

    if (night || mins < 6 * 60 + 30 || mins > 19 * 60 + 30) {
        int vis = night ? 256 : 110;
        for (int i = 0; i < NSTAR; i++) {
            if (!projAngle(r, starA[i], starE[i])) continue;
            if (s_projX < 1 || s_projY < 1 || s_projX >= W - 1 || s_projY >= H - 1 || s_projY > r->horizon) continue;
            int tw = starB[i] + ((Fx_sin(w->time / 3 + i * 97) * 40) >> 14);
            int b = (tw * vis) >> 8;
            int k = s_projY * W + s_projX;
            fb[k] = Ui_blend(fb[k], 0xFFFFFF, b > 255 ? 255 : b);
            if (starB[i] > 210) {
                fb[k + 1] = Ui_blend(fb[k + 1], 0xC8D8FF, b / 3);
                fb[k - 1] = Ui_blend(fb[k - 1], 0xC8D8FF, b / 3);
            }
        }
        if (proj(r, -Sky_sunX, -Sky_sunY, -Sky_sunZ) || proj(r, -Sky_sunX, 9000, -Sky_sunZ)) {
            int s = f / 7;
            Sky_spr(fb, W, H, moon, sMoon, s_projX, s_projY, s, s, 0xE8ECFF, 0, 256);
            Sky_spr(fb, W, H, corona, sCorona, s_projX, s_projY, s * 3, s * 3, 0x30384C, 0, 256);
        }
    }

    int skyB = r->skyBot;
    int ccol, cmul, hiCol = 0;
    if (night) {
        ccol = Sky_mix(skyB, 0x6070A0, 60);
        cmul = 140;
    } else if (mins < 8 * 60 || mins > 18 * 60) {
        ccol = Sky_mix(0xFFFFFF, skyB, 110);
        cmul = 210;
        hiCol = mins > 12 * 60 ? 0xFF7830 : 0xFFA060;
    } else {
        ccol = Sky_mix(0xFFFFFF, skyB, 50);
        cmul = 230;
    }

    int drift = w->time / 260;
    for (int i = 0; i < NCLOUD; i++) {
        if (!projAngle(r, (cloA[i] + drift) & 1023, cloE[i])) continue;
        int cw = f * cloS[i] / 100, ch = cw / 2;
        if (s_projX + cw / 2 < 0 || s_projX - cw / 2 >= W || s_projY + ch / 2 < 0 || s_projY - ch / 2 >= H) continue;
        const int* t = cloK[i] == 0 ? cloud1 : (cloK[i] == 1 ? cloud2 : cloud3);
        Sky_spr(fb, W, H, t, sCloud, s_projX, s_projY, cw, ch, ccol, 1, cmul);
        if (hiCol != 0) Sky_spr(fb, W, H, hilit, sCloud, s_projX, s_projY + ch / 6, cw, ch, hiCol, 0, 150);
    }

    sunVis = 0;
    if (Sky_sunY > -1500 && proj(r, Sky_sunX, Sky_sunY, Sky_sunZ)) {
        int low = Sky_sunY < 5000 ? 256 - Sky_sunY * 256 / 5000 : 0;
        int core = Sky_mix(0xFFF6D0, 0xFF8C30, low);
        int glow = Sky_mix(0x80703C, 0xA04818, low);
        Sky_spr(fb, W, H, corona, sCorona, s_projX, s_projY, f * 2 / 3, f * 2 / 3, glow, 0, 256);
        Sky_spr(fb, W, H, corona, sCorona, s_projX, s_projY, f / 5, f / 5, core, 0, 256);
        Sky_spr(fb, W, H, corona, sCorona, s_projX, s_projY, f / 10, f / 10, 0xFFFFFF, 0, 256);
        sunSX = s_projX;
        sunSY = s_projY;
        sunVis = 1;
    }
}

static const int FL_POS[6]  = { 28, 55, 82, 118, 150, 185 };
static const int FL_SIZE[6] = { 9, 5, 14, 7, 20, 11 };
static const int FL_COL[6]  = { 0x5A4020, 0x30483A, 0x203C50, 0x502838, 0x284020, 0x4A3018 };

void Sky_drawPost(World* w, R3D* r) {
    int W = r->W, H = r->H, f = r->focal;
    int* fb = r->fb;
    int* zb = r->zb;

    if (w->night) {
        int dx = (w->lampX >> 4) - r->camX, dy = w->lampY - r->camY, dz = (w->lampZ >> 4) - r->camZ;
        const int* m = r->mv;
        int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14;
        int vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14;
        int vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
        if (vz > 64 && vz < 30000) {
            int sx = r->cx + vx * f / vz, sy = r->cy - vy * f / vz;
            if (sx >= 0 && sy >= 0 && sx < W && sy < H) {
                int iz = ((1 << 24) / vz) << 8;
                if (zb[sy * W + sx] <= iz + (iz >> 4)) {
                    int pulse = 180 + ((Fx_sin(w->time / 8) * 76) >> 14);
                    int s = f * 160 / (vz / 8 + 64) + 8;
                    Sky_spr(fb, W, H, corona, sCorona, sx, sy, s, s, 0xFFE8A0, 0, pulse);
                    Sky_spr(fb, W, H, star, sStar, sx, sy, s * 2, s * 2, 0x806838, 0, pulse);
                }
            }
        }
    }

    if (sunVis == 0 || sunSX < 0 || sunSY < 0 || sunSX >= W || sunSY >= H) return;
    if (zb[sunSY * W + sunSX] != 0) return;
    int low = Sky_sunY < 5000 ? 256 - Sky_sunY * 256 / 5000 : 0;
    int vecX = r->cx - sunSX, vecY = r->cy - sunSY;
    int fade = 256 - (Fx_abs(vecX) + Fx_abs(vecY)) * 256 / (W + H);
    if (fade < 40) fade = 40;
    Sky_spr(fb, W, H, streek, sStreek, sunSX, sunSY, f * 3 / 2, f / 9, Sky_mix(0x605838, 0x803818, low), 0, fade);
    for (int k = 0; k < 6; k++) {
        int x = sunSX + vecX * FL_POS[k] / 100, y = sunSY + vecY * FL_POS[k] / 100;
        int s = f * FL_SIZE[k] / 100;
        const int* t = (k % 3 == 0) ? hex_tex : ((k % 3 == 1) ? circle : ringa);
        int ts = (k % 3 == 0) ? sHex : ((k % 3 == 1) ? sCircle : sRing);
        Sky_spr(fb, W, H, t, ts, x, y, s, s, FL_COL[k], 0, fade);
    }
    Sky_spr(fb, W, H, reflect, sReflect, sunSX + vecX * 2, sunSY + vecY * 2, f / 4, f / 4, 0x302418, 0, fade);
}

void Sky_free(void) {
    free(cloud1); cloud1 = NULL;
    free(cloud2); cloud2 = NULL;
    free(cloud3); cloud3 = NULL;
    free(hilit);  hilit  = NULL;
    free(corona); corona = NULL;
    free(star);   star   = NULL;
    free(moon);   moon   = NULL;
    free(ringa);  ringa  = NULL;
    free(hex_tex);hex_tex= NULL;
    free(circle); circle = NULL;
    free(reflect);reflect= NULL;
    free(streek); streek = NULL;
    free(Sky_flash); Sky_flash = NULL;
    free(Sky_smoke); Sky_smoke = NULL;
    free(Sky_fire);  Sky_fire  = NULL;
    free(Sky_explo); Sky_explo = NULL;
    free(Sky_tgt);   Sky_tgt   = NULL;
}
