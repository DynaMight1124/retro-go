/*
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 */

#ifndef _SPC7110DEC_H_
#define _SPC7110DEC_H_
#include "port.h"
#include <stdint.h>

uint8_t spc7110dec_read(void);
void spc7110dec_clear(uint32_t mode, uint32_t offset, uint32_t index);
void spc7110dec_reset(void);

void spc7110dec_init(void);
void spc7110dec_deinit(void);

void spc7110dec_write(uint8_t data);
uint8_t spc7110dec_dataread(void);

void spc7110dec_mode0(bool init);
void spc7110dec_mode1(bool init);
void spc7110dec_mode2(bool init);

uint8_t spc7110dec_probability(uint32_t n);
uint8_t spc7110dec_next_lps(uint32_t n);
uint8_t spc7110dec_next_mps(uint32_t n);
bool spc7110dec_toggle_invert(uint32_t n);

uint32_t spc7110dec_morton_2x8(uint32_t data);
uint32_t spc7110dec_morton_4x8(uint32_t data);

#define SPC7110_DECOMP_BUFFER_SIZE 128

typedef struct
{
   uint32_t mode;
   uint32_t offset;
   uint32_t original_mode;
   uint32_t original_offset;
   uint32_t original_index;
   uint32_t read_counter;
   uint8_t *buffer;
   uint32_t buffer_rdoffset;
   uint32_t buffer_wroffset;
   uint32_t buffer_length;

   struct ContextState
   {
      uint8_t index;
      uint8_t invert;
   } context[32];

   uint32_t morton16[2][256];
   uint32_t morton32[4][256];
} SPC7110Decomp;

EXTERN_C SPC7110Decomp *decompPtr;
#define decomp (*decompPtr)

#endif
