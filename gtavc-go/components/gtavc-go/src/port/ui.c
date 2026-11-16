#include "ui.h"
#include "video_compose.h"
#include "assets.h"
#include "world.h"
#include "fx.h"
#include "ped.h"
#include "car.h"
#include "j2me.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t Ui_fa[UI_AW * UI_CH];
uint8_t *Ui_faMain = NULL, *Ui_faTip = NULL;
int Ui_gw[95];
int *Ui_gwMain = NULL, *Ui_gwTip = NULL;
int Ui_hud[128 * 64];
int *Ui_logo = NULL, *Ui_bg = NULL;
int Ui_logoW = 0, Ui_logoH = 0, Ui_bgW = 0, Ui_bgH = 0;
int* Ui_scr[5] = { NULL, NULL, NULL, NULL, NULL };

static const int MAPPAL[8] = {
    0x4A86C8, 0xE6D9A6, 0x56A456, 0xB4B4BC, 0x2C2C30, 0x8C8478, 0xDCDCE4, 0x000000
};

int* Ui_radImg = NULL;
int Ui_radW = 0, Ui_radH = 0, Ui_radX0 = 0, Ui_radZ0 = 0, Ui_radCell = 0;

/* Fast integer formatting: snprintf costs milliseconds per frame on newlib. */
static void ui_itoa(char* out, int v) {
    char tmp[12];
    int n = 0;
    unsigned u;
    if (v < 0) { *out++ = '-'; u = (unsigned)(-v); }
    else u = (unsigned)v;
    do { tmp[n++] = (char)('0' + (u % 10u)); u /= 10u; } while (u != 0);
    while (n > 0) *out++ = tmp[--n];
    *out = 0;
}

static void ui_itoa8(char* out, int v) {
    unsigned u = v < 0 ? 0u : (unsigned)v;
    for (int i = 7; i >= 0; i--) { out[i] = (char)('0' + (u % 10u)); u /= 10u; }
    out[8] = 0;
}

static void loadFontFile(const char* res, uint8_t* outFa, int* outGw) {
    int dm[2];
    int* p = Assets_imgPix(res, dm);
    if (!p) return;
    int total = dm[0] * dm[1];
    if (total > UI_AW * UI_CH) total = UI_AW * UI_CH;
    for (int i = 0; i < total; i++) {
        outFa[i] = (uint8_t)(((uint32_t)p[i]) >> 24);
    }
    for (int g = 0; g < 95; g++) {
        int w = 0;
        for (int x = 0; x < UI_CW; x++) {
            for (int y = 0; y < UI_CH; y++) {
                if (outFa[y * UI_AW + g * UI_CW + x] > 40) {
                    w = x + 1;
                    break;
                }
            }
        }
        outGw[g] = (g == 0) ? 5 : (w < 3 ? 3 : w);
    }
    free(p);
}

void Ui_loadFont(void) {
    if (Ui_faMain != NULL) return;
    Ui_faMain = (uint8_t*)rg_alloc(UI_AW * UI_CH, MEM_FAST);
    Ui_gwMain = (int*)rg_alloc(95 * sizeof(int), MEM_FAST);
    loadFontFile("/font.png", Ui_faMain, Ui_gwMain);

    Ui_faTip = (uint8_t*)rg_alloc(UI_AW * UI_CH, MEM_FAST);
    Ui_gwTip = (int*)rg_alloc(95 * sizeof(int), MEM_FAST);
    loadFontFile("/font_tip.png", Ui_faTip, Ui_gwTip);

    memcpy(Ui_fa, Ui_faMain, UI_AW * UI_CH);
    memcpy(Ui_gw, Ui_gwMain, 95 * sizeof(int));
}

void Ui_tipFont(bool on) {
    if (on && Ui_faTip) {
        memcpy(Ui_fa, Ui_faTip, UI_AW * UI_CH);
        memcpy(Ui_gw, Ui_gwTip, 95 * sizeof(int));
    } else if (Ui_faMain) {
        memcpy(Ui_fa, Ui_faMain, UI_AW * UI_CH);
        memcpy(Ui_gw, Ui_gwMain, 95 * sizeof(int));
    }
}

