/*


*/

#include "minimal.h"

#include <bcm_host.h>
#include <SDL.h>
#include <assert.h>

static SDL_Surface* sdlscreen = NULL;

unsigned long 			gp2x_dev[3];
unsigned char 			*gp2x_screen8;
unsigned short 			*gp2x_screen15;
unsigned int			gp2x_nflip;
volatile unsigned short 	gp2x_palette[512];
int 				rotate_controls=0;

static int surface_width;
static int surface_height;

#define MAX_SAMPLE_RATE (44100*2)

void gp2x_video_flip(void)
{
    DisplayScreen();
}


void gp2x_video_flip_single(void)
{
    DisplayScreen();
}


void gp2x_video_setpalette(void)
{
}

extern void keyprocess(SDLKey inkey, SDL_bool pressed);

extern unsigned long ExKey1;
extern unsigned long ExKey2;
extern unsigned long ExKey3;
extern unsigned long ExKey4;

static void joyprocess(Uint32 *st, Uint8 button, SDL_bool pressed, Uint8 njoy)
{
    Uint32 val=0;
	unsigned long *mykey;

	switch(njoy)
	{
		case 0:
			mykey = &ExKey1; break;
		case 1:
			mykey = &ExKey2; break;
		case 2:
			mykey = &ExKey3; break;
		case 3:
			mykey = &ExKey4; break;
		default:
			return;
	}
	
    switch(button)
    {
        case 0:
            val=GP2X_LCTRL; break;
        case 1:
            val=GP2X_LALT; break;
        case 2:
            val=GP2X_SPACE; break;
        case 3:
            val=GP2X_LSHIFT; break;
        case 4:
            val=GP2X_TAB; break;
        case 5:
            val=GP2X_RETURN; break;
//sq        case 6:
//sq            val=GP2X_TAB; break;
//sq        case 7:
//sq            val=GP2X_TILDE; break;
//sq        case 8:
//sq            val=GP2X_ESCAPE; break;
//sq        case 9:
//sq            val=GP2X_1; break;
//sq        case 10:
//sq            val=GP2X_5; break;
//sq        case 11: 
//sq            val=GP2X_P; break;
        case 129:
            val=GP2X_DOWN; break;
        case 130:
            val=GP2X_LEFT; break;
        case 131:
            val=GP2X_UP; break;
        case 132:
            val=GP2X_RIGHT; break;
        default:
            return;
    }
    if (pressed)
        (*mykey) |= val;
    else
        (*mykey) ^= val;

//sq    if (pressed)
//sq        (st[njoy]) |= val;
//sq    else
//sq        (st[njoy]) ^= val;
}


Uint32 _st_[4]={0,0,0,0};

void gp2x_joystick_clear(void)
{
    SDL_Event event;
    while(SDL_PollEvent(&event));
    _st_[0]= 0;
    _st_[1]= 0;
    _st_[2]= 0;
    _st_[3]= 0;
    
}

