#include "../compat.h"

#if !defined(__BSD__) && !defined(TARGET_DOS) && (defined(ENABLE_OPENGL) || defined(ENABLE_OPENGL_LEGACY) || defined(ENABLE_SOFTRAST))

#ifdef __MINGW32__
#define FOR_WINDOWS 1
#else
#define FOR_WINDOWS 0
#endif

#if FOR_WINDOWS
#include <GL/glew.h>
#include "SDL.h"
#define GL_GLEXT_PROTOTYPES 1
#include "SDL_opengl.h"
#else
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
/*#define GL_GLEXT_PROTOTYPES 1
#ifdef ENABLE_OPENGL_LEGACY
#include <SDL2/SDL_opengl.h>
#else
#include <SDL2/SDL_opengles2.h>
#endif*/
#endif

#include "../common.h"
#include "../configfile.h"
#include "gfx_window_manager_api.h"
#include "gfx_screen_config.h"
#include "gfx_sdl_menu.h"
#include "../savestate.h"

//#define DEBUG_ADAPTATIVE_RES
#ifdef DEBUG_ADAPTATIVE_RES
#define DEBUG_ADAPTATIVE_RES_PRINTF(...)   printf(__VA_ARGS__);
#else
#define DEBUG_ADAPTATIVE_RES_PRINTF(...)
#endif // DEBUG

// Support math
#define Half(A) (((A) >> 1) & 0x7F7F7F7F)
#define Quarter(A) (((A) >> 2) & 0x3F3F3F3F)
// Error correction expressions to piece back the lower bits together
#define RestHalf(A) ((A) & 0x01010101)
#define RestQuarter(A) ((A) & 0x03030303)

// Error correction expressions for quarters of pixels
#define Corr1_3(A, B)     Quarter(RestQuarter(A) + (RestHalf(B) << 1) + RestQuarter(B))
#define Corr3_1(A, B)     Quarter((RestHalf(A) << 1) + RestQuarter(A) + RestQuarter(B))

// Error correction expressions for halves
#define Corr1_1(A, B)     ((A) & (B) & 0x01010101)

// Quarters
#define Weight1_3(A, B)   (Quarter(A) + Half(B) + Quarter(B) + Corr1_3(A, B))
#define Weight3_1(A, B)   (Half(A) + Quarter(A) + Quarter(B) + Corr3_1(A, B))

// Halves
#define Weight1_1(A, B)   (Half(A) + Half(B) + Corr1_1(A, B))

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


#if defined(VERSION_EU)
# define FRAMERATE 25
#else
# define FRAMERATE 30
#endif

#ifndef SDL_TRIPLEBUF
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

#ifdef ENABLE_SOFTRAST
#define GFX_API_NAME "SDL1.2 - Software"
#include "gfx_soft.h"
SDL_Surface *sdl_screen SAVESTATE_EXCLUDE = NULL;
SDL_Surface *texture SAVESTATE_EXCLUDE = NULL;
#else
#define GFX_API_NAME "SDL2 - OpenGL"
#endif

// Handling subresolutions 
#define SUB_RES_DIVIDER  6     //1 is just fullscreen
#define NB_SUBRESOLUTIONS ( (1<<(SUB_RES_DIVIDER-1))/2 + 1 )
static SDL_Surface *sdl_screen_subRes[NB_SUBRESOLUTIONS] SAVESTATE_EXCLUDE;
static SDL_Rect resolutions[NB_SUBRESOLUTIONS];
//static bool half_res = false;
static int current_res_idx SAVESTATE_EXCLUDE = 0;

// time between consecutive game frames
const int frame_time = 1000 / FRAMERATE;
static int too_slow_in_a_row SAVESTATE_EXCLUDE = 0;
static int fast_speed_in_a_row SAVESTATE_EXCLUDE = 0;
static int elapsed_time_avg SAVESTATE_EXCLUDE = 0;
static int elapsed_time_cnt SAVESTATE_EXCLUDE = 0;
static bool dichotomic_res_change SAVESTATE_EXCLUDE = true;
#define MAX_AVG_ELAPSED_TIME_COUNTS 3
#define MAX_TOO_SLOW_IN_A_ROW       3
#define MAX_FAST_SPEED_IN_A_ROW   3

//static SDL_Window *wnd;
static int inverted_scancode_table[512];
static int vsync_enabled = 0;
static unsigned int window_width SAVESTATE_EXCLUDE = DESIRED_SCREEN_WIDTH;
static unsigned int window_height SAVESTATE_EXCLUDE = DESIRED_SCREEN_HEIGHT;
static void (*on_fullscreen_changed_callback)(bool is_now_fullscreen);
static bool (*on_key_down_callback)(int scancode);
static bool (*on_key_up_callback)(int scancode);
static void (*on_all_keys_up_callback)(void);

// frameskip
static bool do_render SAVESTATE_EXCLUDE = true;
static volatile uint32_t tick SAVESTATE_EXCLUDE = 0;
static uint32_t last SAVESTATE_EXCLUDE = 0;
static SDL_TimerID idTimer SAVESTATE_EXCLUDE = 0;
static Uint32 last_time SAVESTATE_EXCLUDE = 0;

// fps stats tracking
static int f_frames SAVESTATE_EXCLUDE = 0;
static double f_time SAVESTATE_EXCLUDE = 0.0;

// #if defined(DIRECT_SDL) && defined(SDL_SURFACE)
// 	uint32_t *gfx_output SAVESTATE_EXCLUDE;
// #endif


