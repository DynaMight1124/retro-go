/**
 * Pure-integer (setup in float) software renderer for CLDC.
 * World/view units: 1/64 m.  View space: x right, y up, z forward.
 * Front faces are those whose screen area > 0 (see MeshBuilder winding rule).
 */
final class R3D {
    static final int NEAR = 64;

    int W, H, cx, cy, focal;
    int[] fb, zb;

    // textures (ARGB, 0 = transparent)
    int[][] T = new int[1024][];
    int[] uvs = new int[1024];     // UV right-shift (ped meshes are authored for 128px atlases)
    int[] tws = new int[1024], twm = new int[1024], thm = new int[1024], tpu = new int[1024], tpv = new int[1024];

    // camera
    int camX, camY, camZ;
    int[] mv = new int[9];
    int horizon;

    // lighting / fog
    int lx = 5000, ly = 12000, lz = -6000;     // world light direction (towards the light), 2.14
    int amb = 256, dif = 0;
    int fogR = 200, fogG = 200, fogB = 220, fogNear = 2000, fogFar = 8000;
    int farZ = 9000;
    int skyTop = 0x4aa0e0, skyBot = 0xbfe0f0, groundCol = 0x606058;
    int emis = 256;
    int animU, animV;
    int forceTex = -1;
    boolean blend;                 // translucent flat faces (glass): 50% blend, no depth write
    boolean additive;
    boolean night;                 // map faces flagged night-only / day-only (VC time objects)              // additive flat faces (light beams): no depth write
    int tint2 = 0xFFFFFF;          // secondary car colour

    // scratch
    int[] vx = new int[1024], vy = new int[1024], vz = new int[1024], vsx = new int[1024], vsy = new int[1024], viz = new int[1024];
    int[] px = new int[3], py = new int[3], piz = new int[3], pu = new int[3], pv = new int[3];
    float[] qx = new float[10], qy = new float[10], qz = new float[10], qu = new float[10], qv = new float[10];
    float[] rx = new float[10], ry = new float[10], rz = new float[10], ru = new float[10], rv = new float[10];
    int radX, radY;
    int tris;

    void resize(int w, int h, int fovFocal) {
        W = w; H = h; cx = w >> 1; cy = h >> 1;
        focal = fovFocal;
        fb = new int[w * h];
        zb = new int[w * h];
        radX = (int) Math.sqrt((double) focal * focal + (double) cx * cx) + 1;
        radY = (int) Math.sqrt((double) focal * focal + (double) cy * cy) + 1;
    }

    void setTexture(int id, int[] pix, int w, int h) {
        T[id] = pix;
        int s = 0;
        while ((1 << s) < w) s++;
        tws[id] = s; twm[id] = w - 1; thm[id] = h - 1; tpu[id] = w * 16; tpv[id] = h * 16;
    }

    void setCamera(int x, int y, int z, int yaw, int pitch) {
        camX = x; camY = y; camZ = z;
        int cth = Fx.cos(yaw), sth = Fx.sin(yaw), cph = Fx.cos(pitch), sph = Fx.sin(pitch);
        mv[0] = cth; mv[1] = 0; mv[2] = -sth;
        mv[3] = (sth * sph) >> 14; mv[4] = cph; mv[5] = (cth * sph) >> 14;
        mv[6] = (sth * cph) >> 14; mv[7] = -sph; mv[8] = (cth * cph) >> 14;
        int c = cph < 64 ? 64 : cph;
        horizon = cy - (focal * sph) / c;
    }

