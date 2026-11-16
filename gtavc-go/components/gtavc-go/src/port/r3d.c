#include "r3d.h"
#include "fx.h"
#include <rg_system.h>
#include <stdlib.h>
#include <string.h>
#include "fx_math.h"
#include "gtavc_profile.h"
#include <esp_memory_utils.h>

void R3D_init(R3D* r) {
    memset(r, 0, sizeof(R3D));
    r->lx = 5000;
    r->ly = 12000;
    r->lz = -6000;
    r->amb = 256;
    r->dif = 0;
    r->fogR = 200;
    r->fogG = 200;
    r->fogB = 220;
    r->fogNear = 2000;
    r->fogFar = 8000;
    r->farZ = 9000;
    r->skyTop = 0x4aa0e0;
    r->skyBot = 0xbfe0f0;
    r->groundCol = 0x606058;
    r->emis = 256;
    r->forceTex = -1;
    r->tint2 = 0xFFFFFF;

    /* Defer the six scratch arrays until drawing. R3D_resize gets first chance
       at a contiguous internal block for the much hotter colour buffer. */
}

void R3D_resize(R3D* r, int w, int h, int fovFocal) {
    r->W = w;
    r->H = h;
    r->cx = w >> 1;
    r->cy = h >> 1;
    r->focal = fovFocal;
    r->recipF = (1 << 20) / (fovFocal > 0 ? fovFocal : 1);

    free(r->fb);
    free(r->zb);
    size_t fb_bytes = (size_t)w * (size_t)h * sizeof(int);
    r->fb = (int*)rg_alloc(fb_bytes, ((fb_bytes <= 80000) ? MEM_FAST : MEM_SLOW) | MEM_NOPANIC);
    r->zb = (int*)rg_alloc(fb_bytes, MEM_SLOW | MEM_NOPANIC);

    if (r->fb && r->zb) RG_LOGI("World resolution: %dx%d", w, h);
    RG_LOGD("[PROF] buffers world=%dx%d bytes_each=%u colour=%s depth=%s",
            w, h, (unsigned)fb_bytes,
            esp_ptr_internal(r->fb) ? "internal" : "external",
            esp_ptr_internal(r->zb) ? "internal" : "external");

    r->radX = (int)Fx_isqrt((uint64_t)((int64_t)fovFocal * fovFocal) + (uint64_t)((int64_t)r->cx * r->cx)) + 1;
    r->radY = (int)Fx_isqrt((uint64_t)((int64_t)fovFocal * fovFocal) + (uint64_t)((int64_t)r->cy * r->cy)) + 1;

}

void R3D_setTexture(R3D* r, int id, int* pix, int w, int h) {
    if (id < 0 || id >= R3D_MAX_TEX) return;
    r->T[id] = pix;
    int s = 0;
    while ((1 << s) < w) s++;
    r->tws[id] = s;
    r->twm[id] = w - 1;
    r->thm[id] = h - 1;
    r->tpu[id] = w * 16;
    r->tpv[id] = h * 16;
}

IRAM_ATTR void R3D_setCamera(R3D* r, int x, int y, int z, int yaw, int pitch) {
    r->camX = x; r->camY = y; r->camZ = z;
    int cth = Fx_cos(yaw), sth = Fx_sin(yaw);
    int cph = Fx_cos(pitch), sph = Fx_sin(pitch);
    r->mv[0] = cth;
    r->mv[1] = 0;
    r->mv[2] = -sth;
    r->mv[3] = (sth * sph) >> 14;
    r->mv[4] = cph;
    r->mv[5] = (cth * sph) >> 14;
    r->mv[6] = (sth * cph) >> 14;
    r->mv[7] = -sph;
    r->mv[8] = (cth * cph) >> 14;
    int c = cph < 64 ? 64 : cph;
    r->horizon = r->cy - (r->focal * sph) / c;
}

/* Fixed-size bursts reduce loop/branch overhead without extra scratch memory.
   Keep the tail for every supported width, including 106 pixels. */
static inline void R3D_fillColour(int* dst, int count, int colour) {
    while (count >= 8) {
        dst[0] = colour; dst[1] = colour; dst[2] = colour; dst[3] = colour;
        dst[4] = colour; dst[5] = colour; dst[6] = colour; dst[7] = colour;
        dst += 8; count -= 8;
    }
    while (count-- > 0) *dst++ = colour;
}

IRAM_ATTR void R3D_clear(R3D* r) {
    int h = r->horizon;
    if (h < 0) h = 0;
    if (h > r->H) h = r->H;
    int tr = (r->skyTop >> 16) & 255, tg = (r->skyTop >> 8) & 255, tb = r->skyTop & 255;
    int br = (r->skyBot >> 16) & 255, bg = (r->skyBot >> 8) & 255, bb = r->skyBot & 255;
    int W = r->W, H = r->H;
    int* fb = r->fb;
    int* zb = r->zb;
    int groundCol = r->groundCol;
    int skyBot = r->skyBot;

    /* 1. Fast contiguous Z-buffer clear (burst 32-bit stores in PSRAM) */
    int64_t depth_start = GTAVC_PROFILE_TIMER();
    memset(zb, 0, (size_t)W * (size_t)H * sizeof(int));
    int64_t colour_start = GTAVC_PROFILE_TIMER();
    GTAVC_PROF(g_prof.t_depth_clear_us += (uint64_t)(colour_start - depth_start));

    /* 2. Linear sequential row writes to framebuffer (avoids PSRAM cache thrashing) */
    int y = 0;
    for (; y < h && y < H; y++) {
        int t = h == 0 ? 256 : (y * 256) / h;
        int c = (((tr + (((br - tr) * t) >> 8)) << 16) |
                 ((tg + (((bg - tg) * t) >> 8)) << 8) |
                 (tb + (((bb - tb) * t) >> 8)));
        int *row = &fb[y * W];
        R3D_fillColour(row, W, c);
    }
    for (; y < h + 2 && y < H; y++) {
        int *row = &fb[y * W];
        R3D_fillColour(row, W, skyBot);
    }
    int ground_pixels = (H - y) * W;
    if (ground_pixels > 0) {
        int *ptr = &fb[y * W];
        R3D_fillColour(ptr, ground_pixels, groundCol);
    }
    GTAVC_PROF(g_prof.t_colour_clear_us += (uint64_t)(rg_system_timer() - colour_start));

    r->tris = 0;
    r->verts = 0;
}

