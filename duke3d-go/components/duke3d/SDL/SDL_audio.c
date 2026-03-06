#include "SDL_audio.h"
#include "freertos/semphr.h"
#include "rg_audio.h"
#include "oplplayer.h"

SDL_AudioSpec as;
bool paused = true;
bool locked = false;

static int16_t *mono_buffer = NULL;
static int16_t *music_buffer = NULL;
static rg_audio_frame_t *stereo_buffer = NULL;
TaskHandle_t audio_task_handle = NULL;
static bool audio_task_running = false;

#define ENGINE_RATE 11025
#define HW_RATE     22050

IRAM_ATTR void updateTask(void *arg)
{
  audio_task_running = true;
  if (!mono_buffer) mono_buffer = malloc(SAMPLECOUNT * sizeof(int16_t));
  if (!music_buffer) music_buffer = malloc(SAMPLECOUNT * 2 * sizeof(int16_t));
  if (!stereo_buffer) stereo_buffer = malloc(SAMPLECOUNT * 2 * sizeof(rg_audio_frame_t));

  while(audio_task_running)
  {
	  if(!paused && !locked && as.callback){
			// Ask engine for sound effects (at 11025Hz)
            memset(mono_buffer, 0, SAMPLECOUNT * sizeof(int16_t));
			(*as.callback)(as.userdata, (uint8_t *)mono_buffer, SAMPLECOUNT * sizeof(int16_t));

            // Ask OPL synth for music (at 11025Hz)
            memset(music_buffer, 0, SAMPLECOUNT * 2 * sizeof(int16_t));
            opl_synth_player.render(music_buffer, SAMPLECOUNT);

            for (int i = 0; i < SAMPLECOUNT; i++) {
                int32_t mixed_l = mono_buffer[i] + music_buffer[i*2];
                int32_t mixed_r = mono_buffer[i] + music_buffer[i*2+1];
                
                // Clamp
                if (mixed_l > 32767) mixed_l = 32767; else if (mixed_l < -32768) mixed_l = -32768;
                if (mixed_r > 32767) mixed_r = 32767; else if (mixed_r < -32768) mixed_r = -32768;
                
                // 2x Oversampling (duplicate samples) to fill HW_RATE buffer
                stereo_buffer[i*2].left = mixed_l;
                stereo_buffer[i*2].right = mixed_r;
                stereo_buffer[i*2+1].left = mixed_l;
                stereo_buffer[i*2+1].right = mixed_r;
            }
            
			rg_audio_submit(stereo_buffer, SAMPLECOUNT * 2);
            vTaskDelay(pdMS_TO_TICKS(1)); // Yield
	  } else {
		  vTaskDelay(pdMS_TO_TICKS(10));
      }
  }
  
  if (mono_buffer) { free(mono_buffer); mono_buffer = NULL; }
  if (music_buffer) { free(music_buffer); music_buffer = NULL; }
  if (stereo_buffer) { free(stereo_buffer); stereo_buffer = NULL; }
  audio_task_handle = NULL;
  vTaskDelete(NULL);
}

void SDL_AudioInit()
{
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
	SDL_AudioInit();
	memset(obtained, 0, sizeof(SDL_AudioSpec));
    
	obtained->freq = ENGINE_RATE;
	obtained->format = desired->format;
	obtained->channels = 1;
	obtained->samples = SAMPLECOUNT;
	obtained->callback = desired->callback;
    obtained->userdata = desired->userdata;
	memcpy(&as, obtained, sizeof(SDL_AudioSpec));

    rg_audio_set_sample_rate(HW_RATE);
    opl_synth_player.init(ENGINE_RATE); // Synth renders at 11kHz

	xTaskCreatePinnedToCore(&updateTask, "audioTask", 8192, NULL, 15, &audio_task_handle, 0);
	printf("audio task started at %d Hz output (Rendering at %d Hz)\n", HW_RATE, ENGINE_RATE);
	return 0;
}

void SDL_PauseAudio(int pause_on)
{
	paused = pause_on;
}

void SDL_CloseAudio(void)
{
    if (audio_task_running) {
        audio_task_running = false;
        int retry = 500;
        while (audio_task_handle != NULL && retry-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void audio_shutdown(void)
{
    RG_LOGI("audio_shutdown: Muting and stopping audio task...");
    as.callback = NULL;
    if (rg_audio_get_sink()) {
        rg_audio_set_mute(true);
    }
    SDL_CloseAudio();
}

int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, Uint16 src_format, Uint8 src_channels, int src_rate, Uint16 dst_format, Uint8 dst_channels, int dst_rate)
{
	cvt->len_mult = 1;
	return 0;
}

int SDL_ConvertAudio(SDL_AudioCVT *cvt)
{
	return 0;
}

void SDL_LockAudio(void)
{
	locked = true;
}

void SDL_UnlockAudio(void)
{
    locked = false;
}