const SDLKey windows_scancode_table[] =
{
    /*	0						1							2							3							4						5							6							7 */
    /*	8						9							A							B							C						D							E							F */
    SDLK_UNKNOWN,		SDLK_ESCAPE,		SDLK_1,				SDLK_2,				SDLK_3,			SDLK_4,				SDLK_5,				SDLK_6,			/* 0 */
    SDLK_7,				SDLK_8,				SDLK_9,				SDLK_0,				SDLK_MINUS,		SDLK_EQUALS,		SDLK_BACKSPACE,		SDLK_TAB,		/* 0 */

    SDLK_q,				SDLK_w,				SDLK_e,				SDLK_r,				SDLK_t,			SDLK_y,				SDLK_u,				SDLK_i,			/* 1 */
    SDLK_o,				SDLK_p,				SDLK_LEFTBRACKET,	SDLK_RIGHTBRACKET,	SDLK_RETURN,	SDLK_LCTRL,			SDLK_a,				SDLK_s,			/* 1 */

    SDLK_d,				SDLK_f,				SDLK_g,				SDLK_h,				SDLK_j,			SDLK_k,				SDLK_l,				SDLK_SEMICOLON,	/* 2 */
    SDLK_UNKNOWN,	SDLK_UNKNOWN,			SDLK_LSHIFT,		SDLK_BACKSLASH,		SDLK_z,			SDLK_x,				SDLK_c,				SDLK_v,			/* 2 */

    SDLK_b,				SDLK_n,				SDLK_m,				SDLK_COMMA,			SDLK_PERIOD,	SDLK_SLASH,			SDLK_RSHIFT,		SDLK_PRINT,/* 3 */
    SDLK_LALT,			SDLK_SPACE,			SDLK_CAPSLOCK,		SDLK_F1,			SDLK_F2,		SDLK_F3,			SDLK_F4,			SDLK_F5,		/* 3 */

    SDLK_F6,			SDLK_F7,			SDLK_F8,			SDLK_F9,			SDLK_F10,		SDLK_NUMLOCK,	SDLK_SCROLLOCK,	SDLK_HOME,		/* 4 */
    SDLK_UP,			SDLK_PAGEUP,		SDLK_KP_MINUS,		SDLK_LEFT,			SDLK_KP5,		SDLK_RIGHT,			SDLK_KP_PLUS,		SDLK_END,		/* 4 */

    SDLK_DOWN,			SDLK_PAGEDOWN,		SDLK_INSERT,		SDLK_DELETE,		SDLK_UNKNOWN,	SDLK_UNKNOWN,		SDLK_BACKSLASH,SDLK_F11,		/* 5 */
    SDLK_F12,			SDLK_PAUSE,			SDLK_UNKNOWN,		SDLK_UNKNOWN,			SDLK_UNKNOWN,		SDLK_UNKNOWN,	SDLK_UNKNOWN,		SDLK_UNKNOWN,	/* 5 */

    SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_F13,		SDLK_F14,			SDLK_F15,			SDLK_UNKNOWN,		/* 6 */
    SDLK_UNKNOWN,			SDLK_UNKNOWN,			SDLK_UNKNOWN,			SDLK_UNKNOWN,		SDLK_UNKNOWN,	SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,	/* 6 */

    SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,	SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,	/* 7 */
    SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN,	SDLK_UNKNOWN,		SDLK_UNKNOWN,		SDLK_UNKNOWN	/* 7 */
};

const SDLKey scancode_rmapping_extended[][2] = {
    {SDLK_KP_ENTER, SDLK_RETURN},
    {SDLK_RALT, SDLK_LALT},
    {SDLK_RCTRL, SDLK_LCTRL},
    {SDLK_KP_DIVIDE, SDLK_SLASH},
    //{SDLK_KP_PLUS, SDLK_CAPSLOCK}
};

const SDLKey scancode_rmapping_nonextended[][2] = {
    {SDLK_KP7, SDLK_HOME},
    {SDLK_KP8, SDLK_UP},
    {SDLK_KP9, SDLK_PAGEUP},
    {SDLK_KP4, SDLK_LEFT},
    {SDLK_KP6, SDLK_RIGHT},
    {SDLK_KP1, SDLK_END},
    {SDLK_KP2, SDLK_DOWN},
    {SDLK_KP3, SDLK_PAGEDOWN},
    {SDLK_KP0, SDLK_INSERT},
    {SDLK_KP_PERIOD, SDLK_DELETE},
    {SDLK_KP_MULTIPLY, SDLK_PRINT},
    {SDLK_UP, SDLK_UP},
    {SDLK_LEFT, SDLK_LEFT},
    {SDLK_RIGHT, SDLK_RIGHT},
    {SDLK_DOWN, SDLK_DOWN},
};

#ifdef ENABLE_SOFTRAST
static void apply_subRes(int res) {
  window_width = resolutions[res].w;
  window_height = resolutions[res].h;
  sdl_screen = sdl_screen_subRes[res];
  gfx_output = sdl_screen->pixels;
}
#endif

static void set_higherRes(bool dichotomic, bool call_callback) {
#ifdef ENABLE_SOFTRAST
  //printf("%s\n", __func__);
  
  /** Linear */
  if(!dichotomic){
    current_res_idx = (current_res_idx>0)?(current_res_idx-1):0;
  }
  else{   /** Dichotomic */
    current_res_idx = current_res_idx/2;
  }  
  
  apply_subRes(current_res_idx);
  DEBUG_ADAPTATIVE_RES_PRINTF("Set higher resolution %s (idx %d/%d): %dx%d\n", 
    dichotomic?"dichotomic":"linear", current_res_idx, (NB_SUBRESOLUTIONS-1), window_width, window_height);

  if (on_fullscreen_changed_callback != NULL && call_callback) {
      on_fullscreen_changed_callback(true);
  }
#endif
}

static void set_lowerRes(bool dichotomic, bool call_callback) {
#ifdef ENABLE_SOFTRAST
  //printf("%s\n", __func__);

  /** Linear */
  if(!dichotomic){
    current_res_idx = (current_res_idx<(NB_SUBRESOLUTIONS-1))?(current_res_idx+1):NB_SUBRESOLUTIONS-1;
  }
  else{   /** Dichotomic */
    current_res_idx = (NB_SUBRESOLUTIONS-1) - ((NB_SUBRESOLUTIONS-1)-current_res_idx)/2; // do not factorize or it won't be the same res because of integer fractions
  }

  apply_subRes(current_res_idx);
  DEBUG_ADAPTATIVE_RES_PRINTF("Set Lower resolution %s (idx %d/%d): %dx%d\n", 
    dichotomic?"dichotomic":"linear", current_res_idx, (NB_SUBRESOLUTIONS-1), window_width, window_height);

  if (on_fullscreen_changed_callback != NULL && call_callback) {
      on_fullscreen_changed_callback(true);
  }
#endif
}

static void set_fullscreen(bool on, bool call_callback) {
#ifndef ENABLE_SOFTRAST
  if (on) {
      SDL_DisplayMode mode;
      SDL_GetDesktopDisplayMode(0, &mode);
      window_width = mode.w;
      window_height = mode.h;
  } else {
      window_width = configScreenWidth;
      window_height = configScreenHeight;
  }
  //printf("window_width=%d, window_height=%d\n", window_width, window_height);
  SDL_SetWindowSize(wnd, window_width, window_height);
  SDL_SetWindowFullscreen(wnd, on ? SDL_WINDOW_FULLSCREEN : 0);

  if (on_fullscreen_changed_callback != NULL && call_callback) {
      on_fullscreen_changed_callback(on);
  }
#else
  //set_halfResScreen(!on, call_callback);
#endif
}

