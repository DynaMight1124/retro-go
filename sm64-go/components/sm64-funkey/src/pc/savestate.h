#ifndef _SAVESTATE_H
#define _SAVESTATE_H

#define QUICKSAVE_SLOT 0xFFFF

/* Retro-Go: We don't use the 'dontsave' section as it ends up in read-only Flash. 
   Everything should be in regular DRAM/BSS. */
#define SAVESTATE_EXCLUDE

void savestate_check();

void savestate_request_save(int slot);
void savestate_request_load(int slot);

void savestate_get_name(int slot, char* namebuffer);

#endif
