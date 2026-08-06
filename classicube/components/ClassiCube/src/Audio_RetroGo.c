#include "Core.h"
#if defined CC_BUILD_RETROGO

#include <stdint.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define AUDIO_CONTEXT_DEFINED
struct AudioContext {
    int sampleRate, channels;
    int volume;
    int count;

    // Asynchronous playback state for the background mixer
    const int16_t* data;
    int numSamples;
    float playPos;
    float step;
    volatile int active;
};

#include "Audio.h"
#include "Platform.h"
#include "Funcs.h"
#include "Errors.h"
#include <rg_system.h>
#include <string.h>

// Forward declarations required by _AudioBase.h
cc_bool Audio_DescribeError(cc_result res, cc_string* dst);
cc_result Audio_Init(struct AudioContext* ctx, int buffers);
void Audio_Close(struct AudioContext* ctx);
void Audio_SetVolume(struct AudioContext* ctx, int volume);
cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data);
cc_result SoundContext_PollBusy(struct AudioContext* ctx, cc_bool* isBusy);
cc_bool SoundContext_FastPlay(struct AudioContext* ctx, struct AudioData* data);
cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks);
void Audio_FreeChunks(struct AudioChunk* chunks, int numChunks);

#define AUDIO_OVERRIDE_ALLOC
#include "_AudioBase.h"

#define MIXER_RATE 44100
#define MIXER_BUF_SAMPLES 512

static rg_task_t* mixer_task;

static void audio_mixer_task(void* arg) {
    rg_audio_frame_t* out_buf = (rg_audio_frame_t*)rg_alloc(MIXER_BUF_SAMPLES * sizeof(rg_audio_frame_t), MEM_FAST);
    rg_task_msg_t msg;
    cc_bool output_active = false;
    if (!out_buf) return;

    rg_audio_set_sample_rate(MIXER_RATE);

    while (1) {
        if (rg_task_receive(&msg, 0) && msg.type == RG_TASK_MSG_STOP) break;

        memset(out_buf, 0, MIXER_BUF_SAMPLES * sizeof(rg_audio_frame_t));
        int any_active = 0;

        for (int s = 0; s < MIXER_BUF_SAMPLES; s++) {
            int mixed_sample = 0;

            // Mix all active contexts in context_pool
            for (int i = 0; i < 8; i++) { // POOL_MAX_CONTEXTS is 8
                struct AudioContext* ctx = &context_pool[i];
                if (ctx->active && ctx->data) {
                    any_active = 1;
                    int idx = (int)ctx->playPos;
                    if (idx < ctx->numSamples) {
                        int sample = 0;
                        if (ctx->channels == 1) {
                            sample = ctx->data[idx];
                        } else {
                            sample = (ctx->data[idx * 2] + ctx->data[idx * 2 + 1]) / 2;
                        }
                        mixed_sample += (sample * ctx->volume) / 100;
                        ctx->playPos += ctx->step;
                    } else {
                        ctx->active = 0;
                    }
                }
            }

            // Mix music context (just in case)
            struct AudioContext* m_ctx = &music_ctx;
            if (m_ctx->active && m_ctx->data) {
                any_active = 1;
                int idx = (int)m_ctx->playPos;
                if (idx < m_ctx->numSamples) {
                    int sample = 0;
                    if (m_ctx->channels == 1) {
                        sample = m_ctx->data[idx];
                    } else {
                        sample = (m_ctx->data[idx * 2] + m_ctx->data[idx * 2 + 1]) / 2;
                    }
                    mixed_sample += (sample * m_ctx->volume) / 100;
                    m_ctx->playPos += m_ctx->step;
                } else {
                    m_ctx->active = 0;
                }
            }

            // Clip mixed_sample to standard 16-bit signed range
            if (mixed_sample > 32767) mixed_sample = 32767;
            else if (mixed_sample < -32768) mixed_sample = -32768;

            out_buf[s].left = mixed_sample;
            out_buf[s].right = mixed_sample;
        }

        if (any_active) {
            rg_audio_submit(out_buf, MIXER_BUF_SAMPLES);
            output_active = true;
        } else {
            // Ensure the I2S DMA path cannot retain the tail of the last audible buffer.
            if (output_active) {
                rg_audio_submit(out_buf, MIXER_BUF_SAMPLES);
                output_active = false;
            }
            // Block on the task queue so shutdown does not have to wait for an idle delay.
            if (rg_task_receive(&msg, 10) && msg.type == RG_TASK_MSG_STOP) break;
        }
    }

    free(out_buf);
}

