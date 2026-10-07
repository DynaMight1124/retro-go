/*
 * (C) Gražvydas "notaz" Ignotas, 2010-2011
 *
 * This work is licensed under the terms of GNU GPL version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifdef PCSX_DUAL_DYNAREC
/* Shared core files use LIGHTREC, but this translation unit must compile the
 * Ari64 implementation and export a separate CPU descriptor. */
#undef LIGHTREC
#define psxRec psxRecRv32
#define lightrec_plugin_get_profile rv32_plugin_get_profile
#endif

#include <stdio.h>
#include <assert.h>
#if defined(ESP_PLATFORM) && defined(NDRC_GTE_PROFILE)
#include <esp_cpu.h>
/* PCSX's portable config supplies its own bounded path length. */
#undef MAXPATHLEN
#endif

#include "emu_if.h"
#include "pcsxmem.h"
#include "../psxhle.h"
#include "../psxinterpreter.h"
#include "../psxcounters.h"
#include "../psxevents.h"
#include "../sio.h"
#include "../psxbios.h"
#include "../r3000a.h"
#include "../gte_arm.h"
#include "../gte_neon.h"
#include "compiler_features.h"
#include "arm_features.h"
#define FLAGLESS
#include "../gte.h"
#if defined(NDRC_THREAD) && !defined(DRC_DISABLE) && !defined(LIGHTREC)
#include "../../frontend/pcsxr-threads.h"
#include "features/features_cpu.h"
#include "retro_timers.h"
#endif
#ifdef _3DS
#include <3ds_utils.h>
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

//#define evprintf printf
#define evprintf(...)

static void ari64_thread_sync(void);

void ndrc_freeze(void *f, int mode)
{
	const char header_save[8] = "ariblks";
	uint32_t addrs[1024 * 4];
	int32_t size = 0;
	int bytes;
	char header[8];

	ari64_thread_sync();

	if (mode != 0) { // save
		size = new_dynarec_save_blocks(addrs, sizeof(addrs));
		if (size == 0)
			return;

		SaveFuncs.write(f, header_save, sizeof(header_save));
		SaveFuncs.write(f, &size, sizeof(size));
		SaveFuncs.write(f, addrs, size);
	}
	else {
		do {
			bytes = SaveFuncs.read(f, header, sizeof(header));
			if (bytes != sizeof(header) || strcmp(header, header_save)) {
				if (bytes > 0)
					SaveFuncs.seek(f, -bytes, SEEK_CUR);
				break;
			}
			SaveFuncs.read(f, &size, sizeof(size));
			if (size <= 0) {
				size = 0;
				break;
			}
			if (size > sizeof(addrs)) {
				bytes = size - sizeof(addrs);
				SaveFuncs.seek(f, bytes, SEEK_CUR);
				size = sizeof(addrs);
			}
			bytes = SaveFuncs.read(f, addrs, size);
			if (bytes != size)
				size = 0;
		} while (0);

		if (psxCpu != &psxInt)
			new_dynarec_load_blocks(addrs, size);
	}

	//printf("drc: %d block info entries %s\n", size/8, mode ? "saved" : "loaded");
}

void ndrc_clear_full(void)
{
	ari64_thread_sync();
	new_dynarec_clear_full();
}

#if !defined(DRC_DISABLE) && !defined(LIGHTREC)
#include "linkage_offsets.h"

static void ari64_thread_init(void);
static int  ari64_thread_check_range(unsigned int start, unsigned int end);

void pcsx_mtc0(psxRegisters *regs, u32 reg, u32 val)
{
	evprintf("MTC0 %d #%x @%08x %u\n", reg, val, regs->pc, regs->cycle);
	MTC0(regs, reg, val);
	gen_interupt(&regs->CP0);
}

void pcsx_mtc0_ds(psxRegisters *regs, u32 reg, u32 val)
{
	evprintf("MTC0 %d #%x @%08x %u\n", reg, val, regs->pc, regs->cycle);
	MTC0(regs, reg, val);
}

/* GTE stuff */
void *gte_handlers[64];

void *gte_handlers_nf[64] = {
	NULL      , gteRTPS_nf , NULL       , NULL      , NULL     , NULL       , gteNCLIP_nf, NULL      , // 00
	NULL      , NULL       , NULL       , NULL      , gteOP_nf , NULL       , NULL       , NULL      , // 08
	gteDPCS_nf, gteINTPL_nf, gteMVMVA_nf, gteNCDS_nf, gteCDP_nf, NULL       , gteNCDT_nf , NULL      , // 10
	NULL      , NULL       , NULL       , gteNCCS_nf, gteCC_nf , NULL       , gteNCS_nf  , NULL      , // 18
	gteNCT_nf , NULL       , NULL       , NULL      , NULL     , NULL       , NULL       , NULL      , // 20
	gteSQR_nf , gteDCPL_nf , gteDPCT_nf , NULL      , NULL     , gteAVSZ3_nf, gteAVSZ4_nf, NULL      , // 28 
	gteRTPT_nf, NULL       , NULL       , NULL      , NULL     , NULL       , NULL       , NULL      , // 30
	NULL      , NULL       , NULL       , NULL      , NULL     , gteGPF_nf  , gteGPL_nf  , gteNCCT_nf, // 38
};

#if defined(ESP_PLATFORM)
#define GTE_PROFILE_SAMPLE_SHIFT 4
#define GTE_PROFILE_SAMPLE_COUNT (1u << GTE_PROFILE_SAMPLE_SHIFT)

typedef void (*gte_handler_t)(psxCP2Regs *regs);

#ifdef NDRC_GTE_PROFILE
static gte_handler_t gte_profile_handlers[64];
static gte_handler_t gte_profile_handlers_nf[64];
static u64 gte_profile_cycles;
static u32 gte_profile_calls;
static u32 gte_profile_no_flags_calls;
static u64 gte_profile_opcode_cycles[64];
static u32 gte_profile_opcode_calls[64];
static u32 gte_profile_sample_phase;

static void gte_profile_call(psxCP2Regs *regs, int no_flags)
{
	u32 op = psxRegs.code & 0x3f;
	gte_handler_t handler = no_flags ? gte_profile_handlers_nf[op] :
		gte_profile_handlers[op];
	u32 sample_phase = ++gte_profile_sample_phase;
	int sample = (sample_phase & (GTE_PROFILE_SAMPLE_COUNT - 1)) == 0;
	u32 start_cycles = sample ? esp_cpu_get_cycle_count() : 0;

	handler(regs);
	if (sample) {
		u32 elapsed = (u32)(esp_cpu_get_cycle_count() - start_cycles);
		gte_profile_cycles += elapsed;
		gte_profile_calls++;
		gte_profile_no_flags_calls += no_flags;
		gte_profile_opcode_cycles[op] += elapsed;
		gte_profile_opcode_calls[op]++;
	}
}

