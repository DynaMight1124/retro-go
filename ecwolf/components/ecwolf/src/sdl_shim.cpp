#include "sdl_shim.h"
#include <rg_system.h>
#include <rg_input.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern "C" {

char *realpath(const char *path, char *resolved_path) {
    if (resolved_path == NULL) resolved_path = (char *)malloc(260);
    strncpy(resolved_path, path, 260);
    return resolved_path;
}

FILE *popen(const char *command, const char *type) { return NULL; }
int pclose(FILE *stream) { return -1; }

void SDL_GetVersion(SDL_version *ver) {
    if (ver) {
        ver->major = 2;
        ver->minor = 0;
        ver->patch = 12;
    }
}

int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, Uint16 src_format, Uint8 src_channels, int src_rate, Uint16 dst_format, Uint8 dst_channels, int dst_rate) {
    memset(cvt, 0, sizeof(SDL_AudioCVT));
    if (src_channels == 1 && dst_channels == 2) {
        cvt->needed = 1;
        cvt->src_format = src_format;
        cvt->dst_format = dst_format;
        cvt->len_mult = 2;
        cvt->len_ratio = 1.0;
        return 1;
    }
    return 0;
}

int SDL_ConvertAudio(SDL_AudioCVT *cvt) {
    if (cvt->needed && cvt->len_mult == 2) {
        // Mono to Stereo 16-bit
        int16_t *data = (int16_t *)cvt->buf;
        int count = cvt->len / 2; // Original length in 16-bit samples
        // Traverse backwards to avoid overwriting during in-place expansion
        for (int i = count - 1; i >= 0; i--) {
            data[i*2] = data[i];     // Left
            data[i*2+1] = data[i];   // Right
        }
        cvt->len *= 2;
    }
    return 0;
}

void SDL_MixAudioFormat(Uint8 *dst, const Uint8 *src, Uint16 format, Uint32 len, int volume) {
    if (volume == 0) return;
    int16_t *d = (int16_t *)dst;
    const int16_t *s = (const int16_t *)src;
    uint32_t count = len / 2;
    for (uint32_t i = 0; i < count; i++) {
        int32_t mixed = d[i] + (s[i] * volume / SDL_MIX_MAXVOLUME);
        if (mixed > 32767) mixed = 32767;
        else if (mixed < -32768) mixed = -32768;
        d[i] = (int16_t)mixed;
    }
}

void SDL_MixAudio(Uint8 *dst, const Uint8 *src, Uint32 len, int volume) {
    SDL_MixAudioFormat(dst, src, AUDIO_S16, len, volume);
}

int SDL_Init(Uint32 flags) { return 0; }
int SDL_InitSubSystem(Uint32 flags) { return 0; }
void SDL_Quit(void) {}
void SDL_QuitSubSystem(Uint32 flags) {}

Uint32 SDL_GetTicks(void) {
    return rg_system_timer() / 1000;
}

static void rg_idle_tick(void) {
    static int64_t last_tick = 0;
    const int64_t now = rg_system_timer();
    // Match Retro-Go GUI polling cadence and avoid turning 5 ms menu sleeps
    // into an artificial 200 FPS stream.
    if (now - last_tick >= 20000) {
        rg_system_tick(0);
        last_tick = now;
    }
}

void SDL_Delay(Uint32 ms) {
    rg_usleep(ms * 1000);
    rg_idle_tick();
}

const char *SDL_GetError(void) {
    return "Unknown error";
}

#define SDLK_UP 0x40000052
#define SDLK_DOWN 0x40000051
#define SDLK_LEFT 0x40000050
#define SDLK_RIGHT 0x4000004f
#define SDLK_LCTRL 0x400000e0
#define SDLK_LALT 0x400000e2
#define SDLK_SPACE ' '
#define SDLK_y 'y'
#define SDLK_COMMA ','
#define SDLK_PERIOD '.'
#define SDLK_LSHIFT 0x400000e1
#define SDLK_ESCAPE 0x0000001b
#define SDLK_BACKSPACE 0x00000008
#define SDLK_TAB 0x00000009