static bool R3D_ensure(R3D* r, int n) {
    if (n > r->vxCap) {
        int capacity = n + 256;
        if (capacity < 1024) capacity = 1024;
        int *scratch[6] = {0};
        unsigned internal = 0;
        for (int i = 0; i < 6; i++) {
            scratch[i] = (int*)rg_alloc((size_t)capacity * sizeof(int), MEM_SLOW | MEM_NOPANIC);
            if (!scratch[i]) {
                for (int j = 0; j < i; j++) free(scratch[j]);
                RG_LOGW("Renderer scratch allocation failed capacity=%d", capacity);
                return false;
            }
            if (esp_ptr_internal(scratch[i])) internal++;
        }
        /* Scratch is overwritten for every mesh, so growth needs no copy.
           Preserve all old arrays until the complete replacement succeeds. */
        free(r->vx); free(r->vy); free(r->vz);
        free(r->vsx); free(r->vsy); free(r->viz);
        r->vx=scratch[0]; r->vy=scratch[1]; r->vz=scratch[2];
        r->vsx=scratch[3]; r->vsy=scratch[4]; r->viz=scratch[5];
        r->vxCap=capacity;
        RG_LOGD("[PROF] renderer scratch PSRAM preferred capacity=%d bytes_each=%u internal_arrays=%u/6",
                capacity, (unsigned)((size_t)capacity*sizeof(int)), internal);
    }
    return true;
}

IRAM_ATTR int R3D_shadePx(int c, int s, int add) {
    return (int)(((((uint32_t)(c & 0xFF00FF) * s) >> 8) & 0xFF00FF) |
                 ((((uint32_t)(c & 0xFF00) * s) >> 8) & 0xFF00)) + add;
}

/*
 * Rasteriser setup in fixed point (Q40 reciprocal of the triangle area).
 * The original Java build used floats here; on the SF2000/GB300 there is no
 * FPU, so every float operation was a libgcc soft-float call.  The integer
 * inner loops are unchanged - only the per-triangle/per-scanline setup moved
 * to integers.  Inputs are bounded by the renderer (screen coords, 24-bit
 * depth, UV span <= 4090), so the 64-bit intermediates cannot overflow.
 */
static inline int64_t r3d_recip40(int area) {
    return ((int64_t)1 << 40) / area;
}

static void IRAM_ATTR rasterFlat(R3D* r, int color) {
    int a = 0, b = 1, c = 2, t;
    if (r->py[a] > r->py[b]) { t = a; a = b; b = t; }
    if (r->py[b] > r->py[c]) { t = b; b = c; c = t; }
    if (r->py[a] > r->py[b]) { t = a; a = b; b = t; }

    int xa = r->px[a], ya = r->py[a];
    int xb = r->px[b], yb = r->py[b];
    int xc = r->px[c], yc = r->py[c];
    if (ya == yc || yc < 0 || ya >= r->H) return;

    int dx1 = xb - xa, dy1 = yb - ya;
    int dx2 = xc - xa, dy2 = yc - ya;
    int area = dx1 * dy2 - dx2 * dy1;
    if (area == 0) return;

    int64_t ndz_x = (int64_t)(r->piz[b] - r->piz[a]) * dy2 - (int64_t)(r->piz[c] - r->piz[a]) * dy1;
    int64_t ndz_y = (int64_t)(r->piz[c] - r->piz[a]) * dx1 - (int64_t)(r->piz[b] - r->piz[a]) * dx2;
    int64_t dzdx_q16, dzdy_q16;
    int aa = area < 0 ? -area : area;
    if (aa < 64) {
        dzdx_q16 = (ndz_x << 16) / area;
        dzdy_q16 = (ndz_y << 16) / area;
    } else {
        int64_t inv = r3d_recip40(area);
        dzdx_q16 = (ndz_x * inv) >> 24;
        dzdy_q16 = (ndz_y * inv) >> 24;
    }
    int dz8 = (int)(dzdx_q16 >> 8);

    int dxl = (dx2 << 16) / dy2;
    int dxab = dy1 > 0 ? (dx1 << 16) / dy1 : 0;
    int dybc = yc - yb;
    int dxbc = dybc > 0 ? ((xc - xb) << 16) / dybc : 0;
    int ys = ya < 0 ? 0 : ya;
    int ye = yc > r->H ? r->H : yc;
    int* fbb = r->fb;
    int* zbb = r->zb;
    int W = r->W;
    int64_t iza16 = (int64_t)r->piz[a] << 16;

    for (int y = ys; y < ye; y++) {
        int xl = (xa << 16) + dxl * (y - ya);
        int xs2 = y < yb ? (xa << 16) + dxab * (y - ya) : (xb << 16) + dxbc * (y - yb);
        int xL, xR;
        if (xs2 < xl) { xL = xs2; xR = xl; } else { xL = xl; xR = xs2; }
        int xs = (xL + 0xFFFF) >> 16, xe = (xR + 0xFFFF) >> 16;
        if (xs < 0) xs = 0;
        if (xe > W) xe = W;
        if (xs >= xe) continue;

        int iz = (int)((iza16 + dzdx_q16 * (xs - xa) + dzdy_q16 * (y - ya)) >> 8);
        int i = y * W + xs;
        for (int n = xe - xs; n > 0; n--) {
            if (iz > zbb[i]) {
                if (r->additive) {
                    int d = fbb[i];
                    int rb = (d & 0xFF00FF) + (color & 0xFF00FF);
                    int g = (d & 0xFF00) + (color & 0xFF00);
                    if ((rb & 0x1000000) != 0) rb |= 0xFF0000;
                    if ((rb & 0x100) != 0) rb |= 0xFF;
                    if ((g & 0x10000) != 0) g = 0xFF00;
                    fbb[i] = (rb & 0xFF00FF) | (g & 0xFF00);
                } else if (r->blend) {
                    fbb[i] = (((fbb[i] >> 1) & 0x7F7F7F) + ((color >> 1) & 0x7F7F7F));
                } else {
                    zbb[i] = iz;
                    fbb[i] = color;
                }
            }
            iz += dz8;
            i++;
        }
    }
}