unsigned long gp2x_joystick_read()
{
  	unsigned long res=0;

    SDL_Event event;
//	if (n==1)
      while(SDL_PollEvent(&event)) {
        switch(event.type)
		{
			case SDL_KEYDOWN:
				keyprocess(event.key.keysym.sym, SDL_TRUE);
				break;
			case SDL_KEYUP:
				keyprocess(event.key.keysym.sym, SDL_FALSE);
				break;
            case SDL_JOYBUTTONDOWN:
                joyprocess((Uint32 *)&_st_,event.jbutton.button, SDL_TRUE, event.jbutton.which);
                break;
            case SDL_JOYBUTTONUP:
                joyprocess((Uint32 *)&_st_,event.jbutton.button, SDL_FALSE, event.jbutton.which);
                break;
            case SDL_JOYAXISMOTION:
                if (event.jaxis.axis==0)
                {
                    static int reset_xl[4]={0,0,0,0};
                    static int reset_xr[4]={0,0,0,0};
                    if (event.jaxis.value<-6000)
                    {
                        if (!reset_xl[event.jaxis.which]){
                            joyprocess((Uint32 *)&_st_,130, SDL_TRUE, event.jaxis.which);}
                        reset_xl[event.jaxis.which]=1;
                    }
                    else if (event.jaxis.value>6000)
                    {
                        if (!reset_xr[event.jaxis.which]){
                            joyprocess((Uint32 *)&_st_,132, SDL_TRUE, event.jaxis.which);}
                        reset_xr[event.jaxis.which]=1;
                    }
                    else
                    {
                        if (reset_xr[event.jaxis.which])
                            joyprocess((Uint32 *)&_st_,132, SDL_FALSE, event.jaxis.which);
                        reset_xr[event.jaxis.which]=0;
                        if (reset_xl[event.jaxis.which])
                            joyprocess((Uint32 *)&_st_,130, SDL_FALSE, event.jaxis.which);
                        reset_xl[event.jaxis.which]=0;
                    }
                }
                else if (event.jaxis.axis==1)
                {
                    static int reset_yu[4]={0,0,0,0};
                    static int reset_yd[4]={0,0,0,0};
                    if (event.jaxis.value<-6000)
                    {
                        if (!reset_yu[event.jaxis.which]){
                            joyprocess((Uint32 *)&_st_,131,SDL_TRUE, event.jaxis.which);}
                        reset_yu[event.jaxis.which]=1;
                    }
                    else if (event.jaxis.value>6000)
                    {
                        if (!reset_yd[event.jaxis.which]) {
                            joyprocess((Uint32 *)&_st_,129,SDL_TRUE, event.jaxis.which);}
                        reset_yd[event.jaxis.which]=1;
                    }
                    else
                    {
                        if (reset_yd[event.jaxis.which])
                            joyprocess((Uint32 *)&_st_,129,SDL_FALSE, event.jaxis.which);
                        reset_yd[event.jaxis.which]=0;
                        if (reset_yu[event.jaxis.which])
                            joyprocess((Uint32 *)&_st_,131,SDL_FALSE, event.jaxis.which);
                        reset_yu[event.jaxis.which]=0;
                    }
                }
                break;
		}
	  }
//sq	return _st_[n];

    
//sq	if (n==0)
//sq	{
//sq	  	unsigned long value=(gp2x_memregs[0x1198>>1] & 0x00FF);
//sq	  	if(value==0xFD) value=0xFA;
//sq	  	if(value==0xF7) value=0xEB;
//sq	  	if(value==0xDF) value=0xAF;
//sq	  	if(value==0x7F) value=0xBE;
//sq	  	res=~((gp2x_memregs[0x1184>>1] & 0xFF00) | value | (gp2x_memregs[0x1186>>1] << 16));
//sq	
//sq	  	/* GP2X F200 Push Button */
//sq	  	if ((res & GP2X_UP) && (res & GP2X_DOWN) && (res & GP2X_LEFT) && (res & GP2X_RIGHT))
//sq	  		res |= GP2X_P;
//sq	}
//sq	
//sq	if (num_of_joys>n)
//sq	{
//sq	  	/* Check USB Joypad */
//sq		res |= gp2x_usbjoy_check(joys[n]);
//sq	}
//sq
//sq	return res;
}

unsigned long gp2x_joystick_press ()
{
	unsigned long ExKey=0;
	while(gp2x_joystick_read()&0x8c0ff55) { gp2x_timer_delay(150000); }
	while(!(ExKey=gp2x_joystick_read()&0x8c0ff55)) { }
	return ExKey;
}

void gp2x_sound_volume(int l, int r)
{
}

void gp2x_timer_delay(unsigned long ticks)
{
	unsigned long ini=gp2x_timer_read();
	while (gp2x_timer_read()-ini<ticks);
}


unsigned long gp2x_timer_read(void)
{
//sq    struct timeval current_time;
//sq    gettimeofday(&current_time, NULL);
//sq
//sq    return ((unsigned long long)current_time.tv_sec * 1000LL + (current_time.tv_usec / 1000LL));

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);

	return ((unsigned long long)now.tv_sec * 1000000LL + (now.tv_nsec / 1000LL));


}

// create two resources for 'page flipping'
static DISPMANX_RESOURCE_HANDLE_T   resource0;
static DISPMANX_RESOURCE_HANDLE_T   resource1;
static DISPMANX_RESOURCE_HANDLE_T   resource_bg;

