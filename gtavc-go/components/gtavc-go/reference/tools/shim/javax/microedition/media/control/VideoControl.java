package javax.microedition.media.control;
import javax.microedition.media.MediaException;
public interface VideoControl extends GUIControl {
    int USE_DIRECT_VIDEO = 1;
    Object initDisplayMode(int mode, Object arg);
    void setDisplayLocation(int x, int y);
    void setDisplaySize(int width, int height) throws MediaException;
    void setDisplayFullScreen(boolean fullScreenMode) throws MediaException;
    void setVisible(boolean visible);
    int getSourceWidth();
    int getSourceHeight();
}
