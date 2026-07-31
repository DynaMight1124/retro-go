/*
  Sound.c

  author: Henryk Richter <henryk.richter@gmx.net>

  purpose: adapter Code for music player

  test build:
   m68k-amigaos-gcc -o Sound Sound.c PTPlay30B.o -noixemul -DTESTSOUND
*/

#include "Sound.h"
#include "PTPlay30B.h"
#include <stdio.h>

#include <exec/types.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <dos/dos.h>
#include <dos/rdargs.h>

#include <proto/exec.h>
#include <proto/dos.h>

#ifdef TESTSOUND 
int main( int argc, char **argv )
{
	char *modname = "data/mod.194x";
	unsigned char*mod;

	if( (mod = SND_LoadModule( modname )) )
	{
		SND_PlayModule( mod );

		Delay( 50*160 ); /* simple: wait 60 seconds */

		SND_StopModule(); /* not exactly needed, EjectModule does this */

		SND_EjectModule( mod ); /* loadmodule/ejectmodule pair */
	}
	else
		printf("cannot load module %s\n",modname);
	

	return 0;
}
#endif

/* can play only one file at a time */
unsigned char *SND_curmod = NULL;
unsigned char SND_playing = 0;

/* sanity check for input file: accept mods <= 2MB */
#define MAX_MODSIZE 2000000

/* multiple modules could be loaded - but on the other hand chipmem is scarce */
/* loadmodule won't start playing (yet) */
unsigned char *SND_LoadModule( char *path )
{
	unsigned char *buf = NULL;
	FILE *ifile;

	ifile = fopen( path, "rb" );
	if( ifile )
	{
	 int src_size;

	 fseek(ifile,0,SEEK_END);
	 src_size=ftell(ifile);
	 rewind(ifile);

	 /* sanity check: max. 2 MB (right now) */
	 if( (src_size > 0) && (src_size < MAX_MODSIZE) )
	 {
	  if( (buf = AllocVec( src_size, MEMF_CHIP ) ) )
	  {
		fread(buf, 1, src_size, ifile );
	  }
	 }

	 fclose( ifile );
	}
	
	return buf;
}


/*
  stop playing and discard module
*/
PROTOHEADER int SND_EjectModule( unsigned char *buf )
{
	if( buf )
	{
		SND_StopModule();
		FreeVec( buf );
	}
}

/*
  Load file and start playing

  if buf is empty, previous module will be played (if present)
*/
PROTOHEADER int SND_PlayModule( unsigned char *buf )
{
	if( !buf )
		buf = SND_curmod;

	if( buf )
	{
		if( SND_playing )
			PT_StopPlay();

		SND_curmod = buf;
		PT_StartPlay( buf );
		SND_playing = 1;

		return 1;
	}
	else
		return	0;
}

/*
  stop playing but keep module in memory
*/
PROTOHEADER int SND_StopModule( void )
{
	if( SND_playing )
		PT_StopPlay();
	SND_playing = 0;
	return 0;
}

PROTOHEADER void SND_SetFXChannel( unsigned int channel )
{
	PT_SetFXChannel( channel );
}

PROTOHEADER void SND_SetFX( unsigned int fx )
{
	PT_SetFX( fx );
}

