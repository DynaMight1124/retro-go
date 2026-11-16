package javax.microedition.rms;
/** desktop stand-in: keeps records in memory */
public class RecordStore {
    static java.util.HashMap<String, java.util.ArrayList<byte[]>> all = new java.util.HashMap<String, java.util.ArrayList<byte[]>>();
    java.util.ArrayList<byte[]> recs;
    public static RecordStore openRecordStore(String name, boolean create) throws RecordStoreException {
        if (!all.containsKey(name)) { if (!create) throw new RecordStoreException("none"); all.put(name, new java.util.ArrayList<byte[]>()); }
        RecordStore r = new RecordStore(); r.recs = all.get(name); return r;
    }
    public int getNumRecords() { return recs.size(); }
    public byte[] getRecord(int id) throws RecordStoreException { return recs.get(id - 1); }
    public int addRecord(byte[] d, int o, int l) throws RecordStoreException { byte[] b = new byte[l]; System.arraycopy(d, o, b, 0, l); recs.add(b); return recs.size(); }
    public void setRecord(int id, byte[] d, int o, int l) throws RecordStoreException { byte[] b = new byte[l]; System.arraycopy(d, o, b, 0, l); recs.set(id - 1, b); }
    public void closeRecordStore() throws RecordStoreException { }
}
