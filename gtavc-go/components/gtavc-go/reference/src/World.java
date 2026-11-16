import java.io.*;

/** Streamed Vice City: tiles, ground/solid grids, peds, traffic, cops, weapons, simulation and rendering. */
final class World implements Runnable {
    R3D r;
    World(R3D r) { this.r = r; }

    // ================================================================== map streaming
    int mox, moz, tileMm, tnx, tnz;
    byte[] tflag;
    Tile[] tiles;
    int rx0, rz0, rcell, rw, rh;
    byte[] rad;
    int spX, spZ, spY, spYaw;
    int lampX, lampY, lampZ;
    Mesh beam, blood;
    static final int S_WATER = 0, S_SAND = 1, S_GRASS = 2, S_PAVE = 3, S_ROAD = 4, S_ROCK = 5;
    static final boolean LEGACY = Assets.legacy;
    static final int RING = LEGACY ? 1 : 2;
    private final int[] reqQ = new int[64];
    private int reqN;
    private final Tile[] readyQ = new Tile[16];
    private int readyN;
    private boolean[] requested;
    private Thread loader;
    volatile boolean alive = true;

    void loadIndex() throws IOException {
        DataInputStream d = new DataInputStream(new ByteArrayInputStream(Assets.readAll("/mapidx.bin")));
        byte[] mg = new byte[4]; d.readFully(mg);
        mox = d.readInt(); moz = d.readInt(); tileMm = d.readUnsignedShort(); tnx = d.readUnsignedShort(); tnz = d.readUnsignedShort();
        tileMm = 100 * 1024;
        byte[] bits = new byte[(tnx * tnz + 7) / 8]; d.readFully(bits);
        tflag = new byte[tnx * tnz];
        for (int k = 0; k < tnx * tnz; k++) tflag[k] = (byte) ((bits[k >> 3] >> (7 - (k & 7))) & 1);
        d.readUnsignedShort();
        rx0 = d.readInt(); rz0 = d.readInt(); rcell = d.readUnsignedShort(); rw = d.readUnsignedShort(); rh = d.readUnsignedShort();
        rad = new byte[rw * rh]; d.readFully(rad);
        spX = d.readInt(); spZ = d.readInt(); spY = d.readInt(); spYaw = d.readUnsignedShort();
        lampX = d.readInt(); lampY = d.readInt(); lampZ = d.readInt();
        d.close();
        tiles = new Tile[tnx * tnz];
        requested = new boolean[tnx * tnz];
        buildFxMeshes();
    }

    void preload(int x, int z) {
        int ci = (x - mox) / tileMm, cj = (z - moz) / tileMm;
        for (int j = cj - 2; j <= cj + 2; j++) for (int i = ci - 2; i <= ci + 2; i++) {
            int k = idx(i, j);
            if (k < 0 || tiles[k] != null || tflag[k] == 0) continue;
            try { Tile t = Tile.load(i, j, mox, moz, tileMm); t.install(); tiles[k] = t; } catch (Exception e) { tflag[k] = 0; }
        }
    }
    void startLoader() { if (loader == null) { loader = new Thread(this); loader.start(); } }
    int idx(int i, int j) { return i < 0 || j < 0 || i >= tnx || j >= tnz ? -1 : j * tnx + i; }

    public void run() {
        while (alive) {
            int k = -1;
            synchronized (reqQ) {
                if (reqN == 0) { try { reqQ.wait(500); } catch (InterruptedException e) { } }
                if (reqN > 0) { k = reqQ[0]; System.arraycopy(reqQ, 1, reqQ, 0, --reqN); }
            }
            if (k < 0) continue;
            Tile t;
            try { t = Tile.load(k % tnx, k / tnx, mox, moz, tileMm); } catch (Throwable e) { t = null; }
            synchronized (readyQ) {
                if (t == null) { tflag[k] = 0; requested[k] = false; }
                else if (readyN < readyQ.length) readyQ[readyN++] = t;
                else requested[k] = false;
            }
            try { Thread.sleep(5); } catch (InterruptedException e) { }
        }
    }

    /** each frame: install finished tiles, request missing ones nearest first, drop far ones */
    void streamTiles(int x, int z) {
        synchronized (readyQ) {
            for (int n = 0; n < readyN; n++) {
                Tile t = readyQ[n];
                int k = idx(t.i, t.j);
                requested[k] = false;
                if (tiles[k] == null) { t.install(); tiles[k] = t; }
                readyQ[n] = null;
            }
            readyN = 0;
        }
        if (loader == null) return;
        int ci = (x - mox) / tileMm, cj = (z - moz) / tileMm;
        for (int ring = 0; ring <= RING; ring++) {
            for (int j = cj - ring; j <= cj + ring; j++) for (int i = ci - ring; i <= ci + ring; i++) {
                if (Math.max(Math.abs(i - ci), Math.abs(j - cj)) != ring) continue;
                int k = idx(i, j);
                if (k < 0 || tflag[k] == 0 || tiles[k] != null || requested[k]) continue;
                synchronized (reqQ) { if (reqN < reqQ.length) { reqQ[reqN++] = k; requested[k] = true; reqQ.notify(); } }
            }
        }
        for (int k = 0; k < tiles.length; k++) {
            Tile t = tiles[k];
            if (t == null) continue;
            if (Math.abs(t.i - ci) > RING + 1 || Math.abs(t.j - cj) > RING + 1) { t.unload(); tiles[k] = null; }
        }
    }

    void buildFxMeshes() {
        int L = 70 * 64, w = 7 * 64;
        beam = new Mesh();
        beam.nv = 5; beam.x = new int[] { 0, -w, w, 0, 0 }; beam.y = new int[] { 0, 0, 0, -w, w }; beam.z = new int[] { 0, L, L, L, L };
        beam.u = new int[5]; beam.v = new int[5];
        beam.nt = 2; beam.ti = new int[] { 0, 1, 2, 0, 3, 4 };
        beam.tex = new short[] { -1, -1 }; beam.col = new int[] { 0x2A2618, 0x2A2618 };
        beam.flag = new byte[] { (byte) (Mesh.F_DOUBLE | Mesh.F_EMISSIVE), (byte) (Mesh.F_DOUBLE | Mesh.F_EMISSIVE) };
        beam.nx = new byte[2]; beam.ny = new byte[2]; beam.nz = new byte[2];
        beam.bounds();
        blood = new Mesh();
        int q = 64;
        blood.nv = 4; blood.x = new int[] { -q, q, q, -q }; blood.y = new int[4]; blood.z = new int[] { -q, -q, q, q };
        blood.u = new int[] { 0, 511, 511, 0 }; blood.v = new int[] { 0, 0, 511, 511 };
        blood.nt = 2; blood.ti = new int[] { 0, 2, 1, 0, 3, 2 };
        int bt = Assets.acquire("/t_blood.png");
        blood.tex = new short[] { (short) bt, (short) bt }; blood.col = new int[2];
        blood.flag = new byte[] { Mesh.F_DOUBLE, Mesh.F_DOUBLE };
        blood.nx = new byte[2]; blood.ny = new byte[] { 127, 127 }; blood.nz = new byte[2];
        blood.bounds();
    }

    // ------------------------------------------------------------------ grids (global cell indices)
    private Tile tileFor(int gi, int gj, int per) {
        if (gi < 0 || gj < 0) return null;
        int k = idx(gi / per, gj / per);
        return k < 0 ? null : tiles[k];
    }
    private int hCell(int gi, int gj, int y) {
        Tile t = tileFor(gi, gj, Tile.NH);
        if (t == null || t.top == null) return -640;
        int k = (gj % Tile.NH) * Tile.NH + (gi % Tile.NH);
        int a = t.top[k];
        if (a == -32768) return -640;
        if (a > y + 140) { int b = t.low[k]; return b == -32768 ? -640 : b; }
        return a;
    }
    /** ground height (1/64 m) for something currently at height y: the top layer unless it is overhead (bridges) */
    int groundH(int xm, int zm, int y) {
        int fx = (int) (((long) (xm - mox - 1024) << 8) / 2048), fz = (int) (((long) (zm - moz - 1024) << 8) / 2048);
        int i = fx >> 8, j = fz >> 8, ax = fx & 255, az = fz & 255;
        int h00 = hCell(i, j, y), h10 = hCell(i + 1, j, y), h01 = hCell(i, j + 1, y), h11 = hCell(i + 1, j + 1, y);
        if (h00 == -640 || h10 == -640 || h01 == -640 || h11 == -640) {         // fill holes from the neighbours
            int v = Math.max(Math.max(h00, h10), Math.max(h01, h11));
            if (v == -640) return -640;
            if (h00 == -640) h00 = v; if (h10 == -640) h10 = v; if (h01 == -640) h01 = v; if (h11 == -640) h11 = v;
        }
        int mn = Math.min(Math.min(h00, h10), Math.min(h01, h11)), mx = Math.max(Math.max(h00, h10), Math.max(h01, h11));
        if (mx - mn > 96) return ax < 128 ? (az < 128 ? h00 : h01) : (az < 128 ? h10 : h11);   // kerbs/walls: no blending across steps
        int a = h00 + (((h10 - h00) * ax) >> 8), b = h01 + (((h11 - h01) * ax) >> 8);
        return a + (((b - a) * az) >> 8);
    }
    int groundH(int xm, int zm) { return groundH(xm, zm, 4000); }
    int surfAt(int xm, int zm) {
        if (xm < mox || zm < moz) return S_WATER;
        int gi = (xm - mox) / 2048, gj = (zm - moz) / 2048;
        Tile t = tileFor(gi, gj, Tile.NH);
        if (t == null || t.surf == null) return S_WATER;
        return t.surf[(gj % Tile.NH) * Tile.NH + (gi % Tile.NH)];
    }
    boolean solidCell(int gi, int gj) {
        Tile t = tileFor(gi, gj, Tile.NS);
        if (t == null || t.solid == null) return false;
        int li = gi % Tile.NS, lj = gj % Tile.NS;
        return (t.solid[lj * Tile.SROW + (li >> 3)] & (0x80 >> (li & 7))) != 0;
    }
    int scell(int m, int o) { return m < o ? -1 : (m - o) / 1024; }
    boolean deep(int xm, int zm, int y) { return groundH(xm, zm, y) < -70; }
    boolean walkable(int xm, int zm, int y) {
        if (surfAt(xm, zm) == S_WATER) return false;
        int g = groundH(xm, zm, y);
        if (g < -70 || g > y + 56) return false;
        return !solidCell(scell(xm, mox), scell(zm, moz));
    }

    boolean resolve(int rad) {
        boolean hit = false;
        int i0 = scell(rx - rad, mox), i1 = scell(rx + rad, mox), j0 = scell(rz - rad, moz), j1 = scell(rz + rad, moz);
        for (int j = j0; j <= j1; j++) {
            for (int i = i0; i <= i1; i++) {
                if (!solidCell(i, j)) continue;
                int bx0 = mox + i * 1024, bz0 = moz + j * 1024, bx1 = bx0 + 1024, bz1 = bz0 + 1024;
                int cx = rx < bx0 ? bx0 : (rx > bx1 ? bx1 : rx);
                int cz = rz < bz0 ? bz0 : (rz > bz1 ? bz1 : rz);
                int dx = rx - cx, dz = rz - cz;
                if (dx == 0 && dz == 0) {
                    int l = rx - bx0, rr = bx1 - rx, t = rz - bz0, bt = bz1 - rz;
                    int m = Math.min(Math.min(l, rr), Math.min(t, bt));
                    if (m == l) rx = bx0 - rad; else if (m == rr) rx = bx1 + rad; else if (m == t) rz = bz0 - rad; else rz = bz1 + rad;
                    hit = true;
                } else {
                    int d2 = dx * dx + dz * dz;
                    if (d2 < rad * rad) {
                        int d = (int) Math.sqrt((double) d2);
                        if (d < 1) d = 1;
                        rx = cx + dx * rad / d; rz = cz + dz * rad / d;
                        hit = true;
                    }
                }
            }
        }
        return hit;
    }

    boolean stepOk(int ox, int oz, int oy, int maxStep) {
        if (groundH(rx, rz, oy) - oy <= maxStep) return true;
        int nx = rx, nz = rz;
        rx = nx; rz = oz;
        if (groundH(rx, rz, oy) - oy <= maxStep) return true;
        rx = ox; rz = nz;
        if (groundH(rx, rz, oy) - oy <= maxStep) return true;
        rx = ox; rz = oz;
        return false;
    }

    // ================================================================== model pools
    static final int K_UP = 1, K_DOWN = 2, K_LEFT = 4, K_RIGHT = 8, K_FIRE = 16, K_CAML = 32, K_CAMR = 64, K_CAMU = 128,
            K_CAMD = 256, K_SPRINT = 512, K_HAND = 1024;
    static final int PA_NONE = 0, PA_WALKTO = 1, PA_GETIN = 2, PA_GETOUT = 3;
    static final int NPOOL = LEGACY ? 5 : 12, NCPOOL = LEGACY ? 4 : 10;
    static final int MAXPED = LEGACY ? 10 : 22, MAXTRAFFIC = LEGACY ? 3 : 8;

    String[] pedNames; int[] pedFlags;
    PedModel pm0; int playerTex;
    PedModel[] pool = new PedModel[NPOOL], poolLo = new PedModel[NPOOL];
    int[] poolTex = new int[NPOOL], poolSrc = new int[NPOOL];
    int copSlot = 0;
    String[] carNames;
    CarModel[] cpool = new CarModel[NCPOOL];
    int[] cpoolSrc = new int[NCPOOL];
    int policeSlot = 0;
    Mesh[] weaponMesh = new Mesh[3];