    /** clear colour+depth with a sky gradient */
    void clear() {
        int h = horizon;
        if (h < 0) h = 0;
        if (h > H) h = H;
        int i = 0;
        int tr = (skyTop >> 16) & 255, tg = (skyTop >> 8) & 255, tb = skyTop & 255;
        int br = (skyBot >> 16) & 255, bg = (skyBot >> 8) & 255, bb = skyBot & 255;
        for (int y = 0; y < H; y++) {
            int c;
            if (y >= h + 2) c = groundCol; else if (y >= h) c = skyBot;
            else {
                int t = h == 0 ? 256 : (y * 256) / h;
                c = (((tr + (((br - tr) * t) >> 8)) << 16) | ((tg + (((bg - tg) * t) >> 8)) << 8) | (tb + (((bb - tb) * t) >> 8)));
            }
            for (int x = 0; x < W; x++) { fb[i] = c; zb[i] = 0; i++; }
        }
        tris = 0;
    }

    private void ensure(int n) {
        if (n > vx.length) {
            int m = n + 256;
            vx = new int[m]; vy = new int[m]; vz = new int[m]; vsx = new int[m]; vsy = new int[m]; viz = new int[m];
        }
    }

    /**
     * mm: model->view rotation (2.14). (tx,ty,tz): view-space translation of model origin.
     * (lmx,lmy,lmz): light direction in model space (2.14).  body: colour for Mesh.BODY faces.
     */
    void drawMesh(Mesh m, int[] mm, int tx, int ty, int tz, int lmx, int lmy, int lmz, int body) {
        int bx = m.bx, by = m.by, bz = m.bz, br = m.br;
        int sz = ((mm[6] * bx + mm[7] * by + mm[8] * bz) >> 14) + tz;
        if (sz + br < NEAR || sz - br > farZ) return;
        int sx = ((mm[0] * bx + mm[1] * by + mm[2] * bz) >> 14) + tx;
        int sy = ((mm[3] * bx + mm[4] * by + mm[5] * bz) >> 14) + ty;
        if (sx * focal + sz * cx < -br * radX) return;
        if (-sx * focal + sz * (W - cx) < -br * radX) return;
        if (sy * focal > sz * cy + br * radY) return;
        if (-sy * focal > sz * (H - cy) + br * radY) return;

        int nv = m.nv;
        ensure(nv);
        int[] mx = m.x, my = m.y, mzz = m.z;
        int m0 = mm[0], m1 = mm[1], m2 = mm[2], m3 = mm[3], m4 = mm[4], m5 = mm[5], m6 = mm[6], m7 = mm[7], m8 = mm[8];
        int f = focal, ccx = cx, ccy = cy;
        for (int i = 0; i < nv; i++) {
            int x = mx[i], y = my[i], z = mzz[i];
            int X = ((m0 * x + m1 * y + m2 * z) >> 14) + tx;
            int Y = ((m3 * x + m4 * y + m5 * z) >> 14) + ty;
            int Z = ((m6 * x + m7 * y + m8 * z) >> 14) + tz;
            vx[i] = X; vy[i] = Y; vz[i] = Z;
            if (Z >= NEAR) {
                viz[i] = (1 << 24) / Z;
                vsx[i] = ccx + (X * f) / Z;
                vsy[i] = ccy - (Y * f) / Z;
            }
        }

        int[] ti = m.ti;
        int nt = m.nt;
        int lim = 2 * W;
        int limY = 2 * H;
        int fogSpan = fogFar - fogNear;
        if (fogSpan < 1) fogSpan = 1;
        for (int t = 0; t < nt; t++) {
            int i0 = ti[t * 3], i1 = ti[t * 3 + 1], i2 = ti[t * 3 + 2];
            int z0 = vz[i0], z1 = vz[i1], z2 = vz[i2];
            if (z0 < NEAR && z1 < NEAR && z2 < NEAR) continue;
            int fl = m.flag[t];
            if ((fl & 8) != 0 && !night) continue;
            if ((fl & 0x80) != 0 && night) continue;
            // far reject
            if (z0 > farZ && z1 > farZ && z2 > farZ) continue;
            boolean inside = z0 >= NEAR && z1 >= NEAR && z2 >= NEAR;
            int x0 = vsx[i0], x1 = vsx[i1], x2 = vsx[i2], y0 = vsy[i0], y1 = vsy[i1], y2 = vsy[i2];
            if (inside) {
                if (x0 < 0 && x1 < 0 && x2 < 0) continue;
                if (x0 >= W && x1 >= W && x2 >= W) continue;
                if (y0 < 0 && y1 < 0 && y2 < 0) continue;
                if (y0 >= H && y1 >= H && y2 >= H) continue;
                int area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
                if (area == 0) continue;
                if (area < 0 && (fl & Mesh.F_DOUBLE) == 0) continue;
                if (x0 < -lim || x0 > lim || x1 < -lim || x1 > lim || x2 < -lim || x2 > lim
                        || y0 < -limY || y0 > limY || y1 < -limY || y1 > limY || y2 < -limY || y2 > limY) inside = false;
            } else {
                // cheap backface test in view space for partially clipped triangles
                if ((fl & Mesh.F_DOUBLE) == 0) {
                    int ax = vx[i1] - vx[i0], ay = vy[i1] - vy[i0], az = vz[i1] - vz[i0];
                    int bx2 = vx[i2] - vx[i0], by2 = vy[i2] - vy[i0], bz2 = vz[i2] - vz[i0];
                    float cX = (float) ay * bz2 - (float) az * by2;
                    float cY = (float) az * bx2 - (float) ax * bz2;
                    float cZ = (float) ax * by2 - (float) ay * bx2;
                    // front-facing when cross points toward the viewer
                    if (cX * (float) -vx[i0] + cY * (float) -vy[i0] + cZ * (float) -vz[i0] <= 0) continue;
                }
            }

            // ---- shading
            int s;
            if ((fl & Mesh.F_EMISSIVE) != 0) s = emis;
            else {
                int d = (m.nx[t] * lmx + m.ny[t] * lmy + m.nz[t] * lmz) >> 7;
                if (d < 0) d = 0;
                s = amb + ((dif * d) >> 14);
                if (s > 256) s = 256;
            }
            int zav = (z0 + z1 + z2) / 3;
            int fg = ((zav - fogNear) << 8) / fogSpan;
            if (fg < 0) fg = 0; else if (fg > 256) fg = 256;
            s = (s * (256 - fg)) >> 8;
            int add = ((((fogR * fg) >> 8) << 16) | (((fogG * fg) >> 8) << 8) | ((fogB * fg) >> 8));

            int tid = m.tex[t];
            if (tid >= 0 && forceTex >= 0) tid = forceTex;
            int u0 = 0, v0 = 0, u1 = 0, v1 = 0, u2 = 0, v2 = 0, phU = 0, phV = 0;
            if (tid >= 0) {
                if (T[tid] == null) continue;
                u0 = m.u[i0]; u1 = m.u[i1]; u2 = m.u[i2];
                v0 = m.v[i0]; v1 = m.v[i1]; v2 = m.v[i2];
                int ush = uvs[tid];
                if (ush != 0) { u0 >>= ush; u1 >>= ush; u2 >>= ush; v0 >>= ush; v1 >>= ush; v2 >>= ush; }
                if ((fl & Mesh.F_WATER) != 0) { u0 += animU; u1 += animU; u2 += animU; v0 += animV; v1 += animV; v2 += animV; }
                int umin = u0 < u1 ? (u0 < u2 ? u0 : u2) : (u1 < u2 ? u1 : u2);
                int vmin = v0 < v1 ? (v0 < v2 ? v0 : v2) : (v1 < v2 ? v1 : v2);
                int umax = u0 > u1 ? (u0 > u2 ? u0 : u2) : (u1 > u2 ? u1 : u2);
                int vmax = v0 > v1 ? (v0 > v2 ? v0 : v2) : (v1 > v2 ? v1 : v2);
                if (umax - umin > 4090 || vmax - vmin > 4090) continue;
                phU = umin & (tpu[tid] - 1);
                phV = vmin & (tpv[tid] - 1);
                u0 -= umin; u1 -= umin; u2 -= umin; v0 -= vmin; v1 -= vmin; v2 -= vmin;
            }
            int fcol = 0;
            tintC = 0;
            if (tid >= 0 && (fl & (Mesh.F_TINT | Mesh.F_TINT2)) != 0) { tintC = (fl & Mesh.F_TINT) != 0 ? (body != 0 ? body : m.col[t]) : tint2; tintC |= 0x1000000; }
            if (tid < 0) {
                int c = m.col[t];
                if ((c & Mesh.BODY) != 0) c = body;
                else if ((fl & Mesh.F_TINT) != 0) {
                    int lum = c & 255;
                    c = ((((body >> 16) & 255) * lum / 255) << 16) | ((((body >> 8) & 255) * lum / 255) << 8) | ((body & 255) * lum / 255);
                }
                if ((fl & Mesh.F_TINT2) != 0) c = tint2;
                fcol = shadePx(c & 0xFFFFFF, s, add);
            }
            tris++;
            if (inside) {
                px[0] = x0; py[0] = y0; piz[0] = viz[i0]; pu[0] = u0; pv[0] = v0;
                px[1] = x1; py[1] = y1; piz[1] = viz[i1]; pu[1] = u1; pv[1] = v1;
                px[2] = x2; py[2] = y2; piz[2] = viz[i2]; pu[2] = u2; pv[2] = v2;
                if (tid >= 0) { if (tintC != 0) rasterTexTint(tid, s, add, phU, phV, tintC); else rasterTex(tid, s, add, phU, phV); } else rasterFlat(fcol);
            } else {
                clipTri(i0, i1, i2, tid, u0, v0, u1, v1, u2, v2, phU, phV, s, add, fcol);
            }
        }
    }