static void gte_profile_flags(psxCP2Regs *regs)
{
	gte_profile_call(regs, 0);
}

static void gte_profile_no_flags(psxCP2Regs *regs)
{
	gte_profile_call(regs, 1);
}
#endif

/* Retain the frontend profiling ABI used by the previous Lightrec build so
 * the same 60-frame report can compare both recompilers directly. */
void lightrec_plugin_get_profile(u64 *cycles, u32 *calls,
		u32 *no_flags_calls, u64 opcode_cycles[64],
		u32 opcode_calls[64])
{
#ifdef NDRC_GTE_PROFILE
	*cycles = gte_profile_cycles << GTE_PROFILE_SAMPLE_SHIFT;
	*calls = gte_profile_calls << GTE_PROFILE_SAMPLE_SHIFT;
	*no_flags_calls = gte_profile_no_flags_calls << GTE_PROFILE_SAMPLE_SHIFT;
	for (u32 i = 0; i < ARRAY_SIZE(gte_profile_opcode_cycles); i++) {
		opcode_cycles[i] =
			gte_profile_opcode_cycles[i] << GTE_PROFILE_SAMPLE_SHIFT;
		opcode_calls[i] =
			gte_profile_opcode_calls[i] << GTE_PROFILE_SAMPLE_SHIFT;
		gte_profile_opcode_cycles[i] = 0;
		gte_profile_opcode_calls[i] = 0;
	}
	gte_profile_cycles = 0;
	gte_profile_calls = 0;
	gte_profile_no_flags_calls = 0;
#else
	*cycles = 0;
	*calls = 0;
	*no_flags_calls = 0;
	memset(opcode_cycles, 0, 64 * sizeof(*opcode_cycles));
	memset(opcode_calls, 0, 64 * sizeof(*opcode_calls));
#endif
}
#endif

const char *gte_regnames[64] = {
	NULL  , "RTPS" , NULL   , NULL  , NULL , NULL   , "NCLIP", NULL  , // 00
	NULL  , NULL   , NULL   , NULL  , "OP" , NULL   , NULL   , NULL  , // 08
	"DPCS", "INTPL", "MVMVA", "NCDS", "CDP", NULL   , "NCDT" , NULL  , // 10
	NULL  , NULL   , NULL   , "NCCS", "CC" , NULL   , "NCS"  , NULL  , // 18
	"NCT" , NULL   , NULL   , NULL  , NULL , NULL   , NULL   , NULL  , // 20
	"SQR" , "DCPL" , "DPCT" , NULL  , NULL , "AVSZ3", "AVSZ4", NULL  , // 28 
	"RTPT", NULL   , NULL   , NULL  , NULL , NULL   , NULL   , NULL  , // 30
	NULL  , NULL   , NULL   , NULL  , NULL , "GPF"  , "GPL"  , "NCCT", // 38
};

#define GCBIT(x) \
	(1ll << (32+x))
#define GDBIT(x) \
	(1ll << (x))
#define GCBITS3(b0,b1,b2) \
	(GCBIT(b0) | GCBIT(b1) | GCBIT(b2))
#define GDBITS2(b0,b1) \
	(GDBIT(b0) | GDBIT(b1))
#define GDBITS3(b0,b1,b2) \
	(GDBITS2(b0,b1) | GDBIT(b2))
#define GDBITS4(b0,b1,b2,b3) \
	(GDBITS3(b0,b1,b2) | GDBIT(b3))
#define GDBITS5(b0,b1,b2,b3,b4) \
	(GDBITS4(b0,b1,b2,b3) | GDBIT(b4))
#define GDBITS6(b0,b1,b2,b3,b4,b5) \
	(GDBITS5(b0,b1,b2,b3,b4) | GDBIT(b5))
#define GDBITS7(b0,b1,b2,b3,b4,b5,b6) \
	(GDBITS6(b0,b1,b2,b3,b4,b5) | GDBIT(b6))
#define GDBITS8(b0,b1,b2,b3,b4,b5,b6,b7) \
	(GDBITS7(b0,b1,b2,b3,b4,b5,b6) | GDBIT(b7))
#define GDBITS9(b0,b1,b2,b3,b4,b5,b6,b7,b8) \
	(GDBITS8(b0,b1,b2,b3,b4,b5,b6,b7) | GDBIT(b8))
#define GDBITS10(b0,b1,b2,b3,b4,b5,b6,b7,b8,b9) \
	(GDBITS9(b0,b1,b2,b3,b4,b5,b6,b7,b8) | GDBIT(b9))

const uint64_t gte_reg_reads[64] = {
	[GTE_RTPS]  = 0x1f0000ff00000000ll | GDBITS7(0,1,13,14,17,18,19),
	[GTE_NCLIP] =                        GDBITS3(12,13,14),
	[GTE_OP]    = GCBITS3(0,2,4)       | GDBITS3(9,10,11),
	[GTE_DPCS]  = GCBITS3(21,22,23)    | GDBITS4(6,8,21,22),
	[GTE_INTPL] = GCBITS3(21,22,23)    | GDBITS7(6,8,9,10,11,21,22),
	[GTE_MVMVA] = 0x00ffffff00000000ll | GDBITS9(0,1,2,3,4,5,9,10,11), // XXX: maybe decode further?
	[GTE_NCDS]  = 0x00ffff0000000000ll | GDBITS6(0,1,6,8,21,22),
	[GTE_CDP]   = 0x00ffe00000000000ll | GDBITS7(6,8,9,10,11,21,22),
	[GTE_NCDT]  = 0x00ffff0000000000ll | GDBITS8(0,1,2,3,4,5,6,8),
	[GTE_NCCS]  = 0x001fff0000000000ll | GDBITS5(0,1,6,21,22),
	[GTE_CC]    = 0x001fe00000000000ll | GDBITS6(6,9,10,11,21,22),
	[GTE_NCS]   = 0x001fff0000000000ll | GDBITS5(0,1,6,21,22),
	[GTE_NCT]   = 0x001fff0000000000ll | GDBITS7(0,1,2,3,4,5,6),
	[GTE_SQR]   =                        GDBITS3(9,10,11),
	[GTE_DCPL]  = GCBITS3(21,22,23)    | GDBITS7(6,8,9,10,11,21,22),
	[GTE_DPCT]  = GCBITS3(21,22,23)    | GDBITS4(8,20,21,22),
	[GTE_AVSZ3] = GCBIT(29)            | GDBITS3(17,18,19),
	[GTE_AVSZ4] = GCBIT(30)            | GDBITS4(16,17,18,19),
	[GTE_RTPT]  = 0x1f0000ff00000000ll | GDBITS7(0,1,2,3,4,5,19),
	[GTE_GPF]   =                        GDBITS7(6,8,9,10,11,21,22),
	[GTE_GPL]   =                        GDBITS10(6,8,9,10,11,21,22,25,26,27),
	[GTE_NCCT]  = 0x001fff0000000000ll | GDBITS7(0,1,2,3,4,5,6),
};

