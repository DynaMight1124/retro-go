package javax.microedition.media.control;
public interface VolumeControl extends javax.microedition.media.Control {
    void setMute(boolean mute);
    boolean isMuted();
    int setLevel(int level);
    int getLevel();
}
