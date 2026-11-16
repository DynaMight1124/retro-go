/** Flat-shaded triangle mesh. Model units: 1/64 m. UV: 1/16 texel. */
final class Mesh {
    static final int BODY = 0x1000000;       // colour marker: use instance body colour
    static final int F_DOUBLE = 1, F_EMISSIVE = 2, F_WATER = 4, F_TINT = 16, F_GLASS = 32, F_TINT2 = 64;

    int nv, nt;
    int[] x, y, z, u, v;
    int[] ti;
    short[] tex;
    int[] col;
    byte[] flag;
    byte[] nx, ny, nz;
    int bx, by, bz, br;

    void bounds() {
        if (nv == 0) return;
        int x0 = x[0], x1 = x[0], y0 = y[0], y1 = y[0], z0 = z[0], z1 = z[0];
        for (int i = 1; i < nv; i++) {
            if (x[i] < x0) x0 = x[i]; if (x[i] > x1) x1 = x[i];
            if (y[i] < y0) y0 = y[i]; if (y[i] > y1) y1 = y[i];
            if (z[i] < z0) z0 = z[i]; if (z[i] > z1) z1 = z[i];
        }
        bx = (x0 + x1) >> 1; by = (y0 + y1) >> 1; bz = (z0 + z1) >> 1;
        int dx = (x1 - x0) >> 1, dy = (y1 - y0) >> 1, dz = (z1 - z0) >> 1;
        br = (int) Math.sqrt((double) dx * dx + (double) dy * dy + (double) dz * dz) + 2;
    }
}
