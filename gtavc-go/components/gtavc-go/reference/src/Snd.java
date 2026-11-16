import java.io.ByteArrayInputStream;
import javax.microedition.media.*;
import javax.microedition.media.control.VolumeControl;

/** A handful of original Vice City sound effects (from sfx.raw) played through MMAPI. Fails silently where unsupported. */
final class Snd implements Runnable {
    static final int PUNCH = 0, GUN = 1, BAT = 2, SWING = 3, CRASH = 4, DOOR_OPEN = 5, DOOR_CLOSE = 6, HORN = 7, PICKUP = 8,
            EXPLODE = 9, SPLASH = 10, HITPED = 11, SIREN = 12, IDLE = 13, REV = 14, N = 15;
    static final String[] FILE = { "punch", "gun", "bat", "swing", "crash", "dooropen", "doorclose", "horn", "pickup",
            "explode", "splash", "hitped", "siren", "idle", "rev" };
    static byte[][] data = new byte[N][];
    static Player[] pl = new Player[N];
    static boolean on = true, ok;
    static int engineState = -1;
    static boolean sirenOn;
    // ped / Tommy voices (one at a time, like VC's ped comments)
    static final int V_PANIC = 0, V_DEATH = 1, V_FIGHT = 2, V_CHAT = 3, V_TOMMY = 4;
    static final String[][] VOICE = { { "v_mpanic0", "v_mpanic1", "v_mpanic2" }, { "v_mdeath0", "v_mdeath1" }, { "v_fight0", "v_fight1" },
            { "v_chat0", "v_chat1", "v_chat2" }, { "v_tommy0", "v_tommy1" } };
    static final String[][] VOICE_F = { { "v_fpanic0", "v_fpanic1", "v_fpanic2" }, { "v_fdeath0", "v_fdeath1" }, { "v_fight0", "v_fight1" },
            { "v_chat0", "v_chat1", "v_chat2" }, { "v_tommy0", "v_tommy1" } };
    static Player voicePl;
    static String pendingVoice;
    static int wantEngine = -1;
    static long wantSince;
    private static final int[] queue = new int[16];
    private static int qn;
    private static final int[] qvol = new int[16];
    private static Thread worker;

    static void init() {
        try {
            for (int i = 0; i < N; i++) {
                try { data[i] = Assets.readAll("/s_" + FILE[i] + ".wav"); } catch (Exception e) { data[i] = null; }
            }
            // MMAPI on the desktop harness / without audio support: stay silent
            Class.forName("javax.microedition.media.Manager");
            ok = true;
            worker = new Thread(new Snd());
            worker.start();
        } catch (Throwable t) { ok = false; }
    }

    private static Player get(int id) {
        if (pl[id] != null) return pl[id];
        if (data[id] == null) return null;
        try {
            Player p = Manager.createPlayer(new ByteArrayInputStream(data[id]), "audio/x-wav");
            p.realize(); p.prefetch();
            if (id == SIREN || id == IDLE || id == REV) p.setLoopCount(-1);
            pl[id] = p;
            return p;
        } catch (Throwable t) { data[id] = null; return null; }
    }

    static void play(int id) { play(id, 100); }
    static void play3d(int id, int x, int z, int lx, int lz) {
        int d = Fx.dist(x - lx, z - lz) / 1024;
        if (d > 70) return;
        play(id, 100 - d * 100 / 75);
    }
    static void play(int id, int vol) {
        if (!ok || !on) return;
        synchronized (queue) { if (qn < queue.length) { queue[qn] = id; qvol[qn] = vol; qn++; queue.notify(); } }
    }

    /** sound work happens off the game thread (MMAPI calls can block for a few ms on S60) */
    public void run() {
        while (true) {
            int id = -1, vol = 100;
            synchronized (queue) {
                if (qn == 0) { try { queue.wait(); } catch (InterruptedException e) { } }
                if (qn > 0) { id = queue[0]; vol = qvol[0]; qn--; System.arraycopy(queue, 1, queue, 0, qn); System.arraycopy(qvol, 1, qvol, 0, qn); }
            }
            if (id < 0) continue;
            if (id == 300) { playVoice(); continue; }
            try {
                if (id >= 100) { Player p = get(id - 100); if (p != null) p.stop(); continue; }
                Player p = get(id);
                if (p == null) continue;
                VolumeControl vc = (VolumeControl) p.getControl("VolumeControl");
                if (vc != null) vc.setLevel(vol);
                if (id != SIREN && id != IDLE && id != REV) { p.stop(); p.setMediaTime(0); }
                p.start();
            } catch (Throwable t) { }
        }
    }

    private static void playVoice() {
        String n;
        synchronized (queue) { n = pendingVoice; pendingVoice = null; }
        if (n == null) return;
        try {
            if (voicePl != null) {
                if (voicePl.getState() == Player.STARTED) return;            // still talking
                voicePl.close(); voicePl = null;
            }
            byte[] d = Assets.readAll("/" + n + ".wav");
            voicePl = Manager.createPlayer(new ByteArrayInputStream(d), "audio/x-wav");
            voicePl.realize();
            voicePl.start();
        } catch (Throwable t) { voicePl = null; }
    }

    private static void stop(int id) {
        if (!ok) return;
        synchronized (queue) { if (qn < queue.length) { queue[qn] = 100 + id; qvol[qn] = 0; qn++; queue.notify(); } }
    }

    static void voice(int kind, boolean female) {
        if (!ok || !on) return;
        String[] set = (female ? VOICE_F : VOICE)[kind];
        synchronized (queue) { pendingVoice = set[Fx.rnd(set.length)]; if (qn < queue.length) { queue[qn] = 300; qvol[qn] = 90; qn++; queue.notify(); } }
    }

    /** engine loop: idle when slow, rev on the throttle; a change has to hold for 0.7 s so the loops don't restart constantly */
    static void engine(boolean running, int pct) {
        int s = !running ? 0 : (pct < 22 ? 1 : (pct > 30 ? 2 : (engineState > 0 ? engineState : 1)));
        long now = System.currentTimeMillis();
        if (s != wantEngine) { wantEngine = s; wantSince = now; }
        if (s == engineState) return;
        if (s != 0 && engineState > 0 && now - wantSince < 700) return;
        if (engineState == 1) stop(IDLE);
        if (engineState == 2) stop(REV);
        engineState = s;
        if (s == 1) play(IDLE, 60); else if (s == 2) play(REV, 70);
    }
    static void siren(boolean onNow) {
        if (onNow == sirenOn) return;
        sirenOn = onNow;
        if (onNow) play(SIREN, 70); else stop(SIREN);
    }
    static void stopAll() { engine(false, 0); siren(false); }
}
