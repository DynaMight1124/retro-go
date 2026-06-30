#ifndef RG_MAME_CORE_H
#define RG_MAME_CORE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "retrogo/rg_psram.h"
#include "driver.h"
#endif
#ifndef DECLARE

#include "driver.h"

#define DATA_TYPE unsigned char
#define DECLARE(function,args,body) void function##8 args body
#include "scanline.cpp"
#undef DATA_TYPE
#undef DECLARE

#define DATA_TYPE unsigned short
#define DECLARE(function,args,body) void function##16 args body
#include "scanline.cpp"
#undef DATA_TYPE
#undef DECLARE

#else /* DECLARE */

#define ADJUST_FOR_ORIENTATION(type, orientation, bitmap, x, y)		\
	type *dst = &((type *)bitmap->line[y])[x];                      \
	int xadv = 1;                                                   \
	if (orientation)                                                \
	{                                                               \
	    int dy = bitmap->line[1] - bitmap->line[0];                 \
	    int tx = x, ty = y, temp;                                   \
	    if (orientation & ORIENTATION_SWAP_XY)                      \
	    {                                                           \
		temp = tx; tx = ty; ty = temp;                          \
		xadv = dy / sizeof(type);                               \
	    }                                                           \
	    if (orientation & ORIENTATION_FLIP_X)                       \
	    {                                                           \
		tx = bitmap->width - 1 - tx;                            \
		if (!(orientation & ORIENTATION_SWAP_XY)) xadv = -xadv; \
	    }                                                           \
	    if (orientation & ORIENTATION_FLIP_Y)                       \
	    {                                                           \
		ty = bitmap->height - 1 - ty;                           \
		if (orientation & ORIENTATION_SWAP_XY) xadv = -xadv;    \
	    }                                                           \
	    /* can't lookup line because it may be negative! */         \
	    dst = (type *)(bitmap->line[0] + dy * ty) + tx;             \
	}


DECLARE(draw_scanline, (struct osd_bitmap *bitmap,int x,int y,int length, DATA_TYPE *src,unsigned short *pens,int transparent_pen),
{
	/* 8bpp destination */
	if (bitmap->depth == 8)
	{
		/* adjust in case we're oddly oriented */
		ADJUST_FOR_ORIENTATION(UINT8, Machine->orientation, bitmap, x, y);

		/* with pen lookups */
		if (pens)
		{
			if (transparent_pen == -1)
				while (length--)
				{
					*dst = pens[*src++];
					dst += xadv;
				}
			else
				while (length--)
				{
					UINT32 spixel = *src++;
					if (spixel != transparent_pen)
						*dst = pens[spixel];
					dst += xadv;
				}
		}

		/* without pen lookups */
		else
		{
			if (transparent_pen == -1)
				while (length--)
				{
					*dst = *src++;
					dst += xadv;
				}
			else
				while (length--)
				{
					UINT32 spixel = *src++;
					if (spixel != transparent_pen)
						*dst = spixel;
					dst += xadv;
				}
		}
	}

	/* 16bpp destination */
	else if(bitmap->depth == 16)
	{
		/* adjust in case we're oddly oriented */
		ADJUST_FOR_ORIENTATION(UINT16, Machine->orientation, bitmap, x, y);

		/* with pen lookups */
		if (pens)
		{
			if (transparent_pen == -1)
				while (length--)
				{
					*dst = pens[*src++];
					dst += xadv;
				}
			else
				while (length--)
				{
					UINT32 spixel = *src++;
					if (spixel != transparent_pen)
						*dst = pens[spixel];
					dst += xadv;
				}
		}

		/* without pen lookups */
		else
		{
			if (transparent_pen == -1)
				while (length--)
				{
					*dst = *src++;
					dst += xadv;
				}
			else
				while (length--)
				{
					UINT32 spixel = *src++;
					if (spixel != transparent_pen)
						*dst = spixel;
					dst += xadv;
				}
		}
	}
})

