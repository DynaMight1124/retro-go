import javax.microedition.lcdui.*;

/** Canvas + main loop + state machine (intro, loading, menus, game). */
public final class Game extends Canvas implements Runnable {
    static final int S_LOAD = 0, S_MENU = 1, S_OPT = 2, S_PLAY = 3, S_PAUSE = 4, S_CTRL = 5, S_MAP = 6, S_STATS = 7, S_SPLASH = 8, S_VIDEO = 9, S_START = 10, S_DEBUG = 11;
    // logical key actions
    static final int A_UP = 1, A_DOWN = 2, A_LEFT = 4, A_RIGHT = 8, A_FIRE = 16, A_ENTER = 32, A_CAML = 64, A_CAMR = 128,
            A_CAMU = 256, A_CAMD = 512, A_SPRINT = 1024, A_HAND = 2048, A_PAUSE = 4096, A_WEAP = 8192;
    int startMs, startKind, playFade = 9999, dbgSel, dbgCar, gameSpeed = 100;
    boolean legacy = Assets.legacy;

    final GtaMidlet midlet;
    int W, H, u = 1, rs = 1;
    int[] scr, trail;
    R3D r = new R3D();
    World world;
    int state = S_SPLASH, prevState;
    int splashMs, fadeMs = 9999;
    Intro intro;
    volatile boolean running, paused, needResize = true;
    Thread thread;

    // input
    volatile int down, edges;
    // loading
    int loadI, loadMs, loadErr;
    String loadMsg;
    int total;
    boolean uiReady;
    // menu
    int sel, menuSel;
    int optSel;
    // options
    int oBright = 10, oDraw = 8, oRadar = 0, oRes = 0;
    boolean oLimiter = true, oTrails = false, oSubs = true, oWide = false, oLegend = false, oHud = true;
    int fps, fpsCount, fpsMs;
    int blink;
    int titleMs;
    boolean started;

    public Game(GtaMidlet m) {
        midlet = m;
        if (legacy) { oRes = 1; oDraw = 4; }
        setFullScreenMode(true);
        Fx.init();
        total = Ui.UI_STEPS + 7;
        W = getWidth(); H = getHeight();
    }

    // ------------------------------------------------------------ lifecycle
    void start() {
        if (thread != null) return;
        running = true;
        thread = new Thread(this);
        thread.start();
    }
    void stop() { if (started && world != null) Save.write(world); running = false; if (world != null) world.alive = false; Snd.stopAll(); }

    protected void showNotify() { paused = false; }
    protected void hideNotify() {
        paused = true;
        if (state == S_PLAY) { prevState = S_PLAY; state = S_PAUSE; sel = 0; Snd.stopAll(); Save.write(world); }
    }
    protected void sizeChanged(int w, int h) { W = w; H = h; needResize = true; }

    void resize() {
        needResize = false;
        if (W < 64 || H < 48) return;
        u = Math.min(W / 320, H / 240);
        if (u < 1) u = 1;
        rs = oRes == 0 ? 1 : 2;
        int rw = W / rs, rh = H / rs;
        int focal = oWide ? rw * 4 / 8 : rw * 5 / 8;
        r.resize(rw, rh, focal);
        if (rs == 1) scr = r.fb; else scr = new int[W * H];
        trail = null;
    }

    // ------------------------------------------------------------ input
    int mapKey(int k) {
        // number keys first: on Nokia getGameAction(KEY_NUM2/4/6/8) answers UP/LEFT/RIGHT/DOWN, which made 2-4-6-8 walk Tommy
        switch (k) {
            case KEY_NUM2: return A_CAMU;
            case KEY_NUM8: return A_CAMD;
            case KEY_NUM4: return A_CAML;
            case KEY_NUM6: return A_CAMR;
            case KEY_NUM5: case 'f': case 'F': return A_FIRE;
            case KEY_STAR: case 'e': case 'E': return A_ENTER;
            case KEY_NUM1: case 'q': case 'Q': return A_SPRINT;   // tap: run on/off, hold: sprint
            case KEY_NUM0: case ' ': case 'b': case 'B': return A_HAND;
            case KEY_POUND: case 'p': case 'P': case 27: return A_PAUSE;
            case KEY_NUM3: case 'r': case 'R': return A_WEAP;
            case KEY_NUM7: return A_HAND;
            case KEY_NUM9: return A_ENTER;
            case 'w': case 'W': return A_UP;
            case 's': case 'S': return A_DOWN;
            case 'a': case 'A': return A_LEFT;
            case 'd': case 'D': return A_RIGHT;
            case 'i': case 'I': return A_CAMU;
            case 'k': case 'K': return A_CAMD;
            case 'j': case 'J': return A_CAML;
            case 'l': case 'L': return A_CAMR;
            case -6: case -7: case -21: case -22: return A_PAUSE;    // soft keys
            default: break;
        }
        int ga = 0;
        try { ga = getGameAction(k); } catch (Exception e) { }
        if (ga == UP) return A_UP;
        if (ga == DOWN) return A_DOWN;
        if (ga == LEFT) return A_LEFT;
        if (ga == RIGHT) return A_RIGHT;
        if (ga == FIRE) return A_FIRE;
        return 0;
    }
    int anyKey;
    protected void keyPressed(int k) { anyKey++; int a = mapKey(k); if (a == A_SPRINT) sprintPress = (int) System.currentTimeMillis(); press(a); }
    protected void keyRepeated(int k) { int a = mapKey(k); edges |= a & (A_UP | A_DOWN | A_LEFT | A_RIGHT); }
    protected void keyReleased(int k) {
        int a = mapKey(k);
        if (a == A_SPRINT && sprintPress > 0 && (int) System.currentTimeMillis() - sprintPress < 350) edges |= A_TAPRUN;
        if (a == A_SPRINT) sprintPress = 0;
        down &= ~a;
    }
    static final int A_TAPRUN = 16384;
    int sprintPress;
    void press(int a) { down |= a; edges |= a; }