// note: this excludes gteFLAG that is always written to
const uint64_t gte_reg_writes[64] = {
	[GTE_RTPS]  = 0x0f0f7f00ll,
	[GTE_NCLIP] = GDBIT(24),
	[GTE_OP]    = GDBITS6(9,10,11,25,26,27),
	[GTE_DPCS]  = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_INTPL] = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_MVMVA] = GDBITS6(9,10,11,25,26,27),
	[GTE_NCDS]  = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_CDP]   = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_NCDT]  = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_NCCS]  = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_CC]    = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_NCS]   = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_NCT]   = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_SQR]   = GDBITS6(9,10,11,25,26,27),
	[GTE_DCPL]  = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_DPCT]  = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_AVSZ3] = GDBITS2(7,24),
	[GTE_AVSZ4] = GDBITS2(7,24),
	[GTE_RTPT]  = 0x0f0f7f00ll,
	[GTE_GPF]   = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_GPL]   = GDBITS9(9,10,11,20,21,22,25,26,27),
	[GTE_NCCT]  = GDBITS9(9,10,11,20,21,22,25,26,27),
};

static void ari64_reset()
{
	ari64_thread_sync();
	new_dyna_pcsx_mem_reset();
	new_dynarec_invalidate_all_pages();
	new_dyna_pcsx_mem_load_state();
}

// execute until predefined leave points
// (HLE softcall exit and BIOS fastboot end)
static void ari64_execute_until(psxRegisters *regs)
{
	void *drc_local = (char *)regs - LO_psxRegs;

	assert(drc_local == dynarec_local);
	evprintf("+exec %08x, %u->%u (%d)\n", regs->pc, regs->cycle,
		regs->next_interupt, regs->next_interupt - regs->cycle);

	new_dyna_start(drc_local);

	evprintf("-exec %08x, %u->%u (%d) stop %d \n", regs->pc, regs->cycle,
		regs->next_interupt, regs->next_interupt - regs->cycle, regs->stop);
}

static void ari64_execute(struct psxRegisters *regs)
{
	while (!regs->stop) {
		schedule_timeslice(regs);
		ari64_execute_until(regs);
		evprintf("drc left @%08x\n", regs->pc);
	}
}

static void ari64_execute_block(struct psxRegisters *regs, enum blockExecCaller caller)
{
	if (caller == EXEC_CALLER_BOOT)
		regs->stop++;

	regs->next_interupt = regs->cycle + 1;
	ari64_execute_until(regs);

	if (caller == EXEC_CALLER_BOOT)
		regs->stop--;
}

static void ari64_clear(u32 addr, u32 size)
{
	u32 end = addr + size * 4; /* PCSX uses DMA units (words) */

	evprintf("ari64_clear %08x %04x\n", addr, size * 4);

	if (!new_dynarec_quick_check_range(addr, end) &&
	    !ari64_thread_check_range(addr, end))
		return;

	ari64_thread_sync();
	new_dynarec_invalidate_range(addr, end);
}

static void ari64_on_ext_change(int ram_replaced, int other_cpu_emu_exec)
{
	if (ram_replaced)
		ari64_reset();
	else if (other_cpu_emu_exec)
		new_dyna_pcsx_mem_load_state();
}

static void ari64_notify(enum R3000Anote note, void *data) {
	switch (note)
	{
	case R3000ACPU_NOTIFY_CACHE_UNISOLATED:
	case R3000ACPU_NOTIFY_CACHE_ISOLATED:
		new_dyna_pcsx_mem_isolate(note == R3000ACPU_NOTIFY_CACHE_ISOLATED);
		break;
	case R3000ACPU_NOTIFY_BEFORE_SAVE:
		break;
	case R3000ACPU_NOTIFY_AFTER_LOAD:
		ari64_on_ext_change(data == NULL, 0);
		// fallthrough
	// for state load, we take no action to not duplicate ndrc_freeze() work
	case R3000ACPU_NOTIFY_AFTER_LOAD_STATE:
		psxInt.Notify(note, data);
		break;
	}
}

static void ari64_apply_config()
{
	int thread_changed;

	ari64_thread_sync();
	intApplyConfig();

	if (Config.DisableStalls)
		ndrc_g.hacks |= NDHACK_NO_STALLS;
	else
		ndrc_g.hacks &= ~NDHACK_NO_STALLS;

	thread_changed = ((ndrc_g.hacks | ndrc_g.hacks_pergame) ^ ndrc_g.hacks_old)
		& (NDHACK_THREAD_FORCE | NDHACK_THREAD_FORCE_ON);
	if (Config.cycle_multiplier != ndrc_g.cycle_multiplier_old
	    || (ndrc_g.hacks | ndrc_g.hacks_pergame) != ndrc_g.hacks_old)
	{
		new_dynarec_clear_full();
	}
	if (thread_changed)
		ari64_thread_init();
}

#ifdef NDRC_THREAD
static void clear_local_cache(void)
{
#if defined(__arm__) || defined(__aarch64__)
	if (ndrc_g.thread.dirty_start) {
		// see "Ensuring the visibility of updates to instructions"
		// in v7/v8 reference manuals (DDI0406, DDI0487 etc.)
#if defined(__aarch64__) || defined(HAVE_ARMV8)
		// the actual clean/invalidate is broadcast to all cores,
		// the manual only prescribes an isb
		__asm__ volatile("isb");
//#elif defined(_3DS)
//		ctr_invalidate_icache();
#else
		// while on v6 this is always required, on v7 it depends on
		// "Multiprocessing Extensions" being present, but that is difficult
		// to detect so do it always for now
		new_dyna_clear_cache(ndrc_g.thread.dirty_start, ndrc_g.thread.dirty_end);
#endif
		ndrc_g.thread.dirty_start = ndrc_g.thread.dirty_end = 0;
	}
#endif
}

