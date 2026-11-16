/*
 * Minimal MIDP 2.0 / CLDC 1.1 compatibility layer for the C port of
 * TheEscapists-J2ME (https://github.com/shinovon/TheEscapists-j2me).
 *
 * This layer is a platform adaptation only: the game logic is a direct
 * translation of the original Java sources in reference/.
 */
#ifndef TE_J2ME_H
#define TE_J2ME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef const char* String;

#ifndef null
#define null NULL
#endif

/* ------------------------------------------------------------------ */
/* java.lang.Math                                                      */
/* ------------------------------------------------------------------ */
#define Math_abs(x) ({ __typeof__(x) _x = (x); _x < 0 ? -_x : _x; })
#define Math_min(a,b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#define Math_max(a,b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })
#define Math_sqrt(x)  sqrt(x)
#define Math_sin(x)   sin(x)
#define Math_cos(x)   cos(x)
#define Math_PI       3.14159265358979323846

/* ------------------------------------------------------------------ */
/* java.lang.String / StringBuffer                                     */
/* ------------------------------------------------------------------ */
int   String_length(String s);
char  String_charAt(String s, int i);
bool  String_equals(String s, String t);
bool  String_equalsIgnoreCase(String s, String t);
bool  String_startsWith(String s, String prefix);
int   String_indexOf(String s, String needle);
String String_substring(String s, int begin);
String String_substring2(String s, int begin, int end);
String String_concat(String s, String t);
String String_valueOfInt(int v);
bool  String_isEmpty(String s);

typedef struct StringBuffer {
    char* buf;
    int   len;
    int   cap;
} StringBuffer;
typedef StringBuffer StringBuilder;

void   sb_init(StringBuffer* sb);
void   sb_free(StringBuffer* sb);
void   sb_setLength(StringBuffer* sb, int len);
int    sb_length(StringBuffer* sb);
char   sb_charAt(StringBuffer* sb, int i);
String sb_toString(StringBuffer* sb);
StringBuffer* sb_append(StringBuffer* sb, String s);
StringBuffer* sb_appendChar(StringBuffer* sb, char c);
StringBuffer* sb_appendInt(StringBuffer* sb, int v);
StringBuffer* sb_appendBool(StringBuffer* sb, bool v);
StringBuffer* sb_appendBuf(StringBuffer* sb, StringBuffer* other);

/* ------------------------------------------------------------------ */
/* java.util.Random (exact 48-bit LCG)                                 */
/* ------------------------------------------------------------------ */
typedef struct Random {
    int64_t seed;
} Random;

void  Random_init(Random* r);
void  Random_initSeed(Random* r, int64_t seed);
int   Random_nextInt(Random* r);
int   Random_nextIntBound(Random* r, int bound);
bool  Random_nextBoolean(Random* r);
float Random_nextFloat(Random* r);

/* ------------------------------------------------------------------ */
/* java.lang.System / Thread                                           */
/* ------------------------------------------------------------------ */
long long System_currentTimeMillis(void);
void      System_arraycopy(const void* src, int srcPos, void* dst, int dstPos, int length);
void      System_gc(void);
String    System_getProperty(String key);

void Thread_sleep(long long ms);
void Thread_yield(void);

/* ------------------------------------------------------------------ */
/* javax.microedition.lcdui.Image                                      */
/* ------------------------------------------------------------------ */
typedef struct Image {
    int       width;
    int       height;
    uint32_t* argb;    /* 0xAARRGGBB, non-premultiplied */
    bool      owned;
} Image;

Image* Image_createImage(int w, int h);
Image* Image_createImageRes(String path);
Image* Image_createRGBImage(const int* rgb, int w, int h, bool processAlpha);
void   Image_getRGB(Image* img, int* rgb, int offset, int scanlength,
                    int x, int y, int w, int h);
int    Image_getWidth(Image* img);
int    Image_getHeight(Image* img);
void   Image_free(Image* img);

/* ------------------------------------------------------------------ */
/* javax.microedition.lcdui.Graphics                                   */
/* ------------------------------------------------------------------ */
#define HCENTER  1
#define VCENTER  2
#define LEFT     4
#define RIGHT    8
#define TOP      16
#define BOTTOM   32

#define TRANS_NONE            0
#define TRANS_MIRROR_ROT180   1
#define TRANS_MIRROR          2
#define TRANS_ROT180          3
#define TRANS_MIRROR_ROT270   4
#define TRANS_ROT90           5
#define TRANS_ROT270          6
#define TRANS_MIRROR_ROT90    7

typedef struct Graphics {
    Image*    image;
    int       tx, ty;
    int       clipX, clipY, clipW, clipH;
    uint32_t  color;
} Graphics;