    // ------------------------------------------------------------ main loop
    public void run() {
        int last = (int) System.currentTimeMillis();
        while (running) {
            int now = (int) System.currentTimeMillis();
            int dt = now - last;
            last = now;
            if (dt < 0) dt = 1;
            if (paused) { try { Thread.sleep(100); } catch (InterruptedException e) { } continue; }
            frame(dt);
            repaint();
            serviceRepaints();
            int spent = (int) System.currentTimeMillis() - now;
            int wait = (state == S_PLAY && !oLimiter) ? 1 : 33 - spent;
            if (wait < 2) wait = 2;
            try { Thread.sleep(wait); } catch (InterruptedException e) { }
        }
        midlet.notifyDestroyed();
    }

    protected void paint(Graphics g) {
        if (state == S_VIDEO) { g.setColor(0); g.fillRect(0, 0, getWidth(), getHeight()); return; }
        if (scr != null && W == (rs == 1 ? r.W : r.W * rs)) g.drawRGB(scr, 0, W, 0, 0, W, H, false);
    }

    /** one tick of logic + draw into scr (also used by the desktop test harness) */
    void frame(int dt) {
        if (needResize) resize();
        if (r.fb == null) return;
        int e = edges; edges = 0;
        blink += dt;
        fpsMs += dt; fpsCount++;
        if (fpsMs >= 1000) { fps = fpsCount; fpsCount = 0; fpsMs = 0; }
        switch (state) {
            case S_SPLASH: doSplash(dt); break;
            case S_VIDEO: doVideo(); break;
            case S_LOAD: doLoad(dt, e); break;
            case S_MENU: doMenu(e); break;
            case S_START: doStart(dt); break;
            case S_DEBUG: doDebug(e); break;
            case S_OPT: doOptions(e); break;
            case S_CTRL: case S_MAP: case S_STATS: if ((e & (A_FIRE | A_PAUSE | A_ENTER)) != 0) state = prevState == S_PLAY || prevState == S_PAUSE ? S_PAUSE : S_MENU; draw2(); break;
            case S_PAUSE: doPause(e); break;
            case S_PLAY: doPlay(dt, e); break;
            default: break;
        }
        if (state == S_MENU && fadeMs < 900) {          // fade the menu in after the movie instead of a hard cut
            fadeMs += dt;
            int f = fadeMs < 150 ? 0 : (fadeMs - 150) * 256 / 750;
            if (f < 256) for (int q = 0; q < scr.length; q++) { int c = scr[q]; scr[q] = ((((c & 0xFF00FF) * f) >>> 8) & 0xFF00FF) | ((((c & 0xFF00) * f) >>> 8) & 0xFF00); }
        }
        if (rs > 1 && state == S_PLAY) upscale();
        if (oTrails && state == S_PLAY) ps2Filter();
        if (state == S_PLAY) overlay();
        if (state == S_PLAY && legacyTouch()) drawTouch();
        if (state == S_PLAY && playFade < 800) { playFade += dt; dim(playFade < 100 ? 0 : (playFade - 100) * 256 / 700); }
    }

    // ------------------------------------------------------------ splash / intro movie
    static final String CREDIT = "A Port made by AidenCantCode";
    int splashKeys = -1;

