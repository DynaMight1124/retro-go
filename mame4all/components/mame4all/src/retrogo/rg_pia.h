#ifdef __cplusplus
extern "C" {
#endif

#ifndef RG_PIA_H
#define RG_PIA_H

#include "osd_cpu.h"
#include "memory.h"
#include "retrogo/rg_psram.h"

#define MAX_PIA 8

struct pia6821 {
	mem_read_handler in_a_handler;
	mem_read_handler in_b_handler;
	mem_read_handler in_ca1_handler;
	mem_read_handler in_cb1_handler;
	mem_read_handler in_ca2_handler;
	mem_read_handler in_cb2_handler;
	mem_write_handler out_a_handler;
	mem_write_handler out_b_handler;
	mem_write_handler out_ca2_handler;
	mem_write_handler out_cb2_handler;
	void (*irq_a_handler)(int state);
	void (*irq_b_handler)(int state);

	UINT8 in_a;
	UINT8 in_ca1;
	UINT8 in_ca2;
	UINT8 out_a;
	UINT8 out_ca2;
	UINT8 ddr_a;
	UINT8 ctl_a;
	UINT8 irq_a1;
	UINT8 irq_a2;
	UINT8 irq_a_state;

	UINT8 in_b;
	UINT8 in_cb1;
	UINT8 in_cb2;
	UINT8 out_b;
	UINT8 out_cb2;
	UINT8 ddr_b;
	UINT8 ctl_b;
	UINT8 irq_b1;
	UINT8 irq_b2;
	UINT8 irq_b_state;
};

struct pia_psram_struct {
    struct pia6821 L_pia[8];
};

#define pia (rg_psram->ptr_pia->L_pia)

#endif

#ifdef __cplusplus
}
#endif
