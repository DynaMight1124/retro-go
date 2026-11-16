import java.io.*;

/** Tommy (and recoloured pedestrians): 9 rigid parts + hierarchical animation. */
final class PedModel {
    static final int TORSO = 0, ARM_A = 1, FORE_A = 2, ARM_B = 3, FORE_B = 4, THIGH_A = 5, SHIN_A = 6, THIGH_B = 7, SHIN_B = 8;
    static final int[] PAR = { -1, 0, 1, 0, 3, -1, 5, -1, 7 };

    int n, rootH;
    int[] jx, jy, jz;
    Mesh[] part;

    // scratch
    private final int[] rYaw = new int[9], rt = new int[9], rb = new int[9], rtor = new int[9], rbase = new int[9];
    final int[][] rp = new int[9][9];
    final int[] px = new int[9], py = new int[9], pz = new int[9];
    int hx, hy, hz;              // fist point of the right forearm (part FORE_A), part-local

    /** world position (1/64 m) of the right hand after the last drawPose */
    void hand(int[] o) {
        int[] R = rp[FORE_A];
        o[0] = px[FORE_A] + ((R[0] * hx + R[1] * hy + R[2] * hz) >> 14);
        o[1] = py[FORE_A] + ((R[3] * hx + R[4] * hy + R[5] * hz) >> 14);
        o[2] = pz[FORE_A] + ((R[6] * hx + R[7] * hy + R[8] * hz) >> 14);
    }
    private final int[] a = new int[9], b = new int[9], mm = new int[9], tmp = new int[9];

    static PedModel load(String res) throws IOException {
        InputStream in = Assets.open(res);
        if (in == null) throw new IOException("missing " + res);
        DataInputStream d = new DataInputStream(in);
        PedModel m = new PedModel();
        m.n = d.readUnsignedByte();
        m.rootH = d.readShort();
        m.jx = new int[m.n]; m.jy = new int[m.n]; m.jz = new int[m.n];
        m.part = new Mesh[m.n];
        for (int p = 0; p < m.n; p++) {
            m.jx[p] = -d.readShort(); m.jy[p] = d.readShort(); m.jz[p] = d.readShort();   // mirror x: OBJ is right handed
            int nv = d.readShort();
            Mesh me = new Mesh();
            me.nv = nv;
            me.x = new int[nv]; me.y = new int[nv]; me.z = new int[nv]; me.u = new int[nv]; me.v = new int[nv];
            for (int i = 0; i < nv; i++) {
                me.x[i] = -d.readShort(); me.y[i] = d.readShort(); me.z[i] = d.readShort();
                me.u[i] = d.readShort(); me.v[i] = d.readShort();
            }
            int nt = d.readShort();
            me.nt = nt;
            me.ti = new int[nt * 3];
            me.tex = new short[nt]; me.col = new int[nt]; me.flag = new byte[nt];
            me.nx = new byte[nt]; me.ny = new byte[nt]; me.nz = new byte[nt];
            for (int i = 0; i < nt; i++) {
                int i0 = d.readShort(), i1 = d.readShort(), i2 = d.readShort();
                me.ti[i * 3] = i0; me.ti[i * 3 + 1] = i2; me.ti[i * 3 + 2] = i1;    // mirrored => swap winding
                me.tex[i] = 0;
                // face normal
                int ax = me.x[i1] - me.x[i0], ay = me.y[i1] - me.y[i0], az = me.z[i1] - me.z[i0];
                int bx = me.x[i2] - me.x[i0], by = me.y[i2] - me.y[i0], bz = me.z[i2] - me.z[i0];
                float cx = (float) ay * bz - (float) az * by, cy = (float) az * bx - (float) ax * bz, cz = (float) ax * by - (float) ay * bx;
                float l = (float) Math.sqrt(cx * cx + cy * cy + cz * cz);
                if (l < 1e-6f) l = 1f;
                // after mirroring+swap the outward algebraic cross is the reverse of the one computed from (i0,i1,i2)
                me.nx[i] = (byte) Fx.rf(-cx / l * 127f); me.ny[i] = (byte) Fx.rf(-cy / l * 127f); me.nz[i] = (byte) Fx.rf(-cz / l * 127f);
            }
            me.bounds();
            m.part[p] = me;
        }
        in.close();
        Mesh f = m.part[FORE_A];
        long best = -1;
        for (int i = 0; i < f.nv; i++) {
            long dd = (long) f.x[i] * f.x[i] + (long) f.y[i] * f.y[i] + (long) f.z[i] * f.z[i];
            if (dd > best) { best = dd; m.hx = f.x[i]; m.hy = f.y[i]; m.hz = f.z[i]; }
        }
        return m;
    }

