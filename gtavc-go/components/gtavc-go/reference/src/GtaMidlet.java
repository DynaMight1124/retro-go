import javax.microedition.lcdui.*;
import javax.microedition.midlet.*;

/** GTA Vice City - J2ME open world demake (Ocean Beach demo) */
public class GtaMidlet extends MIDlet {
    Game game;

    protected void startApp() {
        if (game == null) {
            game = new Game(this);
            Display.getDisplay(this).setCurrent(game);
            game.start();
        } else {
            Display.getDisplay(this).setCurrent(game);
        }
    }
    protected void pauseApp() { }
    protected void destroyApp(boolean unconditional) {
        if (game != null) game.stop();
    }
}
