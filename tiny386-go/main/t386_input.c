#include "t386_input.h"
#include "rg_input.h"
#include <string.h>
#include <ctype.h>
#include <stddef.h>

static const uint32_t bits[8]={RG_KEY_UP,RG_KEY_RIGHT,RG_KEY_DOWN,RG_KEY_LEFT,RG_KEY_A,RG_KEY_B,RG_KEY_START,RG_KEY_SELECT};
static int mapping[8];
static uint32_t held;
static uint64_t start_since;
static bool start_down,start_chord,start_opened;
static char text_queue[256];
static size_t text_pos;
static int phase, text_key, text_shift;
static bool key_pending;
static int queued_key;
static unsigned queued_modifiers,pressed_modifiers;
static const int modifier_keys[]={29,56,42};
static const char plain[]="1234567890-=qwertyuiop[]asdfghjkl;'`\\zxcvbnm,./ ";
static const char shifted[]="!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"~|ZXCVBNM<>? ";
static const unsigned char scan[]={2,3,4,5,6,7,8,9,10,11,12,13,16,17,18,19,20,21,22,23,24,25,26,27,30,31,32,33,34,35,36,37,38,39,40,41,43,44,45,46,47,48,49,50,51,52,53,57};
static int char_key(unsigned char c, int *shift) {
    const char *p;
    *shift=0;
    if(c=='\n'||c=='\r') return 28;
    if(c=='\t') return 15;
    if(c=='\b') return 14;
    if(c==27) return 1;
    if((p=strchr(plain,c)) && c) return scan[p-plain];
    if((p=strchr(shifted,c)) && c) { *shift=1; return scan[p-shifted]; }
    return -1;
}
int t386_keycode(const char *name) {
    static const struct {const char *name; int code;} names[]={
        {"none",0},{"up",103},{"right",106},{"down",108},{"left",105},
        {"ctrl",29},{"alt",56},{"shift",42},{"enter",28},{"escape",1},
        {"space",57},{"tab",15},{"backspace",14},{"comma",51},{"f1",59},{"f2",60},
        {"f3",61},{"f4",62},{"f5",63},{"f6",64},{"f7",65},{"f8",66},
        {"f9",67},{"f10",68},{"f11",87},{"f12",88},{"home",102},{"end",107},{"pageup",104},
        {"pagedown",109},{"insert",110},{"delete",111}
    };
    char lower[32]; size_t n=strlen(name);
    if(n>=sizeof(lower)) return -1;
    for(size_t i=0;i<=n;i++) lower[i]=(char)tolower((unsigned char)name[i]);
    for(size_t i=0;i<sizeof(names)/sizeof(*names);i++) if(!strcmp(lower,names[i].name)) return names[i].code;
    int shift;
    int code=n==1 ? char_key((unsigned char)lower[0],&shift) : -1;
    return code>=0 && !shift ? code : -1;
}
void t386_input_init(const int keycodes[8]) {
    memcpy(mapping,keycodes,sizeof(mapping)); held=0; start_down=start_chord=start_opened=false; text_queue[0]=0; text_pos=0; phase=0;
    key_pending=false;pressed_modifiers=0;
}
void t386_input_set_mapping(const int keycodes[8]) {
    t386_input_release_all();memcpy(mapping,keycodes,sizeof(mapping));
}
uint32_t t386_input_shortcuts(uint32_t buttons,uint64_t now,bool *open) {
    *open=false;
    if(buttons&RG_KEY_START) {
        if(!start_down) {start_down=true;start_since=now;start_chord=false;start_opened=false;}
        if(buttons&~(uint32_t)RG_KEY_START)start_chord=true;
        if(start_opened)return buttons&~(uint32_t)RG_KEY_START;
        if(start_chord)return buttons;
        if(now-start_since>=700000) {*open=true;start_opened=true;}
        return buttons&~(uint32_t)RG_KEY_START;
    }
    if(start_down&&!start_chord&&!start_opened)buttons|=RG_KEY_START;
    start_down=start_chord=start_opened=false;
    return buttons;
}
bool t386_input_pending(void) {return key_pending || text_queue[text_pos]!=0;}
void t386_input_poll(uint32_t buttons) {
    if(t386_input_pending()) return;
    for(int i=0;i<8;i++) {
        if(!mapping[i]) continue;
        bool duplicate=false, before=false, after=false;
        for(int j=0;j<i;j++) if(mapping[j]==mapping[i]) duplicate=true;
        if(duplicate) continue;
        for(int j=0;j<8;j++) if(mapping[j]==mapping[i]) {
            before|=(held & bits[j])!=0; after|=(buttons & bits[j])!=0;
        }
        if(before!=after) t386_input_emit(mapping[i],after);
    }
    held=buttons;
}
void t386_input_release_all(void) {
    start_down=start_chord=start_opened=false;
    if(key_pending) {
        if(phase==2)t386_input_emit(queued_key,0);
        for(int i=2;i>=0;i--)if(pressed_modifiers&(1u<<i))t386_input_emit(modifier_keys[i],0);
    } else {
        if(phase==2) t386_input_emit(text_key,0);
        if(phase && text_shift) t386_input_emit(42,0);
    }
    key_pending=false;pressed_modifiers=0;
    phase=0; text_queue[0]=0; text_pos=0;
    t386_input_poll(0);
}
bool t386_input_queue_text(const char *text) {
    if(!text || strlen(text)>=sizeof(text_queue) || t386_input_pending()) return false;
    for(const unsigned char *p=(const unsigned char*)text;*p;p++) {int s; if(char_key(*p,&s)<0) return false;}
    t386_input_release_all(); strcpy(text_queue,text); text_pos=0; phase=0; return true;
}
bool t386_input_queue_key(int key,unsigned modifiers) {
    if(key<=0 || key>=128 || (modifiers&~7u) || t386_input_pending())return false;
    /* A modifier must not be pressed twice when it is also the selected key. */
    for(unsigned i=0;i<3;i++)if(key==modifier_keys[i]&&(modifiers&(1u<<i)))return false;
    t386_input_release_all();queued_key=key;queued_modifiers=modifiers;
    key_pending=true;phase=0;return true;
}
void t386_input_step(void) {
    if(key_pending) {
        if(!phase) {
            for(unsigned i=0;i<3;i++)if(queued_modifiers&(1u<<i))t386_input_emit(modifier_keys[i],1);
            pressed_modifiers=queued_modifiers;phase=1;
            if(pressed_modifiers)return;
        }
        if(phase==1) {t386_input_emit(queued_key,1);phase=2;}
        else if(phase==2) {t386_input_emit(queued_key,0);phase=3;}
        else {
            for(int i=2;i>=0;i--)if(pressed_modifiers&(1u<<i))t386_input_emit(modifier_keys[i],0);
            key_pending=false;pressed_modifiers=0;phase=0;
        }
        return;
    }
    if(!text_queue[text_pos]) return;
    if(!phase) {
        text_key=char_key((unsigned char)text_queue[text_pos],&text_shift);
        if(text_shift) {t386_input_emit(42,1); phase=1; return;}
        phase=1;
    }
    if(phase==1) {t386_input_emit(text_key,1); phase=2;}
    else if(phase==2) {t386_input_emit(text_key,0); phase=3; if(!text_shift) {phase=0; text_pos++;}}
    else {t386_input_emit(42,0); phase=0; text_pos++;}
}