    void doSplash(int dt) {
        if (splashMs == 0) { try { Ui.loadFont(); } catch (Exception ex) { } splashKeys = anyKey; }
        splashMs += dt;
        for (int i = 0; i < scr.length; i++) scr[i] = 0;
        int a = splashMs < 600 ? splashMs * 255 / 600 : (splashMs > 2600 ? (3200 - splashMs) * 255 / 600 : 255);
        if (a < 0) a = 0;
        if (Ui.faMain != null) {
            int c = (a << 16) | (a << 8) | a;
            Ui.textC(scr, W, H, W / 2, H / 2 - 8 * u, CREDIT, c, u);
        }
        if (splashMs > 3200 || (anyKey != splashKeys && splashMs > 300)) { state = S_LOAD; loadMs = 0; edges = 0; }
    }

    int videoKeys;
    void doVideo() {
        for (int i = 0; i < scr.length; i++) scr[i] = 0;
        if (intro == null) {
            videoKeys = anyKey;
            intro = new Intro();
            intro.start(this, W, H);
        }
        if (intro.done || anyKey != videoKeys) {
            intro.stop();
            intro = null;
            state = S_MENU; sel = 0; edges = 0; down = 0; fadeMs = 0;
            System.gc();
        }
    }

    // ------------------------------------------------------------ loading
    void doLoad(int dt, int e) {
        loadMs += dt;
        if (loadI < total && loadErr == 0) {
            int t0 = (int) System.currentTimeMillis();
            int budget = loadMs < 30 ? 1 : 3;
            for (int k = 0; k < budget && loadI < total; k++) {
                try { loadOne(loadI); } catch (Throwable ex) { loadErr = 1; loadMsg = String.valueOf(ex); break; }
                loadI++;
                if ((int) System.currentTimeMillis() - t0 > 60) break;
            }
        }
        for (int i = 0; i < scr.length; i++) scr[i] = 0;
        boolean intro = loadMs < 2600;
        if (e != 0 && Ui.scr[1] != null && loadMs > 400) intro = false;
        if (intro && Ui.scr[0] != null) {
            int f = 256;
            if (loadMs < 500) f = loadMs * 256 / 500; else if (loadMs > 2100) f = (2600 - loadMs) * 256 / 500;
            Ui.stretch(Ui.scr[0], 320, 240, scr, W, H, f < 0 ? 0 : f);
        } else {
            int idx = 1 + ((loadMs / 2300) % 4);
            if (Ui.scr[idx] == null) idx = 1;
            if (Ui.scr[idx] != null) Ui.stretch(Ui.scr[idx], 320, 240, scr, W, H, 256);
            // loading bar
            int bw = W * 6 / 10, bx = (W - bw) / 2, by = H - 14 * u;
            Ui.fill(scr, W, H, bx - u, by - u, bw + 2 * u, 5 * u, 0x101018);
            Ui.fill(scr, W, H, bx, by, bw * loadI / total, 3 * u, 0xFF6EC7);
            if (loadErr != 0) Ui.text(scr, W, H, 6 * u, 6 * u, "error " + loadMsg, 0xFF4040, u);
            if (loadI >= total && loadMs > 3200) { state = S_VIDEO; sel = 0; edges = 0; for (int q = 0; q < Ui.scr.length; q++) Ui.scr[q] = null; System.gc(); }
        }
        present();
    }

    void present() { if (rs > 1) { /* UI states render at full size directly */ } }

    void loadOne(int i) throws java.io.IOException {
        if (i < Ui.UI_STEPS) { Ui.loadStep(i); if (i == Ui.UI_STEPS - 1) uiReady = true; return; }
        i -= Ui.UI_STEPS;
        if (i == 0) { Assets.r = r; world = new World(r); world.loadIndex(); }
        else if (i == 1) { AnimData.load(); world.loadTimecyc(); Sky.load(); Snd.init(); }
        else if (i == 2) world.loadPools();
        else if (i == 3) world.preload(world.spX, world.spZ);
        else if (i == 4) { world.reset(); applyOptions(); world.startLoader(); }
        else System.gc();
    }

    // ------------------------------------------------------------ menus
    static final String[] MAIN = { "start game", "options", "controls", "quit game" };
    static final String[] MAIN_C = { "continue", "start new game", "options", "controls", "quit game" };
    boolean hasSave, saveChecked;
    static final String[] PAUSE = { "resume", "start new game", "map", "stats", "brief", "options", "quit game" };

    void menuNav(int e, int n) {
        if ((e & A_UP) != 0) sel = (sel + n - 1) % n;
        if ((e & A_DOWN) != 0) sel = (sel + 1) % n;
    }

    void drawMenu(String title, String[] items) {
        scrClearFull();
        Ui.menuBg(scr, W, H, u);
        Ui.menuTitle(scr, W, H, u, title);
        int n = items.length;
        int y0 = H * 36 / 100 - (n > 5 ? 0 : 0);
        int step = 15 * u;
        for (int i = 0; i < n; i++) Ui.menuItem(scr, W, H, u, W / 2, y0 + i * step, items[i], i == sel);
    }