/* Clear SDL screen (for multiple-buffering) */
static void clear_screen(SDL_Surface *surface) {
  memset(surface->pixels, 0, surface->w*surface->h*surface->format->BytesPerPixel);
}

int test_vsync(void) {
    // Even if SDL_GL_SetSwapInterval succeeds, it doesn't mean that VSync actually works.
    // A 60 Hz monitor should have a swap interval of 16.67 milliseconds.
    // Try to detect the length of a vsync by swapping buffers some times.
    // Since the graphics card may enqueue a fixed number of frames,
    // first send in four dummy frames to hopefully fill the queue.
    // This method will fail if the refresh rate is changed, which, in
    // combination with that we can't control the queue size (i.e. lag)
    // is a reason this generic SDL2 backend should only be used as last resort.
    #ifndef ENABLE_SOFTRAST
    Uint32 start;
    Uint32 end;

    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    start = SDL_GetTicks();
    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    SDL_GL_SwapWindow(wnd);
    end = SDL_GetTicks();

    float average = 4.0 * 1000.0 / (end - start);

    vsync_enabled = 1;
    if (average > 27 && average < 33) {
        SDL_GL_SetSwapInterval(1);
    } else if (average > 57 && average < 63) {
        SDL_GL_SetSwapInterval(2);
    } else if (average > 86 && average < 94) {
        SDL_GL_SetSwapInterval(3);
    } else if (average > 115 && average < 125) {
        SDL_GL_SetSwapInterval(4);
    } else {
        vsync_enabled = 0;
    }
    vsync_enabled = 1;
    #endif
    vsync_enabled = 0;
}


static uint32_t timer_handler(uint32_t interval, void *param)
{
    //printf("%s, interval=%d, tick=%d\n", __func__, interval, tick);
     ++tick;
    return interval;
}

static void gfx_sdl_init(const char *game_name, bool start_in_fullscreen) {
    
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    
    if(TTF_Init()==-1) {
      printf("ERROR TTF_Init: %s\n", TTF_GetError());
      game_exit();
    }

    idTimer = SDL_AddTimer(frame_time, timer_handler, NULL);
    last = tick;

    char title[512];
    sprintf(title, "%s (%s)", game_name, GFX_API_NAME);
	
    window_width = configScreenWidth;
    window_height = configScreenHeight;
#ifdef ENABLE_SOFTRAST

	SDL_ShowCursor(0);
  #ifdef CONVERT
  	 sdl_screen = SDL_SetVideoMode(window_width, window_height, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
  #else
    #ifdef DIRECT_SDL
    	sdl_screen = SDL_SetVideoMode(window_width, window_height, 32, SDL_HWSURFACE | SDL_TRIPLEBUF);
    #else
    	texture = SDL_SetVideoMode(window_width, window_height, 32, SDL_HWSURFACE | SDL_TRIPLEBUF);


      int dividend = (1 << (SUB_RES_DIVIDER-1)); 
      for(int i=0; i < NB_SUBRESOLUTIONS; i++){
        int factor = dividend-i;
        DEBUG_ADAPTATIVE_RES_PRINTF("NB_SUBRESOLUTIONS=%d, i=%d, factor=%d, dividend=%d\n",NB_SUBRESOLUTIONS, i, factor, dividend);
        resolutions[i].w = window_width*factor/dividend;
        resolutions[i].h = window_height*factor/dividend;
        sdl_screen_subRes[i] = SDL_CreateRGBSurface(SDL_SWSURFACE, resolutions[i].w, resolutions[i].h, 32, 0,0,0,0);  
        DEBUG_ADAPTATIVE_RES_PRINTF("Creating surface for sub resolution[%d]: %dx%d \n", i, resolutions[i].w, resolutions[i].h);
      }
      
      //set_halfResScreen(!start_in_fullscreen, false);
      current_res_idx = 0;
      sdl_screen = sdl_screen_subRes[current_res_idx];

      init_menu_SDL();
    #endif
  #endif
	#ifdef SDL_SURFACE
	   gfx_output = sdl_screen->pixels;
	#endif
#else
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    wnd = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (start_in_fullscreen) {
        set_fullscreen(start_in_fullscreen, false);
    }

    SDL_GL_CreateContext(wnd);

    SDL_GL_SetSwapInterval(1);
    test_vsync();
    if (!vsync_enabled)
        puts("Warning: VSync is not enabled or not working. Falling back to timer for synchronization");
#endif

    f_time = SDL_GetTicks();

    for (size_t i = 0; i < sizeof(windows_scancode_table) / sizeof(SDLKey); i++) {
        inverted_scancode_table[windows_scancode_table[i]] = i;
    }

    for (size_t i = 0; i < sizeof(scancode_rmapping_extended) / sizeof(scancode_rmapping_extended[0]); i++) {
        inverted_scancode_table[scancode_rmapping_extended[i][0]] = inverted_scancode_table[scancode_rmapping_extended[i][1]] + 0x100;
    }

    for (size_t i = 0; i < sizeof(scancode_rmapping_nonextended) / sizeof(scancode_rmapping_nonextended[0]); i++) {
        inverted_scancode_table[scancode_rmapping_nonextended[i][0]] = inverted_scancode_table[scancode_rmapping_nonextended[i][1]];
        inverted_scancode_table[scancode_rmapping_nonextended[i][1]] += 0x100;
    }
}

static void gfx_sdl_set_fullscreen_changed_callback(void (*on_fullscreen_changed)(bool is_now_fullscreen)) {
    on_fullscreen_changed_callback = on_fullscreen_changed;
}

static void gfx_sdl_set_fullscreen(bool enable) {
    set_fullscreen(enable, true);
}

static void gfx_sdl_set_keyboard_callbacks(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode), void (*on_all_keys_up)(void)) {
    on_key_down_callback = on_key_down;
    on_key_up_callback = on_key_up;
    on_all_keys_up_callback = on_all_keys_up;
}