    void loadPools() throws IOException {
        String[] pl = Assets.readLines("/peds.lst");
        pedNames = new String[pl.length]; pedFlags = new int[pl.length];
        for (int i = 0; i < pl.length; i++) { int s = pl[i].indexOf(' '); pedNames[i] = pl[i].substring(0, s); pedFlags[i] = Integer.parseInt(pl[i].substring(s + 1).trim()); }
        carNames = Assets.readLines("/cars.lst");
        pm0 = PedModel.load("/p_player.bin");
        playerTex = Assets.acquire("/t_p_player.png");
        loadPed(0, find(pedNames, "cop"));
        for (int i = 1; i < NPOOL; i++) loadPed(i, randomAmbient());
        loadCar(0, find(carNames, "police"));
        String[] start = { "admiral", "taxi", "pcj600", "stallion", "cheetah", "sentinel", "faggio", "pony", "voodoo" };
        for (int i = 1; i < NCPOOL; i++) loadCar(i, find(carNames, start[i - 1]));
        weaponMesh[1] = loadWeapon("bat");
        weaponMesh[2] = loadWeapon("colt45");
    }
    static int find(String[] a, String s) { for (int i = 0; i < a.length; i++) if (a[i].equals(s)) return i; return 0; }
    int randomAmbient() {
        for (int k = 0; k < 40; k++) { int i = Fx.rnd(pedNames.length); if ((pedFlags[i] & 14) == 0) return i; }
        return 0;
    }
    void loadPed(int slot, int src) throws IOException {
        if (pool[slot] != null) Assets.release(poolTex[slot]);
        String n = pedNames[src];
        pool[slot] = PedModel.load("/p_" + n + ".bin");
        poolLo[slot] = PedModel.load("/p_" + n + "_lo.bin");
        poolTex[slot] = Assets.acquire("/t_p_" + n + ".png");
        poolSrc[slot] = src;
    }
    void loadCar(int slot, int src) throws IOException {
        if (cpool[slot] != null) cpool[slot].unload();
        CarModel m = CarModel.load(carNames[src]);
        handling(m);
        cpool[slot] = m; cpoolSrc[slot] = src;
    }
    Mesh loadWeapon(String n) {
        try {
            DataInputStream d = new DataInputStream(new ByteArrayInputStream(Assets.readAll("/w_" + n + ".bin")));
            int[] t = CarModel.readTex(d);
            Mesh m = CarModel.readMesh(d, t)[0];
            d.close();
            return m;
        } catch (Exception e) { return null; }
    }

    static final String SPORTS = " banshee cheetah infernus comet stinger deluxo pheonix sabretur hotring hotrina hotrinb bloodra bloodrb vicechee cuban sentxs ";
    static final String HEAVY = " benson boxville mule packer linerun flatbed trash barracks firetruk bus coach securica enforcer ambulan yankee rumpo pony burrito spand topfun mrwhoop walton bobcat rancher landstal patriot sandking fbiranch moonbeam ";
    static final String BIKE = " angel freeway pcj600 sanchez faggio pizzaboy ";
    static final String SLOW = " caddy baggage ";
    /** handling.cfg-like classes: top speed km/h, acceleration, steering lock, mass, tyre grip */
    String[] hdl;
    void handling(CarModel m) {
        if (hdl == null) hdl = Assets.readLines("/handling.txt");
        String k = " " + m.name + " ";
        int mv = 160, ac = 22, lk = 32, mass = 1500, grip = 100, cls = 0;
        if (BIKE.indexOf(k) >= 0) { mv = m.name.equals("faggio") || m.name.equals("pizzaboy") ? 100 : 180; ac = 30; lk = 38; mass = 350; grip = 130; cls = 3; }
        else if (SPORTS.indexOf(k) >= 0) { mv = 200; ac = 30; lk = 30; mass = 1300; grip = 125; cls = 1; }
        else if (HEAVY.indexOf(k) >= 0) { mv = 130; ac = 15; lk = 30; mass = 3500; grip = 85; cls = 2; }
        else if (SLOW.indexOf(k) >= 0) { mv = 60; ac = 12; lk = 40; mass = 500; grip = 110; cls = 4; }
        if (m.name.equals("police") || m.name.equals("fbicar")) { mv = 190; ac = 26; }
        m.vmax = mv * 11 / 36; m.accel = ac * 30; m.brake = 1100; m.lockDeg = lk; m.mass = mass; m.grip = grip; m.cls = cls;
        // real values from handling.cfg when the vehicle is listed there
        for (int i = 0; i < hdl.length; i++) {
            if (!hdl[i].startsWith(m.name + " ")) continue;
            int[] v = new int[6]; int p = m.name.length() + 1;
            for (int f = 0; f < 6; f++) { int q = hdl[i].indexOf(' ', p); if (q < 0) q = hdl[i].length(); v[f] = Integer.parseInt(hdl[i].substring(p, q).trim()); p = q + 1; }
            m.mass = v[0]; m.grip = Math.max(95, v[1]); m.vmax = v[2] * 11 / 36; m.accel = v[3] * 3; m.brake = v[4] + 200; m.lockDeg = v[5];
            break;
        }
        m.wheelbase = m.dimZ * 16 * 6 / 10;
        if (m.wheelbase < 900) m.wheelbase = 900;
    }

    int poolT;
    /** every ~40 s swap one unused ped model and one unused car model (variety with bounded memory) */
    void rotatePools(int dt) {
        poolT += dt;
        if (poolT < 40000) return;
        poolT = 0;
        try {
            int s = 1 + Fx.rnd(NPOOL - 1);
            boolean used = false;
            for (int i = 0; i < nped; i++) if (peds[i].kind == s && (peds[i].state != Ped.DEAD || peds[i].deadT >= 0)) used = true;
            if (!used) loadPed(s, randomAmbient());
            int c = 1 + Fx.rnd(NCPOOL - 1);
            used = false;
            for (int i = 0; i < ncars; i++) if (cars[i].type == c) used = true;
            if (!used) loadCar(c, Fx.rnd(carNames.length));
        } catch (Exception e) { }
    }

    // ================================================================== state
    byte[] tc;
    int cIdle, cWalk, cWalkB, cRun, cCiviWalk, cCiviRun, cWomanWalk, cWomanRun, cWomanIdle, cPunch, cJab, cHeavy,
            cHitF, cHitB, cKoF, cKoB, cGetup, cOpen, cGetin, cGetout, cPull, cJacked, cSit, cSprint, cJumpL, cJumpG, cJumpLand;

    Ped player;
    Car inCar, pActCar;
    int pAct, combo;
    Car[] cars = new Car[16];
    int ncars;
    Ped[] peds = new Ped[40];
    int nped;
    int money, health = 100, armor, wanted, pedsHit, carsStolen, kills;
    int quietKills, quietLimit = 4;
    int wantedCalmT;
    int minutes = 14 * 60 + 18, clockMs;
    int camYawOff, camYaw, camPitch = 55, camIdle, camXm, camYm, camZm;
    int camDistMm = 4300;
    String help; int helpT, helpShown;
    int wastedT, dayCount, bustedT, drownT;
    boolean busted;
    int drawDist = 150;
    int brightness = 10;
    int time;
    int rx, rz;
    int shake;
    boolean runMode, god, jumpReq;
    int speedMul = 100, sprintHeld, voiceCd;
    Ped target;
    int weapon;                      // 0 fists, 1 bat, 2 pistol
    boolean hasBat, hasPistol;
    int ammo, shotCd, aimT, flashT;
    int npick, saveT;
    static final int NPK = 24;
    int[] pkX = new int[NPK], pkZ = new int[NPK], pkY = new int[NPK], pkType = new int[NPK], pkT = new int[NPK], pkAmt = new int[NPK], pkLife = new int[NPK];
    Mesh cashMesh;
    private final int[] pose = new int[93], pose2 = new int[93];

    void initClips() {
        cIdle = AnimData.find("IDLE_stance"); cWalk = AnimData.find("WALK_player"); cWalkB = AnimData.find("walk_back");
        cRun = AnimData.find("run_player"); cCiviWalk = AnimData.find("WALK_civi"); cCiviRun = AnimData.find("run_civi");
        cWomanWalk = AnimData.find("WOMAN_walknorm"); cWomanRun = AnimData.find("woman_run"); cWomanIdle = AnimData.find("woman_idlestance");
        cPunch = AnimData.find("FIGHTppunch"); cJab = AnimData.find("FIGHTjab"); cHeavy = AnimData.find("FIGHTpunch");
        cHitF = AnimData.find("HIT_front"); cHitB = AnimData.find("HIT_back"); cKoF = AnimData.find("KO_shot_front"); cKoB = AnimData.find("KO_skid_back");
        cSprint = AnimData.find("sprint_civi");
        cGetup = AnimData.find("getup"); cOpen = AnimData.find("CAR_open_LHS");
        cGetin = AnimData.find("CAR_getin_LHS"); cGetout = AnimData.find("CAR_getout_LHS"); cPull = AnimData.find("CAR_pullout_LHS");
        cJacked = AnimData.find("CAR_jackedLHS"); cSit = AnimData.find("CAR_sit");
        cJumpL = AnimData.find("JUMP_launch"); cJumpG = AnimData.find("JUMP_glide"); cJumpLand = AnimData.find("JUMP_land");
    }

    void assignKind(Ped p, int slot) { p.kind = slot; p.tex = poolTex[slot]; int f = pedFlags[poolSrc[slot]]; p.female = (f & 1) != 0; p.gang = (f & 4) != 0; }

    void reset() {
        Fx.seed = (int) System.currentTimeMillis();
        initClips();
        player = new Ped();
        placePlayerAtSpawn();
        player.tex = playerTex; player.loco = cIdle;
        inCar = null; pActCar = null; pAct = 0;
        ncars = 0; nped = 0;
        health = 100; armor = 0; money = 0; wanted = 0; pedsHit = 0; carsStolen = 0; kills = 0; quietKills = 0; quietLimit = 3 + Fx.rnd(3);
        minutes = 22 * 60; wastedT = 0; helpShown = 0; helpT = 0; busted = false; bustedT = 0; drownT = 0; runMode = false;
        weapon = 0; hasBat = false; hasPistol = false; ammo = 0;
        camYawOff = 0; camYaw = player.yaw; camPitch = 55;
        preload(player.x, player.z);
        // the white Admiral parked next to Tommy, like the start of the game
        int adm = find(carNames, "admiral"), slot = -1;
        for (int i = 0; i < NCPOOL; i++) if (cpoolSrc[i] == adm && cpool[i] != null) slot = i;
        if (slot >= 0) {
            int fx = Fx.sin(player.yaw), fz = Fx.cos(player.yaw);
            int[] off = { 3000, 2600, 3000, -2600, 6000, 2600, 6000, -2600, 4000, 0 };
            for (int k = 0; k < off.length; k += 2) {
                int x = player.x + ((fx * off[k] + fz * off[k + 1]) >> 14), z = player.z + ((fz * off[k] - fx * off[k + 1]) >> 14);
                if (!walkable(x, z, player.y)) continue;
                Car c = addCar(Car.PARKED, slot, x, z, player.yaw, 4);
                if (c != null) { c.color = 0xF2F2F2; c.color2 = 0xF2F2F2; c.y = groundH(x, z, player.y); }
                break;
            }
        }
        npick = 0;
        addPickup(2, 2600, 1200);
        addPickup(1, -2200, 1800);
        for (int i = 0; i < MAXTRAFFIC - 1; i++) spawnTraffic(true);
        for (int i = 0; i < MAXPED - 4; i++) spawnPed(true);
        say("Welcome to Vice City.   1: walk / run   0: jump   2 4 6 8: camera   3: switch weapon");
    }
    void addPickup(int type, int fwd, int side) {
        int fx = Fx.sin(player.yaw), fz = Fx.cos(player.yaw);
        rx = player.x + ((fx * fwd + fz * side) >> 14); rz = player.z + ((fz * fwd - fx * side) >> 14);
        if (!walkable(rx, rz, player.y)) { rx = player.x + ((fx * fwd) >> 14); rz = player.z + ((fz * fwd) >> 14); }
        pkX[npick] = rx; pkZ[npick] = rz; pkY[npick] = groundH(rx, rz, player.y); pkType[npick] = type; pkT[npick] = 0; pkLife[npick] = 0; npick++;
    }
    /** dropped by a dead ped / cop: disappears after 30 s */
    void dropPickup(int type, int amt, int x, int z, int y) {
        int k = -1;
        for (int i = 0; i < npick; i++) if (pkLife[i] < 0) { k = i; break; }
        if (k < 0) { if (npick >= NPK) return; k = npick++; }
        pkX[k] = x; pkZ[k] = z; pkY[k] = groundH(x, z, y); pkType[k] = type; pkAmt[k] = amt; pkT[k] = 0; pkLife[k] = 30000;
    }

    /** continue: move Tommy (and his world) to the saved spot */
    void continueAt(int x, int z, int y, int yaw) {
        for (int k = 0; k < tiles.length; k++) if (tiles[k] != null) { tiles[k].unload(); tiles[k] = null; }
        player.x = x; player.z = z; player.yaw = yaw & 1023;
        preload(x, z);
        int g = groundH(x, z, y);
        player.y = g > -70 ? g : y;
        camYaw = player.yaw;
        ncars = 0; nped = 0; npick = 0;
        for (int i = 0; i < MAXTRAFFIC - 1; i++) spawnTraffic(true);
        for (int i = 0; i < MAXPED - 4; i++) spawnPed(true);
        weapon = hasPistol && ammo > 0 ? 2 : (hasBat ? 1 : 0);
        say("Welcome back.");
    }

