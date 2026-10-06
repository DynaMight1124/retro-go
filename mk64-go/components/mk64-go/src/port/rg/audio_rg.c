/* Game/sequence producer; separate core-1 DSP and blocking output tasks. */
#include <string.h>
#include <rg_system.h>
#include <rg_audio.h>
#include <rg_utils.h>
#include <esp_heap_caps.h>
#include "audio_rg.h"
#include "menu_timing.h"
#include "port/audio/mix_jobs.h"
#include "audio/data.h"
#include "audio/load.h"
#include "audio/external.h"

extern void audio_init(void);

#define PCM_SLOTS 4
#define PCM_FRAMES (AIBUFFER_LEN / sizeof(rg_audio_frame_t))
enum { AUDIO_JOB = 1, AUDIO_PCM };
typedef struct {
    rg_audio_frame_t *frames;
    unsigned count, owned;
} AudioSlot;
static AudioSlot sSlots[PCM_SLOTS];
static rg_task_t *sWorker, *sOutputWorker;
static unsigned sSlotIndex, sEnabled = 1, sPaused, sInitialized, sStopped;
static unsigned sQueuedFrames, sPlaybackEnd, sRate = MK64_RG_AUDIO_RATE;
static unsigned sMusicPending, sMusicValid, sMusic, sSpecPending, sSpecMode, sSpec;

void mk64_rg_audio_note_sequence(uint16_t sequence) {
    sMusic = sequence; sMusicValid = 1;
    if (!mk64_rg_audio_enabled()) sMusicPending = 1;
}
void mk64_rg_audio_note_spec(uint8_t mode, uint8_t spec) {
    sSpecMode = mode; sSpec = spec;
    if (!mk64_rg_audio_enabled()) sSpecPending = 1;
}

static unsigned sMixJobs, sMixUs, sMixMaxUs, sOutputFrames;
static unsigned sFrameCalls, sFrameUs, sFrameMaxUs;
void mk64_rg_audio_record_queue(void) {
    if (sMk64MenuTiming.enabled)
        mk64_menu_timing_queue(port_audio_out_queued_bytes());
}
void mk64_rg_audio_record_frame(uint32_t elapsed_us) {
    mk64_menu_timing_add(MK64_MENU_AUDIO, elapsed_us, 0);
    ++sFrameCalls; sFrameUs += elapsed_us;
    if (elapsed_us > sFrameMaxUs) sFrameMaxUs = elapsed_us;
}

bool mk64_rg_audio_requested(void) { return __atomic_load_n(&sEnabled, __ATOMIC_ACQUIRE); }
bool mk64_rg_audio_enabled(void) {
    return mk64_rg_audio_requested() && !__atomic_load_n(&sPaused, __ATOMIC_ACQUIRE);
}

static void mix_worker(void *arg)
{
    (void)arg;
    rg_task_msg_t msg;
    while (rg_task_receive(&msg, -1)) {
        if (msg.type == RG_TASK_MSG_STOP) break;
        if (msg.type != AUDIO_JOB) RG_PANIC("MK64 invalid DSP message");
        unsigned id = msg.dataInt;
        uint32_t started = (uint32_t)rg_system_timer();
        mix_job_execute(&gMixJobs[id % MIXJ_JOBS], 0);
        unsigned elapsed = (uint32_t)rg_system_timer() - started;
        __atomic_add_fetch(&sMixUs, elapsed, __ATOMIC_RELAXED);
        if (elapsed > __atomic_load_n(&sMixMaxUs, __ATOMIC_RELAXED))
            __atomic_store_n(&sMixMaxUs, elapsed, __ATOMIC_RELAXED);
        __atomic_add_fetch(&sMixJobs, 1, __ATOMIC_RELAXED);
        __atomic_store_n(&gMixShared.completed, id + 1, __ATOMIC_RELEASE);
    }
    __atomic_or_fetch(&sStopped, 1, __ATOMIC_RELEASE);
}

