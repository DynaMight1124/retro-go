#include "t386_ui.h"
#include "t386_controls.h"
#include "t386_input.h"
#include "rg_input.h"
#include <stdio.h>
static AdlibState *fm_chip;
static T386Video *video_policy;
void t386_ui_set_video(T386Video *video) {
    video_policy=video;
    double saved=rg_settings_get_number(NS_APP,"renderLimit",0);
    unsigned fps=saved==20?20:saved==15?15:saved==10?10:0;
    if(video_policy)t386_video_set_limit(video_policy,fps);
}
void t386_ui_init(AdlibState *chip) {
    /* The display setter persists its value; only seed an absent preference. */
    if(rg_settings_get_number(NS_APP,"DispScaling",-1)<0)
        rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);
    fm_chip=chip;
    /* Preserve the existing FM preference: AdLib=true, PC Speaker=false. */
    adlib_set_enabled(fm_chip,rg_settings_get_boolean(NS_APP,"fmSynthesis",true));
}
bool t386_ui_unsupported_state(const char *path) {
    (void)path;
    rg_gui_alert(_("Not supported"),_("VM save/load is unavailable. Use in-game saves; they are stored in the disk image."));
    return false;
}
static void (*keyboard_handler)(void);
static void (*special_handler)(int,unsigned);
void t386_ui_set_keyboard_handler(void (*handler)(void)) {keyboard_handler=handler;}
void t386_ui_set_special_handler(void (*handler)(int,unsigned)) {special_handler=handler;}
static rg_gui_event_t keyboard(rg_gui_option_t *opt,rg_gui_event_t id) {
    (void)opt;
    if(id!=RG_DIALOG_ENTER)return RG_DIALOG_VOID;
    if(keyboard_handler)keyboard_handler();
    return RG_DIALOG_REDRAW;
}
/* Special cells: space, Shift, Backspace, Enter, Escape. All input goes through
 * the normal i8042 make/break path, never DOS memory or a BIOS text buffer. */
static const char keyboard_lower[]="1234567890" "qwertyuiop" "asdfghjkl;" "zxcvbnm,.?" " ^<EX-=\\/:";
static const char keyboard_upper[]="!@#$%^&*()" "QWERTYUIOP" "ASDFGHJKL:" "ZXCVBNM<>!" " ^<EX_+|/\"";
_Static_assert(sizeof(keyboard_lower)==51 && sizeof(keyboard_upper)==51,"Keyboard grid size");
static void draw_keyboard(int cursor,bool shift) {
    rg_keyboard_layout_t layout={shift?keyboard_upper:keyboard_lower,10,5,"DOS"};
    static const char *names[]={"Space","Shift","Backspace","Enter","Escape"};
    char title[64];
    if(cursor>=40&&cursor<=44)
        snprintf(title,sizeof(title),"DOS: %s | A:type B:close",_(names[cursor-40]));
    else snprintf(title,sizeof(title),"DOS: %c | A:type B:close",layout.layout[cursor]);
    rg_gui_draw_text(RG_GUI_CENTER,RG_GUI_TOP,rg_display_get_width(),title,0xffff,0,RG_TEXT_ALIGN_CENTER);
    rg_gui_draw_virtual_keyboard(RG_GUI_CENTER,RG_GUI_BOTTOM,&layout,cursor,false);
}
static void keyboard_release(void) {
    /* No timeout: a held A must never type a second character or leak to DOS. */
    while(rg_input_read_gamepad()) {rg_system_tick(0);rg_task_delay(10);}
}
int t386_ui_keyboard_pick(int *cursor,bool *shift) {
    if(*cursor<0||*cursor>=50)*cursor=0;
    draw_keyboard(*cursor,*shift);
    keyboard_release();
    for(;;) {
        uint32_t keys=rg_input_read_gamepad();
        int next=*cursor;
        if(keys&(RG_KEY_B|RG_KEY_MENU|RG_KEY_OPTION))return -1;
        if(keys&RG_KEY_START)return '\n';
        if(keys&RG_KEY_SELECT) {*shift=!*shift;draw_keyboard(*cursor,*shift);}
        else if(keys&RG_KEY_A) {
            if(*cursor==41) {*shift=!*shift;draw_keyboard(*cursor,*shift);}
            else if(*cursor==42)return '\b';
            else if(*cursor==43)return '\n';
            else if(*cursor==44)return 27;
            else if(*cursor==40)return ' ';
            else return (*shift?keyboard_upper:keyboard_lower)[*cursor];
        }
        else if(keys&RG_KEY_LEFT) {if(*cursor%10)next--;}
        else if(keys&RG_KEY_RIGHT) {if(*cursor%10<9)next++;}
        else if(keys&RG_KEY_UP)next-=10;
        else if(keys&RG_KEY_DOWN)next+=10;
        if(next>=0&&next<50&&next!=*cursor) {*cursor=next;draw_keyboard(*cursor,*shift);}
        if(keys&(RG_KEY_A|RG_KEY_SELECT))keyboard_release();
        else if(keys)rg_input_wait_for_key(RG_KEY_ALL,false,150);
        rg_system_tick(0);rg_task_delay(10);
    }
}
/* Canonical key names are also accepted by the INI reader. No guest changes
 * are made while browsing: only an explicit selection updates the mapping. */
