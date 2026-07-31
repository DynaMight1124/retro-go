/*
  PTPlay30B.h

  author: Henryk Richter <henryk.richter@gmx.net>

  purpose: Interface to Protracker Playroutine

*/
#ifndef _INC_PTPLAY_H
#define _INC_PTPLAY_H

#include "asminterface.h"

ASM SAVEDS void PT_StartPlay( ASMR(a0) unsigned char *ptmod ASMREG(a0) );
ASM SAVEDS void PT_StopPlay( void );

/* set FX channel (1-4), 0 to disable */
ASM SAVEDS void PT_SetFXChannel( ASMR(d0) unsigned long fxchan ASMREG(d0) );
/* play note on FX channel */
ASM SAVEDS void PT_SetFX( ASMR(d0) unsigned long fx ASMREG(d0) );




#endif /* _INC_PTPLAY_H */