    void scrClearFull() { if (rs > 1 || scr == null || scr.length != W * H) { } }

    void doMenu(int e) {
        if (!saveChecked) { hasSave = Save.exists(); saveChecked = true; }
        String[] items = hasSave ? MAIN_C : MAIN;
        menuNav(e, items.length);
        if ((e & A_FIRE) != 0) {
            int s = hasSave ? sel - 1 : sel;
            if (hasSave && sel == 0) { state = S_START; startMs = 0; startKind = 1; }
            else if (s == 0) { state = S_START; startMs = 0; startKind = 0; }
            else if (s == 1) { prevState = S_MENU; state = S_OPT; optSel = 0; }
            else if (s == 2) { prevState = S_MENU; state = S_CTRL; }
            else { running = false; }
        }
        drawMenu("grand theft auto", items);
        Ui.textC(scr, W, H, W / 2, H - 14 * u, "a port made by aidencantcode", 0xB0B0C0, 1);
    }

    /** fade the menu to black, set the world up, then fade the game in */
    void doStart(int dt) {
        startMs += dt;
        drawMenu("grand theft auto", hasSave ? MAIN_C : MAIN);
        int f = 256 - startMs * 256 / 500;
        if (f < 0) f = 0;
        dim(f);
        if (startMs >= 550) {
            world.reset();
            if (startKind == 1) Save.load(world);
            applyOptions(); state = S_PLAY; down = 0; edges = 0; titleMs = 0; started = true; playFade = 0;
        }
    }
    void dim(int f) {
        if (f >= 256) return;
        for (int q = 0; q < scr.length; q++) { int c = scr[q]; scr[q] = ((((c & 0xFF00FF) * f) >>> 8) & 0xFF00FF) | ((((c & 0xFF00) * f) >>> 8) & 0xFF00); }
    }

    // ------------------------------------------------------------ debug / trainer menu (0 + *)
    static final String[] DBG = { "spawn car", "time of day", "tommy speed", "game speed", "god mode", "clear wanted level", "give weapons + $5000", "teleport to spawn", "back" };
    void doDebug(int e) {
        World w = world;
        int n = DBG.length;
        if ((e & A_UP) != 0) dbgSel = (dbgSel + n - 1) % n;
        if ((e & A_DOWN) != 0) dbgSel = (dbgSel + 1) % n;
        int d = ((e & A_RIGHT) != 0 ? 1 : 0) - ((e & A_LEFT) != 0 ? 1 : 0);
        boolean ok = (e & A_FIRE) != 0;
        switch (dbgSel) {
            case 0: dbgCar = (dbgCar + d + w.carNames.length) % w.carNames.length; if (ok) { w.debugSpawn(dbgCar); state = S_PLAY; } break;
            case 1: if (d != 0) w.minutes = (w.minutes + d * 30 + 1440) % 1440; break;
            case 2: if (d != 0) w.speedMul = Fx.clamp(w.speedMul + d * 25, 50, 300); break;
            case 3: if (d != 0) gameSpeed = Fx.clamp(gameSpeed + d * 25, 25, 300); break;
            case 4: if (ok || d != 0) w.god = !w.god; break;
            case 5: if (ok) { w.wanted = 0; w.clearCops(); } break;
            case 6: if (ok) { w.hasBat = true; w.hasPistol = true; w.ammo += 200; w.money += 5000; } break;
            case 7: if (ok) { w.inCar = null; w.player.seat = null; w.pAct = 0; w.placePlayerAtSpawn(); w.preload(w.player.x, w.player.z); state = S_PLAY; } break;
            default: if (ok) state = S_PLAY; break;
        }
        if ((e & A_PAUSE) != 0) state = S_PLAY;
        if (state == S_PLAY) { down = 0; return; }
        w.render();
        if (rs > 1) upscale();
        Ui.fillA(scr, W, H, 0, 0, W, H, 0x000000, 150);
        Ui.textC(scr, W, H, W / 2, 6 * u, "debug menu", Ui.PINK, u);
        int y0 = 24 * u, st = 13 * u;
        for (int i = 0; i < n; i++) {
            String v = "";
            switch (i) {
                case 0: v = "< " + w.carNames[dbgCar] + " >"; break;
                case 1: v = "< " + w.clockStr() + " >"; break;
                case 2: v = "< x" + w.speedMul / 100 + "." + (w.speedMul % 100) / 10 + " >"; break;
                case 3: v = "< x" + gameSpeed / 100 + "." + (gameSpeed % 100) / 10 + " >"; break;
                case 4: v = w.god ? "on" : "off"; break;
                default: break;
            }
            int y = y0 + i * st;
            if (i == dbgSel) Ui.fill(scr, W, H, 6 * u, y - u, W - 12 * u, 12 * u, Ui.HILITE);
            Ui.text(scr, W, H, 10 * u, y, DBG[i], Ui.WHITE, u);
            if (v.length() > 0) Ui.textR(scr, W, H, W - 10 * u, y, v, Ui.BLUE, u);
        }
    }