cc_bool AudioBackend_Init(void) {
    if (mixer_task && rg_task_find("CCAudioMixer")) return true;

    mixer_task = rg_task_create("CCAudioMixer", audio_mixer_task, NULL, 4096, 1, RG_TASK_PRIORITY_5, -1);
    return mixer_task != NULL;
}

void AudioBackend_Free(void) {
    rg_task_msg_t msg = { .type = RG_TASK_MSG_STOP };
    if (!mixer_task) return;

    rg_task_send(mixer_task, &msg, -1);
    while (rg_task_find("CCAudioMixer")) rg_task_delay(1);
    mixer_task = NULL;
}
void    AudioBackend_Tick(void) { }

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
    memset(ctx, 0, sizeof(struct AudioContext));
    ctx->volume = 100;
    ctx->count = buffers;
    return 0;
}

void Audio_Close(struct AudioContext* ctx) {
    ctx->active = 0;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
    ctx->volume = volume;
}

cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks) {
    for (int i = 0; i < numChunks; i++) {
        chunks[i].data = Mem_Alloc(1, size, "audio chunks");
        chunks[i].size = size;
    }
    return 0;
}

void Audio_FreeChunks(struct AudioChunk* chunks, int numChunks) {
    for (int i = 0; i < numChunks; i++) {
        Mem_Free(chunks[i].data);
    }
}

cc_bool Audio_DescribeError(cc_result res, cc_string* dst) { return false; }

cc_result StreamContext_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
    ctx->channels   = channels;
    ctx->sampleRate = sampleRate;
    return 0;
}

cc_result StreamContext_Enqueue(struct AudioContext* ctx, struct AudioChunk* chunk) {
    ctx->data = (const int16_t*)chunk->data;
    ctx->channels = ctx->channels ? ctx->channels : 1;
    ctx->sampleRate = ctx->sampleRate ? ctx->sampleRate : 22050;
    ctx->numSamples = chunk->size / (ctx->channels * 2);
    ctx->playPos = 0.0f;
    ctx->step = (float)ctx->sampleRate / (float)MIXER_RATE;
    ctx->active = 1;
    return 0;
}

cc_result StreamContext_Play(struct AudioContext* ctx)  { return 0; }
cc_result StreamContext_Pause(struct AudioContext* ctx) { ctx->active = 0; return 0; }
cc_result StreamContext_Update(struct AudioContext* ctx, int* inUse) {
    *inUse = ctx->active;
    return 0;
}

cc_bool SoundContext_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
    return true;
}

cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data) {
    ctx->data = (const int16_t*)data->chunk.data;
    ctx->channels = data->channels;
    ctx->sampleRate = data->sampleRate;
    ctx->numSamples = data->chunk.size / (data->channels * 2);
    ctx->playPos = 0.0f;
    ctx->step = (float)data->sampleRate / (float)MIXER_RATE;
    ctx->active = 1;
    return 0;
}

cc_result SoundContext_PollBusy(struct AudioContext* ctx, cc_bool* isBusy) {
    *isBusy = ctx->active;
    return 0;
}

/* Static helper stubs required by the shared sound pool manager (_AudioBase.h) */
static cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) __attribute__((unused));
static cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) __attribute__((unused));
static cc_result Audio_Play(struct AudioContext* ctx) __attribute__((unused));
static cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) __attribute__((unused));

static cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) { return 0; }
static cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) { return 0; }
static cc_result Audio_Play(struct AudioContext* ctx) { return 0; }
static cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) { *inUse = 0; return 0; }

#endif
