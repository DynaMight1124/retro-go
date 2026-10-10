#include "adapter.h"
#include "music_esp32.h"
#include "qcommon/qcommon.h"
#include "client/snd_loc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE (32 * 1024)
#define CHUNK_FRAMES 512
#define CHUNK_SIZE (CHUNK_FRAMES * sizeof(rg_audio_frame_t))
int snd_inited;
static unsigned char *ring;
static rg_audio_frame_t *chunk;
static atomic_int position;
static atomic_bool running;
static atomic_bool paused;
static SemaphoreHandle_t paint_mutex, output_mutex, stopped, wake;

static void audio_task(void *arg)
{
    (void)arg;
    while (atomic_load(&running)) {
        xSemaphoreTake(output_mutex, portMAX_DELAY);
        if (atomic_load(&paused) || !atomic_load(&running)) {
            xSemaphoreGive(output_mutex);
            // Block only while paused; active playback is paced by rg_audio_submit.
            if (atomic_load(&running)) xSemaphoreTake(wake, portMAX_DELAY);
            continue;
        }
        xSemaphoreTakeRecursive(paint_mutex, portMAX_DELAY);
        int offset = atomic_load(&position);
        memcpy(chunk, ring + offset, CHUNK_SIZE);
        memset(ring + offset, 0, CHUNK_SIZE);
        atomic_store(&position, (offset + CHUNK_SIZE) % BUFFER_SIZE);
        xSemaphoreGiveRecursive(paint_mutex);
        Music_Mix(chunk, CHUNK_FRAMES);
        // Port output trim: -6 dB before Retro-Go applies the user's volume.
        // Keep engine mixing and archived s_volume settings unchanged.
        for (unsigned i = 0; i < CHUNK_FRAMES; ++i) {
            chunk[i].left /= 2;
            chunk[i].right /= 2;
        }
        rg_audio_submit(chunk, CHUNK_FRAMES);
        xSemaphoreGive(output_mutex);
    }
    xSemaphoreGive(stopped);
}

void q2_audio_pause(bool value)
{
    Music_MenuPause(value);
    if (!snd_inited) return;
    // Publish first so the higher-priority consumer cannot keep reacquiring
    // output_mutex while main waits to open a menu. Drain only the current chunk.
    atomic_store(&paused, value);
    if (value) {
        xSemaphoreTake(output_mutex, portMAX_DELAY);
        xSemaphoreGive(output_mutex);
    }
    if (!value) xSemaphoreGive(wake);
}

qboolean SNDDMA_Init(void)
{
    if (snd_inited) return true;
    paint_mutex = xSemaphoreCreateRecursiveMutex();
    output_mutex = xSemaphoreCreateMutex();
    stopped = xSemaphoreCreateBinary();
    wake = xSemaphoreCreateBinary();
    RG_ASSERT(paint_mutex && output_mutex && stopped && wake, "Audio mutex allocation failed");
    ring = rg_alloc(BUFFER_SIZE, MEM_SLOW);
    chunk = rg_alloc(CHUNK_SIZE, MEM_FAST);
    atomic_store(&position, 0);
    atomic_store(&paused, false);
    dma.samplebits = 16;
    dma.speed = Q2_RATE;
    dma.channels = 2;
    dma.samples = BUFFER_SIZE / sizeof(int16_t);
    dma.samplepos = 0;
    dma.submission_chunk = CHUNK_FRAMES;
    dma.buffer = ring;
    snd_inited = 1;
    atomic_store(&running, true);
    RG_ASSERT(rg_task_create("q2_audio", audio_task, NULL, 4096, 0,
                            RG_TASK_PRIORITY_2, 1), "Audio task allocation failed");
    Com_Printf("SNDDMA_Init: Retro-Go %d Hz S16 stereo\n", Q2_RATE);
    return true;
}

int SNDDMA_GetDMAPos(void) { return snd_inited ? atomic_load(&position) / sizeof(int16_t) : 0; }
void SNDDMA_BeginPainting(void) { if (snd_inited) xSemaphoreTakeRecursive(paint_mutex, portMAX_DELAY); }
void SNDDMA_Submit(void) { if (snd_inited) xSemaphoreGiveRecursive(paint_mutex); }

void SNDDMA_Shutdown(void)
{
    if (!snd_inited) return;
    atomic_store(&running, false);
    xSemaphoreGive(wake);
    xSemaphoreTake(stopped, portMAX_DELAY);
    snd_inited = 0;
    dma.buffer = NULL;
    vSemaphoreDelete(paint_mutex);
    vSemaphoreDelete(output_mutex);
    vSemaphoreDelete(stopped);
    vSemaphoreDelete(wake);
    free(ring); ring = NULL;
    free(chunk); chunk = NULL;
}
