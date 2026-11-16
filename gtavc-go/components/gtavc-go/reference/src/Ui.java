import java.io.*;
import javax.microedition.lcdui.*;

/** Bitmap font, HUD sprites, menus and radar - everything is drawn straight into an int[] screen buffer. */
final class Ui {
    static final int CW = 14, CH = 16, AW = 95 * CW;
    static byte[] fa = new byte[AW * CH], faMain, faTip;
    static int[] gw = new int[95], gwMain, gwTip;
    static int[] hud = new int[128 * 64];
    static int[] logo, bg;
    static int logoW, logoH, bgW, bgH;
    static int[][] scr = new int[5][];        // intro, ls0..ls3 (320x240)
    static final int PINK = 0xF890E0, GREEN = 0x32D88A, BLUE = 0x5AB4F0, WHITE = 0xFFFFFF, HILITE = 0x178A47;
    private static final int[] dm = new int[2];

    // ------------------------------------------------------------ loading
    static void loadStep(int i) throws IOException {
        switch (i) {
            case 0: scr[0] = Assets.imgPix("/intro.png", dm); break;
            case 1: scr[1] = Assets.imgPix("/ls0.png", dm); break;
            case 2: loadFont(); break;
            case 3: hud = Assets.imgPix("/hud.png", dm); break;
            case 4: logo = Assets.imgPix("/logo.png", dm); logoW = dm[0]; logoH = dm[1]; break;
            case 5: bg = Assets.imgPix("/menu_bg.png", dm); bgW = dm[0]; bgH = dm[1]; loadRadar(); break;
            case 6: try { scr[2] = Assets.imgPix("/ls1.png", dm); } catch (IOException e) { } break;
            case 7: try { scr[3] = Assets.imgPix("/ls2.png", dm); } catch (IOException e) { } break;
            case 8: try { scr[4] = Assets.imgPix("/ls3.png", dm); } catch (IOException e) { } break;
            default: break;
        }
    }
    static final int UI_STEPS = 9;

    static void loadFont() throws IOException {
        if (faMain != null) return;
        loadFontFile("/font.png");
        faMain = fa; gwMain = gw;
        try { fa = new byte[AW * CH]; gw = new int[95]; loadFontFile("/font_tip.png"); faTip = fa; gwTip = gw; }
        catch (Exception e) { faTip = faMain; gwTip = gwMain; }
        fa = faMain; gw = gwMain;
    }
    static void tipFont(boolean on) { fa = on ? faTip : faMain; gw = on ? gwTip : gwMain; }

    static void loadFontFile(String res) throws IOException {
        int[] p = Assets.imgPix(res, dm);
        for (int i = 0; i < p.length; i++) fa[i] = (byte) (p[i] >>> 24);
        for (int g = 0; g < 95; g++) {
            int w = 0;
            for (int x = 0; x < CW; x++)
                for (int y = 0; y < CH; y++)
                    if ((fa[y * AW + g * CW + x] & 255) > 40) { w = x + 1; break; }
            gw[g] = g == 0 ? 5 : (w < 3 ? 3 : w);
        }
    }

    // ------------------------------------------------------------ primitives
    static int blend(int d, int c, int a) {
        int ia = 255 - a;
        return (((((d & 0xFF00FF) * ia) + ((c & 0xFF00FF) * a)) >>> 8) & 0xFF00FF) | (((((d & 0xFF00) * ia) + ((c & 0xFF00) * a)) >>> 8) & 0xFF00);
    }

    static int textW(String s, int sc) {
        int w = 0;
        for (int i = 0; i < s.length(); i++) {
            int c = s.charAt(i) - 32;
            if (c < 0 || c > 94) c = 0;
            w += (gw[c] + 1) * sc;
        }
        return w;
    }

    static void glyph(int[] b, int bw, int bh, int x, int y, int g, int col, int sc, int alphaMul) {
        int w = gw[g];
        for (int gy = 0; gy < CH; gy++) {
            for (int gx = 0; gx < w; gx++) {
                int a = fa[gy * AW + g * CW + gx] & 255;
                if (a == 0) continue;
                if (alphaMul != 255) a = a * alphaMul / 255;
                for (int sy = 0; sy < sc; sy++) {
                    int yy = y + gy * sc + sy;
                    if (yy < 0 || yy >= bh) continue;
                    for (int sx = 0; sx < sc; sx++) {
                        int xx = x + gx * sc + sx;
                        if (xx < 0 || xx >= bw) continue;
                        int i = yy * bw + xx;
                        b[i] = a >= 250 ? col : blend(b[i], col, a);
                    }
                }
            }
        }
    }