    static int shadePx(int c, int s, int add) {
        return ((((c & 0xFF00FF) * s) >>> 8) & 0xFF00FF | (((c & 0xFF00) * s) >>> 8) & 0xFF00) + add;
    }

    // ------------------------------------------------------------------ clipping (float, rare path)
    private int tintC;

    private int clipPlane(int n, int plane, float a, float b, float c, float d) {
        // keep points where a*x+b*y+c*z+d >= 0 ; writes result into r*, returns count
        int out = 0;
        for (int i = 0; i < n; i++) {
            int j = i + 1 == n ? 0 : i + 1;
            float di = a * qx[i] + b * qy[i] + c * qz[i] + d;
            float dj = a * qx[j] + b * qy[j] + c * qz[j] + d;
            if (di >= 0) {
                rx[out] = qx[i]; ry[out] = qy[i]; rz[out] = qz[i]; ru[out] = qu[i]; rv[out] = qv[i]; out++;
            }
            if ((di >= 0) != (dj >= 0)) {
                float t = di / (di - dj);
                rx[out] = qx[i] + (qx[j] - qx[i]) * t; ry[out] = qy[i] + (qy[j] - qy[i]) * t; rz[out] = qz[i] + (qz[j] - qz[i]) * t;
                ru[out] = qu[i] + (qu[j] - qu[i]) * t; rv[out] = qv[i] + (qv[j] - qv[i]) * t; out++;
            }
        }
        float[] t1;
        t1 = qx; qx = rx; rx = t1; t1 = qy; qy = ry; ry = t1; t1 = qz; qz = rz; rz = t1;
        t1 = qu; qu = ru; ru = t1; t1 = qv; qv = rv; rv = t1;
        return out;
    }