DECLARE(pdraw_scanline, ( struct osd_bitmap *bitmap,int x,int y,int length, DATA_TYPE *src,unsigned short *pens,int transparent_pen,int pri),
{
	/* 8bpp destination */
	if (bitmap->depth == 8)
	{
		//int dy = bitmap->rowpixels;
		extern const int safety;
		int dy = bitmap->width + (16<<1);

		//int dyp = priority_bitmap->rowpixels;
		int dyp = priority_bitmap->width + (16<<1);

		//UINT8 *dsti = (UINT8 *)bitmap->base + y * dy + x;
		//UINT8 *dstp = (UINT8 *)priority_bitmap->base + y * dyp + x;

		UINT8 *dsti = (UINT8 *)bitmap->line[0] + y * dy + x;
		UINT8 *dstp = (UINT8 *)priority_bitmap->line[0] + y * dyp + x;

		int xadv = 1;

		/* with pen lookups */
		if (pens)
		{
			if (transparent_pen == -1)
				while (length--)
				{
					*dsti = pens[*src++];
					*dstp = pri;
					dsti += xadv;
					dstp += xadv;
				}
			else
				while (length--)
				{
					UINT32 spixel = *src++;
					if (spixel != transparent_pen)
					{
						*dsti = pens[spixel];
						*dstp = pri;
					}
					dsti += xadv;
					dstp += xadv;
				}
		}

		/* without pen lookups */
		else
		{
			if (transparent_pen == -1)
				while (length--)
				{
					*dsti = *src++;
					*dstp = pri;
					dsti += xadv;
					dstp += xadv;
				}
			else
				while (length--)
				{
					UINT32 spixel = *src++;
					if (spixel != transparent_pen)
					{
						*dsti = spixel;
						*dstp = pri;
					}
					dsti += xadv;
					dstp += xadv;
				}
		}
	}

	/* 16bpp destination */
	else if(bitmap->depth == 16)
	{
		//int dy = bitmap->rowpixels;
		extern const int safety;
		int dy = bitmap->width + (16<<1);

		//int dyp = priority_bitmap->rowpixels;
		int dyp = priority_bitmap->width + (16<<1);

		//UINT16 *dsti = (UINT16 *)bitmap->base + y * dy + x;
		//UINT8 *dstp = (UINT8 *)priority_bitmap->base + y * dyp + x;

		UINT16 *dsti = (UINT16 *)bitmap->line[0] + y * dy + x;
		UINT8 *dstp = (UINT8 *)priority_bitmap->line[0] + y * dyp + x;
		int xadv = 1;

		/* with pen lookups */
		if (pens)
		{
			if (transparent_pen == -1)
				while (length--)
				{
					*dsti = pens[*src++];
					*dstp = pri;
					dsti += xadv;
					dstp += xadv;
				}
			else
				while (length--)
				{
					UINT32 spixel = *src++;
					if (spixel != transparent_pen)
					{
						*dsti = pens[spixel];
						*dstp = pri;
					}
					dsti += xadv;
					dstp += xadv;
				}
		}

		/* without pen lookups */
		else
		{
			if (transparent_pen == -1)
				while (length--)
				{
					*dsti = *src++;
					*dstp = pri;
					dsti += xadv;
					dstp += xadv;
				}
			else
				while (length--)
				{
					UINT32 spixel = *src++;
					if (spixel != transparent_pen)
					{
						*dsti = spixel;
						*dstp = pri;
					}
					dsti += xadv;
					dstp += xadv;
				}
		}
	}
})

DECLARE(extract_scanline, ( struct osd_bitmap *bitmap,int x,int y,int length, DATA_TYPE *dst),
{
	/* 8bpp destination */
	if (bitmap->depth == 8)
	{
		//int dy = bitmap->rowpixels;
		extern const int safety;
		int dy = bitmap->width + (16<<1);

		//UINT8 *src = (UINT8 *)bitmap->base + y * dy + x;
		UINT8 *src = (UINT8 *)bitmap->line[0] + y * dy + x;

		int xadv = 1;

		while (length--)
		{
			*dst++ = *src;
			src += xadv;
		}
	}

	/* 16bpp destination */
	else if(bitmap->depth == 16)
	{
		//int dy = bitmap->rowpixels;
		extern const int safety;
		int dy = bitmap->width + (16<<1);

		//UINT16 *src = (UINT16 *)bitmap->base + y * dy + x;
		UINT16 *src = (UINT16 *)bitmap->line[0] + y * dy + x;
		int xadv = 1;

		while (length--)
		{
			*dst++ = *src;
			src += xadv;
		}
	}
})

#endif /* DECLARE */