static void IRAM_ATTR rasterTex(R3D* r, int tid, int s, int add, int phU, int phV) {
    int a = 0, b = 1, c = 2, t;
    if (r->py[a] > r->py[b]) { t = a; a = b; b = t; }
    if (r->py[b] > r->py[c]) { t = b; b = c; c = t; }
    if (r->py[a] > r->py[b]) { t = a; a = b; b = t; }

    int xa = r->px[a], ya = r->py[a];
    int xb = r->px[b], yb = r->py[b];
    int xc = r->px[c], yc = r->py[c];
    if (ya == yc || yc < 0 || ya >= r->H) return;

    int dx1 = xb - xa, dy1 = yb - ya;
    int dx2 = xc - xa, dy2 = yc - ya;
    int area = dx1 * dy2 - dx2 * dy1;
    if (area == 0) return;

    int64_t iza = r->piz[a];
    int64_t uza = (int64_t)r->pu[a] * r->piz[a];
    int64_t vza = (int64_t)r->pv[a] * r->piz[a];
    int64_t uzb = (int64_t)r->pu[b] * r->piz[b];
    int64_t vzb = (int64_t)r->pv[b] * r->piz[b];
    int64_t uzc = (int64_t)r->pu[c] * r->piz[c];
    int64_t vzc = (int64_t)r->pv[c] * r->piz[c];

    int64_t ndz_x = (int64_t)(r->piz[b] - iza) * dy2 - (int64_t)(r->piz[c] - iza) * dy1;
    int64_t ndz_y = (int64_t)(r->piz[c] - iza) * dx1 - (int64_t)(r->piz[b] - iza) * dx2;
    int64_t ndu_x = (uzb - uza) * dy2 - (uzc - uza) * dy1;
    int64_t ndu_y = (uzc - uza) * dx1 - (uzb - uza) * dx2;
    int64_t ndv_x = (vzb - vza) * dy2 - (vzc - vza) * dy1;
    int64_t ndv_y = (vzc - vza) * dx1 - (vzb - vza) * dx2;

    int64_t dzdx_q16, dzdy_q16, dudx_q16, dudy_q16, dvdx_q16, dvdy_q16;
    int aa = area < 0 ? -area : area;
    if (aa < 64) {
        dzdx_q16 = (ndz_x << 16) / area;
        dzdy_q16 = (ndz_y << 16) / area;
        dudx_q16 = (ndu_x << 16) / area;
        dudy_q16 = (ndu_y << 16) / area;
        dvdx_q16 = (ndv_x << 16) / area;
        dvdy_q16 = (ndv_y << 16) / area;
    } else {
        int64_t lim = (int64_t)8000000 * aa;
        if (ndu_x > lim || ndu_x < -lim || ndu_y > lim || ndu_y < -lim ||
            ndv_x > lim || ndv_x < -lim || ndv_y > lim || ndv_y < -lim) return;
        int64_t inv = r3d_recip40(area);
        dzdx_q16 = (ndz_x * inv) >> 24;
        dzdy_q16 = (ndz_y * inv) >> 24;
        dudx_q16 = (ndu_x * inv) >> 24;
        dudy_q16 = (ndu_y * inv) >> 24;
        dvdx_q16 = (ndv_x * inv) >> 24;
        dvdy_q16 = (ndv_y * inv) >> 24;
    }

    int dz8 = (int)(dzdx_q16 >> 8);
    int duz = (int)(dudx_q16 >> 16), dvz = (int)(dvdx_q16 >> 16);

    int dxl = (dx2 << 16) / dy2;
    int dxab = dy1 > 0 ? (dx1 << 16) / dy1 : 0;
    int dybc = yc - yb;
    int dxbc = dybc > 0 ? ((xc - xb) << 16) / dybc : 0;
    int ys = ya < 0 ? 0 : ya;
    int ye = yc > r->H ? r->H : yc;
    int* fbb = r->fb;
    int* zbb = r->zb;
    int* tex = r->T[tid];
    int ws = r->tws[tid], wm = r->twm[tid], hm = r->thm[tid];
    int W = r->W;
    int64_t iza16 = iza << 16, uza16 = uza << 16, vza16 = vza << 16;

    for (int y = ys; y < ye; y++) {
        int xl = (xa << 16) + dxl * (y - ya);
        int xs2 = y < yb ? (xa << 16) + dxab * (y - ya) : (xb << 16) + dxbc * (y - yb);
        int xL, xR;
        if (xs2 < xl) { xL = xs2; xR = xl; } else { xL = xl; xR = xs2; }
        int xs = (xL + 0xFFFF) >> 16, xe = (xR + 0xFFFF) >> 16;
        if (xs < 0) xs = 0;
        if (xe > W) xe = W;
        if (xs >= xe) continue;

        int fx = xs - xa, fy = y - ya;
        int iz = (int)((iza16 + dzdx_q16 * fx + dzdy_q16 * fy) >> 8);
        int uz = (int)((uza16 + dudx_q16 * fx + dudy_q16 * fy) >> 16);
        int vz = (int)((vza16 + dvdx_q16 * fx + dvdy_q16 * fy) >> 16);
        int i = y * W + xs;
        int n = xe - xs;
        int ds = iz >> 8;
        if (ds < 1) ds = 1;
        int uA = uz / ds + phU, vA = vz / ds + phV;

        while (n > 0) {
            int cnt = n < 8 ? n : 8;
            int izE, uzE, vzE, du, dv;
            if (cnt == 8) {
                izE = iz + (dz8 << 3); uzE = uz + (duz << 3); vzE = vz + (dvz << 3);
            } else {
                izE = iz + dz8 * cnt; uzE = uz + duz * cnt; vzE = vz + dvz * cnt;
            }
            int dsE = izE >> 8;
            if (dsE < 1) dsE = 1;
            int uB = uzE / dsE + phU, vB = vzE / dsE + phV;
            if (cnt == 8) {
                du = (uB - uA) << 9;
                dv = (vB - vA) << 9;
            } else {
                du = ((uB - uA) << 12) / cnt;
                dv = ((vB - vA) << 12) / cnt;
            }
            int u = uA << 12, v = vA << 12;

            for (int k = 0; k < cnt; k++) {
                if (iz > zbb[i]) {
                    int col = tex[(((v >> 16) & hm) << ws) | ((u >> 16) & wm)];
                    if (col != 0) {
                        zbb[i] = iz;
                        fbb[i] = (int)(((((uint32_t)(col & 0xFF00FF) * s) >> 8) & 0xFF00FF) |
                                       ((((uint32_t)(col & 0xFF00) * s) >> 8) & 0xFF00)) + add;
                    }
                }
                iz += dz8; u += du; v += dv; i++;
            }
            uA = uB; vA = vB; uz = uzE; vz = vzE;
            n -= cnt;
        }
    }
}