void Ui_loadRadar(void) {
    int d[2];
    Ui_radImg = Assets_imgPix("/radar.png", d);
    Ui_radW = d[0];
    Ui_radH = d[1];
    int lineCount = 0;
    char** l = Assets_readLines("/radar.txt", &lineCount);
    if (l && lineCount > 0) {
        int x0 = 0, z0 = 0, cell = 0;
        if (sscanf(l[0], "%d %d %d", &x0, &z0, &cell) == 3) {
            Ui_radX0 = x0;
            Ui_radZ0 = z0;
            Ui_radCell = cell;
        }
        Assets_freeLines(l, lineCount);
    }
}

bool Ui_loadStep(int i) {
    int dm[2];
    switch (i) {
        case 0: Ui_scr[0] = Assets_imgPix("/intro.png", dm); break;
        case 1: Ui_scr[1] = Assets_imgPix("/ls0.png", dm); break;
        case 2: Ui_loadFont(); break;
        case 3: {
            int* h = Assets_imgPix("/hud.png", dm);
            if (h) {
                int count = dm[0] * dm[1] < 128 * 64 ? dm[0] * dm[1] : 128 * 64;
                memcpy(Ui_hud, h, (size_t)count * sizeof(int));
                free(h);
            }
            break;
        }
        case 4: Ui_logo = Assets_imgPix("/logo.png", dm); Ui_logoW = dm[0]; Ui_logoH = dm[1]; break;
        case 5: Ui_bg = Assets_imgPix("/menu_bg.png", dm); Ui_bgW = dm[0]; Ui_bgH = dm[1]; Ui_loadRadar(); break;
        case 6: Ui_scr[2] = Assets_imgPix("/ls1.png", dm); break;
        case 7: Ui_scr[3] = Assets_imgPix("/ls2.png", dm); break;
        case 8: Ui_scr[4] = Assets_imgPix("/ls3.png", dm); break;
        default: break;
    }
    return true;
}

int Ui_textW(const char* s, int sc) {
    if (!s) return 0;
    int w = 0;
    while (*s) {
        int c = ((uint8_t)*s) - 32;
        if (c < 0 || c > 94) c = 0;
        w += (Ui_gw[c] + 1) * sc;
        s++;
    }
    return w;
}

IRAM_ATTR void Ui_glyph(int* b, int bw, int bh, int x, int y, int g, int col, int sc, int alphaMul) {
    if (g < 0 || g > 94) g = 0;
    int w = Ui_gw[g];
    Video_prepareRect(b, bw, bh, x, y, w * sc, UI_CH * sc);
    for (int gy = 0; gy < UI_CH; gy++) {
        for (int gx = 0; gx < w; gx++) {
            int a = Ui_fa[gy * UI_AW + g * UI_CW + gx];
            if (a == 0) continue;
            if (alphaMul != 255) a = a * alphaMul / 255;
            for (int sy = 0; sy < sc; sy++) {
                int yy = y + gy * sc + sy;
                if (yy < 0 || yy >= bh) continue;
                for (int sx = 0; sx < sc; sx++) {
                    int xx = x + gx * sc + sx;
                    if (xx < 0 || xx >= bw) continue;
                    int i = yy * bw + xx;
                    b[i] = (a >= 250) ? col : Ui_blend(b[i], col, a);
                }
            }
        }
    }
}

IRAM_ATTR void Ui_text(int* b, int bw, int bh, int x, int y, const char* s, int col, int sc) {
    if (!s) return;
    for (int pass = 0; pass < 2; pass++) {
        int cx = x;
        const char* p = s;
        while (*p) {
            int c = ((uint8_t)*p) - 32;
            if (c < 0 || c > 94) c = 0;
            if (pass == 0) Ui_glyph(b, bw, bh, cx + sc, y + sc, c, 0x101018, sc, 210);
            else Ui_glyph(b, bw, bh, cx, y, c, col, sc, 255);
            cx += (Ui_gw[c] + 1) * sc;
            p++;
        }
    }
}

IRAM_ATTR void Ui_textC(int* b, int bw, int bh, int cxm, int y, const char* s, int col, int sc) {
    Ui_text(b, bw, bh, cxm - Ui_textW(s, sc) / 2, y, s, col, sc);
}

IRAM_ATTR void Ui_textR(int* b, int bw, int bh, int xr, int y, const char* s, int col, int sc) {
    Ui_text(b, bw, bh, xr - Ui_textW(s, sc), y, s, col, sc);
}