Graphics* Image_getGraphics(Image* img);
void g_setColor(Graphics* g, int rgb);
void g_fillRect(Graphics* g, int x, int y, int w, int h);
void g_drawRect(Graphics* g, int x, int y, int w, int h);
void g_drawLine(Graphics* g, int x1, int y1, int x2, int y2);
void g_fillTriangle(Graphics* g, int x1, int y1, int x2, int y2, int x3, int y3);
void g_drawRegion(Graphics* g, Image* src, int sx, int sy, int sw, int sh,
                  int transform, int dx, int dy, int anchor);
void g_drawImage(Graphics* g, Image* src, int x, int y, int anchor);
void g_drawRGB(Graphics* g, const int* rgb, int offset, int scanlength,
               int x, int y, int w, int h, bool processAlpha);
void g_translate(Graphics* g, int x, int y);
void g_setClip(Graphics* g, int x, int y, int w, int h);
int  g_getTranslateX(Graphics* g);
int  g_getTranslateY(Graphics* g);
void g_drawString(Graphics* g, String s, int x, int y, int anchor);

/* ------------------------------------------------------------------ */
/* javax.microedition.lcdui.game.Sprite                                */
/* ------------------------------------------------------------------ */
typedef struct Sprite {
    Image* image;
    int    frameW, frameH;
    int    cols;
    int*   sequence;
    int    seqLen;
    int    seqIndex;
    int    refX, refY;
    int    x, y;
} Sprite;

Sprite* Sprite_new(Image* image, int frameWidth, int frameHeight);
Sprite* Sprite_setRefPixelPosition(Sprite* s, int x, int y);
Sprite* Sprite_setPosition(Sprite* s, int x, int y);
Sprite* Sprite_setFrame(Sprite* s, int frame);
Sprite* Sprite_setFrameSequence(Sprite* s, const int* sequence, int length);
void    Sprite_setImage(Sprite* s, Image* image, int frameWidth, int frameHeight);
void    Sprite_paint(Sprite* s, Graphics* g);
int     Sprite_getX(Sprite* s);
int     Sprite_getY(Sprite* s);
void    Sprite_free(Sprite* s);

/* ------------------------------------------------------------------ */
/* javax.microedition.io streams (memory backed)                       */
/* ------------------------------------------------------------------ */
typedef struct DataInputStream {
    const uint8_t* data;
    int            len;
    int            pos;
} DataInputStream;

void   dis_init(DataInputStream* d, const uint8_t* data, int len);
void   dis_initStream(DataInputStream* d, const uint8_t* data, int len);
int    dis_readByte(DataInputStream* d);
int    dis_readUnsignedByte(DataInputStream* d);
int    dis_readShort(DataInputStream* d);
int    dis_readUnsignedShort(DataInputStream* d);
int    dis_readInt(DataInputStream* d);
long long dis_readLong(DataInputStream* d);
bool   dis_readBoolean(DataInputStream* d);
String dis_readUTF(DataInputStream* d);
void   dis_readFully(DataInputStream* d, uint8_t* buf, int len);
int    dis_available(DataInputStream* d);
void   dis_close(DataInputStream* d);

typedef struct DataOutputStream {
    uint8_t* data;
    int      len;
    int      cap;
} DataOutputStream;

void   dos_init(DataOutputStream* d);
void   dos_free(DataOutputStream* d);
void   dos_writeByte(DataOutputStream* d, int v);
void   dos_writeShort(DataOutputStream* d, int v);
void   dos_writeInt(DataOutputStream* d, int v);
void   dos_writeLong(DataOutputStream* d, long long v);
void   dos_writeBoolean(DataOutputStream* d, bool v);
void   dos_writeUTF(DataOutputStream* d, String s);
void   dos_write(DataOutputStream* d, const uint8_t* buf, int len);
const uint8_t* dos_toByteArray(DataOutputStream* d);
int    dos_length(DataOutputStream* d);

/* ------------------------------------------------------------------ */
/* javax.microedition.rms.RecordStore                                  */
/* ------------------------------------------------------------------ */
typedef struct RecordStore {
    char     name[64];
    int      count;
    int      cap;
    uint8_t** records;
    int*     lengths;
} RecordStore;

RecordStore* RecordStore_openRecordStore(String name, bool create);
void         RecordStore_closeRecordStore(RecordStore* rs);
int          RecordStore_addRecord(RecordStore* rs, const uint8_t* data, int offset, int len);
void         RecordStore_setRecord(RecordStore* rs, int recordId, const uint8_t* data, int offset, int len);
const uint8_t* RecordStore_getRecord(RecordStore* rs, int recordId, int* outLen);
int          RecordStore_getNumRecords(RecordStore* rs);
int          RecordStore_getRecordSize(RecordStore* rs, int recordId);
void         RecordStore_deleteRecordStore(String name);

/* ------------------------------------------------------------------ */
/* javax.microedition.media (stubs backed by a platform hook)          */
/* ------------------------------------------------------------------ */
typedef struct Player Player;
typedef struct VolumeControl { int volume; bool mute; } VolumeControl;

