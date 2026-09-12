#ifndef SDL_STUB_H
#define SDL_STUB_H
#define _SDL_H
#define SDL_H

#include <stdint.h>
#include <string.h>

typedef struct {
    int sym;
} SDL_keysym;

typedef struct {
    SDL_keysym keysym;
} SDL_KeyboardEvent;

typedef struct {
    uint8_t which;
    uint8_t axis;
    int16_t value;
} SDL_JoyAxisEvent;

typedef struct {
    uint8_t which;
    uint8_t button;
    uint8_t state;
} SDL_JoyButtonEvent;

typedef struct {
    uint8_t type;
    union {
        SDL_KeyboardEvent key;
        SDL_JoyAxisEvent jaxis;
        SDL_JoyButtonEvent jbutton;
    };
} SDL_Event;

typedef void* SDL_Joystick;

#define SDL_INIT_TIMER    0x00000001
#define SDL_INIT_AUDIO    0x00000010
#define SDL_INIT_VIDEO    0x00000020
#define SDL_INIT_JOYSTICK 0x00000200

#define SDL_KEYDOWN 1
#define SDL_KEYUP 2
#define SDL_JOYAXISMOTION 3
#define SDL_JOYBUTTONDOWN 4
#define SDL_JOYBUTTONUP 5
#define SDL_QUIT 6

#define SDLK_q 'q'
#define SDLK_3 '3'
#define SDLK_HOME 0x100
#define SDLK_UP 0x101
#define SDLK_DOWN 0x102
#define SDLK_LEFT 0x103
#define SDLK_RIGHT 0x104
#define SDLK_LCTRL 0x105
#define SDLK_LALT 0x106
#define SDLK_LSHIFT 0x107
#define SDLK_RETURN 0x108
#define SDLK_ESCAPE 0x109
#define SDLK_SPACE 0x10A
#define SDLK_TAB 0x10B
#define SDLK_BACKSPACE 0x10C
#define SDLK_F1 0x10D
#define SDLK_F2 0x10E
#define SDLK_F3 0x10F
#define SDLK_F5 0x110
#define SDLK_k 'k'
#define SDLK_u 'u'
#define SDLK_d 'd'
#define SDLK_l 'l'
#define SDLK_r 'r'
#define SDLK_b 'b'
#define SDLK_y 'y'
#define SDLK_s 's'
#define SDLK_a 'a'

#define SDL_SWSURFACE 0
#define SDL_HWSURFACE 0
#define SDL_NOFRAME 0
#define SDL_ANYFORMAT 0
#define SDL_FULLSCREEN 0
#define SDL_DOUBLEBUF 0
#define SDL_TRIPLEBUF 0

typedef struct {
    uint16_t w, h;
    void *pixels;
    struct {
        uint8_t Rshift, Gshift, Bshift;
        uint32_t Rmask, Gmask, Bmask;
    } *format;
} SDL_Surface;

typedef uint32_t Uint32;
typedef uint8_t Uint8;

static inline int SDL_Init(uint32_t flags) { return 0; }
static inline void SDL_Quit() {}
static inline char* SDL_GetError() { return ""; }
static inline uint32_t SDL_GetTicks() { return 0; }
static inline void SDL_Delay(uint32_t ms) {}
static inline int SDL_PollEvent(SDL_Event *event) { return 0; }

static inline SDL_Joystick* SDL_JoystickOpen(int i) { return (SDL_Joystick*)1; }
static inline void SDL_JoystickClose(SDL_Joystick* s) {}
static inline int SDL_NumJoysticks() { return 0; }

#define AUDIO_S16SYS 0
#define AUDIO_S16 0
#define SDL_MIX_MAXVOLUME 128

typedef void (*SDL_AudioCallback)(void *userdata, uint8_t *stream, int len);
typedef struct {
    int freq;
    uint16_t format;
    uint8_t channels;
    uint16_t samples;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

typedef struct {
    int needed;
    uint16_t src_format;
    uint16_t dst_format;
    double len_ratio;
    double len_mult;
    double len_cvt;
    int len;
    uint8_t *buf;
} SDL_AudioCVT;

static inline int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) { return 0; }
static inline void SDL_PauseAudio(int pause_on) {}
static inline void SDL_CloseAudio() {}
static inline void SDL_LockAudio() {}
static inline void SDL_UnlockAudio() {}
static inline int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, uint16_t src_fmt, uint8_t src_ch, int src_rate, uint16_t dst_fmt, uint8_t dst_ch, int dst_rate) { return 0; }
static inline int SDL_ConvertAudio(SDL_AudioCVT *cvt) { return 0; }
static inline void SDL_FreeWAV(uint8_t *data) {}
static inline void* SDL_LoadWAV(const char *file, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len) { return NULL; }
static inline void SDL_MixAudio(uint8_t *dst, const uint8_t *src, uint32_t len, int volume) {}

static inline void SDL_FreeSurface(SDL_Surface* s) {}
static inline int SDL_ShowCursor(int i) { return 0; }
static inline void SDL_SetCursor(void* c) {}
static inline void SDL_Flip(SDL_Surface* s) {}
static inline int SDL_MUSTLOCK(SDL_Surface* s) { return 0; }
static inline int SDL_LockSurface(SDL_Surface* s) { return 0; }
static inline void SDL_UnlockSurface(SDL_Surface* s) {}

static inline uint32_t SDL_MapRGB(void* format, uint8_t r, uint8_t g, uint8_t b) { return 0; }

// Dummy I_CAMD functions
static inline void I_CAMD_StopSong(void) {}
static inline void I_CAMD_PlaySong(char* filename) {}
static inline void I_CAMD_ShutdownMusic(void) {}
static inline uint8_t I_CAMD_InitMusic(void) { return 1; }
static inline void I_CAMD_SetMusicVolume(int volume) {}
static inline void I_CAMD_PauseSong(void) {}
static inline void I_CAMD_ResumeSong(void) {}

#endif