    /** draws with a dark drop shadow */
    static void text(int[] b, int bw, int bh, int x, int y, String s, int col, int sc) {
        int cx = x, cy = y;
        for (int pass = 0; pass < 2; pass++) {
            cx = x;
            for (int i = 0; i < s.length(); i++) {
                int c = s.charAt(i) - 32;
                if (c < 0 || c > 94) c = 0;
                if (pass == 0) glyph(b, bw, bh, cx + sc, cy + sc, c, 0x101018, sc, 210);
                else glyph(b, bw, bh, cx, cy, c, col, sc, 255);
                cx += (gw[c] + 1) * sc;
            }
        }
    }
    static void textC(int[] b, int bw, int bh, int cxm, int y, String s, int col, int sc) {
        text(b, bw, bh, cxm - textW(s, sc) / 2, y, s, col, sc);
    }
    static void textR(int[] b, int bw, int bh, int xr, int y, String s, int col, int sc) {
        text(b, bw, bh, xr - textW(s, sc), y, s, col, sc);
    }

    static void fill(int[] b, int bw, int bh, int x, int y, int w, int h, int col) {
        for (int j = y < 0 ? 0 : y; j < y + h && j < bh; j++)
            for (int i = x < 0 ? 0 : x; i < x + w && i < bw; i++) b[j * bw + i] = col;
    }
    static void fillA(int[] b, int bw, int bh, int x, int y, int w, int h, int col, int a) {
        for (int j = y < 0 ? 0 : y; j < y + h && j < bh; j++)
            for (int i = x < 0 ? 0 : x; i < x + w && i < bw; i++) b[j * bw + i] = blend(b[j * bw + i], col, a);
    }

    /** nearest-neighbour stretch of a sw x sh image to the whole destination */
    static void stretch(int[] src, int sw, int sh, int[] dst, int dw, int dh, int fade) {
        for (int y = 0; y < dh; y++) {
            int sy = y * sh / dh;
            int so = sy * sw, dof = y * dw;
            for (int x = 0; x < dw; x++) {
                int c = src[so + x * sw / dw] & 0xFFFFFF;
                if (fade < 256) c = ((((c & 0xFF00FF) * fade) >>> 8) & 0xFF00FF) | ((((c & 0xFF00) * fade) >>> 8) & 0xFF00);
                dst[dof + x] = c;
            }
        }
    }