static void mixed_execute_block(struct psxRegisters *regs, enum blockExecCaller caller)
{
	psxInt.ExecuteBlock(regs, caller);
}

static void mixed_clear(u32 addr, u32 size)
{
	ari64_clear(addr, size);
	psxInt.Clear(addr, size);
}

static void mixed_notify(enum R3000Anote note, void *data)
{
	ari64_notify(note, data);
	psxInt.Notify(note, data);
}

static R3000Acpu psxMixedCpu = {
	NULL /* Init */, NULL /* Reset */, NULL /* Execute */,
	mixed_execute_block,
	mixed_clear,
	mixed_notify,
	NULL /* ApplyConfig */,	NULL /* Shutdown */
};

static noinline void ari64_execute_threaded_slow(struct psxRegisters *regs,
	enum blockExecCaller block_caller)
{
	if (ndrc_g.thread.busy_addr == ~0u) {
		memcpy(ndrc_smrv_regs, regs->GPR.r, sizeof(ndrc_smrv_regs));
		slock_lock(ndrc_g.thread.lock);
		ndrc_g.thread.busy_addr = regs->pc;
		slock_unlock(ndrc_g.thread.lock);
		scond_signal(ndrc_g.thread.cond);
	}

	//ari64_notify(R3000ACPU_NOTIFY_BEFORE_SAVE, NULL);
	psxInt.Notify(R3000ACPU_NOTIFY_AFTER_LOAD, NULL);
	assert(psxCpu == &psxRec);
	psxCpu = &psxMixedCpu;
	for (;;)
	{
		mixed_execute_block(regs, block_caller);

		if (ndrc_g.thread.busy_addr == ~0u)
			break;
		if (block_caller == EXEC_CALLER_HLE) {
			if (!psxBiosSoftcallEnded())
				continue;
			break;
		}
		else if (block_caller == EXEC_CALLER_BOOT) {
			if (!psxExecuteBiosEnded())
				continue;
			break;
		}
		if (regs->stop)
			break;
	}
	psxCpu = &psxRec;

	psxInt.Notify(R3000ACPU_NOTIFY_BEFORE_SAVE, NULL);
	//ari64_notify(R3000ACPU_NOTIFY_AFTER_LOAD, NULL);
	ari64_on_ext_change(0, 1);
}

static void ari64_execute_threaded_once(struct psxRegisters *regs,
	enum blockExecCaller block_caller)
{
	void *drc_local = (char *)regs - LO_psxRegs;
	struct ht_entry *hash_table =
		*(void **)((char *)drc_local + LO_hash_table_ptr);
	void *target;

	if (likely(ndrc_g.thread.busy_addr == ~0u)) {
		target = ndrc_get_addr_ht_param(hash_table, regs->pc,
				ndrc_cm_no_compile);
		if (target) {
			clear_local_cache();
			new_dyna_start_at(drc_local, target);
			return;
		}
	}
	ari64_execute_threaded_slow(regs, block_caller);
}

static void ari64_execute_threaded(struct psxRegisters *regs)
{
	schedule_timeslice(regs);
	while (!regs->stop)
	{
		ari64_execute_threaded_once(regs, EXEC_CALLER_OTHER);

		if ((s32)(regs->cycle - regs->next_interupt) >= 0)
			schedule_timeslice(regs);
	}
}

static void ari64_execute_threaded_block(struct psxRegisters *regs,
	enum blockExecCaller caller)
{
	if (caller == EXEC_CALLER_BOOT)
		regs->stop++;

	regs->next_interupt = regs->cycle + 1;

	ari64_execute_threaded_once(regs, caller);
	if (regs->cpuInRecursion) {
		// must sync since we are returning to compiled code
		ari64_thread_sync();
	}

	if (caller == EXEC_CALLER_BOOT)
		regs->stop--;
}

static void ari64_thread_sync(void)
{
	if (!ndrc_g.thread.lock || ndrc_g.thread.busy_addr == ~0u)
		return;
	for (;;) {
		slock_lock(ndrc_g.thread.lock);
		slock_unlock(ndrc_g.thread.lock);
		if (ndrc_g.thread.busy_addr == ~0)
			break;
		retro_sleep(0);
	}
}

static int ari64_thread_check_range(unsigned int start, unsigned int end)
{
	u32 addr = ndrc_g.thread.busy_addr;
	if (addr == ~0u)
		return 0;

	addr &= 0x1fffffff;
	start &= 0x1fffffff;
	end &= 0x1fffffff;
	if (addr >= end)
		return 0;
	if (addr + MAXBLOCK * 4 <= start)
		return 0;

	//SysPrintf("%x hits %x-%x\n", addr, start, end);
	return 1;
}

static STRHEAD_RET_TYPE ari64_compile_thread(void *unused)
{
	struct ht_entry *hash_table =
		*(void **)((char *)dynarec_local + LO_hash_table_ptr);
	void *target;
	u32 addr;

	slock_lock(ndrc_g.thread.lock);
	while (!ndrc_g.thread.exit)
	{
		addr = *(volatile unsigned int *)&ndrc_g.thread.busy_addr;
		if (addr == ~0u)
			scond_wait(ndrc_g.thread.cond, ndrc_g.thread.lock);
		addr = *(volatile unsigned int *)&ndrc_g.thread.busy_addr;
		if (addr == ~0u || ndrc_g.thread.exit)
			continue;

		target = ndrc_get_addr_ht_param(hash_table, addr,
				ndrc_cm_compile_in_thread);
		//printf("c  %08x -> %p\n", addr, target);
		ndrc_g.thread.busy_addr = ~0u;
	}
	slock_unlock(ndrc_g.thread.lock);
	(void)target;
	STRHEAD_RETURN();
}

static void ari64_thread_shutdown(void)
{
	psxRec.Execute = ari64_execute;
	psxRec.ExecuteBlock = ari64_execute_block;

	if (ndrc_g.thread.lock)
		slock_lock(ndrc_g.thread.lock);
	ndrc_g.thread.exit = 1;
	if (ndrc_g.thread.lock)
		slock_unlock(ndrc_g.thread.lock);
	if (ndrc_g.thread.cond)
		scond_signal(ndrc_g.thread.cond);
	if (ndrc_g.thread.handle) {
		sthread_join(ndrc_g.thread.handle);
		ndrc_g.thread.handle = NULL;
	}
	if (ndrc_g.thread.cond) {
		scond_free(ndrc_g.thread.cond);
		ndrc_g.thread.cond = NULL;
	}
	if (ndrc_g.thread.lock) {
		slock_free(ndrc_g.thread.lock);
		ndrc_g.thread.lock = NULL;
	}
	ndrc_g.thread.busy_addr = ~0u;
}

