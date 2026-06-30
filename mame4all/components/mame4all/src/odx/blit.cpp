#ifndef RG_MAME_CORE_H
#define RG_MAME_CORE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "retrogo/rg_psram.h"
#include "driver.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "driver.h"
#include "dirty.h"
#include "minimal.h"

/* from video.c */
extern char *dirty_old;
extern char *dirty_new;
extern int gfx_xoffset;
extern int gfx_yoffset;
extern int gfx_display_lines;
extern int gfx_display_columns;
extern int gfx_width;
extern int gfx_height;
extern int x_aspect_lookup[];
extern int y_aspect_lookup[];

extern int skiplines;
extern int skipcolumns;

#ifdef USE_DMA
#define SCREEN8 dma_ptr	//od_screen16
#define SCREEN16 dma_ptr	//od_screen16
#define FLIP_VIDEO { }
#else
#define SCREEN8 od_screen16
#define SCREEN16 od_screen16
#define FLIP_VIDEO odx_video_flip();
#endif

extern int video_scale;

#include "minimal.h"

UINT32 *palette_16bit_lookup;

#define BLIT_8_TO_16_half(DEST,SRC,LEN)  \
{ \
	register int n = LEN>>3; \
	register unsigned int *u32dest = (unsigned int *)DEST; \
	register unsigned char *src = SRC; \
	while (n) \
	{ \
		*u32dest++ = (odx_palette[*(src+3)] << 16) | odx_palette[*src]; \
		src+=4; \
		*u32dest++ = (odx_palette[*(src+3)] << 16) | odx_palette[*src]; \
		 src+=4; \
		*u32dest++ = (odx_palette[*(src+3)] << 16) | odx_palette[*src]; \
		src+=4; \
		*u32dest++ = (odx_palette[*(src+3)] << 16) | odx_palette[*src]; \
		src+=4; \
		n--; \
	} \
	if ((n=(LEN&7))) \
	{ \
		register unsigned short *u16dest = ((unsigned short *)DEST) + (LEN & (~7)); \
		do { \
			*u16dest++=odx_palette_rgb[*src]; \
			src+=2; \
		} while (--n); \
	} \
}

#define BLIT_8_TO_16_32bit(DEST,SRC,LEN)  \
{ \
	register int n = LEN>>3; \
	register unsigned int *u32dest = (unsigned int *)DEST; \
	register unsigned char *src = SRC; \
	while (n) \
	{ \
		 *u32dest++ = (odx_palette_rgb[*(src + 1)] << 16) | odx_palette_rgb[*src];  \
		 src += 2; \
		 *u32dest++ = (odx_palette_rgb[*(src + 1)] << 16) | odx_palette_rgb[*src];  \
		 src += 2; \
		 *u32dest++ = (odx_palette_rgb[*(src + 1)] << 16) | odx_palette_rgb[*src];  \
		 src += 2; \
		 *u32dest++ = (odx_palette_rgb[*(src + 1)] << 16) | odx_palette_rgb[*src];  \
		 src += 2; \
		n--; \
	} \
	if ((n=(LEN&7))) \
	{ \
		register unsigned short *u16dest = ((unsigned short *)DEST) + (LEN & (~7)); \
		do { \
			*u16dest++=odx_palette_rgb[*src++]; \
		} while (--n); \
	} \
}

// int width=(bitmap->line[1] - bitmap->line[0])>>1;
// unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns, *bitmap_line;
// register unsigned short *address = SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width)
unsigned short *blit_dest = NULL;
int scan_width = 0;

void init_blit_data()
	{
	blit_dest = SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);
	scan_width = gfx_width * 2;
	}

INLINE unsigned int mix_color16 (unsigned int color1, unsigned int color2)
{
    return odx_video_color16((odx_video_getr16(color1)+odx_video_getr16(color2))>>1,(odx_video_getg16(color1)+odx_video_getg16(color2))>>1,(odx_video_getb16(color1)+odx_video_getb16(color2))>>1,0);
}

void blitscreen_dirty1_color8(struct osd_bitmap *bitmap)
{
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0]);
	unsigned char *lb=bitmap->line[skiplines] + skipcolumns;

	if (SDL_MUSTLOCK(video)) SDL_LockSurface(video);

	register unsigned short *address=SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				int h;
				unsigned char *lb0 = lb + x;
				register unsigned short *address0 = address + x;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    			w += 16;
				if (x + w > gfx_display_columns)
                    			w = gfx_display_columns - x;
				for (h = 0; ((h < 16) && ((y + h) < gfx_display_lines)); h++)
				{
					BLIT_8_TO_16_32bit(address0,lb0,w);
					//memcpy(address0,lb0,w);
					lb0 += width;
					address0 += gfx_width;
				}
			}
			x += w;
        	}
		lb += 16 * width;
		address += 16 * gfx_width;
	}
	if (SDL_MUSTLOCK(video)) SDL_UnlockSurface(video);
	FLIP_VIDEO
}

