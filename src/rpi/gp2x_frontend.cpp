#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "minimal.h"
#include "gp2x_frontend_list.h"
#include <SDL.h>
#include "allegro.h"

int game_num_avail=0;
static int last_game_selected=0;
char playemu[16] = "mame\0";
char playgame[16] = "builtinn\0";

int gp2x_freq=200;
int gp2x_video_depth=8;
int gp2x_video_aspect=0;
int gp2x_video_sync=0;
int gp2x_frameskip=-1;
int gp2x_sound = 14;
int gp2x_volume = 3;
int gp2x_clock_cpu=80;
int gp2x_clock_sound=80;
int gp2x_cpu_cores=1;
int gp2x_ramtweaks=1;
int gp2x_cheat=0;

static unsigned short *gp2xmenu_bmp;
static unsigned short *gp2xsplash_bmp;

static void gp2x_exit(void);

#define gp2x_color15(R,G,B)  ((R >> 3) << 11) | (( G >> 2) << 5 ) | (( B >> 3 ) << 0 )

//sq static void load_bmp_8bpp(unsigned char *out, unsigned char *in)
//sq {
//sq 	int i,x,y;
//sq 	unsigned char r,g,b,c;
//sq 	in+=14; /* Skip HEADER */
//sq 	in+=40; /* Skip INFOHD */
//sq 	/* Set Palette */
//sq 	for (i=0;i<256;i++) {
//sq 		b=*in++;
//sq 		g=*in++;
//sq 		r=*in++;
//sq 		c=*in++;
//sq 		gp2x_video_color8(i,r,g,b);
//sq 	}
//sq 	gp2x_video_setpalette();
//sq 	/* Set Bitmap */	
//sq 	for (y=479;y!=-1;y--) {
//sq 		for (x=0;x<640;x++) {
//sq 			*out++=gp2x_palette[in[x+y*640]];
//sq 		}
//sq 	}
//sq }

static void load_bmp_16bpp(unsigned short *out, unsigned short *in)
{
 	int i,x,y;

	in+=(640*480)-1;
 	for (y=479;y!=-1;y--) {
		memcpy(out, in, 640*2);
		out+=640;
		in-=640;
	}
}

#pragma pack(2) // Align
typedef struct                       /**** BMP file header structure ****/
{
    unsigned short bfType;           /* Magic number for file */
    unsigned int   bfSize;           /* Size of file */
    unsigned short bfReserved1;      /* Reserved */
    unsigned short bfReserved2;      /* ... */
    unsigned int   bfOffBits;        /* Offset to bitmap data */
} BITMAPFILEHEADER;
#pragma pack()

static void gp2x_intro_screen(int first_run) {
	char name[256];
	int offset=0;
	FILE *f;
	BITMAPFILEHEADER h;

	DisplayScreen16();

	sprintf(name,"skins/rpisplash16.bmp");

	f=fopen(name,"rb");
	if (f) {
		//Read header to find where to skip to for bitmap
        fread(&h, sizeof(BITMAPFILEHEADER), 1, f); //reading the FILEHEADER
		fseek(f, h.bfOffBits, SEEK_SET);

		fread(gp2xsplash_bmp,1,1000000,f);
		fclose(f);
	} 	
	else {
		printf("\nERROR: Splash screen missing from skins directory\n");
		gp2x_exit();
	}

	if(first_run) {
		load_bmp_16bpp(gp2x_screen15,gp2xsplash_bmp);
		DisplayScreen16();
		sleep(1);
	}
	
	sprintf(name,"skins/rpimenu16.bmp");
	f=fopen(name,"rb");
	if (f) {
		//Read header to find where to skip to for bitmap
        fread(&h, sizeof(BITMAPFILEHEADER), 1, f); //reading the FILEHEADER
		fseek(f, h.bfOffBits+sizeof(h), SEEK_SET);

		fread(gp2xmenu_bmp,1,1000000,f);
		fclose(f);
	} 
	else {
		printf("\nERROR: Menu screen missing from skins directory\n");
		gp2x_exit();
	}
}

