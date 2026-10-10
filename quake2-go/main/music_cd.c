/* Quake II CD-track requests backed by optional loose PCM WAV files.
 * Streaming/mixing is adapted from this repository's Quake-Go implementation. */
#include "adapter.h"
#include "client/client.h"
#include "music_pcm.h"
#include "music_esp32.h"
#include <stdlib.h>
#include <string.h>

static cvar_t *cd_nocd, *music_volume;

void CDAudio_Play(int track, qboolean looping)
{
    char path[RG_PATH_MAX + 1];
    const rg_app_t *app = rg_system_get_app();
    if (track < 2 || track > 255 ||
        !music_track_path(path, sizeof(path), RG_BASE_PATH_ROMS,
                          app ? app->romPath : NULL, (unsigned)track)) {
        Music_Stop();
        return;
    }
    Music_Play(path, (unsigned)track, looping != 0);
}

void CDAudio_Stop(void) { Music_Stop(); }
void CDAudio_Resume(void) { Music_Pause(false); }
void CDAudio_Activate(qboolean active) { Music_Pause(!active); }

void CDAudio_Update(void)
{
    if (!cd_nocd || !music_volume) return;
    Music_SetEnabled(cd_nocd->value == 0);
    Music_SetVolume(music_volume->value);
    Music_Pause(cl_paused && cl_paused->value != 0);
}

static void CD_f(void)
{
    if (Cmd_Argc() == 3 && (!strcmp(Cmd_Argv(1), "play") || !strcmp(Cmd_Argv(1), "loop"))) {
        CDAudio_Play(atoi(Cmd_Argv(2)), !strcmp(Cmd_Argv(1), "loop"));
    } else if (Cmd_Argc() == 2 && !strcmp(Cmd_Argv(1), "stop")) {
        CDAudio_Stop();
    } else {
        Com_Printf("cd play/loop <track>, cd stop\n");
    }
}

int CDAudio_Init(void)
{
    cd_nocd = Cvar_Get("cd_nocd", "0", CVAR_ARCHIVE);
    music_volume = Cvar_Get("s_musicvolume", "0.5", CVAR_ARCHIVE);
    Cmd_AddCommand("cd", CD_f);
    Music_SetEnabled(cd_nocd->value == 0);
    bool ready = Music_Init();
    CDAudio_Update();
    return ready ? 0 : -1;
}

void CDAudio_Shutdown(void)
{
    Music_Shutdown();
    Cmd_RemoveCommand("cd");
}