    void doPause(int e) {
        menuNav(e, PAUSE.length);
        if ((e & A_PAUSE) != 0) { state = S_PLAY; down = 0; return; }
        if ((e & A_FIRE) != 0) {
            switch (sel) {
                case 0: state = S_PLAY; down = 0; break;
                case 1: world.reset(); applyOptions(); state = S_PLAY; down = 0; break;
                case 2: prevState = S_PAUSE; state = S_MAP; break;
                case 3: prevState = S_PAUSE; state = S_STATS; break;
                case 4: prevState = S_PAUSE; state = S_CTRL; break;
                case 5: prevState = S_PAUSE; state = S_OPT; optSel = 0; break;
                default: Save.write(world); saveChecked = false; state = S_MENU; sel = 0; break;
            }
        }
        if (state == S_PAUSE) drawMenu("pause menu", PAUSE);
    }

    static final String[] OPT_NAMES = { "brightness", "draw distance", "frame limiter", "ps2 filter", "subtitles", "wide screen",
            "map legend", "radar mode", "hud mode", "screen resolution", "restore defaults", "back" };

    void doOptions(int e) {
        int n = OPT_NAMES.length;
        if ((e & A_UP) != 0) optSel = (optSel + n - 1) % n;
        if ((e & A_DOWN) != 0) optSel = (optSel + 1) % n;
        int dlt = ((e & A_RIGHT) != 0 ? 1 : 0) - ((e & A_LEFT) != 0 ? 1 : 0);
        if ((e & A_FIRE) != 0 && optSel < 10) dlt = 1;
        if (dlt != 0) {
            switch (optSel) {
                case 0: oBright = Fx.clamp(oBright + dlt, 0, 15); break;
                case 1: oDraw = Fx.clamp(oDraw + dlt, 0, 15); break;
                case 2: oLimiter = !oLimiter; break;
                case 3: oTrails = !oTrails; break;
                case 4: oSubs = !oSubs; break;
                case 5: oWide = !oWide; needResize = true; break;
                case 6: oLegend = !oLegend; break;
                case 7: oRadar = (oRadar + 3 + dlt) % 3; break;
                case 8: oHud = !oHud; break;
                case 9: oRes = 1 - oRes; needResize = true; break;
                default: break;
            }
        }
        if ((e & A_FIRE) != 0 && optSel == 10) { oBright = 10; oDraw = 8; oLimiter = true; oTrails = false; oSubs = true; oWide = false; oLegend = false; oRadar = 0; oHud = true; oRes = 0; needResize = true; }
        if ((e & A_FIRE) != 0 && optSel == 11 || (e & A_PAUSE) != 0) { state = prevState == S_PAUSE ? S_PAUSE : S_MENU; applyOptions(); }
        applyOptions();
        scrClearFull();
        Ui.menuBg(scr, W, H, u);
        Ui.menuTitle(scr, W, H, u, "display");
        int x0 = W * 15 / 100, xr = W * 87 / 100, y0 = H * 20 / 100, step = 11 * u;
        for (int i = 0; i < n; i++) {
            int y = y0 + i * step;
            if (i == optSel) Ui.fill(scr, W, H, x0 - 4 * u, y - u, xr - x0 + 8 * u, 11 * u, Ui.HILITE);
            if (i >= 10) { Ui.textC(scr, W, H, W / 2, y, OPT_NAMES[i], Ui.PINK, u); continue; }
            Ui.text(scr, W, H, x0, y, OPT_NAMES[i], Ui.PINK, u);
            switch (i) {
                case 0: Ui.bars(scr, W, H, u, xr, y, oBright, 15); break;
                case 1: Ui.bars(scr, W, H, u, xr, y, oDraw, 15); break;
                case 2: Ui.textR(scr, W, H, xr, y, oLimiter ? "on" : "off", Ui.PINK, u); break;
                case 3: Ui.textR(scr, W, H, xr, y, oTrails ? "on" : "off", Ui.PINK, u); break;
                case 4: Ui.textR(scr, W, H, xr, y, oSubs ? "on" : "off", Ui.PINK, u); break;
                case 5: Ui.textR(scr, W, H, xr, y, oWide ? "on" : "off", Ui.PINK, u); break;
                case 6: Ui.textR(scr, W, H, xr, y, oLegend ? "on" : "off", Ui.PINK, u); break;
                case 7: Ui.textR(scr, W, H, xr, y, oRadar == 0 ? "map & blips" : (oRadar == 1 ? "blips only" : "off"), Ui.PINK, u); break;
                case 8: Ui.textR(scr, W, H, xr, y, oHud ? "on" : "off", Ui.PINK, u); break;
                default: Ui.textR(scr, W, H, xr, y, W / (oRes == 0 ? 1 : 2) + "x" + H / (oRes == 0 ? 1 : 2) + "x32", Ui.PINK, u); break;
            }
        }
        Ui.textR(scr, W, H, W - 6 * u, H - 11 * u, "left, right to adjust", 0xE0E0E0, 1);
    }