    /** blit sprite from sheet with alpha, integer scale */
    static void sprite(int[] b, int bw, int bh, int[] sheet, int sheetW, int sx, int sy, int w, int h, int dx, int dy, int sc) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int c = sheet[(sy + y) * sheetW + sx + x];
                int a = c >>> 24;
                if (a < 20) continue;
                for (int j = 0; j < sc; j++) {
                    int yy = dy + y * sc + j;
                    if (yy < 0 || yy >= bh) continue;
                    for (int i = 0; i < sc; i++) {
                        int xx = dx + x * sc + i;
                        if (xx < 0 || xx >= bw) continue;
                        b[yy * bw + xx] = a > 235 ? (c & 0xFFFFFF) : blend(b[yy * bw + xx], c & 0xFFFFFF, a);
                    }
                }
            }
        }
    }

    static void logoAt(int[] b, int bw, int bh, int x, int y, int sc) {
        sprite(b, bw, bh, logo, logoW, 0, 0, logoW, logoH, x, y, sc);
    }

    // ------------------------------------------------------------ menu chrome
    static void menuBg(int[] b, int bw, int bh, int u) {
        stretch(bg, bgW, bgH, b, bw, bh, 256);
        logoAt(b, bw, bh, 4 * u, 3 * u, u);
    }

    static void menuTitle(int[] b, int bw, int bh, int u, String t) {
        textR(b, bw, bh, bw - 6 * u, 4 * u, t, PINK, u + (u > 1 ? 0 : 0));
    }

    static void menuItem(int[] b, int bw, int bh, int u, int cx, int y, String s, boolean sel) {
        int w = textW(s, u);
        if (sel) fill(b, bw, bh, cx - w / 2 - 5 * u, y - 1 * u, w + 10 * u, 14 * u, HILITE);
        text(b, bw, bh, cx - w / 2, y, s, PINK, u);
    }

    static void bars(int[] b, int bw, int bh, int u, int xr, int y, int val, int max) {
        for (int i = 0; i < 15; i++) {
            int h = (2 + i / 2 + (i >> 1)) * u / 2 + u;
            fill(b, bw, bh, xr - (15 - i) * 3 * u, y + 11 * u - h, 2 * u, h, i < val ? 0x4CB8F0 : 0x14304C);
        }
    }

    // ------------------------------------------------------------ HUD
    static void hudDraw(int[] b, int bw, int bh, World w, int u, boolean radar, int radarMode) {
        int right = bw - 5 * u;
        // weapon / fist box
        sprite(b, bw, bh, hud, 128, 0, 20, 38, 30, right - 38 * u, 4 * u, u);
        if (w.weapon > 0) {
            fillA(b, bw, bh, right - 36 * u, 6 * u, 34 * u, 26 * u, 0x101018, 200);
            textC(b, bw, bh, right - 19 * u, 8 * u, w.weapon == 1 ? "bat" : "colt", WHITE, u);
            if (w.weapon == 2) textC(b, bw, bh, right - 19 * u, 19 * u, String.valueOf(w.ammo), BLUE, u);
        }
        int tx = right - 42 * u;
        textR(b, bw, bh, tx, 3 * u, w.clockStr(), BLUE, u);
        String m = String.valueOf(w.money);
        while (m.length() < 8) m = "0" + m;
        textR(b, bw, bh, tx, 3 * u + 13 * u, "$" + m, GREEN, u);
        int hy = 3 * u + 26 * u;
        String hp = String.valueOf(w.health);
        textR(b, bw, bh, tx, hy, hp, PINK, u);
        sprite(b, bw, bh, hud, 128, 0, 0, 14, 13, tx - textW(hp, u) - 16 * u, hy, u);
        if (w.armor > 0) {
            String ar = String.valueOf(w.armor);
            textR(b, bw, bh, tx - textW(hp, u) - 22 * u, hy, ar, 0xC8C8D0, u);
        }
        for (int i = 0; i < 6; i++) {
            sprite(b, bw, bh, hud, 128, i < w.wanted ? 36 : 16, 0, 18, 18, tx - (6 - i) * 17 * u + 4 * u, hy + 14 * u, u);
        }
        if (radar && radarMode != 2) radarDraw(b, bw, bh, w, 4 * u + 30 * u, bh - 4 * u - 30 * u, 30 * u, u, radarMode);
    }

    static final int[] MAPPAL = { 0x4A86C8, 0xE6D9A6, 0x56A456, 0xB4B4BC, 0x2C2C30, 0x8C8478, 0xDCDCE4, 0x000000 };

    static int[] radImg; static int radW, radH, radX0, radZ0, radCell;
    static void loadRadar() {
        try {
            int[] d = new int[2];
            radImg = Assets.imgPix("/radar.png", d); radW = d[0]; radH = d[1];
            String[] l = Assets.readLines("/radar.txt");
            String t = l[0]; int a = t.indexOf(' '), b = t.indexOf(' ', a + 1);
            radX0 = Integer.parseInt(t.substring(0, a)); radZ0 = Integer.parseInt(t.substring(a + 1, b)); radCell = Integer.parseInt(t.substring(b + 1).trim());
        } catch (Exception e) { radImg = null; }
    }
    /** colour of the radar picture at a world position (north up) */
    static int radarCol(World w, int xm, int zm) {
        if (radImg != null) {
            int i = (xm - radX0) / radCell, j = (radZ0 - zm) / radCell;
            if (xm < radX0 || zm > radZ0 || i >= radW || j >= radH) return 0x78BEF0;
            return radImg[j * radW + i] & 0xFFFFFF;
        }
        int i = (xm - w.rx0) / w.rcell, j = (zm - w.rz0) / w.rcell;
        if (xm < w.rx0 || zm < w.rz0 || i >= w.rw || j >= w.rh) return MAPPAL[0];
        return MAPPAL[w.rad[j * w.rw + i] & 7];
    }

    static void radarDraw(int[] b, int bw, int bh, World w, int cx, int cy, int R, int u, int mode) {
        int th = w.camYaw, cs = Fx.cos(th), sn = Fx.sin(th);
        int mpp = 66 * 1024 / R;                                  // mm per pixel
        for (int dy = -R; dy <= R; dy++) {
            for (int dx = -R; dx <= R; dx++) {
                int d2 = dx * dx + dy * dy;
                if (d2 > R * R) continue;
                int xx = cx + dx, yy = cy + dy;
                if (xx < 0 || yy < 0 || xx >= bw || yy >= bh) continue;
                int col;
                if (mode == 1) col = 0x202028;
                else {
                    int wx = w.player.x + (((cs * dx - sn * dy) >> 7) * mpp >> 7);
                    int wz = w.player.z - (((sn * dx + cs * dy) >> 7) * mpp >> 7);
                    col = radarCol(w, wx, wz);
                }
                b[yy * bw + xx] = col;
            }
        }
        // blips
        int mmpp = 66 * 1024 / R;
        // like VC: no ped / car blips, only the cops while you are wanted
        if (w.wanted > 0) {
            for (int i = 0; i < w.nped; i++) if (w.peds[i].cop) blip(b, bw, bh, w, w.peds[i].seat != null ? w.peds[i].seat.x : w.peds[i].x, w.peds[i].seat != null ? w.peds[i].seat.z : w.peds[i].z, cx, cy, R, cs, sn, mmpp, (w.time / 250) % 2 == 0 ? 0x2040FF : 0xFF2020, u, w.peds[i].state == Ped.DEAD);
        }
        // player arrow
        int pyaw = (w.inCar != null ? w.inCar.yaw : w.player.yaw) - th;
        int ax = Fx.sin(pyaw), az = -Fx.cos(pyaw);
        for (int k = -3 * u; k <= 5 * u; k++) {
            int ww = (5 * u - k) / 3;
            for (int s = -ww; s <= ww; s++) {
                int ox = (ax * k >> 14) + (az * s >> 14) * -1 + 0, oy = (az * k >> 14) + (ax * s >> 14);
                int xx = cx + ox, yy = cy + oy;
                if (xx >= 0 && yy >= 0 && xx < bw && yy < bh) b[yy * bw + xx] = 0xFFFFFF;
            }
        }
        // ring
        for (int dy = -R - 2 * u; dy <= R + 2 * u; dy++) {
            for (int dx = -R - 2 * u; dx <= R + 2 * u; dx++) {
                int d2 = dx * dx + dy * dy;
                if (d2 >= R * R && d2 <= (R + 2 * u) * (R + 2 * u)) {
                    int xx = cx + dx, yy = cy + dy;
                    if (xx >= 0 && yy >= 0 && xx < bw && yy < bh) b[yy * bw + xx] = 0xFFA0E8;
                }
            }
        }
        // north marker
        int nx = cx - ((sn * (R)) >> 14), ny = cy - ((cs * (R)) >> 14);
        fill(b, bw, bh, nx - 4 * u, ny - 4 * u, 8 * u, 8 * u, 0x101018);
        text(b, bw, bh, nx - 3 * u, ny - 6 * u, "N", 0xFFFFFF, 1);
    }

    private static void blip(int[] b, int bw, int bh, World w, int x, int z, int cx, int cy, int R, int cs, int sn, int mmpp, int col, int u, boolean dead) {
        if (dead) return;
        int dx = x - w.player.x, dz = z - w.player.z;
        if (dx > 62000 || dx < -62000 || dz > 62000 || dz < -62000) return;
        int right = ((dx >> 2) * cs - (dz >> 2) * sn) >> 12;
        int fwd = ((dx >> 2) * sn + (dz >> 2) * cs) >> 12;
        int px = cx + right / mmpp, py = cy - fwd / mmpp;
        int ddx = px - cx, ddy = py - cy;
        if (ddx * ddx + ddy * ddy > (R - 2) * (R - 2)) return;
        fill(b, bw, bh, px - u / 2, py - u / 2, u + 1, u + 1, col);
    }

    static void helpBox(int[] b, int bw, int bh, int u, String s) {
        tipFont(true);
        // word wrap into a translucent box at the top left
        int maxW = bw * 6 / 10;
        String[] lines = new String[6];
        int n = 0;
        String cur = "";
        int i = 0;
        while (i <= s.length() && n < 6) {
            int j = s.indexOf(' ', i);
            if (j < 0) j = s.length();
            String word = s.substring(i, j);
            String t = cur.length() == 0 ? word : cur + " " + word;
            if (textW(t, u) > maxW && cur.length() > 0) { lines[n++] = cur; cur = word; }
            else cur = t;
            i = j + 1;
        }
        if (cur.length() > 0 && n < 6) lines[n++] = cur;
        int w = 0;
        for (int k = 0; k < n; k++) { int tw = textW(lines[k], u); if (tw > w) w = tw; }
        fillA(b, bw, bh, 4 * u, 4 * u, w + 8 * u, n * 13 * u + 5 * u, 0x000000, 170);
        for (int k = 0; k < n; k++) text(b, bw, bh, 8 * u, 6 * u + k * 13 * u, lines[k], 0xF0F0F0, u);
        tipFont(false);
    }
}