    void placePlayerAtSpawn() {
        player.x = spX; player.z = spZ; player.yaw = spYaw; player.y = spY;
        player.state = Ped.WALK; player.hp = 100; player.clip = -1; player.seat = null; player.speed = 0;
    }

    Car addCar(int mode, int slot, int xm, int zm, int yaw, int colorIdx) {
        if (ncars >= cars.length || cpool[slot] == null) return null;
        Car c = new Car();
        c.mode = mode; c.type = slot; c.x = xm; c.z = zm; c.yaw = yaw & 1023; c.y = groundH(xm, zm);
        c.color = Car.COLORS[colorIdx % Car.COLORS.length];
        c.color2 = Car.COLORS[(colorIdx + 3) % Car.COLORS.length];
        String n = cpool[slot].name;
        if (n.equals("taxi") || n.equals("cabbie") || n.equals("zebra")) { c.color = 0xF0C820; c.color2 = 0xF0C820; }
        if (n.equals("police")) { c.color = 0xF4F4F4; c.color2 = 0x202430; }
        cars[ncars++] = c;
        return c;
    }
    void removeCar(int i) {
        Car c = cars[i];
        if (c.driver != null) { c.driver.seat = null; c.driver.state = Ped.DEAD; c.driver.timer = 0; c.driver.hp = 0; c.driver.deadT = -1; c.driver.cop = false; c.driver = null; }
        cars[i] = cars[ncars - 1]; ncars--;
    }
    void addDriver(Car c, boolean cop) {
        Ped d = null;
        for (int i = 0; i < nped; i++) if (peds[i].state == Ped.DEAD && peds[i].deadT < 0 && peds[i].seat == null) { d = peds[i]; break; }
        if (d == null) { if (nped >= peds.length) return; d = new Ped(); peds[nped++] = d; }
        d.seat = c; c.driver = d; d.cop = cop; assignKind(d, cop ? copSlot : 1 + Fx.rnd(NPOOL - 1)); d.state = Ped.IDLE; d.loco = cIdle; d.hp = 100; d.clip = -1; d.deadT = 0; d.angry = false;
    }
    int vmaxOf(Car c) { return cpool[c.type].vmax * 1024; }
    CarModel cm(Car c) { return cpool[c.type]; }

    final int[] spot = new int[2];
    boolean findSpot(int d0, int d1, int want) {
        for (int k = 0; k < 24; k++) {
            int a = Fx.rnd(1024), d = (d0 + Fx.rnd(d1 - d0 + 1)) * 1024;
            int x = player.x + ((Fx.sin(a) * (d >> 4)) >> 10), z = player.z + ((Fx.cos(a) * (d >> 4)) >> 10);
            int s = surfAt(x, z);
            boolean ok;
            if (want == S_ROAD) ok = s == S_ROAD && !deep(x, z, 4000);
            else { int g = groundH(x, z, 4000); ok = s != S_WATER && s != S_ROAD && walkable(x, z, g); }
            if (ok) { spot[0] = x; spot[1] = z; return true; }
        }
        return false;
    }
    int roadYaw(int x, int z) {
        int best = 0, bl = -1;
        for (int a = 0; a < 1024; a += 64) {
            int l = 0;
            for (int s = 2; s <= 24; s += 2) { if (surfAt(x + ((Fx.sin(a) * s) >> 4), z + ((Fx.cos(a) * s) >> 4)) != S_ROAD) break; l++; }
            if (l > bl) { bl = l; best = a; }
        }
        return best;
    }
    void spawnTraffic(boolean initial) {
        if (!findSpot(initial ? 25 : 80, initial ? 110 : 130, S_ROAD)) return;
        Car c = addCar(Car.TRAFFIC, 1 + Fx.rnd(NCPOOL - 1), spot[0], spot[1], roadYaw(spot[0], spot[1]), Fx.rnd(10));
        if (c == null) return;
        c.vx = (Fx.sin(c.yaw) * 6) >> 4; c.vz = (Fx.cos(c.yaw) * 6) >> 4; c.speed = 6 * 1024;
        addDriver(c, false);
    }
    void spawnPed(boolean initial) {
        if (!findSpot(initial ? 8 : 50, initial ? 75 : 85, S_PAVE)) return;
        Ped p = null;
        for (int i = 0; i < nped; i++) if (peds[i].state == Ped.DEAD && peds[i].timer <= 0 && !peds[i].cop && peds[i].seat == null) { p = peds[i]; break; }
        if (p == null) { if (nped >= peds.length) return; p = new Ped(); peds[nped++] = p; }
        p.x = spot[0]; p.z = spot[1];
        p.state = Ped.WALK; p.hp = 100; p.cash = 0; p.cop = false; p.clip = -1; p.clipNext = -1; p.seat = null; p.deadT = 0; p.angry = false; p.aimT = 0;
        p.yaw = Fx.rnd(1024); p.targetYaw = p.yaw;
        p.speed = 1150 + Fx.rnd(350);
        assignKind(p, 1 + Fx.rnd(NPOOL - 1));
        p.brave = p.gang || (!p.female && Fx.rnd(10) < 3);
        p.turnTimer = 1500 + Fx.rnd(4000);
        p.locoT = Fx.rnd(1000);
        p.y = groundH(p.x, p.z);
        p.timer = 0;
    }
    Ped spawnCopAt(int x, int z) {
        Ped p = null;
        for (int i = 0; i < nped; i++) if (peds[i].state == Ped.DEAD && peds[i].timer <= 0 && peds[i].seat == null) { p = peds[i]; break; }
        if (p == null) { if (nped >= peds.length) return null; p = new Ped(); peds[nped++] = p; }
        p.x = x; p.z = z; p.cop = true; assignKind(p, copSlot); p.hp = 160; p.state = Ped.WALK; p.speed = 0; p.clip = -1; p.seat = null;
        p.deadT = 0; p.cash = 0; p.closeT = 0; p.attackCd = 1500; p.y = groundH(x, z, player.y); p.yaw = Fx.yawOf(player.x - x, player.z - z); p.angry = false; p.aimT = 0;
        return p;
    }
    void spawnCop() { if (findSpot(28, 42, S_PAVE)) spawnCopAt(spot[0], spot[1]); }
    void spawnPoliceCar() {
        if (!findSpot(60, 90, S_ROAD)) return;
        Car c = addCar(Car.POLICE, policeSlot, spot[0], spot[1], Fx.yawOf(player.x - spot[0], player.z - spot[1]), 0);
        if (c != null) addDriver(c, true);
    }
    int copsNear(int rangeMm) {
        int n = 0;
        for (int i = 0; i < nped; i++) {
            Ped p = peds[i];
            if (!p.cop || p.state == Ped.DEAD) continue;
            int x = p.seat != null ? p.seat.x : p.x, z = p.seat != null ? p.seat.z : p.z;
            if (Fx.dist(x - player.x, z - player.z) < rangeMm) n++;
        }
        return n;
    }
    void raiseWanted(int lvl) { if (wanted < lvl) { wanted = lvl; wantedCalmT = 0; } }
    void say(String s) { help = s; helpT = 5000; }

    // ------------------------------------------------------------------ clips
    void startClip(Ped p, int clip, int next, int endMs) { p.clip = clip; p.clipT = 0; p.clipNext = next; p.clipEnd = endMs; p.hitDone = false; }
    int clipLen(Ped p) { int l = AnimData.len(p.clip); return p.clipEnd > 0 && p.clipEnd < l ? p.clipEnd : l; }
    boolean stepClip(Ped p, int dt) {
        if (p.clip < 0) return false;
        p.clipT += dt;
        int len = clipLen(p);
        if (p.clipT < len) return false;
        if (p.clipNext == -2) { p.clipT = len; return false; }
        if (p.clip == cGetout || p.clip == cGetin || p.clip == cPull || p.clip == cJacked || p.clip == cOpen) {
            AnimData.sample(p.clip, len, pose2);
            int offR = pose2[90] * 16, offF = pose2[92] * 16, c = Fx.cos(p.yaw), s = Fx.sin(p.yaw);
            p.x += (offR * c + offF * s) >> 14; p.z += (-offR * s + offF * c) >> 14;
        }
        p.clip = p.clipNext; p.clipNext = -1; p.clipT = 0; p.clipEnd = 0; p.hitDone = false;
        return true;
    }
    void selectLoco(Ped p, int dt, int speed) {
        int sp = Fx.abs(speed), c;
        boolean f = p.female;
        if (sp < 150) { c = f ? cWomanIdle : cIdle; p.locoT += dt; }
        else {
            if (speed < 0) c = cWalkB;
            else if (sp < 3000) c = p == player ? cWalk : (f ? cWomanWalk : cCiviWalk);
            else if (sp > 6000 && p != player) c = cSprint;
            else c = p == player || p.cop ? cRun : (f ? cWomanRun : cCiviRun);
            int ratio = sp * 256 / AnimData.speed(c);
            if (ratio < 90) ratio = 90; if (ratio > 560) ratio = 560;
            p.locoT += dt * ratio >> 8;
        }
        p.loco = c;
    }

    // ------------------------------------------------------------------ update
    void update(int dt, int keys, boolean fire, boolean enter, boolean toggleRun, boolean nextWeapon, boolean jump) {
        jumpReq = jump;
        if (voiceCd > 0) voiceCd -= dt;
        if (dt > 100) dt = 100;
        time += dt;
        clockMs += dt;
        while (clockMs >= 1000) { clockMs -= 1000; minutes++; if (minutes >= 1440) { minutes = 0; dayCount++; } }
        if (helpT > 0) helpT -= dt;
        if (shake > 0) shake -= dt;
        if (shotCd > 0) shotCd -= dt;
        if (aimT > 0) aimT -= dt;
        if (flashT > 0) flashT -= dt;
        streamTiles(player.x, player.z);
        rotatePools(dt);
        if (busted) {
            bustedT += dt; updateWorld(dt); updateCamera(dt, keys);
            if (bustedT > 3600) { busted = false; bustedT = 0; arrestRespawn(); }
            return;
        }
        if (health <= 0) {
            if (wastedT == 0) { player.seat = null; if (inCar != null) { inCar.mode = Car.PARKED; inCar = null; } pAct = 0; startClip(player, cKoF, -2, 0); Snd.engine(false, 0); }
            wastedT += dt; stepClip(player, dt);
            if (wastedT > 3600) respawn();
            updateWorld(dt); updateCamera(dt, keys);
            return;
        }
        if (toggleRun && inCar == null) { runMode = !runMode; say(runMode ? "Run: ON" : "Run: OFF  (walking)"); helpT = 1600; }
        if (nextWeapon && inCar == null) cycleWeapon();
        if (pAct != 0) updateAct(dt);
        else {
            if (enter) handleEnter();
            if (pAct == 0) { if (inCar != null) updateDriving(dt, keys); else updateOnFoot(dt, keys, fire); }
        }
        boolean inWater = inCar != null ? deep(inCar.x, inCar.z, inCar.y) : (pAct == 0 && deep(player.x, player.z, player.y));
        if (inWater) {
            if (drownT == 0) Snd.play(Snd.SPLASH);
            drownT += dt; if (inCar != null) { inCar.vx = inCar.vx * 9 / 10; inCar.vz = inCar.vz * 9 / 10; }
            if (drownT > 1500) { health = 0; wastedT = 0; say("Tommy can't swim!"); }
        } else drownT = 0;
        updatePickups(dt);
        saveT += dt;
        if (saveT > 20000 && pAct == 0) { saveT = 0; Save.write(this); }
        updateWorld(dt);
        updateCamera(dt, keys);
        updateWanted(dt);
        Snd.engine(inCar != null && !inCar.wrecked, inCar != null ? Fx.abs(inCar.speed) * 100 / Math.max(1, vmaxOf(inCar)) : 0);
        boolean siren = false;
        for (int i = 0; i < ncars; i++) if (cars[i].mode == Car.POLICE && Fx.dist(cars[i].x - player.x, cars[i].z - player.z) < 90000) siren = true;
        Snd.siren(siren);
    }

    void cycleWeapon() {
        for (int k = 0; k < 3; k++) {
            weapon = (weapon + 1) % 3;
            if (weapon == 0 || (weapon == 1 && hasBat) || (weapon == 2 && hasPistol && ammo > 0)) break;
        }
        say(weapon == 0 ? "Fists" : (weapon == 1 ? "Baseball bat" : "Pistol  (" + ammo + ")")); helpT = 1200;
    }
    void updatePickups(int dt) {
        for (int i = 0; i < npick; i++) {
            if (pkLife[i] < 0) continue;                                        // free slot
            if (pkLife[i] > 0) { pkLife[i] -= dt; if (pkLife[i] <= 0) { pkLife[i] = -1; continue; } }
            if (pkT[i] > 0) { pkT[i] -= dt; continue; }
            if (inCar == null && Fx.dist(pkX[i] - player.x, pkZ[i] - player.z) < 1200) {
                if (pkType[i] == 3) { money += pkAmt[i]; say("$" + pkAmt[i]); }
                else if (pkType[i] == 2) { hasPistol = true; ammo += pkLife[i] > 0 ? 17 : 68; if (weapon == 0) weapon = 2; say("Pistol  (" + ammo + ")"); }
                else { hasBat = true; if (weapon == 0) weapon = 1; say("Baseball bat"); }
                helpT = 1500;
                Snd.play(Snd.PICKUP);
                if (pkLife[i] > 0) pkLife[i] = -1; else pkT[i] = 30000;
            }
        }
    }

