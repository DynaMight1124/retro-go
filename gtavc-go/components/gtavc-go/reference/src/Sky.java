import java.io.IOException;

/** Vice City sky: timecyc gradient, drifting clouds, sun + corona, moon, stars, lens flare and the lighthouse glow. */
final class Sky {
    static int[] flash, smoke, fire, explo, tgt; static int sFlash, sSmoke, sFire, sExplo, sTgt;
    static int[] cloud1, cloud2, cloud3, hilit, corona, star, moon, ringa, hex, circle, reflect, streek;
    static int sCloud, sCorona, sStar, sMoon, sRing, sHex, sCircle, sReflect, sStreek;
    static int sunX, sunY, sunZ;              // direction towards the sun, 2.14
    private static final int[] dm = new int[2];
    private static final int NSTAR = 70, NCLOUD = 11;
    private static final int[] starA = new int[NSTAR], starE = new int[NSTAR], starB = new int[NSTAR];
    private static final int[] cloA = new int[NCLOUD], cloE = new int[NCLOUD], cloS = new int[NCLOUD], cloK = new int[NCLOUD];
    private static int sunSX = -9999, sunSY, sunVis;

    static void load() throws IOException {
        cloud1 = Assets.intensity("/fx_cloud1.png", dm); sCloud = dm[0];
        cloud2 = Assets.intensity("/fx_cloud2.png", dm);
        cloud3 = Assets.intensity("/fx_cloud3.png", dm);
        hilit = Assets.intensity("/fx_cloudhilit.png", dm);
        corona = Assets.intensity("/fx_corona.png", dm); sCorona = dm[0];
        star = Assets.intensity("/fx_star.png", dm); sStar = dm[0];
        moon = Assets.intensity("/fx_moon.png", dm); sMoon = dm[0];
        ringa = Assets.intensity("/fx_ringa.png", dm); sRing = dm[0];
        hex = Assets.intensity("/fx_hex.png", dm); sHex = dm[0];
        circle = Assets.intensity("/fx_circle.png", dm); sCircle = dm[0];
        reflect = Assets.intensity("/fx_reflect.png", dm); sReflect = dm[0];
        streek = Assets.intensity("/fx_streek.png", dm); sStreek = dm[0];
        flash = Assets.intensity("/fx_flash.png", dm); sFlash = dm[0];
        smoke = Assets.intensity("/fx_smoke.png", dm); sSmoke = dm[0];
        fire = Assets.intensity("/fx_fire.png", dm); sFire = dm[0];
        explo = Assets.intensity("/fx_explo.png", dm); sExplo = dm[0];
        tgt = Assets.intensity("/fx_target.png", dm); sTgt = dm[0];
        int seed = 1234567;
        for (int i = 0; i < NSTAR; i++) {
            seed = seed * 1103515245 + 12345; starA[i] = (seed >>> 8) & 1023;
            seed = seed * 1103515245 + 12345; starE[i] = 20 + ((seed >>> 8) % 210);
            seed = seed * 1103515245 + 12345; starB[i] = 90 + ((seed >>> 8) % 160);
        }
        for (int i = 0; i < NCLOUD; i++) {
            seed = seed * 1103515245 + 12345; cloA[i] = i * 1024 / NCLOUD + ((seed >>> 8) % 50);
            seed = seed * 1103515245 + 12345; cloE[i] = 8 + ((seed >>> 8) % 34);
            seed = seed * 1103515245 + 12345; cloS[i] = 70 + ((seed >>> 8) % 70);
            cloK[i] = i % 3;
        }
    }

    /** sun path: rises in the east at 06:00, sets in the west at 20:00, leaning south */
    static void sunDir(World w) {
        int t = w.minutes;
        int th = (t - 360) * 512 / 840;                    // 0 .. 512 over the day
        sunX = Fx.cos(th) * 14500 >> 14;
        sunY = Fx.sin(th) * 14500 >> 14;
        sunZ = -6500;
    }

