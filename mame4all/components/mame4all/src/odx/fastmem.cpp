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
#define LIMIT 40
//#define TOO_SHORT_PATCH
//#define USE_TWIDLE_PATCH

#if defined(TOO_SHORT_PATCH) && !defined(USE_TWIDLE_PATCH)
#undef TOO_SHORT_PATCH
#endif

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

//#if defined(__cplusplus) && !defined(USE_CPLUS)
//extern "C" {
//void* fast_memcpy(void *OUT, const void *IN, size_t N);
//void* fast_memset(const void *DST, int C, size_t LENGTH);
//}
//#endif

void* fast_memcpy(void *OUT, const void *IN, size_t N)
{
	register unsigned char *s=(unsigned char *)IN;
	register unsigned char *d=(unsigned char *)OUT;
#ifdef USE_TWIDLE_PATCH
	if (N<LIMIT)
#else
	if ((N<LIMIT)||((((unsigned)s)&3) != (((unsigned)d)&3)))
#endif
	{
#ifdef TOO_SHORT_PATCH
		if ((N>8)&&( (((unsigned)s)&3) == (((unsigned)d)&3)))
		{
			while (((unsigned)d)&3)
			{
				*d++=*s++;
				N--;
			}
			register int n=N>>2;
			N-=n<<2;
			while (n--)
			{
				*((unsigned *)d)=*((unsigned *)s);
				d+=4;
				s+=4;
			}
		}
#endif
	}
	else
	{
#ifdef USE_TWIDLE_PATCH
		if ((((unsigned)s)&3) != (((unsigned)d)&3))
		{
			while ((((unsigned)d)&3)&&(N))
			{
				*d++=*s++;
				N--;
			}
			while ((((unsigned)d)&31)&&(N))
			{
				*((unsigned *)d)=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0];
				d+=4;
				s+=4;
				N-=4;
			}
			register int n=N>>5;
			N -= n<<5;
			unsigned *dsq=d;
			while (n--)
			{
				dsq[0]=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0]; s+=4;
				dsq[1]=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0]; s+=4;
				dsq[2]=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0]; s+=4;
				dsq[3]=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0]; s+=4;
				dsq[4]=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0]; s+=4;
				dsq[5]=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0]; s+=4;
				dsq[6]=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0]; s+=4;
				dsq[7]=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0]; s+=4;
				d+=32;
				dsq+=8;
			}
			}
			
			n=N>>2;
			N -= n<<2;
			while (n--)
			{
				*((unsigned *)d)=(s[3]<<24)|(s[2]<<16)|(s[1]<<8)|s[0];
				d+=4;
				s+=4;
			}
		}
		else
#endif
		{
			while ((((unsigned)d)&3)&&(N))
			{
				*d++=*s++;
				N--;
			}
			while ((((unsigned)d)&31)&&(N))
			{
				*((unsigned *)d)=*((unsigned *)s);
				d+=4;
				s+=4;
				N-=4;
			}
			register int n=N>>5;
			N -= n<<5;
			{
			unsigned *dsq=(unsigned *)d;
			while (n--)
			{
				dsq[0]=*((unsigned *)s); s+=4;
				dsq[1]=*((unsigned *)s); s+=4;
				dsq[2]=*((unsigned *)s); s+=4;
				dsq[3]=*((unsigned *)s); s+=4;
				dsq[4]=*((unsigned *)s); s+=4;
				dsq[5]=*((unsigned *)s); s+=4;
				dsq[6]=*((unsigned *)s); s+=4;
				dsq[7]=*((unsigned *)s); s+=4;
				dsq+=8;
				d+=32;
			}
			}
			n=N>>2;
			N -= n<<2;
			while (n--)
			{
				*((unsigned *)d)=*((unsigned *)s);
				d+=4;
				s+=4;
			}
		}
	}
	while (N--)
		*d++=*s++;
	return OUT;
}

void* fast_memset(const void *DST, int C, size_t LENGTH)
{
	register unsigned char *d=(unsigned char *)DST;
	register unsigned dat = C & 0xff;
	dat = (dat << 24) | (dat << 16) | (dat << 8) | dat;

	if (LENGTH<LIMIT)
	{
#ifdef TOO_SHORT_PATCH
		if (LENGTH>=8)
		{
			while (((unsigned)d)&3)
			{
				*d++=dat;
				LENGTH--;
			}
			register int n=LENGTH>>2;
			LENGTH-=n<<2;
			while (n--)
			{
				*((unsigned *)d)=dat;
				d+=4;
			}
		}
#endif
		while(LENGTH--)
			*d++=dat;
		return (void *)DST;
	}
	while (((unsigned)d)&3)
	{
		*d++=dat;
		LENGTH--;
	}
	while (((unsigned)d)&31)
	{
		*((unsigned *)d)=dat;
		d+=4;
		LENGTH-=4;
	}
	register int n=LENGTH>>5;
	LENGTH -= n<<5;
	if (n)
	{
		unsigned *dsq=(unsigned *)d;
		while (n--)
		{
			dsq[0]=dat;
			dsq[1]=dat;
			dsq[2]=dat;
			dsq[3]=dat;
			dsq[4]=dat;
			dsq[5]=dat;
			dsq[6]=dat;
			dsq[7]=dat;
			dsq+=8;
			d+=32;
		}
	}
	n=LENGTH>>2;
	LENGTH -= n<<2;
	while (n--)
	{
		*((unsigned *)d)=dat;
		d+=4;
	}
	while (LENGTH--)
		*d++=dat;
	return (void *)DST;
}

#ifdef __cplusplus
}
#endif
