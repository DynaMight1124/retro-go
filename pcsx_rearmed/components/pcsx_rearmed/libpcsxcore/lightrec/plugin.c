// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2022 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "lightrec.h"
#include "../cdrom.h"
#include "../gpu.h"
#include "../gte.h"
#include "../mdec.h"
#include "../psxdma.h"
#include "../psxhw.h"
#include "../psxmem.h"
#include "../r3000a.h"
#include "../psxinterpreter.h"
#include "../psxhle.h"
#include "../psxevents.h"

#include "../frontend/main.h"

#include "mem.h"
#include "plugin.h"

#ifdef ESP_PLATFORM
#undef _
#include <rg_system.h>
#include <esp_cpu.h>
#include <esp_heap_caps.h>
#include <esp_mmu_map.h>
#endif

#define ARRAY_SIZE(x) (sizeof(x) ? sizeof(x) / sizeof((x)[0]) : 0)

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#	define LE32TOH(x)	__builtin_bswap32(x)
#	define HTOLE32(x)	__builtin_bswap32(x)
#	define LE16TOH(x)	__builtin_bswap16(x)
#	define HTOLE16(x)	__builtin_bswap16(x)
#else
#	define LE32TOH(x)	(x)
#	define HTOLE32(x)	(x)
#	define LE16TOH(x)	(x)
#	define HTOLE16(x)	(x)
#endif

#ifdef LIGHTREC_STATIC
extern psxRegisters psxRegs;
#else
psxRegisters psxRegs;
#endif
#ifndef PCSX_DUAL_DYNAREC
Rcnt rcnts[4];
#endif

void* code_buffer;
#ifdef ESP_PLATFORM
void* phy_buffer;
#endif
static struct lightrec_state *lightrec_state;
static struct lightrec_registers *lightrec_regs;
static u8 *cache_buf;
static bool lightrec_failed;
#ifdef PCSX_PORT_PROFILE
static u32 hot_loop_pc;
static u32 hot_loop_count;
#endif
static u32 recent_pc[16];
static u32 recent_pc_pos;
#ifdef PCSX_PORT_PROFILE
static bool startup_trace_dumped;
static u32 execute_call_count;
static u32 execute_block_call_count;
#endif
#if defined(ESP_PLATFORM) && defined(PCSX_NDRC_RV32_LIVE_CANARY)
extern void ndrc_rv32_live_canary_invalidate(u32 address, u32 bytes);
extern void ndrc_rv32_live_canary_invalidate_all(void);
#endif
#if defined(ESP_PLATFORM) && defined(PCSX_PORT_PROFILE)
#define GTE_PROFILE_SAMPLE_SHIFT 4
#define GTE_PROFILE_SAMPLE_COUNT (1u << GTE_PROFILE_SAMPLE_SHIFT)

static u64 gte_profile_cycles;
static u32 gte_profile_calls;
static u32 gte_profile_no_flags_calls;
static u64 gte_profile_opcode_cycles[64];
static u32 gte_profile_opcode_calls[64];
static u32 gte_profile_sample_phase;
#endif

extern u32 lightrec_hacks;

#ifdef ESP_PLATFORM
/* The Retro-Go main task polls these diagnostics from the other core. This
 * remains observable even if generated code never returns to the emulation
 * task, unlike logging immediately after lightrec_execute(). */
static volatile bool lightrec_execute_active;
static volatile u32 lightrec_execute_sequence;
static volatile u32 lightrec_execute_since_us;
static volatile u32 lightrec_execute_pc;
static volatile u32 lightrec_execute_cycle;
static volatile u32 lightrec_execute_target;
static volatile u32 lightrec_execute_ra;
static volatile u32 lightrec_execute_sp;

static void lightrec_debug_dump_code(const char *name, u32 base)
{
	for (u32 i = 0; i < 32; i += 4) {
		u32 pc = base + i * 4;
		RG_LOGE("Lightrec %s code: %08x %08x %08x %08x %08x",
			name, pc, intFakeFetch(pc), intFakeFetch(pc + 4),
			intFakeFetch(pc + 8), intFakeFetch(pc + 12));
	}
}

void lightrec_plugin_debug_poll(void)
{
	const struct lightrec_registers *regs;
	static u32 reported_sequence;
	u32 sequence, elapsed, live_pc, p;

	if (!__atomic_load_n(&lightrec_execute_active, __ATOMIC_ACQUIRE))
		return;

	sequence = lightrec_execute_sequence;
	elapsed = (u32)rg_system_timer() - lightrec_execute_since_us;
	if (elapsed < 500000 || sequence == reported_sequence)
		return;

	reported_sequence = sequence;
	live_pc = lightrec_current_pc(lightrec_state);
	regs = lightrec_get_registers(lightrec_state);
	RG_LOGE("Lightrec execute stalled: seq=%u elapsed=%u us entry=%08x "
		"cycle=%u target=%u live=%08x/%u ra=%08x sp=%08x",
		sequence, elapsed, lightrec_execute_pc,
		lightrec_execute_cycle, lightrec_execute_target,
		live_pc,
		lightrec_current_cycle_count(lightrec_state),
		lightrec_execute_ra, lightrec_execute_sp);

	/* Read the stalled code once here instead of fetching four opcodes before
	 * every Lightrec call. The latter significantly perturbs this very hot
	 * path and obscures the performance of the branch workaround. */
	lightrec_debug_dump_code("entry", lightrec_execute_pc);
	if (live_pc != lightrec_execute_pc)
		lightrec_debug_dump_code("live", live_pc);
	RG_LOGE("Lightrec live regs: v0=%08x v1=%08x a0=%08x a1=%08x "
		"t0=%08x t1=%08x t2=%08x s0=%08x s1=%08x gp=%08x "
		"sp=%08x ra=%08x lo=%08x hi=%08x",
		regs->gpr[2], regs->gpr[3], regs->gpr[4], regs->gpr[5],
		regs->gpr[8], regs->gpr[9], regs->gpr[10], regs->gpr[16],
		regs->gpr[17], regs->gpr[28], regs->gpr[29], regs->gpr[31],
		regs->gpr[32], regs->gpr[33]);

	/* Also snapshot the calls which successfully returned immediately before
	 * the stall. This distinguishes a bad compiled block from a dispatcher or
	 * recursive-HLE failure without per-block serial logging. */
	p = recent_pc_pos;
	for (u32 i = 0; i < 16; i += 4) {
		u32 p0 = recent_pc[(p + i + 0) & 15];
		u32 p1 = recent_pc[(p + i + 1) & 15];
		u32 p2 = recent_pc[(p + i + 2) & 15];
		u32 p3 = recent_pc[(p + i + 3) & 15];
		RG_LOGE("Lightrec pre-stall: %08x:%08x %08x:%08x "
			"%08x:%08x %08x:%08x",
			p0, intFakeFetch(p0), p1, intFakeFetch(p1),
			p2, intFakeFetch(p2), p3, intFakeFetch(p3));
	}
}
#endif

