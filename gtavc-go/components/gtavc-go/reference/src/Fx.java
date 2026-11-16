/** Fixed point helpers. Angles: 0..1023 = full turn. Trig tables in 2.14 (16384 = 1.0). */
final class Fx {
    static final int[] SIN = new int[1024];
    static int seed = 0x2F6E2B1;

    static void init() {
        for (int i = 0; i < 1024; i++) {
            double s = Math.sin(i * Math.PI * 2.0 / 1024.0) * 16384.0;
            SIN[i] = (int) (s >= 0 ? s + 0.5 : s - 0.5);
        }
        initAtan();
    }
    static int sin(int a) { return SIN[a & 1023]; }
    static int cos(int a) { return SIN[(a + 256) & 1023]; }

    static int rnd() {
        seed = seed * 1103515245 + 12345;
        return (seed >>> 16) & 0x7FFF;
    }
    static int rnd(int n) { return n <= 0 ? 0 : rnd() % n; }
    static int rndRange(int a, int b) { return a + rnd(b - a + 1); }

    /** yaw convention: 0 = +z, 256 = +x  (direction = (sin, cos)) */
    static final int[] ATAN = new int[258];   // tan ratio*256 -> angle (0..128)
    static void initAtan() {
        int k = 0;
        for (int i = 0; i <= 256; i++) {
            while (k < 128 && SIN[k + 1] * 256 / cosv(k + 1) < i) k++;
            ATAN[i] = k;
        }
    }
    private static int cosv(int a) { return SIN[(a + 256) & 1023]; }
    static int yawOf(int dx, int dz) {
        if (dx == 0 && dz == 0) return 0;
        int ax = dx < 0 ? -dx : dx, az = dz < 0 ? -dz : dz, ang;
        if (az >= ax) ang = ATAN[(ax * 256) / az];
        else ang = 256 - ATAN[(az * 256) / ax];
        if (dx >= 0) return dz >= 0 ? ang : 512 - ang;
        return dz < 0 ? 512 + ang : (1024 - ang) & 1023;
    }
    /** round float to nearest int (CLDC has no Math.round) */
    static int rf(float f) { return (int) Math.floor(f + 0.5); }
    static int dist(int dx, int dz) {
        return (int) Math.sqrt((double) dx * (double) dx + (double) dz * (double) dz);
    }
    static int angDiff(int a, int b) {   // shortest signed difference b-a in -512..511
        int d = (b - a) & 1023;
        return d >= 512 ? d - 1024 : d;
    }
    static int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
    static int abs(int v) { return v < 0 ? -v : v; }

    // ---- 3x3 matrices, 2.14, row major
    static void ident(int[] m) { m[0] = 16384; m[1] = 0; m[2] = 0; m[3] = 0; m[4] = 16384; m[5] = 0; m[6] = 0; m[7] = 0; m[8] = 16384; }
    static void rotX(int[] m, int a) { int c = cos(a), s = sin(a); m[0] = 16384; m[1] = 0; m[2] = 0; m[3] = 0; m[4] = c; m[5] = -s; m[6] = 0; m[7] = s; m[8] = c; }
    static void rotY(int[] m, int a) { int c = cos(a), s = sin(a); m[0] = c; m[1] = 0; m[2] = s; m[3] = 0; m[4] = 16384; m[5] = 0; m[6] = -s; m[7] = 0; m[8] = c; }
    static void rotZ(int[] m, int a) { int c = cos(a), s = sin(a); m[0] = c; m[1] = -s; m[2] = 0; m[3] = s; m[4] = c; m[5] = 0; m[6] = 0; m[7] = 0; m[8] = 16384; }
    /** o = a * b  (o must not alias a or b) */
    static void mul(int[] a, int[] b, int[] o) {
        o[0] = (a[0] * b[0] + a[1] * b[3] + a[2] * b[6]) >> 14;
        o[1] = (a[0] * b[1] + a[1] * b[4] + a[2] * b[7]) >> 14;
        o[2] = (a[0] * b[2] + a[1] * b[5] + a[2] * b[8]) >> 14;
        o[3] = (a[3] * b[0] + a[4] * b[3] + a[5] * b[6]) >> 14;
        o[4] = (a[3] * b[1] + a[4] * b[4] + a[5] * b[7]) >> 14;
        o[5] = (a[3] * b[2] + a[4] * b[5] + a[5] * b[8]) >> 14;
        o[6] = (a[6] * b[0] + a[7] * b[3] + a[8] * b[6]) >> 14;
        o[7] = (a[6] * b[1] + a[7] * b[4] + a[8] * b[7]) >> 14;
        o[8] = (a[6] * b[2] + a[7] * b[5] + a[8] * b[8]) >> 14;
    }
}
