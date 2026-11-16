package javax.microedition.media;
import java.io.InputStream;
import java.io.IOException;
public final class Manager {
    public static Player createPlayer(InputStream stream, String type) throws IOException, MediaException { throw new MediaException("no media on desktop"); }
    public static Player createPlayer(String locator) throws IOException, MediaException { throw new MediaException("no media on desktop"); }
}