// these are used for switching between the buffers
static DISPMANX_RESOURCE_HANDLE_T cur_res;
static DISPMANX_RESOURCE_HANDLE_T prev_res;
static DISPMANX_RESOURCE_HANDLE_T tmp_res;

DISPMANX_ELEMENT_HANDLE_T dispman_element;
DISPMANX_ELEMENT_HANDLE_T dispman_element_bg;
DISPMANX_DISPLAY_HANDLE_T dispman_display;
DISPMANX_UPDATE_HANDLE_T dispman_update;

void exitfunc()
{
	SDL_Quit();
	bcm_host_deinit();
}

int init_SDL(void)
{
    SDL_Event e;
	SDL_Joystick *joy;
    
    sdlscreen = SDL_SetVideoMode(0,0, 32, SDL_SWSURFACE);
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        return(0);
    }
    
	/* USB Joysticks Initialization */
//sq	gp2x_usbjoy_init();

    SDL_JoystickEventState(SDL_ENABLE);
	num_of_joys=SDL_NumJoysticks();
	joy=SDL_JoystickOpen(0);
	joy=SDL_JoystickOpen(1);
	joy=SDL_JoystickOpen(2);
	joy=SDL_JoystickOpen(3);
	SDL_EventState(SDL_ACTIVEEVENT,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP,SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT,SDL_IGNORE);
	SDL_EventState(SDL_VIDEORESIZE,SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT,SDL_IGNORE);
	SDL_ShowCursor(SDL_DISABLE);

    //Initialise dispmanx
    bcm_host_init();
    
	//Clean exits, hopefully!
	atexit(exitfunc);
    
    return(1);
}

void deinit_SDL(void)
{
    if(sdlscreen)
    {
        SDL_FreeSurface(sdlscreen);
        sdlscreen = NULL;
    }
    SDL_Quit();
    
    bcm_host_deinit();
}


void gp2x_deinit(void)
{
	int ret;
	/* USB Joysticks Close */
	gp2x_usbjoy_close();

	dispman_update = vc_dispmanx_update_start( 0 );
    ret = vc_dispmanx_element_remove( dispman_update, dispman_element );
    ret = vc_dispmanx_element_remove( dispman_update, dispman_element_bg );
    ret = vc_dispmanx_update_submit_sync( dispman_update );
	ret = vc_dispmanx_resource_delete( resource0 );
	ret = vc_dispmanx_resource_delete( resource1 );
	ret = vc_dispmanx_resource_delete( resource_bg );
	ret = vc_dispmanx_display_close( dispman_display );

    free(gp2x_screen8);
	if(gp2x_screen15)
		free(gp2x_screen15);

}