    void updateWanted(int dt) {
        if (wanted == 0) return;
        if (copsNear(40 * 1024) == 0) {
            wantedCalmT += dt;
            if (wantedCalmT > 22000) { wanted--; wantedCalmT = 0; if (wanted == 0) say("You lost the cops."); }
        } else wantedCalmT = 0;
        int foot = 0, carsP = 0;
        for (int i = 0; i < nped; i++) if (peds[i].cop && peds[i].state != Ped.DEAD && peds[i].seat == null) foot++;
        for (int i = 0; i < ncars; i++) if (cars[i].mode == Car.POLICE) carsP++;
        int maxFoot = wanted == 1 ? 1 : (wanted == 2 ? 2 : 3);
        if (foot < maxFoot && (time % 4000) < dt) spawnCop();
        if (wanted >= 2 && carsP < (wanted >= 4 ? 2 : 1) && (time % 9000) < dt) spawnPoliceCar();
    }

    void respawn() {
        wastedT = 0; health = 100; armor = 0;
        money = money > 100 ? money - 100 : 0;
        wanted = 0; quietKills = 0; weapon = 0; hasPistol = false; hasBat = false; ammo = 0;
        placePlayerAtSpawn(); preload(player.x, player.z); clearCops();
        say("You have been treated. $100 fee.");
    }
    void arrestRespawn() {
        money = money > 100 ? money - 100 : 0;
        wanted = 0; quietKills = 0; weapon = 0; hasPistol = false; hasBat = false; ammo = 0;
        placePlayerAtSpawn(); preload(player.x, player.z); clearCops();
        say("Busted! $100 bail. Weapons confiscated.");
    }
    void clearCops() {
        for (int i = nped - 1; i >= 0; i--) if (peds[i].cop && peds[i].seat == null) { peds[i] = peds[nped - 1]; nped--; }
        for (int i = ncars - 1; i >= 0; i--) if (cars[i].mode == Car.POLICE) removeCar(i);
    }

    int localX(Car c, int lx, int lz) { return c.x + (((Fx.cos(c.yaw) * lx + Fx.sin(c.yaw) * lz) >> 14) * 16); }
    int localZ(Car c, int lx, int lz) { return c.z + (((-Fx.sin(c.yaw) * lx + Fx.cos(c.yaw) * lz) >> 14) * 16); }
    int seatLX(Car c) { CarModel m = cm(c); return m.bike ? 0 : -Fx.abs(m.seatX); }
    int seatLZ(Car c) { return cm(c).seatZ; }

    void handleEnter() {
        if (inCar != null) {
            Car c = inCar; Ped p = player;
            p.x = localX(c, seatLX(c), seatLZ(c)); p.z = localZ(c, seatLX(c), seatLZ(c)); p.yaw = c.yaw; p.y = c.y;
            p.seat = null; c.mode = Car.PARKED; inCar = null;
            Snd.engine(false, 0);
            if (cm(c).bike) { p.x = localX(c, -60, 0); p.z = localZ(c, -60, 0); p.clip = -1; p.y = groundH(p.x, p.z, c.y); return; }
            Snd.play(Snd.DOOR_OPEN);
            startClip(p, cGetout, -1, 0);
            pAct = PA_GETOUT;
            return;
        }
        Car best = null; int bd = 4600;
        for (int i = 0; i < ncars; i++) {
            Car c = cars[i];
            if (c.wrecked) continue;
            int d = Fx.dist(c.x - player.x, c.z - player.z);
            if (d < bd && Fx.abs(c.speed) < 3500) { bd = d; best = c; }
        }
        if (best != null) { pActCar = best; pAct = PA_WALKTO; player.clip = -1; camYawOff = 0; }
    }

    void updateAct(int dt) {
        Car c = pActCar; Ped p = player;
        boolean bike = cm(c).bike;
        if (pAct == PA_WALKTO) {
            int tx = bike ? localX(c, -60, 0) : localX(c, seatLX(c) - 59, seatLZ(c) - 34), tz = bike ? localZ(c, -60, 0) : localZ(c, seatLX(c) - 59, seatLZ(c) - 34);
            int dx = tx - p.x, dz = tz - p.z, d = Fx.dist(dx, dz);
            if (d < 450) {
                p.x = tx; p.z = tz; p.yaw = c.yaw; p.speed = 0;
                if (c.driver != null && c.driver.seat == c) {
                    Ped v = c.driver;
                    v.seat = null; c.driver = null;
                    v.x = localX(c, seatLX(c), seatLZ(c)); v.z = localZ(c, seatLX(c), seatLZ(c)); v.yaw = c.yaw; v.y = c.y;
                    v.state = Ped.JACKED; v.hp = 100;
                    startClip(v, cJacked, cGetup, 0);
                    if (v.cop) raiseWanted(2); else if (copsNear(30 * 1024) > 0) raiseWanted(1);
                    pedsHit++;
                    if (bike) p.clip = -1; else startClip(p, cPull, cGetin, 0);
                } else if (bike) p.clip = -1;
                else { startClip(p, cOpen, cGetin, 0); Snd.play(Snd.DOOR_OPEN); }
                pAct = PA_GETIN;
            } else {
                int want = Fx.yawOf(dx, dz);
                p.yaw = (p.yaw + Fx.clamp(Fx.angDiff(p.yaw, want), -700 * dt / 1000, 700 * dt / 1000)) & 1023;
                int sp = 4200;
                p.x += ((Fx.sin(p.yaw) * sp) >> 14) * dt / 1000; p.z += ((Fx.cos(p.yaw) * sp) >> 14) * dt / 1000;
                p.y = groundH(p.x, p.z, p.y);
                p.speed = sp; selectLoco(p, dt, sp);
            }
        } else if (pAct == PA_GETIN) {
            stepClip(p, dt);
            if (p.clip < 0) {
                inCar = c; c.mode = Car.PLAYER; p.seat = c; p.speed = 0; pAct = 0; carsStolen++;
                p.x = c.x; p.z = c.z;
                if (!bike) Snd.play(Snd.DOOR_CLOSE);
                if (cm(c).name.equals("police")) raiseWanted(1);
            }
        } else if (pAct == PA_GETOUT) {
            stepClip(p, dt);
            if (p.clip < 0) { pAct = 0; p.y = groundH(p.x, p.z, p.y); Snd.play(Snd.DOOR_CLOSE); }
        }
    }

    boolean isAttack(int c) { return c == cJab || c == cPunch || c == cHeavy; }

    /** On foot: speed eases to walk 2 / run 6.2 / sprint 8.8 m/s (VC-like), climbs steps up to ~1.1 m, jumps with gravity. */
    void updateOnFoot(int dt, int keys, boolean fire) {
        Ped p = player;
        if (p.clip >= 0 && isAttack(p.clip)) {
            stepClip(p, dt);
            if (p.clip >= 0 && !p.hitDone && p.clipT >= 170) {
                p.hitDone = true;
                doPunch(weapon == 1 ? 55 : (combo == 3 ? 45 : 28), weapon == 1 ? 2200 : 1700);
            }
            p.speed = 0;
            return;
        }
        if (p.clip >= 0) stepClip(p, dt);
        int turn = 0, move = 0;
        if ((keys & K_LEFT) != 0) turn -= 1;
        if ((keys & K_RIGHT) != 0) turn += 1;
        if ((keys & K_UP) != 0) move += 1;
        if ((keys & K_DOWN) != 0) move -= 1;
        if ((keys & K_SPRINT) != 0) sprintHeld += dt; else sprintHeld = 0;
        boolean sprint = sprintHeld > 350;
        int tr = Fx.abs(p.speed) > 4000 ? 640 : 860;
        if (!p.air) p.yaw = (p.yaw + turn * tr * dt / 1000) & 1023;
        int want = 0;
        if (move > 0) want = sprint ? 8800 : (runMode ? 6200 : 2000);
        else if (move < 0) want = runMode ? -3000 : -2000;
        want = want * speedMul / 100;
        boolean pistol = weapon == 2 && hasPistol;
        if (pistol && (keys & K_FIRE) != 0) {
            aimT = 900;
            lockOn();
            shoot();
            if (want > 2000) want = 2000;
        } else if (!pistol && fire && !p.air) {
            if (weapon == 1) { startClip(p, cPunch, -1, 0); Snd.play(Snd.SWING); return; }
            combo = combo % 3 + 1;
            if (combo == 1) startClip(p, cJab, -1, 560); else if (combo == 2) startClip(p, cPunch, -1, 0); else startClip(p, cJab, -1, 700);
            if (Fx.rnd(4) == 0) voice(Snd.V_TOMMY, p);
            return;
        }
        if ((keys & K_FIRE) == 0 && aimT <= 0) target = null;
        if (!p.air) {
            int accel = (want == 0 || (want > 0) != (p.speed > 0)) ? 18000 : 11000;
            int d = want - p.speed, stp = accel * dt / 1000;
            p.speed += d > stp ? stp : (d < -stp ? -stp : d);
            if (jumpReq && p.clip < 0) { p.air = true; p.vy = 300; startClip(p, cJumpL, -1, 0); }
        }
        int sp = p.speed;
        rx = p.x + ((Fx.sin(p.yaw) * sp) >> 14) * dt / 1000;
        rz = p.z + ((Fx.cos(p.yaw) * sp) >> 14) * dt / 1000;
        resolve(300);
        int gCur = p.air ? p.y : groundH(p.x, p.z, p.y);
        int ox = p.x, oz = p.z, lim = p.air ? 20 : 72;
        if (groundH(rx, rz, gCur + 40) - gCur > lim) {          // higher than a step: slide along it
            int nx = rx, nz = rz;
            rz = oz;
            if (groundH(rx, rz, gCur + 40) - gCur > lim) { rx = ox; rz = nz; if (groundH(rx, rz, gCur + 40) - gCur > lim) { rx = ox; rz = oz; } }
        }
        for (int i = 0; i < ncars; i++) {
            Car c = cars[i];
            CarModel m = cm(c);
            int half = m.bike ? 600 : m.dimZ * 16 / 2 - 900, rr = m.bike ? 750 : m.dimX * 16 / 2 + 350;
            if (half < 0) half = 0;
            for (int k = -1; k <= 1; k++) {
                int cx = c.x + ((Fx.sin(c.yaw) * k * half) >> 14), cz = c.z + ((Fx.cos(c.yaw) * k * half) >> 14);
                int dx = rx - cx, dz = rz - cz;
                if (dx > rr || dx < -rr || dz > rr || dz < -rr) continue;
                int dd = Fx.dist(dx, dz);
                if (dd < rr && dd > 0) { rx = cx + dx * rr / dd; rz = cz + dz * rr / dd; }
            }
        }
        int mv = Fx.dist(rx - p.x, rz - p.z);
        p.x = rx; p.z = rz;
        int gy = groundH(p.x, p.z, (p.air ? p.y : gCur) + 40);
        if (p.air) {
            p.vy -= 627 * dt / 1000;
            p.y += p.vy * dt / 1000;
            if (p.clip < 0) { p.loco = cJumpG; p.locoT += dt; }
            if (p.y <= gy && p.vy <= 0) { p.y = gy; p.air = false; p.vy = 0; startClip(p, cJumpLand, -1, 250); }
            return;
        }
        if (mv == 0 && sp != 0) p.speed = sp / 2;
        if (p.clip < 0) selectLoco(p, dt, mv == 0 && sp != 0 ? sp / 6 : sp);
        if (gy < p.y - 40) { p.air = true; p.vy = 0; }
        else p.y = gy;
    }

    /** VC auto-aim: keep the current target while alive, in range and roughly in front */
    void lockOn() {
        Ped p = player;
        if (target != null && (target.state == Ped.DEAD || target.seat != null || Fx.dist(target.x - p.x, target.z - p.z) > 40000)) target = null;
        if (target == null) {
            int fx = Fx.sin(p.yaw), fz = Fx.cos(p.yaw), bd = 36000;
            for (int i = 0; i < nped; i++) {
                Ped o = peds[i];
                if (o.state == Ped.DEAD || o.seat != null) continue;
                int dx = o.x - p.x, dz = o.z - p.z, d = Fx.dist(dx, dz);
                if (d > bd || d < 1) continue;
                if ((dx * fx + dz * fz) / 16384 < d * 70 / 100) continue;
                bd = d; target = o;
            }
        }
        if (target != null) p.yaw = Fx.yawOf(target.x - p.x, target.z - p.z);
    }

    void voice(int kind, Ped p) {
        if (voiceCd > 0 || Fx.dist(p.x - player.x, p.z - player.z) > 25000) return;
        voiceCd = 1800;
        Snd.voice(kind, p.female);
    }