    void applyOptions() {
        if (world == null) return;
        world.brightness = oBright * 14 / 15;
        world.drawDist = 60 + oDraw * 12;
    }

    void draw2() {
        scrClearFull();
        Ui.menuBg(scr, W, H, u);
        int x = W * 14 / 100, y = H * 24 / 100, st = 12 * u;
        if (state == S_CTRL) {
            Ui.menuTitle(scr, W, H, u, "controls");
            String[] t = { "d-pad: walk / turn / steer", "5: punch, hold to shoot (lock-on)", "*: enter / exit / steal car", "2 8: look up / down   4 6: look around",
                    "1: tap run / walk, hold sprint", "0: jump  (in car: handbrake)", "3: switch weapon   0 + *: debug menu", "soft key or #: pause menu" };
            for (int i = 0; i < t.length; i++) Ui.text(scr, W, H, x, y + i * st, t[i], Ui.PINK, u);
        } else if (state == S_STATS) {
            Ui.menuTitle(scr, W, H, u, "stats");
            World w = world;
            String[] t = { "money: $" + w.money, "time: " + w.clockStr() + "  day " + (w.dayCount + 1), "people hurt: " + w.pedsHit,
                    "cars stolen: " + w.carsStolen, "wanted level: " + w.wanted, "health: " + w.health };
            for (int i = 0; i < t.length; i++) Ui.text(scr, W, H, x, y + i * st, t[i], Ui.PINK, u);
        } else {
            Ui.menuTitle(scr, W, H, u, "map");
            World w = world;
            if (Ui.radImg == null) return;
            int mh = H * 74 / 100, mw = mh * Ui.radW / Ui.radH;
            if (mw > W - 20 * u) { mw = W - 20 * u; mh = mw * Ui.radH / Ui.radW; }
            int ox = W / 2 - mw / 2, oy = H * 20 / 100;
            for (int j = 0; j < mh; j++) {
                int yy = oy + j;
                if (yy >= H) break;
                int sj = j * Ui.radH / mh;
                for (int i = 0; i < mw; i++) scr[yy * W + ox + i] = Ui.radImg[sj * Ui.radW + i * Ui.radW / mw] & 0xFFFFFF;
            }
            int pxm = (w.player.x - Ui.radX0) / Ui.radCell * mw / Ui.radW;
            int pym = (Ui.radZ0 - w.player.z) / Ui.radCell * mh / Ui.radH;
            Ui.fill(scr, W, H, ox + pxm - 3 * u, oy + pym - 3 * u, 6 * u, 6 * u, 0x000000);
            Ui.fill(scr, W, H, ox + pxm - 2 * u, oy + pym - 2 * u, 4 * u, 4 * u, (blink / 300) % 2 == 0 ? 0xFF40D0 : 0xFFFFFF);
        }
    }

    // ------------------------------------------------------------ gameplay
    void doPlay(int dt, int e) {
        if ((down & A_HAND) != 0 && (e & A_ENTER) != 0) { state = S_DEBUG; dbgSel = 0; edges = 0; return; }   // 0 + *
        dt = dt * gameSpeed / 100;
        if ((e & A_PAUSE) != 0) { state = S_PAUSE; sel = 0; prevState = S_PLAY; Save.write(world); Snd.stopAll(); return; }
        int k = 0, d = down;
        if ((d & A_UP) != 0) k |= World.K_UP;
        if ((d & A_DOWN) != 0) k |= World.K_DOWN;
        if ((d & A_LEFT) != 0) k |= World.K_LEFT;
        if ((d & A_RIGHT) != 0) k |= World.K_RIGHT;
        if ((d & A_FIRE) != 0) k |= World.K_FIRE;
        if ((d & A_CAML) != 0) k |= World.K_CAML;
        if ((d & A_CAMR) != 0) k |= World.K_CAMR;
        if ((d & A_CAMU) != 0) k |= World.K_CAMU;
        if ((d & A_CAMD) != 0) k |= World.K_CAMD;
        if ((d & A_SPRINT) != 0) k |= World.K_SPRINT;
        if ((d & A_HAND) != 0) k |= World.K_HAND;
        titleMs += dt;
        world.update(dt, k, (e & A_FIRE) != 0, (e & A_ENTER) != 0, (e & A_TAPRUN) != 0, (e & A_WEAP) != 0, (e & A_HAND) != 0 && world.inCar == null);
        // contextual help like the original
        World w = world;
        if (w.inCar == null && w.helpT <= 0 && w.helpShown == 0) {
            for (int i = 0; i < w.ncars; i++) {
                if (Fx.dist(w.cars[i].x - w.player.x, w.cars[i].z - w.player.z) < 3800) { w.say("Press * to enter or exit a vehicle."); w.helpShown = 1; break; }
            }
        }
        w.render();
    }

