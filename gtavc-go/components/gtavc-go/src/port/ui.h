#ifndef GTAVC_UI_H
#define GTAVC_UI_H

#include <stdint.h>
#include <stdbool.h>
#include <rg_system.h>

struct World;

#define UI_CW 14
#define UI_CH 16
#define UI_AW (95 * UI_CW)

#define UI_PINK   0xF890E0
#define UI_GREEN  0x32D88A
#define UI_BLUE   0x5AB4F0
#define UI_WHITE  0xFFFFFF
#define UI_HILITE 0x178A47

#define UI_STEPS 9

extern uint8_t Ui_fa[UI_AW * UI_CH];
extern uint8_t *Ui_faMain, *Ui_faTip;
extern int Ui_gw[95];
extern int *Ui_gwMain, *Ui_gwTip;
extern int Ui_hud[128 * 64];
extern int *Ui_logo, *Ui_bg;
extern int Ui_logoW, Ui_logoH, Ui_bgW, Ui_bgH;
extern int* Ui_scr[5];

extern int* Ui_radImg;
extern int Ui_radW, Ui_radH, Ui_radX0, Ui_radZ0, Ui_radCell;

bool Ui_loadStep(int i);
void Ui_loadFont(void);
void Ui_tipFont(bool on);

static inline IRAM_ATTR int Ui_blend(int d, int c, int a) {
    uint32_t ia = 255u - (uint32_t)a;
    uint32_t ua = (uint32_t)a;
    uint32_t rb = (((uint32_t)(d & 0xFF00FF) * ia) + ((uint32_t)(c & 0xFF00FF) * ua)) >> 8;
    uint32_t g  = (((uint32_t)(d & 0xFF00)   * ia) + ((uint32_t)(c & 0xFF00)   * ua)) >> 8;
    return (int)((rb & 0xFF00FFu) | (g & 0xFF00u));
}

int  Ui_textW(const char* s, int sc);
void Ui_glyph(int* b, int bw, int bh, int x, int y, int g, int col, int sc, int alphaMul);
void Ui_text(int* b, int bw, int bh, int x, int y, const char* s, int col, int sc);
void Ui_textC(int* b, int bw, int bh, int cxm, int y, const char* s, int col, int sc);
void Ui_textR(int* b, int bw, int bh, int xr, int y, const char* s, int col, int sc);
void Ui_fill(int* b, int bw, int bh, int x, int y, int w, int h, int col);
void Ui_fillA(int* b, int bw, int bh, int x, int y, int w, int h, int col, int a);
void Ui_stretch(const int* src, int sw, int sh, int* dst, int dw, int dh, int fade);
void Ui_sprite(int* b, int bw, int bh, const int* sheet, int sheetW, int sx, int sy, int w, int h, int dx, int dy, int sc);
void Ui_logoAt(int* b, int bw, int bh, int x, int y, int sc);
void Ui_menuBg(int* b, int bw, int bh, int u);
void Ui_menuTitle(int* b, int bw, int bh, int u, const char* t);
void Ui_menuItem(int* b, int bw, int bh, int u, int cx, int y, const char* s, bool sel);
void Ui_bars(int* b, int bw, int bh, int u, int xr, int y, int val, int max);
void Ui_hudDraw(int* b, int bw, int bh, struct World* w, int u, bool radar, int radarMode);
void Ui_loadRadar(void);
int  Ui_radarCol(struct World* w, int xm, int zm);
void Ui_radarDraw(int* b, int bw, int bh, struct World* w, int cx, int cy, int R, int u, int mode);
void Ui_helpBox(int* b, int bw, int bh, int u, const char* text);
void Ui_cleanup(void);

#endif /* GTAVC_UI_H */