static void output_worker(void *arg)
{
    (void)arg;
    rg_task_msg_t msg;
    while (rg_task_receive(&msg, -1)) {
        if (msg.type == RG_TASK_MSG_STOP) break;
        if (msg.type != AUDIO_PCM) RG_PANIC("MK64 invalid PCM message");
        AudioSlot *slot = (AudioSlot *)msg.dataPtr;
        unsigned count = slot->count;
        if (mk64_rg_audio_enabled()) {
            uint32_t now = (uint32_t)rg_system_timer();
            uint32_t end = __atomic_load_n(&sPlaybackEnd, __ATOMIC_ACQUIRE);
            if ((int32_t)(end - now) < 0) end = now;
            end += (uint64_t)count * 1000000 / sRate;
            __atomic_store_n(&sPlaybackEnd, end, __ATOMIC_RELEASE);
            __atomic_sub_fetch(&sQueuedFrames, count, __ATOMIC_ACQ_REL);
            /* Blocking driver submission must never delay DSP completion. */
            rg_audio_submit(slot->frames, count);
            __atomic_add_fetch(&sOutputFrames, count, __ATOMIC_RELAXED);
        } else {
            __atomic_sub_fetch(&sQueuedFrames, count, __ATOMIC_ACQ_REL);
        }
        __atomic_store_n(&slot->owned, 0, __ATOMIC_RELEASE);
    }
    __atomic_or_fetch(&sStopped, 2, __ATOMIC_RELEASE);
}

void mk64_rg_mix_wait(unsigned target)
{
    uint32_t started = (uint32_t)rg_system_timer();
    while ((int32_t)(__atomic_load_n(&gMixShared.completed, __ATOMIC_ACQUIRE) - target) < 0)
        rg_task_delay(1);
    mk64_menu_timing_add(MK64_MENU_DSP_WAIT, (uint32_t)rg_system_timer() - started, 0);
}

void mk64_rg_mix_submit(unsigned id)
{
    __atomic_store_n(&gMixShared.submitted, id + 1, __ATOMIC_RELEASE);
    if (!rg_task_send(sWorker, &(rg_task_msg_t){.type = AUDIO_JOB, .dataInt = id}, -1))
        RG_PANIC("MK64 mixer queue failed");
}

static void audio_drain(void)
{
    if (!sWorker) return;
    mk64_rg_mix_wait(__atomic_load_n(&gMixShared.submitted, __ATOMIC_ACQUIRE));
    for (unsigned i = 0; i < PCM_SLOTS; ++i)
        while (__atomic_load_n(&sSlots[i].owned, __ATOMIC_ACQUIRE)) rg_task_delay(1);
}

void port_audio_out_set_rate(uint32_t rate)
{
    if (rate < 8000 || rate > 48000) RG_PANIC("MK64 invalid audio rate");
    audio_drain();
    sRate = rate;
    __atomic_store_n(&sPlaybackEnd, (uint32_t)rg_system_timer(), __ATOMIC_RELEASE);
    rg_audio_set_sample_rate(rate);
    RG_LOGI("MK64 audio rate: %u Hz stereo", (unsigned)rate);
}

void port_audio_out_push(const int16_t *samples, uint32_t bytes)
{
    if (!mk64_rg_audio_enabled() || !sOutputWorker || !bytes) return;
    if (!samples || bytes % sizeof(rg_audio_frame_t) || bytes > AIBUFFER_LEN)
        RG_PANIC("MK64 invalid PCM buffer");
    AudioSlot *slot = &sSlots[sSlotIndex];
    uint32_t started = (uint32_t)rg_system_timer();
    while (__atomic_load_n(&slot->owned, __ATOMIC_ACQUIRE)) rg_task_delay(1);
    mk64_menu_timing_add(MK64_MENU_PCM_WAIT, (uint32_t)rg_system_timer() - started, 0);
    slot->count = bytes / sizeof(rg_audio_frame_t);
    memcpy(slot->frames, samples, bytes);
    __atomic_add_fetch(&sQueuedFrames, slot->count, __ATOMIC_ACQ_REL);
    __atomic_store_n(&slot->owned, 1, __ATOMIC_RELEASE);
    if (!rg_task_send(sOutputWorker, &(rg_task_msg_t){.type = AUDIO_PCM, .dataPtr = slot}, -1))
        RG_PANIC("MK64 PCM queue failed");
    sSlotIndex = (sSlotIndex + 1) % PCM_SLOTS;
}

uint32_t port_audio_out_queued_bytes(void)
{
    if (!mk64_rg_audio_enabled()) return 0;
    int32_t remaining = __atomic_load_n(&sPlaybackEnd, __ATOMIC_ACQUIRE) - (uint32_t)rg_system_timer();
    unsigned frames = __atomic_load_n(&sQueuedFrames, __ATOMIC_ACQUIRE);
    if (remaining > 0) frames += (uint64_t)(unsigned)remaining * sRate / 1000000;
    return frames * sizeof(rg_audio_frame_t);
}

