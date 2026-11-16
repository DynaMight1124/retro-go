package javax.microedition.lcdui;
import javax.microedition.midlet.MIDlet;
public class Display {
    static Display d = new Display(); Displayable cur;
    public static Display getDisplay(MIDlet m) { return d; }
    public void setCurrent(Displayable x) { cur = x; }
    public Displayable getCurrent() { return cur; }
}
