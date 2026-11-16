#define _POSIX_C_SOURCE 200809L
/*
 * MIDP/CLDC runtime shim for the C port of TheEscapists-J2ME.
 * Core: strings, Random, System/Thread, streams, RecordStore, media,
 * MIDlet/Display, GameCanvas.
 */
#include "j2me.h"

#include <time.h>
#include <strings.h>
#ifdef SF2000
extern uint32_t os_get_tick_count(void);
#endif

/* ------------------------------------------------------------------ */
/* Strings                                                             */
/* ------------------------------------------------------------------ */
int String_length(String s) {
    return s ? (int)strlen(s) : 0;
}

char String_charAt(String s, int i) {
    if (!s) return 0;
    int n = (int)strlen(s);
    if (i < 0 || i >= n) return 0;
    return s[i];
}

bool String_equals(String s, String t) {
    if (s == t) return true;
    if (!s || !t) return false;
    return strcmp(s, t) == 0;
}

bool String_equalsIgnoreCase(String s, String t) {
    if (s == t) return true;
    if (!s || !t) return false;
    return strcasecmp(s, t) == 0;
}

bool String_startsWith(String s, String prefix) {
    if (!s || !prefix) return false;
    size_t n = strlen(prefix);
    return strncmp(s, prefix, n) == 0;
}

int String_indexOf(String s, String needle) {
    if (!s || !needle) return -1;
    const char* p = strstr(s, needle);
    return p ? (int)(p - s) : -1;
}

String String_substring(String s, int begin) {
    if (!s) return "";
    int n = (int)strlen(s);
    if (begin < 0) begin = 0;
    if (begin > n) begin = n;
    char* out = (char*)malloc((size_t)(n - begin) + 1);
    memcpy(out, s + begin, (size_t)(n - begin) + 1);
    return out;
}

String String_substring2(String s, int begin, int end) {
    if (!s) return "";
    int n = (int)strlen(s);
    if (begin < 0) begin = 0;
    if (end > n) end = n;
    if (end < begin) end = begin;
    char* out = (char*)malloc((size_t)(end - begin) + 1);
    memcpy(out, s + begin, (size_t)(end - begin));
    out[end - begin] = 0;
    return out;
}

String String_concat(String s, String t) {
    if (!s) s = "";
    if (!t) t = "";
    size_t a = strlen(s), b = strlen(t);
    char* out = (char*)malloc(a + b + 1);
    memcpy(out, s, a);
    memcpy(out + a, t, b + 1);
    return out;
}

String String_valueOfInt(int v) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%d", v);
    return strdup(buf);
}

bool String_isEmpty(String s) {
    return !s || s[0] == 0;
}

void sb_init(StringBuffer* sb) {
    sb->buf = NULL;
    sb->len = 0;
    sb->cap = 0;
}

static void sb_reserve(StringBuffer* sb, int extra) {
    int need = sb->len + extra + 1;
    if (need <= sb->cap) return;
    int cap = sb->cap ? sb->cap : 32;
    while (cap < need) cap <<= 1;
    sb->buf = (char*)realloc(sb->buf, (size_t)cap);
    sb->cap = cap;
}

void sb_free(StringBuffer* sb) {
    free(sb->buf);
    sb->buf = NULL;
    sb->len = 0;
    sb->cap = 0;
}

void sb_setLength(StringBuffer* sb, int len) {
    if (len < 0) len = 0;
    if (len > sb->len) sb_reserve(sb, len - sb->len);
    sb->len = len;
    if (sb->buf) sb->buf[len] = 0;
}

int sb_length(StringBuffer* sb) { return sb->len; }

char sb_charAt(StringBuffer* sb, int i) {
    if (i < 0 || i >= sb->len) return 0;
    return sb->buf[i];
}

String sb_toString(StringBuffer* sb) {
    if (!sb->buf) return "";
    return strdup(sb->buf);
}