unsigned mk64_rg_audio_target_bytes(void) { return sRate / 20 * sizeof(rg_audio_frame_t); }

void mk64_rg_audio_ensure_init(void)
{
    if (!mk64_rg_audio_enabled()) return;
    if (!sInitialized) {
        rg_audio_frame_t *pcm = rg_alloc(PCM_SLOTS * AIBUFFER_LEN, MEM_FAST);
        if (!pcm) RG_PANIC("MK64 PCM allocation failed");
        for (unsigned i = 0; i < PCM_SLOTS; ++i) sSlots[i].frames = pcm + i * PCM_FRAMES;
        sWorker = rg_task_create("mk64_mix", mix_worker, NULL, 8192, 4, RG_TASK_PRIORITY_2, 1);
        sOutputWorker = rg_task_create("mk64_pcm", output_worker, NULL, 3072, PCM_SLOTS, RG_TASK_PRIORITY_2, 1);
        if (!sWorker || !sOutputWorker) RG_PANIC("MK64 audio worker failed");
        audio_init();
        func_800C5CB8();
        sInitialized = 1;
        RG_LOGI("MK64 sound ready: DSP/output core 1, heap %u, jobs %u, PCM %u bytes",
                (unsigned)gAudioHeapSize, (unsigned)sizeof(gMixJobs), PCM_SLOTS * (unsigned)AIBUFFER_LEN);
    }
    /* Remember just the current music/spec while silent, never sound effects.
     * This also supports a saved Off setting followed by enabling mid-race. */
    if (sSpecPending) {
        sSpecPending = 0;
        func_800CA008(sSpecMode, sSpec);
        if (sMusicValid) sMusicPending = 1;
    }
    if (sMusicPending) { sMusicPending = 0; play_sequence(sMusic); }
}

void mk64_rg_audio_set_enabled(bool enabled)
{
    __atomic_store_n(&sEnabled, enabled, __ATOMIC_RELEASE);
    if (!enabled) {
        audio_drain();
        __atomic_store_n(&sPlaybackEnd, (uint32_t)rg_system_timer(), __ATOMIC_RELEASE);
        if (sInitialized) memset(gAiBufferLengths, 0, NUMAIBUFFERS * sizeof(gAiBufferLengths[0]));
    }
    RG_LOGI("MK64 sound %s: sequence/mixer/output %s", enabled ? "on" : "off", enabled ? "enabled" : "stopped");
}

void mk64_rg_audio_pause(bool paused)
{
    __atomic_store_n(&sPaused, paused, __ATOMIC_RELEASE);
    if (paused) {
        audio_drain();
        __atomic_store_n(&sPlaybackEnd, (uint32_t)rg_system_timer(), __ATOMIC_RELEASE);
    } else {
        rg_audio_set_mute(!mk64_rg_audio_requested());
    }
}

void mk64_rg_audio_report(void)
{
    do { RG_LOGD("MK64 audio game-thread: calls %u total %u us max %u us", sFrameCalls, sFrameUs, sFrameMaxUs); } while (0);
    do { RG_LOGD("MK64 audio: %s jobs %u mix %u us max %u us output %u frames queued %u bytes",
            mk64_rg_audio_requested() ? "on" : "off",
            __atomic_load_n(&sMixJobs, __ATOMIC_RELAXED), __atomic_load_n(&sMixUs, __ATOMIC_RELAXED),
            __atomic_load_n(&sMixMaxUs, __ATOMIC_RELAXED), __atomic_load_n(&sOutputFrames, __ATOMIC_RELAXED),
            (unsigned)port_audio_out_queued_bytes()); } while (0);
}

void mk64_rg_audio_shutdown(void)
{
    mk64_rg_audio_pause(true);
    if (!sWorker) return;
    if (!rg_task_send(sWorker, &(rg_task_msg_t){.type = RG_TASK_MSG_STOP}, -1)) RG_PANIC("MK64 audio stop failed");
    if (!rg_task_send(sOutputWorker, &(rg_task_msg_t){.type = RG_TASK_MSG_STOP}, -1)) RG_PANIC("MK64 PCM stop failed");
    while (__atomic_load_n(&sStopped, __ATOMIC_ACQUIRE) != 3) rg_task_delay(1);
    sWorker = sOutputWorker = NULL;
}