static void sync_to_lightrec(struct lightrec_registers *regs);
static void sync_from_lightrec(const struct lightrec_registers *regs);

#define DECLARE_GTE_NO_FLAGS(name) \
	extern void name##_nf(struct psxCP2Regs *regs)
DECLARE_GTE_NO_FLAGS(gteRTPS);
DECLARE_GTE_NO_FLAGS(gteNCLIP);
DECLARE_GTE_NO_FLAGS(gteOP);
DECLARE_GTE_NO_FLAGS(gteDPCS);
DECLARE_GTE_NO_FLAGS(gteINTPL);
#if defined(CONFIG_IDF_TARGET_ESP32P4)
DECLARE_GTE_NO_FLAGS(gteINTPL_sf0_lm0);
DECLARE_GTE_NO_FLAGS(gteINTPL_sf0_lm1);
DECLARE_GTE_NO_FLAGS(gteINTPL_sf1_lm0);
DECLARE_GTE_NO_FLAGS(gteINTPL_sf1_lm1);
#endif
DECLARE_GTE_NO_FLAGS(gteMVMVA);
DECLARE_GTE_NO_FLAGS(gteNCDS);
DECLARE_GTE_NO_FLAGS(gteCDP);
DECLARE_GTE_NO_FLAGS(gteNCDT);
DECLARE_GTE_NO_FLAGS(gteNCCS);
DECLARE_GTE_NO_FLAGS(gteCC);
DECLARE_GTE_NO_FLAGS(gteNCS);
DECLARE_GTE_NO_FLAGS(gteNCT);
DECLARE_GTE_NO_FLAGS(gteSQR);
DECLARE_GTE_NO_FLAGS(gteDCPL);
DECLARE_GTE_NO_FLAGS(gteDPCT);
DECLARE_GTE_NO_FLAGS(gteAVSZ3);
DECLARE_GTE_NO_FLAGS(gteAVSZ4);
DECLARE_GTE_NO_FLAGS(gteRTPT);
DECLARE_GTE_NO_FLAGS(gteGPF);
DECLARE_GTE_NO_FLAGS(gteGPL);
DECLARE_GTE_NO_FLAGS(gteNCCT);
#undef DECLARE_GTE_NO_FLAGS

typedef void (*gte_handler_t)(struct psxCP2Regs *regs);

static const gte_handler_t psxCP2_no_flags[64] = {
	[0x01] = gteRTPS_nf,
	[0x06] = gteNCLIP_nf,
	[0x0c] = gteOP_nf,
	[0x10] = gteDPCS_nf,
	[0x11] = gteINTPL_nf,
	[0x12] = gteMVMVA_nf,
	[0x13] = gteNCDS_nf,
	[0x14] = gteCDP_nf,
	[0x16] = gteNCDT_nf,
	[0x1b] = gteNCCS_nf,
	[0x1c] = gteCC_nf,
	[0x1e] = gteNCS_nf,
	[0x20] = gteNCT_nf,
	[0x28] = gteSQR_nf,
	[0x29] = gteDCPL_nf,
	[0x2a] = gteDPCT_nf,
	[0x2d] = gteAVSZ3_nf,
	[0x2e] = gteAVSZ4_nf,
	[0x30] = gteRTPT_nf,
	[0x3d] = gteGPF_nf,
	[0x3e] = gteGPL_nf,
	[0x3f] = gteNCCT_nf,
};

#if defined(CONFIG_IDF_TARGET_ESP32P4)
/* Index bit 1 is SF and bit 0 is LM. */
static const gte_handler_t intpl_handlers[4] = {
	gteINTPL_sf0_lm0,
	gteINTPL_sf0_lm1,
	gteINTPL_sf1_lm0,
	gteINTPL_sf1_lm1,
};

static const gte_handler_t intpl_handlers_no_flags[4] = {
	gteINTPL_sf0_lm0_nf,
	gteINTPL_sf0_lm1_nf,
	gteINTPL_sf1_lm0_nf,
	gteINTPL_sf1_lm1_nf,
};
#endif

