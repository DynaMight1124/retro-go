/*
 * MIDP Image / Graphics / Sprite implementation for the C port.
 * Software ARGB rasterizer; the game's original drawing code is used
 * unchanged through this API.
 */
#include "j2me.h"
#include <rg_system.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_THREAD_LOCALS
#include "stb_image.h"

/* ------------------------------------------------------------------ */
/* Platform resources                                                  */
/* ------------------------------------------------------------------ */
static char g_assets_dir[512] = "assets";
static int  g_render_w = 320;
static int  g_render_h = 240;

void platform_set_assets_dir(String dir) {
    if (!dir) dir = "assets";
    snprintf(g_assets_dir, sizeof(g_assets_dir), "%s", dir);
}

String platform_assets_dir(void) {
    return g_assets_dir;
}

uint8_t* platform_read_resource(String path, int* outLen) {
    if (outLen) *outLen = 0;
    if (!path) return NULL;

    const char* p = path;
    while (*p == '/') p++;

    char full[512];
    FILE* f = NULL;

    /* 1. Try direct path */
    f = fopen(path, "rb");
    /* 2. Try configured assets dir */
    if (!f && g_assets_dir[0]) {
        snprintf(full, sizeof(full), "%s/%s", g_assets_dir, p);
        f = fopen(full, "rb");
    }
    /* 3. Try standard fallbacks */
    if (!f) {
        snprintf(full, sizeof(full), "assets/%s", p);
        f = fopen(full, "rb");
    }
    if (!f) {
        snprintf(full, sizeof(full), "res/%s", p);
        f = fopen(full, "rb");
    }
    if (!f) {
        snprintf(full, sizeof(full), "ROMS/gtavc/%s", p);
        f = fopen(full, "rb");
    }
    if (!f) {
        snprintf(full, sizeof(full), "/ROMS/gtavc/%s", p);
        f = fopen(full, "rb");
    }
    if (!f) {
        snprintf(full, sizeof(full), "/sd/roms/gtavc/%s", p);
        f = fopen(full, "rb");
    }

    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return NULL; }
    uint8_t* data = (uint8_t*)malloc((size_t)sz + 1);
    if (!data) { fclose(f); return NULL; }
    size_t got = sz > 0 ? fread(data, 1, (size_t)sz, f) : 0;
    fclose(f);
    if ((long)got != sz) { free(data); return NULL; }
    data[sz] = 0;
    if (outLen) *outLen = (int)sz;
    return data;
}

void platform_log(String msg) {
    if (msg) RG_LOGD("%s", msg);
}

int platform_render_width(void) { return g_render_w; }
int platform_render_height(void) { return g_render_h; }

__attribute__((weak))
void platform_present(const uint32_t* argb, int w, int h) {
    (void)argb;
    g_render_w = w;
    g_render_h = h;
}

/* ------------------------------------------------------------------ */
/* Image                                                               */
/* ------------------------------------------------------------------ */
Image* Image_createImage(int w, int h) {
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    Image* img = (Image*)calloc(1, sizeof(Image));
    img->width = w;
    img->height = h;
    img->argb = (uint32_t*)calloc((size_t)w * h, sizeof(uint32_t));
    img->owned = true;
    return img;
}

Image* Image_createRGBImage(const int* rgb, int w, int h, bool processAlpha) {
    Image* img = Image_createImage(w, h);
    if (!rgb) return img;
    for (int i = 0; i < w * h; i++) {
        uint32_t c = (uint32_t)rgb[i];
        if (!processAlpha) c |= 0xFF000000u;
        img->argb[i] = c;
    }
    return img;
}