IRAM_ATTR void Ui_fill(int* b, int bw, int bh, int x, int y, int w, int h, int col) {
    Video_prepareRect(b, bw, bh, x, y, w, h);
    int ys = y < 0 ? 0 : y;
    int ye = y + h > bh ? bh : y + h;
    int xs = x < 0 ? 0 : x;
    int xe = x + w > bw ? bw : x + w;
    for (int j = ys; j < ye; j++) {
        int row = j * bw;
        for (int i = xs; i < xe; i++) {
            b[row + i] = col;
        }
    }
}

IRAM_ATTR void Ui_fillA(int* b, int bw, int bh, int x, int y, int w, int h, int col, int a) {
    Video_prepareRect(b, bw, bh, x, y, w, h);
    int ys = y < 0 ? 0 : y;
    int ye = y + h > bh ? bh : y + h;
    int xs = x < 0 ? 0 : x;
    int xe = x + w > bw ? bw : x + w;
    for (int j = ys; j < ye; j++) {
        int row = j * bw;
        for (int i = xs; i < xe; i++) {
            b[row + i] = Ui_blend(b[row + i], col, a);
        }
    }
}

void Ui_stretch(const int* src, int sw, int sh, int* dst, int dw, int dh, int fade) {
    if (!src || !dst) return;
    for (int y = 0; y < dh; y++) {
        int sy = y * sh / dh;
        int so = sy * sw, dof = y * dw;
        for (int x = 0; x < dw; x++) {
            int c = src[so + x * sw / dw] & 0xFFFFFF;
            if (fade < 256) {
                uint32_t uf = (uint32_t)fade;
                uint32_t rb = ((uint32_t)(c & 0xFF00FF) * uf) >> 8;
                uint32_t g  = ((uint32_t)(c & 0xFF00)   * uf) >> 8;
                c = (int)((rb & 0xFF00FFu) | (g & 0xFF00u));
            }
            dst[dof + x] = c;
        }
    }
}

void Ui_sprite(int* b, int bw, int bh, const int* sheet, int sheetW, int sx, int sy, int w, int h, int dx, int dy, int sc) {
    if (!sheet) return;
    Video_prepareRect(b, bw, bh, dx, dy, w * sc, h * sc);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int c = sheet[(sy + y) * sheetW + sx + x];
            int a = (uint32_t)c >> 24;
            if (a < 20) continue;
            for (int j = 0; j < sc; j++) {
                int yy = dy + y * sc + j;
                if (yy < 0 || yy >= bh) continue;
                for (int i = 0; i < sc; i++) {
                    int xx = dx + x * sc + i;
                    if (xx < 0 || xx >= bw) continue;
                    b[yy * bw + xx] = (a > 235) ? (c & 0xFFFFFF) : Ui_blend(b[yy * bw + xx], c & 0xFFFFFF, a);
                }
            }
        }
    }
}

void Ui_logoAt(int* b, int bw, int bh, int x, int y, int sc) {
    Ui_sprite(b, bw, bh, Ui_logo, Ui_logoW, 0, 0, Ui_logoW, Ui_logoH, x, y, sc);
}

void Ui_menuBg(int* b, int bw, int bh, int u) {
    Ui_stretch(Ui_bg, Ui_bgW, Ui_bgH, b, bw, bh, 256);
    Ui_logoAt(b, bw, bh, 4 * u, 3 * u, u);
}

void Ui_menuTitle(int* b, int bw, int bh, int u, const char* t) {
    Ui_textR(b, bw, bh, bw - 6 * u, 4 * u, t, UI_PINK, u);
}

void Ui_menuItem(int* b, int bw, int bh, int u, int cx, int y, const char* s, bool sel) {
    int w = Ui_textW(s, u);
    if (sel) Ui_fill(b, bw, bh, cx - w / 2 - 5 * u, y - 1 * u, w + 10 * u, 14 * u, UI_HILITE);
    Ui_text(b, bw, bh, cx - w / 2, y, s, UI_PINK, u);
}

