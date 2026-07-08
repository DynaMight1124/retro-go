#if !defined(_WIN32) && !defined(_WIN64) && !defined(TARGET_DOS)

#ifdef __MINGW32__
#include "SDL.h"
#else
#include "SDL/SDL.h"
#endif

#include "audio_api.h"

//static int32_t BUFFSIZE; // 512 * 2 * 2 * 8
#define BUFFSIZE 16384
static uint8_t buffer[BUFFSIZE];
static uint32_t buf_read_pos = 0;
static uint32_t buf_write_pos = 0;
static int32_t buffered_bytes = 0;

static void sdl_write_buffer(uint8_t* data, int32_t len)
{
	for(uint32_t i = 0; i < len; i += 4) 
	{
		if(buffered_bytes == BUFFSIZE) return; // just drop samples
		*(int32_t*)((char*)(buffer + buf_write_pos)) = *(int32_t*)((char*)(data + i));
		//memcpy(buffer + buf_write_pos, data + i, 4);
		buf_write_pos = (buf_write_pos + 4) % BUFFSIZE;
		buffered_bytes += 4;
	}
}

static int32_t sdl_read_buffer(uint8_t* data, int32_t len)
{
	if (buffered_bytes >= len) 
	{
		if(buf_read_pos + len <= BUFFSIZE ) 
		{
			memcpy(data, buffer + buf_read_pos, len);
		} 
		else 
		{
			int32_t tail = BUFFSIZE - buf_read_pos;
			memcpy(data, buffer + buf_read_pos, tail);
			memcpy(data + tail, buffer, len - tail);
		}
		buf_read_pos = (buf_read_pos + len) % BUFFSIZE;
		buffered_bytes -= len;
	}

	return len;
}

void sdl_callback(void *unused, uint8_t *stream, int32_t len)
{
	sdl_read_buffer((uint8_t *)stream, len);
}

static bool audio_sdl_init(void) {
	//BUFFSIZE = 512 * 2 * 2 * 8;
	//buffer = (uint8_t *) malloc(BUFFSIZE);
    
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return false;
    }
    SDL_AudioSpec want, have;
    want.freq = 32000;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 512;
    want.callback = sdl_callback;
	if (SDL_OpenAudio( &want, &have)  < 0)
	{
        fprintf(stderr, "SDL_OpenAudio error: %s\n", SDL_GetError());
        return false;
    }
    SDL_PauseAudio(0);
    return true;
}

static int audio_sdl_buffered(void) {
	SDL_LockAudio();
	int ret=buffered_bytes / 4;
	SDL_UnlockAudio();
	return ret;
}

static int audio_sdl_get_desired_buffered(void) {
    return 1100;
}

static void audio_sdl_play(const uint8_t *buf, size_t len) {
	SDL_LockAudio();
	sdl_write_buffer(buf, len);
	SDL_UnlockAudio();
}

struct AudioAPI audio_sdl = {
    audio_sdl_init,
    audio_sdl_buffered,
    audio_sdl_get_desired_buffered,
    audio_sdl_play
};

#endif
