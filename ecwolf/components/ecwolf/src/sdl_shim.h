#ifndef SDL_SHIM_H
#define SDL_SHIM_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef uint32_t Uint32;
typedef uint16_t Uint16;
typedef uint8_t Uint8;
typedef int32_t Sint32;
typedef int16_t Sint16;
typedef int8_t Sint8;
typedef int64_t Sint64;
typedef uint64_t Uint64;

#define stricmp strcasecmp
#define strnicmp strncasecmp

#ifndef RG_PATH_MAX
#define RG_PATH_MAX 255
#endif

#define SDL_INIT_VIDEO       0x00000020
#define SDL_INIT_AUDIO       0x00000010
#define SDL_INIT_JOYSTICK    0x00000200
#define SDL_INIT_GAMECONTROLLER 0x00002000

#define SDL_FALSE 0
#define SDL_TRUE 1
typedef int SDL_bool;

typedef struct SDL_Rect {
    int x, y;
    int w, h;
} SDL_Rect;

typedef struct SDL_Color {
    Uint8 r, g, b, a;
} SDL_Color;

typedef struct SDL_Palette {
    int ncolors;
    SDL_Color *colors;
} SDL_Palette;

typedef struct SDL_PixelFormat {
    SDL_Palette *palette;
    Uint8 BitsPerPixel;
    Uint8 BytesPerPixel;
    Uint32 Rmask, Gmask, Bmask, Amask;
} SDL_PixelFormat;

typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat *format;
    int w, h;
    int pitch;
    void *pixels;
    void *userdata;
} SDL_Surface;

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x00001001
#define SDL_RENDERER_ACCELERATED      0x00000002
#define SDL_RENDERER_PRESENTVSYNC    0x00000004
#define SDL_RENDERER_TARGETTEXTURE   0x00000008
#define SDL_TEXTUREACCESS_STREAMING   1

#define SDL_PIXELFORMAT_RGB565 0x15151002
#define SDL_PIXELFORMAT_RGB888 0x16161804
#define SDL_PIXELFORMAT_ARGB8888 0x16362004
#define SDL_PIXELFORMAT_ARGB2101010 0x16362005
#define SDL_PIXELFORMAT_ARGB1555 0x15351002

#define SDL_WINDOWPOS_UNDEFINED_MASK    0x1FFF0000u
#define SDL_WINDOWPOS_UNDEFINED_DISPLAY(X)  (SDL_WINDOWPOS_UNDEFINED_MASK|(X))