StringBuffer* sb_append(StringBuffer* sb, String s) {
    if (!s) s = "null";
    int n = (int)strlen(s);
    sb_reserve(sb, n);
    memcpy(sb->buf + sb->len, s, (size_t)n);
    sb->len += n;
    sb->buf[sb->len] = 0;
    return sb;
}

StringBuffer* sb_appendChar(StringBuffer* sb, char c) {
    sb_reserve(sb, 1);
    sb->buf[sb->len++] = c;
    sb->buf[sb->len] = 0;
    return sb;
}

StringBuffer* sb_appendInt(StringBuffer* sb, int v) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%d", v);
    return sb_append(sb, buf);
}

StringBuffer* sb_appendBool(StringBuffer* sb, bool v) {
    return sb_append(sb, v ? "true" : "false");
}

StringBuffer* sb_appendBuf(StringBuffer* sb, StringBuffer* other) {
    if (!other || !other->buf) return sb;
    return sb_append(sb, other->buf);
}

/* ------------------------------------------------------------------ */
/* Random (java.util.Random)                                           */
/* ------------------------------------------------------------------ */
#define RANDOM_MULT 0x5DEECE66DLL
#define RANDOM_ADD  0xBLL
#define RANDOM_MASK ((1LL << 48) - 1)

static int random_next(Random* r, int bits) {
    r->seed = (r->seed * RANDOM_MULT + RANDOM_ADD) & RANDOM_MASK;
    return (int)((uint64_t)r->seed >> (48 - bits));
}

void Random_initSeed(Random* r, int64_t seed) {
    r->seed = (seed ^ RANDOM_MULT) & RANDOM_MASK;
}

void Random_init(Random* r) {
    static int64_t counter = 0;
#ifdef SF2000
    int64_t seed = (int64_t)os_get_tick_count() * 1000 + (counter++);
#else
    int64_t seed = (int64_t)time(NULL) * 1000 + (counter++);
#endif
    Random_initSeed(r, seed);
}

int Random_nextInt(Random* r) {
    return random_next(r, 32);
}

int Random_nextIntBound(Random* r, int bound) {
    if (bound <= 0) return 0;
    if ((bound & -bound) == bound) {
        return (int)((bound * (int64_t)random_next(r, 31)) >> 31);
    }
    int bits, val;
    do {
        bits = random_next(r, 31);
        val = bits % bound;
    } while (bits - val + (bound - 1) < 0);
    return val;
}

bool Random_nextBoolean(Random* r) {
    return random_next(r, 1) != 0;
}

float Random_nextFloat(Random* r) {
    return random_next(r, 24) / (float)(1 << 24);
}

/* ------------------------------------------------------------------ */
/* System / Thread                                                     */
/* ------------------------------------------------------------------ */
long long System_currentTimeMillis(void) {
#ifdef SF2000
    return (long long)os_get_tick_count();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
#endif
}

void System_arraycopy(const void* src, int srcPos, void* dst, int dstPos, int length) {
    if (!src || !dst || length <= 0) return;
    memmove((uint8_t*)dst + dstPos, (const uint8_t*)src + srcPos, (size_t)length);
}

void System_gc(void) {
}

String System_getProperty(String key) {
    if (!key) return NULL;
    if (strcmp(key, "microedition.platform") == 0) return "SF2000";
    if (strcmp(key, "microedition.encoding") == 0) return "UTF-8";
    if (strcmp(key, "supports.mixing") == 0) return "true";
    if (strcmp(key, "com.nokia.keyboard.type") == 0) return "None";
    if (strcmp(key, "microedition.locale") == 0) return "en";
    return getenv(key);
}

int platform_frame_limit = -1;
int platform_fast_mode = 0;
void (*platform_frame_callback)(void) = NULL;
static char g_data_dir[512];

void Thread_sleep(long long ms) {
#ifdef SF2000
    (void)ms;
    return;
#else
    if (getenv("TE_FAST") || platform_fast_mode) return;
    if (ms <= 0) return;
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000);
    ts.tv_nsec = (long)((ms % 1000) * 1000000LL);
    nanosleep(&ts, NULL);