    /** debug menu: put any vehicle from the list in front of Tommy */
    void debugSpawn(int src) {
        try {
            int slot = -1;
            for (int s2 = NCPOOL - 1; s2 >= 1 && slot < 0; s2--) {
                boolean used = false;
                for (int i = 0; i < ncars; i++) if (cars[i].type == s2) used = true;
                if (!used) slot = s2;
            }
            if (slot < 0) { for (int i = ncars - 1; i >= 0; i--) if (cars[i] != inCar && cars[i].type != 0) { slot = cars[i].type; break; } }
            if (slot < 0) return;
            for (int i = ncars - 1; i >= 0; i--) if (cars[i].type == slot && cars[i] != inCar) removeCar(i);
            loadCar(slot, src);
            int fx = Fx.sin(player.yaw), fz = Fx.cos(player.yaw);
            Car c = addCar(Car.PARKED, slot, player.x + ((fx * 5000) >> 14), player.z + ((fz * 5000) >> 14), player.yaw + 256, Fx.rnd(10));
            if (c != null) say(carNames[src]);
        } catch (Exception e) { say("can't load " + carNames[src]); }
    }

    /** pistol: locks on to the nearest ped in front (VC auto-aim), flash + sound, scares the street */
    void shoot() {
        if (shotCd > 0) return;
        if (ammo <= 0) { weapon = 0; say("Out of ammo"); return; }
        shotCd = 380; ammo--;
        Ped p = player, best = target;
        Snd.play(Snd.GUN);
        flashT = 70;
        if (best != null) {
            p.yaw = Fx.yawOf(best.x - p.x, best.z - p.z);
            damagePed(best, 26 + Fx.rnd(10), true, p.x, p.z);
        }
        for (int i = 0; i < nped; i++) {
            Ped o = peds[i];
            if (o.cop || o.seat != null || o.state == Ped.DEAD || o.state == Ped.JACKED || o.angry) continue;
            if (Fx.dist(o.x - p.x, o.z - p.z) < 30000 && o.clip < 0) { if (o.state != Ped.FLEE) voice(Snd.V_PANIC, o); o.state = Ped.FLEE; o.timer = 5000; o.yaw = Fx.yawOf(o.x - p.x, o.z - p.z); }
        }
        if (copsNear(35 * 1024) > 0) raiseWanted(best != null && best.cop ? 2 : 1);
    }

    void doPunch(int dmgBase, int reach) {
        Ped p = player;
        int fx = Fx.sin(p.yaw), fz = Fx.cos(p.yaw);
        for (int i = 0; i < nped; i++) {
            Ped o = peds[i];
            if (o.state == Ped.DEAD || o.seat != null || o.state == Ped.JACKED) continue;
            int dx = o.x - p.x, dz = o.z - p.z;
            int d = Fx.dist(dx, dz);
            if (d > reach || d == 0) continue;
            if ((dx * fx + dz * fz) / 16384 < d * 5 / 10) continue;
            Snd.play(weapon == 1 ? Snd.BAT : Snd.PUNCH);
            damagePed(o, dmgBase + Fx.rnd(14), true, p.x, p.z);
            int push = o.state == Ped.DEAD ? 0 : 250;
            rx = o.x + dx * push / d; rz = o.z + dz * push / d;
            resolve(280);
            if (walkable(rx, rz, o.y)) { o.x = rx; o.z = rz; }
            break;
        }
    }

    void damagePed(Ped o, int dmg, boolean byPlayer, int ax, int az) {
        if (o.state == Ped.DEAD) return;
        o.hp -= dmg;
        pedsHit++;
        boolean front = ((ax - o.x) >> 4) * (Fx.sin(o.yaw) >> 4) + ((az - o.z) >> 4) * (Fx.cos(o.yaw) >> 4) > 0;
        boolean witnessed = byPlayer && copsNear(30 * 1024) > 0;
        if (o.hp <= 0) {
            o.state = Ped.DEAD; o.timer = 14000; o.deadT = 0; o.angry = false; o.aimT = 0;
            voice(Snd.V_DEATH, o);
            int back = front ? -700 : 700;
            o.bloodX = o.x + ((Fx.sin(o.yaw) * back) >> 14); o.bloodZ = o.z + ((Fx.cos(o.yaw) * back) >> 14);
            o.bloodY = groundH(o.bloodX, o.bloodZ, o.y);
            startClip(o, front ? cKoF : cKoB, -2, 0);
            o.cash = 0;
            if (o.cop) dropPickup(2, 0, o.bloodX, o.bloodZ, o.y);                 // cops always drop their pistol
            else if (Fx.rnd(100) < 55) dropPickup(3, 5 + Fx.rnd(60) + (o.gang ? 40 : 0), o.x + ((Fx.sin(o.yaw) * 400) >> 14), o.z + ((Fx.cos(o.yaw) * 400) >> 14), o.y);
            if (byPlayer) {
                kills++;
                if (o.cop) raiseWanted(wanted >= 1 ? 2 : 1);
                else if (witnessed) raiseWanted(1);
                else if (wanted == 0 && ++quietKills >= quietLimit) { raiseWanted(1); quietKills = 0; quietLimit = 3 + Fx.rnd(3); }
            }
        } else {
            o.state = Ped.HIT;
            startClip(o, front ? cHitF : cHitB, -1, 0);
            voice(o.brave ? Snd.V_FIGHT : Snd.V_PANIC, o);
            if (byPlayer) {
                if (o.cop) raiseWanted(1); else if (witnessed) raiseWanted(1);
                if (o.brave && !o.cop) o.angry = true;
            }
        }
    }

    void hurt(int dmg) {
        if (health <= 0 || dmg <= 0 || god) return;
        if (armor > 0) { armor -= dmg; if (armor < 0) { health += armor; armor = 0; } }
        else health -= dmg;
        if (health <= 0) { health = 0; wastedT = 0; }
        shake = 250;
    }

    // ------------------------------------------------------------------ vehicles
    void updateDriving(int dt, int keys) {
        Car c = inCar;
        int acc = 0, st = 0;
        if ((keys & (K_UP | K_FIRE)) != 0) acc += 1;
        if ((keys & K_DOWN) != 0) acc -= 1;
        if ((keys & K_LEFT) != 0) st -= 1;
        if ((keys & K_RIGHT) != 0) st += 1;
        if (c.wrecked) acc = 0;
        driveCar(c, dt, acc, st, (keys & K_HAND) != 0);
        player.x = c.x; player.z = c.z; player.yaw = c.yaw; player.y = c.y;
    }

    /**
     * Arcade car model with a real velocity vector: engine/brakes act along the heading, tyre grip removes
     * sideways velocity (much less grip with the handbrake -> powerslides), yaw follows the bicycle model
     * with a grip-limited response, slopes add gravity, and the body pitches/rolls from the accelerations.
     */
    void driveCar(Car c, int dt, int acc, int st, boolean hand) {
        CarModel m = cm(c);
        int fx = Fx.sin(c.yaw), fz = Fx.cos(c.yaw);
        int vF = (int) (((long) c.vx * fx + (long) c.vz * fz) >> 14);
        int vR = (int) (((long) c.vx * fz - (long) c.vz * fx) >> 14);
        int vmax = m.vmax * 1024, ac = m.accel * 1024 / 100, br = m.brake * 1024 / 100;
        int a = 0;
        c.braking = false;
        if (acc > 0) {
            if (vF < -400) { a = br; c.braking = true; }
            else if (vF < vmax) a = (int) ((long) ac * (vmax - vF + vmax / 5) / vmax);
        } else if (acc < 0) {
            if (vF > 400) { a = -br; c.braking = true; }
            else if (vF > -vmax / 3) a = -ac / 2;
        }
        int drag = 150 + Fx.abs(vF) / 16 + (int) ((long) vF * vF / 2600000);
        if (acc == 0) drag += 800;
        if (hand) drag += br / 2;
        int stopA = (int) ((long) Fx.abs(vF) * 1000 / Math.max(dt, 1));
        if (drag > stopA) drag = stopA;
        a += vF > 0 ? -drag : (vF < 0 ? drag : 0);
        int hf = groundH(c.x + ((fx * 1600) >> 14), c.z + ((fz * 1600) >> 14), c.y), hb = groundH(c.x - ((fx * 1600) >> 14), c.z - ((fz * 1600) >> 14), c.y);
        int slope = (hf - hb) * 16 * 1024 / 3200;
        if (Fx.abs(slope) < 700) a -= slope * 9800 / 1024;
        else slope = 0;
        int s = surfAt(c.x, c.z);
        if ((s == S_SAND || s == S_GRASS) && vF > 6000) a -= vF / 6;
        vF += a * dt / 1000;
        if (acc == 0 && Fx.abs(vF) < 150) vF = 0;
        int asp = Fx.abs(vF);
        int lock = m.lockDeg * 1024 / 360;
        int k = Math.min(asp, 45000) >> 6, kv = 15000 >> 6;
        int lockEff = lock * 256 / (256 + k * k * 256 / (kv * kv));
        int target = st * lockEff;
        int rate = (st == 0 ? 900 : 480) * dt / 1000 + 1;
        c.steer += Fx.clamp(target - c.steer, -rate, rate);
        int cs = Fx.cos(c.steer);
        int tan14 = (Fx.sin(c.steer) << 14) / (cs < 1000 ? 1000 : cs);
        int want = (int) (((long) vF * tan14 / m.wheelbase * 163) >> 14);
        if (hand && asp > 4000) want = want * 16 / 10;
        int yawGrip = (hand ? 3 : 9) * m.grip / 100;
        c.yawVel += (want - c.yawVel) * Math.min(1000, yawGrip * dt) / 1000;
        c.yaw = (c.yaw + c.yawVel * dt / 1000) & 1023;
        int grip = m.grip * (hand ? 3 : 12);
        if (Fx.abs(vR) > 7000 && !hand) grip = grip * 2 / 3;
        int dR = grip * dt;
        if (vR > dR) vR -= dR; else if (vR < -dR) vR += dR; else vR = 0;
        c.slide = Fx.abs(vR) > 2500 || (hand && asp > 6000);
        fx = Fx.sin(c.yaw); fz = Fx.cos(c.yaw);
        c.vx = (int) (((long) vF * fx + (long) vR * fz) >> 14);
        c.vz = (int) (((long) vF * fz - (long) vR * fx) >> 14);
        c.speed = vF;
        int pitchT = -a / 900, rollT = (int) ((long) c.yawVel * vF / 260000);
        c.pitch += (Fx.clamp(pitchT, -9, 9) - c.pitch) * Math.min(dt, 100) / 140;
        c.roll += (Fx.clamp(rollT, -10, 10) - c.roll) * Math.min(dt, 100) / 140;
        c.slopePitch += ((slope * 163 >> 10) - c.slopePitch) * Math.min(dt, 100) / 100;
        moveCar(c, dt);
    }

    void damageCar(Car c, int region, int amount) {
        if (amount <= 0) return;
        c.hp -= amount;
        int[] parts;
        switch (region) {
            case 0: parts = new int[] { CarModel.P_BUMP_F, CarModel.P_BONNET, CarModel.P_WING_LF, CarModel.P_WING_RF, CarModel.P_WINDSCREEN }; break;
            case 1: parts = new int[] { CarModel.P_BUMP_R, CarModel.P_BOOT }; break;
            case 2: parts = new int[] { CarModel.P_DOOR_LF, CarModel.P_DOOR_LR, CarModel.P_WING_LF }; break;
            default: parts = new int[] { CarModel.P_DOOR_RF, CarModel.P_DOOR_RR, CarModel.P_WING_RF }; break;
        }
        for (int i = 0; i < parts.length; i++) {
            int p = parts[i];
            int a = i == 0 ? amount : amount * 2 / 3;
            if (p == CarModel.P_WINDSCREEN) a = amount / 2;
            c.partDmg[p] += a;
            if (c.partDmg[p] > 150) c.damBits |= 1 << p;
            boolean canFall = p == CarModel.P_BUMP_F || p == CarModel.P_BUMP_R || p == CarModel.P_BONNET || p == CarModel.P_BOOT || (p >= CarModel.P_DOOR_LF && p <= CarModel.P_DOOR_RR);
            if (canFall && c.partDmg[p] > 700) c.lostBits |= 1 << p;
        }
        if (c.hp <= 0 && !c.wrecked && !c.burning) {            // engine catches fire, explodes a few seconds later
            c.hp = 0; c.burning = true; c.burnT = 0;
            if (c == inCar) say("Get out, it's going to blow!");
        }
    }

