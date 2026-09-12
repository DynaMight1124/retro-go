#include "audio.h"
#include <rg_system.h>
#include <rg_utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SFX 64
#define MAX_CHANNELS 8
#define SFX_PATH RG_BASE_PATH_ROMS "/celeste/data/snd%d.wav"
#define MUS_PATH RG_BASE_PATH_ROMS "/celeste/data/mus%d.wav"

typedef struct {
    int16_t *data;
    size_t length; // in samples
    int channels;
} sfx_t;

typedef struct {
    sfx_t *sfx;
    size_t position;
    bool active;
} channel_t;

typedef struct {
    FILE *file;
    uint32_t data_start;
    uint32_t data_size;
    uint32_t position;
    bool active;
} music_t;

static sfx_t *loaded_sfx[MAX_SFX] = {NULL};
static channel_t channels[MAX_CHANNELS];
static music_t current_music;
static rg_task_t *audio_task_handle = NULL;
static rg_mutex_t *audio_lock = NULL;
static volatile bool task_running = false;
static volatile bool task_alive = false;
static int pending_music_index = -2;

static void audio_task(void *arg);

static void load_sfx(int id) {
    char path[256];
    snprintf(path, sizeof(path), SFX_PATH, id);

    void *data = NULL;
    size_t size = 0;
    if (!rg_storage_read_file(path, &data, &size, 0)) return;

    uint8_t *ptr = (uint8_t *)data;
    if (size < 44 || memcmp(ptr, "RIFF", 4) != 0) {
        free(data);
        return;
    }

    int bit_depth = *(uint16_t *)(ptr + 34);
    int channels = *(uint16_t *)(ptr + 22);
    if (bit_depth != 16 || (channels != 1 && channels != 2)) {
        free(data);
        return;
    }

    uint8_t *data_ptr = ptr + 12;
    while (data_ptr < ptr + size - 8) {
        if (memcmp(data_ptr, "data", 4) == 0) {
            uint32_t data_size = *(uint32_t *)(data_ptr + 4);
            if (data_ptr + 8 + data_size > ptr + size) data_size = (ptr + size) - (data_ptr + 8);
            if (data_size == 0) break;

            sfx_t *sfx = malloc(sizeof(sfx_t));
            if (!sfx) break;

            sfx->length = data_size / (bit_depth / 8) / channels;
            sfx->channels = channels;
            sfx->data = rg_alloc(data_size, MEM_SLOW | MEM_NOPANIC);
            if (!sfx->data) {
                free(sfx);
                break;
            }

            memcpy(sfx->data, data_ptr + 8, data_size);
            loaded_sfx[id] = sfx;
            break;
        }
        data_ptr += 8 + *(uint32_t *)(data_ptr + 4);
    }
    free(data);
}

void audio_init(int sample_rate) {
    if (audio_task_handle || task_alive) return;

    (void)sample_rate;
    current_music.active = false;
    current_music.file = NULL;
    pending_music_index = -2;

    audio_lock = rg_mutex_create();
    if (!audio_lock) {
        RG_LOGE("Unable to create audio command lock.");
        return;
    }

    for (int i = 0; i < MAX_CHANNELS; i++) {
        channels[i].active = false;
    }

    static const int ids[] = {0,1,2,3,4,5,6,7,8,9,13,14,15,16,23,35,37,38,40,50,51,54,55};
    for (size_t i = 0; i < sizeof(ids)/sizeof(ids[0]); i++) {
        if (!loaded_sfx[ids[i]]) load_sfx(ids[i]);
    }

    task_running = true;
    task_alive = true;
    audio_task_handle = rg_task_create("audio_task", audio_task, NULL, 4096, 1, RG_TASK_PRIORITY_2, 1);
    if (!audio_task_handle) {
        task_running = false;
        task_alive = false;
        rg_mutex_free(audio_lock);
        audio_lock = NULL;
    }
}

void audio_shutdown(void) {
    task_running = false;
}

void audio_deinit(void) {
    task_running = false;

    int timeout = 100;
    while (task_alive && timeout-- > 0) {
        rg_task_delay(10);
    }

    if (task_alive) {
        RG_LOGW("Audio task did not stop in time; keeping its buffers alive.");
        return;
    }

    audio_task_handle = NULL;

    for (int i = 0; i < MAX_SFX; i++) {
        if (loaded_sfx[i]) {
            if (loaded_sfx[i]->data) free(loaded_sfx[i]->data);
            free(loaded_sfx[i]);
            loaded_sfx[i] = NULL;
        }
    }

    rg_mutex_free(audio_lock);
    audio_lock = NULL;
}

void audio_sfx_play(int id) {
    if (id < 0 || id >= MAX_SFX || !loaded_sfx[id] ||
        !task_running || !audio_lock) return;

    if (!rg_mutex_take(audio_lock, -1)) return;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!channels[i].active) {
            channels[i].sfx = loaded_sfx[id];
            channels[i].position = 0;
            channels[i].active = true;
            break;
        }
    }
    rg_mutex_give(audio_lock);
}