static void game_list_init_nocache(void)
{
	int i;
	FILE *f;
	DIR *d=opendir("roms");
	char game[32];
	if (d)
	{
		struct dirent *actual=readdir(d);
		while(actual)
		{
			for (i=0;i<NUMGAMES;i++)
			{
				if (fe_drivers[i].available==0)
				{
					sprintf(game,"%s.zip",fe_drivers[i].name);
					if (strcmp(actual->d_name,game)==0)
					{
						fe_drivers[i].available=1;
						game_num_avail++;
						break;
					}
				}
			}
			actual=readdir(d);
		}
		closedir(d);
	}
	
	if (game_num_avail)
	{
		remove("frontend/mame.lst");
		sync();
		f=fopen("frontend/mame.lst","w");
		if (f)
		{
			for (i=0;i<NUMGAMES;i++)
			{
				fputc(fe_drivers[i].available,f);
			}
			fclose(f);
			sync();
		}
	}
}

static void game_list_init_cache(void)
{
	FILE *f;
	int i;
	f=fopen("frontend/mame.lst","r");
	if (f)
	{
		for (i=0;i<NUMGAMES;i++)
		{
			fe_drivers[i].available=fgetc(f);
			if (fe_drivers[i].available)
				game_num_avail++;
		}
		fclose(f);
	}
	else
		game_list_init_nocache();
}

static void game_list_init(int argc)
{
	if (argc==1)
		game_list_init_nocache();
	else
		game_list_init_cache();
}

static void game_list_view(int *pos) {

	int i;
	int view_pos;
	int aux_pos=0;
	int screen_y = 45;
	int screen_x = 38;

	/* Draw background image */
	load_bmp_16bpp(gp2x_screen15,gp2xmenu_bmp);

	/* Check Limits */
	if (*pos<0)
		*pos=game_num_avail-1;
	if (*pos>(game_num_avail-1))
		*pos=0;
					   
	/* Set View Pos */
	if (*pos<10) {
		view_pos=0;
	} else {
		if (*pos>game_num_avail-11) {
			view_pos=game_num_avail-21;
			view_pos=(view_pos<0?0:view_pos);
		} else {
			view_pos=*pos-10;
		}
	}

	/* Show List */
	for (i=0;i<NUMGAMES;i++) {
		if (fe_drivers[i].available==1) {
			if (aux_pos>=view_pos && aux_pos<=view_pos+20) {
				if (aux_pos==*pos) {
					gp2x_gamelist_text_out( screen_x, screen_y, fe_drivers[i].description, gp2x_color15(0,150,255));
					gp2x_gamelist_text_out( screen_x-10, screen_y,">",gp2x_color15(255,255,255) );
					gp2x_gamelist_text_out( screen_x-13, screen_y-1,"-",gp2x_color15(255,255,255) );
				}
				else {
					gp2x_gamelist_text_out( screen_x, screen_y, fe_drivers[i].description, gp2x_color15(255,255,255));
				}
				
				screen_y+=8;
			}
			aux_pos++;
		}
	}
}

static void game_list_select (int index, char *game, char *emu) {
	int i;
	int aux_pos=0;
	for (i=0;i<NUMGAMES;i++)
	{
		if (fe_drivers[i].available==1)
		{
			if(aux_pos==index)
			{
				strcpy(game,fe_drivers[i].name);
				strcpy(emu,fe_drivers[i].exe);
				gp2x_cpu_cores=fe_drivers[i].cores;
				break;
			}
			aux_pos++;
		}
	}
}

static char *game_list_description (int index)
{
	int i;
	int aux_pos=0;
	for (i=0;i<NUMGAMES;i++) {
		if (fe_drivers[i].available==1) {
			if(aux_pos==index) {
				return(fe_drivers[i].description);
			}
			aux_pos++;
		   }
	}
	return ((char *)0);
}


static void gp2x_exit(void)
{
	remove("frontend/mame.lst");
	sync();
	gp2x_deinit();
    deinit_SDL();
	exit(0);
}

extern unsigned long ExKey1;

extern int osd_is_key_pressed(int keycode);
extern int is_joy_axis_pressed (int axis, int dir, int joynum);