#endif
}

void Thread_yield(void) {
}

/* ------------------------------------------------------------------ */
/* DataInputStream / DataOutputStream                                  */
/* ------------------------------------------------------------------ */
void dis_init(DataInputStream* d, const uint8_t* data, int len) {
    d->data = data;
    d->len = len;
    d->pos = 0;
}

void dis_initStream(DataInputStream* d, const uint8_t* data, int len) {
    dis_init(d, data, len);
}

static bool dis_need(DataInputStream* d, int n) {
    return d->pos + n <= d->len;
}

int dis_readByte(DataInputStream* d) {
    if (!dis_need(d, 1)) return 0;
    return (int8_t)d->data[d->pos++];
}

int dis_readUnsignedByte(DataInputStream* d) {
    if (!dis_need(d, 1)) return 0;
    return d->data[d->pos++];
}

int dis_readShort(DataInputStream* d) {
    int a, b;
    if (!dis_need(d, 2)) return 0;
    a = d->data[d->pos++];
    b = d->data[d->pos++];
    return (int16_t)((a << 8) | b);
}

int dis_readUnsignedShort(DataInputStream* d) {
    int a, b;
    if (!dis_need(d, 2)) return 0;
    a = d->data[d->pos++];
    b = d->data[d->pos++];
    return ((a << 8) | b) & 0xFFFF;
}

int dis_readInt(DataInputStream* d) {
    if (!dis_need(d, 4)) return 0;
    uint32_t v = ((uint32_t)d->data[d->pos] << 24) |
                 ((uint32_t)d->data[d->pos + 1] << 16) |
                 ((uint32_t)d->data[d->pos + 2] << 8) |
                 ((uint32_t)d->data[d->pos + 3]);
    d->pos += 4;
    return (int)v;
}

long long dis_readLong(DataInputStream* d) {
    if (!dis_need(d, 8)) return 0;
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v = (v << 8) | d->data[d->pos + i];
    d->pos += 8;
    return (long long)v;
}

bool dis_readBoolean(DataInputStream* d) {
    return dis_readByte(d) != 0;
}

String dis_readUTF(DataInputStream* d) {
    int len = (dis_readByte(d) & 0xFF) << 8;
    len |= dis_readByte(d) & 0xFF;
    if (len < 0 || !dis_need(d, len)) return strdup("");
    char* s = (char*)malloc((size_t)len + 1);
    memcpy(s, d->data + d->pos, (size_t)len);
    s[len] = 0;
    d->pos += len;
    return s;
}

void dis_readFully(DataInputStream* d, uint8_t* buf, int len) {
    if (!dis_need(d, len)) len = d->len - d->pos;
    if (len > 0) memcpy(buf, d->data + d->pos, (size_t)len);
    d->pos += len;
}

int dis_available(DataInputStream* d) {
    return d->len - d->pos;
}

void dis_close(DataInputStream* d) {
    d->data = NULL;
    d->len = d->pos = 0;
}

void dos_init(DataOutputStream* d) {
    d->data = NULL;
    d->len = 0;
    d->cap = 0;
}

void dos_free(DataOutputStream* d) {
    free(d->data);
    d->data = NULL;
    d->len = d->cap = 0;
}

static void dos_reserve(DataOutputStream* d, int extra) {
    int need = d->len + extra;
    if (need <= d->cap) return;
    int cap = d->cap ? d->cap : 64;
    while (cap < need) cap <<= 1;
    d->data = (uint8_t*)realloc(d->data, (size_t)cap);
    d->cap = cap;
}

void dos_writeByte(DataOutputStream* d, int v) {
    dos_reserve(d, 1);
    d->data[d->len++] = (uint8_t)v;
}

void dos_writeShort(DataOutputStream* d, int v) {
    dos_reserve(d, 2);
    d->data[d->len++] = (uint8_t)(v >> 8);
    d->data[d->len++] = (uint8_t)v;
}