static void cop2_op_common(struct lightrec_state *state, u32 opcode,
			   bool no_flags)
{
	/* This PCSX plugin owns exactly one Lightrec instance.  Cache its register
	 * file at initialization instead of making an accessor call for every GTE
	 * command (several thousand times per rendered frame). */
	struct lightrec_registers *regs = lightrec_regs;
	u32 function = opcode & 0x3f;
	gte_handler_t handler;

#if defined(CONFIG_IDF_TARGET_ESP32P4)
	if (function == 0x11) {
		u32 variant = ((opcode >> 18) & 2) | ((opcode >> 10) & 1);

		handler = no_flags ? intpl_handlers_no_flags[variant] :
			intpl_handlers[variant];
	} else
#endif
	{
		handler = no_flags ? psxCP2_no_flags[function] : NULL;
		if (!handler)
			handler = psxCP2[function];
	}
	(void)state;
#if defined(ESP_PLATFORM) && defined(PCSX_PORT_PROFILE)
	/* Profiling every GTE command became measurable once the hot handlers
	 * were optimized: Ridge Racer issues roughly 6,000 commands per frame.
	 * Sample one in 16 calls and scale the window totals instead.  Keeping the
	 * phase across reporting windows also avoids repeatedly sampling the same
	 * point at each 60-frame boundary. */
	u32 sample_phase = ++gte_profile_sample_phase;
	bool profile_this_call =
		(sample_phase & (GTE_PROFILE_SAMPLE_COUNT - 1)) == 0;
	u32 start_cycles = profile_this_call ? esp_cpu_get_cycle_count() : 0;
#endif

	/* The shared GTE handlers take their register file as an argument, but
	 * decode SF/MX/V/CV/LM and other command fields from psxRegs.code. The
	 * interpreter sets it during instruction fetch; Lightrec must do so here
	 * before dispatching the COP2 operation. cp2c immediately follows cp2d,
	 * matching psxCP2Regs. */
	psxRegs.code = opcode;
	handler((psxCP2Regs *)regs->cp2d);
#if defined(ESP_PLATFORM) && defined(PCSX_PORT_PROFILE)
	if (profile_this_call) {
		u32 elapsed_cycles =
			(u32)(esp_cpu_get_cycle_count() - start_cycles);

		gte_profile_cycles += elapsed_cycles;
		gte_profile_calls++;
		gte_profile_no_flags_calls += no_flags;
		gte_profile_opcode_cycles[function] += elapsed_cycles;
		gte_profile_opcode_calls[function]++;
	}
#endif
}

static void cop2_op(struct lightrec_state *state, u32 opcode)
{
	cop2_op_common(state, opcode, false);
}

static void cop2_op_no_flags(struct lightrec_state *state, u32 opcode)
{
	cop2_op_common(state, opcode, true);
}

#if defined(ESP_PLATFORM) && defined(PCSX_PORT_PROFILE)
void lightrec_plugin_get_profile(u64 *gte_cycles, u32 *gte_calls,
		u32 *gte_no_flags_calls,
		u64 opcode_cycles[64], u32 opcode_calls[64])
{
	*gte_cycles = gte_profile_cycles << GTE_PROFILE_SAMPLE_SHIFT;
	*gte_calls = gte_profile_calls << GTE_PROFILE_SAMPLE_SHIFT;
	*gte_no_flags_calls =
		gte_profile_no_flags_calls << GTE_PROFILE_SAMPLE_SHIFT;
	for (u32 i = 0; i < ARRAY_SIZE(gte_profile_opcode_cycles); i++) {
		opcode_cycles[i] =
			gte_profile_opcode_cycles[i] << GTE_PROFILE_SAMPLE_SHIFT;
		opcode_calls[i] =
			gte_profile_opcode_calls[i] << GTE_PROFILE_SAMPLE_SHIFT;
	}
	gte_profile_cycles = 0;
	gte_profile_calls = 0;
	gte_profile_no_flags_calls = 0;
	memset(gte_profile_opcode_cycles, 0, sizeof(gte_profile_opcode_cycles));
	memset(gte_profile_opcode_calls, 0, sizeof(gte_profile_opcode_calls));
}
#endif

static void lightrec_enable_ram(struct lightrec_state *state, bool enable)
{
	if (enable)
		memcpy(psxRegs.ptrs.psxM, cache_buf, 64 * 1024);
	else
		memcpy(cache_buf, psxRegs.ptrs.psxM, 64 * 1024);

}

static void lightrec_transition_to_pcsx(struct lightrec_state *state)
{
	psxRegs.cycle = lightrec_current_cycle_count(state);
}

static void lightrec_transition_from_pcsx(struct lightrec_state *state)
{
	lightrec_reset_cycle_count(state, psxRegs.cycle);
}

static void hw_write_byte(struct lightrec_state *state, u32 opcode,
		void *host, u32 addr, u32 data)
{
	lightrec_transition_to_pcsx(state);
	psxHwWrite8(addr, data);
	lightrec_transition_from_pcsx(state);
}

static void hw_write_half(struct lightrec_state *state, u32 opcode,
		void *host, u32 addr, u32 data)
{
	lightrec_transition_to_pcsx(state);
	psxHwWrite16(addr, data);
	lightrec_transition_from_pcsx(state);
}

static void hw_write_word(struct lightrec_state *state, u32 opcode,
		void *host, u32 addr, u32 data)
{
	lightrec_transition_to_pcsx(state);
	psxHwWrite32(addr, data);
	lightrec_transition_from_pcsx(state);
}

static u8 hw_read_byte(struct lightrec_state *state, u32 opcode,
		void *host, u32 addr)
{
	u8 data;

	lightrec_transition_to_pcsx(state);
	data = psxHwRead8(addr);
	lightrec_transition_from_pcsx(state);
	return data;
}