INLINE void blitscreen_dirty0_color8_noscale(struct osd_bitmap *bitmap)
//void blitscreen_dirty0_color8(struct osd_bitmap *bitmap)
{
	int y=gfx_display_lines;
	//int y;
	int width=(bitmap->line[1] - bitmap->line[0]);
	int columns=gfx_display_columns;
	unsigned char *lb = bitmap->line[skiplines] + skipcolumns;
	register unsigned short *address = SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

    do
	//for (y = 0; y < gfx_display_lines; y++)
	{
		BLIT_8_TO_16_32bit(address,lb,columns);
		//memcpy(address,lb,columns);
		lb+=width;
		address+=gfx_width;
		y--;
	}
	while (y);
	//FLIP_VIDEO
}

INLINE void blitscreen_dirty0_color8_doublevertical(struct osd_bitmap *bitmap)
//void blitscreen_dirty0_color8(struct osd_bitmap *bitmap)
{
	int y=gfx_display_lines;
	//int y;
	int width=(bitmap->line[1] - bitmap->line[0]);
	int columns=gfx_display_columns;
	unsigned char *lb = bitmap->line[skiplines] + skipcolumns;
	register unsigned short *address = SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);
	
    do
	//for (y = 0; y < gfx_display_lines; y++)
	{
		BLIT_8_TO_16_32bit(address,lb,columns);
		//memcpy(address,lb,columns);
		address+=gfx_width;
		BLIT_8_TO_16_32bit(address,lb,columns);
		address+=gfx_width;
		lb+=width;
		y--;
	}
	while (y);
	//FLIP_VIDEO
}

INLINE void blitscreen_dirty0_color8_halfscale(struct osd_bitmap *bitmap)
{
	int y=(gfx_display_lines>>1);
	int width=(bitmap->line[1] - bitmap->line[0]);
	int columns=gfx_display_columns>>1;
	unsigned char *lb = bitmap->line[skiplines] + skipcolumns;
	register unsigned short *address = SCREEN8 + (gfx_xoffset/2) + ((gfx_yoffset/2) * gfx_width);
	
	do
	{
		//memcpy(address,lb,columns);
		BLIT_8_TO_16_half(address,lb,columns)
		lb+=(width*2);
		address+=gfx_width;
		y--;
	}
	while (y);
}

