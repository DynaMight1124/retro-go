package javax.microedition.lcdui;
import java.awt.image.BufferedImage;
public class Graphics {
    public static final int HCENTER = 1, VCENTER = 2, LEFT = 4, RIGHT = 8, TOP = 16, BOTTOM = 32, BASELINE = 64;
    BufferedImage img; java.awt.Graphics2D g2; int color;
    public Graphics(BufferedImage i) { img = i; g2 = i.createGraphics(); }
    public void setColor(int rgb) { color = rgb & 0xFFFFFF; g2.setColor(new java.awt.Color(color)); }
    public void setColor(int r, int g, int b) { setColor((r << 16) | (g << 8) | b); }
    public void fillRect(int x, int y, int w, int h) { g2.fillRect(x, y, w, h); }
    public void drawRect(int x, int y, int w, int h) { g2.drawRect(x, y, w, h); }
    public void drawLine(int a, int b, int c, int d) { g2.drawLine(a, b, c, d); }
    public void drawString(String s, int x, int y, int anchor) { g2.drawString(s, x, y + 10); }
    public void setClip(int x, int y, int w, int h) { g2.setClip(x, y, w, h); }
    public void drawImage(Image im, int x, int y, int anchor) { g2.drawImage(im.img, x, y, null); }
    public void drawRGB(int[] rgb, int off, int scan, int x, int y, int w, int h, boolean alpha) {
        for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
            int p = rgb[off + j * scan + i];
            img.setRGB(x + i, y + j, alpha ? p : (p | 0xFF000000));
        }
    }
    public int getClipWidth() { return img.getWidth(); }
    public int getClipHeight() { return img.getHeight(); }
}