static u16 hw_read_half(struct lightrec_state *state, u32 opcode,
		void *host, u32 addr)
{
	u16 data;

	lightrec_transition_to_pcsx(state);
	data = psxHwRead16(addr);
	lightrec_transition_from_pcsx(state);
	return data;
}

static u32 hw_read_word(struct lightrec_state *state, u32 opcode,
		void *host, u32 addr)
{
	u32 data;

	lightrec_transition_to_pcsx(state);
	data = psxHwRead32(addr);
	lightrec_transition_from_pcsx(state);
	return data;
}

static const struct lightrec_mem_map_ops hw_regs_ops = {
	.sb = hw_write_byte,
	.sh = hw_write_half,
	.sw = hw_write_word,
	.lb = hw_read_byte,
	.lh = hw_read_half,
	.lw = hw_read_word,
};

static u32 cache_ctrl;

static void cache_ctrl_write_word(struct lightrec_state *state, u32 opcode,
		void *host, u32 addr, u32 data)
{
	cache_ctrl = data;
}

static u32 cache_ctrl_read_word(struct lightrec_state *state, u32 opcode,
		void *host, u32 addr)
{
	return cache_ctrl;
}

static const struct lightrec_mem_map_ops cache_ctrl_ops = {
	.sw = cache_ctrl_write_word,
	.lw = cache_ctrl_read_word,
};

static bool lightrec_can_hw_direct(u32 kaddr, bool is_write, u8 size)
{
	if (is_write && size != 32) {
		/* These ranges implement force-32 and must use the handlers. */
		if (0x1f801000 <= kaddr && kaddr < 0x1f801024)
			return false;
		if ((kaddr & 0x1fffff80) == 0x1f801080)
			return false;
	}

	switch (size) {
	case 8:
		switch (kaddr) {
		case 0x1f801040:
		case 0x1f801050:
		case 0x1f801800:
		case 0x1f801801:
		case 0x1f801802:
		case 0x1f801803:
			return false;
		default:
			return true;
		}
	case 16:
		switch (kaddr) {
		case 0x1f801040:
		case 0x1f801044:
		case 0x1f801048:
		case 0x1f80104a:
		case 0x1f80104e:
		case 0x1f801050:
		case 0x1f801054:
		case 0x1f80105a:
		case 0x1f80105e:
		case 0x1f801100:
		case 0x1f801104:
		case 0x1f801108:
		case 0x1f801110:
		case 0x1f801114:
		case 0x1f801118:
		case 0x1f801120:
		case 0x1f801124:
		case 0x1f801128:
			return false;
		case 0x1f801070:
		case 0x1f801074:
			return !is_write;
		default:
			return kaddr < 0x1f801c00 || kaddr >= 0x1f801e00;
		}
	default:
		switch (kaddr) {
		case 0x1f801040:
		case 0x1f801050:
		case 0x1f801100:
		case 0x1f801104:
		case 0x1f801108:
		case 0x1f801110:
		case 0x1f801114:
		case 0x1f801118:
		case 0x1f801120:
		case 0x1f801124:
		case 0x1f801128:
		case 0x1f801810:
		case 0x1f801814:
		case 0x1f801820:
		case 0x1f801824:
			return false;
		case 0x1f801070:
		case 0x1f801074:
		case 0x1f801088:
		case 0x1f801098:
		case 0x1f8010a8:
		case 0x1f8010b8:
		case 0x1f8010c8:
		case 0x1f8010e8:
		case 0x1f8010f4:
			return !is_write;
		default:
			return !is_write || kaddr < 0x1f801c00 || kaddr >= 0x1f801e00;
		}
	}
}

static const struct lightrec_ops lightrec_ops = {
	.cop2_op = cop2_op,
	.cop2_op_no_flags = cop2_op_no_flags,
	.enable_ram = lightrec_enable_ram,
	.hw_direct = lightrec_can_hw_direct,
};

static struct lightrec_mem_map lightrec_map[PSX_MAP_UNKNOWN] = {
	[PSX_MAP_KERNEL_USER_RAM] = { .length = 0x200000, .pc = 0x00000000 },
	[PSX_MAP_BIOS] = { .length = 0x80000, .pc = 0x1fc00000 },
	[PSX_MAP_SCRATCH_PAD] = { .length = 0x400, .pc = 0x1f800000 },
	[PSX_MAP_PARALLEL_PORT] = { .length = 0x10000, .pc = 0x1f000000 },
	[PSX_MAP_HW_REGISTERS] = {
		.length = 0x8000, .pc = 0x1f801000, .ops = &hw_regs_ops,
	},
	[PSX_MAP_CACHE_CONTROL] = {
		.length = 4, .pc = 0x5ffe0130, .ops = &cache_ctrl_ops,
	},
	[PSX_MAP_MIRROR1] = {
		.length = 0x200000, .pc = 0x00200000,
		.mirror_of = &lightrec_map[PSX_MAP_KERNEL_USER_RAM],
	},
	[PSX_MAP_MIRROR2] = {
		.length = 0x200000, .pc = 0x00400000,
		.mirror_of = &lightrec_map[PSX_MAP_KERNEL_USER_RAM],
	},
	[PSX_MAP_MIRROR3] = {
		.length = 0x200000, .pc = 0x00600000,
		.mirror_of = &lightrec_map[PSX_MAP_KERNEL_USER_RAM],
	},
	[PSX_MAP_CODE_BUFFER] = { .length = CODE_BUFFER_SIZE, .pc = 0 },
	[PSX_MAP_PPORT_MIRROR] = {
		.length = 0x10000, .pc = 0x1fa00000,
		.mirror_of = &lightrec_map[PSX_MAP_PARALLEL_PORT],
	},
};

