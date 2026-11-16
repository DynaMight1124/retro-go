import java.io.*;

/** A converted Vice City vehicle (car_<name>.bin, CAR2): chassis + detachable parts with ok / damaged variants. */
final class CarModel {
    static final int P_CHASSIS = 0, P_BONNET = 1, P_BOOT = 2, P_DOOR_LF = 3, P_DOOR_RF = 4, P_DOOR_LR = 5, P_DOOR_RR = 6,
            P_WING_LF = 7, P_WING_RF = 8, P_BUMP_F = 9, P_BUMP_R = 10, P_WINDSCREEN = 11;

    int np;
    int[] partId;
    Mesh[][] ok, dam, okGlass, damGlass;     // [part] -> body / glass meshes
    Mesh far, farGlass;
    int[] wx = new int[4], wy = new int[4], wz = new int[4], wr = new int[4];
    int nw, dimX, dimY, dimZ, seatX, seatY, seatZ;
    boolean bike;
    String name;
    int[] tex;
    int mass, grip, cls;           // handling class: 0 car, 1 sports, 2 heavy, 3 bike, 4 slow
    // handling
    int vmax, accel, brake, lockDeg, wheelbase;
    static Mesh wheel;

    static CarModel load(String name) throws IOException {
        DataInputStream d = new DataInputStream(new ByteArrayInputStream(Assets.readAll("/car_" + name + ".bin")));
        byte[] mg = new byte[4]; d.readFully(mg);
        CarModel c = new CarModel();
        c.name = name;
        int[] texId = readTex(d);
        c.tex = texId;
        c.dimX = d.readShort(); c.dimY = d.readShort(); c.dimZ = d.readShort();
        c.seatX = d.readShort(); c.seatY = d.readShort(); c.seatZ = d.readShort();
        c.nw = d.readUnsignedByte();
        for (int i = 0; i < c.nw; i++) { int x = d.readShort(), y = d.readShort(), z = d.readShort(), r = d.readShort(); if (i < 4) { c.wx[i] = x; c.wy[i] = y; c.wz[i] = z; c.wr[i] = r; } }
        if (c.nw > 4) c.nw = 4;
        c.bike = d.readUnsignedByte() != 0;
        c.np = d.readUnsignedByte();
        c.partId = new int[c.np];
        c.ok = new Mesh[c.np][]; c.dam = new Mesh[c.np][];
        for (int p = 0; p < c.np; p++) {
            c.partId[p] = d.readUnsignedByte();
            d.readShort(); d.readShort(); d.readShort();
            int nv = d.readUnsignedByte();
            Mesh[] a = readMesh(d, texId);
            c.ok[p] = a;
            c.dam[p] = nv > 1 ? readMesh(d, texId) : a;
        }
        Mesh[] f = readMesh(d, texId);
        c.far = f[0]; c.farGlass = f[1];
        d.close();
        if (wheel == null) wheel = loadWheel();
        return c;
    }

    static int[] readTex(DataInputStream d) throws IOException {
        int nt = d.readUnsignedByte();
        int[] texId = new int[nt];
        for (int i = 0; i < nt; i++) {
            byte[] b = new byte[d.readUnsignedByte()];
            d.readFully(b);
            texId[i] = Assets.acquire("/t_" + new String(b) + ".png");
        }
        return texId;
    }
    void unload() { if (tex != null) for (int i = 0; i < tex.length; i++) Assets.release(tex[i]); tex = null; }

    static Mesh loadWheel() throws IOException {
        DataInputStream d = new DataInputStream(new ByteArrayInputStream(Assets.readAll("/wheel.bin")));
        int[] t = readTex(d);
        Mesh m = readMesh(d, t)[0];
        d.close();
        return m;
    }

    /** returns {body, glass-or-null} */
    static Mesh[] readMesh(DataInputStream d, int[] texId) throws IOException {
        int nv = d.readUnsignedShort();
        int[] x = new int[nv], y = new int[nv], z = new int[nv], u = new int[nv], v = new int[nv];
        for (int i = 0; i < nv; i++) { x[i] = d.readShort(); y[i] = d.readShort(); z[i] = d.readShort(); u[i] = d.readShort(); v[i] = d.readShort(); }
        int nt = d.readUnsignedShort();
        int[] ta = new int[nt], tb = new int[nt], tc = new int[nt], tex = new int[nt], fl = new int[nt], col = new int[nt];
        int ng = 0;
        for (int i = 0; i < nt; i++) {
            ta[i] = d.readUnsignedShort(); tb[i] = d.readUnsignedShort(); tc[i] = d.readUnsignedShort();
            tex[i] = d.readByte(); fl[i] = d.readUnsignedByte();
            col[i] = (d.readUnsignedByte() << 16) | (d.readUnsignedByte() << 8) | d.readUnsignedByte();
            if ((fl[i] & 8) != 0) ng++;
        }
        Mesh[] r = new Mesh[2];
        r[0] = make(x, y, z, u, v, ta, tb, tc, tex, fl, col, texId, false, nt - ng);
        r[1] = ng > 0 ? make(x, y, z, u, v, ta, tb, tc, tex, fl, col, texId, true, ng) : null;
        return r;
    }

    private static Mesh make(int[] x, int[] y, int[] z, int[] u, int[] v, int[] ta, int[] tb, int[] tc, int[] tex, int[] fl, int[] col,
                             int[] texId, boolean glass, int n) {
        Mesh m = new Mesh();
        m.nv = x.length; m.nt = n;
        m.x = x; m.y = y; m.z = z; m.u = u; m.v = v;
        m.ti = new int[n * 3]; m.tex = new short[n]; m.col = new int[n]; m.flag = new byte[n];
        m.nx = new byte[n]; m.ny = new byte[n]; m.nz = new byte[n];
        int k = 0;
        for (int i = 0; i < ta.length; i++) {
            if (((fl[i] & 8) != 0) != glass) continue;
            m.ti[k * 3] = ta[i]; m.ti[k * 3 + 1] = tb[i]; m.ti[k * 3 + 2] = tc[i];
            m.tex[k] = (short) (tex[i] >= 0 && tex[i] < texId.length ? texId[tex[i]] : -1);
            m.col[k] = col[i];
            int f = 0;
            if ((fl[i] & 2) != 0) f |= Mesh.F_EMISSIVE;
            if ((fl[i] & 4) != 0) f |= Mesh.F_TINT;
            if ((fl[i] & 16) != 0) f |= Mesh.F_TINT2;
            m.flag[k] = (byte) f;
            normal(m, k, ta[i], tb[i], tc[i]);
            k++;
        }
        m.bounds();
        return m;
    }

    static void normal(Mesh m, int k, int a, int b, int c) {
        float ax = m.x[b] - m.x[a], ay = m.y[b] - m.y[a], az = m.z[b] - m.z[a], bx = m.x[c] - m.x[a], by = m.y[c] - m.y[a], bz = m.z[c] - m.z[a];
        float cx = ay * bz - az * by, cy = az * bx - ax * bz, cz = ax * by - ay * bx;
        float l = (float) Math.sqrt(cx * cx + cy * cy + cz * cz);
        if (l < 1e-6f) l = 1f;
        m.nx[k] = (byte) Fx.rf(cx / l * 127f); m.ny[k] = (byte) Fx.rf(cy / l * 127f); m.nz[k] = (byte) Fx.rf(cz / l * 127f);
    }
}
