#include "SDL_event.h"
#include "SDL_audio.h"
#include "SDL_video.h"
#include <rg_system.h>
#include <rg_input.h>
#include <rg_gui.h>
#include <string.h>

#define EVENT_QUEUE_SIZE 32

static SDL_Event event_queue[EVENT_QUEUE_SIZE];
static int eq_head = 0;
static int eq_tail = 0;

static void queue_key_event(SDL_Keycode key, uint8_t state)
{
    int next = (eq_head + 1) % EVENT_QUEUE_SIZE;
    if (next == eq_tail)
        return;

    SDL_Event *ev = &event_queue[eq_head];
    memset(ev, 0, sizeof(SDL_Event));
    ev->type = (state == SDL_PRESSED) ? SDL_KEYDOWN : SDL_KEYUP;
    ev->key.type = ev->type;
    ev->key.state = state;
    ev->key.keysym.sym = key;
    ev->key.keysym.scancode = (SDL_Scancode)key;
    ev->key.keysym.mod = 0;

    eq_head = next;
}

bool link_sidekicks_to_a = false;
bool opentyrian_in_gameplay = false;
extern bool pause_pressed, ingamemenu_pressed;

static const struct {
    uint32_t mask;
    SDL_Keycode key;
} keymap[] = {
    { RG_KEY_UP,     SDLK_UP },
    { RG_KEY_DOWN,   SDLK_DOWN },
    { RG_KEY_LEFT,   SDLK_LEFT },
    { RG_KEY_RIGHT,  SDLK_RIGHT },
    { RG_KEY_A,      SDLK_SPACE },   // Front Weapon / Menu Confirm
    { RG_KEY_B,      SDLK_RETURN },  // Rear Weapon (in-flight) / Back out (in menus)
    { RG_KEY_SELECT, SDLK_LCTRL },   // Left Sidekick
    { RG_KEY_START,  SDLK_LALT },    // Right Sidekick
    { RG_KEY_X,      SDLK_LCTRL },   // Left Sidekick (4/6 button devices)
    { RG_KEY_Y,      SDLK_LALT },    // Right Sidekick (4/6 button devices)
    { RG_KEY_L,      SDLK_LCTRL },   // Left Sidekick (Shoulder button)
    { RG_KEY_R,      SDLK_LALT },    // Right Sidekick (Shoulder button)
};

