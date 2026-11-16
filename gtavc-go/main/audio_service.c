#include "audio_service.h"
#include "snd.h"
#include <rg_system.h>
#include <string.h>

enum { AUDIO_STARTING, AUDIO_RUNNING, AUDIO_PAUSED, AUDIO_STOPPED };
static struct {
    rg_mutex_t *lock;
    rg_task_t *task;
    unsigned pauseDepth;
    bool stop;
    int state;
} audioService;

static void audioWorker(void *arg) {
    (void)arg;
    rg_audio_frame_t samples[GTAVC_AUDIO_FRAMES];
    for (;;) {
        rg_mutex_take(audioService.lock, -1);
        if (audioService.stop) {
            audioService.state = AUDIO_STOPPED;
            rg_mutex_give(audioService.lock);
            return;
        }
        if (audioService.pauseDepth) {
            /* Only acknowledge between complete mix/submit operations. */
            audioService.state = AUDIO_PAUSED;
            rg_mutex_give(audioService.lock);
            rg_task_delay(10);
            continue;
        }
        audioService.state = AUDIO_RUNNING;
        rg_mutex_give(audioService.lock);
        if (!Snd_ok || !Snd_on || rg_audio_get_mute()) memset(samples, 0, sizeof(samples));
        else Snd_mix((int16_t*)samples, GTAVC_AUDIO_FRAMES, GTAVC_AUDIO_RATE);
        rg_audio_submit(samples, GTAVC_AUDIO_FRAMES);
    }
}

bool AudioService_start(void) {
    if (audioService.task) {
        rg_mutex_take(audioService.lock, -1);
        bool active = !audioService.stop;
        rg_mutex_give(audioService.lock);
        return active;
    }
    audioService.lock = rg_mutex_create();
    if (!audioService.lock) return false;
    audioService.state = AUDIO_STARTING;
    audioService.task = rg_task_create("gtavc_sound", audioWorker, NULL, 4*1024, 1, RG_TASK_PRIORITY_2, 1);
    if (!audioService.task) {
        rg_mutex_free(audioService.lock);
        memset(&audioService, 0, sizeof(audioService));
        return false;
    }
    return true;
}

void AudioService_pause(void) {
    if (!audioService.task) return;
    rg_mutex_take(audioService.lock, -1);
    audioService.pauseDepth++;
    rg_mutex_give(audioService.lock);
    for (;;) {
        rg_mutex_take(audioService.lock, -1);
        bool quiet = audioService.state == AUDIO_PAUSED || audioService.state == AUDIO_STOPPED;
        rg_mutex_give(audioService.lock);
        if (quiet) return;
        rg_task_delay(10);
    }
}

void AudioService_resume(void) {
    if (!audioService.task) return;
    rg_mutex_take(audioService.lock, -1);
    if (audioService.pauseDepth) audioService.pauseDepth--;
    rg_mutex_give(audioService.lock);
}

void AudioService_stop(void) {
    if (!audioService.task) return;
    rg_mutex_take(audioService.lock, -1);
    audioService.stop = true;
    rg_mutex_give(audioService.lock);
    for (;;) {
        rg_mutex_take(audioService.lock, -1);
        bool quiet = audioService.state == AUDIO_STOPPED;
        rg_mutex_give(audioService.lock);
        if (quiet) return;
        rg_task_delay(10);
    }
    /* The small control mutex lives until application restart. Reset reuses the
       same task; stop is terminal and can be called again during shutdown. */
}
