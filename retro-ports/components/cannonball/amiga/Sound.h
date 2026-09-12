
#ifndef _INC_SOUND_H
#define _INC_SOUND_H

#ifdef __cplusplus
#define PROTOHEADER extern "C"
#else
#define PROTOHEADER
#endif

PROTOHEADER unsigned char *SND_LoadModule( char *path );
/* load and play (if path is empty, resume) */
PROTOHEADER int SND_PlayModule( unsigned char *buf );
/* stop playing but keep module */
PROTOHEADER int SND_StopModule( void );
/* clear module */
PROTOHEADER int SND_EjectModule( unsigned char*buf );

PROTOHEADER void SND_SetFXChannel( unsigned int channel );
PROTOHEADER void SND_SetFX( unsigned int fx );

#define FXSIGNAL2  	0x10de5000
#define FXSIGNAL1  	0x10de4000
#define FXGETREADY 	0x10de3000
#define FXCONGRATS 	0x10de2000
#define FXCOININ 		0x10de1000
#define FXCHECKPOINT	0x10de0000

#endif /* _INC_SOUND_H */
