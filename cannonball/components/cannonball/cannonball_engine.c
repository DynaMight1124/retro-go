/***************************************************************************
    Cannonball Main Entry Point.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

// SDL Library
#include <SDL/SDL.h>
#include <stdint.h>

// SDL Specific Code
#include "sdl/timer.h"
#include "sdl/input.h"
#include "sdl/music.h"
#include "video.h"

#include "romloader.h"
#include "trackloader.h"
#include <stdint.h>
#include "main.h"
#include "setup.h"

#include "frontend/config.h"
#include "frontend/menu.h"

#include "cannonboard/interface.h"
#include "engine/oinputs.h"
#include "engine/ooutputs.h"
#include "engine/omusic.h"

// Initialize Shared Variables
int    cannonball_state       = STATE_BOOT;
double cannonball_frame_ms    = 0;
int    cannonball_frame       = 0;
uint8_t   cannonball_tick_frame  = 1;
int    cannonball_fps_counter = 0;

#ifndef RETRO_GO
static void Clear_Cannonball()
{
#ifdef COMPILE_SOUND_CODE
    Audio_stop_audio();
#endif
    I_CAMD_StopSong();
    I_CAMD_ShutdownMusic();
    Input_close();
    Render_disable();
    SDL_Quit();
}

static void process_events(void)
{
    SDL_Event event;

    // Grab all events from the queue.
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                // Handle key presses.
                if (event.key.keysym.sym == SDLK_q || event.key.keysym.sym == SDLK_3 || event.key.keysym.sym == SDLK_HOME)
                    cannonball_state = STATE_QUIT;
                else
                    Input_handle_key_down(&event.key.keysym);
                break;

            case SDL_KEYUP:
                Input_handle_key_up(&event.key.keysym);
                break;

            case SDL_JOYAXISMOTION:
                Input_handle_joy_axis(&event.jaxis);
                break;

            case SDL_JOYBUTTONDOWN:
                Input_handle_joy_down(&event.jbutton);
                break;

            case SDL_JOYBUTTONUP:
                Input_handle_joy_up(&event.jbutton);
                break;

            case SDL_QUIT:
                // Handle quit requests (like Ctrl-c).
                cannonball_state = STATE_QUIT;
                break;
        }
    }
}
#endif

// Pause Engine
uint8_t pause_engine;

void tick()
{
    cannonball_frame++;

    Packet* packet = Config_cannonboard.enabled ? Interface_get_packet() : NULL;

    // Non standard FPS.
    // Determine whether to tick the current frame.
    if (Config_fps == 30)
        cannonball_tick_frame = 1;
    else if (Config_fps == 60)
        cannonball_tick_frame = cannonball_frame & 1;
    else if (Config_fps == 120)
        cannonball_tick_frame = (cannonball_frame & 3) == 1;

    process_events();

    if (cannonball_tick_frame)
        OInputs_tick(packet); // Do Controls
    OInputs_do_gear();        // Digital Gear

    switch (cannonball_state)
    {
        case STATE_GAME:
        {
            if (Input_has_pressed(INPUT_TIMER))
                Outrun_freeze_timer = !Outrun_freeze_timer;

            if (Input_has_pressed(INPUT_PAUSE))
                pause_engine = !pause_engine;

            if (Input_has_pressed(INPUT_MENU))
                cannonball_state = STATE_INIT_MENU;

            if (!pause_engine || Input_has_pressed(INPUT_STEP))
            {
                Outrun_tick(packet, cannonball_tick_frame);
                Input_frame_done(); // Denote keys read

                #ifdef COMPILE_SOUND_CODE
                // Tick audio program code
                OSoundInt_tick();
                #endif
            }
            else
            {
                Input_frame_done(); // Denote keys read
            }
            }
            break;

            case STATE_INIT_GAME:
            if (Config_engine.jap && !Roms_load_japanese_roms())
            {
                cannonball_state = STATE_QUIT;
            }
            else
            {
                pause_engine = 0;
                Outrun_init();
                cannonball_state = STATE_GAME;
            }
            break;

            case STATE_MENU:
            {
            Menu_tick(packet);
            Input_frame_done();
            #ifdef COMPILE_SOUND_CODE
            // Tick audio program code
            OSoundInt_tick();
            #endif
            }        break;

        case STATE_INIT_MENU:
            OInputs_init();
            OOutputs_init();
            Menu_init();
            cannonball_state = STATE_MENU;
            break;
    }

    // Draw SDL Video
    Video_draw_frame();  
}

#ifndef RETRO_GO
static void main_loop()
{
    // FPS Counter (If Enabled)
    Timer fps_count;
    int frame = 0;
    Timer_init(&fps_count);
    Timer_start(&fps_count);

    // General Frame Timing
    Timer frame_time;
    Timer_init(&frame_time);
    int t;
    double deltatime  = 0;
    int deltaintegral = 0;

    while (cannonball_state != STATE_QUIT)
    {
		uint32_t start;
		start = SDL_GetTicks();
		
        Timer_start(&frame_time);
        tick();
        #ifdef COMPILE_SOUND_CODE
        deltatime += (cannonball_frame_ms * Audio_adjust_speed());
        #else
        deltatime += cannonball_frame_ms;
        #endif
        deltaintegral  = (int) deltatime;
        t = Timer_get_ticks(&frame_time);

		// Cap Frame Rate: Sleep Remaining Frame Time
        if (t < deltatime)
        {
            SDL_Delay((Uint32) (deltatime - t));
        }
        
        deltatime -= deltaintegral;

        if (Config_video.fps_count)
        {
            frame++;
            if (Timer_get_ticks(&fps_count) >= 1000)
            {
                cannonball_fps_counter = frame;
                frame       = 0;
                Timer_start(&fps_count);
            }
        }
    }
}

int cannonball_main(int argc, char* argv[])
{
    // Initialize timer and video systems
	if ( SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1 ) 
	{ 
		fprintf(stderr, "SDL Initialization failed: %d\n", SDL_GetError());
		return 1; 
    }

    initTimer();
    
    TrackLoader_Create();

    // Load LayOut File
    uint8_t loaded = 0;
    if (argc == 3 && strcmp(argv[1], "-file") == 0)
    {
        if (TrackLoader_set_layout_track(argv[2]))
        {
            loaded = Roms_load_revb_roms();
        }
    }
    // Load Roms Only
    else
    {
        loaded = Roms_load_revb_roms();
    }
    
    //TrackLoader_set_layout_track("d:/temp.bin");
    //loaded = Roms_load_revb_roms();

    if (loaded)
    {
        // Load XML Config
        Config_load(FILENAME_CONFIG);
         
        I_CAMD_InitMusic();
 
        // Load fixed PCM ROM based on config
        if (Config_sound.fix_samples)
            Roms_load_pcm_rom(1);

        // Load patched widescreen tilemaps
        if (!OMusic_load_widescreen_map())
        {
            fprintf(stderr, "Unable to load widescreen tilemaps.\n");
        }

        Video_Create();

        // Initialize SDL Video, Only allow to run it if it is init
        if (Video_init(&Config_video))
		{
#ifdef COMPILE_SOUND_CODE
			Audio_init();
#endif
			cannonball_state = Config_menu.enabled ? STATE_INIT_MENU : STATE_INIT_GAME;

			// Initalize controls
			Input_init(Config_controls.pad_id,
					   Config_controls.keyconfig, Config_controls.padconfig, 
					   Config_controls.analog,    Config_controls.axis, Config_controls.asettings);


			main_loop();  // Loop until we quit the app
		}
    }
    else
    {
		Show_Warning();
	}

    Clear_Cannonball();

    return 0;
}
#endif