    int dbgHit;
    void moveCar(Car c, int dt) {
        int ox = c.x, oz = c.z;
        int gHere = Math.max(c.y, groundH(c.x, c.z, c.y));
        c.x += c.vx * dt / 1000; c.z += c.vz * dt / 1000;
        CarModel m = cm(c);
        int half = m.bike ? 500 : m.dimZ * 16 / 2 - 900, rad = m.bike ? 380 : m.dimX * 16 * 4 / 10;
        if (half < 200) half = 200;
        boolean hit = false;
        int nx = 0, nz = 0;
        for (int k = -1; k <= 1; k++) {
            int ox2 = (Fx.sin(c.yaw) * k * half) >> 14, oz2 = (Fx.cos(c.yaw) * k * half) >> 14;
            rx = c.x + ox2; rz = c.z + oz2;
            int bx = rx, bz = rz;
            if (resolve(rad)) { dbgHit = 10 + k; hit = true; nx += rx - bx; nz += rz - bz; c.x = rx - ox2; c.z = rz - oz2; }
            int g = groundH(c.x + ox2, c.z + oz2, gHere);
            if (g - gHere > 72 && g > -60 && groundH(c.x + ox2 * 12 / 10, c.z + oz2 * 12 / 10, gHere) - gHere > 72) { dbgHit = 20 + k + g * 100; hit = true; nx += ox - c.x; nz += oz - c.z; c.x = ox; c.z = oz; }   // > 1.1 m: a wall, not a kerb or ramp
        }
        if (hit) {
            int nl = Fx.dist(nx, nz);
            int vn;
            if (nl > 0) {
                vn = (int) (((long) c.vx * nx + (long) c.vz * nz) / nl);
                if (vn < 0) { c.vx -= (int) ((long) vn * nx * 13 / 10 / nl); c.vz -= (int) ((long) vn * nz * 13 / 10 / nl); }
            } else { vn = -Fx.abs(c.speed); c.vx = -c.vx / 3; c.vz = -c.vz / 3; }
            int imp = Fx.abs(vn);
            if (imp > 3500) {
                if (c.mode == Car.PLAYER) { shake = 300; hurt(imp / 6000); }
                damageCar(c, c.speed > 0 ? 0 : 1, imp / 140);
                if (Fx.dist(c.x - player.x, c.z - player.z) < 40000) Snd.play(Snd.CRASH);
            }
            c.vx = c.vx * 85 / 100; c.vz = c.vz * 85 / 100;
        }
        for (int i = 0; i < ncars; i++) {
            Car o = cars[i];
            if (o == c) continue;
            int ddx = o.x - c.x, ddz = o.z - c.z;
            if (ddx > 7000 || ddx < -7000 || ddz > 7000 || ddz < -7000) continue;
            CarModel om = cm(o);
            int rr = (m.bike ? 900 : 1500) + (om.bike ? 900 : 1500);
            int d = Fx.dist(ddx, ddz);
            if (d < rr && d > 0) {
                int push = (rr - d) / 2;
                o.x += ddx * push / d; o.z += ddz * push / d;
                c.x -= ddx * push / d; c.z -= ddz * push / d;
                int rvx = c.vx - o.vx, rvz = c.vz - o.vz;
                int vn = (int) (((long) rvx * ddx + (long) rvz * ddz) / d);
                if (vn > 0) {
                    int mc = m.mass, mo = om.mass;
                    int jc = (int) ((long) vn * mo * 16 / 10 / (mc + mo)), jo = (int) ((long) vn * mc * 16 / 10 / (mc + mo));
                    c.vx -= jc * ddx / d; c.vz -= jc * ddz / d;
                    o.vx += jo * ddx / d; o.vz += jo * ddz / d;
                    if (vn > 3000) {
                        if (c.mode == Car.PLAYER || o.mode == Car.PLAYER) { shake = 250; Snd.play(Snd.CRASH); }
                        damageCar(c, regionOf(c, o.x, o.z), vn / 200);
                        damageCar(o, regionOf(o, c.x, c.z), vn / 200);
                        if ((o.mode == Car.POLICE || (o.driver != null && o.driver.cop)) && c.mode == Car.PLAYER) raiseWanted(1);
                    }
                }
            }
        }
        int g = groundH(c.x, c.z, c.y);
        if (g < c.y - 8) { c.vy -= 9800 * 64 / 1024 * dt / 1000; c.y += c.vy * dt / 1000; if (c.y < g) { c.y = g; c.vy = 0; } }
        else { if (g > c.y) c.y = g; else c.y += (g - c.y) * Math.min(dt, 100) / 40; c.vy = 0; }
        c.wheelRot = (c.wheelRot + c.speed * dt / 1000 * 1024 / 2200) & 1023;
        if (Fx.abs(c.speed) > 2500) {
            for (int i = 0; i < nped; i++) {
                Ped p = peds[i];
                if (p.state == Ped.DEAD || p.seat != null || p.state == Ped.JACKED) continue;
                int ddx = p.x - c.x, ddz = p.z - c.z;
                if (ddx > 2800 || ddx < -2800 || ddz > 2800 || ddz < -2800) continue;
                if (Fx.dist(ddx, ddz) < (m.bike ? 1300 : 2000)) {
                    damagePed(p, Fx.abs(c.speed) / 140, c.mode == Car.PLAYER, c.x, c.z);
                    p.x += ((Fx.sin(c.yaw) * 1500) >> 14); p.z += ((Fx.cos(c.yaw) * 1500) >> 14);
                    if (c.mode == Car.PLAYER) { shake = 150; Snd.play(Snd.HITPED); }
                }
            }
            if (c.mode != Car.PLAYER && inCar == null && pAct == 0) {
                int ddx = player.x - c.x, ddz = player.z - c.z;
                if (Fx.dist(ddx, ddz) < 1900) { hurt(Fx.abs(c.speed) / 1024 * 3); c.vx /= 3; c.vz /= 3; }
            }
        }
    }

    void explode(Car c) {
        c.wrecked = true; c.burning = false; c.color = 0x2A2622; c.color2 = 0x2A2622; c.vx = c.vz = 0; c.speed = 0;
        c.lostBits |= (1 << CarModel.P_BONNET) | (1 << CarModel.P_DOOR_LF) | (1 << CarModel.P_BOOT); c.damBits = -1;
        Snd.play3d(Snd.EXPLODE, c.x, c.z, player.x, player.z);
        for (int k = 0; k < 6; k++) spawnFx(2, (c.x >> 4) + Fx.rnd(120) - 60, c.y + 40 + Fx.rnd(60), (c.z >> 4) + Fx.rnd(120) - 60);
        int d = Fx.dist(c.x - player.x, c.z - player.z);
        if (d < 30000) shake = 600;
        if (c == inCar) { health = 0; wastedT = 0; }
        else if (d < 7000 && inCar == null) hurt(110 - d / 100);
        for (int i = 0; i < nped; i++) {
            Ped p = peds[i];
            if (p.state != Ped.DEAD && p.seat == null && Fx.dist(p.x - c.x, p.z - c.z) < 6000) damagePed(p, 200, false, c.x, c.z);
        }
        for (int i = 0; i < ncars; i++) if (cars[i] != c && Fx.dist(cars[i].x - c.x, cars[i].z - c.z) < 7000) damageCar(cars[i], regionOf(cars[i], c.x, c.z), 300);
    }

    // ---- particles: 0 smoke (blended), 1 fire, 2 explosion (additive). positions 1/64 m
    static final int NFX = 72;
    int[] fxX = new int[NFX], fxY = new int[NFX], fxZ = new int[NFX], fxT = new int[NFX], fxL = new int[NFX], fxM = new int[NFX];
    void spawnFx(int type, int x, int y, int z) {
        int k = -1;
        for (int i = 0; i < NFX; i++) if (fxL[i] <= 0) { k = i; break; }
        if (k < 0) k = Fx.rnd(NFX);
        fxX[k] = x; fxY[k] = y; fxZ[k] = z; fxT[k] = type;
        fxM[k] = fxL[k] = type == 0 ? 1800 + Fx.rnd(600) : (type == 1 ? 450 + Fx.rnd(250) : 900);
    }
    void updateParticles(int dt) {
        for (int i = 0; i < NFX; i++) {
            if (fxL[i] <= 0) continue;
            fxL[i] -= dt;
            int up = fxT[i] == 0 ? 90 : (fxT[i] == 1 ? 130 : 30);
            fxY[i] += up * dt / 1000;
            if (fxT[i] == 0) fxX[i] += 20 * dt / 1000;                 // a little wind
        }
    }
    void drawParticles() {
        for (int i = 0; i < NFX; i++) {
            if (fxL[i] <= 0) continue;
            int age = 256 - fxL[i] * 256 / fxM[i];
            int size = fxT[i] == 0 ? 70 + age / 2 : (fxT[i] == 1 ? 130 - age / 6 : 160 + age);
            int fade = fxT[i] == 0 ? (age < 40 ? age * 4 : 300 - age) : 256 - age;
            if (fade <= 0) continue;
            Sky.particle(r, fxT[i], fxX[i], fxY[i], fxZ[i], size, fade > 256 ? 256 : fade);
        }
    }

    int regionOf(Car c, int x, int z) {
        int dx = x - c.x, dz = z - c.z;
        int lx = (Fx.cos(c.yaw) * dx - Fx.sin(c.yaw) * dz) >> 14, lz = (Fx.sin(c.yaw) * dx + Fx.cos(c.yaw) * dz) >> 14;
        if (Fx.abs(lz) > Fx.abs(lx) * 2) return lz > 0 ? 0 : 1;
        return lx < 0 ? 2 : 3;
    }

    void updateWorld(int dt) {
        updateParticles(dt);
        for (int i = 0; i < ncars; i++) {
            Car c = cars[i];
            if (!c.wrecked) {
                c.fxT += dt;
                CarModel m = cm(c);
                int ez = m.bike ? 0 : m.dimZ * 30 / 100;
                if (c.burning) {
                    c.burnT += dt;
                    if (c.fxT > 50) { c.fxT = 0; spawnFx(1, localX(c, Fx.rnd(60) - 30, ez - 30), c.y + m.dimY * 6 / 10, localZ(c, Fx.rnd(60) - 30, ez - 30)); spawnFx(1, localX(c, Fx.rnd(40) - 20, ez), c.y + m.dimY * 7 / 10, localZ(c, Fx.rnd(40) - 20, ez)); spawnFx(0, localX(c, 0, ez), c.y + m.dimY, localZ(c, 0, ez)); }
                    if (c.burnT > 5000) explode(c);
                } else if (c.hp < 450 && c.fxT > (c.hp < 250 ? 90 : 220)) {
                    c.fxT = 0; spawnFx(0, localX(c, 0, ez), c.y + m.dimY * 7 / 10, localZ(c, 0, ez));
                }
            }
            if (c.mode == Car.TRAFFIC) trafficAI(c, dt);
            else if (c.mode == Car.POLICE) policeAI(c, dt);
            else if (c.mode == Car.PARKED && (c.vx != 0 || c.vz != 0)) driveCar(c, dt, 0, 0, true);
        }
        int traffic = 0;
        for (int i = ncars - 1; i >= 0; i--) {
            Car c = cars[i];
            int d = Fx.dist(c.x - player.x, c.z - player.z);
            if (c != inCar && c != pActCar && d > 150 * 1024) { removeCar(i); continue; }
            if (c.mode == Car.TRAFFIC) traffic++;
        }
        if (traffic < MAXTRAFFIC && (time % 1200) < dt) spawnTraffic(false);
        for (int i = 0; i < nped; i++) updatePed(peds[i], dt);
        int alive = 0;
        for (int i = 0; i < nped; i++) {
            Ped p = peds[i];
            if (p.seat != null) continue;
            int d = Fx.dist(p.x - player.x, p.z - player.z);
            if (p.cop) {
                if ((wanted == 0 && d > 50000) || d > 120000) { p.state = Ped.DEAD; p.timer = 0; p.deadT = -1; p.cop = false; }
                continue;
            }
            if (p.state != Ped.DEAD && p.state != Ped.JACKED && p.state != Ped.GETUP && d > 100000) { p.state = Ped.DEAD; p.timer = 0; p.deadT = -1; }
            if (p.state != Ped.DEAD) alive++;
        }
        if (alive < MAXPED && (time % 500) < dt) spawnPed(false);
    }

    /** right-hand lane follower on the road mask */
    void trafficAI(Car c, int dt) {
        if (c.driver == null || c.driver.seat != c) { c.mode = Car.PARKED; return; }
        int fx = Fx.sin(c.yaw), fz = Fx.cos(c.yaw), rxv = fz, rzv = -fx;
        int look = 6000 + Fx.abs(c.speed) / 2;
        int ax = c.x + ((fx * (look >> 4)) >> 10), az = c.z + ((fz * (look >> 4)) >> 10);
        int eR = 0, eL = 0;
        for (int d = 1; d <= 12; d++) { if (surfAt(ax + rxv * d / 16, az + rzv * d / 16) != S_ROAD) break; eR = d; }
        for (int d = 1; d <= 12; d++) { if (surfAt(ax - rxv * d / 16, az - rzv * d / 16) != S_ROAD) break; eL = d; }
        int st = 0;
        boolean aheadRoad = surfAt(ax, az) == S_ROAD;
        if (aheadRoad) {
            int width = eL + eR + 1, target = width / 4;
            if (eR > target + 1) st = 1; else if (eR < target - 1 || eR == 0) st = -1;
        } else {
            int bestA = 0, bestScore = -1;
            for (int a = -192; a <= 192; a += 64) {
                if (a == 0) continue;
                int yy = (c.yaw + a) & 1023, sc2 = 0;
                for (int s = 4; s <= 16; s += 4) if (surfAt(c.x + ((Fx.sin(yy) * s) >> 4), c.z + ((Fx.cos(yy) * s) >> 4)) == S_ROAD) sc2++;
                if (a > 0) sc2 = sc2 * 2 + 1; else sc2 *= 2;
                if (sc2 > bestScore) { bestScore = sc2; bestA = a; }
            }
            st = bestScore <= 1 ? 1 : (bestA > 0 ? 1 : -1);
        }
        boolean block = false;
        for (int i = 0; i < ncars && !block; i++) {
            Car o = cars[i];
            if (o == c) continue;
            int dx = o.x - c.x, dz = o.z - c.z;
            int fwd = (dx * fx + dz * fz) >> 14, lat = (dx * rxv + dz * rzv) >> 14;
            if (fwd > 0 && fwd < look && lat < 2200 && lat > -2200) block = true;
        }
        if (!block) {
            int dx = player.x - c.x, dz = player.z - c.z;
            int fwd = (dx * fx + dz * fz) >> 14, lat = (dx * rxv + dz * rzv) >> 14;
            if (inCar == null && fwd > 0 && fwd < look && lat < 2000 && lat > -2000) block = true;
            for (int i = 0; i < nped && !block; i++) {
                Ped p = peds[i];
                if (p.seat != null || p.state == Ped.DEAD) continue;
                dx = p.x - c.x; dz = p.z - c.z;
                fwd = (dx * fx + dz * fz) >> 14; lat = (dx * rxv + dz * rzv) >> 14;
                if (fwd > 0 && fwd < look && lat < 1600 && lat > -1600) block = true;
            }
        }
        int target = aheadRoad ? Math.min(vmaxOf(c), 11 * 1024) : 4 * 1024;
        int acc = block ? -1 : (c.speed < target ? 1 : 0);
        if (block && c.speed < 600) { acc = 0; c.vx = c.vz = 0; c.speed = 0; c.blocked += dt; } else c.blocked = 0;
        if (c.blocked > 2500 && (time % 4000) < dt) Snd.play3d(Snd.HORN, c.x, c.z, player.x, player.z);
        if (c.blocked > 7000) { c.blocked = 0; c.yaw = (c.yaw + 512) & 1023; }
        driveCar(c, dt, acc, st, false);
    }