void dos_writeInt(DataOutputStream* d, int v) {
    dos_reserve(d, 4);
    d->data[d->len++] = (uint8_t)(v >> 24);
    d->data[d->len++] = (uint8_t)(v >> 16);
    d->data[d->len++] = (uint8_t)(v >> 8);
    d->data[d->len++] = (uint8_t)v;
}

void dos_writeLong(DataOutputStream* d, long long v) {
    for (int i = 7; i >= 0; i--) {
        dos_writeByte(d, (int)((v >> (i * 8)) & 0xFF));
    }
}

void dos_writeBoolean(DataOutputStream* d, bool v) {
    dos_writeByte(d, v ? 1 : 0);
}

void dos_writeUTF(DataOutputStream* d, String s) {
    if (!s) s = "";
    int n = (int)strlen(s);
    if (n > 65535) n = 65535;
    dos_writeShort(d, n);
    dos_write(d, (const uint8_t*)s, n);
}

void dos_write(DataOutputStream* d, const uint8_t* buf, int len) {
    if (len <= 0) return;
    dos_reserve(d, len);
    memcpy(d->data + d->len, buf, (size_t)len);
    d->len += len;
}

const uint8_t* dos_toByteArray(DataOutputStream* d) {
    return d->data;
}

int dos_length(DataOutputStream* d) {
    return d->len;
}

/* ------------------------------------------------------------------ */
/* RecordStore (file backed)                                           */
/* ------------------------------------------------------------------ */
#define RMS_MAGIC 0x54455253u

const char* platform_data_dir(void) {
    if (g_data_dir[0]) return g_data_dir;
    const char* d = getenv("TE_DATA_DIR");
    if (d && *d) return d;
#ifdef SF2000
    return "/mnt/sda1";
#else
    return ".";
#endif
}

void platform_set_data_dir(String dir) {
    if (!dir) { g_data_dir[0] = 0; return; }
    snprintf(g_data_dir, sizeof(g_data_dir), "%s", dir);
}

static void rms_path(String name, char* out, size_t cap) {
    snprintf(out, cap, "%s/te_%s.rms", platform_data_dir(), name ? name : "store");
}

static void rms_persist(RecordStore* rs) {
    char path[512];
    rms_path(rs->name, path, sizeof(path));
    FILE* f = fopen(path, "wb");
    if (!f) return;
    uint32_t magic = RMS_MAGIC;
    uint32_t count = (uint32_t)rs->count;
    fwrite(&magic, 4, 1, f);
    fwrite(&count, 4, 1, f);
    for (int i = 0; i < rs->count; i++) {
        uint32_t len = (uint32_t)rs->lengths[i];
        fwrite(&len, 4, 1, f);
        if (len && rs->records[i]) fwrite(rs->records[i], 1, len, f);
    }
    fclose(f);
}

RecordStore* RecordStore_openRecordStore(String name, bool create) {
    char path[512];
    rms_path(name, path, sizeof(path));
    FILE* f = fopen(path, "rb");
    if (!f) {
        if (!create) return NULL;
        RecordStore* rs = (RecordStore*)calloc(1, sizeof(RecordStore));
        snprintf(rs->name, sizeof(rs->name), "%s", name ? name : "store");
        rms_persist(rs);
        return rs;
    }
    uint32_t magic = 0, count = 0;
    if (fread(&magic, 4, 1, f) != 1 || magic != RMS_MAGIC ||
        fread(&count, 4, 1, f) != 1 || count > 4096) {
        fclose(f);
        return NULL;
    }
    RecordStore* rs = (RecordStore*)calloc(1, sizeof(RecordStore));
    snprintf(rs->name, sizeof(rs->name), "%s", name ? name : "store");
    rs->cap = (int)count;
    rs->records = (uint8_t**)calloc((size_t)count, sizeof(uint8_t*));
    rs->lengths = (int*)calloc((size_t)count, sizeof(int));
    rs->count = (int)count;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t len = 0;
        if (fread(&len, 4, 1, f) != 1 || len > (16u << 20)) {
            fclose(f);
            RecordStore_closeRecordStore(rs);
            return NULL;
        }
        rs->lengths[i] = (int)len;
        rs->records[i] = (uint8_t*)malloc(len ? len : 1);
        if (len && fread(rs->records[i], 1, len, f) != len) {
            fclose(f);
            RecordStore_closeRecordStore(rs);
            return NULL;
        }
    }
    fclose(f);
    return rs;
}

