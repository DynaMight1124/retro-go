#include "audio.h"
#include <rg_system.h>
#include <rg_utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

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
    float volume;
    float target_volume;
    float fade_step;
} music_t;

static sfx_t *loaded_sfx[MAX_SFX] = {NULL};
static channel_t channels[MAX_CHANNELS];
static music_t current_music;
static int system_sample_rate = 22050;
static volatile rg_task_t *audio_task_handle = NULL;
static volatile bool task_running = false;
static volatile int pending_music_index = -2;

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

    uint8_t *data_ptr = ptr + 12;
    while (data_ptr < ptr + size - 8) {
        if (memcmp(data_ptr, "data", 4) == 0) {
            uint32_t data_size = *(uint32_t *)(data_ptr + 4);
            if (data_ptr + 8 + data_size > ptr + size) data_size = (ptr + size) - (data_ptr + 8);
            
            sfx_t *sfx = malloc(sizeof(sfx_t));
            sfx->length = data_size / (bit_depth / 8) / channels;
            sfx->channels = channels;
            sfx->data = rg_alloc(data_size, MEM_SLOW); 
            memcpy(sfx->data, data_ptr + 8, data_size);
            loaded_sfx[id] = sfx;
            break;
        }
        data_ptr += 8 + *(uint32_t *)(data_ptr + 4);
    }
    free(data);
}

void audio_init(int sample_rate) {
    if (audio_task_handle) return;

    system_sample_rate = sample_rate;
    current_music.active = false;
    current_music.file = NULL;
    pending_music_index = -2;

    for (int i = 0; i < MAX_CHANNELS; i++) {
        channels[i].active = false;
    }

    static const int ids[] = {0,1,2,3,4,5,6,7,8,9,13,14,15,16,23,35,37,38,40,50,51,54,55};
    for (size_t i = 0; i < sizeof(ids)/sizeof(ids[0]); i++) {
        if (!loaded_sfx[ids[i]]) load_sfx(ids[i]);
    }

    task_running = true;
    audio_task_handle = rg_task_create("audio_task", audio_task, NULL, 4096, 1, RG_TASK_PRIORITY_7, 1);
}

void audio_shutdown(void) {
    task_running = false;
}

void audio_deinit(void) {
    if (!task_running) return;

    task_running = false;
    
    int timeout = 100;
    while (audio_task_handle && timeout-- > 0) {
        rg_task_delay(10);
    }

    for (int i = 0; i < MAX_SFX; i++) {
        if (loaded_sfx[i]) {
            if (loaded_sfx[i]->data) free(loaded_sfx[i]->data);
            free(loaded_sfx[i]);
            loaded_sfx[i] = NULL;
        }
    }
}

void audio_sfx_play(int id) {
    if (id < 0 || id >= MAX_SFX || !loaded_sfx[id] || !task_running) return;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!channels[i].active) {
            channels[i].sfx = loaded_sfx[id];
            channels[i].position = 0;
            channels[i].active = true;
            return;
        }
    }
}

void audio_music_play(int index, int fade_ms) {
    if (!task_running) return;
    pending_music_index = index;
}

void audio_stop_all(void) {
    for (int i = 0; i < MAX_CHANNELS; i++) channels[i].active = false;
    audio_music_play(-1, 0);
}

static void audio_task(void *arg) {
    const int buffer_frames = 512;
    rg_audio_frame_t *buffer = malloc(buffer_frames * sizeof(rg_audio_frame_t));
    int16_t *mus_buffer = malloc(buffer_frames * sizeof(int16_t));

    RG_LOGI("Audio task started.\n");

    while (task_running) {
        if (pending_music_index != -2) {
            if (current_music.file) {
                fclose(current_music.file);
                current_music.file = NULL;
            }
            current_music.active = false;
            
            int index = pending_music_index;
            pending_music_index = -2;

            if (index >= 0) {
                char path[256];
                snprintf(path, sizeof(path), MUS_PATH, index);
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
                                current_music.volume = 1.0f;
                                current_music.target_volume = 1.0f;
                                current_music.active = true;
                                RG_LOGI("Music %d started (offset %u, size %u)\n", index, (unsigned int)current_music.data_start, (unsigned int)current_music.data_size);
                                break;
                            }
                            ptr += 8 + *(uint32_t *)(ptr + 4);
                        }
                    }
                    if (!current_music.active) fclose(f);
                }
            }
        }

        memset(buffer, 0, buffer_frames * sizeof(rg_audio_frame_t));

        if (current_music.active && current_music.file) {
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
                int32_t sample = (int32_t)(mus_buffer[f] * current_music.volume);
                buffer[f].left = sample;
                buffer[f].right = sample;
            }
        }

        for (int i = 0; i < MAX_CHANNELS; i++) {
            if (!channels[i].active) continue;
            sfx_t *sfx = channels[i].sfx;
            for (int f = 0; f < buffer_frames; f++) {
                if (channels[i].position >= sfx->length) {
                    channels[i].active = false;
                    break;
                }
                int16_t sample = (sfx->channels == 1) ? sfx->data[channels[i].position] : sfx->data[channels[i].position * 2];
                int32_t l = (int32_t)buffer[f].left + sample;
                int32_t r = (int32_t)buffer[f].right + sample;
                buffer[f].left = (l > 32767) ? 32767 : (l < -32768 ? -32768 : l);
                buffer[f].right = (r > 32767) ? 32767 : (r < -32768 ? -32768 : r);
                channels[i].position++;
            }
        }

        rg_audio_submit(buffer, buffer_frames);
        rg_task_delay(1); 
    }
    
    if (current_music.file) {
        fclose(current_music.file);
        current_music.file = NULL;
    }

    free(buffer);
    free(mus_buffer);
    
    RG_LOGI("Audio task finished.\n");
    
    audio_task_handle = NULL;
#ifdef ESP_PLATFORM
    vTaskDelete(NULL);
#endif
}
