package javax.microedition.lcdui;
import java.awt.image.BufferedImage;
import java.io.*;
public class Image {
    BufferedImage img;
    Image(BufferedImage b) { img = b; }
    public static Image createImage(InputStream in) throws IOException {
        BufferedImage b = javax.imageio.ImageIO.read(in);
        if (b == null) throw new IOException("bad image");
        BufferedImage c = new BufferedImage(b.getWidth(), b.getHeight(), BufferedImage.TYPE_INT_ARGB);
        c.getGraphics().drawImage(b, 0, 0, null);
        return new Image(c);
    }
    public static Image createImage(String name) throws IOException {
        InputStream in = Image.class.getResourceAsStream(name);
        if (in == null) throw new IOException("missing " + name);
        return createImage(in);
    }
    public static Image createImage(byte[] d, int o, int l) throws IOException { return createImage(new ByteArrayInputStream(d, o, l)); }
    public static Image createImage(int w, int h) { return new Image(new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB)); }
    public static Image createRGBImage(int[] rgb, int w, int h, boolean a) {
        BufferedImage b = new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB);
        b.setRGB(0, 0, w, h, rgb, 0, w); return new Image(b);
    }
    public int getWidth() { return img.getWidth(); }
    public int getHeight() { return img.getHeight(); }
    public void getRGB(int[] out, int off, int scan, int x, int y, int w, int h) { img.getRGB(x, y, w, h, out, off, scan); }
    public Graphics getGraphics() { return new Graphics(img); }
}