INLINE void blitscreen_dirty0_color8_horzscale(struct osd_bitmap *bitmap)
{
	register unsigned short *buffer_scr = (unsigned short *)(SCREEN8+gfx_xoffset+(ODX_SCREEN_WIDTH*gfx_yoffset));
	unsigned char *buffer_mem = (unsigned char *)(bitmap->line[skiplines]+skipcolumns);
	int buffer_mem_offset = (bitmap->line[1] - bitmap->line[0])-gfx_width;
	int step,i;
	int x,y=gfx_display_lines;
	
	if (gfx_width>ODX_SCREEN_WIDTH)
	{
		/* Strech */
		step=ODX_SCREEN_WIDTH/(gfx_width-ODX_SCREEN_WIDTH);
		do {
			x=ODX_SCREEN_WIDTH; i=step;
			do {
				*buffer_scr++=(odx_palette_rgb[*buffer_mem++]);
				x--; i--;
				if (!i) { buffer_mem++; i=step; }
			} while (x);
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
	else
	{
		/* Scale */
		step=ODX_SCREEN_WIDTH/(ODX_SCREEN_WIDTH-gfx_width);
		do {
			x=ODX_SCREEN_WIDTH; i=1;
			do {
				i--;
				if (i) { *buffer_scr++=(odx_palette_rgb[*buffer_mem++]); }
				else { *buffer_scr++=(odx_palette_rgb[*buffer_mem]); i=step; }
				x--;
			} while (x);						
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
}

/* video_scale = 3/4 Best fit blitters */
extern unsigned int iAddX;
extern unsigned int iModuloX;
extern unsigned int iAddY;
extern unsigned int iModuloY;

INLINE void blitscreen_dirty0_color8_fitscale_merge1(struct osd_bitmap *bitmap)
{
	unsigned int lines=gfx_display_lines;
	unsigned int columns=gfx_display_columns;
	unsigned int src_width=bitmap->line[1] - bitmap->line[0];
	unsigned int dst_width=gfx_width;
	unsigned char *src=bitmap->line[skiplines] + skipcolumns;
	register unsigned short *dst=SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

	unsigned int _iAddY =iAddY;
	unsigned int _iModuloY = iModuloY;
	unsigned int _iAddX = iAddX;
	unsigned int _iModuloX = iModuloX;

	unsigned int merge = 0;
	int accumulatorY = 0;

	unsigned int h;
	for (h = 0; h < lines; h++)
	{
		register unsigned int pixel = 0;
		register unsigned short *dstX=dst;
		register unsigned char *srcX=src;
		register int accumulatorX = 0;
		register int w;

		for (w = columns - 1; w >= 0; w--)
		{
			register unsigned int srcPixel = odx_palette_rgb[*srcX++];
			if (accumulatorX >= _iAddX)
			{
				srcPixel = mix_color16(pixel, srcPixel);
			}
			pixel = srcPixel;

			accumulatorX += _iAddX;
			if (accumulatorX >= _iModuloX || w == 0)
			{
				if (merge)
				{
					pixel = mix_color16(*dstX, pixel);
				}
				*dstX = pixel;
				dstX++;
				accumulatorX -= _iModuloX;
			}
		}

		accumulatorY += _iAddY;
		if (accumulatorY >= _iModuloY || h == lines - 1)
		{
			dst += dst_width;
			merge = 0;
			accumulatorY -= _iModuloY;
		}
		else
		{
			merge = 1;
		}
		src += src_width; // Next src line
	}
}

INLINE void blitscreen_dirty0_color8_fitscale_merge0(struct osd_bitmap *bitmap)
{
	unsigned int lines=gfx_display_lines;
	unsigned int columns=gfx_display_columns;
	unsigned int src_width=bitmap->line[1] - bitmap->line[0];
	unsigned int dst_width=gfx_width;
	unsigned char *src=bitmap->line[skiplines] + skipcolumns;
	register unsigned short *dst=SCREEN8 + gfx_xoffset + (gfx_yoffset * gfx_width);

	unsigned int _iAddY =iAddY;
	unsigned int _iModuloY = iModuloY;
	unsigned int _iAddX = iAddX;
	unsigned int _iModuloX = iModuloX;

	int accumulatorY = 0;
	unsigned int h;

	for (h = 0; h < lines; h++)
	{
		register unsigned int pixel = 0;
		register unsigned short *dstX=dst;
		register unsigned char *srcX=src;
		register int accumulatorX = 0;

		accumulatorY += _iAddY;
		if (accumulatorY >= _iModuloY || h == lines - 1)
		{
			accumulatorY -= _iModuloY;
			register int w;

			for (w = columns - 1; w >= 0; w--)
			{
				register unsigned int srcPixel = odx_palette_rgb[*srcX++];
				if (accumulatorX >= _iAddX)
				{
					srcPixel = mix_color16(pixel, srcPixel);
				}
				pixel = srcPixel;

				accumulatorX += _iAddX;
				if (accumulatorX >= _iModuloX || w == 0)
				{
					*dstX++ = pixel;
					accumulatorX -= _iModuloX;
				}
			}
			dst += dst_width;
		}
		src += src_width;
	}
}

void blitscreen_dirty0_color8(struct osd_bitmap *bitmap)
{
	if (SDL_MUSTLOCK(video)) SDL_LockSurface(video);
	if (video_scale == 1) /* Horizontal Only */
	{
		blitscreen_dirty0_color8_horzscale(bitmap);
	}
	else if (video_scale == 2) /* Half Scale */
	{
		blitscreen_dirty0_color8_halfscale(bitmap);
	}
	else if (video_scale == 3) /* Best Fit Scale */
	{
		blitscreen_dirty0_color8_fitscale_merge1(bitmap);
	}
	else if (video_scale == 4) /* Fast Fit Scale */
	{
		blitscreen_dirty0_color8_fitscale_merge0(bitmap);
	}
	else if (video_scale == 5) /* Double Vertical */
	{
		blitscreen_dirty0_color8_doublevertical(bitmap);
	}
	else /* Default is normal blit with no scaling */
	{
		blitscreen_dirty0_color8_noscale(bitmap);
	}
	if (SDL_MUSTLOCK(video)) SDL_UnlockSurface(video);
	FLIP_VIDEO
}

void blitscreen_dirty1_palettized16(struct osd_bitmap *bitmap)
{
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned short *lb=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;

	if (SDL_MUSTLOCK(video)) SDL_LockSurface(video);
	
	register unsigned short *address=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				int h;
				unsigned short *lb0 = lb + x;
				register unsigned short *address0 = address + x;
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
	
	if (SDL_MUSTLOCK(video)) SDL_UnlockSurface(video);
	FLIP_VIDEO
}

INLINE void blitscreen_dirty0_palettized16_noscale(struct osd_bitmap *bitmap)
//void blitscreen_dirty0_palettized16(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *address = SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	for (y = 0; y < gfx_display_lines; y++)
	{
		for (x = 0; x < columns; x++)
		{
			address[x] = palette_16bit_lookup[lb[x]];
		}
		lb+=width;
		address+=gfx_width;
	}
}

INLINE void blitscreen_dirty0_palettized16_scanlines(struct osd_bitmap *bitmap)
//void blitscreen_dirty0_palettized16(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *address = blit_dest;

	for (y = 0; y < gfx_display_lines; y++)
	{
		for (x = 0; x < columns; x++)
		{
			address[x] = palette_16bit_lookup[lb[x]];
		}
		address = address + scan_width;
		lb+=width;
	}
}

INLINE void blitscreen_dirty0_palettized16_doublevertical(struct osd_bitmap *bitmap)
//void blitscreen_dirty0_palettized16(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *address = blit_dest, *address2;

	for (y = 0; y < gfx_display_lines; y++)
	{
		for (x = 0; x < columns; x++)
		{
			address[x] = palette_16bit_lookup[lb[x]];
		}
		address2 = address + gfx_width;
		memcpy(address2, address, columns * sizeof(short));
		address = address2 + gfx_width;
		lb+=width;
	}
}

INLINE void blitscreen_dirty0_palettized16_aspect_fast(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns, *bitmap_line;
	register unsigned short *address = blit_dest;

	int source_line = 0;
	bool copy_line = gfx_display_columns < gfx_width ? true : false;

	for (y = 0; y < gfx_height; y++)
	{
		source_line = y_aspect_lookup[y];
		bitmap_line = lb + (source_line * width);

		if(copy_line)
			{
			for (x = 0; x < columns; x++)
				{
				address[x] = palette_16bit_lookup[bitmap_line[x]];
				}
			}
		else
			{
			for (x = 0; x < gfx_width; x++)
				address[x] = palette_16bit_lookup[bitmap_line[x_aspect_lookup[x]]];
			}

		address+=gfx_width;
	}
}

INLINE void blitscreen_dirty0_palettized16_aspect(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns, *bitmap_line;
	register unsigned short *address = blit_dest;

	int source_line = 0, source_x;
	bool copy_line = gfx_display_columns < gfx_width ? true : false;

	for (y = 0; y < gfx_height; y++)
	{
		source_line = y_aspect_lookup[y];
		bitmap_line = lb + (source_line * width);

		if(copy_line)
			{
			for (x = 0; x < columns; x++)
				{
				address[x] = palette_16bit_lookup[bitmap_line[x]];
				}
			}
		else
			{
			for (x = 1; x < gfx_width; x++)
				{
				source_x = x_aspect_lookup[x];
				address[x] = mix_color16(palette_16bit_lookup[bitmap_line[source_x]],
							palette_16bit_lookup[bitmap_line[source_x - 1]]);	
				}
			}

		address+=gfx_width;
	}
}

INLINE void blitscreen_dirty0_palettized16_aspect_full(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns, *bitmap_line;
	register unsigned short *address = blit_dest;

	int source_line = 0;

	for (y = 0; y < gfx_height; y++)
	{
		source_line = y_aspect_lookup[y];
		bitmap_line = lb + (source_line * width);

		for (x = 0; x < gfx_width; x++)
			address[x] = palette_16bit_lookup[bitmap_line[x_aspect_lookup[x]]];

		address+=gfx_width;
	}
}

INLINE void blitscreen_dirty0_palettized16_horzscale(struct osd_bitmap *bitmap)
{
	register unsigned short *buffer_scr = SCREEN16 + gfx_xoffset + (ODX_SCREEN_WIDTH*gfx_yoffset);
	unsigned short *buffer_mem = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	int buffer_mem_offset = ((bitmap->line[1] - bitmap->line[0])>>1)-gfx_width;
	int step,i;
	int x,y=gfx_display_lines;
	
	if (gfx_width>ODX_SCREEN_WIDTH)
	{
		/* Strech */
		step=ODX_SCREEN_WIDTH/(gfx_width-ODX_SCREEN_WIDTH);
		do {
			x=ODX_SCREEN_WIDTH; i=step;
			do {
				*buffer_scr++=palette_16bit_lookup[*buffer_mem++];
				x--; i--;
				if (!i) { if (x) { *buffer_scr++=mix_color16(palette_16bit_lookup[*buffer_mem++],palette_16bit_lookup[*buffer_mem++]); x--; i=step-1; } else { buffer_mem++; i=step; } }
			} while (x);
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
	else
	{
		/* Scale */
		step=ODX_SCREEN_WIDTH/(ODX_SCREEN_WIDTH-gfx_width);
		do {
			x=ODX_SCREEN_WIDTH; i=1;
			do {
				i--;
				if (i) { *buffer_scr++=palette_16bit_lookup[*buffer_mem++]; }
				else { *buffer_scr++=mix_color16(palette_16bit_lookup[buffer_mem[0]],palette_16bit_lookup[buffer_mem[-1]]); i=step; }
				x--;
			} while (x);						
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
}

INLINE void blitscreen_dirty0_palettized16_halfscale(struct osd_bitmap *bitmap)
{
	int x,y;
	unsigned int lines=gfx_display_lines>>1;
	int width=bitmap->line[1] - bitmap->line[0];
	int columns=(gfx_display_columns>>1);
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *address = SCREEN16 + (gfx_xoffset/2) + ((gfx_yoffset/2) * gfx_width);

	for (y = 0; y < lines; y++)
	{
		for (x = 0; x < columns; x++)
		{
			address[x] = mix_color16(palette_16bit_lookup[lb[(x<<1)]], palette_16bit_lookup[lb[(x<<1)+1]]);
		}
		lb+=width;
		address+=gfx_width;
	}
}

INLINE void blitscreen_dirty0_palettized16_fitscale_merge0(struct osd_bitmap *bitmap)
{
	unsigned int lines=gfx_display_lines;
	unsigned int columns=gfx_display_columns;
	unsigned int src_width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned int dst_width=gfx_width;
	unsigned short *src=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *dst=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	unsigned int _iAddY =iAddY;
	unsigned int _iModuloY = iModuloY;
	unsigned int _iAddX = iAddX;
	unsigned int _iModuloX = iModuloX;

	int accumulatorY = 0;
	unsigned int h;

	for (h = 0; h < lines; h++)
	{
		accumulatorY += _iAddY;
		if (accumulatorY >= _iModuloY || h == lines - 1)
		{
			register unsigned int pixel = 0;
			register unsigned short *dstX=dst;
			register unsigned short *srcX=src;
			register int accumulatorX = 0;

			accumulatorY -= _iModuloY;
			register int w;

			for (w = columns - 1; w >= 0; w--)
			{
				register unsigned int srcPixel = palette_16bit_lookup[*srcX++];
				if (accumulatorX >= _iAddX)
				{
					srcPixel = mix_color16(pixel, srcPixel);
				}
				pixel = srcPixel;

				accumulatorX += _iAddX;
				if (accumulatorX >= _iModuloX || w == 0)
				{
					*dstX++ = pixel;
					accumulatorX -= _iModuloX;
				}
			}
			dst += dst_width;
		}

		src += src_width;
	}
}

INLINE void blitscreen_dirty0_palettized16_fitscale_merge1(struct osd_bitmap *bitmap)
{
	unsigned int lines=gfx_display_lines;
	unsigned int columns=gfx_display_columns;
	unsigned int src_width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned int dst_width=gfx_width;
	unsigned short *src=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *dst=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	unsigned int _iAddY =iAddY;
	unsigned int _iModuloY = iModuloY;
	unsigned int _iAddX = iAddX;
	unsigned int _iModuloX = iModuloX;

	unsigned int merge = 0;
	int accumulatorY = 0;
	unsigned int h;

	for (h = 0; h < lines; h++)
	{
		register unsigned int pixel = 0;
		register unsigned short *dstX=dst;
		register unsigned short *srcX=src;
		register int accumulatorX = 0;
		register int w;

		for (w = columns - 1; w >= 0; w--)
		{
			register unsigned int srcPixel = palette_16bit_lookup[*srcX++];
			if (accumulatorX >= _iAddX)
			{
				srcPixel = mix_color16(pixel, srcPixel);
			}
			pixel = srcPixel;

			accumulatorX += _iAddX;
			if (accumulatorX >= _iModuloX || w == 0)
			{
				if (merge)
				{
					pixel = mix_color16(*dstX, pixel);
				}
				*dstX = pixel;
				dstX++;
				accumulatorX -= _iModuloX;
			}
		}

		accumulatorY += _iAddY;
		if (accumulatorY >= _iModuloY || h == lines - 1)
		{
			dst += dst_width;
			merge = 0;
			accumulatorY -= _iModuloY;
		}
		else
		{
			merge = 1;
		}
		src += src_width; // Next src line
	}
}

void blitscreen_dirty0_palettized16(struct osd_bitmap *bitmap)
{
#ifndef USE_DMA
	if (SDL_MUSTLOCK(video)) SDL_LockSurface(video);
#endif
	switch(video_scale)
		{
		case 1:		// Horizontal Only
			blitscreen_dirty0_palettized16_horzscale(bitmap);
			break;

		case 2:		// Half Scale
			blitscreen_dirty0_palettized16_halfscale(bitmap);
			break;

		case 3:		// Best Fit Scale
			blitscreen_dirty0_palettized16_fitscale_merge1(bitmap);
			break;

		case 4: 	// Fast Fit Scale
			blitscreen_dirty0_palettized16_fitscale_merge0(bitmap);
			break;

		case 5:		// Double Vertical
			blitscreen_dirty0_palettized16_doublevertical(bitmap);
			break;

		case 6:		// Scale aspect
			blitscreen_dirty0_palettized16_aspect(bitmap);
			break;

		case 7:		// Scale aspect fast
			blitscreen_dirty0_palettized16_aspect_fast(bitmap);
			break;

		case 8:		// Full screen
			blitscreen_dirty0_palettized16_aspect_full(bitmap);
			break;

		case 9:		// Scanlines
			blitscreen_dirty0_palettized16_scanlines(bitmap);
			break;

		default:
			blitscreen_dirty0_palettized16_noscale(bitmap);
			break;
		}

#ifndef USE_DMA
	if (SDL_MUSTLOCK(video)) SDL_UnlockSurface(video);
	FLIP_VIDEO
#endif
}

void blitscreen_dirty1_color16(struct osd_bitmap *bitmap)
{
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned short *lb=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *address=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

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
					memcpy(address0,lb0,w<<1);
					lb0 += width;
					address0 += gfx_width;
				}
			}
			x += w;
        	}
		lb += 16 * width;
		address += 16 * gfx_width;
	}
	FLIP_VIDEO
}

INLINE void blitscreen_dirty0_color16_noscale(struct osd_bitmap *bitmap)
//void blitscreen_dirty0_color16(struct osd_bitmap *bitmap)
{
	int y=gfx_display_lines;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns<<1;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *address = SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	do
	{
	    memcpy(address,lb,columns);
		lb+=width;
		address+=gfx_width;
		y--;
	}
	while (y);
	//FLIP_VIDEO
}

INLINE void blitscreen_dirty0_color16_scanlines(struct osd_bitmap *bitmap)
//void blitscreen_dirty0_color16(struct osd_bitmap *bitmap)
{
	int y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns<<1;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *address = blit_dest;

	for(y = 0; y < gfx_display_lines; y++)
		{
		memcpy(address,lb,columns);
		address = address + scan_width;
		lb+=width;
		}

	//FLIP_VIDEO
}

INLINE void blitscreen_dirty0_color16_doublevertical(struct osd_bitmap *bitmap)
//void blitscreen_dirty0_color16(struct osd_bitmap *bitmap)
{
	int y=gfx_display_lines;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns<<1;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *address = blit_dest;

	do
	{
	    memcpy(address,lb,columns);
		address+=gfx_width;
	    memcpy(address,lb,columns);
		lb+=width;
		address+=gfx_width;
		y--;
	}
	while (y);
	//FLIP_VIDEO
}

INLINE void blitscreen_dirty0_color16_aspect_fast(struct osd_bitmap *bitmap)
{
	int y,x;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns<<1;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns, *bitmap_line;
	register unsigned short *address = blit_dest;

	int source_line = 0;
	bool copy_line = gfx_display_columns < gfx_width ? true : false;

	for(y = 0; y < gfx_height; y ++)
	{
		source_line = y_aspect_lookup[y];
		if(copy_line)
			{
			memcpy(address, lb + (source_line * width), columns);
			}
		else
			{
			bitmap_line = lb + (source_line * width);

			for(x = 0; x < gfx_width; x++)
				address[x] = bitmap_line[x_aspect_lookup[x]];
			}
		address+=gfx_width;
	}
}

INLINE void blitscreen_dirty0_color16_aspect(struct osd_bitmap *bitmap)
{
	int y,x;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	int columns=gfx_display_columns<<1;
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns, *bitmap_line;
	register unsigned short *address = blit_dest;

	int source_line = 0,  source_x;
	bool copy_line = gfx_display_columns < gfx_width ? true : false;

	for(y = 0; y < gfx_height; y++)
	{
		source_line = y_aspect_lookup[y];
		if(copy_line)
			{
			memcpy(address, lb + (source_line * width), columns);
			}
		else
			{
			bitmap_line = lb + (source_line * width);

			for(x = 1; x < gfx_width; x++)
				{
				source_x = x_aspect_lookup[x];
				address[x] = mix_color16(bitmap_line[source_x],
							bitmap_line[source_x - 1]);
				}
			}
		address+=gfx_width;
	}
}

INLINE void blitscreen_dirty0_color16_aspect_full(struct osd_bitmap *bitmap)
{
	int x,y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;

	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns, *bitmap_line;
	register unsigned short *address = blit_dest;

	int source_line = 0;

	for (y = 0; y < gfx_height; y++)
	{
		source_line = y_aspect_lookup[y];
		bitmap_line = lb + (source_line * width);

		for(x = 0; x < gfx_width; x++)
			address[x] = bitmap_line[x_aspect_lookup[x]];

		address+=gfx_width;
	}
}

INLINE void blitscreen_dirty0_color16_horzscale(struct osd_bitmap *bitmap)
{
	unsigned short *buffer_scr = SCREEN16 + gfx_xoffset + (ODX_SCREEN_WIDTH*gfx_yoffset);
	register unsigned short *buffer_mem = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	int buffer_mem_offset = ((bitmap->line[1] - bitmap->line[0])>>1)-gfx_width;
	int step,i;
	int x,y=gfx_display_lines;
	
	if (gfx_width>ODX_SCREEN_WIDTH)
	{
		/* Strech */
		step=ODX_SCREEN_WIDTH/(gfx_width-ODX_SCREEN_WIDTH);
		do {
			x=ODX_SCREEN_WIDTH; i=step;
			do {
				*buffer_scr++=*buffer_mem++;
				x--; i--;
				if (!i) { if (x) { *buffer_scr++=mix_color16(*buffer_mem++,*buffer_mem++); x--; i=step-1; } else { buffer_mem++; i=step; } }
			} while (x);
			buffer_mem+=buffer_mem_offset;		
			y--;
		} while (y);
	}
	else
	{
		/* Scale */
		step=ODX_SCREEN_WIDTH/(ODX_SCREEN_WIDTH-gfx_width);
		do {
			x=ODX_SCREEN_WIDTH; i=1;
			do {
				i--;
				if (i) { *buffer_scr++=*buffer_mem++; }
				else { *buffer_scr++=mix_color16(buffer_mem[0],buffer_mem[-1]); i=step; }
				x--;
			} while (x);
			buffer_mem+=buffer_mem_offset;
			y--;
		} while (y);
	}
}

INLINE void blitscreen_dirty0_color16_halfscale(struct osd_bitmap *bitmap)
{
	int x,y;
	unsigned int lines=gfx_display_lines>>1;
	int width=bitmap->line[1] - bitmap->line[0];
	int columns=(gfx_display_columns>>1);
	unsigned short *lb = ((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *address = SCREEN16 + (gfx_xoffset/2) + ((gfx_yoffset/2) * gfx_width);

	for (y = 0; y < lines; y++)
	{
			for (x = 0; x < columns; x++)
			{
					address[x] = mix_color16(lb[(x<<1)], lb[(x<<1)+1]);
			}
			lb+=width;
			address+=gfx_width;
	}
}

INLINE void blitscreen_dirty0_color16_fitscale_merge0(struct osd_bitmap *bitmap)
{
	unsigned int lines=gfx_display_lines;
	unsigned int columns=gfx_display_columns;
	unsigned int src_width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned int dst_width=gfx_width;
	unsigned short *src=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *dst=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	unsigned int _iAddY =iAddY;
	unsigned int _iModuloY = iModuloY;
	unsigned int _iAddX = iAddX;
	unsigned int _iModuloX = iModuloX;

	int accumulatorY = 0;
	unsigned int h;

	for (h = 0; h < lines; h++)
	{
		accumulatorY += _iAddY;
		if (accumulatorY >= _iModuloY || h == lines - 1)
		{
		register unsigned int pixel = 0;
		register unsigned short *dstX=dst;
		register unsigned short *srcX=src;
		register int accumulatorX = 0;

		accumulatorY -= _iModuloY;
		register int w;

		for (w = columns - 1; w >= 0; w--)
		{
			register unsigned int srcPixel = *srcX++;
			if (accumulatorX >= _iAddX)
			{
				srcPixel = mix_color16(pixel, srcPixel);
			}
			pixel = srcPixel;

			accumulatorX += _iAddX;
			if (accumulatorX >= _iModuloX || w == 0)
			{
				*dstX++ = pixel;
				accumulatorX -= _iModuloX;
			}
		}
		dst += dst_width;
	}

	src += src_width;
	}
}

INLINE void blitscreen_dirty0_color16_fitscale_merge1(struct osd_bitmap *bitmap)
{
	unsigned int lines=gfx_display_lines;
	unsigned int columns=gfx_display_columns;
	unsigned int src_width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned int dst_width=gfx_width;
	unsigned short *src=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;
	register unsigned short *dst=SCREEN16 + gfx_xoffset + (gfx_yoffset * gfx_width);

	unsigned int _iAddY =iAddY;
	unsigned int _iModuloY = iModuloY;
	unsigned int _iAddX = iAddX;
	unsigned int _iModuloX = iModuloX;

	unsigned int merge = 0;
	int accumulatorY = 0;
	unsigned int h;

	for (h = 0; h < lines; h++)
	{
		register unsigned int pixel = 0;
		register unsigned short *dstX=dst;
		register unsigned short *srcX=src;
		register int accumulatorX = 0;
		register int w;

		for (w = columns - 1; w >= 0; w--)
		{
			register unsigned int srcPixel = *srcX++;
			if (accumulatorX >= _iAddX)
			{
				srcPixel = mix_color16(pixel, srcPixel);
			}
			pixel = srcPixel;
  
			accumulatorX += _iAddX;
			if (accumulatorX >= _iModuloX || w == 0)
			{
				if (merge)
				{
				pixel = mix_color16(*dstX, pixel);
				}
				*dstX = pixel;
				dstX++;
				accumulatorX -= _iModuloX;
			}
		}

		accumulatorY += _iAddY;
		if (accumulatorY >= _iModuloY || h == lines - 1)
		{
			dst += dst_width;
			merge = 0;
			accumulatorY -= _iModuloY;
		}
		else
		{
			merge = 1;
		}
		src += src_width; // Next src line
	}
}

void blitscreen_dirty0_color16(struct osd_bitmap *bitmap)
{
#ifndef USE_DMA
	if (SDL_MUSTLOCK(video)) SDL_LockSurface(video);
#endif

	switch(video_scale)
		{
		case 1:		// Horizontal Only
			blitscreen_dirty0_color16_horzscale(bitmap);
			break;

		case 2:		// Half Scale
			blitscreen_dirty0_color16_halfscale(bitmap);
			break;

		case 3:		// Best Fit Scale
			blitscreen_dirty0_color16_fitscale_merge1(bitmap);
			break;

		case 4: 	// Fast Fit Scale
			blitscreen_dirty0_color16_fitscale_merge0(bitmap);
			break;

		case 5:		// Double Vertical
			blitscreen_dirty0_color16_doublevertical(bitmap);
			break;

		case 6:		// Scale aspect
			blitscreen_dirty0_color16_aspect(bitmap);
			break;

		case 7:		// Scale aspect fast
			blitscreen_dirty0_color16_aspect_fast(bitmap);
			break;

		case 8:		// Full screen
			blitscreen_dirty0_color16_aspect_full(bitmap);
			break;

		case 9:		// Scanlines
			blitscreen_dirty0_color16_scanlines(bitmap);
			break;

		default:
			blitscreen_dirty0_color16_noscale(bitmap);
			break;
		}

#ifndef USE_DMA
	if (SDL_MUSTLOCK(video)) SDL_UnlockSurface(video);
	FLIP_VIDEO
#endif
}

#ifdef __cplusplus
}
#endif