void RecordStore_closeRecordStore(RecordStore* rs) {
    if (!rs) return;
    for (int i = 0; i < rs->count; i++) free(rs->records[i]);
    free(rs->records);
    free(rs->lengths);
    free(rs);
}

int RecordStore_addRecord(RecordStore* rs, const uint8_t* data, int offset, int len) {
    if (!rs || len < 0) return -1;
    if (rs->count >= rs->cap) {
        rs->cap = rs->cap ? rs->cap * 2 : 4;
        rs->records = (uint8_t**)realloc(rs->records, (size_t)rs->cap * sizeof(uint8_t*));
        rs->lengths = (int*)realloc(rs->lengths, (size_t)rs->cap * sizeof(int));
    }
    uint8_t* copy = (uint8_t*)malloc(len ? (size_t)len : 1);
    if (len) memcpy(copy, data + offset, (size_t)len);
    rs->records[rs->count] = copy;
    rs->lengths[rs->count] = len;
    rs->count++;
    rms_persist(rs);
    return rs->count;
}

void RecordStore_setRecord(RecordStore* rs, int recordId, const uint8_t* data, int offset, int len) {
    if (!rs || recordId < 1 || recordId > rs->count || len < 0) return;
    uint8_t* copy = (uint8_t*)malloc(len ? (size_t)len : 1);
    if (len) memcpy(copy, data + offset, (size_t)len);
    free(rs->records[recordId - 1]);
    rs->records[recordId - 1] = copy;
    rs->lengths[recordId - 1] = len;
    rms_persist(rs);
}

const uint8_t* RecordStore_getRecord(RecordStore* rs, int recordId, int* outLen) {
    if (outLen) *outLen = 0;
    if (!rs || recordId < 1 || recordId > rs->count) return NULL;
    if (outLen) *outLen = rs->lengths[recordId - 1];
    return rs->records[recordId - 1];
}

int RecordStore_getNumRecords(RecordStore* rs) {
    return rs ? rs->count : 0;
}

int RecordStore_getRecordSize(RecordStore* rs, int recordId) {
    if (!rs || recordId < 1 || recordId > rs->count) return 0;
    return rs->lengths[recordId - 1];
}

void RecordStore_deleteRecordStore(String name) {
    char path[512];
    rms_path(name, path, sizeof(path));
    remove(path);
}

/* ------------------------------------------------------------------ */
/* Media stubs                                                         */
/* ------------------------------------------------------------------ */
struct Player {
    const uint8_t* data;
    int            len;
    String         type;
    int            loopCount;
    VolumeControl  vc;
    int            state;
    bool           playing;
};

Player* Manager_createPlayerData(const uint8_t* data, int len, String type) {
    Player* p = (Player*)calloc(1, sizeof(Player));
    p->data = data;
    p->len = len;
    p->type = type;
    p->loopCount = 1;
    p->vc.volume = 100;
    p->state = PLAYER_UNREALIZED;
    return p;
}

Player* Manager_createPlayerRes(String path, String type) {
    int len = 0;
    uint8_t* data = platform_read_resource(path, &len);
    Player* p = Manager_createPlayerData(data, len, type);
    return p;
}

