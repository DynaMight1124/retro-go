import java.io.*;
import javax.microedition.lcdui.*;

/** Resources + the renderer texture cache (512 slots, reference counted, loaded on demand). */
final class Assets {
    static int SLOTS = 1024;
    static String[] slotName = new String[1024];
    static int[] slotRef = new int[1024], slotUse = new int[1024];
    static boolean legacy = open("/legacy.txt") != null;
    static { if (legacy) SLOTS = 360; }
    static int useClock;
    static R3D r;

    static InputStream open(String res) { return new Assets().getClass().getResourceAsStream(res); }

    static int[] decode(String path, int[] dim) throws IOException {
        Image im = Image.createImage(path);
        int w = im.getWidth(), h = im.getHeight();
        int[] p = new int[w * h];
        im.getRGB(p, 0, w, 0, 0, w, h);
        for (int i = 0; i < p.length; i++) {
            int c = p[i];
            p[i] = ((c >>> 24) < 128) ? 0 : (c | 0xFF000000);
        }
        dim[0] = w; dim[1] = h;
        return p;
    }

    /** slot already holding this texture, or -1 */
    static synchronized int find(String path) {
        for (int i = 0; i < SLOTS; i++) if (path.equals(slotName[i])) return i;
        return -1;
    }

    /** returns a slot with the texture loaded (pixels may be pre-decoded by the loader thread), ref +1 */
    static synchronized int acquire(String path, int[] pix, int w, int h) {
        int s = find(path);
        if (s < 0) {
            int[] d = new int[2];
            if (pix == null) {
                try { pix = decode(path, d); w = d[0]; h = d[1]; } catch (Exception e) { return -1; }
            }
            s = freeSlot();
            if (s < 0) return -1;
            slotName[s] = path;
            r.setTexture(s, pix, w, h);
            r.uvs[s] = 0;
        }
        slotRef[s]++;
        slotUse[s] = ++useClock;
        return s;
    }
    static int acquire(String path) { return acquire(path, null, 0, 0); }

    static synchronized void release(int s) {
        if (s >= 0 && s < SLOTS && slotRef[s] > 0) slotRef[s]--;
    }

    private static int freeSlot() {
        int best = -1, bu = Integer.MAX_VALUE;
        for (int i = 0; i < SLOTS; i++) {
            if (slotName[i] == null) return i;
            if (slotRef[i] == 0 && slotUse[i] < bu) { bu = slotUse[i]; best = i; }
        }
        if (best >= 0) { r.T[best] = null; slotName[best] = null; }
        return best;
    }

    static int[] imgPix(String res, int[] dm) throws IOException {
        Image im = Image.createImage(res);
        int w = im.getWidth(), h = im.getHeight();
        int[] p = new int[w * h];
        im.getRGB(p, 0, w, 0, 0, w, h);
        dm[0] = w; dm[1] = h;
        return p;
    }

    static int[] intensity(String res, int[] dm) throws IOException {
        int[] p = imgPix(res, dm);
        for (int i = 0; i < p.length; i++) p[i] = (p[i] >> 16) & 255;
        return p;
    }

    static byte[] readAll(String res) throws IOException {
        InputStream in = open(res);
        if (in == null) throw new IOException("missing " + res);
        ByteArrayOutputStream bo = new ByteArrayOutputStream();
        byte[] b = new byte[4096];
        int n;
        while ((n = in.read(b)) > 0) bo.write(b, 0, n);
        in.close();
        return bo.toByteArray();
    }

    static String[] readLines(String res) {
        try {
            byte[] b = readAll(res);
            String s = new String(b);
            String[] tmp = new String[400];
            int n = 0, p = 0;
            while (p < s.length() && n < tmp.length) {
                int q = s.indexOf('\n', p);
                if (q < 0) q = s.length();
                String line = s.substring(p, q).trim();
                if (line.length() > 0) tmp[n++] = line;
                p = q + 1;
            }
            String[] out = new String[n];
            System.arraycopy(tmp, 0, out, 0, n);
            return out;
        } catch (Exception e) { return new String[0]; }
    }
}