    void upscale() {
        int rw = r.W, rh = r.H, s = rs;
        int[] src = r.fb;
        for (int y = 0; y < H; y++) {
            int sy = y / s; if (sy >= rh) sy = rh - 1;
            int so = sy * rw, d0 = y * W;
            for (int x = 0; x < W; x++) { int sx = x / s; if (sx >= rw) sx = rw - 1; scr[d0 + x] = src[so + sx]; }
        }
    }

    /** "PS2" look: motion trails, warm colour grade and a soft bloom from the bright parts of the frame */
    int[] blm, blm2;
    void ps2Filter() {
        int bw = W >> 2, bh = H >> 2;
        if (blm == null || blm.length != bw * bh) { blm = new int[bw * bh]; blm2 = new int[bw * bh]; }
        // bright pass at 1/4 resolution
        for (int y = 0; y < bh; y++) for (int x = 0; x < bw; x++) {
            int c = scr[(y << 2) * W + (x << 2)];
            int r0 = (c >> 16) & 255, g0 = (c >> 8) & 255, b0 = c & 255;
            int l = (r0 * 77 + g0 * 150 + b0 * 29) >> 8;
            int k = l > 150 ? (l - 150) * 2 : 0;
            blm[y * bw + x] = ((r0 * k >> 8) << 16) | ((g0 * k >> 8) << 8) | (b0 * k >> 8);
        }
        // 2 box blur passes (3x3, separable-ish)
        for (int pass = 0; pass < 2; pass++) {
            for (int y = 0; y < bh; y++) for (int x = 0; x < bw; x++) {
                int rr = 0, gg = 0, bb = 0, n = 0;
                for (int j = -1; j <= 1; j++) { int yy = y + j; if (yy < 0 || yy >= bh) continue;
                    for (int i = -1; i <= 1; i++) { int xx = x + i; if (xx < 0 || xx >= bw) continue; int c = blm[yy * bw + xx]; rr += (c >> 16) & 255; gg += (c >> 8) & 255; bb += c & 255; n++; } }
                blm2[y * bw + x] = ((rr / n) << 16) | ((gg / n) << 8) | (bb / n);
            }
            int[] t = blm; blm = blm2; blm2 = t;
        }
        boolean trails = trail != null && trail.length == scr.length;
        if (!trails) trail = new int[scr.length];
        for (int y = 0; y < H; y++) {
            int by = (y >> 2) < bh ? (y >> 2) : bh - 1, o = y * W;
            for (int x = 0; x < W; x++) {
                int c = scr[o + x], b = blm[by * bw + ((x >> 2) < bw ? (x >> 2) : bw - 1)];
                int r0 = ((c >> 16) & 255) * 270 >> 8, g0 = ((c >> 8) & 255) * 252 >> 8, b0 = (c & 255) * 222 >> 8;   // warm
                r0 += ((b >> 16) & 255); g0 += ((b >> 8) & 255); b0 += (b & 255);                                // bloom
                if (trails) { int t = trail[o + x]; r0 = (r0 * 5 + ((t >> 16) & 255) * 3) >> 3; g0 = (g0 * 5 + ((t >> 8) & 255) * 3) >> 3; b0 = (b0 * 5 + (t & 255) * 3) >> 3; }
                if (r0 > 255) r0 = 255; if (g0 > 255) g0 = 255; if (b0 > 255) b0 = 255;
                int out = (r0 << 16) | (g0 << 8) | b0;
                scr[o + x] = out; trail[o + x] = out;
            }
        }
    }