void Ui_bars(int* b, int bw, int bh, int u, int xr, int y, int val, int max) {
    (void)max;
    for (int i = 0; i < 15; i++) {
        int h = (2 + i / 2 + (i >> 1)) * u / 2 + u;
        Ui_fill(b, bw, bh, xr - (15 - i) * 3 * u, y + 11 * u - h, 2 * u, h, i < val ? 0x4CB8F0 : 0x14304C);
    }
}

int Ui_radarCol(World* w, int xm, int zm) {
    if (Ui_radImg != NULL && Ui_radCell > 0) {
        int i = (xm - Ui_radX0) / Ui_radCell;
        int j = (Ui_radZ0 - zm) / Ui_radCell;
        if (xm < Ui_radX0 || zm > Ui_radZ0 || i < 0 || j < 0 || i >= Ui_radW || j >= Ui_radH) {
            return 0x78BEF0;
        }
        return Ui_radImg[j * Ui_radW + i] & 0xFFFFFF;
    }
    if (w->rad && w->rw > 0 && w->rh > 0 && w->rcell > 0) {
        int i = (xm - w->rx0) / w->rcell;
        int j = (zm - w->rz0) / w->rcell;
        if (xm < w->rx0 || zm < w->rz0 || i < 0 || j < 0 || i >= w->rw || j >= w->rh) {
            return MAPPAL[0];
        }
        return MAPPAL[w->rad[j * w->rw + i] & 7];
    }
    return MAPPAL[0];
}

static void blip(int* b, int bw, int bh, World* w, int x, int z, int cx, int cy, int R, int cs, int sn, int mmpp, int col, int u, bool dead) {
    if (dead) return;
    int dx = x - w->player->x, dz = z - w->player->z;
    if (dx > 62000 || dx < -62000 || dz > 62000 || dz < -62000) return;
    int right = ((dx >> 2) * cs - (dz >> 2) * sn) >> 12;
    int fwd   = ((dx >> 2) * sn + (dz >> 2) * cs) >> 12;
    int px = cx + right / mmpp;
    int py = cy - fwd / mmpp;
    int ddx = px - cx, ddy = py - cy;
    if (ddx * ddx + ddy * ddy > (R - 2) * (R - 2)) return;
    Ui_fill(b, bw, bh, px - u / 2, py - u / 2, u + 1, u + 1, col);
}