    void policeAI(Car c, int dt) {
        if (c.driver == null || c.driver.seat != c) { c.mode = Car.PARKED; return; }
        int dx = player.x - c.x, dz = player.z - c.z, d = Fx.dist(dx, dz);
        int diff = Fx.angDiff(c.yaw, Fx.yawOf(dx, dz));
        int st = diff > 20 ? 1 : (diff < -20 ? -1 : 0);
        if (d < 10000 && inCar == null) {
            driveCar(c, dt, -1, 0, true);
            if (Fx.abs(c.speed) < 800) {
                Ped dr = c.driver; dr.seat = null; c.driver = null; c.mode = Car.PARKED;
                dr.x = localX(c, -90, 0); dr.z = localZ(c, -90, 0); dr.state = Ped.WALK; dr.y = groundH(dr.x, dr.z, c.y); dr.hp = 160;
                spawnCopAt(localX(c, 90, 0), localZ(c, 90, 0));
            }
            return;
        }
        driveCar(c, dt, Fx.abs(diff) > 300 && c.speed > 6000 ? 0 : 1, st, Fx.abs(diff) > 200 && c.speed > 8000);
    }

    void updatePed(Ped p, int dt) {
        if (p.seat != null) {
            Car c = p.seat;
            p.x = localX(c, seatLX(c), seatLZ(c)); p.z = localZ(c, seatLX(c), seatLZ(c)); p.yaw = c.yaw; p.y = c.y;
            return;
        }
        if (p.state == Ped.DEAD && p.deadT >= 0) p.deadT += dt;
        boolean finished = stepClip(p, dt);
        if (p.clip >= 0) {
            if (p.state == Ped.DEAD) { if (p.timer > 0) p.timer -= dt; }
            if ((p.cop || p.angry) && (p.clip == cPunch || p.clip == cJab) && p.clipT > 200 && !p.hitDone) {
                p.hitDone = true;
                if (inCar == null && pAct == 0 && Fx.dist(player.x - p.x, player.z - p.z) < 1800) { hurt(5 + Fx.rnd(4)); Snd.play(Snd.PUNCH); }
            }
            return;
        }
        if (finished || p.state == Ped.HIT || p.state == Ped.JACKED || p.state == Ped.GETUP) {
            if (p.state == Ped.JACKED) { p.state = Ped.GETUP; startClip(p, cGetup, -1, 0); return; }
            if (p.state == Ped.HIT || p.state == Ped.GETUP) { p.state = (p.cop || p.angry) ? Ped.WALK : Ped.FLEE; p.timer = 4000; p.yaw = Fx.yawOf(p.x - player.x, p.z - player.z); }
        }
        if (p.state == Ped.DEAD) {
            if (p.timer > 0) p.timer -= dt;
            return;
        }
        int sp;
        int dxp = player.x - p.x, dzp = player.z - p.z, dpl = Fx.dist(dxp, dzp);
        if ((p.cop && wanted > 0 && health > 0 && !busted) || (p.angry && health > 0)) {
            int want = Fx.yawOf(dxp, dzp);
            p.yaw = (p.yaw + Fx.clamp(Fx.angDiff(p.yaw, want), -500 * dt / 1000, 500 * dt / 1000)) & 1023;
            sp = dpl > 1500 ? (p.cop && wanted == 1 ? 3600 : 4600) : 0;
            if (p.attackCd > 0) p.attackCd -= dt;
            if (p.angry && dpl > 30000) p.angry = false;
            if (p.cop && wanted == 1) {
                if (dpl < 1500 && inCar == null && pAct == 0) {
                    p.closeT += dt;
                    if (p.closeT > 2200) { busted = true; bustedT = 0; p.closeT = 0; }
                } else p.closeT = 0;
                if ((player.clip == cPunch || player.clip == cJab || player.clip == cHeavy || aimT > 0) && dpl < 1700 && p.attackCd <= 0) { startClip(p, cPunch, -1, 0); p.attackCd = 2200; }
            } else if (p.cop && wanted >= 2 && dpl < 16000 && dpl > 2500 && p.attackCd <= 0 && inCar == null) {
                p.attackCd = 1600 + Fx.rnd(1200); p.aimT = 600; sp = 0;           // 2+ stars: cops shoot
                Snd.play3d(Snd.GUN, p.x, p.z, player.x, player.z);
                if (Fx.rnd(100) < 55) hurt(4 + Fx.rnd(4));
            } else if (dpl <= 1600 && p.attackCd <= 0 && inCar == null) { startClip(p, Fx.rnd(2) == 0 ? cPunch : cJab, -1, 0); p.attackCd = 1600 + Fx.rnd(900); }
            if (p.aimT > 0) { p.aimT -= dt; sp = 0; }
            p.speed = sp;
        } else {
            if (p.state == Ped.FLEE) { p.timer -= dt; sp = 5200; if (p.timer <= 0) p.state = Ped.WALK; }
            else {
                p.turnTimer -= dt;
                sp = p.cop ? 1300 : p.speed;
                if (p.state == Ped.IDLE) { sp = 0; if (p.turnTimer <= 0) { p.state = Ped.WALK; p.turnTimer = 2500; } }
                else if (p.turnTimer <= 0) {
                    p.targetYaw = (p.yaw + Fx.rnd(160) - 80) & 1023;
                    p.turnTimer = 3000 + Fx.rnd(6000);
                    if (Fx.rnd(6) == 0) { p.state = Ped.IDLE; p.turnTimer = 1500 + Fx.rnd(2500); }
                }
                p.yaw = (p.yaw + Fx.clamp(Fx.angDiff(p.yaw, p.targetYaw), -300 * dt / 1000, 300 * dt / 1000)) & 1023;
                if (dpl < 4000 && inCar == null && (time % 9000) < dt && Fx.rnd(2) == 0) voice(Snd.V_CHAT, p);
            }
            for (int i = 0; i < ncars; i++) {
                Car c = cars[i];
                if (Fx.abs(c.speed) < 2000) continue;
                int dx = p.x - c.x, dz = p.z - c.z;
                if (dx > 5000 || dx < -5000 || dz > 5000 || dz < -5000) continue;
                if (p.state != Ped.FLEE && Fx.dist(dx, dz) < 4000) { p.state = Ped.FLEE; p.timer = 2200; p.yaw = Fx.yawOf(dx, dz); if (Fx.rnd(3) == 0) voice(Snd.V_PANIC, p); }
            }
        }
        // look ahead: stay off water and walls, follow the pavement (rarely cross the road)
        int ax = p.x + ((Fx.sin(p.yaw) * 1500) >> 14), az = p.z + ((Fx.cos(p.yaw) * 1500) >> 14);
        boolean calm = p.state == Ped.WALK && !p.cop && !p.angry;
        if (sp > 0 && (!walkable(ax, az, p.y) || (calm && surfAt(ax, az) == S_ROAD && surfAt(p.x, p.z) != S_ROAD && Fx.rnd(40) != 0))) {
            int l = (p.yaw - 256) & 1023, rr = (p.yaw + 256) & 1023;
            boolean okL = walkable(p.x + ((Fx.sin(l) * 1500) >> 14), p.z + ((Fx.cos(l) * 1500) >> 14), p.y);
            boolean okR = walkable(p.x + ((Fx.sin(rr) * 1500) >> 14), p.z + ((Fx.cos(rr) * 1500) >> 14), p.y);
            p.targetYaw = okR && (!okL || Fx.rnd(2) == 0) ? rr : (okL ? l : (p.yaw + 512) & 1023);
            p.yaw = (p.yaw + Fx.clamp(Fx.angDiff(p.yaw, p.targetYaw), -90, 90)) & 1023;
            if (!p.cop && !p.angry) sp = sp / 3;
        }
        rx = p.x + ((Fx.sin(p.yaw) * sp) >> 14) * dt / 1000;
        rz = p.z + ((Fx.cos(p.yaw) * sp) >> 14) * dt / 1000;
        resolve(280);
        stepOk(p.x, p.z, p.y, 60);
        if (deep(rx, rz, p.y)) { rx = p.x; rz = p.z; }
        p.x = rx; p.z = rz;
        selectLoco(p, dt, sp);
        p.y = groundH(p.x, p.z, p.y);
    }

    // ------------------------------------------------------------------ camera
    void updateCamera(int dt, int keys) {
        int turnCam = 0;
        if ((keys & K_CAML) != 0) turnCam -= 1;
        if ((keys & K_CAMR) != 0) turnCam += 1;
        if (turnCam != 0) { camYawOff += turnCam * 520 * dt / 1000; camIdle = 0; }
        else {
            camIdle += dt;
            boolean moving = inCar != null ? Fx.abs(inCar.speed) > 1500 : player.speed > 0;
            if (moving && camIdle > 1500) camYawOff -= camYawOff * 3 * dt / 1000;
        }
        camYawOff = Fx.clamp(camYawOff, -512, 512);
        if ((keys & K_CAMU) != 0) { camPitch += 80 * dt / 1000 + 1; camIdle = 0; }
        if ((keys & K_CAMD) != 0) { camPitch -= 80 * dt / 1000 + 1; camIdle = 0; }
        camPitch = Fx.clamp(camPitch, 4, 150);
        int hy;
        if (inCar != null) {
            int spd = Fx.dist(inCar.vx, inCar.vz);
            hy = spd > 2500 ? Fx.yawOf(inCar.vx, inCar.vz) : inCar.yaw;           // follow the direction of travel
            if (inCar.speed < -1500) hy = (inCar.yaw + 512) & 1023;
        } else hy = player.yaw;
        int want = (hy + camYawOff) & 1023;
        int df = Fx.angDiff(camYaw, want);
        int rate = inCar != null ? 3 : 7;
        camYaw = (camYaw + df * rate * dt / 1000 + (df > 0 ? 1 : (df < 0 ? -1 : 0))) & 1023;
        if (Fx.abs(Fx.angDiff(camYaw, want)) < 2) camYaw = want;
        int tx = player.x, tz = player.z, ty = player.y + 100;
        int dist = inCar != null ? (cm(inCar).bike ? 5200 : 6800 + Fx.abs(inCar.speed) / 25) : camDistMm;
        int cp = Fx.cos(camPitch), sp = Fx.sin(camPitch);
        int hd = (dist * cp) >> 14, vd = (dist * sp) >> 14;
        int bestD = 120;
        for (int s = 8; s >= 1; s--) {
            int f = s * 1000 / 8;
            int cx = tx - ((Fx.sin(camYaw) * (hd * f / 1000)) >> 14), cz = tz - ((Fx.cos(camYaw) * (hd * f / 1000)) >> 14);
            int cyy = ty + (vd * f / 1000 >> 4);
            if (cyy > groundH(cx, cz, cyy) + 20) { bestD = f; break; }
        }
        int hdf = hd * bestD / 1000, vdf = vd * bestD / 1000;
        camXm = tx - ((Fx.sin(camYaw) * hdf) >> 14);
        camZm = tz - ((Fx.cos(camYaw) * hdf) >> 14);
        camYm = ty + (vdf >> 4);
        int g = groundH(camXm, camZm, camYm);
        if (camYm < g + 24) camYm = g + 24;
    }

    // ================================================================== rendering
    void loadTimecyc() throws IOException {
        DataInputStream d = new DataInputStream(Assets.open("/timecyc.bin"));
        tc = new byte[360];
        d.readFully(tc);
        d.close();
    }
    String clockStr() {
        int h = minutes / 60, m = minutes % 60;
        return (h < 10 ? "0" : "") + h + ":" + (m < 10 ? "0" : "") + m;
    }

    final int[] lv = new int[15];
    boolean night;