static void IRAM_ATTR rasterTexTint(R3D* r, int tid, int s, int add, int phU, int phV, int tc) {
    int sr = (((tc >> 16) & 255) * s) >> 8;
    int sg = (((tc >> 8) & 255) * s) >> 8;
    int sb = ((tc & 255) * s) >> 8;

    int a = 0, b = 1, c = 2, t;
    if (r->py[a] > r->py[b]) { t = a; a = b; b = t; }
    if (r->py[b] > r->py[c]) { t = b; b = c; c = t; }
    if (r->py[a] > r->py[b]) { t = a; a = b; b = t; }

    int xa = r->px[a], ya = r->py[a];
    int xb = r->px[b], yb = r->py[b];
    int xc = r->px[c], yc = r->py[c];
    if (ya == yc || yc < 0 || ya >= r->H) return;

    int dx1 = xb - xa, dy1 = yb - ya;
    int dx2 = xc - xa, dy2 = yc - ya;
    int area = dx1 * dy2 - dx2 * dy1;
    if (area == 0) return;

    int64_t iza = r->piz[a];
    int64_t uza = (int64_t)r->pu[a] * r->piz[a];
    int64_t vza = (int64_t)r->pv[a] * r->piz[a];
    int64_t uzb = (int64_t)r->pu[b] * r->piz[b];
    int64_t vzb = (int64_t)r->pv[b] * r->piz[b];
    int64_t uzc = (int64_t)r->pu[c] * r->piz[c];
    int64_t vzc = (int64_t)r->pv[c] * r->piz[c];

    int64_t ndz_x = (int64_t)(r->piz[b] - iza) * dy2 - (int64_t)(r->piz[c] - iza) * dy1;
    int64_t ndz_y = (int64_t)(r->piz[c] - iza) * dx1 - (int64_t)(r->piz[b] - iza) * dx2;
    int64_t ndu_x = (uzb - uza) * dy2 - (uzc - uza) * dy1;
    int64_t ndu_y = (uzc - uza) * dx1 - (uzb - uza) * dx2;
    int64_t ndv_x = (vzb - vza) * dy2 - (vzc - vza) * dy1;
    int64_t ndv_y = (vzc - vza) * dx1 - (vzb - vza) * dx2;

    int64_t dzdx_q16, dzdy_q16, dudx_q16, dudy_q16, dvdx_q16, dvdy_q16;
    int aa = area < 0 ? -area : area;
    if (aa < 64) {
        dzdx_q16 = (ndz_x << 16) / area;
        dzdy_q16 = (ndz_y << 16) / area;
        dudx_q16 = (ndu_x << 16) / area;
        dudy_q16 = (ndu_y << 16) / area;
        dvdx_q16 = (ndv_x << 16) / area;
        dvdy_q16 = (ndv_y << 16) / area;
    } else {
        int64_t lim = (int64_t)8000000 * aa;
        if (ndu_x > lim || ndu_x < -lim || ndu_y > lim || ndu_y < -lim ||
            ndv_x > lim || ndv_x < -lim || ndv_y > lim || ndv_y < -lim) return;
        int64_t inv = r3d_recip40(area);
        dzdx_q16 = (ndz_x * inv) >> 24;
        dzdy_q16 = (ndz_y * inv) >> 24;
        dudx_q16 = (ndu_x * inv) >> 24;
        dudy_q16 = (ndu_y * inv) >> 24;
        dvdx_q16 = (ndv_x * inv) >> 24;
        dvdy_q16 = (ndv_y * inv) >> 24;
    }

    int dz8 = (int)(dzdx_q16 >> 8);
    int duz = (int)(dudx_q16 >> 16), dvz = (int)(dvdx_q16 >> 16);

    int dxl = (dx2 << 16) / dy2;
    int dxab = dy1 > 0 ? (dx1 << 16) / dy1 : 0;
    int dybc = yc - yb;
    int dxbc = dybc > 0 ? ((xc - xb) << 16) / dybc : 0;
    int ys = ya < 0 ? 0 : ya;
    int ye = yc > r->H ? r->H : yc;
    int* fbb = r->fb;
    int* zbb = r->zb;
    int* tex = r->T[tid];
    int ws = r->tws[tid], wm = r->twm[tid], hm = r->thm[tid];
    int W = r->W;
    int64_t iza16 = iza << 16, uza16 = uza << 16, vza16 = vza << 16;

    for (int y = ys; y < ye; y++) {
        int xl = (xa << 16) + dxl * (y - ya);
        int xs2 = y < yb ? (xa << 16) + dxab * (y - ya) : (xb << 16) + dxbc * (y - yb);
        int xL, xR;
        if (xs2 < xl) { xL = xs2; xR = xl; } else { xL = xl; xR = xs2; }
        int xs = (xL + 0xFFFF) >> 16, xe = (xR + 0xFFFF) >> 16;
        if (xs < 0) xs = 0;
        if (xe > W) xe = W;
        if (xs >= xe) continue;

        int fx = xs - xa, fy = y - ya;
        int iz = (int)((iza16 + dzdx_q16 * fx + dzdy_q16 * fy) >> 8);
        int uz = (int)((uza16 + dudx_q16 * fx + dudy_q16 * fy) >> 16);
        int vz = (int)((vza16 + dvdx_q16 * fx + dvdy_q16 * fy) >> 16);
        int i = y * W + xs;
        int n = xe - xs;
        int ds = iz >> 8;
        if (ds < 1) ds = 1;
        int uA = uz / ds + phU, vA = vz / ds + phV;

        while (n > 0) {
            int cnt = n < 8 ? n : 8;
            int izE, uzE, vzE, du, dv;
            if (cnt == 8) {
                izE = iz + (dz8 << 3); uzE = uz + (duz << 3); vzE = vz + (dvz << 3);
            } else {
                izE = iz + dz8 * cnt; uzE = uz + duz * cnt; vzE = vz + dvz * cnt;
            }
            int dsE = izE >> 8;
            if (dsE < 1) dsE = 1;
            int uB = uzE / dsE + phU, vB = vzE / dsE + phV;
            if (cnt == 8) {
                du = (uB - uA) << 9;
                dv = (vB - vA) << 9;
            } else {
                du = ((uB - uA) << 12) / cnt;
                dv = ((vB - vA) << 12) / cnt;
            }
            int u = uA << 12, v = vA << 12;

            for (int k = 0; k < cnt; k++) {
                if (iz > zbb[i]) {
                    int col = tex[(((v >> 16) & hm) << ws) | ((u >> 16) & wm)];
                    if (col != 0) {
                        zbb[i] = iz;
                        int colR = (col >> 16) & 255;
                        int colG = (col >> 8) & 255;
                        int colB = col & 255;
                        fbb[i] = ((((colR * sr) >> 8) << 16) |
                                  (((colG * sg) >> 8) << 8) |
                                  (((colB * sb) >> 8))) + add;
                    }
                }
                iz += dz8; u += du; v += dv; i++;
            }
            uA = uB; vA = vB; uz = uzE; vz = vzE;
            n -= cnt;
        }
    }
}

