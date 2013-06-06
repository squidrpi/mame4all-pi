#include "driver.h"
#include "dirty.h"

/* from video.c */
extern char *dirty_old;
extern char *dirty_new;
extern int gfx_xoffset;
extern int gfx_yoffset;
extern int gfx_display_lines;
extern int gfx_display_columns;
extern int gfx_width;
extern int gfx_height;
extern int skiplines;
extern int skipcolumns;

#define SCREEN8 gp2x_screen8
#define SCREEN16 gp2x_screen15
#define FLIP_VIDEO gp2x_video_flip(bitmap)

#include "minimal.h"

UINT32 *palette_16bit_lookup;

//sqvoid blitscreen_dirty1_color8(struct osd_bitmap *bitmap)
//sq{
//sq	int x, y, m;
//sq	int width=(bitmap->line[1] - bitmap->line[0]);
//sq	unsigned char *lb=bitmap->line[skiplines] + skipcolumns;
//sq	unsigned char *address=SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);
//sq
//sq	for (y = 0; y < gfx_display_lines; y += 16)
//sq	{
//sq		for (x = 0; x < gfx_display_columns; )
//sq		{
//sq			int w = 16;
//sq			if (ISDIRTY(x,y))
//sq			{
//sq				int h;
//sq				unsigned char *lb0 = lb + x;
//sq				unsigned char *address0 = address + x;
//sq				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
//sq                    			w += 16;
//sq				if (x + w > gfx_display_columns)
//sq                    			w = gfx_display_columns - x;
//sq				for (h = 0; ((h < 16) && ((y + h) < gfx_display_lines)); h++)
//sq				{
//sq					memcpy(address0,lb0,w);
//sq					lb0 += width;
//sq					address0 += gfx_width;
//sq				}
//sq			}
//sq			x += w;
//sq        	}
//sq		lb += 16 * width;
//sq		address += 16 * gfx_width;
//sq	}
//sq
//sq    FLIP_VIDEO;
//sq}

void blitscreen_dirty1_color8(struct osd_bitmap *bitmap)
{   
    int x,y=gfx_display_lines;
    int width=(bitmap->line[1] - bitmap->line[0]);
    int columns=gfx_display_columns;
    unsigned char *lb = bitmap->line[skiplines] + skipcolumns;
    unsigned char *address = SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

    do
        {
                memcpy(address,lb,columns);
                lb+=width;
                address+=gfx_width;
                y--;
        }
        while (y); 

    gp2x_video_flip(bitmap);
}

void blitscreen_dirty0_color8(struct osd_bitmap *bitmap)
{   
    int x,y=gfx_display_lines;
    int width=(bitmap->line[1] - bitmap->line[0]);
    int columns=gfx_display_columns;
    unsigned char *lb = bitmap->line[skiplines] + skipcolumns;
    unsigned char *address = SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

    do
        {
                memcpy(address,lb,columns);
                lb+=width;
                address+=gfx_width;
                y--;
        }
        while (y);  

    gp2x_video_flip(bitmap);
}

void blitscreen_dirty1_palettized16(struct osd_bitmap *bitmap)
{
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned short *lb=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	unsigned short *address=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				int h;
				unsigned short *lb0 = lb + x;
				unsigned short *address0 = address + x;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    			w += 16;
				if (x + w > gfx_display_columns)
                    			w = gfx_display_columns - x;
				for (h = 0; ((h < 16) && ((y + h) < gfx_display_lines)); h++)
				{
					int wx;
					for (wx=0;wx<w;wx++)
					{
						address0[wx] = palette_16bit_lookup[lb0[wx]];
					}
					lb0 += width;
					address0 += gfx_width;
				}
			}
			x += w;
        	}
		lb += 16 * width;
		address += 16 * gfx_width;
	}
}

void blitscreen_dirty0_palettized16(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	unsigned short *address = SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y++)
	{
		for (x = 0; x < columns; x++)
		{
			address[x] = palette_16bit_lookup[lb[x]];
		}
		lb+=width;
		address+=gfx_width;
	}
	
	FLIP_VIDEO;
}

void blitscreen_dirty1_color16(struct osd_bitmap *bitmap)
{
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned short *lb=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	unsigned short *address=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				int h;
				unsigned short *lb0 = lb + x;
				unsigned short *address0 = address + x;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    			w += 16;
				if (x + w > gfx_display_columns)
                    			w = gfx_display_columns - x;
				for (h = 0; ((h < 16) && ((y + h) < gfx_display_lines)); h++)
				{
					int wx;
					for (wx=0;wx<w;wx++)
					{
						address0[wx] = lb0[wx]&0xFFDF;
					}
					lb0 += width;
					address0 += gfx_width;
				}
			}
			x += w;
        	}
		lb += 16 * width;
		address += 16 * gfx_width;
	}
}

void blitscreen_dirty0_color16(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	unsigned short *address = SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y++)
	{
		for (x = 0; x < columns; x++)
		{
			address[x] = lb[x]&0xFFDF;
		}
		lb+=width;
		address+=gfx_width;
	}
	
	FLIP_VIDEO;
}
