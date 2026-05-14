#include "rt_def.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "dsl.h"
#include "oplplayer.h"

// Hack to avoid collision with ESP-IDF's internal assert usage
#ifdef ESP_PLATFORM
#undef assert
#define assert(x) ((void)0)
#endif

#include <rg_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef ESP_PLATFORM
#undef assert
#include <assert.h>
#endif

extern volatile int MV_MixPage;
extern char *MV_MixBuffer[];

static int DSL_ErrorCode = DSL_Ok;
static int mixer_initialized;

static void ( *_CallBackFunc )( void );
static volatile char *_BufferStart;
static int _BufferSize;
static int _NumDivisions;
static int _SampleRate;
static bool stop_task = false;

static rg_audio_sample_t *mix_buffer = NULL;
static int mix_buffer_capacity = 0;

static SemaphoreHandle_t audio_mutex = NULL;

char *DSL_ErrorString( int ErrorNumber )
{
    switch (ErrorNumber) {
        case DSL_Ok: return "Retro-Go Audio Driver ok.";
        case DSL_SDLInitFailure: return "Audio initialization failed.";
        default: return "Unknown Audio Driver error.";
    }
}

static void dsl_task(void *arg)
{
    while (!stop_task)
    {
        if (audio_mutex && xSemaphoreTakeRecursive(audio_mutex, 0) == pdTRUE) {
            // 1. Mixer mixes into MV_MixBuffer[MV_MixPage]
            _CallBackFunc();
            xSemaphoreGiveRecursive(audio_mutex);
        } else {
            // Game task is currently modifying audio state, yield and skip this cycle
            vTaskDelay(1);
            continue;
        }

        // 2. Read the page that was JUST mixed. 
        // It's 8-bit UNSIGNED mono (128 is silence).
        uint8_t *fxptr = (uint8_t *)MV_MixBuffer[MV_MixPage];
        
        if (fxptr == NULL) {
            vTaskDelay(1);
            continue;
        }

        // 3. Render music into mix_buffer (Stereo 16-bit interleaved)
        memset(mix_buffer, 0, _BufferSize * sizeof(rg_audio_sample_t));
        opl_synth_player.render((void *)mix_buffer, _BufferSize);

        // 4. Mix sound effects and convert to Retro-Go stereo signed 16-bit format.
        for (int i = 0; i < _BufferSize; i++)
        {
            // Center at 128, then scale up to 16-bit
            int32_t sample = (int32_t)((int)fxptr[i] - 128) << 8;
            int32_t mixed_l = mix_buffer[i].left + sample;
            int32_t mixed_r = mix_buffer[i].right + sample;

            // Clamp to 16-bit limits
            if (mixed_l > 32767) mixed_l = 32767; else if (mixed_l < -32768) mixed_l = -32768;
            if (mixed_r > 32767) mixed_r = 32767; else if (mixed_r < -32768) mixed_r = -32768;

            mix_buffer[i].left = (int16_t)mixed_l;
            mix_buffer[i].right = (int16_t)mixed_r;
        }

        // 5. Submit to Retro-Go. Blocks if ringbuffer is full.
        rg_audio_submit(mix_buffer, _BufferSize);

        // Yield slightly
        vTaskDelay(1);
    }
    mixer_initialized = 0;
    vTaskDelete(NULL);
}

int DSL_Init( void )
{
    return DSL_Ok;
}

void DSL_Shutdown( void )
{
    DSL_StopPlayback();
}

int DSL_BeginBufferedPlayback( char *BufferStart,
      int BufferSize, int NumDivisions, unsigned SampleRate,
      int MixMode, void ( *CallBackFunc )( void ) )
{
    if (mixer_initialized) return DSL_Error;

    if (audio_mutex == NULL) {
        audio_mutex = xSemaphoreCreateRecursiveMutex();
    }

    _CallBackFunc = CallBackFunc;
    _BufferStart = BufferStart;
    _BufferSize = (BufferSize / NumDivisions);
    _NumDivisions = NumDivisions;
    _SampleRate = SampleRate;

    mix_buffer_capacity = _BufferSize;
    mix_buffer = rg_alloc(mix_buffer_capacity * sizeof(rg_audio_sample_t), MEM_SLOW);

    stop_task = false;
    mixer_initialized = 1;

    // Run audio on core 0 with high priority (15)
    xTaskCreatePinnedToCore(dsl_task, "dsl_audio", 8192, NULL, 15, NULL, 0);

    return DSL_Ok;
}

void DSL_StopPlayback( void )
{
    if (mixer_initialized) {
        stop_task = true;
    }
}

unsigned DSL_GetPlaybackRate( void )
{
    return _SampleRate;
}

unsigned long DisableInterrupts( void )
{
    if (audio_mutex) xSemaphoreTakeRecursive(audio_mutex, portMAX_DELAY);
    return 0;
}

void RestoreInterrupts( unsigned long flags )
{
    if (audio_mutex) xSemaphoreGiveRecursive(audio_mutex);
    (void)flags;
}