void Ui_radarDraw(int* b, int bw, int bh, World* w, int cx, int cy, int R, int u, int mode) {
    int extent = R + 2 * u;
    Video_prepareRect(b, bw, bh, cx - extent, cy - extent, extent * 2 + 1, extent * 2 + 1);
    int th = w->camYaw, cs = Fx_cos(th), sn = Fx_sin(th);
    int mpp = 66 * 1024 / R;

    if (mode == 0 && Ui_radImg != NULL && Ui_radCell > 0) {
        int span = 2 * R;
        for (int dy = -R; dy <= R; dy++) {
            int d2_y = dy * dy;
            int r2 = R * R;
            int wx_start = w->player->x + (((cs * (-R) - sn * dy) >> 7) * mpp >> 7);
            int wz_start = w->player->z - (((sn * (-R) + cs * dy) >> 7) * mpp >> 7);
            int wx_end   = w->player->x + (((cs * R - sn * dy) >> 7) * mpp >> 7);
            int wz_end   = w->player->z - (((sn * R + cs * dy) >> 7) * mpp >> 7);

            int u0 = (int)(((int64_t)(wx_start - Ui_radX0) << 12) / Ui_radCell);
            int v0 = (int)(((int64_t)(Ui_radZ0 - wz_start) << 12) / Ui_radCell);
            int u1 = (int)(((int64_t)(wx_end - Ui_radX0) << 12) / Ui_radCell);
            int v1 = (int)(((int64_t)(Ui_radZ0 - wz_end) << 12) / Ui_radCell);
            int du = (u1 - u0) / span;
            int dv = (v1 - v0) / span;

            int cur_u = u0;
            int cur_v = v0;
            int yy = cy + dy;
            if (yy < 0 || yy >= bh) continue;
            int row_offset = yy * bw;

            for (int dx = -R; dx <= R; dx++, cur_u += du, cur_v += dv) {
                if (dx * dx + d2_y > r2) continue;
                int xx = cx + dx;
                if (xx < 0 || xx >= bw) continue;
                int i = cur_u >> 12;
                int j = cur_v >> 12;
                int col = ((unsigned)i < (unsigned)Ui_radW && (unsigned)j < (unsigned)Ui_radH)
                            ? (Ui_radImg[j * Ui_radW + i] & 0xFFFFFF)
                            : 0x78BEF0;
                b[row_offset + xx] = col;
            }
        }
    } else {
        for (int dy = -R; dy <= R; dy++) {
            int yy = cy + dy;
            if (yy < 0 || yy >= bh) continue;
            int row_offset = yy * bw;
            for (int dx = -R; dx <= R; dx++) {
                if (dx * dx + dy * dy > R * R) continue;
                int xx = cx + dx;
                if (xx < 0 || xx >= bw) continue;
                int col;
                if (mode == 1) {
                    col = 0x202028;
                } else {
                    int wx = w->player->x + (((cs * dx - sn * dy) >> 7) * mpp >> 7);
                    int wz = w->player->z - (((sn * dx + cs * dy) >> 7) * mpp >> 7);
                    col = Ui_radarCol(w, wx, wz);
                }
                b[row_offset + xx] = col;
            }
        }
    }

    if (w->wanted > 0) {
        for (int i = 0; i < w->nped; i++) {
            if (w->peds[i].cop) {
                int px = (w->peds[i].seat != NULL) ? w->peds[i].seat->x : w->peds[i].x;
                int pz = (w->peds[i].seat != NULL) ? w->peds[i].seat->z : w->peds[i].z;
                int col = ((w->time / 250) % 2 == 0) ? 0x2040FF : 0xFF2020;
                blip(b, bw, bh, w, px, pz, cx, cy, R, cs, sn, mpp, col, u, w->peds[i].state == PED_DEAD);
            }
        }
    }

    if (w->missionState != WORLD_MISSION_PASSED) {
        int col = ((w->time / 250) & 1) ? 0xFFD040 : 0xFFFF80;
        blip(b, bw, bh, w, w->missionX, w->missionZ, cx, cy, R, cs, sn, mpp, col, u, false);
    }

    int pyaw = (w->inCar != NULL ? w->inCar->yaw : w->player->yaw) - th;
    int ax = Fx_sin(pyaw), az = -Fx_cos(pyaw);
    for (int k = -3 * u; k <= 5 * u; k++) {
        int ww = (5 * u - k) / 3;
        for (int s = -ww; s <= ww; s++) {
            int ox = (ax * k >> 14) + (az * s >> 14) * -1;
            int oy = (az * k >> 14) + (ax * s >> 14);
            int xx = cx + ox, yy = cy + oy;
            if (xx >= 0 && yy >= 0 && xx < bw && yy < bh) b[yy * bw + xx] = 0xFFFFFF;
        }
    }

    for (int dy = -R - 2 * u; dy <= R + 2 * u; dy++) {
        for (int dx = -R - 2 * u; dx <= R + 2 * u; dx++) {
            int d2 = dx * dx + dy * dy;
            if (d2 >= R * R && d2 <= (R + 2 * u) * (R + 2 * u)) {
                int xx = cx + dx, yy = cy + dy;
                if (xx >= 0 && yy >= 0 && xx < bw && yy < bh) b[yy * bw + xx] = 0xFFA0E8;
            }
        }
    }

    int nx = cx - ((sn * R) >> 14);
    int ny = cy - ((cs * R) >> 14);
    Ui_fill(b, bw, bh, nx - 4 * u, ny - 4 * u, 8 * u, 8 * u, 0x101018);
    Ui_text(b, bw, bh, nx - 3 * u, ny - 6 * u, "N", 0xFFFFFF, 1);
}