int SDL_PollEvent(SDL_Event *event)
{
    static uint32_t prev_buttons = 0;
    static int64_t menu_press_time = 0;
    static bool menu_is_down = false;
    static bool menu_long_fired = false;

    static int64_t option_press_time = 0;
    static bool option_is_down = false;
    static bool option_long_fired = false;

    static SDL_Keycode b_active_key = SDLK_UNKNOWN;

    int64_t now = rg_system_timer();

    if (eq_tail != eq_head)
    {
        *event = event_queue[eq_tail];
        eq_tail = (eq_tail + 1) % EVENT_QUEUE_SIZE;
        return 1;
    }

    uint32_t buttons = rg_input_read_gamepad();

    // MENU button: Short tap = Esc (In-game menu/Cancel), Long hold (>=450ms) = Retro-Go game menu
    if (buttons & RG_KEY_MENU)
    {
        if (!menu_is_down)
        {
            menu_is_down = true;
            menu_long_fired = false;
            menu_press_time = now;
        }
        else if (!menu_long_fired && (now - menu_press_time >= 450000))
        {
            menu_long_fired = true;
            bool resume_async_flip = opentyrian_in_gameplay;
            SDL_SetAsyncFlip(false);
            SDL_PauseAudio(1);
            rg_gui_game_menu();
            rg_input_wait_for_key(RG_KEY_MENU | RG_KEY_OPTION, false, 500);
            prev_buttons = rg_input_read_gamepad();
            menu_is_down = false;
            SDL_PauseAudio(0);
            SDL_ResetFrameTime();
            SDL_SetAsyncFlip(resume_async_flip);
            return 0;
        }
    }
    else if (menu_is_down)
    {
        menu_is_down = false;
        if (!menu_long_fired && (now - menu_press_time >= 20000))
        {
            if (opentyrian_in_gameplay)
            {
                ingamemenu_pressed = true;
            }
            else
            {
                queue_key_event(SDLK_ESCAPE, SDL_PRESSED);
                queue_key_event(SDLK_ESCAPE, SDL_RELEASED);
            }
        }
    }

    // OPTION button: Short tap = P (Pause), Long hold (>=450ms) = Retro-Go options menu
    if (buttons & RG_KEY_OPTION)
    {
        if (!option_is_down)
        {
            option_is_down = true;
            option_long_fired = false;
            option_press_time = now;
        }
        else if (!option_long_fired && (now - option_press_time >= 450000))
        {
            option_long_fired = true;
            bool resume_async_flip = opentyrian_in_gameplay;
            SDL_SetAsyncFlip(false);
            SDL_PauseAudio(1);
            rg_gui_options_menu();
            rg_input_wait_for_key(RG_KEY_MENU | RG_KEY_OPTION, false, 500);
            prev_buttons = rg_input_read_gamepad();
            option_is_down = false;
            SDL_PauseAudio(0);
            SDL_ResetFrameTime();
            SDL_SetAsyncFlip(resume_async_flip);
            return 0;
        }
    }
    else if (option_is_down)
    {
        option_is_down = false;
        if (!option_long_fired && (now - option_press_time >= 20000))
        {
            if (opentyrian_in_gameplay)
            {
                pause_pressed = true;
            }
            else
            {
                queue_key_event(SDLK_p, SDL_PRESSED);
                queue_key_event(SDLK_p, SDL_RELEASED);
            }
        }
    }

    uint32_t action_buttons = buttons & ~(RG_KEY_MENU | RG_KEY_OPTION);
    uint32_t prev_action = prev_buttons & ~(RG_KEY_MENU | RG_KEY_OPTION);
    uint32_t changed = action_buttons ^ prev_action;

    if (changed)
    {
        for (int i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++)
        {
            if (changed & keymap[i].mask)
            {
                if (action_buttons & keymap[i].mask)
                {
                    if (keymap[i].mask == RG_KEY_B)
                    {
                        // In flight: B toggles rear weapon mode. In menus: B backs out/cancels.
                        b_active_key = opentyrian_in_gameplay ? SDLK_RETURN : SDLK_ESCAPE;
                        queue_key_event(b_active_key, SDL_PRESSED);
                    }
                    else
                    {
                        queue_key_event(keymap[i].key, SDL_PRESSED);

                        if (link_sidekicks_to_a && keymap[i].mask == RG_KEY_A)
                        {
                            queue_key_event(SDLK_LCTRL, SDL_PRESSED);
                            queue_key_event(SDLK_LALT, SDL_PRESSED);
                        }
                    }
                }
                else
                {
                    if (keymap[i].mask == RG_KEY_B)
                    {
                        if (b_active_key != SDLK_UNKNOWN)
                        {
                            queue_key_event(b_active_key, SDL_RELEASED);
                            b_active_key = SDLK_UNKNOWN;
                        }
                    }
                    else
                    {
                        bool still_down = false;
                        for (int j = 0; j < sizeof(keymap) / sizeof(keymap[0]); j++)
                        {
                            if (keymap[j].key == keymap[i].key && (action_buttons & keymap[j].mask))
                            {
                                still_down = true;
                                break;
                            }
                        }
                        if (!still_down)
                        {
                            queue_key_event(keymap[i].key, SDL_RELEASED);
                        }

                        if (link_sidekicks_to_a && keymap[i].mask == RG_KEY_A)
                        {
                            if (!(action_buttons & (RG_KEY_SELECT | RG_KEY_X | RG_KEY_L)))
                                queue_key_event(SDLK_LCTRL, SDL_RELEASED);
                            if (!(action_buttons & (RG_KEY_START | RG_KEY_Y | RG_KEY_R)))
                                queue_key_event(SDLK_LALT, SDL_RELEASED);
                        }
                    }
                }
            }
        }
    }

    prev_buttons = buttons;

    if (eq_tail != eq_head)
    {
        *event = event_queue[eq_tail];
        eq_tail = (eq_tail + 1) % EVENT_QUEUE_SIZE;
        return 1;
    }

    return 0;
}

void inputInit(void)
{
}
