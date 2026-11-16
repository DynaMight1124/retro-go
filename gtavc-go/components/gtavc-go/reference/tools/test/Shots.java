import java.awt.image.BufferedImage;
import java.io.*;

/** Headless session: boots the canvas, plays a scripted run and writes screenshots to build/shots. */
public class Shots {
    static Game g; static World w;
    static void shot(String n) throws Exception {
        new File("build/shots").mkdirs();
        BufferedImage bi = new BufferedImage(g.W, g.H, BufferedImage.TYPE_INT_RGB);
        bi.setRGB(0, 0, g.W, g.H, g.scr, 0, g.W);
        javax.imageio.ImageIO.write(bi, "png", new File("build/shots/" + n + ".png"));
    }
    static void frames(int n) { for (int i = 0; i < n; i++) { g.frame(33); try { Thread.sleep(2); } catch (Exception e) { } } }
    static void key(int code, int n) { g.keyPressed(code); frames(n); g.keyReleased(code); }
    public static void main(String[] a) throws Exception {
        g = new Game(null);
        frames(5); shot("00_splash");
        int guard = 0; while (g.state != Game.S_MENU && guard++ < 9000) g.frame(33);
        System.out.println("menu after " + guard + " frames");
        frames(10); shot("01_menu");
        g.press(Game.A_FIRE); g.frame(33); g.down = 0;
        w = g.world;
        frames(40); shot("02_spawn"); g.press(Game.A_PAUSE); g.frame(33); g.down = 0; g.sel = 2; g.press(Game.A_FIRE); g.frame(33); g.down = 0; frames(2); shot("02_map"); g.press(Game.A_FIRE); g.frame(33); g.down=0; g.sel = 0; g.press(Game.A_FIRE); g.frame(33); g.down = 0;
        System.out.println("spawn ground " + w.groundH(w.player.x, w.player.z, w.player.y) + " y " + w.player.y + " surf " + w.surfAt(w.player.x, w.player.z) + " peds " + w.nped + " cars " + w.ncars);
        for (int k = 1; k < 4; k++) { w.camYawOff = k * 256; frames(25); shot("03_look" + k); }
        w.camYawOff = 0;
        // walk to the pickups
        for (int i = 0; i < w.npick; i++) { w.player.x = w.pkX[i]; w.player.z = w.pkZ[i]; frames(3); }
        System.out.println("weapons: bat " + w.hasBat + " pistol " + w.hasPistol + " ammo " + w.ammo + " current " + w.weapon);
        w.weapon = 2; g.keyPressed(53); frames(8); shot("04_aim"); frames(20); g.keyReleased(53);
        System.out.println("after shooting: ammo " + w.ammo + " wanted " + w.wanted);
        w.weapon = 1; frames(5); shot("04_bat");
        Ped v = null;
        for (int i = 0; i < w.nped; i++) if (!w.peds[i].cop && w.peds[i].seat == null && w.peds[i].state != Ped.DEAD) { v = w.peds[i]; break; }
        if (v != null) { w.damagePed(v, 500, true, w.player.x, w.player.z); w.player.x = v.x + 2600; w.player.z = v.z; w.player.y = w.groundH(w.player.x, w.player.z, v.y); w.player.yaw = Fx.yawOf(v.x - w.player.x, v.z - w.player.z); w.camYaw = w.player.yaw; frames(70); shot("04_dead"); }
        int[] times = { 9 * 60, 18 * 60 + 40, 23 * 60 };
        for (int i = 0; i < times.length; i++) { w.minutes = times[i]; frames(3); shot("05_time_" + times[i] / 60); }
        w.minutes = 13 * 60;
        // drive: take the nearest car
        Car c = null; int bd = 1 << 30;
        for (int i = 0; i < w.ncars; i++) { if (w.cm(w.cars[i]).bike) continue; int d = Fx.dist(w.cars[i].x - w.player.x, w.cars[i].z - w.player.z); if (d < bd) { bd = d; c = w.cars[i]; } }
        if (c != null) { c.mode = Car.PARKED; c.vx = c.vz = 0; c.speed = 0; }
        if (c != null) {
            w.player.x = c.x - 2500; w.player.z = c.z; w.player.y = c.y; c.vx = c.vz = 0; c.speed = 0;
            key(42, 1); frames(150);
            System.out.println("in car: " + (w.inCar != null) + " model " + w.cm(c).name);
            if (w.findSpot(5, 40, World.S_ROAD)) { c.x = w.spot[0]; c.z = w.spot[1]; c.yaw = w.roadYaw(c.x, c.z); c.y = w.groundH(c.x, c.z); w.player.x = c.x; w.player.z = c.z; }
            g.keyPressed(-1);
            for (int t = 0; t < 8; t++) { frames(30); System.out.println("t=" + (t + 1) + "s speed " + c.speed * 36 / 10240 + " km/h hp " + c.hp); }
            shot("06_drive");
            { int fx = Fx.sin(c.yaw), fz = Fx.cos(c.yaw); CarModel m = w.cm(c); int half = m.dimZ * 16 / 2 - 900;
              int px = c.x + ((fx * half) >> 14), pz = c.z + ((fz * half) >> 14);
              System.out.println("FRONT m " + px / 1024.0 + " " + pz / 1024.0 + " y " + c.y / 64.0);
              for (int dz = -2; dz <= 2; dz++) { StringBuffer sb = new StringBuffer("  "); for (int dx = -2; dx <= 2; dx++) sb.append(w.solidCell(w.scell(px + dx * 1024, w.mox), w.scell(pz + dz * 1024, w.moz)) ? "#" : "."); System.out.println(sb); } }
            for (int q = 0; q < 6; q++) { int ox = c.x, oz = c.z; w.dbgHit = 0; g.frame(33); System.out.println("hit " + w.dbgHit + " inCar " + (w.inCar == c) + " pAct " + w.pAct + " speed " + c.speed + " v " + c.vx + "," + c.vz + " moved " + Fx.dist(c.x - ox, c.z - oz) + " down " + g.down + " ncars " + w.ncars); }
            { CarModel m = w.cm(c); int half = m.dimZ * 16 / 2 - 900, rad = m.dimX * 16 / 2 + 60; int gH = Math.max(c.y, w.groundH(c.x, c.z, c.y));
              System.out.println("dims " + m.dimX + "," + m.dimY + "," + m.dimZ + " half " + half + " rad " + rad + " gHere " + gH);
              for (int k = -1; k <= 1; k++) { int ox2 = (Fx.sin(c.yaw) * k * half) >> 14, oz2 = (Fx.cos(c.yaw) * k * half) >> 14; w.rx = c.x + ox2; w.rz = c.z + oz2; boolean rs = w.resolve(rad);
                System.out.println("  k " + k + " resolve " + rs + " g " + w.groundH(c.x + ox2, c.z + oz2, gH) + " g2 " + w.groundH(c.x + ox2 * 12 / 10, c.z + oz2 * 12 / 10, gH)); } }
            for (int i = 0; i < w.ncars; i++) if (w.cars[i] != c) System.out.println("  other car dist " + Fx.dist(w.cars[i].x - c.x, w.cars[i].z - c.z));
            { int fx = Fx.sin(c.yaw), fz = Fx.cos(c.yaw); System.out.println("car y " + c.y + " ground " + w.groundH(c.x, c.z, c.y) + " surf " + w.surfAt(c.x, c.z));
              for (int d = 500; d <= 5000; d += 500) { int x = c.x + ((fx * d) >> 14), z = c.z + ((fz * d) >> 14); System.out.println("  ahead " + d + "mm ground " + w.groundH(x, z, c.y) + " solid " + w.solidCell(w.scell(x, w.mox), w.scell(z, w.moz)) + " surf " + w.surfAt(x, z)); }
              for (int sd = -2000; sd <= 2000; sd += 500) { int x = c.x + ((fz * sd) >> 14), z = c.z - ((fx * sd) >> 14); System.out.println("  side " + sd + " solid " + w.solidCell(w.scell(x, w.mox), w.scell(z, w.moz)) + " g " + w.groundH(x, z, c.y)); } }

            System.out.println("speed " + c.speed / 1024 + " m/s, vx " + c.vx + " vz " + c.vz + " hp " + c.hp);
            g.keyPressed(-3); frames(40); shot("07_turn"); g.keyReleased(-3);
            g.keyPressed(48); g.keyPressed(-4); frames(30); shot("08_handbrake"); g.keyReleased(-4); g.keyReleased(48); g.keyReleased(-1);
            System.out.println("slide " + c.slide + " yawVel " + c.yawVel);
            g.keyPressed(42); frames(2); g.keyReleased(42); frames(60);
            w.camYawOff = 300; w.damageCar(c, 0, 2000); frames(50); shot("09_fire");
            System.out.println("burning " + c.burning); frames(120); shot("10_explode");
            System.out.println("wrecked " + c.wrecked + " health " + w.health);
        }
        long t0 = System.currentTimeMillis(); frames(60); System.out.println("desktop ms/frame " + (System.currentTimeMillis() - t0) / 60.0 + " tris " + g.r.tris);
        int loaded = 0; for (int k = 0; k < w.tiles.length; k++) if (w.tiles[k] != null) loaded++;
        System.out.println("tiles loaded " + loaded);
        w.alive = false;
        System.exit(0);
    }
}
