import java.awt.image.BufferedImage;
import java.io.*;

/** Headless: start fade, run speed, jump, lock-on, debug spawn, PS2 filter, neon at night on Ocean Drive, slope walk. */
public class Test2 {
    static Game g; static World w;
    static void shot(String n) throws Exception {
        new File("build/shots").mkdirs();
        BufferedImage bi = new BufferedImage(g.W, g.H, BufferedImage.TYPE_INT_RGB);
        bi.setRGB(0, 0, g.W, g.H, g.scr, 0, g.W);
        javax.imageio.ImageIO.write(bi, "png", new File("build/shots/" + n + ".png"));
    }
    static void frames(int n) { for (int i = 0; i < n; i++) { g.frame(33); try { Thread.sleep(2); } catch (Exception e) { } } }
    public static void main(String[] a) throws Exception {
        g = new Game(null);
        int guard = 0; while (g.state != Game.S_MENU && guard++ < 9000) g.frame(33);
        frames(30);
        g.sel = 0; g.press(Game.A_FIRE); g.frame(33); g.down = 0;
        frames(8); shot("20_startfade"); System.out.println("state during fade " + g.state);
        frames(20); shot("21_fadein");
        w = g.world;
        // run speed
        w.runMode = true; int x0 = w.player.x, z0 = w.player.z;
        g.keyPressed(-1); frames(30); g.keyReleased(-1);
        System.out.println("run 1s: " + Fx.dist(w.player.x - x0, w.player.z - z0) + " mm (speed " + w.player.speed + ")");
        // jump
        int y0 = w.player.y; g.keyPressed(48); g.frame(33); g.keyReleased(48); int maxY = y0;
        for (int i = 0; i < 40; i++) { g.frame(33); if (w.player.y > maxY) maxY = w.player.y; }
        System.out.println("jump height " + (maxY - y0) * 100 / 64 + " cm, landed " + !w.player.air);
        // lock-on
        w.hasPistol = true; w.ammo = 50; w.weapon = 2;
        Ped v = null; int bd = 1 << 30;
        for (int i = 0; i < w.nped; i++) { Ped p = w.peds[i]; if (p.seat == null && p.state != Ped.DEAD) { int d = Fx.dist(p.x - w.player.x, p.z - w.player.z); if (d < bd) { bd = d; v = p; } } }
        if (v != null) { w.player.yaw = Fx.yawOf(v.x - w.player.x, v.z - w.player.z); w.camYaw = w.player.yaw; }
        g.keyPressed(53); frames(4); shot("22_lockon"); System.out.println("target " + (w.target != null) + " dist " + bd); frames(10); g.keyReleased(53);
        // debug menu: 0 + *
        g.keyPressed(48); g.keyPressed(42); g.frame(33); g.keyReleased(42); g.keyReleased(48);
        System.out.println("debug open " + (g.state == Game.S_DEBUG));
        g.dbgCar = World.find(w.carNames, "infernus"); g.dbgSel = 0; frames(1); shot("23_debug");
        g.press(Game.A_FIRE); g.frame(33); g.down = 0; frames(5);
        System.out.println("spawned: " + w.help + " state " + g.state);
        shot("24_spawned");
        // PS2 filter + night neon at the Ocean View
        g.oTrails = true; w.minutes = 23 * 60;
        w.inCar = null; w.player.seat = null; w.pAct = 0;
        w.player.x = 250 * 1024; w.player.z = -1350 * 1024; w.preload(w.player.x, w.player.z); w.player.y = w.groundH(w.player.x, w.player.z, 400);
        w.player.yaw = 768; w.camYaw = 768; w.camPitch = 30;
        frames(20); shot("25_oceandrive_ps2");
        g.oTrails = false; frames(3); shot("26_oceandrive_plain");
        long t0 = System.currentTimeMillis(); frames(30); System.out.println("ms/frame " + (System.currentTimeMillis() - t0) / 30.0 + " tris " + g.r.tris);
        w.alive = false; System.exit(0);
    }
}