    // ------------------------------------------------------------------ projection
    private static int px, py, pz;
    /** direction (2.14) -> screen; false if behind */
    private static boolean proj(R3D r, int dx, int dy, int dz) {
        int[] m = r.mv;
        int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14, vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14, vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
        if (vz < 1200) return false;
        px = r.cx + vx * r.focal / vz; py = r.cy - vy * r.focal / vz; pz = vz;
        return true;
    }
    private static boolean projAngle(R3D r, int az, int el) {
        int ce = Fx.cos(el);
        return proj(r, (Fx.sin(az) * ce) >> 14, Fx.sin(el), (Fx.cos(az) * ce) >> 14);
    }

    // ------------------------------------------------------------------ sprites
    /** mode 0: additive, 1: alpha blend with intensity as alpha */
    static void spr(int[] fb, int W, int H, int[] tex, int ts, int cx, int cy, int w, int h, int col, int mode, int mul) {
        if (w < 2 || h < 2 || mul <= 0) return;
        int x0 = cx - w / 2, y0 = cy - h / 2;
        int xs = x0 < 0 ? 0 : x0, ys = y0 < 0 ? 0 : y0, xe = x0 + w > W ? W : x0 + w, ye = y0 + h > H ? H : y0 + h;
        if (xs >= xe || ys >= ye) return;
        int cr = (col >> 16) & 255, cg = (col >> 8) & 255, cb = col & 255;
        int stepX = (ts << 16) / w, stepY = (ts << 16) / h;
        for (int y = ys; y < ye; y++) {
            int ty = ((y - y0) * stepY) >> 16;
            int row = ty * ts, i = y * W + xs, u = (xs - x0) * stepX;
            for (int x = xs; x < xe; x++, i++, u += stepX) {
                int a = tex[row + (u >> 16)];
                if (a < 6) continue;
                a = a * mul >> 8;
                int d = fb[i];
                if (mode == 0) {
                    int rr = ((d >> 16) & 255) + (cr * a >> 8), gg = ((d >> 8) & 255) + (cg * a >> 8), bb = (d & 255) + (cb * a >> 8);
                    fb[i] = ((rr > 255 ? 255 : rr) << 16) | ((gg > 255 ? 255 : gg) << 8) | (bb > 255 ? 255 : bb);
                } else {
                    if (a > 255) a = 255;
                    fb[i] = Ui.blend(d, col, a);
                }
            }
        }
    }

    /** muzzle flash at a world point (1/64 m), drawn straight into the frame */
    static void flash(R3D r, int x, int y, int z) {
        int dx = x - r.camX, dy = y - r.camY, dz = z - r.camZ;
        int[] m = r.mv;
        int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14, vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14, vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
        if (vz < 64) return;
        int s = r.focal * 40 / vz + 4;
        spr(r.fb, r.W, r.H, flash, sFlash, r.cx + vx * r.focal / vz, r.cy - vy * r.focal / vz, s, s, 0xFFD890, 0, 256);
    }

    /** world-space particle sprite; size in 1/64 m */
    static void particle(R3D r, int type, int x, int y, int z, int size, int fade) {
        int dx = x - r.camX, dy = y - r.camY, dz = z - r.camZ;
        int[] m = r.mv;
        int vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
        if (vz < 80 || vz > r.farZ) return;
        int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14, vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14;
        int sx = r.cx + vx * r.focal / vz, sy = r.cy - vy * r.focal / vz, s = size * r.focal / vz;
        if (s < 2) return;
        if (sx >= 0 && sy >= 0 && sx < r.W && sy < r.H && r.zb[sy * r.W + sx] > ((1 << 24) / vz) * 260) return;   // behind a wall
        if (type == 0) spr(r.fb, r.W, r.H, smoke, sSmoke, sx, sy, s, s, 0x606060, 1, fade * 3 / 4);
        else if (type == 1) spr(r.fb, r.W, r.H, fire, sFire, sx, sy, s, s * 5 / 4, 0xFF9030, 0, fade);
        else spr(r.fb, r.W, r.H, explo, sExplo, sx, sy, s, s, 0xFFB050, 0, fade);
    }