static struct {
    uint32_t rg_key;
    SDL_Scancode sdl_scancode;
    int sdl_keycode;
} key_map[] = {
    {RG_KEY_UP, SDL_SCANCODE_UP, SDLK_UP},
    {RG_KEY_DOWN, SDL_SCANCODE_DOWN, SDLK_DOWN},
    {RG_KEY_LEFT, SDL_SCANCODE_LEFT, SDLK_LEFT},
    {RG_KEY_RIGHT, SDL_SCANCODE_RIGHT, SDLK_RIGHT},
    {RG_KEY_A, SDL_SCANCODE_LCTRL, SDLK_LCTRL},   // Fire
    {RG_KEY_B, SDL_SCANCODE_LALT, SDLK_LALT},     // Strafe
    {RG_KEY_X, SDL_SCANCODE_TAB, SDLK_TAB},       // Map (Automap)
    {RG_KEY_Y, SDL_SCANCODE_BACKSPACE, SDLK_BACKSPACE}, // Weapon Cycle (Next)
    {RG_KEY_L, SDL_SCANCODE_COMMA, SDLK_COMMA},   // Strafe left
    {RG_KEY_R, SDL_SCANCODE_PERIOD, SDLK_PERIOD}, // Strafe right
    {RG_KEY_START, SDL_SCANCODE_SPACE, SDLK_SPACE}, // Open/Use
    {RG_KEY_SELECT, SDL_SCANCODE_BACKSPACE, SDLK_BACKSPACE}, // Weapon Cycle (Next)
    {RG_KEY_OPTION, SDL_SCANCODE_LSHIFT, SDLK_LSHIFT}, // Run
    {RG_KEY_MENU, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE}, // Menu
};

static uint32_t last_gamepad = 0;

extern "C" void Quit();

int SDL_PollEvent(SDL_Event *event) {
    rg_task_msg_t msg;
    if (rg_task_receive(&msg, 0)) {
        if (msg.type == RG_TASK_MSG_STOP) {
            Quit();
            return 0;
        }
    }

    uint32_t gamepad = rg_input_read_gamepad();
    uint32_t changed = gamepad ^ last_gamepad;

    if (changed) {
        for (size_t i = 0; i < sizeof(key_map)/sizeof(key_map[0]); i++) {
            if (changed & key_map[i].rg_key) {
                event->type = (gamepad & key_map[i].rg_key) ? SDL_KEYDOWN : SDL_KEYUP;
                event->key.type = event->type;
                event->key.keysym.scancode = key_map[i].sdl_scancode;
                event->key.keysym.sym = key_map[i].sdl_keycode;
                last_gamepad ^= key_map[i].rg_key;
                return 1;
            }
        }
    }
    return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
    while (!SDL_PollEvent(event)) {
        rg_usleep(10000);
        rg_idle_tick();
    }
    return 1;
}

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags) { return (SDL_Window *)1; }
void SDL_DestroyWindow(SDL_Window *window) {}
SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags) { return (SDL_Renderer *)1; }
void SDL_DestroyRenderer(SDL_Renderer *renderer) {}
SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h) { return (SDL_Texture *)1; }
void SDL_DestroyTexture(SDL_Texture *texture) {}
int SDL_LockTexture(SDL_Texture *texture, const SDL_Rect *rect, void **pixels, int *pitch) { return 0; }
void SDL_UnlockTexture(SDL_Texture *texture) {}
int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch) { return 0; }
int SDL_LockSurface(SDL_Surface *surface) { return 0; }
void SDL_UnlockSurface(SDL_Surface *surface) {}

Uint32 SDL_GetWindowFlags(SDL_Window *window) { return SDL_WINDOW_FULLSCREEN_DESKTOP; }
void SDL_SetWindowSize(SDL_Window *window, int w, int h) {}
void SDL_GetWindowSize(SDL_Window *window, int *w, int *h) { if(w) *w = 320; if(h) *h = 240; }
SDL_Surface *SDL_GetWindowSurface(SDL_Window *window) { return NULL; }
int SDL_UpdateWindowSurface(SDL_Window *window) { return 0; }
void SDL_SetWindowFullscreen(SDL_Window *window, Uint32 flags) {}

int SDL_RenderClear(SDL_Renderer *renderer) { return 0; }
int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect) { return 0; }
void SDL_RenderPresent(SDL_Renderer *renderer) {}
int SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a) { return 0; }
int SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h) { return 0; }

int SDL_QueryTexture(SDL_Texture *texture, Uint32 *format, int *access, int *w, int *h) {
    if(format) *format = SDL_PIXELFORMAT_ARGB8888;
    if(w) *w = 320;
    if(h) *h = 240;
    return 0;
}
void SDL_PixelFormatEnumToMasks(Uint32 format, int *bpp, Uint32 *Rmask, Uint32 *Gmask, Uint32 *Bmask, Uint32 *Amask) {
    if(bpp) *bpp = 32;
}
int SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors) { return 0; }

void SDL_SetRelativeMouseMode(SDL_bool enabled) {}
Uint32 SDL_GetMouseState(int *x, int *y) { if(x) *x = 0; if(y) *y = 0; return 0; }
Uint32 SDL_GetRelativeMouseState(int *x, int *y) { if(x) *x = 0; if(y) *y = 0; return 0; }
void SDL_EventState(Uint32 type, int state) {}
SDL_Keymod SDL_GetModState(void) { return KMOD_NUM; }
void SDL_SetModState(SDL_Keymod modstate) {}
int SDL_ShowCursor(int toggle) { return 0; }

