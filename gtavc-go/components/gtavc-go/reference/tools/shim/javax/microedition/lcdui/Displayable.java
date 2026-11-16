package javax.microedition.lcdui;
public abstract class Displayable {
    protected int w = 320, h = 240;
    public int getWidth() { return w; }
    public int getHeight() { return h; }
    public void shimSetSize(int nw, int nh) { w = nw; h = nh; }
}