static void select_game(char *emu, char *game)
{

	unsigned long ExKey=0;

	/* No Selected game */
	strcpy(game,"builtinn");

	/* Clean screen */
	DisplayScreen16();

	gp2x_joystick_clear();	

	/* Wait until user selects a game */
	while(1)
	{
		game_list_view(&last_game_selected);
		DisplayScreen16();
       	gp2x_timer_delay(100000);

//sq        if( (gp2x_joystick_read()))
//sq        	gp2x_timer_delay(100000);
		while(1)
		{
            usleep(10000);
			gp2x_joystick_read();	

			//Any joy buttons pressed?
			if (ExKey1)
			{
				ExKey=ExKey1;
				break;
			}

			//Any keyboard key pressed?
			if(osd_is_key_pressed(KEY_LEFT) ||
			   osd_is_key_pressed(KEY_RIGHT) ||
			   osd_is_key_pressed(KEY_UP) ||
			   osd_is_key_pressed(KEY_DOWN) ||
			   osd_is_key_pressed(KEY_ENTER) ||
			   osd_is_key_pressed(KEY_LCONTROL) ||
			   osd_is_key_pressed(KEY_ESC)) 
			{
				break;
			}

			//Any stick direction?
			if(is_joy_axis_pressed (0, 1, 0) ||
			   is_joy_axis_pressed (0, 2, 0) ||
			   is_joy_axis_pressed (1, 1, 0) ||
			   is_joy_axis_pressed (1, 2, 0))
			{
				break;
			}

		}

		int updown=0;
		if(is_joy_axis_pressed (1, 1, 0)) {last_game_selected--; updown=1;};
		if(is_joy_axis_pressed (1, 2, 0)) {last_game_selected++; updown=1;};

		// Stop diagonals on game selection
		if(!updown) {
			if(is_joy_axis_pressed (0, 1, 0)) last_game_selected-=21;
			if(is_joy_axis_pressed (0, 2, 0)) last_game_selected+=21;
		}

		if (ExKey & GP2X_ESCAPE) gp2x_exit();

		if (osd_is_key_pressed(KEY_UP)) last_game_selected--;
		if (osd_is_key_pressed(KEY_DOWN)) last_game_selected++;
		if (osd_is_key_pressed(KEY_LEFT)) last_game_selected-=21;
		if (osd_is_key_pressed(KEY_RIGHT)) last_game_selected+=21;
		if (osd_is_key_pressed(KEY_ESC)) gp2x_exit();

		if ((ExKey & GP2X_LCTRL) || (ExKey & GP2X_RETURN) || 
			osd_is_key_pressed(KEY_LCONTROL) || osd_is_key_pressed(KEY_ENTER))
		{
			/* Select the game */
			game_list_select(last_game_selected, game, emu);

			break;
		}
	}
}

void frontend_gui (char *gamename, int first_run)
{
	FILE *f;

	/* GP2X Initialization */
	gp2x_frontend_init();

	gp2xmenu_bmp = (unsigned short*)calloc(1, 1000000);
	gp2xsplash_bmp = (unsigned short*)calloc(1, 1000000);

	/* Show load bitmaps and show intro screen */
    gp2x_intro_screen(first_run);

	/* Initialize list of available games */
	game_list_init(1);
	if (game_num_avail==0)
	{
		/* Draw background image */
    	load_bmp_16bpp(gp2x_screen15,gp2xmenu_bmp);
		gp2x_gamelist_text_out(35, 110, "ERROR: NO AVAILABLE GAMES FOUND",gp2x_color15(255,255,255));
		DisplayScreen16();
		sleep(5);
		gp2x_exit();
	}

	/* Read default configuration */
	f=fopen("frontend/mame.cfg","r");
	if (f) {
		fscanf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&gp2x_freq,&gp2x_video_depth,&gp2x_video_aspect,&gp2x_video_sync,
		&gp2x_frameskip,&gp2x_sound,&gp2x_clock_cpu,&gp2x_clock_sound,&gp2x_cpu_cores,&gp2x_ramtweaks,&last_game_selected,&gp2x_cheat,&gp2x_volume);
		fclose(f);
	}
	
	/* Select Game */
	select_game(playemu,playgame); 

	/* Write default configuration */
	f=fopen("frontend/mame.cfg","w");
	if (f) {
		fprintf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",gp2x_freq,gp2x_video_depth,gp2x_video_aspect,gp2x_video_sync,
		gp2x_frameskip,gp2x_sound,gp2x_clock_cpu,gp2x_clock_sound,gp2x_cpu_cores,gp2x_ramtweaks,last_game_selected,gp2x_cheat,gp2x_volume);
		fclose(f);
		sync();
	}
	
    strcpy(gamename, playgame);
    
    gp2x_frontend_deinit();

	free(gp2xmenu_bmp);
	free(gp2xsplash_bmp);
	
}