static IRAM_ATTR int clipPlane(R3D* r, int n, float a, float b, float c, float d) {
    int out = 0;
    for (int i = 0; i < n; i++) {
        int j = (i + 1 == n) ? 0 : i + 1;
        float di = a * r->qx[i] + b * r->qy[i] + c * r->qz[i] + d;
        float dj = a * r->qx[j] + b * r->qy[j] + c * r->qz[j] + d;
        if (di >= 0.0f) {
            r->rx[out] = r->qx[i]; r->ry[out] = r->qy[i]; r->rz[out] = r->qz[i];
            r->ru[out] = r->qu[i]; r->rv[out] = r->qv[i];
            out++;
        }
        if ((di >= 0.0f) != (dj >= 0.0f)) {
            float t = di / (di - dj);
            r->rx[out] = r->qx[i] + (r->qx[j] - r->qx[i]) * t;
            r->ry[out] = r->qy[i] + (r->qy[j] - r->qy[i]) * t;
            r->rz[out] = r->qz[i] + (r->qz[j] - r->qz[i]) * t;
            r->ru[out] = r->qu[i] + (r->qu[j] - r->qu[i]) * t;
            r->rv[out] = r->qv[i] + (r->qv[j] - r->qv[i]) * t;
            out++;
        }
    }
    for (int i = 0; i < out; i++) {
        r->qx[i] = r->rx[i]; r->qy[i] = r->ry[i]; r->qz[i] = r->rz[i];
        r->qu[i] = r->ru[i]; r->qv[i] = r->rv[i];
    }
    return out;
}