static void gfx_sdl_main_loop(void (*run_one_game_iter)(void)) {
    const uint32_t now = tick;

    const uint32_t frames = now - last;
    if (frames) {

        // catch up but skip the first FRAMESKIP frames
        int skip = (frames > configFrameskip) ? configFrameskip : (frames - 1);
        for (uint32_t f = 0; f < frames; ++f, --skip) {
            do_render = (skip <= 0);
            
            /*if(!do_render){
              printf("frameskip! %d missed frames\n", frames-1);
            }*/

            run_one_game_iter();
        }
    }
    else{
        do_render = true;
        run_one_game_iter();
    }
    
    last = now;

    /*while (1) {
        run_one_game_iter();
    }*/
}

static void gfx_sdl_get_dimensions(uint32_t *width, uint32_t *height) {
/*#ifdef ENABLE_SOFTRAST
    *width = configScreenWidth;
    *height = configScreenHeight;
#else
    *width = window_width;
    *height = window_height;
#endif*/
    *width = window_width;
    *height = window_height;
}

static int translate_scancode(int scancode) {
    if (scancode < 512) {
        return inverted_scancode_table[scancode];
    } else {
        return 0;
    }
}

static void gfx_sdl_onkeydown(int scancode) {
    int key = translate_scancode(scancode);

    if (on_key_down_callback != NULL) {
        on_key_down_callback(key);
    }
}

static void gfx_sdl_onkeyup(int scancode) {
    int key = translate_scancode(scancode);
    if (on_key_up_callback != NULL) {
        on_key_up_callback(key);
    }
}

static void gfx_sdl_handle_events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
#ifndef TARGET_WEB
            // Scancodes are broken in Emscripten SDL2: https://bugzilla.libsdl.org/show_bug.cgi?id=3259
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym){
                  
                  case SDLK_q:
                  //game_exit();
                  run_menu_loop();
                  clear_screen(texture);
                  last_time = SDL_GetTicks(); // otherwise frameskip will kickoff
                  last = tick; // same
                  break;

                  // case SDLK_h:
                  // //printf("Aspect ratio change\n");
                  // aspect_ratio = (aspect_ratio+1)%NB_ASPECT_RATIOS_TYPES;

                  // char shell_cmd_tmp[100];
                  // sprintf(shell_cmd_tmp, "%s %d \"    DISPLAY MODE: %s\"", 
                  //   SHELL_CMD_NOTIF_SET, NOTIF_SECONDS_DISP, aspect_ratio_name[aspect_ratio]);
                  // FILE *fp_tmp = popen(shell_cmd_tmp, "r");
                  // if (fp_tmp == NULL) {
                  //   printf("Failed to run command %s\n", shell_cmd_tmp);
                  // }
                  // pclose(fp_tmp);
                  // break;

#ifdef DEBUG
                  // testing quicksave on desktop
                  case SDLK_F8:
                    savestate_request_save(QUICKSAVE_SLOT);
                  break;

                  // testing output of dynamics resolutions on desktop
                  case SDLK_F11:
                    set_lowerRes(dichotomic_res_change, true);
                  break;

                  case SDLK_F12:
                    set_higherRes(dichotomic_res_change, true);
                  break;
#endif

                  default:
                  break;
                }

                gfx_sdl_onkeydown(event.key.keysym.sym);
                break;
            case SDL_KEYUP:
                gfx_sdl_onkeyup(event.key.keysym.sym);
                break;
#endif
            case SDL_QUIT:
                game_exit();
                break;
        }
    }
}

static bool gfx_sdl_start_frame(void) {
    //return true;
    return do_render;
}

static void sync_framerate_with_timer(void) {
    //printf("%s\n", __func__);

    // get base timestamp on the first frame (might be different from 0)
    if (last_time == 0) last_time = SDL_GetTicks();
    const int elapsed = SDL_GetTicks() - last_time;
    if (elapsed < frame_time){
      SDL_Delay(frame_time - elapsed);
    }
    last_time = SDL_GetTicks();
    
    elapsed_time_avg += elapsed;
    elapsed_time_cnt++;

    if(elapsed_time_cnt > MAX_AVG_ELAPSED_TIME_COUNTS){
      elapsed_time_avg /= elapsed_time_cnt;

      int interval = frame_time/(NB_SUBRESOLUTIONS)+1;
      int min_frame_time = frame_time - current_res_idx*(frame_time/2)/NB_SUBRESOLUTIONS - 1;
      int max_frame_time = frame_time;

      if (elapsed_time_avg < min_frame_time && current_res_idx!= 0){
        too_slow_in_a_row = 0;
        fast_speed_in_a_row++;
        dichotomic_res_change = (elapsed_time_avg < min_frame_time - interval)?true:false;
        DEBUG_ADAPTATIVE_RES_PRINTF("speed %d, elapsed_time_avg=%d, min_frame_time=%d, interval=%d, frametime=%d\n", 
              fast_speed_in_a_row, elapsed_time_avg, min_frame_time, interval, frame_time);
      }
      else if(elapsed_time_avg > max_frame_time){
        too_slow_in_a_row++;
        fast_speed_in_a_row = 0;
        dichotomic_res_change = (elapsed_time_avg > max_frame_time + interval)?true:false;
        DEBUG_ADAPTATIVE_RES_PRINTF("slow %d, elapsed_time_avg=%d, min_frame_time=%d, interval=%d, frametime=%d\n", 
              too_slow_in_a_row, elapsed_time_avg, min_frame_time, interval, frame_time);
      }
      else{
        too_slow_in_a_row=0;
        fast_speed_in_a_row = 0;
      }

      elapsed_time_cnt = 0;
      elapsed_time_avg = 0;
    }
      
}


static uint16_t rgb888Torgb565(uint32_t s)
{
	return (uint16_t) ((s >> 8 & 0xf800) + (s >> 5 & 0x7e0) + (s >> 3 & 0x1f));
}


