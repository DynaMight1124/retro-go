#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <stdbool.h>

extern bool         configFullscreen;
extern bool         configDrawSky;
extern bool         configFiltering;
extern bool         configEnableSound;
extern bool         configEnableFog;
extern unsigned int configFrameskip;
extern unsigned int configScreenWidth;
extern unsigned int configScreenHeight;
extern unsigned int configKeyA;
extern unsigned int configKeyB;
extern unsigned int configKeyStart;
extern unsigned int configKeyR;
extern unsigned int configKeyZ;
extern unsigned int configKeyCUp;
extern unsigned int configKeyCDown;
extern unsigned int configKeyCLeft;
extern unsigned int configKeyCRight;
extern unsigned int configKeyStickUp;
extern unsigned int configKeyStickDown;
extern unsigned int configKeyStickLeft;
extern unsigned int configKeyStickRight;

void configfile_load(const char *filename);
void configfile_save(const char *filename);

#endif