    // ------------------------------------------------------------ touch buttons (legacy build on touch phones)
    static final int[] TB_ACT = { A_UP, A_DOWN, A_LEFT, A_RIGHT, A_FIRE, A_ENTER, A_HAND, A_SPRINT, A_WEAP, A_PAUSE, A_CAML, A_CAMR };
    static final String[] TB_LBL = { "^", "v", "<", ">", "fire", "car", "jump", "run", "wpn", "||", "(", ")" };
    int[] tbX = new int[12], tbY = new int[12];
    int tbS, touchAct;
    boolean legacyTouch() { return legacy && hasPointerEvents(); }
    void layoutTouch() {
        int s = Math.max(28, Math.min(W, H) / 7); tbS = s;
        int bx = s / 2 + 4, by = H - s * 2 - 6;
        tbX[0] = bx + s; tbY[0] = by; tbX[1] = bx + s; tbY[1] = by + s * 2; tbX[2] = bx; tbY[2] = by + s; tbX[3] = bx + s * 2; tbY[3] = by + s;
        int rx2 = W - s - 6;
        tbX[4] = rx2; tbY[4] = H - s - 6; tbX[5] = rx2; tbY[5] = H - s * 2 - 12; tbX[6] = rx2 - s - 6; tbY[6] = H - s - 6;
        tbX[7] = rx2 - s - 6; tbY[7] = H - s * 2 - 12; tbX[8] = rx2; tbY[8] = H - s * 3 - 18; tbX[9] = W - s / 2 - 4; tbY[9] = s / 2 + 4;
        tbX[10] = W / 2 - s; tbY[10] = H - s / 2 - 4; tbX[11] = W / 2 + s; tbY[11] = H - s / 2 - 4;
    }
    void drawTouch() {
        if (tbS == 0) layoutTouch();
        for (int i = 0; i < 12; i++) {
            int s = i >= 10 ? tbS * 2 / 3 : tbS;
            Ui.fillA(scr, W, H, tbX[i] - s / 2, tbY[i] - s / 2, s, s, (down & TB_ACT[i]) != 0 ? 0xFF90E0 : 0xFFFFFF, 60);
            Ui.textC(scr, W, H, tbX[i], tbY[i] - 6, TB_LBL[i], 0xFFFFFF, 1);
        }
    }
    int touchAt(int x, int y) {
        if (tbS == 0) layoutTouch();
        for (int i = 0; i < 12; i++) { int s = i >= 10 ? tbS * 2 / 3 : tbS; if (Math.abs(x - tbX[i]) <= s / 2 + 4 && Math.abs(y - tbY[i]) <= s / 2 + 4) return TB_ACT[i]; }
        return 0;
    }
    protected void pointerPressed(int x, int y) {
        anyKey++;
        if (state != S_PLAY) { press(A_FIRE); touchAct = A_FIRE; return; }
        touchAct = touchAt(x, y);
        if (touchAct == A_SPRINT) sprintPress = (int) System.currentTimeMillis();
        press(touchAct);
    }
    protected void pointerDragged(int x, int y) {
        int a = state == S_PLAY ? touchAt(x, y) : touchAct;
        if (a != touchAct) { down &= ~touchAct; touchAct = a; press(a); }
    }
    protected void pointerReleased(int x, int y) {
        if (touchAct == A_SPRINT && sprintPress > 0 && (int) System.currentTimeMillis() - sprintPress < 350) edges |= A_TAPRUN;
        down &= ~touchAct; touchAct = 0;
    }

    void overlay() {
        World w = world;
        if (oHud) Ui.hudDraw(scr, W, H, w, u, true, oRadar);
        if (w.helpT > 0 && oSubs && w.help != null) Ui.helpBox(scr, W, H, u, w.help);
        if (titleMs < 4200) {
            int a = titleMs < 600 ? titleMs * 255 / 600 : (titleMs > 3400 ? (4200 - titleMs) * 255 / 800 : 255);
            String z = legacy ? "Ocean Beach" : "Washington Beach";
            int tw = Ui.textW(z, u * 2);
            for (int i = 0; i < z.length(); i++) { }
            if (a > 120) Ui.text(scr, W, H, W - 8 * u - tw, H - 28 * u, z, 0x28D890, u * 2);
        }
        if (w.busted) {
            Ui.fillA(scr, W, H, 0, 0, W, H, 0x000000, Math.min(150, w.bustedT / 14));
            String s = "busted";
            int sc = u * 3;
            Ui.text(scr, W, H, W / 2 - Ui.textW(s, sc) / 2, H / 2 - 8 * sc, s, Ui.PINK, sc);
        }
        if (w.health <= 0) {
            Ui.fillA(scr, W, H, 0, 0, W, H, 0x000000, Math.min(150, w.wastedT / 14));
            String s = "wasted";
            int sc = u * 3;
            Ui.text(scr, W, H, W / 2 - Ui.textW(s, sc) / 2, H / 2 - 8 * sc, s, Ui.PINK, sc);
        }
        if (oLegend) Ui.text(scr, W, H, 4 * u, H - 11 * u, "fps " + fps, 0xFFFF80, 1);
    }
}