static void ari64_thread_init(void)
{
	int enable;

	if (ndrc_g.hacks_pergame & NDHACK_THREAD_FORCE)
		enable = 0;
	else if (ndrc_g.hacks & NDHACK_THREAD_FORCE)
		enable = ndrc_g.hacks & NDHACK_THREAD_FORCE_ON;
	else {
		u32 cpu_count = cpu_features_get_core_amount();
		enable = cpu_count > 1;
#ifdef _3DS
		// bad for old3ds, reprotedly no improvement for new3ds
		enable = 0;
#endif
	}

	if (!ndrc_g.thread.handle == !enable)
		return;

	ari64_thread_shutdown();
	ndrc_g.thread.exit = 0;
	ndrc_g.thread.busy_addr = ~0u;

	if (enable) {
		ndrc_g.thread.lock = slock_new();
		ndrc_g.thread.cond = scond_new();
	}
	if (ndrc_g.thread.lock && ndrc_g.thread.cond)
		ndrc_g.thread.handle = pcsxr_sthread_create(ari64_compile_thread, PCSXRT_DRC);
	if (ndrc_g.thread.handle) {
		psxRec.Execute = ari64_execute_threaded;
		psxRec.ExecuteBlock = ari64_execute_threaded_block;
	}
	else {
		// clean up potential incomplete init
		ari64_thread_shutdown();
	}
	SysPrintf("compiler thread %sabled\n", ndrc_g.thread.handle ? "en" : "dis");
}
#else // if !NDRC_THREAD
static void ari64_thread_init(void) {}
static void ari64_thread_shutdown(void) {}
static int ari64_thread_check_range(unsigned int start, unsigned int end) { return 0; }
#endif

static int ari64_init()
{
	static u32 scratch_buf[8*8*2] __attribute__((aligned(64)));
	size_t i;

	new_dynarec_init();
	new_dyna_pcsx_mem_init();

	for (i = 0; i < ARRAY_SIZE(gte_handlers); i++)
		if (psxCP2[i] != gteNULL)
			gte_handlers[i] = psxCP2[i];

#if defined(__arm__) && !defined(DRC_DBG)
	gte_handlers[0x06] = gteNCLIP_arm;
#ifdef HAVE_ARMV5
	gte_handlers_nf[0x01] = gteRTPS_nf_arm;
	gte_handlers_nf[0x30] = gteRTPT_nf_arm;
#endif
#ifdef __ARM_NEON__
	// compiler's _nf version is still a lot slower than neon
	// _nf_arm RTPS is roughly the same, RTPT slower
	gte_handlers[0x01] = gte_handlers_nf[0x01] = gteRTPS_neon;
	gte_handlers[0x30] = gte_handlers_nf[0x30] = gteRTPT_neon;
#endif
#endif
#ifdef DRC_DBG
	memcpy(gte_handlers_nf, gte_handlers, sizeof(gte_handlers_nf));
#endif
#if defined(ESP_PLATFORM) && defined(NDRC_GTE_PROFILE)
	for (i = 0; i < ARRAY_SIZE(gte_handlers); i++) {
		if (gte_handlers[i] == NULL)
			continue;
		gte_profile_handlers[i] = (gte_handler_t)gte_handlers[i];
		gte_profile_handlers_nf[i] = (gte_handler_t)gte_handlers_nf[i];
		gte_handlers[i] = gte_profile_flags;
		gte_handlers_nf[i] = gte_profile_no_flags;
	}
#endif
	psxH_ptr = psxRegs.ptrs.psxH;
	zeromem_ptr = zero_mem;
	scratch_buf_ptr = scratch_buf; // for gte_neon.S

	ndrc_g.cycle_multiplier_old = Config.cycle_multiplier;
	ndrc_g.hacks_old = ndrc_g.hacks | ndrc_g.hacks_pergame;
	ari64_apply_config();
	ari64_thread_init();

	return 0;
}

static void ari64_shutdown()
{
	ari64_thread_shutdown();
	new_dynarec_cleanup();
	new_dyna_pcsx_mem_shutdown();
}

R3000Acpu psxRec = {
	ari64_init,
	ari64_reset,
	ari64_execute,
	ari64_execute_block,
	ari64_clear,
	ari64_notify,
	ari64_apply_config,
	ari64_shutdown
};

#else // if DRC_DISABLE

struct ndrc_globals ndrc_g; // dummy
void new_dynarec_init() {}
void new_dyna_start(void *context) {}
void new_dynarec_cleanup() {}
void new_dynarec_clear_full() {}
void new_dynarec_invalidate_all_pages() {}
void new_dynarec_invalidate_range(unsigned int start, unsigned int end) {}
void new_dyna_pcsx_mem_init(void) {}
void new_dyna_pcsx_mem_reset(void) {}
void new_dyna_pcsx_mem_load_state(void) {}
void new_dyna_pcsx_mem_isolate(int enable) {}
void new_dyna_pcsx_mem_shutdown(void) {}
int  new_dynarec_save_blocks(void *save, int size) { return 0; }
void new_dynarec_load_blocks(const void *save, int size) {}

#endif // DRC_DISABLE

#ifndef NDRC_THREAD
static void ari64_thread_sync(void) {}
#endif

#ifdef DRC_DBG

#include <stddef.h>
static FILE *f;
static int drc_dbg_mode;
static int drc_dbg_failed;
static unsigned drc_dbg_trace_insns;
/* The 23.0M--23.7M window is now instruction- and RAM-write-exact after the
 * RV32 LWL/LWR variable-mask fix. Overlap that proven boundary, then continue
 * through the original frame-60 symptom near 33.8M cycles in one capture. */
#define DRC_DBG_START_CYCLE 23700000u
#define DRC_DBG_END_CYCLE 34000000u
static u32 drc_dbg_block, drc_dbg_ccadj;
u32 drc_dbg_irq_rel_before, drc_dbg_irq_last_before;
u32 drc_dbg_irq_cycle_after, drc_dbg_irq_next_after;
static char drc_dbg_iobuf[32768];
static psxRegisters trace_oldregs;
static u32 trace_event_cycles[PSXINT_COUNT];
static u32 trace_old_irq_test_cycle;
static u32 trace_old_handler_cycle;
static u32 trace_old_io_addr;
static u32 trace_old_io_data;
static psxRegisters cmp_regs;
static u32 cmp_mem_addr, cmp_mem_val;
static u32 cmp_irq_test_cycle;
static u32 cmp_handler_cycle;
static u32 cmp_ppc, cmp_failcount;
static u32 cmp_badregs_mask_prev;
static unsigned drc_dbg_compare_insns;
int ndrc_dbg_stopped(void) { return drc_dbg_failed; }
static int miss_log_i, miss_log_valid;
static struct {
	u32 pc, actual_cycle, expected_cycle, block, ccadj, last;
	s32 raw_cc, stored_cc;
} cycle_log[32];
static unsigned cycle_log_i;
u32 irq_test_cycle;
u32 handler_cycle;
u32 last_io_addr;
static unsigned drc_dbg_interpreter_nesting;