Image* Image_createImageRes(String path) {
    int len = 0;
    uint8_t* data = platform_read_resource(path, &len);
    if (data && len > 0) {
        int w = 0, h = 0, comp = 0;
        unsigned char* rgba = stbi_load_from_memory(data, len, &w, &h, &comp, 4);
        free(data);
        if (rgba && w > 0 && h > 0) {
            Image* img = Image_createImage(w, h);
            for (int i = 0; i < w * h; i++) {
                uint32_t r = rgba[i * 4 + 0];
                uint32_t g = rgba[i * 4 + 1];
                uint32_t b = rgba[i * 4 + 2];
                uint32_t a = rgba[i * 4 + 3];
                img->argb[i] = (a << 24) | (r << 16) | (g << 8) | b;
            }
            stbi_image_free(rgba);
            return img;
        }
        if (rgba) stbi_image_free(rgba);
        RG_LOGW("Cannot decode image %s", path ? path : "(null)");
    } else {
        RG_LOGW("Missing image resource %s", path ? path : "(null)");
    }

    Image* img = Image_createImage(16, 16);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            img->argb[y * 16 + x] = ((x ^ y) & 8) ? 0xFFFF00FFu : 0xFF202020u;
        }
    }
    return img;
}

void Image_getRGB(Image* img, int* rgb, int offset, int scanlength,
                  int x, int y, int w, int h) {
    if (!img || !rgb) return;
    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            int sx = x + xx, sy = y + yy;
            int out = offset + yy * scanlength + xx;
            if (sx < 0 || sy < 0 || sx >= img->width || sy >= img->height) {
                rgb[out] = 0;
            } else {
                rgb[out] = (int)img->argb[sy * img->width + sx];
            }
        }
    }
}

int Image_getWidth(Image* img) { return img ? img->width : 0; }
int Image_getHeight(Image* img) { return img ? img->height : 0; }

void Image_free(Image* img) {
    if (!img) return;
    free(img->argb);
    free(img);
}

/* ------------------------------------------------------------------ */
/* Graphics helpers                                                    */
/* ------------------------------------------------------------------ */
static inline uint32_t blend_over(uint32_t dst, uint32_t src) {
    uint32_t sa = src >> 24;
    if (sa == 255) return src;
    if (sa == 0) return dst;
    uint32_t ia = 255 - sa;
    uint32_t sr = (src >> 16) & 0xFF, sg = (src >> 8) & 0xFF, sb = src & 0xFF;
    uint32_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
    uint32_t r = (sr * sa + dr * ia) / 255;
    uint32_t g = (sg * sa + dg * ia) / 255;
    uint32_t b = (sb * sa + db * ia) / 255;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static inline uint32_t color_argb(Graphics* g) {
    uint32_t c = g->color;
    if ((c >> 24) == 0) c |= 0xFF000000u;
    return c;
}

static void put_px(Graphics* g, int x, int y, uint32_t src) {
    if (!g || !g->image) return;
    int px = x + g->tx;
    int py = y + g->ty;
    if (px < g->clipX || py < g->clipY ||
        px >= g->clipX + g->clipW || py >= g->clipY + g->clipH) return;
    if (px < 0 || py < 0 || px >= g->image->width || py >= g->image->height) return;
    uint32_t* dst = &g->image->argb[py * g->image->width + px];
    *dst = blend_over(*dst, src);
}

Graphics* Image_getGraphics(Image* img) {
    Graphics* g = (Graphics*)calloc(1, sizeof(Graphics));
    g->image = img;
    g->clipX = 0;
    g->clipY = 0;
    g->clipW = img ? img->width : 0;
    g->clipH = img ? img->height : 0;
    g->color = 0xFF000000u;
    return g;
}

void g_setColor(Graphics* g, int rgb) {
    if (!g) return;
    g->color = (uint32_t)rgb;
    if ((g->color >> 24) == 0) g->color |= 0xFF000000u;
}

void g_fillRect(Graphics* g, int x, int y, int w, int h) {
    if (!g || w <= 0 || h <= 0) return;
    uint32_t c = color_argb(g);
    int x0 = x + g->tx, y0 = y + g->ty;
    int x1 = x0 + w, y1 = y0 + h;
    if (x0 < g->clipX) x0 = g->clipX;
    if (y0 < g->clipY) y0 = g->clipY;
    if (x1 > g->clipX + g->clipW) x1 = g->clipX + g->clipW;
    if (y1 > g->clipY + g->clipH) y1 = g->clipY + g->clipH;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > g->image->width) x1 = g->image->width;
    if (y1 > g->image->height) y1 = g->image->height;
    for (int yy = y0; yy < y1; yy++) {
        uint32_t* row = &g->image->argb[yy * g->image->width];
        for (int xx = x0; xx < x1; xx++) row[xx] = blend_over(row[xx], c);
    }
}

