#ifdef __cplusplus
extern "C" {
#endif
#ifndef F68000_H
#define F68000_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fame.h"

extern M68K_CONTEXT FAME_CONTEXT;
extern int io_cycle_counter;

//ALEK #define FAME_IO_CYCLE io_cycle_counter

//ALEK extern int FAME_IO_CYCLE;

//ALEK #define FAME_ICount FAME_IO_CYCLE

#define fame_ICount (io_cycle_counter)

#define fame_INT_NONE 0							  
#define fame_IRQ_1    1
#define fame_IRQ_2    2
#define fame_IRQ_3    3
#define fame_IRQ_4    4
#define fame_IRQ_5    5
#define fame_IRQ_6    6
#define fame_IRQ_7    7
#define fame_INT_ACK_AUTOVECTOR    -1
#define fame_STOP     0x10

void fame_reset(void *param);                      
int  fame_execute(int cycles);
void fame_set_context(void *src);
unsigned fame_get_context(void *dst);
unsigned int fame_get_pc(void);                      
void fame_exit(void);
void fame_set_pc(unsigned int val);
unsigned fame_get_sp(void);
void fame_set_sp(unsigned int val);
unsigned fame_get_reg(int regnum);
void fame_set_reg(int regnum, unsigned val);
void fame_set_nmi_line(int state);
void fame_set_irq_line(int irqline, int state);
void fame_set_irq_callback(int (*callback)(int irqline));
const char *fame_info(void *context, int regnum);
unsigned fame_dasm(char *buffer, unsigned pc);

#ifdef FAME_CHANGE_PC
void fame_change_pc(unsigned pc);
#endif

#ifdef __cplusplus
}
#endif

#endif
#ifdef __cplusplus
}
#endif