void gp2x_set_video_mode(int bpp,int width,int height)
{
	int ret;
	uint32_t success;
	uint32_t display_width, display_height;
	uint32_t display_width_save, display_height_save;
	uint32_t display_x=0, display_y=0;
	uint32_t display_border=24;
	float display_ratio,game_ratio;

	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	surface_width = width;
	surface_height = height;

	gp2x_screen8=(unsigned char *) calloc(1, width*height);
	gp2x_screen15=0;
	gp2x_nflip=0;
	
	success = graphics_get_display_size(0 /* LCD */, &display_width, &display_height);

    dispman_display = vc_dispmanx_display_open( 0 );
	assert( dispman_display != 0 );

	display_width_save = display_width;
	display_height_save = display_height;

	// Add border around bitmap for TV
	display_width -= display_border*2;
	display_height -= display_border*2;

	//Create two surfaces for flipping between
	//Make sure bitmap type matches the source for better performance
    uint32_t crap;
    resource0 = vc_dispmanx_resource_create(VC_IMAGE_8BPP, width, height, &crap);
    resource1 = vc_dispmanx_resource_create(VC_IMAGE_8BPP, width, height, &crap);

	//Create a blank background for the whole screen, make sure width is divisible by 32!
    //Make sure the resource is cleared by writing to it
    resource_bg = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 128, 128, &crap);
    
 	// Work out the position and size on the display
 	display_ratio = (float)display_width/(float)display_height;
 	game_ratio = (float)width/(float)height;
 
 	display_x = display_width;
 	display_y = display_height;
 
 	if (game_ratio>display_ratio) {
 		display_height = (float)display_width/(float)game_ratio;
 	} else {
 		display_width = display_height*(float)game_ratio;;
 	}
 
	// Centre bitmap on screen
 	display_x = (display_x - display_width) / 2;
 	display_y = (display_y - display_height) / 2;

	vc_dispmanx_rect_set( &dst_rect, display_x + display_border, display_y + display_border, 
								display_width, display_height);
	vc_dispmanx_rect_set( &src_rect, 0, 0, width << 16, height << 16);

	dispman_update = vc_dispmanx_update_start( 0 );

    // create the 'window' element - based on the first buffer resource (resource0)
    dispman_element = vc_dispmanx_element_add(  dispman_update,
                                         dispman_display,
                                         10,
                                         &dst_rect,
                                         resource0,
                                         &src_rect,
                                         DISPMANX_PROTECTION_NONE,
                                         0,
                                         0,
                                         (DISPMANX_TRANSFORM_T) 0 );

	vc_dispmanx_rect_set( &dst_rect, 0, 0, display_width_save, display_height_save );
	vc_dispmanx_rect_set( &src_rect, 0, 0, 128 << 16, 128 << 16);

	//Create a blank background to cover the whole screen
    dispman_element_bg = vc_dispmanx_element_add(  dispman_update,
                                         dispman_display,
                                         9,
                                         &dst_rect,
                                         resource_bg,
                                         &src_rect,
                                         DISPMANX_PROTECTION_NONE,
                                         0,
                                         0,
                                         (DISPMANX_TRANSFORM_T) 0 );

    ret = vc_dispmanx_update_submit_sync( dispman_update );

	// setup swapping of double buffers
	cur_res = resource1;
	prev_res = resource0;
}

void DisplayScreen(void)
{
	VC_RECT_T dst_rect;

	vc_dispmanx_rect_set( &dst_rect, 0, 0, surface_width, surface_height );

	// blit image to the current resource
	vc_dispmanx_resource_write_data( cur_res, VC_IMAGE_8BPP, surface_width, gp2x_screen8, &dst_rect );

	// begin display update
	dispman_update = vc_dispmanx_update_start( 0 );

	// change element source to be the current resource
	vc_dispmanx_element_change_source( dispman_update, dispman_element, cur_res );

	// finish display update, vsync is handled by software throttling
	// dispmanx avoids any tearing. vsync here would be limited to 30fps
	// on a CRT TV.
	vc_dispmanx_update_submit( dispman_update, 0, 0 );

	// swap current resource
	tmp_res = cur_res;
	cur_res = prev_res;
	prev_res = tmp_res;

}

void gp2x_frontend_init(void)
{
	int ret;
        
	uint32_t success;
	uint32_t display_width, display_height;
	uint32_t display_width_save, display_height_save;
	uint32_t display_border=24;
    
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
    
	surface_width = 640;
	surface_height = 480;
    
	gp2x_screen8=(unsigned char *) calloc(1, 640*480);
	gp2x_screen15=0;
	gp2x_nflip=0;
	
	success = graphics_get_display_size(0 /* LCD */, &display_width, &display_height);
    
    dispman_display = vc_dispmanx_display_open( 0 );
	assert( dispman_display != 0 );
        
	// Add border around bitmap for TV
	display_width -= display_border*2;
	display_height -= display_border*2;
    
	//Create two surfaces for flipping between
	//Make sure bitmap type matches the source for better performance
    uint32_t crap;
    resource0 = vc_dispmanx_resource_create(VC_IMAGE_8BPP, 640, 480, &crap);
    resource1 = vc_dispmanx_resource_create(VC_IMAGE_8BPP, 640, 480, &crap);
    
	vc_dispmanx_rect_set( &dst_rect, display_border, display_border,
                         display_width, display_height);
	vc_dispmanx_rect_set( &src_rect, 0, 0, 640 << 16, 480 << 16);
    
	//Make sure mame and background overlay the menu program
	dispman_update = vc_dispmanx_update_start( 0 );
    
    // create the 'window' element - based on the first buffer resource (resource0)
    dispman_element = vc_dispmanx_element_add(  dispman_update,
                                              dispman_display,
                                              1,
                                              &dst_rect,
                                              resource0,
                                              &src_rect,
                                              DISPMANX_PROTECTION_NONE,
                                              0,
                                              0,
                                              (DISPMANX_TRANSFORM_T) 0 );
    
    ret = vc_dispmanx_update_submit_sync( dispman_update );
    
	// setup swapping of double buffers
	cur_res = resource1;
	prev_res = resource0;
    
}

