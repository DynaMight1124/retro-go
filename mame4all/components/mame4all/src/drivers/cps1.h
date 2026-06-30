#ifdef __cplusplus
extern "C" {
#endif
#ifndef _CPS1_H_
#define _CPS1_H_

extern unsigned char * cps1_gfxram;     /* Video RAM */
extern unsigned char * cps1_output;     /* Output ports */
extern size_t cps1_gfxram_size;
extern size_t cps1_output_size;

extern struct QSound_interface qsound_interface;
extern struct GfxDecodeInfo cps1_gfxdecodeinfo[];

extern struct MemoryReadAddress qsound_readmem[];
extern struct MemoryWriteAddress qsound_writemem[];

READ_HANDLER( qsound_sharedram_r );
WRITE_HANDLER( qsound_sharedram_w );

READ_HANDLER( cps1_eeprom_port_r );
WRITE_HANDLER( cps1_eeprom_port_w );

READ_HANDLER( cps1_output_r );
WRITE_HANDLER( cps1_output_w );

WRITE_HANDLER( cps2_objram_bank_w );
READ_HANDLER( cps2_objram1_r );
READ_HANDLER( cps2_objram2_r );
WRITE_HANDLER( cps2_objram1_w );
WRITE_HANDLER( cps2_objram2_w );

int  cps1_vh_start(void);
void cps1_vh_stop(void);
void cps1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void cps1_eof_callback(void);

void init_cps1(void);
void init_cps2(void);

#endif
#ifdef __cplusplus
}
#endif