typedef enum {
    SDL_SCANCODE_UNKNOWN = 0,
    SDL_SCANCODE_A = 4,
    SDL_SCANCODE_B = 5,
    SDL_SCANCODE_C = 6,
    SDL_SCANCODE_D = 7,
    SDL_SCANCODE_E = 8,
    SDL_SCANCODE_F = 9,
    SDL_SCANCODE_G = 10,
    SDL_SCANCODE_H = 11,
    SDL_SCANCODE_I = 12,
    SDL_SCANCODE_J = 13,
    SDL_SCANCODE_K = 14,
    SDL_SCANCODE_L = 15,
    SDL_SCANCODE_M = 16,
    SDL_SCANCODE_N = 17,
    SDL_SCANCODE_O = 18,
    SDL_SCANCODE_P = 19,
    SDL_SCANCODE_Q = 20,
    SDL_SCANCODE_R = 21,
    SDL_SCANCODE_S = 22,
    SDL_SCANCODE_T = 23,
    SDL_SCANCODE_U = 24,
    SDL_SCANCODE_V = 25,
    SDL_SCANCODE_W = 26,
    SDL_SCANCODE_X = 27,
    SDL_SCANCODE_Y = 28,
    SDL_SCANCODE_Z = 29,
    SDL_SCANCODE_1 = 30,
    SDL_SCANCODE_2 = 31,
    SDL_SCANCODE_3 = 32,
    SDL_SCANCODE_4 = 33,
    SDL_SCANCODE_5 = 34,
    SDL_SCANCODE_6 = 35,
    SDL_SCANCODE_7 = 36,
    SDL_SCANCODE_8 = 37,
    SDL_SCANCODE_9 = 38,
    SDL_SCANCODE_0 = 39,
    SDL_SCANCODE_RETURN = 40,
    SDL_SCANCODE_ESCAPE = 41,
    SDL_SCANCODE_BACKSPACE = 42,
    SDL_SCANCODE_TAB = 43,
    SDL_SCANCODE_SPACE = 44,
    SDL_SCANCODE_MINUS = 45,
    SDL_SCANCODE_EQUALS = 46,
    SDL_SCANCODE_LEFTBRACKET = 47,
    SDL_SCANCODE_RIGHTBRACKET = 48,
    SDL_SCANCODE_BACKSLASH = 49,
    SDL_SCANCODE_SEMICOLON = 51,
    SDL_SCANCODE_APOSTROPHE = 52,
    SDL_SCANCODE_GRAVE = 53,
    SDL_SCANCODE_COMMA = 54,
    SDL_SCANCODE_PERIOD = 55,
    SDL_SCANCODE_SLASH = 56,
    SDL_SCANCODE_CAPSLOCK = 57,
    SDL_SCANCODE_F1 = 58,
    SDL_SCANCODE_F2 = 59,
    SDL_SCANCODE_F3 = 60,
    SDL_SCANCODE_F4 = 61,
    SDL_SCANCODE_F5 = 62,
    SDL_SCANCODE_F6 = 63,
    SDL_SCANCODE_F7 = 64,
    SDL_SCANCODE_F8 = 65,
    SDL_SCANCODE_F9 = 66,
    SDL_SCANCODE_F10 = 67,
    SDL_SCANCODE_F11 = 68,
    SDL_SCANCODE_F12 = 69,
    SDL_SCANCODE_PRINTSCREEN = 70,
    SDL_SCANCODE_SCROLLLOCK = 71,
    SDL_SCANCODE_PAUSE = 72,
    SDL_SCANCODE_SYSREQ = 154,
    SDL_SCANCODE_INSERT = 73,
    SDL_SCANCODE_HOME = 74,
    SDL_SCANCODE_PAGEUP = 75,
    SDL_SCANCODE_DELETE = 76,
    SDL_SCANCODE_END = 77,
    SDL_SCANCODE_PAGEDOWN = 78,
    SDL_SCANCODE_RIGHT = 79,
    SDL_SCANCODE_LEFT = 80,
    SDL_SCANCODE_DOWN = 81,
    SDL_SCANCODE_UP = 82,
    SDL_SCANCODE_NUMLOCKCLEAR = 83,
    SDL_SCANCODE_KP_DIVIDE = 84,
    SDL_SCANCODE_KP_MULTIPLY = 85,
    SDL_SCANCODE_KP_MINUS = 86,
    SDL_SCANCODE_KP_PLUS = 87,
    SDL_SCANCODE_KP_ENTER = 88,
    SDL_SCANCODE_KP_1 = 89,
    SDL_SCANCODE_KP_2 = 90,
    SDL_SCANCODE_KP_3 = 91,
    SDL_SCANCODE_KP_4 = 92,
    SDL_SCANCODE_KP_5 = 93,
    SDL_SCANCODE_KP_6 = 94,
    SDL_SCANCODE_KP_7 = 95,
    SDL_SCANCODE_KP_8 = 96,
    SDL_SCANCODE_KP_9 = 97,
    SDL_SCANCODE_KP_0 = 98,
    SDL_SCANCODE_KP_PERIOD = 99,
    SDL_SCANCODE_APPLICATION = 101,
    SDL_SCANCODE_POWER = 102,
    SDL_SCANCODE_KP_EQUALS = 103,
    SDL_SCANCODE_F13 = 104,
    SDL_SCANCODE_F14 = 105,
    SDL_SCANCODE_F15 = 106,
    SDL_SCANCODE_HELP = 117,
    SDL_SCANCODE_MENU = 118,
    SDL_SCANCODE_UNDO = 122,
    SDL_SCANCODE_CLEAR = 156,
    SDL_SCANCODE_MODE = 257,
    SDL_SCANCODE_AC_BACK = 270,
    SDL_SCANCODE_LCTRL = 224,
    SDL_SCANCODE_LSHIFT = 225,
    SDL_SCANCODE_LALT = 226,
    SDL_SCANCODE_LGUI = 227,
    SDL_SCANCODE_RCTRL = 228,
    SDL_SCANCODE_RSHIFT = 229,
    SDL_SCANCODE_RALT = 230,
    SDL_SCANCODE_RGUI = 231,
    SDL_NUM_SCANCODES = 512
} SDL_Scancode;