/// Nearest neighboor optimized with possible out of screen coordinates (for cropping)
static void flip_NNOptimized_AllowOutOfScreen(SDL_Surface *src_surface, SDL_Rect *src_rect, SDL_Surface *dst_surface, int new_w, int new_h) {
  int w1 = src_rect->w;
  int h1 = src_rect->h;
  int w2 = new_w;
  int h2 = new_h;
  int x_ratio = (int) ((w1 << 16) / w2);
  int y_ratio = (int) ((h1 << 16) / h2);
  int x2, y2;

  /// --- Compute padding for centering when out of bounds ---
  int y_padding = (RES_HW_SCREEN_VERTICAL - new_h) / 2;
  int x_padding = 0;
  if (w2 > RES_HW_SCREEN_HORIZONTAL) {
    x_padding = (w2 - RES_HW_SCREEN_HORIZONTAL) / 2 + 1;
  }
  int x_padding_ratio = x_padding * w1 / w2;

  /// --- Offset to get first src_pixels row
  uint32_t *src_row = (uint32_t*)(src_surface->pixels) + src_surface->w * src_rect->y + src_rect->x;

  for (int i = 0; i < h2; i++) {
    if (i >= RES_HW_SCREEN_VERTICAL) {
      continue;
    }

    uint32_t *t = ((uint32_t *)dst_surface->pixels) + ((i + y_padding) * ((w2 > RES_HW_SCREEN_HORIZONTAL) ? RES_HW_SCREEN_HORIZONTAL : w2)) ;
    y2 = (i * y_ratio) >> 16;
    uint32_t *p = (uint32_t*)(src_row) + (y2*src_surface->w + x_padding_ratio) ;
    int rat = 0;
    for (int j = 0; j < w2; j++) {
      if (j >= RES_HW_SCREEN_HORIZONTAL) {
        continue;
      }
      x2 = rat >> 16;
      *t++ = p[x2];
      rat += x_ratio;
    }
  }
}



/// Nearest neighboor optimized with possible out of screen coordinates (for cropping)
void flip_Upscaling_Bilinear(SDL_Surface *src_surface, SDL_Rect *src_rect, SDL_Surface *dst_surface, int new_w, int new_h){
  int w1 = src_rect->w;
  int h1 = src_rect->h;
  int w2=new_w;
  int h2=new_h;
  int x_ratio = (int) ((w1 << 16) / w2);
  int y_ratio = (int) ((h1 << 16) / h2);
  uint32_t x_diff, y_diff;
  uint32_t red_comp, green_comp, blue_comp, alpha_comp;
  uint32_t p_val_tl, p_val_tr, p_val_bl, p_val_br;
  int x, y ;
  //printf("src_surface->h=%d, h2=%d\n", src_surface->h, h2);

  /// --- Compute padding for centering when out of bounds ---
  int y_padding = (RES_HW_SCREEN_VERTICAL-new_h)/2;
  int x_padding = 0;
  if(w2>RES_HW_SCREEN_HORIZONTAL){
    x_padding = (w2-RES_HW_SCREEN_HORIZONTAL)/2 + 1;
  }
  int x_padding_ratio = x_padding * w1 / w2;

  /// --- Offset to get first src_pixels row
  uint32_t *src_row = (uint32_t*)(src_surface->pixels) + src_surface->w * src_rect->y + src_rect->x;

  for (int i=0;i<h2;i++)
  {
    if(i>=RES_HW_SCREEN_VERTICAL){
      continue;
    }

    uint32_t *t = ((uint32_t *)dst_surface->pixels) + ((i + y_padding) * ((w2 > RES_HW_SCREEN_HORIZONTAL) ? RES_HW_SCREEN_HORIZONTAL : w2)) ;
    y = ((i*y_ratio)>>16);
    y_diff = (i*y_ratio) - (y<<16) ;
    uint32_t *p = (uint32_t*)(src_row) + (y*src_surface->w + x_padding_ratio) ;
    int rat = 0;
    for (int j=0;j<w2;j++)
    {
      if(j>=RES_HW_SCREEN_HORIZONTAL){
        continue;
      }
      x = (rat>>16);
      x_diff = rat - (x<<16) ;

      /// --- Getting adjacent pixels ---
      p_val_tl = p[x] ;
      p_val_tr = (x+1<w1)?p[x+1]:p[x];
      p_val_bl = (y+1<h1)?p[x+w1]:p[x];
      p_val_br = (y+1<h1 && x+1<w1)?p[x+w1+1]:p[x];

      // red element
      // Yr = Ar(1-w)(1-h) + Br(w)(1-h) + Cr(h)(1-w) + Dr(wh)
      red_comp = (( ((p_val_tl&0xFF000000)>>24) * ( (((1<<16)-x_diff) * ((1<<16)-y_diff)) >>8) )>>24) +
          (( ((p_val_tr&0xFF000000)>>24) * ((x_diff * ((1<<16)-y_diff)) >>8) )>>24) +
            (( ((p_val_bl&0xFF000000)>>24) * ((y_diff * ((1<<16)-x_diff)) >>8) )>>24) +
            (( ((p_val_br&0xFF000000)>>24) * ((y_diff * x_diff) >>8) )>>24);

      // green element
      // Yg = Ag(1-w)(1-h) + Bg(w)(1-h) + Cg(h)(1-w) + Dg(wh)
      green_comp = (( ((p_val_tl&0x00FF0000)>>16) * ((((1<<16)-x_diff) * ((1<<16)-y_diff))>>8) )>>24) +
          (( ((p_val_tr&0x00FF0000)>>16) * ((x_diff * ((1<<16)-y_diff)) >>8) )>>24) +
            (( ((p_val_bl&0x00FF0000)>>16) * ((y_diff * ((1<<16)-x_diff)) >>8) )>>24) +
            (( ((p_val_br&0x00FF0000)>>16) * ((y_diff * x_diff) >>8) )>>24);

      // blue element
      // Yb = Ab(1-w)(1-h) + Bb(w)(1-h) + Cb(h)(1-w) + Db(wh)
      blue_comp = (( ((p_val_tl&0x0000FF00)>>8) * ((((1<<16)-x_diff) * ((1<<16)-y_diff))>>8) )>>24) +
          (( ((p_val_tr&0x0000FF00)>>8) * ((x_diff * ((1<<16)-y_diff)) >>8) )>>24) +
            (( ((p_val_bl&0x0000FF00)>>8) * ((y_diff * ((1<<16)-x_diff)) >>8) )>>24) +
            (( ((p_val_br&0x0000FF00)>>8) * ((y_diff * x_diff) >>8) )>>24);

      // alpha element
      // Ya = Aa(1-w)(1-h) + Ba(w)(1-h) + Ca(h)(1-w) + Da(wh)
      alpha_comp = (( ((p_val_tl&0x000000FF)) * ((((1<<16)-x_diff) * ((1<<16)-y_diff))>>8) )>>24) +
          (( ((p_val_tr&0x000000FF)) * ((x_diff * ((1<<16)-y_diff)) >>8) )>>24) +
            (( ((p_val_bl&0x000000FF)) * ((y_diff * ((1<<16)-x_diff)) >>8) )>>24) +
            (( ((p_val_br&0x000000FF)) * ((y_diff * x_diff) >>8) )>>24);

     //   printf("red_comp=%d, green_comp=%d, blue_comp=%d, alpha_comp=%d, \n", red_comp, green_comp, blue_comp, alpha_comp);

      /// --- Write pixel value ---
      *t++ = ((red_comp<<24)&0xFF000000) + ((green_comp<<16)&0x00FF0000) + ((blue_comp<<8)&0x0000FF00) + ((alpha_comp)&0x000000FF);

      /// --- Update x ----
      rat += x_ratio;
    }
  }
}