void audio_music_play(int index, int fade_ms) {
    (void)fade_ms;
    if (!task_running || !audio_lock) return;

    if (!rg_mutex_take(audio_lock, -1)) return;
    pending_music_index = index;
    rg_mutex_give(audio_lock);
}

void audio_stop_all(void) {
    if (!audio_lock) return;

    if (!rg_mutex_take(audio_lock, -1)) return;
    for (int i = 0; i < MAX_CHANNELS; i++) channels[i].active = false;
    pending_music_index = -1;
    rg_mutex_give(audio_lock);
}

static void audio_task(void *arg) {
    const int buffer_frames = 512;
    /*
     * These small buffers are touched for every output sample. Request internal
     * memory explicitly, while allowing a clean task-start failure if the
     * allocation cannot be satisfied anywhere.
     */
    rg_audio_frame_t *buffer =
        rg_alloc(buffer_frames * sizeof(*buffer), MEM_FAST | MEM_NOPANIC);
    int16_t *mus_buffer =
        rg_alloc(buffer_frames * sizeof(*mus_buffer), MEM_FAST | MEM_NOPANIC);
    int32_t *mix_buffer =
        rg_alloc(buffer_frames * sizeof(*mix_buffer), MEM_FAST | MEM_NOPANIC);

    if (!buffer || !mus_buffer || !mix_buffer) {
        RG_LOGE("Unable to allocate audio mix buffers.");
        free(buffer);
        free(mus_buffer);
        free(mix_buffer);
        task_running = false;
        task_alive = false;
        return;
    }

    RG_LOGI("Audio task started.\n");

    while (task_running) {
        int music_index = -2;
        if (rg_mutex_take(audio_lock, -1)) {
            music_index = pending_music_index;
            pending_music_index = -2;
            rg_mutex_give(audio_lock);
        }

        if (music_index != -2) {
            if (current_music.file) {
                fclose(current_music.file);
                current_music.file = NULL;
            }
            current_music.active = false;

            if (music_index >= 0) {
                char path[256];
                snprintf(path, sizeof(path), MUS_PATH, music_index);
                FILE *f = fopen(path, "rb");
                if (f) {
                    uint8_t header[1024];
                    size_t read_len = fread(header, 1, sizeof(header), f);
                    if (read_len > 44 && memcmp(header, "RIFF", 4) == 0) {
                        uint8_t *ptr = header + 12;
                        while (ptr < header + read_len - 8) {
                            if (memcmp(ptr, "data", 4) == 0) {
                                current_music.data_start = (ptr + 8) - header;
                                current_music.data_size = *(uint32_t *)(ptr + 4);
                                current_music.file = f;
                                fseek(f, current_music.data_start, SEEK_SET);
                                current_music.position = 0;
                                current_music.active = true;
                                RG_LOGI("Music %d started (offset %u, size %u)\n", music_index, (unsigned int)current_music.data_start, (unsigned int)current_music.data_size);
                                break;
                            }
                            ptr += 8 + *(uint32_t *)(ptr + 4);
                        }
                    }
                    if (!current_music.active) fclose(f);
                }
            }
        }

        memset(mix_buffer, 0, buffer_frames * sizeof(int32_t));

        if (current_music.active && current_music.file) {
            memset(mus_buffer, 0, buffer_frames * sizeof(int16_t));
            size_t read = fread(mus_buffer, sizeof(int16_t), buffer_frames, current_music.file);
            current_music.position += read * sizeof(int16_t);

            if (read < (size_t)buffer_frames || current_music.position >= current_music.data_size) {
                fseek(current_music.file, current_music.data_start, SEEK_SET);
                current_music.position = 0;
                if (read < (size_t)buffer_frames) {
                    size_t read2 = fread(mus_buffer + read, sizeof(int16_t), buffer_frames - read, current_music.file);
                    current_music.position += read2 * sizeof(int16_t);
                }
            }

            for (int f = 0; f < buffer_frames; f++) {
                mix_buffer[f] = mus_buffer[f];
            }
        }

        if (rg_mutex_take(audio_lock, -1)) {
            for (int i = 0; i < MAX_CHANNELS; i++) {
                if (!channels[i].active) continue;
                sfx_t *sfx = channels[i].sfx;
                for (int f = 0; f < buffer_frames; f++) {
                    if (channels[i].position >= sfx->length) {
                        channels[i].active = false;
                        break;
                    }
                    int16_t sample = (sfx->channels == 1) ? sfx->data[channels[i].position] : sfx->data[channels[i].position * 2];
                    mix_buffer[f] += sample;
                    channels[i].position++;
                }
            }
            rg_mutex_give(audio_lock);
        }

        for (int f = 0; f < buffer_frames; f++) {
            int32_t sample = mix_buffer[f];
            if (sample > INT16_MAX) sample = INT16_MAX;
            else if (sample < INT16_MIN) sample = INT16_MIN;
            buffer[f].left = sample;
            buffer[f].right = sample;
        }

        rg_audio_submit(buffer, buffer_frames);
    }

    if (current_music.file) {
        fclose(current_music.file);
        current_music.file = NULL;
    }

    free(buffer);
    free(mus_buffer);
    free(mix_buffer);

    RG_LOGI("Audio task finished.\n");

    task_alive = false;
}