void gp2x_frontend_deinit(void)
{
	int ret;
	/* USB Joysticks Close */
//sq	gp2x_usbjoy_close();
    
	dispman_update = vc_dispmanx_update_start( 0 );
    ret = vc_dispmanx_element_remove( dispman_update, dispman_element );
    ret = vc_dispmanx_update_submit_sync( dispman_update );
	ret = vc_dispmanx_resource_delete( resource0 );
	ret = vc_dispmanx_resource_delete( resource1 );
	ret = vc_dispmanx_display_close( dispman_display );
    
    free(gp2x_screen8);
    
//    if(sdlscreen)
//    {
//        SDL_FreeSurface(sdlscreen);
//        sdlscreen = NULL;
//    }
//    SDL_Quit();
//    
//	bcm_host_deinit();
    
}



static unsigned char fontdata8x8[] =
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x3C,0x42,0x99,0xBD,0xBD,0x99,0x42,0x3C,0x3C,0x42,0x81,0x81,0x81,0x81,0x42,0x3C,
	0xFE,0x82,0x8A,0xD2,0xA2,0x82,0xFE,0x00,0xFE,0x82,0x82,0x82,0x82,0x82,0xFE,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x64,0x74,0x7C,0x38,0x00,0x00,
	0x80,0xC0,0xF0,0xFC,0xF0,0xC0,0x80,0x00,0x01,0x03,0x0F,0x3F,0x0F,0x03,0x01,0x00,
	0x18,0x3C,0x7E,0x18,0x7E,0x3C,0x18,0x00,0xEE,0xEE,0xEE,0xCC,0x00,0xCC,0xCC,0x00,
	0x00,0x00,0x30,0x68,0x78,0x30,0x00,0x00,0x00,0x38,0x64,0x74,0x7C,0x38,0x00,0x00,
	0x3C,0x66,0x7A,0x7A,0x7E,0x7E,0x3C,0x00,0x0E,0x3E,0x3A,0x22,0x26,0x6E,0xE4,0x40,
	0x18,0x3C,0x7E,0x3C,0x3C,0x3C,0x3C,0x00,0x3C,0x3C,0x3C,0x3C,0x7E,0x3C,0x18,0x00,
	0x08,0x7C,0x7E,0x7E,0x7C,0x08,0x00,0x00,0x10,0x3E,0x7E,0x7E,0x3E,0x10,0x00,0x00,
	0x58,0x2A,0xDC,0xC8,0xDC,0x2A,0x58,0x00,0x24,0x66,0xFF,0xFF,0x66,0x24,0x00,0x00,
	0x00,0x10,0x10,0x38,0x38,0x7C,0xFE,0x00,0xFE,0x7C,0x38,0x38,0x10,0x10,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x1C,0x1C,0x18,0x00,0x18,0x18,0x00,
	0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x28,0x7C,0x28,0x7C,0x28,0x00,0x00,
	0x10,0x38,0x60,0x38,0x0C,0x78,0x10,0x00,0x40,0xA4,0x48,0x10,0x24,0x4A,0x04,0x00,
	0x18,0x34,0x18,0x3A,0x6C,0x66,0x3A,0x00,0x18,0x18,0x20,0x00,0x00,0x00,0x00,0x00,
	0x30,0x60,0x60,0x60,0x60,0x60,0x30,0x00,0x0C,0x06,0x06,0x06,0x06,0x06,0x0C,0x00,
	0x10,0x54,0x38,0x7C,0x38,0x54,0x10,0x00,0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00,
	0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x04,0x08,0x10,0x20,0x40,0x00,0x00,
	0x38,0x4C,0xC6,0xC6,0xC6,0x64,0x38,0x00,0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00,
	0x7C,0xC6,0x0E,0x3C,0x78,0xE0,0xFE,0x00,0x7E,0x0C,0x18,0x3C,0x06,0xC6,0x7C,0x00,
	0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00,0xFC,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00,
	0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00,0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00,
	0x78,0xC4,0xE4,0x78,0x86,0x86,0x7C,0x00,0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00,
	0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30,
	0x1C,0x38,0x70,0xE0,0x70,0x38,0x1C,0x00,0x00,0x7C,0x00,0x00,0x7C,0x00,0x00,0x00,
	0x70,0x38,0x1C,0x0E,0x1C,0x38,0x70,0x00,0x7C,0xC6,0xC6,0x1C,0x18,0x00,0x18,0x00,
	0x3C,0x42,0x99,0xA1,0xA5,0x99,0x42,0x3C,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00,
	0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00,0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00,
	0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00,0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xFE,0x00,
	0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xC0,0x00,0x3E,0x60,0xC0,0xCE,0xC6,0x66,0x3E,0x00,
	0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,
	0x06,0x06,0x06,0x06,0xC6,0xC6,0x7C,0x00,0xC6,0xCC,0xD8,0xF0,0xF8,0xDC,0xCE,0x00,
	0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00,
	0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
	0xFC,0xC6,0xC6,0xC6,0xFC,0xC0,0xC0,0x00,0x7C,0xC6,0xC6,0xC6,0xDE,0xCC,0x7A,0x00,
	0xFC,0xC6,0xC6,0xCE,0xF8,0xDC,0xCE,0x00,0x78,0xCC,0xC0,0x7C,0x06,0xC6,0x7C,0x00,
	0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
	0xC6,0xC6,0xC6,0xEE,0x7C,0x38,0x10,0x00,0xC6,0xC6,0xD6,0xFE,0xFE,0xEE,0xC6,0x00,
	0xC6,0xEE,0x3C,0x38,0x7C,0xEE,0xC6,0x00,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00,
	0xFE,0x0E,0x1C,0x38,0x70,0xE0,0xFE,0x00,0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,
	0x60,0x60,0x30,0x18,0x0C,0x06,0x06,0x00,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,
	0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
	0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x06,0x3E,0x66,0x66,0x3C,0x00,
	0x60,0x7C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x3C,0x66,0x60,0x60,0x66,0x3C,0x00,
	0x06,0x3E,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x3C,0x00,
	0x1C,0x30,0x78,0x30,0x30,0x30,0x30,0x00,0x00,0x3E,0x66,0x66,0x66,0x3E,0x06,0x3C,
	0x60,0x7C,0x76,0x66,0x66,0x66,0x66,0x00,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x00,
	0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x38,0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00,
	0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0xEC,0xFE,0xFE,0xFE,0xD6,0xC6,0x00,
	0x00,0x7C,0x76,0x66,0x66,0x66,0x66,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x3C,0x00,
	0x00,0x7C,0x66,0x66,0x66,0x7C,0x60,0x60,0x00,0x3E,0x66,0x66,0x66,0x3E,0x06,0x06,
	0x00,0x7E,0x70,0x60,0x60,0x60,0x60,0x00,0x00,0x3C,0x60,0x3C,0x06,0x66,0x3C,0x00,
	0x30,0x78,0x30,0x30,0x30,0x30,0x1C,0x00,0x00,0x66,0x66,0x66,0x66,0x6E,0x3E,0x00,
	0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x7C,0x6C,0x00,
	0x00,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x06,0x3C,
	0x00,0x7E,0x0C,0x18,0x30,0x60,0x7E,0x00,0x0E,0x18,0x0C,0x38,0x0C,0x18,0x0E,0x00,
	0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,0x70,0x18,0x30,0x1C,0x30,0x18,0x70,0x00,
	0x00,0x00,0x76,0xDC,0x00,0x00,0x00,0x00,0x10,0x28,0x10,0x54,0xAA,0x44,0x00,0x00,
};

