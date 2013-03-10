/*

  GP2X minimal library v0.A by rlyeh, (c) 2005. emulnation.info@rlyeh (swap it!)

  Thanks to Squidge, Robster, snaff, Reesy and NK, for the help & previous work! :-)

  License
  =======

  Free for non-commercial projects (it would be nice receiving a mail from you).
  Other cases, ask me first.

  GamePark Holdings is not allowed to use this library and/or use parts from it.

*/

#include <fcntl.h>
#include <linux/fb.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include "usbjoy_mame.h"

#ifndef __MINIMAL_H__
#define __MINIMAL_H__

//SQ Pi uses web safe 8 bit palette
#define gp2x_video_color8(C,R,G,B) (gp2x_palette[C]=(36*(R/51)+6*(G/51)+(B/51)))
#define gp2x_video_color15(R,G,B,A) ((((R)&0xF8)<<8)|(((G)&0xF8)<<3)|(((B)&0xF8)>>3)|((A)<<5))

//sqdebug #define gp2x_video_color8(C,R,G,B) (gp2x_palette[((C)<<1)+0]=((G)<<8)|(B),gp2x_palette[((C)<<1)+1]=(R))

#define gp2x_video_getr15(C) (((C)>>8)&0xF8)
#define gp2x_video_getg15(C) (((C)>>3)&0xF8)
#define gp2x_video_getb15(C) (((C)<<3)&0xF8)

enum  { GP2X_UP=1<<0,       GP2X_LEFT=1<<1,       GP2X_DOWN=1<<2,  GP2X_RIGHT=1<<3,
        GP2X_LCTRL=1<<4,        GP2X_LALT=1<<5,    GP2X_SPACE=1<<6, GP2X_LSHIFT=1<<7,
		GP2X_RETURN=1<<8,	GP2X_TAB=1<<9, GP2X_ESCAPE=1<<10,	GP2X_TILDE=1<<11,
        GP2X_1=1<<12,	GP2X_5=1<<13,    GP2X_2=1<<14,    GP2X_6=1<<15,
		GP2X_T=1<<16,	GP2X_O=1<<17, 	GP2X_K=1<<18,
		GP2X_F3=1<<19,	   GP2X_F5=1<<20,  GP2X_F10=1<<21, GP2X_F11=1<<22,
        GP2X_9=1<<23, GP2X_P=1<<24 };
                                            
extern volatile unsigned short 	gp2x_palette[512];
extern unsigned char 		*gp2x_screen8;
extern unsigned short 		*gp2x_screen15;

extern void gp2x_init(int tickspersecond, int bpp, int rate, int bits, int stereo, int hz, int caller);
extern void gp2x_deinit(void);
extern void gp2x_timer_delay(unsigned long ticks);
extern void gp2x_video_flip(void);
extern void gp2x_video_flip_single(void);
extern void gp2x_video_setpalette(void);
extern void gp2x_joystick_clear(void);
extern unsigned long gp2x_joystick_read(void);
extern unsigned long gp2x_joystick_press (void);
extern unsigned long gp2x_timer_read(void);
//sq extern unsigned long gp2x_timer_read_real(void);
//sq extern unsigned long gp2x_timer_read_scale(void);
//sq extern void gp2x_timer_profile(void);

extern void gp2x_set_video_mode(int bpp,int width,int height);
extern void gp2x_set_clock(int mhz);

extern void gp2x_printf(char* fmt, ...);
extern void gp2x_printf_init(void);
extern void gp2x_gamelist_text_out(int x, int y, char *eltexto, int color);
extern void gp2x_gamelist_text_out_fmt(int x, int y, char* fmt, ...);

extern void DisplayScreen(void);

extern void gp2x_frontend_init(void);
extern void gp2x_frontend_deinit(void);

extern void deinit_SDL(void);
extern int init_SDL(void);

#endif
