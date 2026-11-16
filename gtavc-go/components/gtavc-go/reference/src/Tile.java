import java.io.*;

/** One streamed 100 m map tile: near + far mesh, two-layer ground grid, surface types, solid cells. */
final class Tile {
    static final int NH = 50, NS = 100, SROW = 13;
    int i, j;                 // grid index
    int cx, cz;               // centre, 1/64 m
    Mesh near, far;
    short[] top, low;         // 1/64 m, -32768 = none (sea)
    byte[] surf, solid;
    String[] texPath;         // local texture -> resource
    int[][] texPix;           // decoded by the loader thread
    int[] texW, texH;
    int[] slot;               // renderer slots once installed
    boolean installed;
    int lastUse;

    static Tile load(int i, int j, int ox, int oz, int tileMm) throws IOException {
        byte[] b = Assets.readAll("/t/" + i + "_" + j + ".bin");
        DataInputStream d = new DataInputStream(new ByteArrayInputStream(b));
        Tile t = new Tile();
        t.i = i; t.j = j;
        t.cx = (ox + i * tileMm + tileMm / 2) >> 4; t.cz = (oz + j * tileMm + tileMm / 2) >> 4;
        d.readByte(); d.readByte();
        int nt = d.readUnsignedShort();
        t.texPath = new String[nt]; t.texPix = new int[nt][]; t.texW = new int[nt]; t.texH = new int[nt]; t.slot = new int[nt];
        for (int k = 0; k < nt; k++) t.texPath[k] = "/x/" + d.readUnsignedShort() + ".png";
        t.near = readMesh(d);
        t.far = readMesh(d);
        t.top = new short[NH * NH]; t.low = new short[NH * NH];
        for (int k = 0; k < NH * NH; k++) t.top[k] = d.readShort();
        for (int k = 0; k < NH * NH; k++) t.low[k] = d.readShort();
        t.surf = new byte[NH * NH]; d.readFully(t.surf);
        t.solid = new byte[SROW * NS]; d.readFully(t.solid);
        d.close();
        int[] dim = new int[2];
        for (int k = 0; k < nt; k++) {
            if (Assets.find(t.texPath[k]) >= 0) continue;                 // already cached
            try { t.texPix[k] = Assets.decode(t.texPath[k], dim); t.texW[k] = dim[0]; t.texH[k] = dim[1]; } catch (Exception e) { }
        }
        return t;
    }

    static Mesh readMesh(DataInputStream d) throws IOException {
        int nv = d.readUnsignedShort();
        Mesh m = new Mesh();
        m.nv = nv;
        m.x = new int[nv]; m.y = new int[nv]; m.z = new int[nv]; m.u = new int[nv]; m.v = new int[nv];
        for (int i = 0; i < nv; i++) { m.x[i] = d.readShort(); m.y[i] = d.readShort(); m.z[i] = d.readShort(); m.u[i] = d.readShort(); m.v[i] = d.readShort(); }
        int nt = d.readUnsignedShort();
        m.nt = nt; m.ti = new int[nt * 3]; m.tex = new short[nt]; m.col = new int[nt]; m.flag = new byte[nt];
        m.nx = new byte[nt]; m.ny = new byte[nt]; m.nz = new byte[nt];
        for (int i = 0; i < nt; i++) {
            int a = d.readUnsignedShort(), bb = d.readUnsignedShort(), c = d.readUnsignedShort();
            m.ti[i * 3] = a; m.ti[i * 3 + 1] = bb; m.ti[i * 3 + 2] = c;
            int t = d.readUnsignedShort();
            m.tex[i] = (short) (t == 0xFFFF ? -1 : t);                     // local index until installed
            m.flag[i] = (byte) d.readUnsignedByte();
            m.col[i] = (d.readUnsignedByte() << 16) | (d.readUnsignedByte() << 8) | d.readUnsignedByte();
            CarModel.normal(m, i, a, bb, c);
        }
        m.bounds();
        return m;
    }

    /** main thread: textures into renderer slots, mesh texture ids -> slots */
    void install() {
        for (int k = 0; k < texPath.length; k++) {
            slot[k] = Assets.acquire(texPath[k], texPix[k], texW[k], texH[k]);
            texPix[k] = null;
        }
        remap(near); remap(far);
        installed = true;
    }
    private void remap(Mesh m) {
        for (int i = 0; i < m.nt; i++) { int t = m.tex[i]; if (t >= 0) m.tex[i] = (short) slot[t]; }
    }
    void unload() {
        if (installed) for (int k = 0; k < slot.length; k++) Assets.release(slot[k]);
        installed = false; near = far = null; top = low = null; surf = solid = null;
    }
}