static void gp2x_text(unsigned char *screen, int x, int y, char *text, int color)
{
	unsigned int i,l;
	screen=screen+(x*2)+(y*2)*640;

	for (i=0;i<strlen(text);i++) {
		
		for (l=0;l<16;l=l+2) {
			screen[l*640+0]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x80)?color:screen[l*640+0]];
			screen[l*640+1]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x80)?color:screen[l*640+1]];

			screen[l*640+2]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x40)?color:screen[l*640+2]];
			screen[l*640+3]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x40)?color:screen[l*640+3]];

			screen[l*640+4]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x20)?color:screen[l*640+4]];
			screen[l*640+5]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x20)?color:screen[l*640+5]];

			screen[l*640+6]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x10)?color:screen[l*640+6]];
			screen[l*640+7]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x10)?color:screen[l*640+7]];

			screen[l*640+8]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x08)?color:screen[l*640+8]];
			screen[l*640+9]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x08)?color:screen[l*640+9]];

			screen[l*640+10]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x04)?color:screen[l*640+10]];
			screen[l*640+11]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x04)?color:screen[l*640+11]];

			screen[l*640+12]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x02)?color:screen[l*640+12]];
			screen[l*640+13]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x02)?color:screen[l*640+13]];

			screen[l*640+14]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x01)?color:screen[l*640+14]];
			screen[l*640+15]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x01)?color:screen[l*640+15]];
		}
		for (l=1;l<16;l=l+2) {
			screen[l*640+0]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x80)?color:screen[l*640+0]];
			screen[l*640+1]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x80)?color:screen[l*640+1]];

			screen[l*640+2]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x40)?color:screen[l*640+2]];
			screen[l*640+3]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x40)?color:screen[l*640+3]];

			screen[l*640+4]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x20)?color:screen[l*640+4]];
			screen[l*640+5]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x20)?color:screen[l*640+5]];

			screen[l*640+6]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x10)?color:screen[l*640+6]];
			screen[l*640+7]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x10)?color:screen[l*640+7]];

			screen[l*640+8]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x08)?color:screen[l*640+8]];
			screen[l*640+9]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x08)?color:screen[l*640+9]];

			screen[l*640+10]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x04)?color:screen[l*640+10]];
			screen[l*640+11]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x04)?color:screen[l*640+11]];

			screen[l*640+12]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x02)?color:screen[l*640+12]];
			screen[l*640+13]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x02)?color:screen[l*640+13]];

			screen[l*640+14]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x01)?color:screen[l*640+14]];
			screen[l*640+15]=gp2x_palette[(fontdata8x8[((text[i])*8)+l/2]&0x01)?color:screen[l*640+15]];
		}
		screen+=16;
	} 
}