    /**
     * fx,fy,fz: feet position in world units (1/64 m). yaw 0..1023. tilt: whole body pitch (lying down).
     * lift: extra pelvis height. ang: 3 angles (x,y,z) per part, local, 0..1023.
     */
    void draw(R3D r, int fx, int fy, int fz, int yaw, int tilt, int lift, int[] ang, int tex) {
        Fx.rotY(rYaw, yaw);
        if (tilt != 0) { Fx.rotX(a, tilt); Fx.mul(rYaw, a, rbase); } else System.arraycopy(rYaw, 0, rbase, 0, 9);
        // torso frame
        Fx.rotX(a, ang[0]); Fx.rotY(b, ang[1]); Fx.mul(a, b, tmp); Fx.rotZ(b, ang[2]); Fx.mul(tmp, b, rt);
        Fx.mul(rbase, rt, rtor);
        int p0x = fx, p0y = fy + rootH + lift, p0z = fz;
        // pelvis translation: with tilt the pelvis rotates around feet; keep simple
        for (int i = 0; i < n; i++) {
            int par = PAR[i];
            int[] R = rp[i];
            // local rotation
            Fx.rotX(a, ang[i * 3]); Fx.rotY(b, ang[i * 3 + 1]); Fx.mul(a, b, tmp); Fx.rotZ(b, ang[i * 3 + 2]); Fx.mul(tmp, b, a);
            int[] frame;
            int ox, oy, oz;
            if (i == TORSO) {
                System.arraycopy(rtor, 0, R, 0, 9);
                px[i] = p0x; py[i] = p0y; pz[i] = p0z;
            } else {
                int[] pf; int bx, by, bz, ox0, oy0, oz0;
                if (par < 0) {    // thigh: attached to pelvis with base frame
                    pf = rbase; bx = p0x; by = p0y; bz = p0z; ox0 = jx[i]; oy0 = jy[i]; oz0 = jz[i];
                } else if (par == 0) {   // arms: torso frame
                    pf = rtor; bx = p0x; by = p0y; bz = p0z; ox0 = jx[i]; oy0 = jy[i]; oz0 = jz[i];
                } else {
                    pf = rp[par]; bx = px[par]; by = py[par]; bz = pz[par];
                    ox0 = jx[i] - jx[par]; oy0 = jy[i] - jy[par]; oz0 = jz[i] - jz[par];
                }
                px[i] = bx + ((pf[0] * ox0 + pf[1] * oy0 + pf[2] * oz0) >> 14);
                py[i] = by + ((pf[3] * ox0 + pf[4] * oy0 + pf[5] * oz0) >> 14);
                pz[i] = bz + ((pf[6] * ox0 + pf[7] * oy0 + pf[8] * oz0) >> 14);
                Fx.mul(pf, a, R);
            }
            // view transform
            int dx = px[i] - r.camX, dy = py[i] - r.camY, dz = pz[i] - r.camZ;
            if (dx > 30000 || dx < -30000 || dz > 30000 || dz < -30000) continue;
            int[] mv = r.mv;
            int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
            int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
            int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
            Fx.mul(mv, R, mm);
            // light in model space: R^T * L
            int lmx = (R[0] * r.lx + R[3] * r.ly + R[6] * r.lz) >> 14;
            int lmy = (R[1] * r.lx + R[4] * r.ly + R[7] * r.lz) >> 14;
            int lmz = (R[2] * r.lx + R[5] * r.ly + R[8] * r.lz) >> 14;
            r.forceTex = tex;
            r.drawMesh(part[i], mm, tx, ty, tz, lmx, lmy, lmz, 0);
            r.forceTex = -1;
        }
    }