typedef enum {
    SDL_KEYDOWN = 0x300,
    SDL_KEYUP,
    SDL_QUIT = 0x100,
    SDL_TEXTINPUT = 0x303,
    SDL_MOUSEMOTION = 0x400,
    SDL_MOUSEBUTTONDOWN = 0x401,
    SDL_MOUSEBUTTONUP = 0x402,
    SDL_MOUSEWHEEL = 0x403
} SDL_EventType;

typedef struct SDL_Keysym {
    SDL_Scancode scancode;
    int sym;
} SDL_Keysym;

typedef struct SDL_KeyboardEvent {
    Uint32 type;
    Uint8 state;
    SDL_Keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_TextInputEvent {
    Uint32 type;
    char text[32];
} SDL_TextInputEvent;

typedef struct SDL_MouseWheelEvent {
    Uint32 type;
    Sint32 x, y;
} SDL_MouseWheelEvent;

typedef union SDL_Event {
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_TextInputEvent text;
    SDL_MouseWheelEvent wheel;
} SDL_Event;

typedef struct SDL_AudioCVT {
    int needed;
    Uint16 src_format;
    Uint16 dst_format;
    double rate_incr;
    Uint8 *buf;
    int len;
    int len_cvt;
    int len_mult;
    double len_ratio;
    void (*filters[10])(struct SDL_AudioCVT *cvt, Uint16 format);
    int filter_index;
} SDL_AudioCVT;

#define AUDIO_S16 0x8010
#define SDL_MIX_MAXVOLUME 128

typedef enum {
    KMOD_NONE = 0x0000,
    KMOD_NUM = 0x1000
} SDL_Keymod;

#define SDLK_SCROLLLOCK 1073741892

#define SDL_HAT_UP    0x01
#define SDL_HAT_RIGHT 0x02
#define SDL_HAT_DOWN  0x04
#define SDL_HAT_LEFT  0x08

#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3
#define SDL_BUTTON(X)       (1 << ((X)-1))

typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_GameController SDL_GameController;
typedef Sint32 SDL_JoystickID;

typedef enum {
    SDL_CONTROLLER_AXIS_INVALID = -1,
    SDL_CONTROLLER_AXIS_LEFTX,
    SDL_CONTROLLER_AXIS_LEFTY,
    SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
    SDL_CONTROLLER_AXIS_MAX
} SDL_GameControllerAxis;

typedef enum {
    SDL_CONTROLLER_BUTTON_INVALID = -1,
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_MAX
} SDL_GameControllerButton;

#define SDL_IGNORE 0
#define SDL_DISABLE 0
#define SDL_ENABLE 1

typedef struct SDL_RWops {
    Sint64 (*seek)(struct SDL_RWops *context, Sint64 offset, int whence);
    size_t (*read)(struct SDL_RWops *context, void *ptr, size_t size, size_t maxnum);
    size_t (*write)(struct SDL_RWops *context, const void *ptr, size_t size, size_t num);
    int (*close)(struct SDL_RWops *context);
    Uint32 type;
    union {
        struct {
            void *data1;
            void *data2;
        } unknown;
    } hidden;
} SDL_RWops;

#define RW_SEEK_SET 0
#define RW_SEEK_CUR 1
#define RW_SEEK_END 2

typedef struct SDL_version {
    Uint8 major;
    Uint8 minor;
    Uint8 patch;
} SDL_version;

#ifndef WIFEXITED
#define WIFEXITED(status) 1
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(status) 0
#endif

#define SDL_RWsize(ctx) (ctx)->seek(ctx, 0, RW_SEEK_END)
#define SDL_RWseek(ctx, offset, whence) (ctx)->seek(ctx, offset, whence)
#define SDL_RWread(ctx, ptr, size, n) (ctx)->read(ctx, ptr, size, n)
#define SDL_RWclose(ctx) (ctx)->close(ctx)

#ifdef __cplusplus
extern "C" {
#endif

// itoa and ltoa are often in stdlib.h but let's be sure
#ifndef itoa
static inline char* id_itoa(int value, char* str, int base) { sprintf(str, "%d", value); return str; }
#define itoa id_itoa
#endif
#ifndef ltoa
static inline char* id_ltoa(long value, char* str, int base) { sprintf(str, "%ld", value); return str; }
#define ltoa id_ltoa
#endif

void SDL_GetVersion(SDL_version *ver);

int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, Uint16 src_format, Uint8 src_channels, int src_rate, Uint16 dst_format, Uint8 dst_channels, int dst_rate);
int SDL_ConvertAudio(SDL_AudioCVT *cvt);
void SDL_MixAudioFormat(Uint8 *dst, const Uint8 *src, Uint16 format, Uint32 len, int volume);
void SDL_MixAudio(Uint8 *dst, const Uint8 *src, Uint32 len, int volume);

int SDL_Init(Uint32 flags);
int SDL_InitSubSystem(Uint32 flags);
void SDL_Quit(void);
void SDL_QuitSubSystem(Uint32 flags);
Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32 ms);
const char *SDL_GetError(void);

