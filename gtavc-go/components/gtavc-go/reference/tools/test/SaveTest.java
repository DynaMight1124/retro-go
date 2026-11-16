/** Headless check: drops (cash / cop pistol), save, back to menu, continue. */
public class SaveTest {
    public static void main(String[] a) throws Exception {
        Game g = new Game(null);
        int guard = 0; while (g.state != Game.S_MENU && guard++ < 9000) g.frame(33);
        g.press(Game.A_FIRE); g.frame(33); g.down = 0;
        World w = g.world;
        for (int i = 0; i < 10; i++) g.frame(33);
        Ped cop = w.spawnCopAt(w.player.x + 3000, w.player.z);
        w.damagePed(cop, 999, true, w.player.x, w.player.z);
        int drops = 0, cash = 0;
        for (int i = 0; i < w.nped; i++) { Ped p = w.peds[i]; if (!p.cop && p.seat == null && p.state != Ped.DEAD) w.damagePed(p, 999, true, w.player.x, w.player.z); }
        for (int i = 0; i < w.npick; i++) if (w.pkLife[i] > 0) { drops++; if (w.pkType[i] == 3) cash++; }
        System.out.println("dropped pickups " + drops + " (cash " + cash + ", cop pistol " + (drops - cash) + ")");
        int m0 = w.money;
        for (int i = 0; i < w.npick; i++) if (w.pkLife[i] > 0) { w.player.x = w.pkX[i]; w.player.z = w.pkZ[i]; g.frame(33); g.frame(33); }
        System.out.println("money " + m0 + " -> " + w.money + ", pistol " + w.hasPistol + " ammo " + w.ammo);
        w.health = 100; w.wanted = 0; w.busted = false;
        int sx = w.player.x, sz = w.player.z, money = w.money;
        Save.write(w);
        g.state = Game.S_MENU; g.saveChecked = false; g.sel = 0; g.frame(33);
        System.out.println("menu has continue: " + g.hasSave);
        g.sel = 0; g.press(Game.A_FIRE); g.frame(33); g.down = 0;
        System.out.println("continued at dist " + Fx.dist(w.player.x - sx, w.player.z - sz) + " mm, money " + w.money + " (saved " + money + "), state " + g.state);
        w.alive = false;
        System.exit(0);
    }
}
