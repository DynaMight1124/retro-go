#include "SDL_system.h"
#include <rg_system.h>
#include <rg_audio.h>
#include <stdlib.h>
#include <string.h>

void SDL_ClearError(void)
{
}

static int64_t frame_wait_time = 0;

void SDL_Delay(Uint32 ms)
{
    int64_t t0 = rg_system_timer();
    rg_usleep((uint64_t)ms * 1000);
    frame_wait_time += rg_system_timer() - t0;
}

int64_t SDL_GetAndResetWaitTime(void)
{
    int64_t wait = frame_wait_time;
    frame_wait_time = 0;
    return wait;
}

char *SDL_GetError(void)
{
    return (char *)"";
}

Uint32 SDL_GetTicks(void)
{
    return (Uint32)(rg_system_timer() / 1000);
}

int SDL_Init(Uint32 flags)
{
    if (flags & SDL_INIT_VIDEO)
        SDL_InitSubSystem(flags);
    return 0;
}

void SDL_Quit(void)
{
    rg_audio_set_mute(true);
    rg_system_exit();
}

void SDL_InitSD(void)
{
}

const SDL_version *SDL_Linked_Version(void)
{
    static SDL_version vers = {SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL};
    return &vers;
}

void SDL_DestroyMutex(SDL_mutex *mutex)
{
}

SDL_mutex *SDL_CreateMutex(void)
{
    return NULL;
}

int SDL_LockMutex(SDL_mutex *mutex)
{
    return 0;
}

int SDL_UnlockMutex(SDL_mutex *mutex)
{
    return 0;
}
