package javax.microedition.media;
public interface Player extends Controllable {
    int UNREALIZED = 100, REALIZED = 200, PREFETCHED = 300, STARTED = 400, CLOSED = 0;
    void realize() throws MediaException;
    void prefetch() throws MediaException;
    void start() throws MediaException;
    void stop() throws MediaException;
    void deallocate();
    void close();
    int getState();
    void addPlayerListener(PlayerListener l);
    void removePlayerListener(PlayerListener l);
    void setLoopCount(int count);
    long setMediaTime(long now) throws MediaException;
    long getMediaTime();
}