    private void clipTri(int i0, int i1, int i2, int tid, int u0, int v0, int u1, int v1, int u2, int v2,
                         int phU, int phV, int s, int add, int fcol) {
        qx[0] = vx[i0]; qy[0] = vy[i0]; qz[0] = vz[i0]; qu[0] = u0; qv[0] = v0;
        qx[1] = vx[i1]; qy[1] = vy[i1]; qz[1] = vz[i1]; qu[1] = u1; qv[1] = v1;
        qx[2] = vx[i2]; qy[2] = vy[i2]; qz[2] = vz[i2]; qu[2] = u2; qv[2] = v2;
        int n = 3;
        float f = focal;
        float gx = W / 2f, gy = H / 2f;
        n = clipPlane(n, 0, 0f, 0f, 1f, -(float) NEAR); if (n < 3) return;
        n = clipPlane(n, 1, f, 0f, cx + gx, 0f); if (n < 3) return;
        n = clipPlane(n, 2, -f, 0f, W + gx - cx, 0f); if (n < 3) return;
        n = clipPlane(n, 3, 0f, -f, cy + gy, 0f); if (n < 3) return;
        n = clipPlane(n, 4, 0f, f, H + gy - cy, 0f); if (n < 3) return;
        // project
        int[] sx = new int[n], sy = new int[n], sw = new int[n], su = new int[n], sv = new int[n];
        for (int i = 0; i < n; i++) {
            float z = qz[i];
            if (z < NEAR) z = NEAR;
            sx[i] = cx + Fx.rf(qx[i] * f / z);
            sy[i] = cy - Fx.rf(qy[i] * f / z);
            sw[i] = (int) ((float) (1 << 24) / z);
            int uu = (int) qu[i], vv = (int) qv[i];
            su[i] = uu < 0 ? 0 : (uu > 4095 ? 4095 : uu);
            sv[i] = vv < 0 ? 0 : (vv > 4095 ? 4095 : vv);
        }
        for (int i = 1; i + 1 < n; i++) {
            int a = 0, b = i, c = i + 1;
            int area = (sx[b] - sx[a]) * (sy[c] - sy[a]) - (sx[c] - sx[a]) * (sy[b] - sy[a]);
            if (area == 0) continue;
            px[0] = sx[a]; py[0] = sy[a]; piz[0] = sw[a]; pu[0] = su[a]; pv[0] = sv[a];
            px[1] = sx[b]; py[1] = sy[b]; piz[1] = sw[b]; pu[1] = su[b]; pv[1] = sv[b];
            px[2] = sx[c]; py[2] = sy[c]; piz[2] = sw[c]; pu[2] = su[c]; pv[2] = sv[c];
            if (tid >= 0) { if (tintC != 0) rasterTexTint(tid, s, add, phU, phV, tintC); else rasterTex(tid, s, add, phU, phV); } else rasterFlat(fcol);
        }
    }