int SDL_PollEvent(SDL_Event *event);
int SDL_WaitEvent(SDL_Event *event);

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags);
void SDL_DestroyWindow(SDL_Window *window);
SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags);
void SDL_DestroyRenderer(SDL_Renderer *renderer);
SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h);
void SDL_DestroyTexture(SDL_Texture *texture);
int SDL_LockTexture(SDL_Texture *texture, const SDL_Rect *rect, void **pixels, int *pitch);
void SDL_UnlockTexture(SDL_Texture *texture);
int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch);
int SDL_LockSurface(SDL_Surface *surface);
void SDL_UnlockSurface(SDL_Surface *surface);

Uint32 SDL_GetWindowFlags(SDL_Window *window);
void SDL_SetWindowSize(SDL_Window *window, int w, int h);
void SDL_GetWindowSize(SDL_Window *window, int *w, int *h);
SDL_Surface *SDL_GetWindowSurface(SDL_Window *window);
int SDL_UpdateWindowSurface(SDL_Window *window);
void SDL_SetWindowFullscreen(SDL_Window *window, Uint32 flags);

int SDL_RenderClear(SDL_Renderer *renderer);
int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect);
void SDL_RenderPresent(SDL_Renderer *renderer);
int SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
int SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h);

int SDL_QueryTexture(SDL_Texture *texture, Uint32 *format, int *access, int *w, int *h);
void SDL_PixelFormatEnumToMasks(Uint32 format, int *bpp, Uint32 *Rmask, Uint32 *Gmask, Uint32 *Bmask, Uint32 *Amask);
int SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors);

void SDL_SetRelativeMouseMode(SDL_bool enabled);
Uint32 SDL_GetMouseState(int *x, int *y);
Uint32 SDL_GetRelativeMouseState(int *x, int *y);
void SDL_EventState(Uint32 type, int state);
SDL_Keymod SDL_GetModState(void);
void SDL_SetModState(SDL_Keymod modstate);
int SDL_ShowCursor(int toggle);

int SDL_NumJoysticks(void);
SDL_bool SDL_IsGameController(int joystick_index);
SDL_GameController *SDL_GameControllerOpen(int joystick_index);
void SDL_GameControllerClose(SDL_GameController *gamecontroller);
const char *SDL_GameControllerName(SDL_GameController *gamecontroller);
void SDL_GameControllerUpdate(void);
Sint16 SDL_GameControllerGetAxis(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis);
SDL_bool SDL_GameControllerGetButton(SDL_GameController *gamecontroller, SDL_GameControllerButton button);
int SDL_GameControllerEventState(int state);

SDL_Joystick *SDL_JoystickOpen(int joystick_index);
void SDL_JoystickClose(SDL_Joystick *joystick);
void SDL_JoystickUpdate(void);
int SDL_JoystickNumButtons(SDL_Joystick *joystick);
int SDL_JoystickNumAxes(SDL_Joystick *joystick);
int SDL_JoystickNumHats(SDL_Joystick *joystick);
Sint16 SDL_JoystickGetAxis(SDL_Joystick *joystick, int axis);
Uint8 SDL_JoystickGetHat(SDL_Joystick *joystick, int hat);
SDL_bool SDL_JoystickGetButton(SDL_Joystick *joystick, int button);

SDL_RWops *SDL_RWFromMem(void *mem, int size);
SDL_RWops *SDL_AllocRW(void);
void SDL_FreeRW(SDL_RWops *area);

typedef struct SDL_mutex SDL_mutex;
SDL_mutex *SDL_CreateMutex(void);
void SDL_DestroyMutex(SDL_mutex *mutex);
int SDL_LockMutex(SDL_mutex *mutex);
int SDL_UnlockMutex(SDL_mutex *mutex);

#ifdef __cplusplus
}
#endif

#define SDL_VERSION_ATLEAST(X,Y,Z) (1)

#endif