void g_drawRect(Graphics* g, int x, int y, int w, int h) {
    if (!g || w <= 0 || h <= 0) return;
    uint32_t c = color_argb(g);
    for (int i = 0; i < w; i++) {
        put_px(g, x + i, y, c);
        put_px(g, x + i, y + h - 1, c);
    }
    for (int i = 0; i < h; i++) {
        put_px(g, x, y + i, c);
        put_px(g, x + w - 1, y + i, c);
    }
}

void g_drawLine(Graphics* g, int x1, int y1, int x2, int y2) {
    if (!g) return;
    uint32_t c = color_argb(g);
    int dx = Math_abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -Math_abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        put_px(g, x1, y1, c);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

static int edge(int ax, int ay, int bx, int by, int px, int py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

void g_fillTriangle(Graphics* g, int x1, int y1, int x2, int y2, int x3, int y3) {
    if (!g) return;
    uint32_t c = color_argb(g);
    int minx = Math_min(x1, Math_min(x2, x3));
    int maxx = Math_max(x1, Math_max(x2, x3));
    int miny = Math_min(y1, Math_min(y2, y3));
    int maxy = Math_max(y1, Math_max(y2, y3));
    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            int w0 = edge(x2, y2, x3, y3, x, y);
            int w1 = edge(x3, y3, x1, y1, x, y);
            int w2 = edge(x1, y1, x2, y2, x, y);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                put_px(g, x, y, c);
            }
        }
    }
}

void g_translate(Graphics* g, int x, int y) {
    if (!g) return;
    g->tx += x;
    g->ty += y;
}

void g_setClip(Graphics* g, int x, int y, int w, int h) {
    if (!g) return;
    int x0 = x + g->tx, y0 = y + g->ty;
    int x1 = x0 + w, y1 = y0 + h;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > g->image->width) x1 = g->image->width;
    if (y1 > g->image->height) y1 = g->image->height;
    if (x1 < x0) x1 = x0;
    if (y1 < y0) y1 = y0;
    g->clipX = x0;
    g->clipY = y0;
    g->clipW = x1 - x0;
    g->clipH = y1 - y0;
}

int g_getTranslateX(Graphics* g) { return g ? g->tx : 0; }
int g_getTranslateY(Graphics* g) { return g ? g->ty : 0; }

void g_drawRegion(Graphics* g, Image* src, int sx, int sy, int sw, int sh,
                  int transform, int dx, int dy, int anchor) {
    if (!g || !src || sw <= 0 || sh <= 0) return;

    int dw = sw, dh = sh;
    if (transform == TRANS_ROT90 || transform == TRANS_ROT270 ||
        transform == TRANS_MIRROR_ROT90 || transform == TRANS_MIRROR_ROT270) {
        dw = sh;
        dh = sw;
    }

    if (anchor & HCENTER) dx -= dw / 2;
    else if (anchor & RIGHT) dx -= dw;
    if (anchor & VCENTER) dy -= dh / 2;
    else if (anchor & BOTTOM) dy -= dh;

    for (int y = 0; y < dh; y++) {
        for (int x = 0; x < dw; x++) {
            int i = 0, j = 0;
            switch (transform) {
            case TRANS_MIRROR:            i = sw - 1 - x; j = y; break;
            case TRANS_ROT180:            i = sw - 1 - x; j = sh - 1 - y; break;
            case TRANS_MIRROR_ROT180:     i = x; j = sh - 1 - y; break;
            case TRANS_ROT90:             i = y; j = sh - 1 - x; break;
            case TRANS_ROT270:            i = sw - 1 - y; j = x; break;
            case TRANS_MIRROR_ROT90:      i = sw - 1 - y; j = sh - 1 - x; break;
            case TRANS_MIRROR_ROT270:     i = y; j = x; break;
            default:                      i = x; j = y; break;
            }
            int gx = sx + i;
            int gy = sy + j;
            if (gx < 0 || gy < 0 || gx >= src->width || gy >= src->height) continue;
            uint32_t c = src->argb[gy * src->width + gx];
            if ((c >> 24) == 0) continue;
            put_px(g, dx + x, dy + y, c);
        }
    }
}