static IRAM_ATTR void clipTri(R3D* r, int i0, int i1, int i2, int tid,
                    int u0, int v0, int u1, int v1, int u2, int v2,
                    int phU, int phV, int s, int add, int fcol) {
    r->qx[0] = (float)r->vx[i0]; r->qy[0] = (float)r->vy[i0]; r->qz[0] = (float)r->vz[i0]; r->qu[0] = (float)u0; r->qv[0] = (float)v0;
    r->qx[1] = (float)r->vx[i1]; r->qy[1] = (float)r->vy[i1]; r->qz[1] = (float)r->vz[i1]; r->qu[1] = (float)u1; r->qv[1] = (float)v1;
    r->qx[2] = (float)r->vx[i2]; r->qy[2] = (float)r->vy[i2]; r->qz[2] = (float)r->vz[i2]; r->qu[2] = (float)u2; r->qv[2] = (float)v2;
    int n = 3;
    float f = (float)r->focal;
    float gx = (float)r->W * 0.5f, gy = (float)r->H * 0.5f;

    n = clipPlane(r, n, 0.0f, 0.0f, 1.0f, -(float)R3D_NEAR); if (n < 3) return;
    n = clipPlane(r, n, f, 0.0f, (float)r->cx + gx, 0.0f); if (n < 3) return;
    n = clipPlane(r, n, -f, 0.0f, (float)r->W + gx - (float)r->cx, 0.0f); if (n < 3) return;
    n = clipPlane(r, n, 0.0f, -f, (float)r->cy + gy, 0.0f); if (n < 3) return;
    n = clipPlane(r, n, 0.0f, f, (float)r->H + gy - (float)r->cy, 0.0f); if (n < 3) return;

    int sx[10], sy[10], sw[10], su[10], sv[10];
    for (int i = 0; i < n; i++) {
        float z = r->qz[i];
        if (z < (float)R3D_NEAR) z = (float)R3D_NEAR;
        sx[i] = r->cx + Fx_rf(r->qx[i] * f / z);
        sy[i] = r->cy - Fx_rf(r->qy[i] * f / z);
        sw[i] = (int)((float)(1 << 24) / z);
        int uu = (int)r->qu[i], vv = (int)r->qv[i];
        su[i] = uu < 0 ? 0 : (uu > 4095 ? 4095 : uu);
        sv[i] = vv < 0 ? 0 : (vv > 4095 ? 4095 : vv);
    }
    for (int i = 1; i + 1 < n; i++) {
        int a = 0, b = i, c = i + 1;
        int area = (sx[b] - sx[a]) * (sy[c] - sy[a]) - (sx[c] - sx[a]) * (sy[b] - sy[a]);
        if (area == 0) continue;
        r->px[0] = sx[a]; r->py[0] = sy[a]; r->piz[0] = sw[a]; r->pu[0] = su[a]; r->pv[0] = sv[a];
        r->px[1] = sx[b]; r->py[1] = sy[b]; r->piz[1] = sw[b]; r->pu[1] = su[b]; r->pv[1] = sv[b];
        r->px[2] = sx[c]; r->py[2] = sy[c]; r->piz[2] = sw[c]; r->pu[2] = su[c]; r->pv[2] = sv[c];
        if (tid >= 0) {
            if (r->tintC != 0) rasterTexTint(r, tid, s, add, phU, phV, r->tintC);
            else rasterTex(r, tid, s, add, phU, phV);
        } else {
            rasterFlat(r, fcol);
        }
    }
}

