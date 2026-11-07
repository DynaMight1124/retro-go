#include "t386_controls.h"
#include "t386_input.h"
#include "rg_system.h"
#include <stdio.h>
#include <string.h>

static int ini_keys[8],current[8];
static char overrides[8][16],settings_key[48];

static void save(void) {
    char value[8*16]={0};size_t used=0;bool any=false;
    for(unsigned i=0;i<8;i++) {
        if(i)value[used++]=',';
        size_t n=strlen(overrides[i]);memcpy(value+used,overrides[i],n);used+=n;
        any|=n!=0;
    }
    if(any)rg_settings_set_string(settings_key,"keys",value);
    else rg_settings_delete(settings_key,"keys");
}
void t386_controls_init(const char *game_path,const int defaults[8]) {
    memcpy(ini_keys,defaults,sizeof(ini_keys));memcpy(current,defaults,sizeof(current));
    memset(overrides,0,sizeof(overrides));
    /* Full-path identity avoids collisions between two directories' game.ini.
     * The platform owns each profile file; only the active game is loaded. */
    uint64_t hash=UINT64_C(14695981039346656037);
    for(const unsigned char *p=(const unsigned char*)game_path;*p;p++) {
        hash^=*p;hash*=UINT64_C(1099511628211);
    }
    snprintf(settings_key,sizeof(settings_key),"tiny386-controls-%08x%08x",(unsigned)(hash>>32),(unsigned)hash);
    char *value=rg_settings_get_string(settings_key,"keys",NULL);
    if(value) {
        int parsed[8];memcpy(parsed,defaults,sizeof(parsed));
        char names[8][16]={{0}},*field=value;bool valid=true;
        for(unsigned i=0;i<8;i++) {
            char *comma=strchr(field,',');
            if((i<7&&!comma)||(i==7&&comma)) {valid=false;break;}
            if(comma)*comma=0;
            if(*field) {
                int code=t386_keycode(field);
                if(code<0 || strlen(field)>=sizeof(names[i])) {valid=false;break;}
                parsed[i]=code;strcpy(names[i],field);
            }
            if(comma)field=comma+1;
        }
        if(valid) {memcpy(current,parsed,sizeof(current));memcpy(overrides,names,sizeof(overrides));}
        free(value);
    }
    t386_input_release_all();t386_input_init(current);
}
int t386_controls_get(unsigned button) {return button<8?current[button]:-1;}
bool t386_controls_set(unsigned button,const char *key_name) {
    if(key_name && !strcmp(key_name,","))key_name="comma";
    if(button>=8 || !key_name || strlen(key_name)>=sizeof(overrides[0]))return false;
    int code=t386_keycode(key_name);if(code<0)return false;
    current[button]=code;
    strcpy(overrides[button],code==ini_keys[button]?"":key_name);
    t386_input_set_mapping(current);save();return true;
}
void t386_controls_reset(void) {
    memcpy(current,ini_keys,sizeof(current));memset(overrides,0,sizeof(overrides));
    t386_input_set_mapping(current);save();
}
