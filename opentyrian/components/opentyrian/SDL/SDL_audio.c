#include "SDL_audio.h"
#include <rg_system.h>
#include <rg_audio.h>
#include <string.h>

#define AUDIO_OUTPUT_FRAMES 512
#define AUDIO_SOURCE_FRAMES (AUDIO_OUTPUT_FRAMES / 2)

static SDL_AudioSpec active_as;
static volatile bool audio_running = false;
static volatile bool audio_task_done = false;
static volatile bool audio_paused = true;
static volatile bool audio_in_submit = false;
static SemaphoreHandle_t audio_mutex = NULL;

static int16_t mono_buffer[AUDIO_SOURCE_FRAMES];
static rg_audio_frame_t stereo_buffer[AUDIO_OUTPUT_FRAMES];

static void audio_update_task(void *arg)
{
    while (audio_running)
    {
        if (audio_paused || rg_audio_get_mute() || !active_as.callback)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        memset(mono_buffer, 0, sizeof(mono_buffer));
        (*active_as.callback)(active_as.userdata, (Uint8 *)mono_buffer, sizeof(mono_buffer));

        // OpenTyrian's native 11,025 Hz mix is sufficient for its original
        // assets. Duplicate the completed mix for Retro-Go's 22,050 Hz sink
        // instead of running every SFX channel and the OPL synth twice.
        for (int i = 0; i < AUDIO_SOURCE_FRAMES; i++)
        {
            rg_audio_frame_t frame = { mono_buffer[i], mono_buffer[i] };
            stereo_buffer[i * 2] = frame;
            stereo_buffer[i * 2 + 1] = frame;
        }

        if (audio_running && !audio_paused && !rg_audio_get_mute())
        {
            audio_in_submit = true;
            rg_audio_submit(stereo_buffer, AUDIO_OUTPUT_FRAMES);
            audio_in_submit = false;
        }
    }

    audio_task_done = true;
    return;
}

void SDL_AudioInit(void)
{
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
    if (obtained)
    {
        *obtained = *desired;
        obtained->freq = TYRIAN_SAMPLERATE;
        obtained->format = AUDIO_S16SYS;
        obtained->channels = 1;
        obtained->samples = AUDIO_SOURCE_FRAMES;
    }

    active_as = *desired;
    active_as.freq = TYRIAN_SAMPLERATE;
    active_as.format = AUDIO_S16SYS;
    active_as.samples = AUDIO_SOURCE_FRAMES;

    if (!audio_mutex)
        audio_mutex = xSemaphoreCreateMutex();

    rg_audio_set_mute(false);

    audio_running = true;
    audio_task_done = false;
    audio_paused = true;

    rg_task_create("audio_task", audio_update_task, NULL, 16 * 1024, 1, RG_TASK_PRIORITY_2, 1);
    return 0;
}

void SDL_PauseAudio(int pause_on)
{
    audio_paused = pause_on ? true : false;
    if (pause_on)
    {
        while (audio_in_submit)
        {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

void SDL_CloseAudio(void)
{
    rg_audio_set_mute(true);
    if (!audio_running)
        return;

    audio_running = false;
    audio_paused = true;

    for (int i = 0; i < 50 && !audio_task_done; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (audio_mutex)
    {
        vSemaphoreDelete(audio_mutex);
        audio_mutex = NULL;
    }
}

int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, Uint16 src_format, Uint8 src_channels, int src_rate, Uint16 dst_format, Uint8 dst_channels, int dst_rate)
{
    if (cvt)
        cvt->len_mult = 1;
    return 0;
}

int SDL_ConvertAudio(SDL_AudioCVT *cvt)
{
    return 0;
}

void SDL_LockAudio(void)
{
    if (audio_mutex)
        xSemaphoreTake(audio_mutex, portMAX_DELAY);
}

void SDL_UnlockAudio(void)
{
    if (audio_mutex)
        xSemaphoreGive(audio_mutex);
}