IRAM_ATTR void R3D_drawMesh(R3D* r, Mesh* m, const int* mm, int tx, int ty, int tz, int lmx, int lmy, int lmz, int body) {
    if (!m || m->nv == 0) return;
    int bx = m->bx, by = m->by, bz = m->bz, br = m->br;
    int sz = ((mm[6] * bx + mm[7] * by + mm[8] * bz) >> 14) + tz;
    if (sz + br < R3D_NEAR || sz - br > r->farZ) return;
    int sx = ((mm[0] * bx + mm[1] * by + mm[2] * bz) >> 14) + tx;
    int sy = ((mm[3] * bx + mm[4] * by + mm[5] * bz) >> 14) + ty;
    if (sx * r->focal + sz * r->cx < -br * r->radX) return;
    if (-sx * r->focal + sz * (r->W - r->cx) < -br * r->radX) return;
    if (sy * r->focal > sz * r->cy + br * r->radY) return;
    if (-sy * r->focal > sz * (r->H - r->cy) + br * r->radY) return;

#if GTAVC_DETAILED_PROFILE
    int64_t transform_start = rg_system_timer();
#endif
    int nv = m->nv;
    r->verts += nv;
    if (!R3D_ensure(r, nv)) return;
    int* mx = m->x;
    int* my = m->y;
    int* mzz = m->z;
    int m0 = mm[0], m1 = mm[1], m2 = mm[2];
    int m3 = mm[3], m4 = mm[4], m5 = mm[5];
    int m6 = mm[6], m7 = mm[7], m8 = mm[8];
    int f = r->focal, ccx = r->cx, ccy = r->cy;

    /* Select representation once per mesh; projection arithmetic is identical. */
    if (m->vertices16) {
        for (int i = 0; i < nv; i++) {
            const int16_t *v = m->vertices16 + i*5;
            int x = v[0], y = v[1], z = v[2];
            int X = ((m0 * x + m1 * y + m2 * z) >> 14) + tx;
            int Y = ((m3 * x + m4 * y + m5 * z) >> 14) + ty;
            int Z = ((m6 * x + m7 * y + m8 * z) >> 14) + tz;
            r->vx[i] = X; r->vy[i] = Y; r->vz[i] = Z;
            if (Z >= R3D_NEAR) {
                r->viz[i] = (1 << 24) / Z;
                r->vsx[i] = ccx + (X * f) / Z;
                r->vsy[i] = ccy - (Y * f) / Z;
            }
        }
    } else {
        for (int i = 0; i < nv; i++) {
            int x = mx[i], y = my[i], z = mzz[i];
            int X = ((m0 * x + m1 * y + m2 * z) >> 14) + tx;
            int Y = ((m3 * x + m4 * y + m5 * z) >> 14) + ty;
            int Z = ((m6 * x + m7 * y + m8 * z) >> 14) + tz;
            r->vx[i] = X; r->vy[i] = Y; r->vz[i] = Z;
            if (Z >= R3D_NEAR) {
                r->viz[i] = (1 << 24) / Z;
                r->vsx[i] = ccx + (X * f) / Z;
                r->vsy[i] = ccy - (Y * f) / Z;
            }
        }
    }

#if GTAVC_DETAILED_PROFILE
    int64_t transform_end = rg_system_timer();
    GTAVC_PROF(g_prof.t_transform_us += (uint64_t)(transform_end - transform_start));
    GTAVC_PROF(g_prof.vertices += (uint32_t)nv);
    GTAVC_PROF(g_prof.meshes++);
    GTAVC_PROF(g_prof.tri_candidates += (uint32_t)m->nt);
    uint64_t dispatch_us = 0;
#endif
    int* ti = m->ti;
    int nt = m->nt;
    int lim = 2 * r->W;
    int limY = 2 * r->H;
    int fogSpan = r->fogFar - r->fogNear;
    if (fogSpan < 1) fogSpan = 1;

    for (int t = 0; t < nt; t++) {
        int i0, i1, i2;
        if (m->indices16) {
            i0 = m->indices16[t * 3]; i1 = m->indices16[t * 3 + 1]; i2 = m->indices16[t * 3 + 2];
        } else {
            i0 = ti[t * 3]; i1 = ti[t * 3 + 1]; i2 = ti[t * 3 + 2];
        }
        int z0 = r->vz[i0], z1 = r->vz[i1], z2 = r->vz[i2];
        if (z0 < R3D_NEAR && z1 < R3D_NEAR && z2 < R3D_NEAR) continue;

        int fl = m->flag[t];
        if ((fl & 8) != 0 && !r->night) continue;
        if ((fl & 0x80) != 0 && r->night) continue;
        if (z0 > r->farZ && z1 > r->farZ && z2 > r->farZ) continue;

        bool inside = (z0 >= R3D_NEAR && z1 >= R3D_NEAR && z2 >= R3D_NEAR);
        int x0 = r->vsx[i0], x1 = r->vsx[i1], x2 = r->vsx[i2];
        int y0 = r->vsy[i0], y1 = r->vsy[i1], y2 = r->vsy[i2];

        if (inside) {
            if (x0 < 0 && x1 < 0 && x2 < 0) continue;
            if (x0 >= r->W && x1 >= r->W && x2 >= r->W) continue;
            if (y0 < 0 && y1 < 0 && y2 < 0) continue;
            if (y0 >= r->H && y1 >= r->H && y2 >= r->H) continue;
            int area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
            if (area == 0) continue;
            if (area < 0 && (fl & MESH_F_DOUBLE) == 0) continue;
            if (x0 < -lim || x0 > lim || x1 < -lim || x1 > lim || x2 < -lim || x2 > lim ||
                y0 < -limY || y0 > limY || y1 < -limY || y1 > limY || y2 < -limY || y2 > limY) {
                inside = false;
            }
        } else {
            if ((fl & MESH_F_DOUBLE) == 0) {
                int ax = r->vx[i1] - r->vx[i0], ay = r->vy[i1] - r->vy[i0], az = r->vz[i1] - r->vz[i0];
                int bx2 = r->vx[i2] - r->vx[i0], by2 = r->vy[i2] - r->vy[i0], bz2 = r->vz[i2] - r->vz[i0];
                int64_t cX = (int64_t)ay * bz2 - (int64_t)az * by2;
                int64_t cY = (int64_t)az * bx2 - (int64_t)ax * bz2;
                int64_t cZ = (int64_t)ax * by2 - (int64_t)ay * bx2;
                if (cX * -r->vx[i0] + cY * -r->vy[i0] + cZ * -r->vz[i0] <= 0) continue;
            }
        }

        /* Shading */
        int s;
        if ((fl & MESH_F_EMISSIVE) != 0) {
            s = r->emis;
        } else {
            int d = (m->nx[t] * lmx + m->ny[t] * lmy + m->nz[t] * lmz) >> 7;
            if (d < 0) d = 0;
            s = r->amb + ((r->dif * d) >> 14);
            if (s > 256) s = 256;
        }
        int zav = (z0 + z1 + z2) / 3;
        int fg = ((zav - r->fogNear) << 8) / fogSpan;
        if (fg < 0) fg = 0; else if (fg > 256) fg = 256;
        s = (s * (256 - fg)) >> 8;
        int add = ((((r->fogR * fg) >> 8) << 16) |
                   (((r->fogG * fg) >> 8) << 8) |
                   ((r->fogB * fg) >> 8));

        int tid = m->tex[t];
        if (tid >= 0 && r->forceTex >= 0) tid = r->forceTex;

        int u0 = 0, v0 = 0, u1 = 0, v1 = 0, u2 = 0, v2 = 0, phU = 0, phV = 0;
        if (tid >= 0) {
            if (r->T[tid] == NULL) continue;
            if (m->vertices16) {
                u0 = m->vertices16[i0 * 5 + 3]; u1 = m->vertices16[i1 * 5 + 3]; u2 = m->vertices16[i2 * 5 + 3];
                v0 = m->vertices16[i0 * 5 + 4]; v1 = m->vertices16[i1 * 5 + 4]; v2 = m->vertices16[i2 * 5 + 4];
            } else {
                u0 = m->u[i0]; u1 = m->u[i1]; u2 = m->u[i2];
                v0 = m->v[i0]; v1 = m->v[i1]; v2 = m->v[i2];
            }
            int ush = r->uvs[tid];
            if (ush != 0) {
                u0 >>= ush; u1 >>= ush; u2 >>= ush;
                v0 >>= ush; v1 >>= ush; v2 >>= ush;
            }
            if ((fl & MESH_F_WATER) != 0) {
                u0 += r->animU; u1 += r->animU; u2 += r->animU;
                v0 += r->animV; v1 += r->animV; v2 += r->animV;
            }
            int umin = u0 < u1 ? (u0 < u2 ? u0 : u2) : (u1 < u2 ? u1 : u2);
            int vmin = v0 < v1 ? (v0 < v2 ? v0 : v2) : (v1 < v2 ? v1 : v2);
            int umax = u0 > u1 ? (u0 > u2 ? u0 : u2) : (u1 > u2 ? u1 : u2);
            int vmax = v0 > v1 ? (v0 > v2 ? v0 : v2) : (v1 > v2 ? v1 : v2);
            if (umax - umin > 4090 || vmax - vmin > 4090) continue;
            phU = umin & (r->tpu[tid] - 1);
            phV = vmin & (r->tpv[tid] - 1);
            u0 -= umin; u1 -= umin; u2 -= umin;
            v0 -= vmin; v1 -= vmin; v2 -= vmin;
        }

        int fcol = 0;
        r->tintC = 0;
        if (tid >= 0 && (fl & (MESH_F_TINT | MESH_F_TINT2)) != 0) {
            r->tintC = (fl & MESH_F_TINT) != 0 ? (body != 0 ? body : m->col[t]) : r->tint2;
            r->tintC |= 0x1000000;
        }
        if (tid < 0) {
            int c = m->col[t];
            if ((c & MESH_BODY) != 0) {
                c = body;
            } else if ((fl & MESH_F_TINT) != 0) {
                int lum = c & 255;
                c = (((((body >> 16) & 255) * lum / 255) << 16) |
                     ((((body >> 8) & 255) * lum / 255) << 8) |
                     ((body & 255) * lum / 255));
            }
            if ((fl & MESH_F_TINT2) != 0) c = r->tint2;
            fcol = R3D_shadePx(c & 0xFFFFFF, s, add);
        }

        r->tris++;
#if GTAVC_DETAILED_PROFILE
        int64_t dispatch_start = rg_system_timer();
#endif
        if (inside) {
            r->px[0] = x0; r->py[0] = y0; r->piz[0] = r->viz[i0]; r->pu[0] = u0; r->pv[0] = v0;
            r->px[1] = x1; r->py[1] = y1; r->piz[1] = r->viz[i1]; r->pu[1] = u1; r->pv[1] = v1;
            r->px[2] = x2; r->py[2] = y2; r->piz[2] = r->viz[i2]; r->pu[2] = u2; r->pv[2] = v2;
            if (tid >= 0) {
                if (r->tintC != 0) rasterTexTint(r, tid, s, add, phU, phV, r->tintC);
                else rasterTex(r, tid, s, add, phU, phV);
            } else {
                rasterFlat(r, fcol);
            }
        } else {
            clipTri(r, i0, i1, i2, tid, u0, v0, u1, v1, u2, v2, phU, phV, s, add, fcol);
        }
#if GTAVC_DETAILED_PROFILE
        uint64_t duration = (uint64_t)(rg_system_timer() - dispatch_start);
        dispatch_us += duration;
        if (!inside) {
            GTAVC_PROF(g_prof.t_clip_us += duration);
            GTAVC_PROF(g_prof.clipped++);
        } else if (tid < 0) {
            GTAVC_PROF(g_prof.t_flat_us += duration);
        } else if (r->tintC != 0) {
            GTAVC_PROF(g_prof.t_tint_us += duration);
        } else {
            GTAVC_PROF(g_prof.t_tex_us += duration);
        }
#endif
    }
    /* Includes rejection, shading, UV setup and timing bookkeeping; excludes
       transforms and dispatch. Clip time includes its rasterization. */
#if GTAVC_DETAILED_PROFILE
    GTAVC_PROF(g_prof.t_setup_us += (uint64_t)(rg_system_timer() - transform_end) - dispatch_us);
#endif
}

void R3D_free(R3D* r) {
    if (!r) return;
    free(r->fb); r->fb = NULL;
    free(r->zb); r->zb = NULL;
    free(r->vx);  r->vx = NULL;
    free(r->vy);  r->vy = NULL;
    free(r->vz);  r->vz = NULL;
    free(r->vsx); r->vsx = NULL;
    free(r->vsy); r->vsy = NULL;
    free(r->viz); r->viz = NULL;
    r->vxCap = 0;
}