    void lighting() {
        int h = minutes / 60, n = (h + 1) % 24, f = (minutes % 60) * 256 / 60;
        for (int k = 0; k < 15; k++) {
            int a = tc[h * 15 + k] & 255, b = tc[n * 15 + k] & 255;
            lv[k] = a + (((b - a) * f) >> 8);
        }
        r.skyTop = (lv[0] << 16) | (lv[1] << 8) | lv[2];
        r.skyBot = (lv[3] << 16) | (lv[4] << 8) | lv[5];
        r.fogR = lv[3]; r.fogG = lv[4]; r.fogB = lv[5];
        r.groundCol = Sky.mix(r.skyBot, 0x5C5A50, 150);
        int ambObj = (lv[9] + lv[10] + lv[11]) / 3, dir = (lv[12] + lv[13] + lv[14]) / 3;
        int fb = 200 + brightness * 4;
        int a = (ambObj * 256 / 255 + 28) * fb >> 8, d = (dir * 140 / 255) * fb >> 8;
        r.amb = a; r.dif = d;
        r.emis = 256;
        night = minutes < 6 * 60 || minutes >= 20 * 60;
        Sky.sunDir(this);
        if (Sky.sunY > 1500) { r.lx = Sky.sunX; r.ly = Sky.sunY; r.lz = Sky.sunZ; }
        else { r.lx = 5000; r.ly = 12000; r.lz = -6000; }
        r.night = minutes < 6 * 60 + 30 || minutes >= 19 * 60 + 30;
        r.farZ = drawDist * 64;
        r.fogFar = r.farZ; r.fogNear = r.farZ * 35 / 100;
    }

    private final int[] rYaw = new int[9], mmA = new int[9], wA = new int[9], wB = new int[9], wC = new int[9], sc = new int[9];

    void render() {
        lighting();
        int sx = 0, sy = 0;
        if (shake > 0) { sx = (Fx.rnd(9) - 4) * 2; sy = (Fx.rnd(7) - 3); }
        r.animU = time / 22; r.animV = time / 31;
        r.setCamera((camXm >> 4) + sx, camYm + sy, camZm >> 4, camYaw, camPitch);
        r.clear();
        Sky.drawSky(this, r);
        int[] mv = r.mv;
        int nearLim = 150 * 64 + 3200, lim = r.farZ + 3300;
        int fx = Fx.sin(camYaw), fz = Fx.cos(camYaw);
        for (int k = 0; k < tiles.length; k++) {
            Tile t = tiles[k];
            if (t == null || !t.installed) continue;
            int dx = t.cx - r.camX, dz = t.cz - r.camZ;
            if (dx > lim || dx < -lim || dz > lim || dz < -lim) continue;
            if (((dx * fx + dz * fz) >> 14) < -4600) continue;
            int dy = -r.camY;
            int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14, ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14, tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
            boolean near = Math.max(Fx.abs(dx), Fx.abs(dz)) < nearLim;
            r.drawMesh(near ? t.near : t.far, mv, tx, ty, tz, r.lx, r.ly, r.lz, 0);
        }
        for (int i = 0; i < nped; i++) drawBlood(peds[i]);
        for (int i = 0; i < ncars; i++) drawCar(cars[i], false);
        for (int i = 0; i < nped; i++) {
            Ped p = peds[i];
            if (p.state == Ped.DEAD && p.deadT < 0) continue;
            if (Fx.abs(p.x - camXm) > 90000 || Fx.abs(p.z - camZm) > 90000) continue;
            drawPed(p);
        }
        drawPed(player);
        if (target != null && aimT > 0 && target.state != Ped.DEAD) Sky.target(r, target.x >> 4, target.y + 70, target.z >> 4, target.hp, target.cop ? 160 : 100, time);
        drawPickups();
        drawParticles();
        for (int i = 0; i < ncars; i++) drawCar(cars[i], true);
        if (night) drawBeam();
        Sky.drawPost(this, r);
    }

    void drawPickups() {
        for (int i = 0; i < npick; i++) {
            if (pkT[i] > 0 || pkLife[i] < 0) continue;
            Mesh m = pkType[i] == 3 ? cashMesh() : weaponMesh[pkType[i]];
            if (m == null) continue;
            int dx = (pkX[i] >> 4) - r.camX, dy = pkY[i] + 40 + ((Fx.sin(time / 3) * 6) >> 14) - r.camY, dz = (pkZ[i] >> 4) - r.camZ;
            if (dx > 6000 || dx < -6000 || dz > 6000 || dz < -6000) continue;
            Fx.rotY(rYaw, time / 4);
            Fx.mul(r.mv, rYaw, mmA);
            int[] mv = r.mv;
            int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14, ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14, tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
            r.drawMesh(m, mmA, tx, ty, tz, 0, 16384, 0, 0);
        }
    }

    /** a small green bundle of notes */
    Mesh cashMesh() {
        if (cashMesh != null) return cashMesh;
        Mesh m = new Mesh();
        int a = 12, b = 4, c = 7;
        m.nv = 8; m.x = new int[] { -a, a, a, -a, -a, a, a, -a }; m.y = new int[] { -b, -b, b, b, -b, -b, b, b }; m.z = new int[] { -c, -c, -c, -c, c, c, c, c };
        m.u = new int[8]; m.v = new int[8];
        int[] q = { 0, 1, 2, 0, 2, 3, 5, 4, 7, 5, 7, 6, 4, 0, 3, 4, 3, 7, 1, 5, 6, 1, 6, 2, 3, 2, 6, 3, 6, 7, 4, 5, 1, 4, 1, 0 };
        m.nt = 12; m.ti = q; m.tex = new short[12]; m.col = new int[12]; m.flag = new byte[12];
        m.nx = new byte[12]; m.ny = new byte[12]; m.nz = new byte[12];
        for (int i = 0; i < 12; i++) { m.tex[i] = -1; m.col[i] = i < 4 ? 0x3FA53F : 0x2E8B2E; m.flag[i] = (byte) (Mesh.F_DOUBLE | Mesh.F_EMISSIVE); }
        m.bounds();
        cashMesh = m;
        return m;
    }

    void drawBlood(Ped p) {
        if (p.state != Ped.DEAD || p.deadT <= 400 || p.seat != null) return;
        int dx = (p.bloodX >> 4) - r.camX, dz = (p.bloodZ >> 4) - r.camZ, dy = p.bloodY + 2 - r.camY;
        if (dx > 4000 || dx < -4000 || dz > 4000 || dz < -4000) return;
        int grow = p.deadT - 400; if (grow > 3000) grow = 3000;
        int s = 4000 + grow * 9 / 2;
        Fx.rotY(rYaw, p.yaw);
        for (int k = 0; k < 9; k++) sc[k] = 0;
        sc[0] = s; sc[4] = 16384; sc[8] = s;
        Fx.mul(rYaw, sc, wA);
        Fx.mul(r.mv, wA, mmA);
        int[] mv = r.mv;
        int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14, ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14, tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
        r.drawMesh(blood, mmA, tx, ty, tz, 0, 16384, 0, 0);
    }

    void drawBeam() {
        int dx = (lampX >> 4) - r.camX, dy = lampY - r.camY, dz = (lampZ >> 4) - r.camZ;
        if (lampY < -50 || dx > 48000 || dx < -48000 || dz > 48000 || dz < -48000) return;
        Fx.rotY(rYaw, (time / 8) & 1023);
        Fx.mul(r.mv, rYaw, mmA);
        int[] mv = r.mv;
        int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14, ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14, tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
        int fn = r.fogNear, ff = r.fogFar, fz0 = r.farZ;
        r.fogNear = 1 << 28; r.fogFar = (1 << 28) + 1; r.farZ = 60000;
        r.additive = true;
        r.drawMesh(beam, mmA, tx, ty, tz, 0, 16384, 0, 0);
        r.additive = false;
        r.farZ = fz0; r.fogNear = fn; r.fogFar = ff;
    }

    void drawCar(Car c, boolean glassPass) {
        int px = c.x >> 4, pz = c.z >> 4;
        int dx = px - r.camX, dz = pz - r.camZ, dy = c.y - r.camY;
        if (dx > r.farZ || dx < -r.farZ || dz > r.farZ || dz < -r.farZ) return;
        int dist = Math.max(Fx.abs(dx), Fx.abs(dz));
        boolean near = dist < 1500;
        int[] mv = r.mv;
        CarModel m = cm(c);
        int roll = m.bike ? -c.roll * 3 : c.roll, pitch = c.pitch - c.slopePitch;
        Fx.rotY(wA, c.yaw); Fx.rotX(wB, pitch); Fx.mul(wA, wB, wC); Fx.rotZ(wA, roll); Fx.mul(wC, wA, rYaw);
        Fx.mul(mv, rYaw, mmA);
        int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14, ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14, tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
        int lmx = (rYaw[0] * r.lx + rYaw[3] * r.ly + rYaw[6] * r.lz) >> 14, lmy = (rYaw[1] * r.lx + rYaw[4] * r.ly + rYaw[7] * r.lz) >> 14, lmz = (rYaw[2] * r.lx + rYaw[5] * r.ly + rYaw[8] * r.lz) >> 14;
        r.tint2 = c.color2;
        if (!near) {
            if (glassPass) { if (m.farGlass != null && dist < 2600) { r.blend = true; r.drawMesh(m.farGlass, mmA, tx, ty, tz, lmx, lmy, lmz, 0); r.blend = false; } }
            else r.drawMesh(m.far, mmA, tx, ty, tz, lmx, lmy, lmz, c.color);
            return;
        }
        for (int p = 0; p < m.np; p++) {
            int id = m.partId[p];
            if (id < 12 && (c.lostBits & (1 << id)) != 0) continue;
            Mesh[] v = id < 12 && (c.damBits & (1 << id)) != 0 ? m.dam[p] : m.ok[p];
            if (glassPass) { if (v[1] != null) { r.blend = true; r.drawMesh(v[1], mmA, tx, ty, tz, lmx, lmy, lmz, 0); r.blend = false; } }
            else r.drawMesh(v[0], mmA, tx, ty, tz, lmx, lmy, lmz, c.color);
        }
        if (glassPass || m.bike) return;
        int cs = Fx.cos(c.yaw), sn = Fx.sin(c.yaw);
        for (int k = 0; k < m.nw; k++) {
            int wxl = m.wx[k], wzl = m.wz[k];
            int wpx = px + ((cs * wxl + sn * wzl) >> 14), wpz = pz + ((-sn * wxl + cs * wzl) >> 14), wpy = c.y + m.wy[k];
            Fx.rotY(wA, c.yaw + (wzl > 0 ? c.steer : 0));
            Fx.rotX(wB, c.wheelRot);
            Fx.mul(wA, wB, wC);
            if (wxl < 0) { Fx.rotY(wA, 512); Fx.mul(wC, wA, wB); System.arraycopy(wB, 0, wC, 0, 9); }
            int ex = wpx - r.camX, ey = wpy - r.camY, ez = wpz - r.camZ;
            int wtx = (mv[0] * ex + mv[1] * ey + mv[2] * ez) >> 14, wty = (mv[3] * ex + mv[4] * ey + mv[5] * ez) >> 14, wtz = (mv[6] * ex + mv[7] * ey + mv[8] * ez) >> 14;
            Fx.mul(mv, wC, wA);
            int wlx = (wC[0] * r.lx + wC[3] * r.ly + wC[6] * r.lz) >> 14, wly = (wC[1] * r.lx + wC[4] * r.ly + wC[7] * r.lz) >> 14, wlz = (wC[2] * r.lx + wC[5] * r.ly + wC[8] * r.lz) >> 14;
            r.drawMesh(CarModel.wheel, wA, wtx, wty, wtz, wlx, wly, wlz, 0);
        }
    }

    static final int[] AIM = new int[9];
    private final int[] handW = new int[3];

    void drawPed(Ped p) {
        int clip, t, fx = p.x >> 4, fz = p.z >> 4, fy = p.y;
        if (p.seat != null) { clip = cSit; t = 0; fy = p.seat.y - 18; if (cm(p.seat).bike) fy = p.seat.y + 10; }
        else if (p.clip >= 0) { clip = p.clip; t = p.clipT; }
        else { clip = p.loco; t = p.locoT; }
        if (clip < 0) clip = cIdle;
        int dx = fx - r.camX, dz = fz - r.camZ;
        if (dx > r.farZ || dx < -r.farZ || dz > r.farZ || dz < -r.farZ) return;
        AnimData.sample(clip, t, pose);
        boolean aiming = p.seat == null && p.clip < 0 && ((p == player && weapon == 2 && aimT > 0) || (p != player && p.aimT > 0));
        if (aiming) {
            if (AIM[4] == 0) Fx.rotY(AIM, 768);
            System.arraycopy(AIM, 0, pose, 9, 9); System.arraycopy(AIM, 0, pose, 18, 9);
        }
        PedModel m = p == player ? pm0 : (Math.max(Fx.abs(dx), Fx.abs(dz)) < 1150 ? pool[p.kind] : poolLo[p.kind]);
        if (m == null) return;
        m.drawPose(r, fx, fy, fz, p.yaw, pose, p == player ? playerTex : p.tex);
        if (p == player && weapon > 0 && p.seat == null && weaponMesh[weapon] != null) drawHeld(m, weaponMesh[weapon]);
        if (aiming && (p == player ? flashT > 0 : p.aimT > 450)) { m.hand(handW); Sky.flash(r, handW[0], handW[1], handW[2]); }
    }

    void drawHeld(PedModel m, Mesh w) {
        m.hand(handW);
        int[] R = m.rp[2];
        int dx = handW[0] - r.camX, dy = handW[1] - r.camY, dz = handW[2] - r.camZ;
        int[] mv = r.mv;
        int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14, ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14, tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
        Fx.mul(mv, R, mmA);
        r.drawMesh(w, mmA, tx, ty, tz, (R[0] * r.lx + R[3] * r.ly + R[6] * r.lz) >> 14, (R[1] * r.lx + R[4] * r.ly + R[7] * r.lz) >> 14, (R[2] * r.lx + R[5] * r.ly + R[8] * r.lz) >> 14, 0);
    }
}
