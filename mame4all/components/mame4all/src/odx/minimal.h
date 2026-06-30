#ifdef __cplusplus
extern "C" {
#endif
/*

  GP2X minimal library v0.A by rlyeh, (c) 2005. emulnation.info@rlyeh (swap it!)

  Thanks to Squidge, Robster, snaff, Reesy and NK, for the help & previous work! :-)

  License
  =======

  Free for non-commercial projects (it would be nice receiving a mail from you).
  Other cases, ask me first.

  GamePark Holdings is not allowed to use this library and/or use parts from it.

*/
#ifndef __MINIMAL_H__
#define __MINIMAL_H__

#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#include <SDL/SDL.h>

#define ODX_SCREEN_WIDTH 320
#define ODX_SCREEN_HEIGHT 480

#define odx_video_color8(C,R,G,B)  (odx_palette_rgb[C] = ((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3)))
#define odx_video_color16(R,G,B,A) ((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3))
#define odx_video_getr16(C) (((C)>>8)&0xF8)
#define odx_video_getg16(C) (((C)>>3)&0xFC)
#define odx_video_getb16(C) (((C)<<3)&0xF8)

enum  { OD_UP=1<<0,         OD_LEFT=1<<1,       OD_DOWN=1<<2,  OD_RIGHT=1<<3,
        OD_START=1<<4,  OD_SELECT=1<<5,    OD_L=1<<6,    OD_R=1<<7,
        OD_A=1<<8,       OD_B=1<<9,        OD_X=1<<10,    OD_Y=1<<11 ,

	/* Virtual keys * SELECT + 'button' */
        OD_SEL_START=1<<12,  OD_SEL_SELECT=1<<13,    OD_SEL_L=1<<14,    OD_SEL_R=1<<15,
        OD_SEL_A=1<<16,       OD_SEL_B=1<<17,        OD_SEL_X=1<<18,    OD_SEL_Y=1<<19
};

#define OD_KEY_MAX 16

//extern SDL_Surface 				*layer,*video;
extern SDL_Surface 				*video;
extern unsigned char			*od_screen8;
extern unsigned short			*od_screen16;

extern volatile unsigned short	odx_palette[512];
extern unsigned short			odx_palette_rgb[256];

extern int						odx_clock;

extern unsigned int				odx_sound_rate;
extern int						odx_sound_stereo;

extern int						rotate_controls;

#ifdef _GCW0_
extern SDL_Joystick				*odx_joyanalog;
#endif

extern void odx_video_flip(void);
extern void odx_video_flip_single(void);
extern void odx_video_flip_double(void);
extern void odx_video_wait_vsync(void);
extern void odx_video_setpalette(void);

extern unsigned int odx_joystick_read();
extern unsigned int odx_joystick_press();

extern void odx_sound_volume(int vol);

extern void odx_timer_delay(unsigned int ticks);
extern unsigned long odx_timer_read(void);

extern void odx_sound_play(void *buff, int len);
extern void odx_sound_thread_start(void);
extern void odx_sound_thread_stop(void);

extern void odx_init(int ticks_per_second, int bpp, int rate, int bits, int stereo, int Hz);
extern void odx_deinit(void);

extern void odx_set_clock(int mhz);
extern void odx_set_video_mode(int bpp,int width,int height);
extern void odx_clear_video();

extern void odx_printf(char* fmt, ...);
extern void odx_printf_init(void);
extern void odx_gamelist_text_out(int x, int y, char *eltexto);
extern void odx_gamelist_text_out_fmt(int x, int y, char* fmt, ...);

extern int abs_x, abs_y, abs_z;

#define odx_video_wait_vsync()  { }
#define odx_video_setpalette()  { }

#define USE_DMA

#ifdef USE_DMA
extern volatile uint16_t *dma_ptr;
#endif

#endif
#ifdef __cplusplus
}
#endif
