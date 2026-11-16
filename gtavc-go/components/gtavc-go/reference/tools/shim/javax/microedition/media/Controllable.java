package javax.microedition.media;
public interface Controllable {
    Control[] getControls();
    Control getControl(String controlType);
}