static void do_insn_cmp_internal(u32 block, u32 ccadj, s32 raw_cc,
	u32 cycle_base);

void ndrc_dbg_interpreter_compare_begin(void)
{
	drc_dbg_interpreter_nesting++;
}

void ndrc_dbg_interpreter_compare_end(void)
{
	if (drc_dbg_interpreter_nesting != 0)
		drc_dbg_interpreter_nesting--;
}

void dump_mem(const char *fname, void *mem, size_t size)
{
	FILE *f1 = fopen(fname, "wb");
	if (f1 == NULL)
		f1 = fopen(strrchr(fname, '/') + 1, "wb");
	fwrite(mem, 1, size, f1);
	fclose(f1);
}

static u32 memcheck_read(u32 a)
{
	if ((a >> 16) == 0x1f80)
		// scratchpad/IO
		return *(u32 *)(psxRegs.ptrs.psxH + (a & 0xfffc));

	if ((a >> 16) == 0x1f00)
		// parallel
		return *(u32 *)(psxRegs.ptrs.psxP + (a & 0xfffc));

//	if ((a & ~0xe0600000) < 0x200000)
	// RAM
	return *(u32 *)(psxRegs.ptrs.psxM + (a & 0x1ffffc));
}

int ndrc_dbg_trace_begin(const char *path)
{
	if (f != NULL)
		fclose(f);
	f = fopen(path, "wb");
	if (f == NULL) {
		SysPrintf("RV32 DIFF: unable to create %s\n", path);
		drc_dbg_mode = 0;
		return -1;
	}
	setvbuf(f, drc_dbg_iobuf, _IOFBF, sizeof(drc_dbg_iobuf));
	memset(&trace_oldregs, 0, sizeof(trace_oldregs));
	memset(trace_event_cycles, 0, sizeof(trace_event_cycles));
	trace_old_irq_test_cycle = trace_old_handler_cycle = 0xbad0c0de;
	trace_old_io_addr = trace_old_io_data = 0xbad0c0de;
	drc_dbg_trace_insns = 0;
	drc_dbg_failed = 0;
	drc_dbg_mode = 1;
	SysPrintf("RV32 DIFF: recording interpreter reference to %s\n", path);
	return 0;
}

void ndrc_dbg_trace_end(void)
{
	long bytes = f != NULL ? ftell(f) : -1;
	if (f != NULL) {
		fclose(f);
		f = NULL;
	}
	drc_dbg_mode = 0;
	SysPrintf("RV32 DIFF: reference complete: insns=%u bytes=%ld\n",
		drc_dbg_trace_insns, bytes);
}

int ndrc_dbg_compare_begin(const char *path)
{
	if (f != NULL)
		fclose(f);
	f = fopen(path, "rb");
	if (f == NULL) {
		SysPrintf("RV32 DIFF: unable to open %s\n", path);
		drc_dbg_mode = 0;
		return -1;
	}
	setvbuf(f, drc_dbg_iobuf, _IOFBF, sizeof(drc_dbg_iobuf));
	memset(&cmp_regs, 0, sizeof(cmp_regs));
	cmp_mem_addr = cmp_mem_val = 0;
	cmp_irq_test_cycle = cmp_handler_cycle = 0;
	cmp_ppc = cmp_failcount = cmp_badregs_mask_prev = 0;
	drc_dbg_compare_insns = 0;
	miss_log_i = miss_log_valid = 0;
	cycle_log_i = 0;
	drc_dbg_failed = 0;
	drc_dbg_mode = 2;
	SysPrintf("RV32 DIFF: native comparison starting\n");
	return 0;
}

void do_insn_trace(void)
{
	u32 *allregs_p = (void *)&psxRegs;
	u32 *allregs_o = (void *)&trace_oldregs;
	u32 io_data;
	int i;
	u8 byte;

	/* Some HLE helpers deliberately execute a guest instruction through the
	 * interpreter (notably the generic BIOS vsync-loop accelerator). The
	 * reference records those instructions, but there is no generated DRC
	 * comparison site while the same helper is running during native replay.
	 * Consume and compare those records at the interpreter's own boundaries. */
	if (drc_dbg_mode == 2 && drc_dbg_interpreter_nesting != 0) {
		do_insn_cmp_internal(psxRegs.pc, 0, (s32)psxRegs.cycle, 0);
		return;
	}
	if (drc_dbg_mode != 1 || f == NULL)
		return;
	if (psxRegs.cycle < DRC_DBG_START_CYCLE)
		return;
	if (psxRegs.cycle >= DRC_DBG_END_CYCLE) {
		drc_dbg_mode = 0;
		drc_dbg_failed = 1;
		psxRegs.stop = 1;
		SysPrintf("RV32 DIFF: reference window complete at cycle=%u\n", psxRegs.cycle);
		return;
	}
	if (drc_dbg_trace_insns == 0)
		SysPrintf("RV32 DIFF: reference window begins pc=%08x cycle=%u\n",
			psxRegs.pc, psxRegs.cycle);
	drc_dbg_trace_insns++;
	/* Bound SD usage during an extended reference run, and detect full-card
	 * errors before attempting to replay an incomplete instruction record. */
	if ((drc_dbg_trace_insns & 65535) == 0 &&
	    (ferror(f) || ftell(f) >= 128 * 1024 * 1024)) {
		SysPrintf("RV32 DIFF: reference stopped at storage limit/error, insns=%u\n",
			drc_dbg_trace_insns);
		drc_dbg_failed = 1;
		drc_dbg_mode = 0;
		psxRegs.stop = 1;
		return;
	}

	// log reg changes
	trace_oldregs.code = psxRegs.code; // don't care
	for (i = 0; i < offsetof(psxRegisters, intCycle) / 4; i++) {
		if (allregs_p[i] != allregs_o[i]) {
			fwrite(&i, 1, 1, f);
			fwrite(&allregs_p[i], 1, 4, f);
			allregs_o[i] = allregs_p[i];
		}
	}
	// log event changes
	for (i = 0; i < PSXINT_COUNT; i++) {
		if (psxRegs.event_cycles[i] != trace_event_cycles[i]) {
			byte = 0xf8;
			fwrite(&byte, 1, 1, f);
			fwrite(&i, 1, 1, f);
			fwrite(&psxRegs.event_cycles[i], 1, 4, f);
			trace_event_cycles[i] = psxRegs.event_cycles[i];
		}
	}
	#define SAVE_IF_CHANGED(code_, old_, name_) { \
		if ((old_) != (name_)) { \
			byte = code_; \
			fwrite(&byte, 1, 1, f); \
			fwrite(&(name_), 1, 4, f); \
			(old_) = (name_); \
		} \
	}
	SAVE_IF_CHANGED(0xfb, trace_old_irq_test_cycle, irq_test_cycle);
	SAVE_IF_CHANGED(0xfc, trace_old_handler_cycle, handler_cycle);
	SAVE_IF_CHANGED(0xfd, trace_old_io_addr, last_io_addr);
	io_data = memcheck_read(last_io_addr);
	SAVE_IF_CHANGED(0xfe, trace_old_io_data, io_data);
	#undef SAVE_IF_CHANGED
	byte = 0xff;
	fwrite(&byte, 1, 1, f);
}