static void lightrec_plugin_apply_config();

#if defined(ESP_PLATFORM) && defined(CONFIG_IDF_TARGET_ESP32P4)
extern void jit_flush(void *start, void *end);

static int lightrec_jit_smoke_test(void *buffer)
{
	volatile u32 *code = (volatile u32 *)buffer;
	int (*test_fn)(void) = (int (*)(void))buffer;

	/* addi a0, zero, 42; jalr zero, 0(ra) */
	code[0] = 0x02a00513;
	code[1] = 0x00008067;
	jit_flush(buffer, (u8 *)buffer + 2 * sizeof(*code));

	return test_fn();
}
#endif

static int lightrec_plugin_init(void)
{
	lightrec_map[PSX_MAP_KERNEL_USER_RAM].address = psxRegs.ptrs.psxM;
	lightrec_map[PSX_MAP_BIOS].address = psxRegs.ptrs.psxR;
	lightrec_map[PSX_MAP_SCRATCH_PAD].address = psxRegs.ptrs.psxH;
	lightrec_map[PSX_MAP_HW_REGISTERS].address = psxRegs.ptrs.psxH + 0x1000;
	lightrec_map[PSX_MAP_PARALLEL_PORT].address = psxRegs.ptrs.psxP;
	lightrec_map[PSX_MAP_CACHE_CONTROL].address = &cache_ctrl;

#ifdef ESP_PLATFORM
    cache_buf = heap_caps_malloc(64 * 1024, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    cache_buf = malloc(64 * 1024);
#endif
    if (!cache_buf) {
        RG_LOGE("Lightrec cache-isolation buffer allocation failed");
        return -ENOMEM;
    }

#ifdef ESP_PLATFORM
    /* Diagnostic: Verify JIT configuration */
    RG_LOGI("JIT Build Config: __WORDSIZE=%d, __riscv_xlen=%d, sizeof(void*)=%d", 
            (int)__WORDSIZE, (int)__riscv_xlen, (int)sizeof(void*));

    const size_t page_size = CONFIG_MMU_PAGE_SIZE;
    const size_t size = (CODE_BUFFER_SIZE + page_size - 1) & ~(page_size - 1);
    
    // Allocate page-aligned physical memory in SPIRAM.
    phy_buffer = heap_caps_aligned_alloc(page_size, size,
                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!phy_buffer) {
        RG_LOGE("JIT physical buffer allocation failed!");
		free(cache_buf);
		cache_buf = NULL;
        return -ENOMEM;
    }

    esp_paddr_t paddr = 0;
    mmu_target_t target;
    esp_err_t err = esp_mmu_vaddr_to_paddr(phy_buffer, &paddr, &target);
    if (err != ESP_OK) {
        RG_LOGE("esp_mmu_vaddr_to_paddr failed: %d", err);
        free(phy_buffer);
        phy_buffer = NULL;
		free(cache_buf);
		cache_buf = NULL;
        return -1;
    }

    void *vaddr_exec = NULL;
    /* P4 PSRAM executable mappings are also accessible through DBUS. Keep one
     * address for allocation, emission and execution to avoid translating
     * every generated code pointer between aliases. */
    err = esp_mmu_map(paddr, size, target,
                      MMU_MEM_CAP_EXEC,
                      ESP_MMU_MMAP_FLAG_PADDR_SHARED, &vaddr_exec);
    if (err != ESP_OK) {
        RG_LOGE("esp_mmu_map failed: %d", err);
        free(phy_buffer);
        phy_buffer = NULL;
		free(cache_buf);
		cache_buf = NULL;
        return -1;
    }

    code_buffer = vaddr_exec;
    memset(code_buffer, 0, size);
    RG_LOGI("JIT Buffer: Base=%p, backing=%p, Size=%d, Type=MMU-Mapped-SPIRAM",
            code_buffer, phy_buffer, (int)size);

#if defined(CONFIG_IDF_TARGET_ESP32P4)
    int smoke_result = lightrec_jit_smoke_test(code_buffer);
    if (smoke_result != 42) {
        RG_LOGE("JIT executable-memory smoke test failed: result=%d", smoke_result);
        esp_mmu_unmap(code_buffer);
        code_buffer = NULL;
        free(phy_buffer);
        phy_buffer = NULL;
		free(cache_buf);
		cache_buf = NULL;
        return -EIO;
    }
    RG_LOGI("JIT executable-memory smoke test passed");
#ifdef PCSX_NDRC_RV32_SELFTEST
    /* Reuse the empty executable arena: no second PSRAM allocation, no guest
     * state changes, and no change to the selected CPU backend. */
    extern void ndrc_rv32_p4_diagnostic(void *code, size_t size);
    ndrc_rv32_p4_diagnostic(code_buffer, size);
#endif
#endif
    
    lightrec_map[PSX_MAP_CODE_BUFFER].address = code_buffer;
    lightrec_map[PSX_MAP_CODE_BUFFER].length = size;
#else
	if (!LIGHTREC_CUSTOM_MAP) {
		code_buffer = malloc(CODE_BUFFER_SIZE_DFT);
		if (!code_buffer)
		{
			free(cache_buf);
			cache_buf = NULL;
			return -ENOMEM;
		}
	}
	lightrec_map[PSX_MAP_CODE_BUFFER].address = code_buffer;
#endif

	lightrec_state = lightrec_init(NULL, lightrec_map, ARRAY_SIZE(lightrec_map), &lightrec_ops);

	if (!lightrec_state) {
        RG_LOGE("lightrec_init failed!");
#ifdef ESP_PLATFORM
        esp_mmu_unmap(code_buffer);
        code_buffer = NULL;
		free(phy_buffer);
		phy_buffer = NULL;
#else
		if (!LIGHTREC_CUSTOM_MAP)
			free(code_buffer);
#endif
		free(cache_buf);
		cache_buf = NULL;
		return -1;
	}
	lightrec_regs = (struct lightrec_registers *)
		lightrec_get_registers(lightrec_state);

	lightrec_failed = false;
#ifdef PCSX_PORT_PROFILE
	hot_loop_pc = 0;
	hot_loop_count = 0;
#endif
	recent_pc_pos = 0;
#ifdef PCSX_PORT_PROFILE
	startup_trace_dumped = false;
	execute_call_count = 0;
	execute_block_call_count = 0;
#endif
#ifdef ESP_PLATFORM
	lightrec_execute_active = false;
	lightrec_execute_sequence = 0;
#endif
	cache_ctrl = 0;
	lightrec_plugin_apply_config();
	return 0;
}

static void lightrec_plugin_shutdown(void)
{

#if defined(ESP_PLATFORM) && defined(PCSX_NDRC_RV32_LIVE_CANARY)
	extern void ndrc_rv32_live_canary_shutdown(void);
	ndrc_rv32_live_canary_shutdown();
#endif
	lightrec_destroy(lightrec_state);
	lightrec_state = NULL;
	lightrec_regs = NULL;
#ifdef ESP_PLATFORM
    if (code_buffer) {
        esp_mmu_unmap(code_buffer);
        code_buffer = NULL;
    }
    if (phy_buffer) {
        free(phy_buffer);
        phy_buffer = NULL;
    }
#else
	if (!LIGHTREC_CUSTOM_MAP)
		free(code_buffer);
#endif
	free(cache_buf);
	cache_buf = NULL;
}

static void sync_to_lightrec(struct lightrec_registers *regs)
{
	memcpy(regs->gpr, psxRegs.GPR.r, 32 * sizeof(u32));
	regs->gpr[32] = psxRegs.GPR.n.lo;
	regs->gpr[33] = psxRegs.GPR.n.hi;
	memcpy(regs->cp0, psxRegs.CP0.r, 32 * sizeof(u32));
	memcpy(regs->cp2d, psxRegs.CP2D.r, 32 * sizeof(u32));
	memcpy(regs->cp2c, psxRegs.CP2C.r, 32 * sizeof(u32));
}

static void sync_from_lightrec(const struct lightrec_registers *regs)
{
	memcpy(psxRegs.GPR.r, regs->gpr, 32 * sizeof(u32));
	psxRegs.GPR.n.lo = regs->gpr[32];
	psxRegs.GPR.n.hi = regs->gpr[33];
	memcpy(psxRegs.CP0.r, regs->cp0, 32 * sizeof(u32));
	memcpy(psxRegs.CP2D.r, regs->cp2d, 32 * sizeof(u32));
	memcpy(psxRegs.CP2C.r, regs->cp2c, 32 * sizeof(u32));
}

static void lightrec_plugin_reset(void)
{
	struct lightrec_registers *regs = (struct lightrec_registers *)lightrec_get_registers(lightrec_state);

	lightrec_invalidate_all(lightrec_state);
#if defined(ESP_PLATFORM) && defined(PCSX_NDRC_RV32_LIVE_CANARY)
	ndrc_rv32_live_canary_invalidate_all();
#endif
	sync_to_lightrec(regs);
	lightrec_reset_cycle_count(lightrec_state, psxRegs.cycle);
	lightrec_failed = false;
#ifdef PCSX_PORT_PROFILE
	hot_loop_pc = 0;
	hot_loop_count = 0;
#endif
	recent_pc_pos = 0;
#ifdef PCSX_PORT_PROFILE
	startup_trace_dumped = false;
	execute_call_count = 0;
	execute_block_call_count = 0;
#endif
	cache_ctrl = 0;
}

static bool lightrec_handle_exit(u32 flags)
{
	if (unlikely(flags & (LIGHTREC_EXIT_SEGFAULT | LIGHTREC_EXIT_NOMEM))) {
		RG_LOGE("Lightrec fatal exit: PC=%08x cycle=%u flags=%02x",
			psxRegs.pc, psxRegs.cycle, flags);
		lightrec_failed = true;
		psxRegs.stop = 1;
		return false;
	}

	if (flags & LIGHTREC_EXIT_SYSCALL)
		psxException(R3000E_Syscall << 2, R3000A_BRANCH_NONE_OR_EXCEPTION,
			&psxRegs.CP0);
	if (flags & LIGHTREC_EXIT_BREAK)
		psxException(R3000E_Bp << 2, R3000A_BRANCH_NONE_OR_EXCEPTION,
			&psxRegs.CP0);
	else if (flags & LIGHTREC_EXIT_UNKNOWN_OP) {
		u32 opcode = intFakeFetch(psxRegs.pc);
		u32 hle_code = opcode & 0x03ffffff;

		if ((opcode >> 26) == 0x3b && hle_code < hleop_count_ && Config.HLE) {
			psxRegs.pc += 4;
			psxHLEt[hle_code]();
		} else {
			RG_LOGE("Lightrec unknown opcode %08x at PC=%08x",
				opcode, psxRegs.pc);
			psxException(R3000E_RI << 2, R3000A_BRANCH_NONE_OR_EXCEPTION,
				&psxRegs.CP0);
		}
	}

	return true;
}

bool lightrec_plugin_failed(void)
{
	return lightrec_failed;
}

static bool lightrec_plugin_execute_internal(bool single_block)
{
	struct lightrec_registers *regs =
		(struct lightrec_registers *)lightrec_get_registers(lightrec_state);
	u32 old_pc, old_cycle, new_cycle, target_cycle, flags, pre_branch_pc;
	bool rv32_native_batch_advanced = false;

	if (unlikely(lightrec_failed)) {
		psxRegs.stop = 1;
		return false;
	}

	/* psxReset() clears next_interupt and set_event() only shortens an
	 * existing deadline. Establish the first deadline, then service and
	 * reschedule it whenever a previous slice has expired. Without this,
	 * Lightrec can execute guest code forever while timers, CD-ROM and GPU
	 * events never become visible to the emulated CPU. */
	if (unlikely((s32)(psxRegs.cycle - psxRegs.next_interupt) >= 0))
		gen_interupt(&psxRegs.CP0);

#if defined(ESP_PLATFORM) && defined(PCSX_NDRC_RV32_LIVE_CANARY)
	/* A bounded RV32 core loop may advance canonical PCSX state through several
	 * cached blocks here. Lightrec is synchronized below only after that loop
	 * stops, so no completed block is run a second time. */
	extern int ndrc_rv32_live_canary_try(void *regs);
	extern int ndrc_rv32_live_canary_native_batch_advanced(void);
	(void)ndrc_rv32_live_canary_try(&psxRegs);
	rv32_native_batch_advanced =
		ndrc_rv32_live_canary_native_batch_advanced() != 0;
	/* An interpreter fallback may have reached the frontend's frame/event
	 * stop condition. Preserve canonical state in Lightrec, but do not execute
	 * an extra guest block after that stop. */
	if (unlikely(psxRegs.stop)) {
		sync_to_lightrec(regs);
		return false;
	}
	/* A productive general batch already advanced canonical guest state. Keep
	 * Lightrec's register file coherent for recursive HLE notifications, then
	 * return directly to the CPU outer loop instead of executing a redundant
	 * Lightrec bridge block. A no-progress batch falls through so Lightrec can
	 * own the pending event boundary. */
	if (rv32_native_batch_advanced) {
		sync_to_lightrec(regs);
		return true;
	}
#endif

	old_pc = psxRegs.pc;
	old_cycle = psxRegs.cycle;
	recent_pc[recent_pc_pos++ & 15] = old_pc;
	sync_to_lightrec(regs);
	lightrec_reset_cycle_count(lightrec_state, old_cycle);

	/* Normal execution must stay in Lightrec until the next PCSX event. A
	 * two-cycle target makes the generated dispatcher return after every guest
	 * block, paying full C dispatch and register synchronization each time and
	 * erasing most of the dynarec's benefit. ExecuteBlock() is different: BIOS
	 * boot and HLE softcalls depend on it returning after one branch block, so
	 * retain the minimal positive budget for that interface. */
	target_cycle = single_block ? old_cycle + 2 : psxRegs.next_interupt;
	if (unlikely(target_cycle == old_cycle))
		target_cycle = old_cycle + 2;

#ifdef PCSX_PORT_PROFILE
	if (single_block)
		execute_block_call_count++;
	else
		execute_call_count++;
#endif

#ifdef ESP_PLATFORM
	lightrec_execute_pc = old_pc;
	lightrec_execute_cycle = old_cycle;
	lightrec_execute_target = target_cycle;
	lightrec_execute_ra = psxRegs.GPR.n.ra;
	lightrec_execute_sp = psxRegs.GPR.n.sp;
	lightrec_execute_since_us = (u32)rg_system_timer();
	lightrec_execute_sequence++;
	__atomic_store_n(&lightrec_execute_active, true, __ATOMIC_RELEASE);
#endif
	psxRegs.pc = lightrec_execute(lightrec_state, old_pc, target_cycle);
#ifdef ESP_PLATFORM
	__atomic_store_n(&lightrec_execute_active, false, __ATOMIC_RELEASE);
#endif
	sync_from_lightrec(regs);
	new_cycle = lightrec_current_cycle_count(lightrec_state);
	psxRegs.cycle = new_cycle;
	flags = lightrec_exit_flags(lightrec_state);

	if (!lightrec_handle_exit(flags))
		return false;

	if (unlikely(new_cycle == old_cycle && flags == LIGHTREC_EXIT_NORMAL)) {
		RG_LOGE("Lightrec made no progress: PC=%08x opcode=%08x cycle=%u "
			"ra=%08x sp=%08x",
			old_pc, intFakeFetch(old_pc), old_cycle,
			psxRegs.GPR.n.ra, psxRegs.GPR.n.sp);
		lightrec_failed = true;
		psxRegs.stop = 1;
		return false;
	}

#ifdef PCSX_PORT_PROFILE
	if (unlikely(psxRegs.pc == old_pc)) {
		if (hot_loop_pc != old_pc) {
			hot_loop_pc = old_pc;
			hot_loop_count = 1;
		} else if (++hot_loop_count == 4096) {
			RG_LOGW("Lightrec hot loop: PC=%08x ops=%08x/%08x/%08x/%08x "
				"v0=%08x a0=%08x t0=%08x t1=%08x ra=%08x "
				"SR=%08x Cause=%08x I=%08x/%08x next=%u",
				old_pc, intFakeFetch(old_pc), intFakeFetch(old_pc + 4),
				intFakeFetch(old_pc + 8), intFakeFetch(old_pc + 12),
				psxRegs.GPR.n.v0, psxRegs.GPR.n.a0,
				psxRegs.GPR.n.t0, psxRegs.GPR.n.t1,
				psxRegs.GPR.n.ra, psxRegs.CP0.n.SR,
				psxRegs.CP0.n.Cause, psxHu32(0x1070),
				psxHu32(0x1074), psxRegs.next_interupt);
		}
	} else {
		hot_loop_pc = psxRegs.pc;
		hot_loop_count = 0;
	}

	/* Preserve a small rolling trace instead of flooding the serial port.
	 * If boot has produced no meaningful code expansion after roughly one
	 * emulated second, this identifies the exact guest loop and instructions. */
	if (unlikely(!startup_trace_dumped && new_cycle >= 60 * 564480u)) {
		u32 p = recent_pc_pos;
		startup_trace_dumped = true;
		RG_LOGI("Lightrec batching: execute=%u block=%u at cycle=%u",
			execute_call_count, execute_block_call_count, new_cycle);
		for (u32 i = 0; i < 16; i += 4) {
			u32 p0 = recent_pc[(p + i + 0) & 15];
			u32 p1 = recent_pc[(p + i + 1) & 15];
			u32 p2 = recent_pc[(p + i + 2) & 15];
			u32 p3 = recent_pc[(p + i + 3) & 15];
			RG_LOGW("Lightrec trace: %08x:%08x %08x:%08x "
				"%08x:%08x %08x:%08x",
				p0, intFakeFetch(p0), p1, intFakeFetch(p1),
				p2, intFakeFetch(p2), p3, intFakeFetch(p3));
		}
		RG_LOGW("Lightrec regs: v0=%08x v1=%08x a0=%08x a1=%08x "
			"t0=%08x t1=%08x s0=%08x s1=%08x gp=%08x sp=%08x ra=%08x",
			psxRegs.GPR.n.v0, psxRegs.GPR.n.v1,
			psxRegs.GPR.n.a0, psxRegs.GPR.n.a1,
			psxRegs.GPR.n.t0, psxRegs.GPR.n.t1,
			psxRegs.GPR.n.s0, psxRegs.GPR.n.s1,
			psxRegs.GPR.n.gp, psxRegs.GPR.n.sp,
			psxRegs.GPR.n.ra);
	}
#endif

	pre_branch_pc = psxRegs.pc;
	psxBranchTest();

	/* HLE handlers update psxRegs directly. In particular, softCall() and
	 * softCallInException() call ExecuteBlock() and then issue BEFORE_SAVE,
	 * which copies Lightrec's register file back to psxRegs. Keep Lightrec
	 * coherent before returning from such a recursive ExecuteBlock or the
	 * HLE return value (and exception CP0 state) is replaced with stale data.
	 * psxBranchTest() can also take an interrupt after an otherwise normal
	 * exit, so cover a PC change made there as well. */
	if (unlikely(flags != LIGHTREC_EXIT_NORMAL ||
		psxRegs.pc != pre_branch_pc))
		sync_to_lightrec(regs);
	return true;
}

static void lightrec_plugin_execute(psxRegisters *regs_)
{
	(void)regs_;
	while (!psxRegs.stop && lightrec_plugin_execute_internal(false))
		;
}

static void lightrec_plugin_execute_block(psxRegisters *regs_, enum blockExecCaller caller)
{
	(void)regs_;
	(void)caller;
	lightrec_plugin_execute_internal(true);
}

static void lightrec_plugin_clear(u32 addr, u32 size)
{
	if (addr == 0 && size == UINT32_MAX)
		lightrec_invalidate_all(lightrec_state);
	else
		/* PCSX supplies this size in DMA words. */
		lightrec_invalidate(lightrec_state, addr, size * 4);
#if defined(ESP_PLATFORM) && defined(PCSX_NDRC_RV32_LIVE_CANARY)
	if (addr == 0 && size == UINT32_MAX)
		ndrc_rv32_live_canary_invalidate_all();
	else
		ndrc_rv32_live_canary_invalidate(addr, size * 4);
#endif
}

static void lightrec_plugin_notify(enum R3000Anote note, void *data)
{
	struct lightrec_registers *regs = (struct lightrec_registers *)lightrec_get_registers(lightrec_state);
	if (!regs)
		return;

	switch (note) {
	case R3000ACPU_NOTIFY_AFTER_LOAD:
	case R3000ACPU_NOTIFY_AFTER_LOAD_STATE:
		sync_to_lightrec(regs);
#if defined(ESP_PLATFORM) && defined(PCSX_NDRC_RV32_LIVE_CANARY)
		ndrc_rv32_live_canary_invalidate_all();
#endif
		if (data == NULL)
			lightrec_invalidate_all(lightrec_state);
		break;
	case R3000ACPU_NOTIFY_BEFORE_SAVE:
		sync_from_lightrec(regs);
		break;
	default:
		break;
	}
}

static void lightrec_plugin_apply_config()
{
	u32 opt_flags = lightrec_hacks;

	lightrec_set_cycles_per_opcode(lightrec_state, 2);
	/* Apply only the existing per-game compatibility database's flags. Do not
	 * force SP_GP_HIT_RAM globally: unresolved accesses through $sp/$gp can
	 * target BIOS or scratchpad as well as RAM. Constant-proven RAM accesses
	 * still receive the fast RAM path from Lightrec's optimizer. */
	lightrec_set_unsafe_opt_flags(lightrec_state, opt_flags);
#ifdef PCSX_PORT_PROFILE
	SysPrintf("Lightrec optimization flags: 0x%x\n", opt_flags);
#endif
	intApplyConfig();
}

R3000Acpu psxRec = {
	lightrec_plugin_init,
	lightrec_plugin_reset,
	lightrec_plugin_execute,
	lightrec_plugin_execute_block,
	lightrec_plugin_clear,
	lightrec_plugin_notify,
	lightrec_plugin_apply_config,
	lightrec_plugin_shutdown,
};