void Player_prefetch(Player* p) { if (p) p->state = PLAYER_PREFETCHED; }
void Player_realize(Player* p) { if (p && p->state < PLAYER_REALIZED) p->state = PLAYER_REALIZED; }
void Player_start(Player* p) { if (p) { p->state = PLAYER_STARTED; p->playing = true; } }
void Player_stop(Player* p) { if (p) p->playing = false; }
void Player_deallocate(Player* p) { if (p) p->playing = false; }
void Player_close(Player* p) { if (p) { p->playing = false; p->state = PLAYER_CLOSED; } }
void Player_free(Player* p) { if (!p) return; free((void*)p->data); free(p); }
void Player_setLoopCount(Player* p, int count) { if (p) p->loopCount = count; }
void Player_addPlayerListener(Player* p, void* listener) { (void)p; (void)listener; }
void Player_setMediaTime(Player* p, long long time) { (void)p; (void)time; }
int Player_getState(Player* p) { return p ? p->state : PLAYER_CLOSED; }
VolumeControl* Player_getVolumeControl(Player* p) { return p ? &p->vc : NULL; }
void Player_setVolume(Player* p, int volume) { if (p) p->vc.volume = volume; }
void VolumeControl_setMute(VolumeControl* c, bool mute) { if (c) c->mute = mute; }
void VolumeControl_setLevel(VolumeControl* c, int level) { if (c) c->volume = level; }

/* ------------------------------------------------------------------ */
/* GameCanvas / MIDlet / Display                                       */
/* ------------------------------------------------------------------ */
void GameCanvas_init(GameCanvas* c, int w, int h) {
    c->width = w;
    c->height = h;
    c->screen = Image_createImage(w, h);
    c->graphics = Image_getGraphics(c->screen);
    c->keyStates = 0;
    c->shown = true;
}

void GameCanvas_initFromImage(GameCanvas* c, Image* img) {
    c->screen = img;
    c->graphics = Image_getGraphics(img);
    c->width = img ? img->width : 0;
    c->height = img ? img->height : 0;
    c->keyStates = 0;
    c->shown = true;
}

int GameCanvas_getWidth(GameCanvas* c) { return c ? c->width : 0; }
int GameCanvas_getHeight(GameCanvas* c) { return c ? c->height : 0; }
Graphics* GameCanvas_getGraphics(GameCanvas* c) { return c ? c->graphics : NULL; }
int GameCanvas_getKeyStates(GameCanvas* c) { return c ? c->keyStates : 0; }
bool GameCanvas_isShown(GameCanvas* c) { return c ? c->shown : false; }
void GameCanvas_setFullScreenMode(GameCanvas* c, bool on) { (void)c; (void)on; }

int GameCanvas_getGameAction(int key) {
    switch (key) {
    case KEY_UP: case KEY_NUM2: return GC_UP;
    case KEY_DOWN: case KEY_NUM8: return GC_DOWN;
    case KEY_LEFT: case KEY_NUM4: return GC_LEFT;
    case KEY_RIGHT: case KEY_NUM6: return GC_RIGHT;
    case KEY_FIRE: case KEY_NUM5: return GC_FIRE;
    case KEY_GAME_A: return GC_GAME_A;
    case KEY_GAME_B: return GC_GAME_B;
    case KEY_GAME_C: return GC_GAME_C;
    case KEY_GAME_D: return GC_GAME_D;
    default: return 0;
    }
}

bool GameCanvas_hasPointerEvents(void) {
    return false;
}

void GameCanvas_flushGraphics(GameCanvas* c) {
    if (!c || !c->screen) return;
    platform_present(c->screen->argb, c->screen->width, c->screen->height);
}

String MIDlet_getAppProperty(MIDlet* m, String key) {
    (void)m;
    if (!key) return "";
    if (strcmp(key, "MIDlet-Version") == 0) return "1.0.0-c";
    if (strcmp(key, "MIDlet-Name") == 0) return "The Escapists";
    if (strcmp(key, "MIDlet-Vendor") == 0) return "port";
    if (strcmp(key, "TE-Build") == 0) return "C port";
    return "";
}

int Display_numAlphaLevels(Display* d) {
    (void)d;
    return 256;
}