static const char *regnames[offsetof(psxRegisters, intCycle) / 4] = {
	"r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
	"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
	"lo",  "hi",
	"C0_0",  "C0_1",  "C0_2",  "C0_3",  "C0_4",  "C0_5",  "C0_6",  "C0_7",
	"C0_8",  "C0_9",  "C0_10", "C0_11", "C0_12", "C0_13", "C0_14", "C0_15",
	"C0_16", "C0_17", "C0_18", "C0_19", "C0_20", "C0_21", "C0_22", "C0_23",
	"C0_24", "C0_25", "C0_26", "C0_27", "C0_28", "C0_29", "C0_30", "C0_31",

	"C2D0",  "C2D1",  "C2D2",  "C2D3",  "C2D4",  "C2D5",  "C2D6",  "C2D7",
	"C2D8",  "C2D9",  "C2D10", "C2D11", "C2D12", "C2D13", "C2D14", "C2D15",
	"C2D16", "C2D17", "C2D18", "C2D19", "C2D20", "C2D21", "C2D22", "C2D23",
	"C2D24", "C2D25", "C2D26", "C2D27", "C2D28", "C2D29", "C2D30", "C2D31",

	"C2C0",  "C2C1",  "C2C2",  "C2C3",  "C2C4",  "C2C5",  "C2C6",  "C2C7",
	"C2C8",  "C2C9",  "C2C10", "C2C11", "C2C12", "C2C13", "C2C14", "C2C15",
	"C2C16", "C2C17", "C2C18", "C2C19", "C2C20", "C2C21", "C2C22", "C2C23",
	"C2C24", "C2C25", "C2C26", "C2C27", "C2C28", "C2C29", "C2C30", "C2C31",

	"PC", "code", "cycle", "interrupt",
};

static struct {
	int reg;
	u32 val, val_expect;
	u32 pc, cycle;
} miss_log[64];
#define miss_log_len (sizeof(miss_log)/sizeof(miss_log[0]))
#define miss_log_mask (miss_log_len-1)

static void miss_log_add(int reg, u32 val, u32 val_expect, u32 pc, u32 cycle)
{
	miss_log[miss_log_i].reg = reg;
	miss_log[miss_log_i].val = val;
	miss_log[miss_log_i].val_expect = val_expect;
	miss_log[miss_log_i].pc = pc;
	miss_log[miss_log_i].cycle = cycle;
	miss_log_i = (miss_log_i + 1) & miss_log_mask;
	if (miss_log_valid < miss_log_len)
		miss_log_valid++;
}

void breakme() {}

