import java.io.*;

/** Baked IFP clips: per frame 10 world rotations (9 shorts, 2.14) + 3 root offset shorts. 20 frames per second. */
final class AnimData {
    static int n;
    static String[] name;
    static int[] dur, nf, travel;
    static boolean[] loop;
    static short[][] fr;

    static void load() throws IOException {
        DataInputStream d = new DataInputStream(Assets.open("/anim.bin"));
        n = d.readUnsignedShort();
        name = new String[n]; dur = new int[n]; nf = new int[n]; travel = new int[n]; loop = new boolean[n]; fr = new short[n][];
        for (int i = 0; i < n; i++) {
            int l = d.readUnsignedByte();
            byte[] b = new byte[l];
            d.readFully(b);
            name[i] = new String(b);
            dur[i] = d.readUnsignedShort(); nf[i] = d.readUnsignedShort(); loop[i] = d.readUnsignedByte() != 0; travel[i] = d.readShort();
            short[] f = new short[nf[i] * 93];
            for (int k = 0; k < f.length; k++) f[k] = d.readShort();
            fr[i] = f;
        }
        d.close();
    }

    static int find(String s) {
        for (int i = 0; i < n; i++) if (name[i].equals(s)) return i;
        return 0;
    }

    /** clip length in ms as played (loop cycle or one-shot) */
    static int len(int c) { return (nf[c] - 1) * 50; }

    /** forward travel speed of a locomotion clip in mm/s */
    static int speed(int c) { return travel[c] == 0 ? 1 : travel[c] * 16 * 1000 / len(c); }

    /** out[0..89] matrices, out[90..92] = right, up, forward offset (1/64 m) */
    static void sample(int c, int tMs, int[] out) {
        short[] f = fr[c];
        int last = nf[c] - 1;
        int pos = tMs * 20;                       // frame * 1000
        int i = pos / 1000, w = ((pos % 1000) << 8) / 1000;
        int j;
        if (loop[c]) { i %= last; j = i + 1 >= last ? 0 : i + 1; }
        else { if (i >= last) { i = last; j = last; w = 0; } else j = i + 1; }
        int a = i * 93, b = j * 93;
        for (int k = 0; k < 93; k++) out[k] = f[a + k] + (((f[b + k] - f[a + k]) * w) >> 8);
    }
}