void g_drawImage(Graphics* g, Image* src, int x, int y, int anchor) {
    if (!src) return;
    g_drawRegion(g, src, 0, 0, src->width, src->height, TRANS_NONE, x, y, anchor);
}

void g_drawRGB(Graphics* g, const int* rgb, int offset, int scanlength,
               int x, int y, int w, int h, bool processAlpha) {
    if (!g || !rgb) return;
    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            uint32_t c = (uint32_t)rgb[offset + yy * scanlength + xx];
            if (!processAlpha) c |= 0xFF000000u;
            if ((c >> 24) == 0) continue;
            put_px(g, x + xx, y + yy, c);
        }
    }
}

void g_drawString(Graphics* g, String s, int x, int y, int anchor) {
    (void)g; (void)s; (void)x; (void)y; (void)anchor;
}

/* ------------------------------------------------------------------ */
/* Sprite                                                              */
/* ------------------------------------------------------------------ */
Sprite* Sprite_new(Image* image, int frameWidth, int frameHeight) {
    Sprite* s = (Sprite*)calloc(1, sizeof(Sprite));
    s->image = image;
    s->frameW = frameWidth > 0 ? frameWidth : 16;
    s->frameH = frameHeight > 0 ? frameHeight : 16;
    s->cols = image && s->frameW ? image->width / s->frameW : 1;
    if (s->cols < 1) s->cols = 1;
    s->seqLen = 0;
    s->seqIndex = 0;
    s->refX = 0;
    s->refY = 0;
    s->x = 0;
    s->y = 0;
    return s;
}

Sprite* Sprite_setRefPixelPosition(Sprite* s, int x, int y) {
    if (s) { s->refX = x; s->refY = y; }
    return s;
}

Sprite* Sprite_setPosition(Sprite* s, int x, int y) {
    if (s) { s->x = x; s->y = y; }
    return s;
}

Sprite* Sprite_setFrame(Sprite* s, int frame) {
    if (!s) return s;
    if (frame < 0) frame = 0;
    if (s->seqLen > 0 && frame >= s->seqLen) frame = s->seqLen - 1;
    if (s->seqLen == 0 && s->image) {
        int frames = s->cols * (s->image->height / s->frameH);
        if (frame >= frames) frame = frames - 1;
    }
    s->seqIndex = frame;
    return s;
}

Sprite* Sprite_setFrameSequence(Sprite* s, const int* sequence, int length) {
    if (!s || !sequence || length <= 0) return s;
    free(s->sequence);
    s->sequence = (int*)malloc((size_t)length * sizeof(int));
    memcpy(s->sequence, sequence, (size_t)length * sizeof(int));
    s->seqLen = length;
    s->seqIndex = 0;
    return s;
}

void Sprite_setImage(Sprite* s, Image* image, int frameWidth, int frameHeight) {
    if (!s) return;
    s->image = image;
    s->frameW = frameWidth > 0 ? frameWidth : s->frameW;
    s->frameH = frameHeight > 0 ? frameHeight : s->frameH;
    s->cols = image && s->frameW ? image->width / s->frameW : 1;
    if (s->cols < 1) s->cols = 1;
}

void Sprite_paint(Sprite* s, Graphics* g) {
    if (!s || !s->image) return;
    int raw = s->seqIndex;
    if (s->seqLen > 0 && s->sequence) {
        if (raw < 0) raw = 0;
        if (raw >= s->seqLen) raw = s->seqLen - 1;
        raw = s->sequence[raw];
    }
    int col = s->cols > 0 ? raw % s->cols : 0;
    int row = s->cols > 0 ? raw / s->cols : 0;
    g_drawRegion(g, s->image, col * s->frameW, row * s->frameH,
                 s->frameW, s->frameH, TRANS_NONE,
                 s->x - s->refX, s->y - s->refY, 0);
}

int Sprite_getX(Sprite* s) { return s ? s->x : 0; }
int Sprite_getY(Sprite* s) { return s ? s->y : 0; }

void Sprite_free(Sprite* s) {
    if (!s) return;
    free(s->sequence);
    free(s);
}