void Ui_hudDraw(int* b, int bw, int bh, World* w, int u, bool radar, int radarMode) {
    int right = bw - 5 * u;
    Ui_sprite(b, bw, bh, Ui_hud, 128, 0, 20, 38, 30, right - 38 * u, 4 * u, u);
    if (w->weapon > 0) {
        Ui_fillA(b, bw, bh, right - 36 * u, 6 * u, 34 * u, 26 * u, 0x101018, 200);
        Ui_textC(b, bw, bh, right - 19 * u, 8 * u, w->weapon == 1 ? "bat" : "colt", UI_WHITE, u);
        if (w->weapon == 2) {
            char buf[16];
            ui_itoa(buf, w->ammo);
            Ui_textC(b, bw, bh, right - 19 * u, 19 * u, buf, UI_BLUE, u);
        }
    }

    int tx = right - 42 * u;
    char clk[32];
    World_clockStr(w, clk, sizeof(clk));
    Ui_textR(b, bw, bh, tx, 3 * u, clk, UI_BLUE, u);

    char mbuf[12];
    mbuf[0] = '$';
    ui_itoa8(mbuf + 1, w->money);
    Ui_textR(b, bw, bh, tx, 3 * u + 13 * u, mbuf, UI_GREEN, u);

    int hy = 3 * u + 26 * u;
    char hpbuf[16];
    ui_itoa(hpbuf, w->health);
    Ui_textR(b, bw, bh, tx, hy, hpbuf, UI_PINK, u);
    Ui_sprite(b, bw, bh, Ui_hud, 128, 0, 0, 14, 13, tx - Ui_textW(hpbuf, u) - 16 * u, hy, u);

    if (w->armor > 0) {
        char arbuf[16];
        ui_itoa(arbuf, w->armor);
        Ui_textR(b, bw, bh, tx - Ui_textW(hpbuf, u) - 22 * u, hy, arbuf, 0xC8C8D0, u);
    }

    for (int i = 0; i < 6; i++) {
        Ui_sprite(b, bw, bh, Ui_hud, 128, i < w->wanted ? 36 : 16, 0, 18, 18,
                  tx - (6 - i) * 17 * u + 4 * u, hy + 14 * u, u);
    }

    if (radar && radarMode != 2) {
        Ui_radarDraw(b, bw, bh, w, 4 * u + 30 * u, bh - 4 * u - 30 * u, 30 * u, u, radarMode);
    }
}

void Ui_helpBox(int* b, int bw, int bh, int u, const char* text) {
    if (!text || !*text) return;
    Ui_tipFont(true);
    int maxW = bw * 6 / 10;
    char lines[6][256];
    int n = 0;
    char cur[256];
    cur[0] = '\0';

    const char* p = text;
    while (*p && n < 6) {
        const char* sp = strchr(p, ' ');
        int wordLen = sp ? (int)(sp - p) : (int)strlen(p);
        char word[128];
        if (wordLen >= (int)sizeof(word)) wordLen = (int)sizeof(word) - 1;
        memcpy(word, p, (size_t)wordLen);
        word[wordLen] = '\0';

        char t[256];
        if (cur[0] == '\0') {
            snprintf(t, sizeof(t), "%s", word);
        } else {
            snprintf(t, sizeof(t), "%s %s", cur, word);
        }

        if (Ui_textW(t, u) > maxW && cur[0] != '\0') {
            snprintf(lines[n++], sizeof(lines[0]), "%s", cur);
            snprintf(cur, sizeof(cur), "%s", word);
        } else {
            snprintf(cur, sizeof(cur), "%s", t);
        }

        if (!sp) break;
        p = sp + 1;
    }
    if (cur[0] != '\0' && n < 6) {
        snprintf(lines[n++], sizeof(lines[0]), "%s", cur);
    }

    int w = 0;
    for (int k = 0; k < n; k++) {
        int tw = Ui_textW(lines[k], u);
        if (tw > w) w = tw;
    }
    Ui_fillA(b, bw, bh, 4 * u, 4 * u, w + 8 * u, n * 13 * u + 5 * u, 0x000000, 170);
    for (int k = 0; k < n; k++) {
        Ui_text(b, bw, bh, 8 * u, 6 * u + k * 13 * u, lines[k], 0xF0F0F0, u);
    }
    Ui_tipFont(false);
}

void Ui_cleanup(void) {
    free(Ui_faMain); Ui_faMain = NULL;
    free(Ui_gwMain); Ui_gwMain = NULL;
    free(Ui_faTip);  Ui_faTip  = NULL;
    free(Ui_gwTip);  Ui_gwTip  = NULL;
    free(Ui_logo);   Ui_logo   = NULL;
    free(Ui_bg);     Ui_bg     = NULL;
    free(Ui_radImg); Ui_radImg = NULL;
    for (int i = 0; i < 5; i++) {
        free(Ui_scr[i]);
        Ui_scr[i] = NULL;
    }
}
