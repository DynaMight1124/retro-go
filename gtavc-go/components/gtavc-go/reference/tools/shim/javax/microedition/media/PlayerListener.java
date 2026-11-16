package javax.microedition.media;
public interface PlayerListener {
    String STARTED = "started", STOPPED = "stopped", END_OF_MEDIA = "endOfMedia", DURATION_UPDATED = "durationUpdated", DEVICE_UNAVAILABLE = "deviceUnavailable", DEVICE_AVAILABLE = "deviceAvailable", VOLUME_CHANGED = "volumeChanged", ERROR = "error", CLOSED = "closed";
    void playerUpdate(Player player, String event, Object eventData);
}