int SDL_NumJoysticks(void) { return 0; }
SDL_bool SDL_IsGameController(int joystick_index) { return SDL_FALSE; }
SDL_GameController *SDL_GameControllerOpen(int joystick_index) { return NULL; }
void SDL_GameControllerClose(SDL_GameController *gamecontroller) {}
const char *SDL_GameControllerName(SDL_GameController *gamecontroller) { return "None"; }
void SDL_GameControllerUpdate(void) {}
Sint16 SDL_GameControllerGetAxis(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis) { return 0; }
SDL_bool SDL_GameControllerGetButton(SDL_GameController *gamecontroller, SDL_GameControllerButton button) { return SDL_FALSE; }
int SDL_GameControllerEventState(int state) { return 0; }

SDL_Joystick *SDL_JoystickOpen(int joystick_index) { return NULL; }
void SDL_JoystickClose(SDL_Joystick *joystick) {}
void SDL_JoystickUpdate(void) {}
int SDL_JoystickNumButtons(SDL_Joystick *joystick) { return 0; }
int SDL_JoystickNumAxes(SDL_Joystick *joystick) { return 0; }
int SDL_JoystickNumHats(SDL_Joystick *joystick) { return 0; }
Sint16 SDL_JoystickGetAxis(SDL_Joystick *joystick, int axis) { return 0; }
Uint8 SDL_JoystickGetHat(SDL_Joystick *joystick, int hat) { return 0; }
SDL_bool SDL_JoystickGetButton(SDL_Joystick *joystick, int button) { return SDL_FALSE; }

struct MemContext {
    uint8_t *base;
    uint8_t *here;
    uint8_t *stop;
};

static Sint64 mem_seek(SDL_RWops *context, Sint64 offset, int whence) {
    MemContext *mem = (MemContext *)context->hidden.unknown.data1;
    switch (whence) {
        case RW_SEEK_SET: mem->here = mem->base + offset; break;
        case RW_SEEK_CUR: mem->here += offset; break;
        case RW_SEEK_END: mem->here = mem->stop + offset; break;
    }
    if (mem->here < mem->base) mem->here = mem->base;
    if (mem->here > mem->stop) mem->here = mem->stop;
    return mem->here - mem->base;
}

static size_t mem_read(SDL_RWops *context, void *ptr, size_t size, size_t maxnum) {
    MemContext *mem = (MemContext *)context->hidden.unknown.data1;
    size_t total = size * maxnum;
    size_t avail = mem->stop - mem->here;
    if (total > avail) {
        total = avail;
        maxnum = total / size;
        total = maxnum * size;
    }
    memcpy(ptr, mem->here, total);
    mem->here += total;
    return maxnum;
}

static int mem_close(SDL_RWops *context) {
    if (context) {
        if (context->hidden.unknown.data1) {
            free(context->hidden.unknown.data1);
        }
        free(context);
    }
    return 0;
}

SDL_RWops *SDL_RWFromMem(void *mem, int size) {
    SDL_RWops *rwops = (SDL_RWops *)malloc(sizeof(SDL_RWops));
    if (!rwops) return NULL;
    MemContext *ctx = (MemContext *)malloc(sizeof(MemContext));
    if (!ctx) { free(rwops); return NULL; }
    ctx->base = (uint8_t *)mem;
    ctx->here = ctx->base;
    ctx->stop = ctx->base + size;
    rwops->seek = mem_seek;
    rwops->read = mem_read;
    rwops->write = NULL;
    rwops->close = mem_close;
    rwops->hidden.unknown.data1 = ctx;
    return rwops;
}
SDL_RWops *SDL_AllocRW(void) { return (SDL_RWops *)malloc(sizeof(SDL_RWops)); }
void SDL_FreeRW(SDL_RWops *area) { free(area); }

struct SDL_mutex {
    SemaphoreHandle_t handle;
};

SDL_mutex *SDL_CreateMutex(void) {
    SDL_mutex *m = (SDL_mutex *)malloc(sizeof(SDL_mutex));
    m->handle = xSemaphoreCreateRecursiveMutex();
    return m;
}

void SDL_DestroyMutex(SDL_mutex *mutex) {
    vSemaphoreDelete(mutex->handle);
    free(mutex);
}

int SDL_LockMutex(SDL_mutex *mutex) {
    if (!mutex) return 0;
    return xSemaphoreTakeRecursive(mutex->handle, portMAX_DELAY) == pdTRUE ? 0 : -1;
}

int SDL_UnlockMutex(SDL_mutex *mutex) {
    if (!mutex) return 0;
    return xSemaphoreGiveRecursive(mutex->handle) == pdTRUE ? 0 : -1;
}

}