void gp2x_gamelist_text_out(int x, int y, char *eltexto, int color)
{
	char texto[33];
	strncpy(texto,eltexto,32);
	texto[32]=0;
//sq	if (texto[0]!='-')
//sq		gp2x_text(gp2x_screen8,x+1,y+1,texto,color);
	gp2x_text(gp2x_screen8,x,y,texto,color);
}

/* Variadic functions guide found at http://www.unixpapa.com/incnote/variadic.html */
void gp2x_gamelist_text_out_fmt(int x, int y, char* fmt, ...)
{
	char strOut[128];
	va_list marker;
	
	va_start(marker, fmt);
	vsprintf(strOut, fmt, marker);
	va_end(marker);	

	gp2x_gamelist_text_out(x, y, strOut, 255);
}

static int log=0;

void gp2x_printf_init(void)
{
	log=0;
}

static void gp2x_text_log(char *texto)
{
	if (!log)
	{
		memset(gp2x_screen8,0,320*240);
	}
	gp2x_text(gp2x_screen8,0,log,texto,255);
	log+=8;
	if(log>239) log=0;
}

/* Variadic functions guide found at http://www.unixpapa.com/incnote/variadic.html */
void gp2x_printf(char* fmt, ...)
{
	int i,c;
	char strOut[4096];
	char str[41];
	va_list marker;
	
	va_start(marker, fmt);
	vsprintf(strOut, fmt, marker);
	va_end(marker);	

	c=0;
	for (i=0;i<strlen(strOut);i++)
	{
		str[c]=strOut[i];
		if (str[c]=='\n')
		{
			str[c]=0;
			gp2x_text_log(str);
			c=0;
		}
		else if (c==39)
		{
			str[40]=0;
			gp2x_text_log(str);
			c=0;
		}		
		else
		{
			c++;
		}
	}
}