    /** PS2-style lock-on reticle over the target; green -> yellow -> red with its health */
    static void target(R3D r, int x, int y, int z, int hp, int maxHp, int time) {
        int dx = x - r.camX, dy = y - r.camY, dz = z - r.camZ;
        int[] m = r.mv;
        int vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
        if (vz < 80) return;
        int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14, vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14;
        int sx = r.cx + vx * r.focal / vz, sy = r.cy - vy * r.focal / vz;
        int s = r.focal * 70 / vz + 10 + ((Fx.sin(time / 2) * 3) >> 14);
        int f = hp * 255 / Math.max(1, maxHp);
        if (f < 0) f = 0; if (f > 255) f = 255;
        int col = f > 128 ? (((255 - f) * 2) << 16) | 0xFF00 : 0xFF0000 | ((f * 2) << 8);
        spr(r.fb, r.W, r.H, tgt, sTgt, sx, sy, s, s, col, 1, 900);
        spr(r.fb, r.W, r.H, tgt, sTgt, sx, sy, s, s, col, 0, 256);
    }

    static int mix(int a, int b, int t) {
        int ia = 256 - t;
        return ((((a >> 16) & 255) * ia + ((b >> 16) & 255) * t) >> 8) << 16 | ((((a >> 8) & 255) * ia + ((b >> 8) & 255) * t) >> 8) << 8 | (((a & 255) * ia + (b & 255) * t) >> 8);
    }

    // ------------------------------------------------------------------ sky pass (before geometry)
    static void drawSky(World w, R3D r) {
        int W = r.W, H = r.H, f = r.focal;
        int[] fb = r.fb;
        int mins = w.minutes;
        boolean night = w.night;
        // stars + moon
        if (night || mins < 6 * 60 + 30 || mins > 19 * 60 + 30) {
            int vis = night ? 256 : 110;
            for (int i = 0; i < NSTAR; i++) {
                if (!projAngle(r, starA[i], starE[i])) continue;
                if (px < 1 || py < 1 || px >= W - 1 || py >= H - 1 || py > r.horizon) continue;
                int tw = starB[i] + ((Fx.sin(w.time / 3 + i * 97) * 40) >> 14);
                int b = tw * vis >> 8;
                int k = py * W + px;
                fb[k] = Ui.blend(fb[k], 0xFFFFFF, b > 255 ? 255 : b);
                if (starB[i] > 210) { fb[k + 1] = Ui.blend(fb[k + 1], 0xC8D8FF, b / 3); fb[k - 1] = Ui.blend(fb[k - 1], 0xC8D8FF, b / 3); }
            }
            if (proj(r, -sunX, -sunY, -sunZ) || proj(r, -sunX, 9000, -sunZ)) {
                int s = f / 7;
                spr(fb, W, H, moon, sMoon, px, py, s, s, 0xE8ECFF, 0, 256);
                spr(fb, W, H, corona, sCorona, px, py, s * 3, s * 3, 0x30384C, 0, 256);
            }
        }
        // clouds
        int skyB = r.skyBot;
        int ccol, cmul, hiCol = 0;
        if (night) { ccol = mix(skyB, 0x6070A0, 60); cmul = 140; }
        else if (mins < 8 * 60 || mins > 18 * 60) { ccol = mix(0xFFFFFF, skyB, 110); cmul = 210; hiCol = mins > 12 * 60 ? 0xFF7830 : 0xFFA060; }
        else { ccol = mix(0xFFFFFF, skyB, 50); cmul = 230; }
        int drift = w.time / 260;
        for (int i = 0; i < NCLOUD; i++) {
            if (!projAngle(r, (cloA[i] + drift) & 1023, cloE[i])) continue;
            int cw = f * cloS[i] / 100, ch = cw / 2;
            if (px + cw / 2 < 0 || px - cw / 2 >= W || py + ch / 2 < 0 || py - ch / 2 >= H) continue;
            int[] t = cloK[i] == 0 ? cloud1 : (cloK[i] == 1 ? cloud2 : cloud3);
            spr(fb, W, H, t, sCloud, px, py, cw, ch, ccol, 1, cmul);
            if (hiCol != 0) spr(fb, W, H, hilit, sCloud, px, py + ch / 6, cw, ch, hiCol, 0, 150);
        }
        // sun disc + glow (geometry is drawn over it)
        sunVis = 0;
        if (sunY > -1500 && proj(r, sunX, sunY, sunZ)) {
            int low = sunY < 5000 ? 256 - sunY * 256 / 5000 : 0;
            int core = mix(0xFFF6D0, 0xFF8C30, low), glow = mix(0x80703C, 0xA04818, low);
            spr(fb, W, H, corona, sCorona, px, py, f * 2 / 3, f * 2 / 3, glow, 0, 256);
            spr(fb, W, H, corona, sCorona, px, py, f / 5, f / 5, core, 0, 256);
            spr(fb, W, H, corona, sCorona, px, py, f / 10, f / 10, 0xFFFFFF, 0, 256);
            sunSX = px; sunSY = py; sunVis = 1;
        }
    }