    // ------------------------------------------------------------------ rasteriser
    private void rasterFlat(int color) {
        int a = 0, b = 1, c = 2, t;
        if (py[a] > py[b]) { t = a; a = b; b = t; }
        if (py[b] > py[c]) { t = b; b = c; c = t; }
        if (py[a] > py[b]) { t = a; a = b; b = t; }
        int xa = px[a], ya = py[a], xb = px[b], yb = py[b], xc = px[c], yc = py[c];
        if (ya == yc || yc < 0 || ya >= H) return;
        int dx1 = xb - xa, dy1 = yb - ya, dx2 = xc - xa, dy2 = yc - ya;
        int area = dx1 * dy2 - dx2 * dy1;
        if (area == 0) return;
        float inv = 1f / area;
        float iza = piz[a];
        float dzdx = ((piz[b] - piz[a]) * (float) dy2 - (piz[c] - piz[a]) * (float) dy1) * inv;
        float dzdy = ((piz[c] - piz[a]) * (float) dx1 - (piz[b] - piz[a]) * (float) dx2) * inv;
        int dz8 = (int) (dzdx * 256f);
        int dxl = (dx2 << 16) / dy2;
        int dxab = dy1 > 0 ? (dx1 << 16) / dy1 : 0;
        int dybc = yc - yb;
        int dxbc = dybc > 0 ? ((xc - xb) << 16) / dybc : 0;
        int ys = ya < 0 ? 0 : ya;
        int ye = yc > H ? H : yc;
        int[] fbb = fb, zbb = zb;
        for (int y = ys; y < ye; y++) {
            int xl = (xa << 16) + dxl * (y - ya);
            int xs2 = y < yb ? (xa << 16) + dxab * (y - ya) : (xb << 16) + dxbc * (y - yb);
            int xL, xR;
            if (xs2 < xl) { xL = xs2; xR = xl; } else { xL = xl; xR = xs2; }
            int xs = (xL + 0xFFFF) >> 16, xe = (xR + 0xFFFF) >> 16;
            if (xs < 0) xs = 0;
            if (xe > W) xe = W;
            if (xs >= xe) continue;
            int iz = (int) ((iza + dzdx * (xs - xa) + dzdy * (y - ya)) * 256f);
            int i = y * W + xs;
            for (int n = xe - xs; n > 0; n--) {
                if (iz > zbb[i]) {
                    if (additive) { int d = fbb[i], rb = (d & 0xFF00FF) + (color & 0xFF00FF), g = (d & 0xFF00) + (color & 0xFF00);
                        if ((rb & 0x1000000) != 0) rb |= 0xFF0000; if ((rb & 0x100) != 0) rb |= 0xFF; if ((g & 0x10000) != 0) g = 0xFF00;
                        fbb[i] = (rb & 0xFF00FF) | (g & 0xFF00); }
                    else if (blend) fbb[i] = (((fbb[i] >> 1) & 0x7F7F7F) + ((color >> 1) & 0x7F7F7F)); else { zbb[i] = iz; fbb[i] = color; } }
                iz += dz8;
                i++;
            }
        }
    }