static const char *key_choices[]={
    "none","up","right","down","left","ctrl","alt","shift","enter","escape",
    "space","tab","backspace","f1","f2","f3","f4","f5","f6","f7","f8","f9","f10","f11","f12",
    "home","end","pageup","pagedown","insert","delete",
    "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z",
    "0","1","2","3","4","5","6","7","8","9","-","=","[","]",";","'","`","\\","comma",".","/"
};
#define KEY_COUNT ((int)(sizeof(key_choices)/sizeof(key_choices[0])))
static int key_index(int code) {
    for(int i=0;i<KEY_COUNT;i++)if(t386_keycode(key_choices[i])==code)return i;
    return 0;
}
static int choose_key(int current) {
    rg_gui_option_t *options=calloc(KEY_COUNT+1,sizeof(*options));
    if(!options) {rg_gui_alert(_("Controls"),_("Not enough memory to open the key list."));return -1;}
    for(int i=0;i<KEY_COUNT;i++)options[i]=(rg_gui_option_t){i,_(key_choices[i]),NULL,RG_DIALOG_FLAG_NORMAL,NULL};
    intptr_t result=rg_gui_dialog(_("Keyboard key"),options,key_index(current));
    free(options);return result>=0&&result<KEY_COUNT?(int)result:-1;
}
static rg_gui_event_t mapping_option(rg_gui_option_t *opt,rg_gui_event_t id) {
    unsigned button=(unsigned)opt->arg;int index=key_index(t386_controls_get(button));
    if(id==RG_DIALOG_ENTER)index=choose_key(t386_controls_get(button));
    else if(id==RG_DIALOG_PREV)index=(index+KEY_COUNT-1)%KEY_COUNT;
    else if(id==RG_DIALOG_NEXT)index=(index+1)%KEY_COUNT;
    if(index>=0 && (id==RG_DIALOG_ENTER || id==RG_DIALOG_PREV || id==RG_DIALOG_NEXT))
        t386_controls_set(button,key_choices[index]);
    opt->value=(char*)_(key_choices[key_index(t386_controls_get(button))]);
    return id==RG_DIALOG_ENTER||id==RG_DIALOG_PREV||id==RG_DIALOG_NEXT?RG_DIALOG_REDRAW:RG_DIALOG_VOID;
}
static rg_gui_event_t controls(rg_gui_option_t *opt,rg_gui_event_t id) {
    (void)opt;if(id!=RG_DIALOG_ENTER)return RG_DIALOG_VOID;
    static const char *buttons[]={"Up","Right","Down","Left","A","B","Start","Select"};
    int selected=0;
    for(;;) {
        rg_gui_option_t options[11]={0};
        for(unsigned i=0;i<8;i++)options[i]=(rg_gui_option_t){i,_(buttons[i]),
            (char*)_(key_choices[key_index(t386_controls_get(i))]),RG_DIALOG_FLAG_NORMAL,mapping_option};
        options[8]=(rg_gui_option_t){8,_("Reset to INI defaults"),NULL,RG_DIALOG_FLAG_NORMAL,NULL};
        options[9]=(rg_gui_option_t){0,_("Hold Start: keyboard"),NULL,RG_DIALOG_FLAG_MESSAGE,NULL};
        if(rg_gui_dialog(_("Remap Controls"),options,selected)!=8)break;
        t386_controls_reset();selected=8;
    }
    return RG_DIALOG_REDRAW;
}
static const struct {const char *label,*key;unsigned modifiers;} special_keys[]={
    {"Escape","escape",0},{"Tab","tab",0},{"Space","space",0},{"Enter","enter",0},{"Backspace","backspace",0},
    {"F1","f1",0},{"F2","f2",0},{"F3","f3",0},{"F4","f4",0},{"F5","f5",0},{"F6","f6",0},
    {"F7","f7",0},{"F8","f8",0},{"F9","f9",0},{"F10","f10",0},{"F11","f11",0},{"F12","f12",0},
    {"Ctrl+C","c",T386_MOD_CTRL},{"Ctrl+Q","q",T386_MOD_CTRL},
    {"Alt+Enter","enter",T386_MOD_ALT},{"Shift+Tab","tab",T386_MOD_SHIFT}
};
static rg_gui_event_t special(rg_gui_option_t *opt,rg_gui_event_t id) {
    (void)opt;if(id!=RG_DIALOG_ENTER)return RG_DIALOG_VOID;
    unsigned count=sizeof(special_keys)/sizeof(special_keys[0]);
    rg_gui_option_t *options=calloc(count+1,sizeof(*options));
    if(!options) {rg_gui_alert(_("Special keys"),_("Not enough memory to open the key list."));return RG_DIALOG_REDRAW;}
    for(unsigned i=0;i<count;i++)options[i]=(rg_gui_option_t){i,_(special_keys[i].label),NULL,RG_DIALOG_FLAG_NORMAL,NULL};
    intptr_t selected=rg_gui_dialog(_("Special keys"),options,0);free(options);
    if(selected>=0&&(unsigned)selected<count&&special_handler)
        special_handler(t386_keycode(special_keys[selected].key),special_keys[selected].modifiers);
    return RG_DIALOG_REDRAW;
}
static rg_gui_event_t audio_mode(rg_gui_option_t *opt,rg_gui_event_t id) {
    if(id==RG_DIALOG_PREV || id==RG_DIALOG_NEXT || id==RG_DIALOG_ENTER) {
        adlib_set_enabled(fm_chip,!adlib_get_enabled(fm_chip));
        rg_settings_set_boolean(NS_APP,"fmSynthesis",adlib_get_enabled(fm_chip)!=0);
        RG_LOGI("Audio: %s (Sound Blaster digital remains enabled)",adlib_get_enabled(fm_chip)?"AdLib":"PC Speaker");
        opt->value=(char *)(adlib_get_enabled(fm_chip)?_("AdLib"):_("PC Speaker"));
        return RG_DIALOG_REDRAW;
    }
    opt->value=(char *)(adlib_get_enabled(fm_chip)?_("AdLib"):_("PC Speaker"));
    return RG_DIALOG_VOID;
}
static rg_gui_event_t render_limit(rg_gui_option_t *opt,rg_gui_event_t id) {
    static const unsigned caps[]={0,20,15,10};
    static const char *labels[]={"Auto","20 fps","15 fps","10 fps"};
    unsigned index=0;
    for(unsigned i=0;video_policy&&i<4;i++)if(caps[i]==video_policy->limit_fps)index=i;
    if(video_policy && (id==RG_DIALOG_PREV || id==RG_DIALOG_NEXT || id==RG_DIALOG_ENTER)) {
        index=(index+(id==RG_DIALOG_PREV?3:1))%4;
        t386_video_set_limit(video_policy,caps[index]);
        rg_settings_set_number(NS_APP,"renderLimit",caps[index]);
        opt->value=(char*)_(labels[index]);return RG_DIALOG_REDRAW;
    }
    opt->value=(char*)_(labels[index]);return RG_DIALOG_VOID;
}
void t386_ui_options(rg_gui_option_t *dest) {
    *dest++=(rg_gui_option_t){0,_("Show Keyboard"),NULL,RG_DIALOG_FLAG_NORMAL,keyboard};
    *dest++=(rg_gui_option_t){0,_("Remap Controls"),NULL,RG_DIALOG_FLAG_NORMAL,controls};
    *dest++=(rg_gui_option_t){0,_("Special keys"),NULL,RG_DIALOG_FLAG_NORMAL,special};
    /* RetroGo initializes callback values only when the initial value is non-null. */
    *dest++=(rg_gui_option_t){0,_("Audio"),"-",RG_DIALOG_FLAG_NORMAL,audio_mode};
    *dest++=(rg_gui_option_t){0,_("Render limit"),"-",RG_DIALOG_FLAG_NORMAL,render_limit};
    *dest=(rg_gui_option_t)RG_DIALOG_END;
}