    // ------------------------------------------------------------------ post pass (after geometry)
    static final int[] FL_POS = { 28, 55, 82, 118, 150, 185 };
    static final int[] FL_SIZE = { 9, 5, 14, 7, 20, 11 };
    static final int[] FL_COL = { 0x5A4020, 0x30483A, 0x203C50, 0x502838, 0x284020, 0x4A3018 };

    static void drawPost(World w, R3D r) {
        int W = r.W, H = r.H, f = r.focal;
        int[] fb = r.fb, zb = r.zb;
        // lighthouse glow at night
        if (w.night) {
            int dx = (w.lampX >> 4) - r.camX, dy = w.lampY - r.camY, dz = (w.lampZ >> 4) - r.camZ;
            int[] m = r.mv;
            int vx = (m[0] * dx + m[1] * dy + m[2] * dz) >> 14, vy = (m[3] * dx + m[4] * dy + m[5] * dz) >> 14, vz = (m[6] * dx + m[7] * dy + m[8] * dz) >> 14;
            if (vz > 64 && vz < 30000) {
                int sx = r.cx + vx * f / vz, sy = r.cy - vy * f / vz;
                if (sx >= 0 && sy >= 0 && sx < W && sy < H) {
                    int iz = ((1 << 24) / vz) << 8;
                    if (zb[sy * W + sx] <= iz + (iz >> 4)) {
                        int pulse = 180 + ((Fx.sin(w.time / 8) * 76) >> 14);
                        int s = f * 160 / (vz / 8 + 64) + 8;
                        spr(fb, W, H, corona, sCorona, sx, sy, s, s, 0xFFE8A0, 0, pulse);
                        spr(fb, W, H, star, sStar, sx, sy, s * 2, s * 2, 0x806838, 0, pulse);
                    }
                }
            }
        }
        // lens flare when the sun is not hidden
        if (sunVis == 0 || sunSX < 0 || sunSY < 0 || sunSX >= W || sunSY >= H) return;
        if (zb[sunSY * W + sunSX] != 0) return;
        int low = sunY < 5000 ? 256 - sunY * 256 / 5000 : 0;
        int vecX = r.cx - sunSX, vecY = r.cy - sunSY;
        int fade = 256 - (Fx.abs(vecX) + Fx.abs(vecY)) * 256 / (W + H);
        if (fade < 40) fade = 40;
        spr(fb, W, H, streek, sStreek, sunSX, sunSY, f * 3 / 2, f / 9, mix(0x605838, 0x803818, low), 0, fade);
        for (int k = 0; k < FL_POS.length; k++) {
            int x = sunSX + vecX * FL_POS[k] / 100, y = sunSY + vecY * FL_POS[k] / 100;
            int s = f * FL_SIZE[k] / 100;
            int[] t = k % 3 == 0 ? hex : (k % 3 == 1 ? circle : ringa);
            int ts = k % 3 == 0 ? sHex : (k % 3 == 1 ? sCircle : sRing);
            spr(fb, W, H, t, ts, x, y, s, s, FL_COL[k], 0, fade);
        }
        spr(fb, W, H, reflect, sReflect, sunSX + vecX * 2, sunSY + vecY * 2, f / 4, f / 4, 0x302418, 0, fade);
    }
}
