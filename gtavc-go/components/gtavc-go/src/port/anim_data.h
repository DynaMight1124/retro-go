#ifndef GTAVC_ANIM_DATA_H
#define GTAVC_ANIM_DATA_H

#include <stdint.h>
#include <stdbool.h>

extern int AnimData_n;
extern char** AnimData_name;
extern int* AnimData_dur;
extern int* AnimData_nf;
extern int* AnimData_travel;
extern bool* AnimData_loop;
extern int16_t** AnimData_fr;

bool AnimData_load(void);
int  AnimData_find(const char* s);
int  AnimData_len(int c);
int  AnimData_speed(int c);
void AnimData_sample(int c, int tMs, int* out);
void AnimData_free(void);

#endif /* GTAVC_ANIM_DATA_H */