void upscale_160x120_to_320x240_bilinearish(SDL_Surface *src_surface, SDL_Surface *dst_surface)
{
  if (src_surface->w != 160)
  {
    printf("src_surface->w (%d) != 160 \n", src_surface->w);
    return;
  }
  if (src_surface->h != 120)
  {
    printf("src_surface->h (%d) != 120 \n", src_surface->h);
    return;
  }

  uint32_t *Src32 = (uint32_t *) src_surface->pixels;
  uint32_t *Dst32 = (uint32_t *) dst_surface->pixels;

  // There are 80 blocks of 2 pixels horizontally, and 48 of 3 horizontally.
  // Horizontally: 320=80*4 160=80*2
  // Vertically: 240=60*4 120=60*2
  // Each block of 2*2 becomes 4x4.
  uint32_t BlockX, BlockY;
  uint32_t *BlockSrc;
  uint32_t *BlockDst;
  uint32_t _a, _b, _aaab, _abbb, _c, _d, _cccd, _cddd;
  for (BlockY = 0; BlockY < 60; BlockY++)
  {
    BlockSrc = Src32 + BlockY * 160 * 2;
    BlockDst = Dst32 + BlockY * RES_HW_SCREEN_HORIZONTAL * 4;
    for (BlockX = 0; BlockX < 80; BlockX++)
    {
      /* Horizontaly:
       * Before(2):
       * (a)(b)
       * After(4):
       * (a)(aaab)(abbb)(b)
       */

      /* Verticaly:
       * Before(2):
       * (1)(2)
       * After(4):
       * (1)(1112)(1222)(2)
       */

      // -- Line 1 --
      _a = *(BlockSrc                          );
      _b = *(BlockSrc                       + 1);
      _aaab = Weight3_1( _a,  _b);
      _abbb = Weight1_3( _a,  _b);
      *(BlockDst                               ) = _a;
      *(BlockDst                            + 1) = _aaab;
      *(BlockDst                            + 2) = _abbb;
      *(BlockDst                            + 3) = _b;

      // -- Line 2 --
      _c = *(BlockSrc             + 160 * 1    );
      _d = *(BlockSrc             + 160 * 1 + 1);
      _cccd = Weight3_1( _c,  _d);
      _cddd = Weight1_3( _c,  _d);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 1    ) = Weight3_1(_a, _c);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 1 + 1) = Weight3_1(_aaab, _cccd);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 1 + 2) = Weight3_1(_abbb, _cddd);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 1 + 3) = Weight3_1(_b, _d);

      // -- Line 3 --
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 2    ) = Weight1_3(_a, _c);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 2 + 1) = Weight1_3(_aaab, _cccd);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 2 + 2) = Weight1_3(_abbb, _cddd);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 2 + 3) = Weight1_3(_b, _d);

      // -- Line 4 --
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 3    ) = _c;
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 3 + 1) = _cccd;
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 3 + 2) = _cddd;
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 3 + 3) = _d;

      BlockSrc += 2;
      BlockDst += 4;
    }
  }
}


//void upscale_160x120_to_320x240_bilinearish_cropScreen(SDL_Surface *src_surface, SDL_Surface *dst_surface)
//{
//  if (src_surface->w != 160)
//  {
//    printf("src_surface->w (%d) != 160 \n", src_surface->w);
//    return;
//  }
//  if (src_surface->h != 120)
//  {
//    printf("src_surface->h (%d) != 120 \n", src_surface->h);
//    return;
//  }

//  uint32_t *Src32 = (uint32_t *) src_surface->pixels;
//  uint32_t *Dst32 = (uint32_t *) dst_surface->pixels;
//  uint32_t x_src_padding = 20;

//  // There are 80 blocks of 2 pixels horizontally, and 48 of 3 horizontally.
//  // Horizontally: 320=80*4 160=80*2
//  // Vertically: 240=60*4 120=60*2
//  // Each block of 2*2 becomes 4x4.
//  uint32_t BlockX, BlockY;
//  uint32_t *BlockSrc;
//  uint32_t *BlockDst;
//  uint32_t _a, _b, _aaab, _abbb, _c, _d, _cccd, _cddd;
//  for (BlockY = 0; BlockY < 60; BlockY++)
//  {
//    BlockSrc = Src32 + BlockY * 160 * 2 + x_src_padding;
//    BlockDst = Dst32 + BlockY * 240 * 4;
//    for (BlockX = 0; BlockX < 80-x_src_padding; BlockX++)
//    {
//      /* Horizontaly:
//       * Before(2):
//       * (a)(b)
//       * After(3):
//       * (a)(aaab)(abbb)(b)
//       */

//      /* Verticaly:
//       * Before(2):
//       * (1)(2)
//       * After(4):
//       * (1)(1112)(1222)(2)
//       */

//      // -- Line 1 --
//      _a = *(BlockSrc                          );
//      _b = *(BlockSrc                       + 1);
//      _aaab = Weight3_1( _a,  _b);
//      _abbb = Weight1_3( _a,  _b);
//      *(BlockDst                               ) = _a;
//      *(BlockDst                            + 1) = _aaab;
//      *(BlockDst                            + 2) = _abbb;
//      *(BlockDst                            + 3) = _b;

//      // -- Line 2 --
//      _c = *(BlockSrc             + 160 * 1    );
//      _d = *(BlockSrc             + 160 * 1 + 1);
//      _cccd = Weight3_1( _c,  _d);
//      _cddd = Weight1_3( _c,  _d);
//      *(BlockDst                  + 240 * 1    ) = Weight3_1(_a, _c);
//      *(BlockDst                  + 240 * 1 + 1) = Weight3_1(_aaab, _cccd);
//      *(BlockDst                  + 240 * 1 + 2) = Weight3_1(_abbb, _cddd);
//      *(BlockDst                  + 240 * 1 + 3) = Weight3_1(_b, _d);

//      // -- Line 3 --
//      *(BlockDst                  + 240 * 2    ) = Weight1_3(_a, _c);
//      *(BlockDst                  + 240 * 2 + 1) = Weight1_3(_aaab, _cccd);
//      *(BlockDst                  + 240 * 2 + 2) = Weight1_3(_abbb, _cddd);
//      *(BlockDst                  + 240 * 2 + 3) = Weight1_3(_b, _d);

