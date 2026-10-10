#pragma once
#include <rg_system.h>

#define Q2_WIDTH 320
#define Q2_HEIGHT 240
#define Q2_RATE 22050

void q2_video_init(void);
void q2_video_redraw(void);
bool q2_video_screenshot(const char *filename, int width, int height);
void q2_video_enable(bool enabled);
int q2_video_take_wait(void);
void q2_input_poll(void);
void q2_input_release(void);
void q2_input_configure(void);
void q2_audio_pause(bool paused);
void q2_menu(bool options);
const char *QG_WriteDirectory(void);
const char *QG_ConfigDirectory(void);
void QG_LoadProgress(void);
_Noreturn void QG_Quit(void);
_Noreturn void QG_Error(const char *message);