    private void rasterTex(int tid, int s, int add, int phU, int phV) {
        int a = 0, b = 1, c = 2, t;
        if (py[a] > py[b]) { t = a; a = b; b = t; }
        if (py[b] > py[c]) { t = b; b = c; c = t; }
        if (py[a] > py[b]) { t = a; a = b; b = t; }
        int xa = px[a], ya = py[a], xb = px[b], yb = py[b], xc = px[c], yc = py[c];
        if (ya == yc || yc < 0 || ya >= H) return;
        int dx1 = xb - xa, dy1 = yb - ya, dx2 = xc - xa, dy2 = yc - ya;
        int area = dx1 * dy2 - dx2 * dy1;
        if (area == 0) return;
        float inv = 1f / area;
        float iza = piz[a];
        float uza = (float) pu[a] * piz[a], vza = (float) pv[a] * piz[a];
        float uzb = (float) pu[b] * piz[b], vzb = (float) pv[b] * piz[b];
        float uzc = (float) pu[c] * piz[c], vzc = (float) pv[c] * piz[c];
        float fdy1 = dy1, fdy2 = dy2, fdx1 = dx1, fdx2 = dx2;
        float dzdx = ((piz[b] - piz[a]) * fdy2 - (piz[c] - piz[a]) * fdy1) * inv;
        float dzdy = ((piz[c] - piz[a]) * fdx1 - (piz[b] - piz[a]) * fdx2) * inv;
        float dudx = ((uzb - uza) * fdy2 - (uzc - uza) * fdy1) * inv;
        float dudy = ((uzc - uza) * fdx1 - (uzb - uza) * fdx2) * inv;
        float dvdx = ((vzb - vza) * fdy2 - (vzc - vza) * fdy1) * inv;
        float dvdy = ((vzc - vza) * fdx1 - (vzb - vza) * fdx2) * inv;
        if (dudx > 1e8f || dudx < -1e8f || dvdx > 1e8f || dvdx < -1e8f) return;
        int dz8 = (int) (dzdx * 256f);
        int duz = (int) dudx, dvz = (int) dvdx;
        int dxl = (dx2 << 16) / dy2;
        int dxab = dy1 > 0 ? (dx1 << 16) / dy1 : 0;
        int dybc = yc - yb;
        int dxbc = dybc > 0 ? ((xc - xb) << 16) / dybc : 0;
        int ys = ya < 0 ? 0 : ya;
        int ye = yc > H ? H : yc;
        int[] fbb = fb, zbb = zb, tex = T[tid];
        int ws = tws[tid], wm = twm[tid], hm = thm[tid];
        for (int y = ys; y < ye; y++) {
            int xl = (xa << 16) + dxl * (y - ya);
            int xs2 = y < yb ? (xa << 16) + dxab * (y - ya) : (xb << 16) + dxbc * (y - yb);
            int xL, xR;
            if (xs2 < xl) { xL = xs2; xR = xl; } else { xL = xl; xR = xs2; }
            int xs = (xL + 0xFFFF) >> 16, xe = (xR + 0xFFFF) >> 16;
            if (xs < 0) xs = 0;
            if (xe > W) xe = W;
            if (xs >= xe) continue;
            float fx = xs - xa, fy = y - ya;
            int iz = (int) ((iza + dzdx * fx + dzdy * fy) * 256f);
            int uz = (int) (uza + dudx * fx + dudy * fy);
            int vz = (int) (vza + dvdx * fx + dvdy * fy);
            int i = y * W + xs;
            int n = xe - xs;
            int ds = iz >> 8;
            if (ds < 1) ds = 1;
            int uA = uz / ds + phU, vA = vz / ds + phV;
            while (n > 0) {
                int cnt = n < 8 ? n : 8;
                int izE = iz + dz8 * cnt, uzE = uz + duz * cnt, vzE = vz + dvz * cnt;
                int dsE = izE >> 8;
                if (dsE < 1) dsE = 1;
                int uB = uzE / dsE + phU, vB = vzE / dsE + phV;
                int du = ((uB - uA) << 12) / cnt, dv = ((vB - vA) << 12) / cnt;
                int u = uA << 12, v = vA << 12;
                for (int k = 0; k < cnt; k++) {
                    if (iz > zbb[i]) {
                        int col = tex[(((v >> 16) & hm) << ws) | ((u >> 16) & wm)];
                        if (col != 0) {
                            zbb[i] = iz;
                            fbb[i] = ((((col & 0xFF00FF) * s) >>> 8) & 0xFF00FF | (((col & 0xFF00) * s) >>> 8) & 0xFF00) + add;
                        }
                    }
                    iz += dz8; u += du; v += dv; i++;
                }
                uA = uB; vA = vB; uz = uzE; vz = vzE;
                n -= cnt;
            }
        }
    }