//      // -- Line 4 --
//      *(BlockDst                  + 240 * 3    ) = _c;
//      *(BlockDst                  + 240 * 3 + 1) = _cccd;
//      *(BlockDst                  + 240 * 3 + 2) = _cddd;
//      *(BlockDst                  + 240 * 3 + 3) = _d;

//      BlockSrc += 2;
//      BlockDst += 4;
//    }
//  }
//}


/// Interpolation with left, right pixels, pseudo gaussian weighting for downscaling - operations on 32bits
void downscale_320x240_to_320x240_bilinearish(SDL_Surface *src_surface, SDL_Surface *dst_surface){
  int w1=src_surface->w;
  int h1=src_surface->h;
  int w2=dst_surface->w;
  int h2=dst_surface->h;

  if(w1!=320){
    printf("src_surface->w (%d) != 320\n", src_surface->w);
    return;
  }

  //printf("src = %dx%d\n", w1, h1);
  int y_ratio = (int)((h1<<16)/h2);
  int y_padding = (RES_HW_SCREEN_VERTICAL-h2)/2;
  int y1;
  uint32_t *src_screen = (uint32_t *)src_surface->pixels;
  uint32_t *dst_screen = (uint32_t *)dst_surface->pixels;

  /* Interpolation */
  for (int i=0;i<h2;i++)
  {
    if(i>=RES_HW_SCREEN_VERTICAL){
      continue;
    }
    uint32_t* t = (uint32_t*)(dst_screen +
      (i+y_padding)*((w2>RES_HW_SCREEN_HORIZONTAL)?RES_HW_SCREEN_HORIZONTAL:w2) );

    // ------ current and next y value ------
    y1 = ((i*y_ratio)>>16);
    uint32_t* p = (uint32_t*)(src_screen + (y1*w1) );

    for (int j=0;j<w2;j++)
    {
      *t++ = *p++;
    }
  }
}


void downscale_320x240_to_320x180_bilinearish(SDL_Surface *src_surface, SDL_Surface *dst_surface)
{
  if (src_surface->w != 320)
  {
    printf("src_surface->w (%d) != 320 \n", src_surface->w);
    return;
  }
  if (src_surface->h != 240)
  {
    printf("src_surface->h (%d) != 240 \n", src_surface->h);
    return;
  }

  /// Compute padding for centering when out of bounds
  int y_padding = (RES_HW_SCREEN_VERTICAL-180)/2;

  uint32_t *Src32 = (uint32_t *) src_surface->pixels;
  uint32_t *Dst32 = (uint32_t *) dst_surface->pixels + y_padding*RES_HW_SCREEN_HORIZONTAL;

  // There are 80 blocks of 4 pixels horizontally, and 80 of 4 horizontally.
  // Horizontally: 320=80*4 320=80*4
  // Vertically: 240=60*4 180=60*3
  // Each block of 4*4 becomes 4*3
  uint32_t BlockX, BlockY;
  uint32_t *BlockSrc;
  uint32_t *BlockDst;
  for (BlockY = 0; BlockY < 60; BlockY++)
  {
    BlockSrc = Src32 + BlockY * 320 * 4;
    BlockDst = Dst32 + BlockY * RES_HW_SCREEN_HORIZONTAL * 3;
    for (BlockX = 0; BlockX < 80; BlockX++)
    {
      // -- Data --
      uint32_t _a1 = *(BlockSrc                          );
      uint32_t _b1 = *(BlockSrc                       + 1);
      uint32_t _c1 = *(BlockSrc                       + 2);
      uint32_t _d1 = *(BlockSrc                       + 3);
      uint32_t _a2 = *(BlockSrc             + 320 * 1    );
      uint32_t _b2 = *(BlockSrc             + 320 * 1 + 1);
      uint32_t _c2 = *(BlockSrc             + 320 * 1 + 2);
      uint32_t _d2 = *(BlockSrc             + 320 * 1 + 3);
      uint32_t _a3 = *(BlockSrc             + 320 * 2    );
      uint32_t _b3 = *(BlockSrc             + 320 * 2 + 1);
      uint32_t _c3 = *(BlockSrc             + 320 * 2 + 2);
      uint32_t _d3 = *(BlockSrc             + 320 * 2 + 3);
      uint32_t _a4 = *(BlockSrc             + 320 * 3    );
      uint32_t _b4 = *(BlockSrc             + 320 * 3 + 1);
      uint32_t _c4 = *(BlockSrc             + 320 * 3 + 2);
      uint32_t _d4 = *(BlockSrc             + 320 * 3 + 3);

      // -- Line 1 --
      *(BlockDst                               ) = Weight3_1(_a1, _a2);
      *(BlockDst                            + 1) = Weight3_1(_b1, _b2);
      *(BlockDst                            + 2) = Weight3_1(_c1, _c2);
      *(BlockDst                            + 3) = Weight3_1(_d1, _d2);

      // -- Line 2 --
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 1    ) = Weight1_1(_a2, _a3);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 1 + 1) = Weight1_1(_b2, _b3);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 1 + 2) = Weight1_1(_c2, _c3);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 1 + 3) = Weight1_1(_d2, _d3);

      // -- Line 3 --
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 2    ) = Weight1_3(_a3, _a4);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 2 + 1) = Weight1_3(_b3, _b4);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 2 + 2) = Weight1_3(_c3, _c4);
      *(BlockDst                  + RES_HW_SCREEN_HORIZONTAL * 2 + 3) = Weight1_3(_d3, _d4);

      BlockSrc += 4;
      BlockDst += 4;
    }
  }
}

// void gfx_sdl_upscale_to_screen(void) {
//    /*#ifndef SDL_SURFACE
//    SDL_BlitSurface(sdl_screen, NULL, texture, NULL);
//    #else
//      SDL_BlitSurface(sdl_screen, NULL, texture, NULL);
//    #endif*/

//   /** Clear screen on aspect ratio change */
//   static prev_aspect_ratio = ASPECT_RATIOS_TYPE_SCALED;
//   if(prev_aspect_ratio != aspect_ratio){
//     prev_aspect_ratio = aspect_ratio;
//     clear_screen(texture);
//   }

//   switch (aspect_ratio){

