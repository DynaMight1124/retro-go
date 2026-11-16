import java.io.InputStream;
import javax.microedition.lcdui.Canvas;
import javax.microedition.media.*;
import javax.microedition.media.control.VideoControl;

/** The Vice City opening movie (res/intro.3gp, MPEG-4 SP + AAC) played full width with MMAPI. Any key skips it. */
final class Intro implements PlayerListener {
    Player p;
    volatile boolean done;

    void start(Canvas c, int W, int H) {
        try {
            InputStream in = Assets.open("/intro.3gp");
            if (in == null) { done = true; return; }
            p = Manager.createPlayer(in, "video/3gpp");
            p.addPlayerListener(this);
            p.realize();
            VideoControl vc = (VideoControl) p.getControl("VideoControl");
            if (vc != null) {
                vc.initDisplayMode(VideoControl.USE_DIRECT_VIDEO, c);
                int vw = W, vh = W * 9 / 16;
                if (vh > H) { vh = H; vw = H * 16 / 9; }
                try { vc.setDisplaySize(vw, vh); } catch (Exception e) { }
                vc.setDisplayLocation((W - vw) / 2, (H - vh) / 2);
                vc.setVisible(true);
            }
            p.prefetch();
            p.start();
        } catch (Throwable t) {
            stop();
        }
    }

    public void playerUpdate(Player pl, String ev, Object data) {
        if (PlayerListener.END_OF_MEDIA.equals(ev) || PlayerListener.ERROR.equals(ev) || PlayerListener.CLOSED.equals(ev)) done = true;
    }

    void stop() {
        Player q = p;
        p = null;
        done = true;
        if (q == null) return;
        try { q.stop(); } catch (Throwable t) { }
        try { q.close(); } catch (Throwable t) { }
    }
}
