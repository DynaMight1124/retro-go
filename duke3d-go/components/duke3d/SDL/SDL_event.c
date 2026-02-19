#include "SDL_event.h"
#include <rg_system.h>
#include "rg_input.h"
#include "SDL_scancode.h"
#include "controls.h"
#include "duke3d.h"

int keyMode = 1;

static uint32_t last_map_state[64]; // Track up to 64 mapping entries
static int current_map_idx = 0;

int SDL_PollEvent(SDL_Event * event)
{
    if (event) memset(event, 0, sizeof(SDL_Event));
    uint32_t rg_state = rg_input_read_gamepad();
    
    // Check if we are in menu mode
    extern struct player_struct ps[];
    extern short myconnectindex;
    bool in_menu = (ps[myconnectindex].gm & 1); // MODE_MENU is 1, not 2!

    static bool last_in_menu = false;
    if (in_menu != last_in_menu) {
        last_in_menu = in_menu;
    }

    while (current_map_idx < keymap_count && current_map_idx < 64) {
        const key_mapping_t *m = &keymap[current_map_idx];
        
        // Skip mappings that don't match our current context
        if (m->mode == RG_MODE_GAME && in_menu) {
            current_map_idx++;
            continue;
        }
        if (m->mode == RG_MODE_MENU && !in_menu) {
            current_map_idx++;
            continue;
        }

        bool is_pressed = (rg_state & m->rg_key);
        bool was_pressed = last_map_state[current_map_idx];

        if (is_pressed != was_pressed) {
            last_map_state[current_map_idx] = is_pressed;
            if (is_pressed) {
                event->type = SDL_KEYDOWN;
                event->key.state = SDL_PRESSED;
                event->key.keysym.scancode = m->scancode;
                event->key.keysym.sym = m->keycode;
                return 1;
            } else {
                event->type = SDL_KEYUP;
                event->key.state = SDL_RELEASED;
                event->key.keysym.scancode = m->scancode;
                event->key.keysym.sym = m->keycode;
                return 1;
            }
        }
        current_map_idx++;
    }

    current_map_idx = 0;
    return 0;
}

void inputInit()
{
	printf("keyboard: retro-go input initialized.\n");
}