    private void rasterTexTint(int tid, int s, int add, int phU, int phV, int tc) {
        int sr = ((tc >> 16) & 255) * s >> 8, sg = ((tc >> 8) & 255) * s >> 8, sb = (tc & 255) * s >> 8;
        int a = 0, b = 1, c = 2, t;
        if (py[a] > py[b]) { t = a; a = b; b = t; }
        if (py[b] > py[c]) { t = b; b = c; c = t; }
        if (py[a] > py[b]) { t = a; a = b; b = t; }
        int xa = px[a], ya = py[a], xb = px[b], yb = py[b], xc = px[c], yc = py[c];
        if (ya == yc || yc < 0 || ya >= H) return;
        int dx1 = xb - xa, dy1 = yb - ya, dx2 = xc - xa, dy2 = yc - ya;
        int area = dx1 * dy2 - dx2 * dy1;
        if (area == 0) return;
        float inv = 1f / area;
        float iza = piz[a];
        float uza = (float) pu[a] * piz[a], vza = (float) pv[a] * piz[a];
        float uzb = (float) pu[b] * piz[b], vzb = (float) pv[b] * piz[b];
        float uzc = (float) pu[c] * piz[c], vzc = (float) pv[c] * piz[c];
        float fdy1 = dy1, fdy2 = dy2, fdx1 = dx1, fdx2 = dx2;
        float dzdx = ((piz[b] - piz[a]) * fdy2 - (piz[c] - piz[a]) * fdy1) * inv;
        float dzdy = ((piz[c] - piz[a]) * fdx1 - (piz[b] - piz[a]) * fdx2) * inv;
        float dudx = ((uzb - uza) * fdy2 - (uzc - uza) * fdy1) * inv;
        float dudy = ((uzc - uza) * fdx1 - (uzb - uza) * fdx2) * inv;
        float dvdx = ((vzb - vza) * fdy2 - (vzc - vza) * fdy1) * inv;
        float dvdy = ((vzc - vza) * fdx1 - (vzb - vza) * fdx2) * inv;
        if (dudx > 1e8f || dudx < -1e8f || dvdx > 1e8f || dvdx < -1e8f) return;
        int dz8 = (int) (dzdx * 256f);
        int duz = (int) dudx, dvz = (int) dvdx;
        int dxl = (dx2 << 16) / dy2;
        int dxab = dy1 > 0 ? (dx1 << 16) / dy1 : 0;
        int dybc = yc - yb;
        int dxbc = dybc > 0 ? ((xc - xb) << 16) / dybc : 0;
        int ys = ya < 0 ? 0 : ya;
        int ye = yc > H ? H : yc;
        int[] fbb = fb, zbb = zb, tex = T[tid];
        int ws = tws[tid], wm = twm[tid], hm = thm[tid];
        for (int y = ys; y < ye; y++) {
            int xl = (xa << 16) + dxl * (y - ya);
            int xs2 = y < yb ? (xa << 16) + dxab * (y - ya) : (xb << 16) + dxbc * (y - yb);
            int xL, xR;
            if (xs2 < xl) { xL = xs2; xR = xl; } else { xL = xl; xR = xs2; }
            int xs = (xL + 0xFFFF) >> 16, xe = (xR + 0xFFFF) >> 16;
            if (xs < 0) xs = 0;
            if (xe > W) xe = W;
            if (xs >= xe) continue;
            float fx = xs - xa, fy = y - ya;
            int iz = (int) ((iza + dzdx * fx + dzdy * fy) * 256f);
            int uz = (int) (uza + dudx * fx + dudy * fy);
            int vz = (int) (vza + dvdx * fx + dvdy * fy);
            int i = y * W + xs;
            int n = xe - xs;
            int ds = iz >> 8;
            if (ds < 1) ds = 1;
            int uA = uz / ds + phU, vA = vz / ds + phV;
            while (n > 0) {
                int cnt = n < 8 ? n : 8;
                int izE = iz + dz8 * cnt, uzE = uz + duz * cnt, vzE = vz + dvz * cnt;
                int dsE = izE >> 8;
                if (dsE < 1) dsE = 1;
                int uB = uzE / dsE + phU, vB = vzE / dsE + phV;
                int du = ((uB - uA) << 12) / cnt, dv = ((vB - vA) << 12) / cnt;
                int u = uA << 12, v = vA << 12;
                for (int k = 0; k < cnt; k++) {
                    if (iz > zbb[i]) {
                        int col = tex[(((v >> 16) & hm) << ws) | ((u >> 16) & wm)];
                        if (col != 0) {
                            zbb[i] = iz;
                            fbb[i] = (((((col >> 16) & 255) * sr) >> 8) << 16 | ((((col >> 8) & 255) * sg) >> 8) << 8 | (((col & 255) * sb) >> 8)) + add;
                        }
                    }
                    iz += dz8; u += du; v += dv; i++;
                }
                uA = uB; vA = vB; uz = uzE; vz = vzE;
                n -= cnt;
            }
        }
    }
}