static void do_insn_cmp_internal(u32 block, u32 ccadj, s32 raw_cc,
	u32 cycle_base)
{
	extern int cycle_count;
	u32 *allregs_p = (void *)&psxRegs;
	u32 *allregs_e = (void *)&cmp_regs;
	u32 badregs_mask = 0;
	int i, ret, bad = 0, fatal = 0, which_event = -1;
	s32 stored_cc = cycle_count;
	u32 ev_cycles = 0;
	u8 code;

	if (drc_dbg_mode != 2 || drc_dbg_failed || f == NULL)
		return;
	drc_dbg_block = block;
	drc_dbg_ccadj = ccadj;
	/* Generated comparison sites publish a cycle relative to last_count. */
	if (psxRegs.cycle + cycle_base < DRC_DBG_START_CYCLE)
		return;
	if (drc_dbg_compare_insns == 0)
		SysPrintf("RV32 DIFF: native window begins pc=%08x cycle=%u\n",
			psxRegs.pc, psxRegs.cycle + cycle_base);

	while (1) {
		if ((ret = fread(&code, 1, 1, f)) <= 0)
			break;
		if (ret <= 0)
			break;
		if (code == 0xff)
			break;
		switch (code) {
		case 0xf8:
			which_event = 0;
			fread(&which_event, 1, 1, f);
			fread(&ev_cycles, 1, 4, f);
			continue;
		case 0xfb:
			fread(&cmp_irq_test_cycle, 1, 4, f);
			continue;
		case 0xfc:
			fread(&cmp_handler_cycle, 1, 4, f);
			continue;
		case 0xfd:
			fread(&cmp_mem_addr, 1, 4, f);
			continue;
		case 0xfe:
			fread(&cmp_mem_val, 1, 4, f);
			continue;
		}
		assert(code < offsetof(psxRegisters, intCycle) / 4);
		fread(&allregs_e[code], 1, 4, f);
	}

	if (ret <= 0) {
		SysPrintf("RV32 DIFF: reference exhausted without an earlier mismatch\n");
		drc_dbg_failed = 1;
		psxRegs.stop = 1;
		return;
	}
	drc_dbg_compare_insns++;

	psxRegs.code = cmp_regs.code; // don't care
	psxRegs.cycle += cycle_base;
	psxRegs.CP0.r[9] = cmp_regs.CP0.r[9]; // Count
	cycle_log[cycle_log_i & 31] = (typeof(cycle_log[0])) {
		psxRegs.pc, psxRegs.cycle, cmp_regs.cycle,
		drc_dbg_block, drc_dbg_ccadj, cycle_base,
		raw_cc, stored_cc
	};
	cycle_log_i++;

	//if (psxRegs.cycle == 166172) breakme();

	if (which_event >= 0 && psxRegs.event_cycles[which_event] != ev_cycles) {
		printf("bad ev_cycles #%d: %u %u / %u\n", which_event,
			psxRegs.event_cycles[which_event], ev_cycles, psxRegs.cycle);
		fatal = 1;
	}

	if (irq_test_cycle > cmp_irq_test_cycle) {
		printf("bad irq_test_cycle: %u %u\n", irq_test_cycle, cmp_irq_test_cycle);
		fatal = 1;
	}

	if (handler_cycle != cmp_handler_cycle) {
		printf("bad handler_cycle: %u %u\n", handler_cycle, cmp_handler_cycle);
		fatal = 1;
	}

	if (cmp_mem_val != memcheck_read(cmp_mem_addr)) {
		printf("bad mem @%08x: %08x %08x\n", cmp_mem_addr,
			memcheck_read(cmp_mem_addr), cmp_mem_val);
		SysPrintf("RV32 DIFF memory context: s0=%08x/%08x s5=%08x/%08x\n",
			psxRegs.GPR.r[16], cmp_regs.GPR.r[16],
			psxRegs.GPR.r[21], cmp_regs.GPR.r[21]);
		for (i = 0; i < 6; i++) {
			u32 src = cmp_regs.GPR.r[16] + i * 4;
			u32 dst = cmp_regs.GPR.r[21] + i * 4;
			SysPrintf("RV32 DIFF memory word %d: src[%08x]=%08x "
				"dst[%08x]=%08x\n", i, src, memcheck_read(src),
				dst, memcheck_read(dst));
		}
		fatal = 1;
	}

	if (!fatal && !memcmp(&psxRegs, &cmp_regs, offsetof(psxRegisters, intCycle))) {
		cmp_failcount = 0;
		goto ok;
	}

	for (i = 0; i < offsetof(psxRegisters, intCycle) / 4; i++) {
		if (allregs_p[i] != allregs_e[i]) {
			miss_log_add(i, allregs_p[i], allregs_e[i], psxRegs.pc, psxRegs.cycle);
			bad++;
			if (i >= 32)
				fatal = 1;
			else
				badregs_mask |= 1u << i;
		}
	}

	if (cmp_badregs_mask_prev & badregs_mask)
		cmp_failcount++;
	else
		cmp_failcount = 0;

	if (!fatal && psxRegs.pc == cmp_regs.pc && bad < 6 && cmp_failcount < 24) {
		static int last_mcycle;
		if (last_mcycle != psxRegs.cycle >> 20) {
			printf("%u\n", psxRegs.cycle);
			last_mcycle = psxRegs.cycle >> 20;
		}
		goto ok;
	}

	miss_log_i = (miss_log_i - miss_log_valid) & miss_log_mask;
	for (i = 0; i < miss_log_valid; i++, miss_log_i = (miss_log_i + 1) & miss_log_mask)
		printf("bad %5s: %08x %08x, pc=%08x, cycle %u\n",
			regnames[miss_log[miss_log_i].reg], miss_log[miss_log_i].val,
			miss_log[miss_log_i].val_expect, miss_log[miss_log_i].pc, miss_log[miss_log_i].cycle);
	printf("-- %d\n", bad);
	for (i = 0; i < 8; i++)
		printf("r%d=%08x r%2d=%08x r%2d=%08x r%2d=%08x\n", i, allregs_p[i],
			i+8, allregs_p[i+8], i+16, allregs_p[i+16], i+24, allregs_p[i+24]);
	printf("PC: %08x/%08x, cycle %u/%u, next %u, instruction %u\n",
		psxRegs.pc, cmp_regs.pc, psxRegs.cycle, cmp_regs.cycle,
		psxRegs.next_interupt, drc_dbg_compare_insns);
	SysPrintf("RV32 DIFF FAILED: first persistent architectural mismatch\n");
	SysPrintf("RV32 DIFF timing: previous_pc=%08x block=%08x ccadj=%d last_count=%u\n",
		cmp_ppc, drc_dbg_block, (int)drc_dbg_ccadj, cycle_base);
	SysPrintf("RV32 DIFF interrupt: rel_before=%d last_before=%u cycle_after=%u next_after=%u\n",
		(int)drc_dbg_irq_rel_before, drc_dbg_irq_last_before,
		drc_dbg_irq_cycle_after, drc_dbg_irq_next_after);
	SysPrintf("RV32 DIFF recent cycle history (oldest first):\n");
	{
		unsigned count = cycle_log_i < 32 ? cycle_log_i : 32;
		unsigned first = cycle_log_i - count;
		for (unsigned n = 0; n < count; n++) {
			const typeof(cycle_log[0]) *h = &cycle_log[(first + n) & 31];
			SysPrintf("  %08x %u/%u block=%08x adj=%d last=%u raw=%d stored=%d\n",
				h->pc, h->actual_cycle, h->expected_cycle,
				h->block, (int)h->ccadj, h->last,
				h->raw_cc, h->stored_cc);
		}
	}
	/* Read mapped guest memory directly: diagnostics must not trigger I/O. */
	for (int side = 0; side < 3; side++) {
		u32 center = side == 2 ? cmp_ppc : side ? cmp_regs.pc : psxRegs.pc;
		for (int offset = -32; offset <= 8; offset += 4) {
			u32 addr = (center + offset) & ~3u;
			const u32 *word = PSXM(addr);
			if (word != NULL && word != INVALID_PTR)
				SysPrintf("RV32 DIFF code %s %08x: %08x\n",
					side == 2 ? "previous" : side ? "reference" : "native", addr, SWAP32(*word));
		}
	}
	drc_dbg_failed = 1;
	psxRegs.stop = 1;
	return;
ok:
	cmp_ppc = psxRegs.pc;
	cmp_badregs_mask_prev = badregs_mask;
}

void do_insn_cmp(u32 block, u32 ccadj, s32 raw_cc)
{
	extern int last_count;
	do_insn_cmp_internal(block, ccadj, raw_cc, last_count);
}

#else

/* Keep the Retro-Go front end independent of diagnostic build flags. A
 * developer can re-enable the full recorder/comparator with DRC_DBG without
 * changing the application loop. */
int ndrc_dbg_stopped(void) { return 0; }
int ndrc_dbg_trace_begin(const char *path)
{
	(void)path;
	return -1;
}

void ndrc_dbg_trace_end(void) {}

int ndrc_dbg_compare_begin(const char *path)
{
	(void)path;
	return -1;
}

#endif // DRC_DBG