#define PLAYER_CLOSED       0
#define PLAYER_UNREALIZED   100
#define PLAYER_REALIZED     200
#define PLAYER_PREFETCHED   300
#define PLAYER_STARTED      400

Player*        Manager_createPlayerData(const uint8_t* data, int len, String type);
Player*        Manager_createPlayerRes(String path, String type);
void           Player_prefetch(Player* p);
void           Player_realize(Player* p);
void           Player_start(Player* p);
void           Player_stop(Player* p);
void           Player_deallocate(Player* p);
void           Player_close(Player* p);
void           Player_free(Player* p);
void           Player_setLoopCount(Player* p, int count);
void           Player_addPlayerListener(Player* p, void* listener);
void           Player_setMediaTime(Player* p, long long time);
int            Player_getState(Player* p);
VolumeControl* Player_getVolumeControl(Player* p);
void           Player_setVolume(Player* p, int volume);
void           VolumeControl_setMute(VolumeControl* c, bool mute);
void           VolumeControl_setLevel(VolumeControl* c, int level);

/* ------------------------------------------------------------------ */
/* javax.microedition.lcdui.game.GameCanvas                            */
/* ------------------------------------------------------------------ */
#define GC_UP    1
#define GC_DOWN  6
#define GC_LEFT  2
#define GC_RIGHT 5
#define GC_FIRE  8
#define GC_GAME_A 9
#define GC_GAME_B 10
#define GC_GAME_C 11
#define GC_GAME_D 12

#define UP_PRESSED    (1 << GC_UP)
#define DOWN_PRESSED  (1 << GC_DOWN)
#define LEFT_PRESSED  (1 << GC_LEFT)
#define RIGHT_PRESSED (1 << GC_RIGHT)
#define FIRE_PRESSED  (1 << GC_FIRE)
#define GAME_A_PRESSED (1 << GC_GAME_A)
#define GAME_B_PRESSED (1 << GC_GAME_B)
#define GAME_C_PRESSED (1 << GC_GAME_C)
#define GAME_D_PRESSED (1 << GC_GAME_D)

#define KEY_NUM0   48
#define KEY_NUM1   49
#define KEY_NUM2   50
#define KEY_NUM3   51
#define KEY_NUM4   52
#define KEY_NUM5   53
#define KEY_NUM6   54
#define KEY_NUM7   55
#define KEY_NUM8   56
#define KEY_NUM9   57
#define KEY_STAR   42
#define KEY_POUND  35
#define KEY_UP     (-1)
#define KEY_DOWN   (-2)
#define KEY_LEFT   (-3)
#define KEY_RIGHT  (-4)
#define KEY_FIRE   (-5)
#define KEY_GAME_A (-6)
#define KEY_GAME_B (-7)
#define KEY_GAME_C (-8)
#define KEY_GAME_D (-9)

typedef struct GameCanvas {
    int       width;
    int       height;
    Image*    screen;
    Graphics* graphics;
    int       keyStates;
    bool      shown;
} GameCanvas;

void GameCanvas_init(GameCanvas* c, int w, int h);
void GameCanvas_initFromImage(GameCanvas* c, Image* img);
int  GameCanvas_getWidth(GameCanvas* c);
int  GameCanvas_getHeight(GameCanvas* c);
Graphics* GameCanvas_getGraphics(GameCanvas* c);
int  GameCanvas_getKeyStates(GameCanvas* c);
int  GameCanvas_getGameAction(int key);
bool GameCanvas_hasPointerEvents(void);
bool GameCanvas_isShown(GameCanvas* c);
void GameCanvas_setFullScreenMode(GameCanvas* c, bool on);
void GameCanvas_flushGraphics(GameCanvas* c);

/* ------------------------------------------------------------------ */
/* MIDlet / Display                                                    */
/* ------------------------------------------------------------------ */
typedef struct Display {
    GameCanvas* current;
    int         width;
    int         height;
} Display;

typedef struct MIDlet {
    Display* display;
    bool     paused;
} MIDlet;

extern MIDlet* TE_midlet;

String MIDlet_getAppProperty(MIDlet* m, String key);
int    Display_numAlphaLevels(Display* d);

/* ------------------------------------------------------------------ */
/* Platform hooks                                                      */
/* ------------------------------------------------------------------ */
void   platform_set_assets_dir(String dir);
String platform_assets_dir(void);
const char* platform_data_dir(void);
void   platform_set_data_dir(String dir);
uint8_t* platform_read_resource(String path, int* outLen);
void   platform_present(const uint32_t* argb, int w, int h);
void   platform_log(String msg);
int    platform_render_width(void);
extern int platform_frame_limit;
extern int platform_fast_mode;
extern void (*platform_frame_callback)(void);
int    platform_render_height(void);

#endif /* TE_J2ME_H */