    /** draw with baked pose (pose[0..89] world rotations in character space, pose[90..92] pelvis offset) */
    void drawPose(R3D r, int fx, int fy, int fz, int yaw, int[] pose, int tex) {
        Fx.rotY(rYaw, yaw);
        int offR = pose[90], offU = pose[91], offF = pose[92];
        int p0x = fx + ((rYaw[0] * offR + rYaw[2] * offF) >> 14), p0y = fy + rootH + offU, p0z = fz + ((rYaw[6] * offR + rYaw[8] * offF) >> 14);
        for (int i = 0; i < 10; i++) {
            for (int k = 0; k < 9; k++) tmp[k] = pose[i * 9 + k];
            Fx.mul(rYaw, tmp, rp[i == 9 ? 0 : i]);          // part i world rotation (pelvis stored temporarily in slot 0)
            if (i == 9) System.arraycopy(rp[0], 0, rb, 0, 9);
            if (i == 9) { Fx.rotY(rYaw, yaw); }
        }
        // slot 0 was overwritten by the pelvis: recompute torso
        for (int k = 0; k < 9; k++) tmp[k] = pose[k];
        Fx.mul(rYaw, tmp, rp[0]);
        for (int i = 0; i < n; i++) {
            int par = PAR[i];
            int[] R = rp[i];
            if (i == TORSO) { px[i] = p0x; py[i] = p0y; pz[i] = p0z; }
            else {
                int[] pf; int bx, by, bz, ox0, oy0, oz0;
                if (par < 0) { pf = rb; bx = p0x; by = p0y; bz = p0z; ox0 = jx[i]; oy0 = jy[i]; oz0 = jz[i]; }
                else if (par == 0) { pf = rp[0]; bx = p0x; by = p0y; bz = p0z; ox0 = jx[i]; oy0 = jy[i]; oz0 = jz[i]; }
                else { pf = rp[par]; bx = px[par]; by = py[par]; bz = pz[par]; ox0 = jx[i] - jx[par]; oy0 = jy[i] - jy[par]; oz0 = jz[i] - jz[par]; }
                px[i] = bx + ((pf[0] * ox0 + pf[1] * oy0 + pf[2] * oz0) >> 14);
                py[i] = by + ((pf[3] * ox0 + pf[4] * oy0 + pf[5] * oz0) >> 14);
                pz[i] = bz + ((pf[6] * ox0 + pf[7] * oy0 + pf[8] * oz0) >> 14);
            }
            int dx = px[i] - r.camX, dy = py[i] - r.camY, dz = pz[i] - r.camZ;
            if (dx > 30000 || dx < -30000 || dz > 30000 || dz < -30000) continue;
            int[] mv = r.mv;
            int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
            int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
            int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
            Fx.mul(mv, R, mm);
            int lmx = (R[0] * r.lx + R[3] * r.ly + R[6] * r.lz) >> 14;
            int lmy = (R[1] * r.lx + R[4] * r.ly + R[7] * r.lz) >> 14;
            int lmz = (R[2] * r.lx + R[5] * r.ly + R[8] * r.lz) >> 14;
            r.forceTex = tex;
            r.drawMesh(part[i], mm, tx, ty, tz, lmx, lmy, lmz, 0);
            r.forceTex = -1;
        }
    }
}