//     /** Stretched */
//     case ASPECT_RATIOS_TYPE_STRETCHED:
//     if(resolutions[current_res_idx].w==160 && resolutions[current_res_idx].h==120){
//       upscale_160x120_to_240x240_bilinearish(sdl_screen, texture);
//     }
//     else if(resolutions[current_res_idx].w==320 && resolutions[current_res_idx].h==240){
//       downscale_320x240_to_240x240_bilinearish(sdl_screen, texture);
//     }
//     else{
//       flip_NNOptimized_AllowOutOfScreen(sdl_screen, &resolutions[current_res_idx], texture, RES_HW_SCREEN_HORIZONTAL, RES_HW_SCREEN_VERTICAL); 
//     }
//     break;

//     /** Scaled */
//     case ASPECT_RATIOS_TYPE_SCALED:
//     if(resolutions[current_res_idx].w==320 && resolutions[current_res_idx].h==240){
//       downscale_320x240_to_240x180_bilinearish(sdl_screen, texture);
//     }
//     else{
//       flip_NNOptimized_AllowOutOfScreen(sdl_screen, &resolutions[current_res_idx], texture, RES_HW_SCREEN_HORIZONTAL, configScreenHeight*RES_HW_SCREEN_HORIZONTAL/configScreenWidth);
//     }
//     break;

//     /** Cropped */
//     case ASPECT_RATIOS_TYPE_CROPPED:
//     if (current_res_idx==0) {
//       SDL_BlitSurface(sdl_screen, &middle_rect, texture, NULL);
//     }
//     else if(resolutions[current_res_idx].w==160 && resolutions[current_res_idx].h==120){
//       upscale_160x120_to_320x240_bilinearish_cropScreen(sdl_screen, texture);
//     }
//     else{
//       flip_NNOptimized_AllowOutOfScreen(sdl_screen, &resolutions[current_res_idx], texture, configScreenWidth*RES_HW_SCREEN_VERTICAL/configScreenHeight, RES_HW_SCREEN_VERTICAL);
//     }
//     break;

//     default:
//     printf("Wrong aspect ratio value: %s, setting cropped\n", aspect_ratio);
//     aspect_ratio = ASPECT_RATIOS_TYPE_CROPPED;
//     break;
//   }

//   //flip_NNOptimized_AllowOutOfScreen(sdl_screen, &resolutions[current_res_idx], texture, RES_HW_SCREEN_HORIZONTAL, RES_HW_SCREEN_VERTICAL);
//   //flip_NNOptimized_AllowOutOfScreen(sdl_screen, ptr_src_rect, texture, 320, RES_HW_SCREEN_VERTICAL);
//   //SDL_BlitSurface(sdl_screen, NULL, texture, NULL);
// }


static SDL_Rect middle_rect = {0,0,320,240};

void gfx_sdl_upscale_to_fullscreen(void) {
  if (current_res_idx==0) {
    return;
  }
//  else if(resolutions[current_res_idx].w==160 && resolutions[current_res_idx].h==120){
//    upscale_160x120_to_320x240_bilinearish_cropScreen(sdl_screen, sdl_screen_subRes[0]);
//  }
  else{
    flip_NNOptimized_AllowOutOfScreen(sdl_screen, &resolutions[current_res_idx], sdl_screen_subRes[0], configScreenWidth*RES_HW_SCREEN_VERTICAL/configScreenHeight, RES_HW_SCREEN_VERTICAL);
  }
  apply_subRes(0);
  on_fullscreen_changed_callback(true);
}

static void gfx_sdl_swap_buffers_begin(void) {

    if (!vsync_enabled) {
        sync_framerate_with_timer();
    }
#ifdef ENABLE_SOFTRAST
#ifdef DIRECT_SDL

#ifndef SDL_SURFACE
	  sdl_screen->pixels = gfx_output;
#endif
	  SDL_Flip(sdl_screen);
#else
    //gfx_sdl_upscale_to_screen();
    SDL_BlitSurface(sdl_screen_subRes[0], &middle_rect, texture, NULL);
	
    SDL_Flip(texture);
#endif
#else
    SDL_GL_SwapWindow(wnd);
#endif
}

static void gfx_sdl_swap_buffers_end(void) {
    f_frames++;

    // let F11 and F12 control this for testing when debugging
#ifndef DEBUG
    if(NB_SUBRESOLUTIONS > 1){
        if( (fast_speed_in_a_row >= MAX_FAST_SPEED_IN_A_ROW) && current_res_idx != 0){

          fast_speed_in_a_row = 0;
          set_higherRes(dichotomic_res_change, true);
        }
        else if(too_slow_in_a_row >= MAX_TOO_SLOW_IN_A_ROW && (current_res_idx < NB_SUBRESOLUTIONS-1) ){
          
          too_slow_in_a_row = 0;
          set_lowerRes(dichotomic_res_change, true);
        }
        else if (current_res_idx != 0) {
          apply_subRes(current_res_idx);
          on_fullscreen_changed_callback(true);
        }
    }
#else
    if (current_res_idx != 0) {
      apply_subRes(current_res_idx);
      on_fullscreen_changed_callback(true);
    }
#endif
}

static double gfx_sdl_get_time(void) {
    return 0.0;
}

static void gfx_sdl_shutdown(void) {
  for(int i=0; i<NB_SUBRESOLUTIONS; i++){
    if(sdl_screen_subRes[i]){SDL_FreeSurface(sdl_screen_subRes[i]);}
  }

  deinit_menu_SDL();

  TTF_Quit();

  SDL_RemoveTimer(idTimer);

  const double elapsed = (SDL_GetTicks() - f_time) / 1000.0;
  printf("\nstats\n");
  printf("frames    %010d\n", f_frames);
  printf("time      %010.4lf sec\n", elapsed);
  printf("frametime %010.8lf sec\n", elapsed / (double)f_frames);
  printf("framerate %010.5lf fps\n\n", (double)f_frames / elapsed);
  fflush(stdout);

  SDL_Quit();
}

struct GfxWindowManagerAPI gfx_sdl = {
    gfx_sdl_init,
    gfx_sdl_set_keyboard_callbacks,
    gfx_sdl_set_fullscreen_changed_callback,
    gfx_sdl_set_fullscreen,
    gfx_sdl_main_loop,
    gfx_sdl_get_dimensions,
    gfx_sdl_handle_events,
    gfx_sdl_start_frame,
    gfx_sdl_swap_buffers_begin,
    gfx_sdl_swap_buffers_end,
    gfx_sdl_get_time,
    gfx_sdl_shutdown,
};

#endif
