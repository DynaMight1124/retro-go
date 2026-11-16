import java.io.*;
import javax.microedition.rms.RecordStore;

/** Continue game: money, last position (on foot), clock and weapons kept in the phone's record store. */
final class Save {
    static final String NAME = "gtavc_save";

    static boolean exists() {
        try { RecordStore rs = RecordStore.openRecordStore(NAME, false); boolean e = rs.getNumRecords() > 0; rs.closeRecordStore(); return e; }
        catch (Throwable t) { return false; }
    }

    static void write(World w) {
        if (w == null || w.player == null || w.health <= 0 || w.busted) return;
        try {
            ByteArrayOutputStream bo = new ByteArrayOutputStream();
            DataOutputStream d = new DataOutputStream(bo);
            d.writeInt(1);
            d.writeInt(w.money);
            int x = w.inCar != null ? w.inCar.x : w.player.x, z = w.inCar != null ? w.inCar.z : w.player.z;
            d.writeInt(x); d.writeInt(z); d.writeInt(w.inCar != null ? w.inCar.y : w.player.y); d.writeInt(w.player.yaw);
            d.writeInt(w.minutes);
            d.writeBoolean(w.hasBat); d.writeBoolean(w.hasPistol); d.writeInt(w.ammo);
            d.close();
            byte[] b = bo.toByteArray();
            RecordStore rs = RecordStore.openRecordStore(NAME, true);
            if (rs.getNumRecords() > 0) rs.setRecord(1, b, 0, b.length); else rs.addRecord(b, 0, b.length);
            rs.closeRecordStore();
        } catch (Throwable t) { }
    }

    /** applies the save to a freshly reset world; false if there is none */
    static boolean load(World w) {
        try {
            RecordStore rs = RecordStore.openRecordStore(NAME, false);
            byte[] b = rs.getRecord(1);
            rs.closeRecordStore();
            DataInputStream d = new DataInputStream(new ByteArrayInputStream(b));
            d.readInt();
            w.money = d.readInt();
            int x = d.readInt(), z = d.readInt(), y = d.readInt(), yaw = d.readInt();
            w.minutes = d.readInt();
            w.hasBat = d.readBoolean(); w.hasPistol = d.readBoolean(); w.ammo = d.readInt();
            d.close();
            w.continueAt(x, z, y, yaw);
            return true;
        } catch (Throwable t) { return false; }
    }
}
