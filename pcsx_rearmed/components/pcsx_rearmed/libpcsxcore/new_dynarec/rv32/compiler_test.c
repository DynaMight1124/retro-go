/* SPDX-License-Identifier: GPL-2.0-or-later
 * Restricted straight-line harness for the shared decoder, allocation helpers
 * and ALU emitters. Not the complete compiler passes or guest CPU dispatch.
 */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "selftest.h"
#include "emit.h"
#include "block_cache.h"
#ifdef ESP_PLATFORM
#include <rg_system.h>
#include "../../r3000a.h"
#include "../../gte.h"
#include "../../psxmem.h"
#include "../../psxhle.h"
#include "../../psxinterpreter.h"
#include "../../psxevents.h"
#define DECLARE_GTE_NO_FLAGS(name) \
  void name##_nf(struct psxCP2Regs *regs)
DECLARE_GTE_NO_FLAGS(gteRTPS);
DECLARE_GTE_NO_FLAGS(gteNCLIP);
DECLARE_GTE_NO_FLAGS(gteOP);
DECLARE_GTE_NO_FLAGS(gteDPCS);
DECLARE_GTE_NO_FLAGS(gteINTPL);
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
#if defined(CONFIG_IDF_TARGET_ESP32P4)
DECLARE_GTE_NO_FLAGS(gteINTPL_sf0_lm0);
DECLARE_GTE_NO_FLAGS(gteINTPL_sf0_lm1);
DECLARE_GTE_NO_FLAGS(gteINTPL_sf1_lm0);
DECLARE_GTE_NO_FLAGS(gteINTPL_sf1_lm1);
#endif
#undef DECLARE_GTE_NO_FLAGS
extern int ndrc_rv32_interpreter_alu(uint32_t gpr[32], uint32_t code);
extern int ndrc_rv32_interpreter_memory(uint32_t gpr[32], uint32_t ram[64],
    const uint32_t *words, unsigned count, uint32_t *pc, uint32_t *cycles);
#endif
typedef unsigned int u_int;
typedef unsigned char u_char;
typedef uint32_t u32;
static struct rv32_emitter rv32_output;
#include "../assem_rv32_alu.h"

#define TEST_INSNS 64
#define HOST_CCREG NDRC_RV32_CC_SLOT
#define EXCLUDE_REG -1
#define PREFERRED_REG_FIRST 11
#define PREFERRED_REG_LAST 20
#define OVERFLOW_STUB 15
#define add_overflow(a,b,r) __builtin_add_overflow(a,b,&(r))
#define out ((u_char *)(rv32_output.code + rv32_output.count))
#include "../regstat.h"
#include "../decode_types.h"
#include "../compiler_enums.h"
struct compile_state {
  u_int start;
  int slen;
  uint64_t unneeded_reg[TEST_INSNS + 1];
};
/* Cold diagnostic state is temporary PSRAM on P4, not permanent internal BSS. */
static struct compiler_workspace {
  /* Allocation lookahead can inspect one instruction beyond slen. The full
   * compiler has MAXBLOCK headroom; provide an explicit inert sentinel here. */
  struct decoded_insn decoded[TEST_INSNS + 1];
  struct compile_info info[TEST_INSNS + 1];
  struct regstat history[TEST_INSNS];
  uint32_t constants[TEST_INSNS][HOST_REGS], current_constants[HOST_REGS];
  uint64_t gte_read[TEST_INSNS], gte_write[TEST_INSNS];
  struct compile_state state;
} *work;
#define dops (work->decoded)
#define cinfo (work->info)
#define regs (work->history)
#define constmap (work->constants)
#define current_constmap (work->current_constants)
#define gte_rs (work->gte_read)
#define gte_rt (work->gte_write)
/* Non-ALU opcodes are rejected BEFORE invoking the shared decoder, so these
 * unavailable runtime services cannot be reached in this fixture. */
static const struct { int HLE; } fixture_config = { 0 };
static const void *const fixture_psxHLEt[1] = { NULL };
static const void *const gte_handlers[64] = { NULL };
static const uint64_t gte_reg_reads[64] = { 0 }, gte_reg_writes[64] = { 0 };
#define GTE_MVMVA 0x12
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define set_mnemonic(i, text) ((void)0)
static void fixture_log(const char *format, ...) { (void)format; }
#define SysPrintf fixture_log
#define SysPrintf_lim fixture_log
static const unsigned ram_offset = 0;
struct test_cpu {
#ifdef ESP_PLATFORM
  /* Generated GPR accesses now use the exact production CPU-state prefix. */
  union {
    psxRegisters cpu;
    struct { uint32_t gpr[34]; };
  };
  uint32_t exception, pc, branch_target, block_budget;
#else
  uint32_t gpr[34], exception, pc, branch_target, block_budget;
  uint32_t muldiv_busy;
#endif
#ifndef ESP_PLATFORM
  uint32_t pending_value;
  uint8_t pending_reg;
#endif
  uint32_t ram[64]; /* bounded little-endian test RAM, not psxM */
};
#ifdef ESP_PLATFORM
_Static_assert(offsetof(struct test_cpu, gpr) == offsetof(psxRegisters, GPR.r),
  "generated GPR offsets must match psxRegisters");
_Static_assert(offsetof(struct test_cpu, cpu) == 0,
  "generated state pointer must also be a psxRegisters pointer");
_Static_assert(offsetof(struct test_cpu, cpu.pc) == offsetof(psxRegisters, pc),
  "private CPU state must preserve the production PC offset");
#define TEST_PC(v) ((v).cpu.pc)
#define TEST_CYCLE(v) ((v).cpu.cycle)
#define TEST_CYCLE_PTR(v, fallback) (&(v).cpu.cycle)
#define PENDING_VALUE_OFFSET offsetof(struct test_cpu, cpu.dloadVal[0])
#define PENDING_REG_OFFSET offsetof(struct test_cpu, cpu.dloadReg[0])
#define PENDING_ALT_VALUE_OFFSET offsetof(struct test_cpu, cpu.dloadVal[1])
#define PENDING_ALT_REG_OFFSET offsetof(struct test_cpu, cpu.dloadReg[1])
#define PENDING_SEL_OFFSET offsetof(struct test_cpu, cpu.dloadSel)
#define MULDIV_BUSY_OFFSET offsetof(struct test_cpu, cpu.muldivBusyCycle)
#define LOAD_DELAY_CLEAR(v) ((v).cpu.dloadReg[0] == 0 && (v).cpu.dloadVal[0] == 0)
#else
#define TEST_PC(v) ((v).pc)
#define TEST_CYCLE(v) (cycles)
#define TEST_CYCLE_PTR(v, fallback) (fallback)
#define PENDING_VALUE_OFFSET offsetof(struct test_cpu, pending_value)
#define PENDING_REG_OFFSET offsetof(struct test_cpu, pending_reg)
#define MULDIV_BUSY_OFFSET offsetof(struct test_cpu, muldiv_busy)
#define LOAD_DELAY_CLEAR(v) ((v).pending_reg == 0 && (v).pending_value == 0)
#endif
static int memory_mode;
static int allow_pending_exit;
static int full_address_mode;
static unsigned pending_load;
static int branch_pair;
static int runtime_exit_mode;
#ifdef ESP_PLATFORM
static int live_hardware_memory;
/* Emit the production PSX-RAM shortcut only for live blocks and its dedicated
 * hardware selftest. Private LUT fixtures otherwise deliberately own address
 * zero and need to keep using their synthetic mapping. */
static int direct_ram_mode;
/* Private selftest hook: never points at a live device handler. */
static uintptr_t test_memory_helper;
#ifdef PCSX_NDRC_RV32_LIVE_CANARY
#define LIVE_CANARY_RAM_PAGES (0x200000u / 4096u)
#define LIVE_CANARY_SOURCE_PAGES (LIVE_CANARY_RAM_PAGES + 1u)
static uint32_t live_canary_source_generation[LIVE_CANARY_SOURCE_PAGES];
static uint8_t live_canary_compiled_page[LIVE_CANARY_SOURCE_PAGES];
static void live_canary_invalidate_write(uint32_t address, unsigned bytes);

static void emit_live_canary_invalidate_write(unsigned address_reg,
                                              unsigned bytes)
{
  size_t ram, scratch_miss, page_ready, done;
  rv32_li(&rv32_output, RV_A0, 0x1fffffffu);
  rv32_alu(&rv32_output, RV_AND, RV_A0, address_reg, RV_A0);
  rv32_li(&rv32_output, RV_A1, 0x200000u);
  ram = rv32_output.count;
  rv32_branch(&rv32_output, RV_LTU, RV_A0, RV_A1, 0);
  /* Scratchpad is the only writable executable mapping outside main RAM. */
  rv32_shift(&rv32_output, RV_SRL, RV_A0, RV_A0, 12);
  rv32_li(&rv32_output, RV_A1, 0x1f800u);
  scratch_miss = rv32_output.count;
  rv32_branch(&rv32_output, RV_NE, RV_A0, RV_A1, 0);
  rv32_li(&rv32_output, RV_A0, LIVE_CANARY_RAM_PAGES);
  page_ready = rv32_output.count;
  rv32_jal(&rv32_output, RV_ZERO, 0);
  rv32_patch_relative(&rv32_output, ram,
    (int32_t)((rv32_output.count - ram) * 4));
  rv32_shift(&rv32_output, RV_SRL, RV_A0, RV_A0, 12);
  rv32_patch_relative(&rv32_output, page_ready,
    (int32_t)((rv32_output.count - page_ready) * 4));
  rv32_li(&rv32_output, RV_A2,
    (uint32_t)(uintptr_t)live_canary_compiled_page);
  rv32_alu(&rv32_output, RV_ADD, RV_A2, RV_A2, RV_A0);
  rv32_load(&rv32_output, RV_BYTE_U, RV_A2, RV_A2, 0);
  done = rv32_output.count;
  rv32_branch(&rv32_output, RV_EQ, RV_A2, RV_ZERO, 0);
  rv32_imm(&rv32_output, RV_ADD, RV_A0, address_reg, 0);
  rv32_li(&rv32_output, RV_A1, bytes);
  rv32_call_preserved(&rv32_output,
    (uint32_t)(uintptr_t)live_canary_invalidate_write, 0);
  rv32_patch_relative(&rv32_output, scratch_miss,
    (int32_t)((rv32_output.count - scratch_miss) * 4));
  rv32_patch_relative(&rv32_output, done,
    (int32_t)((rv32_output.count - done) * 4));
}
#else
static void live_canary_invalidate_write(uint32_t address, unsigned bytes)
{ (void)address; (void)bytes; }
#endif
#endif
static uint32_t branch_pc;
static uint32_t linear_pc;
static void *test_dispatcher;
static struct {
  size_t at;
  unsigned insn, status, pending, dirty;
  signed char regmap[HOST_REGS];
} overflow_stubs[TEST_INSNS * 2];
static unsigned stub_count;
static unsigned cached_sources, evictions, writebacks, folded_results;
static unsigned memory_temp_spills;
static unsigned memory_preserved_mappings, memory_mapped_sources;
static unsigned cycle_adds_saved;
static unsigned direct_ram_fastpaths;
static unsigned branch_preserved_mappings;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "../regmap_lookup.h"
#include "../regmap_access.h"
#include "../alloc_helpers.h"
#include "../alloc_regs.h"
static void alloc_cc_optional(struct regstat *cur, int i)
{
  (void)i;
  assert(cur->regmap[HOST_CCREG] == CCREG);
  cur->noevict |= get_regm(cur->regmap, CCREG);
}
#include "../alloc_alu.h"
#define Config fixture_config
#define psxHLEt fixture_psxHLEt
#include "../decode_one.h"
#undef psxHLEt
#undef Config
#pragma GCC diagnostic pop
static void emit_loadreg(unsigned guest, int host)
{
  if (!rv32_check(&rv32_output, guest < 32)) return;
  rv32_load(&rv32_output, RV_WORD, rv32_hr(host), RV_S0, (int32_t)(guest * 4));
}
static void emit_jo(const void *target)
{
  if (!rv32_check(&rv32_output, target == NULL)) return;
  /* Keep the conditional leg local, then use JAL's +/-1 MiB reach for the
   * out-of-line exception stub. Long live blocks can exceed a B-type branch's
   * +/-4 KiB range. add_stub_r() records the second instruction. */
  rv32_branch(&rv32_output, RV_EQ, RV_T5, RV_ZERO, 8);
  rv32_jal(&rv32_output, RV_ZERO, 0);
}
static void emit_jmp(const void *target)
{
  if (!rv32_check(&rv32_output, target == NULL)) return;
  rv32_jal(&rv32_output, RV_ZERO, 0);
}
static void add_stub_r(int type, void *addr, void *ret, int i, int a,
                       const struct regstat *mapped, int cc, int b)
{
  (void)ret; (void)a; (void)mapped; (void)cc; (void)b;
  if (!rv32_check(&rv32_output, type == OVERFLOW_STUB && stub_count < TEST_INSNS)) return;
  overflow_stubs[stub_count].at =
    (size_t)((uint32_t *)addr - rv32_output.code);
  /* Constant-folded overflow uses emit_jmp() directly. Runtime overflow uses
   * emit_jo()'s inverted branch followed by the patchable JAL. */
  if ((rv32_output.code[overflow_stubs[stub_count].at] & 0x7f) == 0x63)
    overflow_stubs[stub_count].at++;
  if (!rv32_check(&rv32_output,
      (rv32_output.code[overflow_stubs[stub_count].at] & 0x7f) == 0x6f))
    return;
  overflow_stubs[stub_count].status = 1;
  overflow_stubs[stub_count].pending = pending_load;
  /* The trapping-ALU barrier committed all earlier state. The current
   * instruction's dirty destination must not be published on overflow. */
  overflow_stubs[stub_count].dirty = 0;
  overflow_stubs[stub_count++].insn = (unsigned)i;
}
#pragma GCC diagnostic push
/* Keep upstream's signed host-slot conventions unchanged in the shared code. */
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "../assemble_alu.h"
#pragma GCC diagnostic pop
#undef out

static int memory_load_opcode(unsigned op)
{
  return op == 0x20 || op == 0x21 || op == 0x22 || op == 0x23 ||
    op == 0x24 || op == 0x25 || op == 0x26;
}

static int memory_store_opcode(unsigned op)
{
  return op == 0x28 || op == 0x29 || op == 0x2a || op == 0x2b ||
    op == 0x2e;
}

static int memory_unaligned_opcode(unsigned op)
{
  return op == 0x22 || op == 0x26 || op == 0x2a || op == 0x2e;
}

#ifdef ESP_PLATFORM
static int cop2_command_opcode(unsigned funct)
{
  switch (funct) {
  case 0x01: case 0x06: case 0x0c:
  case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x16:
  case 0x1b: case 0x1c: case 0x1e: case 0x20:
  case 0x28: case 0x29: case 0x2a: case 0x2d: case 0x2e: case 0x30:
  case 0x3d: case 0x3e: case 0x3f:
    return 1;
  default:
    return 0;
  }
}

static int cop2_transfer_opcode(uint32_t word)
{
  unsigned rs = (word >> 21) & 31;
  return (word >> 26) == 0x12 &&
    (rs == 0x00 || rs == 0x02 || rs == 0x04 || rs == 0x06);
}

static int cop2_word(uint32_t word)
{
  unsigned op = word >> 26;
  unsigned rs = (word >> 21) & 31;
  return op == 0x32 || op == 0x3a || cop2_transfer_opcode(word) ||
    (op == 0x12 && (rs & 0x10) && cop2_command_opcode(word & 63));
}

static int cop0_word(uint32_t word)
{
  unsigned rs = (word >> 21) & 31;
  unsigned rd = (word >> 11) & 31;
  if ((word >> 26) != 0x10) return 0;
  if (rs == 0x00) return !(0x00000417u & (1u << rd));
  if (rs == 0x04) return 1;
  return (rs & 0x10) && (word & 0x1f) == 0x10;
}

/* Status/Cause writes and RFE can vector immediately. Keep them at a native
 * block boundary so generated code never continues after an exception. */
static int cop0_control_word(uint32_t word)
{
  unsigned rs = (word >> 21) & 31;
  unsigned rd = (word >> 11) & 31;
  if (!cop0_word(word)) return 0;
  return (rs == 0x04 && (rd == 12 || rd == 13)) ||
    ((rs & 0x10) && (word & 0x1f) == 0x10);
}

static int delayed_load_word(uint32_t word)
{
  unsigned op = word >> 26;
  unsigned rs = (word >> 21) & 31;
  return memory_load_opcode(op) ||
    (op == 0x10 && rs == 0x00 && cop0_word(word)) ||
    (op == 0x12 && (rs == 0x00 || rs == 0x02));
}

/* Artificial straight-line cuts must not export a load delay merely because
 * the diagnostic reached its local prefix limit. End before the complete run
 * of terminal loads so the successor starts with the load and can compile its
 * consumer in the same block. Real unsupported boundaries still use the
 * existing pending-state bridge. */
static unsigned safe_capped_prefix(const uint32_t *words, unsigned count,
                                   int capped)
{
  if (capped)
    while (count && delayed_load_word(words[count - 1])) count--;
  return count;
}

static int safe_capped_prefix_selftest(void)
{
  const uint32_t words[] = {
    0x24210001u, /* ADDIU r1,r1,1 */
    0x8c220000u, /* LW r2,0(r1) */
    0x94430004u  /* LHU r3,4(r2) */
  };
  if (safe_capped_prefix(words, 3, 0) != 3) return __LINE__;
  if (safe_capped_prefix(words, 3, 1) != 1) return __LINE__;
  if (safe_capped_prefix(words, 1, 1) != 1) return __LINE__;
  return 0;
}
#endif

static int decode(unsigned i, uint32_t word)
{
  unsigned op = word >> 26, f = word & 63;
  if (!((memory_mode && (memory_load_opcode(op) ||
#ifdef ESP_PLATFORM
      cop0_word(word) ||
      cop2_word(word) ||
#endif
      memory_store_opcode(op))) || (op >= 8 && op <= 15) || (!op &&
      (f == 0 || f == 2 || f == 3 || f == 4 || f == 6 || f == 7 ||
       (f >= 0x10 && f <= 0x13) || (f >= 0x18 && f <= 0x1b) ||
       (f >= 0x20 && f <= 0x27) || f == 0x2a || f == 0x2b)))) return -1;
  disassemble_one(&work->state, (int)i, word);
#ifdef ESP_PLATFORM
  /* This standalone RV32 compiler is linked beside Lightrec, not beside the
   * mature new_dynarec emu_if.c which normally populates gte_handlers[]. The
   * shared decoder therefore classifies valid GTE commands as OTHER. Preserve
   * its field initialization, then supply the known-valid command class here. */
  if (op == 0x12 && (((word >> 21) & 0x10) != 0) &&
      cop2_command_opcode(f))
    dops[i].itype = C2OP;
#endif
  /* The shared pass1a normally supplies this exception classification. */
  dops[i].may_except = op == 8 || (!op && (f == 0x20 || f == 0x22));
  return dops[i].itype;
}

static void exit_code(unsigned pc, unsigned exception)
{
  void *target;
  if (runtime_exit_mode && branch_pair && !exception) {
    /* The branch adapter already stored the selected architectural PC. */
  }
  else if (branch_pair && !exception)
    rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0, offsetof(struct test_cpu, branch_target));
  else
    rv32_li(&rv32_output, RV_A0, branch_pair ? branch_pc : linear_pc + pc);
  if (!(runtime_exit_mode && branch_pair && !exception))
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0,
#ifdef ESP_PLATFORM
      offsetof(struct test_cpu, cpu.pc));
#else
      offsetof(struct test_cpu, pc));
#endif
  /* Test status 2 means overflow in a delay slot; pc then denotes branch EPC.
   * This is normalized test state, not live CP0 Cause/Status delivery. */
  rv32_li(&rv32_output, RV_A0, exception && branch_pair ? 2 : exception);
  if (!runtime_exit_mode)
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0, offsetof(struct test_cpu, exception));
  /* A live guard/fallback must return its nonzero status directly. The normal
   * dispatcher uses a0 for event/PC checks and deliberately returns zero, so
   * routing a guarded exit through it would hide the exact-PC fallback. */
  target = runtime_exit_mode && exception ? (void *)ndrc_rv32_leave :
    (test_dispatcher ? test_dispatcher : (void *)ndrc_rv32_leave);
  rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6,
    (uint32_t)(uintptr_t)target);
}

static void writeback(unsigned guest, unsigned host)
{
  if (!rv32_check(&rv32_output, guest > 0 && guest < 32 && host < HOST_REGS)) return;
  rv32_store(&rv32_output, RV_WORD, rv32_hr((int)host), RV_S0, (int32_t)(guest * 4));
  writebacks++;
}

static void flush_dirty(struct regstat *r)
{
  for (unsigned h = 0; h < HOST_REGS; h++)
    if ((r->dirty >> h) & 1u) writeback((unsigned)r->regmap[h], h);
  r->dirty = 0;
}

static void commit_load(unsigned guest)
{
  if (!guest) return;
  rv32_load(&rv32_output, RV_WORD, RV_A1, RV_S0, PENDING_VALUE_OFFSET);
  rv32_store(&rv32_output, RV_WORD, RV_A1, RV_S0, (int32_t)(guest * 4));
  rv32_store(&rv32_output, RV_BYTE, RV_ZERO, RV_S0, PENDING_REG_OFFSET);
  rv32_store(&rv32_output, RV_WORD, RV_ZERO, RV_S0, PENDING_VALUE_OFFSET);
}

static void commit_load_mapped(struct regstat *r, unsigned guest)
{
  int host;
  if (!guest) return;
  rv32_load(&rv32_output, RV_WORD, RV_A1, RV_S0, PENDING_VALUE_OFFSET);
  rv32_store(&rv32_output, RV_WORD, RV_A1, RV_S0, (int32_t)(guest * 4));
  host = get_reg(r->regmap, (signed char)guest);
  if (host >= 0) {
    if (rv32_hr(host) != RV_A1)
      rv32_imm(&rv32_output, RV_ADD, rv32_hr(host), RV_A1, 0);
    r->dirty &= ~(1u << host);
    r->isconst &= ~(1u << host);
    r->wasconst &= ~(1u << host);
  }
  rv32_store(&rv32_output, RV_BYTE, RV_ZERO, RV_S0, PENDING_REG_OFFSET);
  rv32_store(&rv32_output, RV_WORD, RV_ZERO, RV_S0, PENDING_VALUE_OFFSET);
}

static void cancel_load(void)
{
  rv32_store(&rv32_output, RV_BYTE, RV_ZERO, RV_S0, PENDING_REG_OFFSET);
  rv32_store(&rv32_output, RV_WORD, RV_ZERO, RV_S0, PENDING_VALUE_OFFSET);
}

static void reset_register_map(struct regstat *r)
{
  memset(r, 0, sizeof(*r));
  memset(r->regmap, -1, sizeof(r->regmap));
  memset(r->regmap_entry, -1, sizeof(r->regmap_entry));
  r->regmap[HOST_CCREG] = CCREG;
  r->noevict = 1u << HOST_CCREG;
  r->u = 1;
}

/* Pure non-trapping ALU instructions cannot observe the live cycle counter.
 * Accumulate their unit costs and materialize one ADDI immediately before a
 * helper, possible exception, or block exit. */
static void emit_deferred_cycles(unsigned *cycles)
{
  if (!*cycles) return;
  rv32_imm(&rv32_output, RV_ADD, RV_S1, RV_S1, (int32_t)*cycles);
  cycle_adds_saved += *cycles - 1;
  *cycles = 0;
}

/* Complete R3000A HI/LO family.  Treat these operations as register-allocation
 * barriers for now: this is less clever than the mature new_dynarec allocator,
 * but keeps canonical HI/LO and GPR state exact while removing an interpreter
 * boundary.  RV32IM supplies the actual multiply/divide operations. */
static void muldiv_schedule_busy(uint32_t word)
{
  unsigned f = word & 63;
  if (f == 0x1a || f == 0x1b) {
    rv32_imm(&rv32_output, RV_ADD, RV_A1, RV_S1, 37);
  }
  else {
    size_t keep_15, keep_11;
    /* The interpreter's variable MULT latency is 7/11/15 cycles according to
     * the significant width of rs. Reproduce its CLZ buckets with compares so
     * this remains valid on RV32IM without relying on the Zbb CLZ extension. */
    if (f == 0x18) {
      rv32_shift(&rv32_output, RV_SRA, RV_A2, RV_A0, 21);
      rv32_alu(&rv32_output, RV_XOR, RV_A2, RV_A0, RV_A2);
      rv32_imm(&rv32_output, RV_OR, RV_A2, RV_A2, 1);
    }
    else
      rv32_imm(&rv32_output, RV_OR, RV_A2, RV_A0, 1);
    rv32_imm(&rv32_output, RV_ADD, RV_A1, RV_S1, 15);
    rv32_li(&rv32_output, RV_A3, 1u << 21);
    keep_15 = rv32_output.count;
    rv32_branch(&rv32_output, RV_GEU, RV_A2, RV_A3, 0);
    rv32_imm(&rv32_output, RV_ADD, RV_A1, RV_S1, 11);
    rv32_li(&rv32_output, RV_A3, 1u << 10);
    keep_11 = rv32_output.count;
    rv32_branch(&rv32_output, RV_GEU, RV_A2, RV_A3, 0);
    rv32_imm(&rv32_output, RV_ADD, RV_A1, RV_S1, 7);
    rv32_patch_relative(&rv32_output, keep_15,
      (int32_t)((rv32_output.count - keep_15) * 4));
    rv32_patch_relative(&rv32_output, keep_11,
      (int32_t)((rv32_output.count - keep_11) * 4));
  }
  rv32_store(&rv32_output, RV_WORD, RV_A1, RV_S0, MULDIV_BUSY_OFFSET);
}

static void muldiv_wait_for_result(void)
{
  size_t ready;
  rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0, MULDIV_BUSY_OFFSET);
  rv32_alu(&rv32_output, RV_SUB, RV_A1, RV_A0, RV_S1);
  rv32_li(&rv32_output, RV_A2, 37);
  ready = rv32_output.count;
  rv32_branch(&rv32_output, RV_LTU, RV_A2, RV_A1, 0);
  rv32_imm(&rv32_output, RV_ADD, RV_S1, RV_A0, 0);
  rv32_patch_relative(&rv32_output, ready,
    (int32_t)((rv32_output.count - ready) * 4));
}

static void special_assemble(uint32_t word)
{
  unsigned f = word & 63;
  unsigned rs = (word >> 21) & 31;
  unsigned rt = (word >> 16) & 31;
  unsigned rd = (word >> 11) & 31;
  size_t nonzero, result_ready;

  if (f == 0x10 || f == 0x12) { /* MFHI/MFLO */
    muldiv_wait_for_result();
    if (rd) {
      rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0,
        (int32_t)((f == 0x10 ? HIREG : LOREG) * 4));
      rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0, (int32_t)(rd * 4));
    }
    return;
  }
  if (f == 0x11 || f == 0x13) { /* MTHI/MTLO */
    rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0, (int32_t)(rs * 4));
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0,
      (int32_t)((f == 0x11 ? HIREG : LOREG) * 4));
    return;
  }

  rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0, (int32_t)(rs * 4));
  rv32_load(&rv32_output, RV_WORD, RV_A1, RV_S0, (int32_t)(rt * 4));
  if (f == 0x18 || f == 0x19) { /* MULT/MULTU */
    rv32_alu(&rv32_output, RV_MUL, RV_A2, RV_A0, RV_A1);
    rv32_alu(&rv32_output, f == 0x18 ? RV_MULH : RV_MULHU,
      RV_A3, RV_A0, RV_A1);
  }
  else if (f == 0x1b) { /* DIVU */
    nonzero = rv32_output.count;
    rv32_branch(&rv32_output, RV_NE, RV_A1, RV_ZERO, 0);
    rv32_imm(&rv32_output, RV_ADD, RV_A2, RV_A0, 0);
    rv32_li(&rv32_output, RV_A3, UINT32_MAX);
    result_ready = rv32_output.count;
    rv32_jal(&rv32_output, RV_ZERO, 0);
    rv32_patch_relative(&rv32_output, nonzero,
      (int32_t)((rv32_output.count - nonzero) * 4));
    rv32_alu(&rv32_output, RV_REMU, RV_A2, RV_A0, RV_A1);
    rv32_alu(&rv32_output, RV_DIVU, RV_A3, RV_A0, RV_A1);
    rv32_patch_relative(&rv32_output, result_ready,
      (int32_t)((rv32_output.count - result_ready) * 4));
  }
  else { /* DIV */
    nonzero = rv32_output.count;
    rv32_branch(&rv32_output, RV_NE, RV_A1, RV_ZERO, 0);
    rv32_imm(&rv32_output, RV_ADD, RV_A2, RV_A0, 0);
    rv32_li(&rv32_output, RV_A3, UINT32_MAX);
    rv32_branch(&rv32_output, RV_GE, RV_A0, RV_ZERO, 8);
    rv32_li(&rv32_output, RV_A3, 1);
    result_ready = rv32_output.count;
    rv32_jal(&rv32_output, RV_ZERO, 0);
    rv32_patch_relative(&rv32_output, nonzero,
      (int32_t)((rv32_output.count - nonzero) * 4));
    rv32_alu(&rv32_output, RV_REM, RV_A2, RV_A0, RV_A1);
    rv32_alu(&rv32_output, RV_DIV, RV_A3, RV_A0, RV_A1);
    rv32_patch_relative(&rv32_output, result_ready,
      (int32_t)((rv32_output.count - result_ready) * 4));
  }
  rv32_store(&rv32_output, RV_WORD,
    f == 0x1a || f == 0x1b ? RV_A3 : RV_A2, RV_S0, LOREG * 4);
  rv32_store(&rv32_output, RV_WORD,
    f == 0x1a || f == 0x1b ? RV_A2 : RV_A3, RV_S0, HIREG * 4);
  muldiv_schedule_busy(word);
}

/* Memory lowering owns a0-a7, t0-t2, s2 and s3, and its slow path may call C.
 * Preserve mappings in s4-s11: these registers are ABI-preserved and are not
 * used as memory temporaries. This is the first production register-persistence
 * step; it avoids a complete architectural writeback/reload around every guest
 * load and store without relying on a fast-path-only assumption. */
static void prepare_memory_map(struct regstat *r)
{
  unsigned h;
  const unsigned clobbered = (1u << (NDRC_RV32_MEM_VALUE_SLOT + 1)) - 1u;
  if (r->dirty & clobbered) memory_temp_spills++;
  for (h = 0; h <= NDRC_RV32_MEM_VALUE_SLOT; h++) {
    if ((r->dirty >> h) & 1u)
      writeback((unsigned)r->regmap[h], h);
    r->regmap[h] = -1;
    r->regmap_entry[h] = -1;
  }
  for (; h < HOST_CCREG; h++)
    if (r->regmap[h] >= 0) memory_preserved_mappings++;
  r->dirty &= ~clobbered;
  r->isconst &= ~clobbered;
  r->wasconst &= ~clobbered;
  r->noevict = 1u << HOST_CCREG;
  r->u = 1;
}

/* A terminal branch and its delay slot are part of the same guest block, but
 * the first live compiler layout emits them into separate native regions.
 * Preserve allocator mappings held in callee-saved registers across that
 * internal jump. Caller-saved mappings must be published because the branch
 * adapter owns a0-a2 while selecting the target. This removes the old full
 * writeback/reload boundary without changing the externally visible block
 * entry or dispatcher contract. */
static void prepare_branch_map(struct regstat *r)
{
  for (unsigned h = 0; h < NDRC_RV32_FIRST_SAVED_SLOT; h++) {
    if ((r->dirty >> h) & 1u)
      writeback((unsigned)r->regmap[h], h);
    r->regmap[h] = -1;
    r->regmap_entry[h] = -1;
  }
  r->dirty &= ~(u_int)NDRC_RV32_CALLER_MASK;
  r->isconst &= ~(u_int)NDRC_RV32_CALLER_MASK;
  r->wasconst &= ~(u_int)NDRC_RV32_CALLER_MASK;
  for (unsigned h = NDRC_RV32_FIRST_SAVED_SLOT; h < HOST_CCREG; h++)
    if (r->regmap[h] >= 0) branch_preserved_mappings++;
  r->noevict = 1u << HOST_CCREG;
  r->u = 1;
}

/* A GTE command calls C but does not read or write the CPU GPRs. Spill only
 * caller-saved allocator slots before marshalling a0-a3 and retain mappings in
 * s2-s11 across the ABI call. COP2 transfers and LWC2/SWC2 use the older full
 * barrier because their helper semantics do access architectural GPR/memory
 * state. */
#ifdef ESP_PLATFORM
static void prepare_cop2_command_map(struct regstat *r)
{
  for (unsigned h = 0; h < NDRC_RV32_FIRST_SAVED_SLOT; h++) {
    if ((r->dirty >> h) & 1u)
      writeback((unsigned)r->regmap[h], h);
    r->regmap[h] = -1;
    r->regmap_entry[h] = -1;
  }
  r->dirty &= ~(u_int)NDRC_RV32_CALLER_MASK;
  r->isconst &= ~(u_int)NDRC_RV32_CALLER_MASK;
  r->wasconst &= ~(u_int)NDRC_RV32_CALLER_MASK;
  r->noevict = 1u << HOST_CCREG;
  r->u = 1;
}
#endif

static void emit_memory_source(const struct regstat *r, unsigned guest,
                               unsigned target)
{
  int host = get_reg(r->regmap, (signed char)guest);
  if (host >= 0) {
    memory_mapped_sources++;
    rv32_imm(&rv32_output, RV_ADD, target, rv32_hr(host), 0);
  }
  else
    rv32_load(&rv32_output, RV_WORD, target, RV_S0, (int32_t)(guest * 4));
}

static void memory_guard_status(unsigned i, unsigned cond, unsigned a,
                                unsigned b, unsigned status,
                                const struct regstat *r)
{
  if (!rv32_check(&rv32_output, stub_count < TEST_INSNS * 2)) return;
  /* Invert the local condition to skip a patchable JAL. The guard itself then
   * has fixed size and its out-of-line stub may be anywhere in this compiler's
   * bounded code buffer without relying on the short B-type branch range. */
  rv32_branch(&rv32_output, cond ^ 1u, a, b, 8);
  overflow_stubs[stub_count].at = rv32_output.count;
  overflow_stubs[stub_count].insn = i;
  overflow_stubs[stub_count].status = status;
  overflow_stubs[stub_count].pending = pending_load;
  overflow_stubs[stub_count].dirty = r->dirty;
  memcpy(overflow_stubs[stub_count].regmap, r->regmap,
    sizeof(overflow_stubs[stub_count].regmap));
  stub_count++;
  rv32_jal(&rv32_output, RV_ZERO, 0);
}

static void memory_guard(unsigned i, unsigned cond, unsigned a, unsigned b,
                         const struct regstat *r)
{
  int load = memory_load_opcode(dops[i].opcode);
  memory_guard_status(i, cond, a, b, load ? 3 : 4, r);
}

#ifdef ESP_PLATFORM
/* Diagnostic safety gate for private psxRegisters. Return zero before an
 * address could fall through psxMem* into live hardware handlers. */
static int ndrc_rv32_mapped_address(const psxRegisters *cpu, uint32_t address,
                                    unsigned bytes, unsigned write)
{
  uintptr_t *lut = write ? cpu->ptrs.memWLUT : cpu->ptrs.memRLUT;
  if (lut && lut[address >> PSXM_SHIFT] != INVALID_PTR_VAL) return 1;
  if (cpu->ptrs.psxH && address - 0x1f800000u <= 0x1000u - bytes) return 1;
  if (!write && cpu->ptrs.psxR && address - 0x1fc00000u <= 0x80000u - bytes) return 1;
  return 0;
}
#endif

/* LWL/LWR merge against a pending value from the immediately preceding load
 * when both target the same register. Keeping this semantic helper independent
 * of the generated sequence makes every byte alignment easy to differential
 * test. The normal RAM load/store family remains inline below. */
static uint32_t memory_lr_load_helper(struct test_cpu *state, uint32_t address,
                                      uint32_t old_value, uint32_t op)
{
  unsigned shift = (address & 3u) * 8u;
  uint32_t memory;
#ifdef ESP_PLATFORM
  memory = psxMemRead32(&state->cpu, address & ~3u);
#else
  memory = state->ram[(address & ~3u) / 4u];
#endif
  if (op == 0x22) {
    shift = 24u - shift;
    return (old_value & ~(UINT32_MAX << shift)) | (memory << shift);
  }
  return (old_value & ~(UINT32_MAX >> shift)) | (memory >> shift);
}

static void memory_lr_store_helper(struct test_cpu *state, uint32_t address,
                                   uint32_t value, uint32_t op)
{
#ifdef ESP_PLATFORM
  psxRegisters *cpu = &state->cpu;
  if (op == 0x2a) switch (address & 3u) {
    case 0: psxMemWrite8(cpu, address, value >> 24); break;
    case 1: psxMemWrite16(cpu, address & ~3u, value >> 16); break;
    case 2:
      psxMemWrite16(cpu, address & ~3u, (value >> 8) & 0xffffu);
      psxMemWrite8(cpu, address, value >> 24);
      break;
    default: psxMemWrite32(cpu, address & ~3u, value); break;
  }
  else switch (address & 3u) {
    case 0: psxMemWrite32(cpu, address, value); break;
    case 1:
      psxMemWrite8(cpu, address, value & 0xffu);
      psxMemWrite16(cpu, address + 1u, (value >> 8) & 0xffffu);
      break;
    case 2: psxMemWrite16(cpu, address, value & 0xffffu); break;
    default: psxMemWrite8(cpu, address, value & 0xffu); break;
  }
#else
  uint8_t *bytes = (uint8_t *)state->ram;
  if (op == 0x2a) {
    unsigned count = (address & 3u) + 1u;
    unsigned base = address & ~3u;
    for (unsigned i = 0; i < count; i++)
      bytes[base + i] = (uint8_t)(value >> ((4u - count + i) * 8u));
  }
  else {
    unsigned count = 4u - (address & 3u);
    for (unsigned i = 0; i < count; i++)
      bytes[address + i] = (uint8_t)(value >> (i * 8u));
  }
#endif
}

static void memory_lr_assemble(unsigned i, struct regstat *r)
{
  unsigned op = dops[i].opcode;
  int load = op == 0x22 || op == 0x26;
#ifdef ESP_PLATFORM
  const unsigned address_reg = rv32_mem_addr_reg();
  const unsigned value_reg = rv32_mem_value_reg();
#else
  const unsigned address_reg = RV_S2;
  const unsigned value_reg = RV_S3;
#endif

  emit_memory_source(r, dops[i].rs1, RV_A0);
  rv32_li(&rv32_output, RV_T6, (uint32_t)cinfo[i].imm);
  rv32_alu(&rv32_output, RV_ADD, address_reg, RV_A0, RV_T6);
  if (!full_address_mode) {
    rv32_li(&rv32_output, RV_A1, 256);
    memory_guard(i, RV_GEU, address_reg, RV_A1, r);
  }
  if (load) {
    if (pending_load == dops[i].rt1 && pending_load)
      rv32_load(&rv32_output, RV_WORD, value_reg, RV_S0,
        PENDING_VALUE_OFFSET);
    else
      emit_memory_source(r, dops[i].rt1, value_reg);
  }
  else
    emit_memory_source(r, dops[i].rs2, value_reg);

#ifdef ESP_PLATFORM
  if (full_address_mode) {
    /* These helpers may enter hardware handlers. Publish exactly the same
     * architectural state as the aligned slow path before calling C. */
    rv32_store(&rv32_output, RV_WORD, RV_S1, RV_S0,
      offsetof(struct test_cpu, cpu.cycle));
    if (runtime_exit_mode && branch_pair) {
      rv32_imm(&rv32_output, RV_ADD, RV_SP, RV_SP, -16);
      rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0,
        offsetof(struct test_cpu, cpu.pc));
      rv32_store(&rv32_output, RV_WORD, RV_A0, RV_SP, 0);
    }
    rv32_li(&rv32_output, RV_A0,
      branch_pair ? branch_pc + 4 : linear_pc + i * 4);
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0,
      offsetof(struct test_cpu, cpu.pc));
  }
#endif
  rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_S0, 0);
  rv32_imm(&rv32_output, RV_ADD, RV_A1, address_reg, 0);
  rv32_imm(&rv32_output, RV_ADD, RV_A2, value_reg, 0);
  rv32_li(&rv32_output, RV_A3, op);
  rv32_abs_jump(&rv32_output, RV_RA, RV_T6,
    (uint32_t)(load ? (uintptr_t)memory_lr_load_helper :
                      (uintptr_t)memory_lr_store_helper));
  if (load)
    rv32_imm(&rv32_output, RV_ADD, RV_A2, RV_A0, 0);
#ifdef ESP_PLATFORM
  if (full_address_mode) {
    rv32_load(&rv32_output, RV_WORD, RV_S1, RV_S0,
      offsetof(struct test_cpu, cpu.cycle));
    if (runtime_exit_mode && branch_pair) {
      rv32_load(&rv32_output, RV_WORD, RV_A0, RV_SP, 0);
      rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0,
        offsetof(struct test_cpu, cpu.pc));
      rv32_imm(&rv32_output, RV_ADD, RV_SP, RV_SP, 16);
    }
    if (!load && runtime_exit_mode && live_hardware_memory) {
      rv32_imm(&rv32_output, RV_AND, address_reg, address_reg, -4);
      emit_live_canary_invalidate_write(address_reg, 4);
    }
  }
#endif

  if (!load || dops[i].rt1 != pending_load)
    commit_load_mapped(r, pending_load);
  pending_load = load ? dops[i].rt1 : 0;
  if (pending_load) {
    rv32_store(&rv32_output, RV_WORD, RV_A2, RV_S0,
      PENDING_VALUE_OFFSET);
    rv32_li(&rv32_output, RV_A1, pending_load);
    rv32_store(&rv32_output, RV_BYTE, RV_A1, RV_S0, PENDING_REG_OFFSET);
  }
}

/* Integer memory lowering: operand capture precedes the older load's commit.
 * Alignment/range guards precede host memory access. LWL/LWR/SWL/SWR use the
 * semantic helpers above initially; aligned RAM transfers retain their inline
 * LUT fast path. */
static void memory_assemble(unsigned i, struct regstat *r)
{
  unsigned op = dops[i].opcode;
  int load = memory_load_opcode(op);
  if (memory_unaligned_opcode(op)) {
    memory_lr_assemble(i, r);
    return;
  }
  unsigned width = (op == 0x20 || op == 0x24 || op == 0x28) ? RV_BYTE :
    (op == 0x21 || op == 0x25 || op == 0x29) ? RV_HALF : RV_WORD;
  unsigned load_width = op == 0x24 ? RV_BYTE_U : op == 0x25 ? RV_HALF_U : width;
  unsigned align_mask = width == RV_WORD ? 3 : width == RV_HALF ? 1 : 0;
  unsigned limit = width == RV_WORD ? 253 : width == RV_HALF ? 255 : 256;
#ifdef ESP_PLATFORM
  (void)load_width;
#endif
  emit_memory_source(r, dops[i].rs1, RV_A0);
  rv32_li(&rv32_output, RV_T6, (uint32_t)cinfo[i].imm);
  rv32_alu(&rv32_output, RV_ADD, RV_A0, RV_A0, RV_T6);
  if (align_mask) {
    rv32_imm(&rv32_output, RV_AND, RV_A1, RV_A0, (int32_t)align_mask);
    memory_guard(i, RV_NE, RV_A1, RV_ZERO, r);
  }
  if (!full_address_mode) {
    rv32_li(&rv32_output, RV_A1, limit);
    memory_guard(i, RV_GEU, RV_A0, RV_A1, r);
  }
#ifdef ESP_PLATFORM
  const unsigned addr_tmp = rv32_mem_addr_reg();
  const unsigned value_tmp = rv32_mem_value_reg();
  uintptr_t helper = load ?
    (width == RV_BYTE ? (uintptr_t)psxMemRead8 :
     width == RV_HALF ? (uintptr_t)psxMemRead16 : (uintptr_t)psxMemRead32) :
    (width == RV_BYTE ? (uintptr_t)psxMemWrite8 :
     width == RV_HALF ? (uintptr_t)psxMemWrite16 : (uintptr_t)psxMemWrite32);
  if (test_memory_helper) helper = test_memory_helper;
  if (!load) {
    emit_memory_source(r, dops[i].rs2, RV_A2);
    if (full_address_mode) rv32_imm(&rv32_output, RV_ADD, value_tmp, RV_A2, 0);
  }
  if (full_address_mode) {
    size_t slow, done = 0, ram_0080 = 0, ram_positive = 0, ram_a000 = 0;
    rv32_imm(&rv32_output, RV_ADD, addr_tmp, RV_A0, 0); /* guest address */
    if (direct_ram_mode) {
      /* Main RAM is mirrored through 00000000-007fffff,
       * 80000000-807fffff and a0000000-a07fffff. The first shift folds the
       * first two aliases together without accepting the nonexistent
       * 20000000 mirror; the signed guard distinguishes that range from a0.
       * The final host offset wraps every 2 MiB exactly like lutMap(). */
      rv32_shift(&rv32_output, RV_SLL, RV_A3, addr_tmp, 1);
      rv32_li(&rv32_output, RV_A1, 0x01000000u);
      ram_0080 = rv32_output.count;
      rv32_branch(&rv32_output, RV_LTU, RV_A3, RV_A1, 0);
      ram_positive = rv32_output.count;
      rv32_branch(&rv32_output, RV_GE, addr_tmp, RV_ZERO, 0);
      rv32_li(&rv32_output, RV_A4, 0x40000000u);
      rv32_alu(&rv32_output, RV_SUB, RV_A3, RV_A3, RV_A4);
      ram_a000 = rv32_output.count;
      rv32_branch(&rv32_output, RV_GEU, RV_A3, RV_A1, 0);
      rv32_patch_relative(&rv32_output, ram_0080,
        (int32_t)((rv32_output.count - ram_0080) * 4));
      rv32_load(&rv32_output, RV_WORD, RV_A1, RV_S0,
        offsetof(struct test_cpu, cpu.ptrs.psxM));
      rv32_shift(&rv32_output, RV_SLL, RV_A3, addr_tmp, 11);
      rv32_shift(&rv32_output, RV_SRL, RV_A3, RV_A3, 11);
      rv32_alu(&rv32_output, RV_ADD, RV_A1, RV_A1, RV_A3);
      if (load) rv32_load(&rv32_output, load_width, RV_A2, RV_A1, 0);
      else rv32_store(&rv32_output, width, value_tmp, RV_A1, 0);
      done = rv32_output.count;
      rv32_jal(&rv32_output, RV_ZERO, 0);
      rv32_patch_relative(&rv32_output, ram_positive,
        (int32_t)((rv32_output.count - ram_positive) * 4));
      rv32_patch_relative(&rv32_output, ram_a000,
        (int32_t)((rv32_output.count - ram_a000) * 4));
      direct_ram_fastpaths++;
    }
    rv32_load(&rv32_output, RV_WORD, RV_A1, RV_S0,
      load ? offsetof(struct test_cpu, cpu.ptrs.memRLUT) :
             offsetof(struct test_cpu, cpu.ptrs.memWLUT));
    rv32_shift(&rv32_output, RV_SRL, RV_A3, addr_tmp, PSXM_SHIFT);
    rv32_shift(&rv32_output, RV_SLL, RV_A3, RV_A3, 2);
    rv32_alu(&rv32_output, RV_ADD, RV_A1, RV_A1, RV_A3);
    rv32_load(&rv32_output, RV_WORD, RV_A1, RV_A1, 0);
    rv32_li(&rv32_output, RV_A3, INVALID_PTR_VAL);
    slow = rv32_output.count;
    rv32_branch(&rv32_output, RV_EQ, RV_A1, RV_A3, 0);
    rv32_alu(&rv32_output, RV_ADD, RV_A1, RV_A1, addr_tmp);
    if (load) rv32_load(&rv32_output, load_width, RV_A2, RV_A1, 0);
    else rv32_store(&rv32_output, width, value_tmp, RV_A1, 0);
    size_t lut_done = rv32_output.count;
    rv32_jal(&rv32_output, RV_ZERO, 0);
    rv32_patch_relative(&rv32_output, slow,
      (int32_t)((rv32_output.count - slow) * 4));
    if (!live_hardware_memory) {
      /* Synthetic clone tests must not reach live hardware handlers. Invalid
       * LUT entries may still be their private scratchpad/BIOS mappings. */
      rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_S0, 0);
      rv32_imm(&rv32_output, RV_ADD, RV_A1, addr_tmp, 0);
      rv32_li(&rv32_output, RV_A2,
        width == RV_WORD ? 4 : width == RV_HALF ? 2 : 1);
      rv32_li(&rv32_output, RV_A3, !load);
      rv32_abs_jump(&rv32_output, RV_RA, RV_T6,
        (uint32_t)(uintptr_t)ndrc_rv32_mapped_address);
      memory_guard_status(i, RV_EQ, RV_A0, RV_ZERO, 5, r);
    }
    /* A real MMIO helper observes global PCSX timing/state. Publish the live
     * generated cycle and exact memory-instruction PC before entering C, then
     * accept any cycle adjustment made by the handler. The address and store
     * value remain in ABI-preserved s2/s3 across the call. */
    rv32_store(&rv32_output, RV_WORD, RV_S1, RV_S0,
      offsetof(struct test_cpu, cpu.cycle));
    if (runtime_exit_mode && branch_pair) {
      /* cpu.pc holds the already selected branch destination. Keep it across
       * publication of the delay-slot PC, using an ABI-aligned private slot. */
      rv32_imm(&rv32_output, RV_ADD, RV_SP, RV_SP, -16);
      rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0,
        offsetof(struct test_cpu, cpu.pc));
      rv32_store(&rv32_output, RV_WORD, RV_A0, RV_SP, 0);
    }
    rv32_li(&rv32_output, RV_A0,
      branch_pair ? branch_pc + 4 : linear_pc + i * 4);
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0,
      offsetof(struct test_cpu, cpu.pc));
    rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_S0, 0);
    rv32_imm(&rv32_output, RV_ADD, RV_A1, addr_tmp, 0);
    if (!load) rv32_imm(&rv32_output, RV_ADD, RV_A2, value_tmp, 0);
    rv32_abs_jump(&rv32_output, RV_RA, RV_T6, (uint32_t)helper);
    if (load) rv32_imm(&rv32_output, RV_ADD, RV_A2, RV_A0, 0);
    rv32_load(&rv32_output, RV_WORD, RV_S1, RV_S0,
      offsetof(struct test_cpu, cpu.cycle));
    if (runtime_exit_mode && branch_pair) {
      rv32_load(&rv32_output, RV_WORD, RV_A0, RV_SP, 0);
      rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0,
        offsetof(struct test_cpu, cpu.pc));
      rv32_imm(&rv32_output, RV_ADD, RV_SP, RV_SP, 16);
    }
    rv32_patch_relative(&rv32_output, lut_done,
      (int32_t)((rv32_output.count - lut_done) * 4));
    if (direct_ram_mode)
      rv32_patch_relative(&rv32_output, done,
        (int32_t)((rv32_output.count - done) * 4));
    if (!load && runtime_exit_mode && live_hardware_memory) {
      /* Direct LUT stores bypass psxMemWrite*. Update the source-page epoch
       * inline for both direct and slow paths; hardware addresses fall through
       * without another C helper call. Aligned stores cannot cross a page. */
      emit_live_canary_invalidate_write(addr_tmp,
        width == RV_WORD ? 4 : width == RV_HALF ? 2 : 1);
    }
  }
  else {
    rv32_imm(&rv32_output, RV_ADD, RV_A1, RV_A0, 0);
    rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_S0, 0);
    rv32_abs_jump(&rv32_output, RV_RA, RV_T6, (uint32_t)helper);
    if (load) rv32_imm(&rv32_output, RV_ADD, RV_A2, RV_A0, 0);
  }
  if (load) {
    if (op == 0x20) {
      rv32_shift(&rv32_output, RV_SLL, RV_A2, RV_A2, 24);
      rv32_shift(&rv32_output, RV_SRA, RV_A2, RV_A2, 24);
    }
    else if (op == 0x21) {
      rv32_shift(&rv32_output, RV_SLL, RV_A2, RV_A2, 16);
      rv32_shift(&rv32_output, RV_SRA, RV_A2, RV_A2, 16);
    }
  }
#else
  rv32_alu(&rv32_output, RV_ADD, RV_A0, RV_A0, RV_S0);
  if (load)
    rv32_load(&rv32_output, load_width, RV_A2, RV_A0, offsetof(struct test_cpu, ram));
  else {
    emit_memory_source(r, dops[i].rs2, RV_A2);
    rv32_store(&rv32_output, width, RV_A2, RV_A0, offsetof(struct test_cpu, ram));
  }
#endif
  /* A successful new load to the same register cancels the older load.
   * Fault stubs still commit it, since the new load never executed. */
  if (!load || dops[i].rt1 != pending_load) commit_load_mapped(r, pending_load);
  pending_load = load ? dops[i].rt1 : 0;
  if (pending_load) {
    rv32_store(&rv32_output, RV_WORD, RV_A2, RV_S0, PENDING_VALUE_OFFSET);
    rv32_li(&rv32_output, RV_A1, pending_load);
    rv32_store(&rv32_output, RV_BYTE, RV_A1, RV_S0, PENDING_REG_OFFSET);
  }
}

#ifdef ESP_PLATFORM
/* Complete the stateful COP0 operations through the emulator's established
 * implementation. MFC0 is emitted directly below; this helper owns MTC0's
 * cache-isolation/coprocessor/interrupt side effects and RFE's exception
 * transition. It reports whether control left the expected sequential PC. */
static uint32_t cop0_semantic_helper(struct test_cpu *state, uint32_t word,
                                    uint32_t expected_pc)
{
  psxRegisters *cpu = &state->cpu;
  unsigned rs = (word >> 21) & 31;
  unsigned rt = (word >> 16) & 31;
  unsigned rd = (word >> 11) & 31;

  cpu->pc = expected_pc;
  cpu->code = word;
  if (rs == 0x04) {
    MTC0(cpu, (int)rd, cpu->GPR.r[rt]);
    if (rd == 12 || rd == 13) gen_interupt(&cpu->CP0);
  }
  else {
    cpu->CP0.n.SR = (cpu->CP0.n.SR & ~0x0fu) |
      ((cpu->CP0.n.SR & 0x3cu) >> 2);
    if ((cpu->CP0.n.Cause & cpu->CP0.n.SR & 0x0300u) &&
        (cpu->CP0.n.SR & 1u)) {
      cpu->CP0.n.Cause &= ~0x7cu;
      psxException(cpu->CP0.n.Cause, R3000A_BRANCH_NONE_OR_EXCEPTION,
        &cpu->CP0);
      cpu->branching = R3000A_BRANCH_NONE_OR_EXCEPTION;
    }
  }
  return cpu->pc != expected_pc;
}

static void cop0_assemble(unsigned i, struct regstat *r, uint32_t word)
{
  unsigned rs = (word >> 21) & 31;
  unsigned rt = (word >> 16) & 31;
  unsigned rd = (word >> 11) & 31;

  flush_dirty(r);
  rv32_imm(&rv32_output, RV_ADD, RV_S1, RV_S1, 1);
  if (rs == 0x00) {
    rv32_load(&rv32_output, RV_WORD, RV_A2, RV_S0,
      offsetof(struct test_cpu, cpu.CP0.r) + (int32_t)(rd * 4));
    if (pending_load) {
      if (rt == pending_load) cancel_load();
      else commit_load(pending_load);
    }
    pending_load = rt;
    if (pending_load) {
      rv32_store(&rv32_output, RV_WORD, RV_A2, RV_S0,
        PENDING_VALUE_OFFSET);
      rv32_li(&rv32_output, RV_A1, pending_load);
      rv32_store(&rv32_output, RV_BYTE, RV_A1, RV_S0,
        PENDING_REG_OFFSET);
    }
    reset_register_map(r);
    return;
  }

  /* A previous load must remain delayed while MTC0 reads its old GPR value.
   * Control-changing COP0 operations are split before compilation so their
   * exception paths never have to reconcile an in-flight generated load. */
  if (!rv32_check(&rv32_output,
      !cop0_control_word(word) || pending_load == 0)) return;
  rv32_store(&rv32_output, RV_WORD, RV_S1, RV_S0,
    offsetof(struct test_cpu, cpu.cycle));
  rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_S0, 0);
  rv32_li(&rv32_output, RV_A1, word);
  rv32_li(&rv32_output, RV_A2, linear_pc + i * 4 + 4);
  rv32_abs_jump(&rv32_output, RV_RA, RV_T6,
    (uint32_t)(uintptr_t)cop0_semantic_helper);
  rv32_load(&rv32_output, RV_WORD, RV_S1, RV_S0,
    offsetof(struct test_cpu, cpu.cycle));
  if (cop0_control_word(word)) {
    size_t sequential = rv32_output.count;
    rv32_branch(&rv32_output, RV_EQ, RV_A0, RV_ZERO, 0);
    rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_ZERO, 0);
    rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6,
      (uint32_t)(uintptr_t)ndrc_rv32_leave);
    rv32_patch_relative(&rv32_output, sequential,
      (int32_t)((rv32_output.count - sequential) * 4));
  }
  if (pending_load) commit_load(pending_load);
  pending_load = 0;
  reset_register_map(r);
}

/* Execute one complete COP2 operation without re-entering the instruction
 * interpreter. The established GTE implementation remains the semantic
 * engine, but fetch/decode/dispatch, CPU register transfers and block handoff
 * stay in generated RV32 code. This deliberately covers the whole valid PSX
 * COP2 command set at once; individual hot GTE functions can be inlined or
 * specialized after the coverage profile settles. */
/* Omit the parameter name: new_dynarec.h defines `regs` as the compile-time
 * register-history shorthand in this translation unit. */
typedef void (*rv32_gte_handler_t)(struct psxCP2Regs *);

static const rv32_gte_handler_t rv32_gte_no_flags[64] = {
  [0x01] = gteRTPS_nf,  [0x06] = gteNCLIP_nf,
  [0x0c] = gteOP_nf,    [0x10] = gteDPCS_nf,
  [0x11] = gteINTPL_nf, [0x12] = gteMVMVA_nf,
  [0x13] = gteNCDS_nf,  [0x14] = gteCDP_nf,
  [0x16] = gteNCDT_nf,  [0x1b] = gteNCCS_nf,
  [0x1c] = gteCC_nf,    [0x1e] = gteNCS_nf,
  [0x20] = gteNCT_nf,   [0x28] = gteSQR_nf,
  [0x29] = gteDCPL_nf,  [0x2a] = gteDPCT_nf,
  [0x2d] = gteAVSZ3_nf, [0x2e] = gteAVSZ4_nf,
  [0x30] = gteRTPT_nf,  [0x3d] = gteGPF_nf,
  [0x3e] = gteGPL_nf,   [0x3f] = gteNCCT_nf,
};

#if defined(CONFIG_IDF_TARGET_ESP32P4)
static const rv32_gte_handler_t rv32_intpl[4] = {
  gteINTPL_sf0_lm0, gteINTPL_sf0_lm1,
  gteINTPL_sf1_lm0, gteINTPL_sf1_lm1,
};
static const rv32_gte_handler_t rv32_intpl_no_flags[4] = {
  gteINTPL_sf0_lm0_nf, gteINTPL_sf0_lm1_nf,
  gteINTPL_sf1_lm0_nf, gteINTPL_sf1_lm1_nf,
};
#endif

static struct {
  uint64_t sampled_cycles;
  uint32_t calls, no_flags_calls, sampled_calls, phase;
} rv32_gte_profile;

/* Every command overwrites FLAG. A command may use the no-FLAG handler only
 * when another command or an explicit CTC2 FLAG write occurs before any CFC2
 * FLAG read in this same straight-line prefix. At a native block boundary the
 * architectural FLAG remains observable, so the final producer stays full. */
static int cop2_flags_unneeded(const uint32_t *words, unsigned count,
                               unsigned producer)
{
  for (unsigned i = producer + 1; i < count; i++) {
    uint32_t word = words[i];
    unsigned op = word >> 26;
    unsigned rs = (word >> 21) & 31;
    unsigned rd = (word >> 11) & 31;
    if (op != 0x12) continue;
    if (rs == 0x02 && rd == 31) return 0; /* CFC2 FLAG */
    if ((rs == 0x06 && rd == 31) ||
        ((rs & 0x10) && cop2_command_opcode(word & 63))) return 1;
  }
  return 0;
}

static uint32_t cop2_semantic_helper(struct test_cpu *state, uint32_t word,
                                    uint32_t address, uint32_t no_flags)
{
  psxRegisters *cpu = &state->cpu;
  unsigned op = word >> 26;
  unsigned rs = (word >> 21) & 31;
  unsigned rt = (word >> 16) & 31;
  unsigned rd = (word >> 11) & 31;
  unsigned funct = word & 63;
  rv32_gte_handler_t handler;
  unsigned gte_cycles = op == 0x12 && (rs & 0x10) ?
    gte_cycletab[funct] : 0;

  if (!Config.DisableStalls) {
    uint32_t left = cpu->gteBusyCycle - cpu->cycle;
    if (left <= 44) cpu->cycle = cpu->gteBusyCycle;
    cpu->gteBusyCycle = cpu->cycle + gte_cycles;
  }

  if (op == 0x32) {
    MTC2(&cpu->CP2, psxMemRead32(cpu, address), rt);
    return 0;
  }
  if (op == 0x3a) {
    psxMemWrite32(cpu, address, MFC2(&cpu->CP2, rt));
    return 0;
  }
  switch (rs) {
  case 0x00: return MFC2(&cpu->CP2, rd);
  case 0x02: return cpu->CP2C.r[rd];
  case 0x04: MTC2(&cpu->CP2, cpu->GPR.r[rt], rd); return 0;
  case 0x06: CTC2(&cpu->CP2, cpu->GPR.r[rt], rd); return 0;
  default:
    /* GTE operations derive SF/LM/MX/V/CV from psxRegs.code. Live execution
     * passes the canonical global register object, as both existing cores do.
     * Preserve the global word around private startup differential tests. */
    {
      uint32_t saved_code = psxRegs.code;
      psxRegs.code = word;
      cpu->code = word;
      handler = no_flags ? rv32_gte_no_flags[funct] : NULL;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
      if (funct == 0x11) {
        unsigned variant = ((word >> 18) & 2) | ((word >> 10) & 1);
        handler = no_flags ? rv32_intpl_no_flags[variant] :
          rv32_intpl[variant];
      }
#endif
      if (!handler) handler = psxCP2[funct];
      {
        uint32_t started = 0;
        int sample = (++rv32_gte_profile.phase & 15u) == 0;
        if (sample) __asm__ volatile("csrr %0, mcycle" : "=r"(started));
        handler(&cpu->CP2);
        if (sample) {
          uint32_t finished;
          __asm__ volatile("csrr %0, mcycle" : "=r"(finished));
          rv32_gte_profile.sampled_cycles += finished - started;
          rv32_gte_profile.sampled_calls++;
        }
      }
      rv32_gte_profile.calls++;
      rv32_gte_profile.no_flags_calls += no_flags != 0;
      if (cpu != &psxRegs) psxRegs.code = saved_code;
    }
    return 0;
  }
}

static void cop2_assemble(unsigned i, struct regstat *r, uint32_t word,
                          int no_flags)
{
  unsigned op, rs, rt;
  int command;
  op = word >> 26;
  rs = (word >> 21) & 31;
  rt = (word >> 16) & 31;
  command = op == 0x12 && (rs & 0x10);

  if (command) prepare_cop2_command_map(r);
  else flush_dirty(r);

  /* COP2 transfers/commands are conditional on CU2. LWC2/SWC2 follow the
   * interpreter's existing mapping and are admitted independently. A miss is
   * replayed at this exact PC so the interpreter delivers the architectural
   * coprocessor-unusable exception. */
  if (op == 0x12) {
    rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0,
      offsetof(struct test_cpu, cpu.CP0.n.SR));
    rv32_li(&rv32_output, RV_A1, 1u << 30);
    rv32_alu(&rv32_output, RV_AND, RV_A0, RV_A0, RV_A1);
    memory_guard_status(i, RV_EQ, RV_A0, RV_ZERO, 5, r);
  }

  if (op == 0x32 || op == 0x3a) {
    rv32_load(&rv32_output, RV_WORD, RV_A2, RV_S0,
      (int32_t)(((word >> 21) & 31) * 4));
    rv32_li(&rv32_output, RV_A3, (uint32_t)(int32_t)(int16_t)word);
    rv32_alu(&rv32_output, RV_ADD, RV_A2, RV_A2, RV_A3);
    rv32_imm(&rv32_output, RV_AND, RV_A0, RV_A2, 3);
    memory_guard_status(i, RV_NE, RV_A0, RV_ZERO,
      op == 0x32 ? 3 : 4, r);
  }
  else
    rv32_imm(&rv32_output, RV_ADD, RV_A2, RV_ZERO, 0);

  rv32_store(&rv32_output, RV_WORD, RV_S1, RV_S0,
    offsetof(struct test_cpu, cpu.cycle));
  if (runtime_exit_mode && branch_pair && (op == 0x32 || op == 0x3a)) {
    rv32_imm(&rv32_output, RV_ADD, RV_SP, RV_SP, -16);
    rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0,
      offsetof(struct test_cpu, cpu.pc));
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_SP, 0);
  }
  if (op == 0x32 || op == 0x3a) {
    rv32_li(&rv32_output, RV_A0,
      branch_pair ? branch_pc + 4 : linear_pc + i * 4);
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0,
      offsetof(struct test_cpu, cpu.pc));
  }
  rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_S0, 0);
  rv32_li(&rv32_output, RV_A1, word);
  rv32_li(&rv32_output, RV_A3, (uint32_t)no_flags);
  rv32_abs_jump(&rv32_output, RV_RA, RV_T6,
    (uint32_t)(uintptr_t)cop2_semantic_helper);
  rv32_load(&rv32_output, RV_WORD, RV_S1, RV_S0,
    offsetof(struct test_cpu, cpu.cycle));
  if (runtime_exit_mode && branch_pair && (op == 0x32 || op == 0x3a)) {
    rv32_load(&rv32_output, RV_WORD, RV_A1, RV_SP, 0);
    rv32_store(&rv32_output, RV_WORD, RV_A1, RV_S0,
      offsetof(struct test_cpu, cpu.pc));
    rv32_imm(&rv32_output, RV_ADD, RV_SP, RV_SP, 16);
  }

  if (pending_load) {
    if (op == 0x12 && (rs == 0x00 || rs == 0x02) && rt == pending_load)
      cancel_load();
    else if (command)
      commit_load_mapped(r, pending_load);
    else
      commit_load(pending_load);
  }
  pending_load = op == 0x12 && (rs == 0x00 || rs == 0x02) ? rt : 0;
  if (pending_load) {
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0, PENDING_VALUE_OFFSET);
    rv32_li(&rv32_output, RV_A1, pending_load);
    rv32_store(&rv32_output, RV_BYTE, RV_A1, RV_S0, PENDING_REG_OFFSET);
  }
  if (!command) reset_register_map(r);
}
#endif

static int compile_mapped(void *code, size_t size, const uint32_t *words,
                          unsigned count, const struct regstat *initial_map,
                          struct regstat *final_map, int preserve_final)
{
  unsigned i, h, deferred_cycles = 0;
  struct regstat r;
  if (!count || count > TEST_INSNS) return -1;
  /* Pending loads across block boundaries are not wired into dispatch yet. */
  unsigned last_op = words[count - 1] >> 26;
  if (memory_mode && !allow_pending_exit &&
#ifdef ESP_PLATFORM
      delayed_load_word(words[count - 1])) return -1;
#else
      memory_load_opcode(last_op)) return -1;
#endif
  pending_load = 0;
  rv32_init(&rv32_output, code, size); stub_count = 0;
  cached_sources = evictions = writebacks = folded_results = 0;
  memory_temp_spills = memory_preserved_mappings = memory_mapped_sources = 0;
  cycle_adds_saved = direct_ram_fastpaths = 0;
  branch_preserved_mappings = 0;
  work->state.start = 0; work->state.slen = (int)count;
  memset(&dops[count], 0, sizeof(dops[count]));
  memset(&cinfo[count], 0, sizeof(cinfo[count]));
  work->state.unneeded_reg[count] = 1;
  for (i = 0; i < count; i++) {
    if (decode(i, words[i]) < 0) return -1;
    work->state.unneeded_reg[i] = 1; /* Conservatively keep all nonzero GPRs live. */
  }
  if (initial_map)
    r = *initial_map;
  else {
    memset(&r, 0, sizeof(r));
    memset(r.regmap, -1, sizeof(r.regmap));
    r.regmap[HOST_CCREG] = CCREG; r.u = 1;
  }
  for (i = 0; i < count; i++) {
    int kind = dops[i].itype;
#ifdef ESP_PLATFORM
    if (kind == COP0 || kind == RFE) {
      if (cop0_control_word(words[i]) &&
          (pending_load || i + 1 != count)) return -1;
      emit_deferred_cycles(&deferred_cycles);
      cop0_assemble(i, &r, words[i]);
      regs[i] = r;
      continue;
    }
    if (kind == COP2 || kind == C2LS || kind == C2OP) {
      emit_deferred_cycles(&deferred_cycles);
      rv32_imm(&rv32_output, RV_ADD, RV_S1, RV_S1, 1);
      cop2_assemble(i, &r, words[i],
        kind == C2OP && cop2_flags_unneeded(words, count, i));
      regs[i] = r;
      continue;
    }
#endif
    if (kind == LOAD || kind == STORE || kind == LOADLR || kind == STORELR) {
      emit_deferred_cycles(&deferred_cycles);
      prepare_memory_map(&r);
      if (r.regmap[NDRC_RV32_MEM_ADDR_SLOT] != -1 ||
          r.regmap[NDRC_RV32_MEM_VALUE_SLOT] != -1)
        return -1;
      rv32_imm(&rv32_output, RV_ADD, RV_S1, RV_S1, 1);
      memory_assemble(i, &r);
      regs[i] = r;
      continue;
    }
    if (kind == MOV || kind == MULTDIV) {
      /* HI/LO are canonical-state barriers in this first complete lowering.
       * Flush mapped inputs, perform the operation directly, then discard the
       * now-stale map. Cross-instruction allocation can be restored later. */
      emit_deferred_cycles(&deferred_cycles);
      flush_dirty(&r);
      rv32_imm(&rv32_output, RV_ADD, RV_S1, RV_S1, 1);
      special_assemble(words[i]);
      if (pending_load) {
        if (kind == MOV && dops[i].rt1 == pending_load) cancel_load();
        else commit_load(pending_load);
        pending_load = 0;
      }
      reset_register_map(&r);
      regs[i] = r;
      continue;
    }
    /* Conservative exception barrier: prior results reach memory before any
     * potentially trapping instruction can overwrite an architectural value.
     * This is not yet the production per-stub dirty-state reconstruction. */
    if (dops[i].may_except) {
      emit_deferred_cycles(&deferred_cycles);
      flush_dirty(&r);
      r.isconst = 0; /* Never fold away an overflow check. */
    }
    unsigned old_dirty = r.dirty;
    for (h = 0; h < HOST_REGS; h++) r.regmap_entry[h] = r.regmap[h];
    r.noevict = 1u << HOST_CCREG;
    r.wasconst = 0; /* Known values are materialized, not delayed constants. */
    /* ADD/SUB r0 still trap: this restricted driver must pin their inputs even
     * without a destination. The shared ALU allocator only pins them for rt1. */
    if (kind == ALU && dops[i].may_except && !dops[i].rt1) {
      alloc_reg(&work->state, &r, (int)i, (signed char)dops[i].rs1);
      alloc_reg(&work->state, &r, (int)i, (signed char)dops[i].rs2);
    }
    switch (kind) {
    case ALU: alu_alloc(&work->state, &r, (int)i); break;
    case IMM16: imm16_alloc(&work->state, &r, (int)i); break;
    case SHIFTIMM: shiftimm_alloc(&work->state, &r, (int)i); break;
    case SHIFT: shift_alloc(&work->state, &r, (int)i); break;
    default: return -1;
    }
    if (dops[i].may_except) r.isconst = 0;
    /* Spill every displaced dirty mapping before reloading any new mapping.
     * This also makes emitter-side source loads see the latest guest value. */
    for (h = 0; h < HOST_REGS; h++)
      if (r.regmap[h] != r.regmap_entry[h] && ((old_dirty >> h) & 1u))
        writeback((unsigned)r.regmap_entry[h], h);
    for (h = 0; h < HOST_REGS; h++) {
      int guest = r.regmap[h], old = r.regmap_entry[h];
      unsigned other;
      if (guest >= 0) for (other = h + 1; other < HOST_REGS; other++)
        if (guest == r.regmap[other]) return -1;
      if (h == HOST_CCREG && guest != CCREG) return -1;
      if (old > 0 && old < 32 && old != guest) evictions++;
      if (guest > 0 && guest < 32 &&
          (guest == dops[i].rs1 || guest == dops[i].rs2)) {
        if (guest != old) {
          emit_loadreg((unsigned)guest, (int)h);
          r.regmap_entry[h] = (signed char)guest;
        }
        else cached_sources++;
      }
    }
    if (dops[i].may_except)
      rv32_imm(&rv32_output, RV_ADD, RV_S1, RV_S1, 1);
    else
      deferred_cycles++;
    switch (kind) {
    case ALU: alu_assemble((int)i, &r, 0); break;
    case IMM16: imm16_assemble((int)i, &r, 0); break;
    case SHIFTIMM: shiftimm_assemble((int)i, &r); break;
    case SHIFT: shift_assemble((int)i, &r); break;
    }
    if (dops[i].rt1) {
      int host = get_reg_w(r.regmap, (signed char)dops[i].rt1);
      if (host < 0) return -1;
      if ((r.isconst >> host) & 1u) {
        emit_movimm(current_constmap[host], host);
        folded_results++;
      }
    }
    regs[i] = r;
    if (pending_load) {
      if (dops[i].rt1 != pending_load) commit_load_mapped(&r, pending_load);
      else cancel_load();
      pending_load = 0;
      regs[i] = r;
    }
  }
  emit_deferred_cycles(&deferred_cycles);
  if (preserve_final) {
    prepare_branch_map(&r);
    if (final_map) *final_map = r;
  }
  else
    flush_dirty(&r);
#ifdef ESP_PLATFORM
  if (allow_pending_exit && delayed_load_word(words[count - 1])) {
    /* A native block may now hand a terminal load to execI.  The generated
     * compiler owns slot zero internally, while the interpreter indexes its
     * retiring slot through dloadSel.  Empty interpreter state can legally
     * leave that selector at either value, so normalize the exported state:
     * the next interpreted instruction observes the old GPR value, then
     * dloadStep retires this value exactly once. */
    rv32_store(&rv32_output, RV_BYTE, RV_ZERO, RV_S0, PENDING_SEL_OFFSET);
    rv32_store(&rv32_output, RV_BYTE, RV_ZERO, RV_S0,
      PENDING_ALT_REG_OFFSET);
    rv32_store(&rv32_output, RV_WORD, RV_ZERO, RV_S0,
      PENDING_ALT_VALUE_OFFSET);
  }
#endif
  exit_code(count * 4, 0);
  for (i = 0; i < stub_count; i++) {
    size_t stub_start = rv32_output.count;
    for (h = 0; h < HOST_REGS; h++)
      if ((overflow_stubs[i].dirty >> h) & 1u)
        writeback((unsigned)overflow_stubs[i].regmap[h], h);
    int32_t offset = (int32_t)((stub_start - overflow_stubs[i].at) * 4);
    rv32_patch_relative(&rv32_output, overflow_stubs[i].at, offset);
    commit_load(overflow_stubs[i].pending);
    exit_code(overflow_stubs[i].insn * 4, overflow_stubs[i].status);
  }
  return rv32_output.error == RV_OK ? 0 : -1;
}

static int compile(void *code, size_t size, const uint32_t *words,
                   unsigned count)
{
  return compile_mapped(code, size, words, count, NULL, NULL, 0);
}

/* A branch-only live block still needs the normal entry-to-branch adapter.
 * Keep compile()'s zero-length rejection for its existing host-test contract,
 * and emit only that adapter here. */
#ifdef ESP_PLATFORM
static int compile_empty_prefix(void *code, size_t size)
{
  pending_load = 0;
  rv32_init(&rv32_output, code, size);
  stub_count = 0;
  cached_sources = evictions = writebacks = folded_results = 0;
  memory_temp_spills = memory_preserved_mappings = memory_mapped_sources = 0;
  cycle_adds_saved = direct_ram_fastpaths = 0;
  branch_preserved_mappings = 0;
  exit_code(0, 0);
  return rv32_output.error == RV_OK ? 0 : -1;
}
#endif

/* Independent, restricted MIPS test interpreter. No fetch, memory or events. */
static int reference_step(uint32_t *r, uint32_t w)
{
  unsigned op = w >> 26, f = w & 63, s = (w >> 21) & 31, t = (w >> 16) & 31;
  unsigned d = (w >> 11) & 31, sh = (w >> 6) & 31;
  uint32_t a = r[s], b = r[t], v = 0;
  int32_t imm = (int16_t)w;
  int64_t wide;
  if (!op) switch (f) {
  case 0: v = b << sh; break;
  case 2: v = b >> sh; break;
  case 3: v = (uint32_t)((int32_t)b >> sh); break;
  case 4: v = b << (a & 31); break;
  case 6: v = b >> (a & 31); break;
  case 7: v = (uint32_t)((int32_t)b >> (a & 31)); break;
  case 0x10: d = (w >> 11) & 31; v = r[HIREG]; break;
  case 0x11: r[HIREG] = a; d = 0; break;
  case 0x12: d = (w >> 11) & 31; v = r[LOREG]; break;
  case 0x13: r[LOREG] = a; d = 0; break;
  case 0x18:
    wide = (int64_t)(int32_t)a * (int32_t)b;
    r[LOREG] = (uint32_t)wide; r[HIREG] = (uint32_t)((uint64_t)wide >> 32);
    d = 0; break;
  case 0x19: {
    uint64_t product = (uint64_t)a * b;
    r[LOREG] = (uint32_t)product; r[HIREG] = (uint32_t)(product >> 32);
    d = 0; break;
  }
  case 0x1a:
    r[HIREG] = a;
    if (!b) r[LOREG] = (a & 0x80000000u) ? 1 : UINT32_MAX;
    else if (a == 0x80000000u && b == UINT32_MAX) {
      r[LOREG] = 0x80000000u; r[HIREG] = 0;
    }
    else {
      r[LOREG] = (uint32_t)((int32_t)a / (int32_t)b);
      r[HIREG] = (uint32_t)((int32_t)a % (int32_t)b);
    }
    d = 0; break;
  case 0x1b:
    r[LOREG] = b ? a / b : UINT32_MAX;
    r[HIREG] = b ? a % b : a;
    d = 0; break;
  case 0x20: case 0x22:
    wide = f == 0x20 ? (int64_t)(int32_t)a + (int32_t)b : (int64_t)(int32_t)a - (int32_t)b;
    if (wide > INT32_MAX || wide < INT32_MIN) return 1;
    v = (uint32_t)wide; break;
  case 0x21: v = a + b; break;
  case 0x23: v = a - b; break;
  case 0x24: v = a & b; break;
  case 0x25: v = a | b; break;
  case 0x26: v = a ^ b; break;
  case 0x27: v = ~(a | b); break;
  case 0x2a: v = (int32_t)a < (int32_t)b; break;
  case 0x2b: v = a < b; break;
  default: return -1;
  }
  else {
    d = t;
    switch (op) {
    case 8:
      wide = (int64_t)(int32_t)a + imm;
      if (wide > INT32_MAX || wide < INT32_MIN) return 1;
      v = (uint32_t)wide; break;
    case 9: v = a + (uint32_t)imm; break;
    case 10: v = (int32_t)a < imm; break;
    case 11: v = a < (uint32_t)imm; break;
    case 12: v = a & (w & 0xffff); break;
    case 13: v = a | (w & 0xffff); break;
    case 14: v = a ^ (w & 0xffff); break;
    case 15: v = w << 16; break;
    default: return -1;
    }
  }
  if (d) r[d] = v;
  return 0;
}

#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)

/* Regression for out-of-line guards in blocks larger than a RISC-V B-type
 * displacement. Both the local inverted branch and the far JAL are executed. */
static int long_guard_selftest(void *code, size_t size,
                               int (*publish)(void *, size_t))
{
  struct regstat r;
  size_t stub_start;
  if (size < 8192) return __LINE__;
  reset_register_map(&r);
  pending_load = 0;
  stub_count = 0;
  branch_pair = runtime_exit_mode = 0;
  linear_pc = 0;
  test_dispatcher = NULL;
  rv32_init(&rv32_output, code, size);
  emit_loadreg(1, 0);
  emit_loadreg(2, 1);
  memory_guard_status(0, RV_EQ, RV_A0, RV_A1, 3, &r);
  while (rv32_output.count < 1100)
    rv32_word(&rv32_output, 0x00000013); /* addi zero,zero,0 */
  exit_code(4, 0);
  CHECK(stub_count == 1);
  stub_start = rv32_output.count;
  CHECK((stub_start - overflow_stubs[0].at) * 4 > 4092);
  rv32_patch_relative(&rv32_output, overflow_stubs[0].at,
    (int32_t)((stub_start - overflow_stubs[0].at) * 4));
  exit_code(0, overflow_stubs[0].status);
  CHECK(rv32_output.error == RV_OK);
  CHECK(publish(code, rv32_output.count * 4) == 0);
  for (unsigned equal = 0; equal < 2; equal++) {
    struct test_cpu actual = {0};
    uint32_t cycles = 0;
    actual.gpr[1] = 7;
    actual.gpr[2] = equal ? 7 : 8;
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(TEST_PC(actual) == (equal ? 0u : 4u));
    CHECK(actual.exception == (equal ? 3u : 0u));
  }
  return 0;
}

/* Terminal branch + ALU delay-slot integration. Register maps start empty at
 * this entry; selected PC is returned to the caller, never dispatched here.
 * Shared decoder and ALU compiler are reused, but this is NOT the upstream
 * branch allocator/assembler or its out-of-order delay-slot scheduling. */
static int compile_branch_mapped(void *code, size_t size, uint32_t pc,
                                 uint32_t word, uint32_t delay,
                                 unsigned incoming_pending,
                                 const struct regstat *incoming_map)
{
  unsigned op = word >> 26, f = word & 63, sub = (word >> 16) & 31;
  unsigned link = 0, s, t;
  int cond = -1;
  struct regstat r;
  size_t jump;
  uint32_t target = pc + 4u + (uint32_t)((int32_t)(int16_t)word * 4);
  if (size < 1024 || decode(0, delay) < 0) return -1;
  if (!((op >= 2 && op <= 7) || (!op && (f == 8 || f == 9)) ||
        (op == 1 && (sub == 0 || sub == 1 || sub == 16 || sub == 17)))) return -1;
  disassemble_one(&work->state, 0, word);
  s = dops[0].rs1; t = op == 4 || op == 5 ? dops[0].rs2 : 0;
  link = dops[0].rt1;
  if (incoming_map)
    r = *incoming_map;
  else
    reset_register_map(&r);
  rv32_init(&rv32_output, code, 256);
  /* Capture operands/indirect target before a link write or delay-slot update. */
  emit_memory_source(&r, s, RV_A0);
  emit_memory_source(&r, t, RV_A1);
  if (!op) rv32_imm(&rv32_output, RV_ADD, RV_A2, RV_A0, 0);
  else if (op == 2 || op == 3) {
    target = ((pc + 4u) & 0xf0000000u) | ((word & 0x03ffffffu) << 2);
    rv32_li(&rv32_output, RV_A2, target);
  }
  else {
    switch (op) {
    case 4: cond = RV_EQ; break;
    case 5: cond = RV_NE; break;
    case 6: cond = RV_GE; break; /* 0 >= rs */
    case 7: cond = RV_LT; break; /* 0 < rs */
    case 1: cond = sub & 1 ? RV_GE : RV_LT; break;
    }
    rv32_li(&rv32_output, RV_A2, target);
    jump = rv32_output.count;
    rv32_branch(&rv32_output, (unsigned)cond,
      op == 6 || op == 7 ? RV_ZERO : RV_A0,
      op == 6 || op == 7 ? RV_A0 : (op == 1 ? RV_ZERO : RV_A1), 0);
    rv32_li(&rv32_output, RV_A2, pc + 8u);
    rv32_patch_relative(&rv32_output, jump, (int32_t)((rv32_output.count - jump) * 4));
  }
  rv32_store(&rv32_output, RV_WORD, RV_A2, RV_S0,
#ifdef ESP_PLATFORM
    runtime_exit_mode ? offsetof(struct test_cpu, cpu.pc) :
#endif
    offsetof(struct test_cpu, branch_target));
  /* A load immediately before this branch is a statically known pipeline
   * boundary. Branch operands above must observe the old GPR, while its delay
   * slot must observe the loaded value. Resolve it here after condition/target
   * capture, cancelling it when the branch's own link write wins the same
   * destination. This avoids a general pending-entry prologue on every block. */
  if (incoming_pending) {
    if (link == incoming_pending) cancel_load();
    else commit_load_mapped(&r, incoming_pending);
  }
  if (link) {
    int host = get_reg(r.regmap, (signed char)link);
    if (host >= 0) {
      rv32_li(&rv32_output, rv32_hr(host), pc + 8u);
      r.dirty |= 1u << host;
      r.isconst &= ~(1u << host);
      r.wasconst &= ~(1u << host);
    }
    else {
      rv32_li(&rv32_output, RV_A0, pc + 8u);
      rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0,
        (int32_t)(link * 4));
    }
  }
  rv32_imm(&rv32_output, RV_ADD, RV_S1, RV_S1, 1);
  rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6, (uint32_t)(uintptr_t)code + 256u);
  if (rv32_output.error != RV_OK) return -1;
  branch_pair = 1; branch_pc = pc;
  int result = compile_mapped((unsigned char *)code + 256, size - 256,
    &delay, 1, &r, NULL, 0);
  branch_pair = 0;
  return result;
}

static int compile_branch(void *code, size_t size, uint32_t pc, uint32_t word,
                          uint32_t delay, unsigned incoming_pending)
{
  return compile_branch_mapped(code, size, pc, word, delay,
    incoming_pending, NULL);
}

static int branch_selftest(void *code, size_t size, int (*publish)(void *, size_t))
{
  static const uint32_t branches[] = {
    0x10220003, 0x1422fffc, 0x18200003, 0x1c20fffc,
    0x04200003, 0x0421fffc, 0x04300003, 0x0431fffc,
    0x08000400, 0x0c000400, 0x00200008, 0x0020f809, 0x00200809,
    0x07f00003, 0x07f1fffc /* link branches testing their own r31 source */
  };
  static const uint32_t delays[] = {
    0x00000000, /* canonical NOP must still consume its delay-slot cycle */
    0x24210001, /* overwrite condition/indirect target */
    0x27ff0001, /* read and overwrite link */
    0x20210001, /* possible positive overflow, destination aliases source */
    0x2021ffff, /* possible negative overflow */
    0x20000000  /* trapping opcode writing zero, but cannot overflow */
  };
  static const uint32_t values[] = {0, 1, 0xffffffff, 0x7fffffff, 0x80000000};
  for (unsigned b = 0; b < ARRAY_SIZE(branches); b++)
    for (unsigned d = 0; d < ARRAY_SIZE(delays); d++)
      for (unsigned page = 0; page < 2; page++) {
        uint32_t pc = page ? 0x8ffffffc : 0x80001000;
        uint32_t w = branches[b], op = w >> 26;
        CHECK(compile_branch(code, size, pc, w, delays[d], 0) == 0);
        CHECK(publish(code, 256 + rv32_output.count * 4) == 0);
        for (unsigned v = 0; v < ARRAY_SIZE(values); v++)
          for (unsigned equal = 0; equal < 2; equal++) {
            struct test_cpu actual = {0}, expected;
            uint32_t cycles = 0, target, source, second;
            unsigned link = 0;
            int taken = 1;
            for (unsigned r = 1; r < 32; r++) actual.gpr[r] = 0x12340000u + r;
            actual.gpr[1] = actual.gpr[31] = values[v];
            actual.gpr[2] = equal ? values[v] : values[(v + 1) % ARRAY_SIZE(values)];
            expected = actual;
            source = expected.gpr[(w >> 21) & 31];
            second = expected.gpr[(w >> 16) & 31];
            target = pc + 4u + (uint32_t)((int32_t)(int16_t)w * 4);
            switch (op) {
            case 0: target = source; link = (w & 63) == 9 ? (w >> 11) & 31 : 0; break;
            case 1:
              taken = (w & 0x10000) ? (int32_t)source >= 0 : (int32_t)source < 0;
              link = w & 0x100000 ? 31 : 0; break;
            case 2: case 3:
              target = ((pc + 4u) & 0xf0000000u) | ((w & 0x03ffffffu) * 4u);
              link = op == 3 ? 31 : 0; break;
            case 4: taken = source == second; break;
            case 5: taken = source != second; break;
            case 6: taken = (int32_t)source <= 0; break;
            case 7: taken = (int32_t)source > 0; break;
            }
            if (link) expected.gpr[link] = pc + 8u;
            int overflow = reference_step(expected.gpr, delays[d]);
            TEST_PC(expected) = overflow ? pc : (taken ? target : pc + 8u);
            expected.exception = overflow ? 2 : 0;
            TEST_CYCLE(actual) = 0;
            ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
            CHECK(TEST_CYCLE(actual) == 2);
            CHECK(TEST_PC(actual) == TEST_PC(expected) && actual.exception == expected.exception);
            for (unsigned r = 0; r < 32; r++) CHECK(actual.gpr[r] == expected.gpr[r]);
          }
      }
  CHECK(compile_branch(code, size, 0x80001000, branches[0], branches[0], 0) != 0);
  CHECK(compile_branch(code, size, 0x80001000, branches[0], 0x8c220000, 0) != 0);
  return 0;
}

/* The production terminal-branch path carries mappings in s2-s11 from the
 * prefix through target selection and the delay slot. Exercise that private
 * entry contract explicitly: the canonical copies are intentionally stale,
 * so either branch evaluation or the delay-slot result is wrong if the map is
 * discarded at either internal boundary. */
static int mapped_branch_selftest(void *code, size_t size,
                                  int (*publish)(void *, size_t))
{
  const uint32_t pc = 0x80001000;
  const uint32_t branch = 0x10220003; /* beq r1,r2,+3 */
  const uint32_t delay = 0x24210001;  /* addiu r1,r1,1 */
  struct regstat map;
  unsigned char *bytes = code;
  if (size < 4096) return __LINE__;

  reset_register_map(&map);
  map.regmap[13] = map.regmap_entry[13] = 1; /* s4 */
  map.regmap[14] = map.regmap_entry[14] = 2; /* s5 */
  map.dirty = (1u << 13) | (1u << 14);

  rv32_init(&rv32_output, code, 256);
  rv32_load(&rv32_output, RV_WORD, RV_S4, RV_S0, 4);
  rv32_load(&rv32_output, RV_WORD, RV_S5, RV_S0, 8);
  rv32_store(&rv32_output, RV_WORD, RV_ZERO, RV_S0, 4);
  rv32_store(&rv32_output, RV_WORD, RV_ZERO, RV_S0, 8);
  rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6,
    (uint32_t)(uintptr_t)(bytes + 256));
  CHECK(rv32_output.error == RV_OK);
  CHECK(compile_branch_mapped(bytes + 256, size - 256, pc, branch, delay,
    0, &map) == 0);
  CHECK(publish(code, 512 + rv32_output.count * 4) == 0);

  for (unsigned equal = 0; equal < 2; equal++) {
    struct test_cpu actual = {0};
    uint32_t cycles = 0;
    actual.gpr[1] = 41;
    actual.gpr[2] = equal ? 41 : 42;
    TEST_CYCLE(actual) = 0;
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(TEST_CYCLE(actual) == 2);
    CHECK(TEST_PC(actual) == (equal ? pc + 16 : pc + 8));
    CHECK(actual.gpr[1] == 42 && actual.gpr[2] == (equal ? 41u : 42u));
  }
  return 0;
}

/* Stores are safe in a branch delay slot because they leave no delayed value
 * for the selected successor to consume. Loads remain excluded until generated
 * block entry/link adapters can resolve an incoming MIPS load delay. */
static int branch_store_delay_selftest(void *code, size_t size,
                                       int (*publish)(void *, size_t))
{
  static const uint32_t stores[] = {
    0xa0230001, /* sb r3,1(r1) */
    0xa4230002, /* sh r3,2(r1) */
    0xac230004, /* sw r3,4(r1) */
    0xa8230002, /* swl r3,2(r1) */
    0xb8230001  /* swr r3,1(r1) */
  };
  static const uint32_t expected_ram0[] = {
    0x1020ef40, 0xcdef3040, 0x10203040, 0x1089abcd, 0xabcdef40
  };
  const uint32_t pc = 0x80002000, branch = 0x10400002; /* beq r2,r0,+2 */
  for (unsigned s = 0; s < ARRAY_SIZE(stores); s++) {
    CHECK(compile_branch(code, size, pc, branch, stores[s], 0) == 0);
    CHECK(publish(code, 256 + rv32_output.count * 4) == 0);
    for (unsigned taken = 0; taken < 2; taken++) {
      struct test_cpu actual = {0};
      uint32_t cycles = 0;
#ifdef ESP_PLATFORM
      uintptr_t lut[1] = {(uintptr_t)actual.ram};
      actual.cpu.ptrs.memRLUT = lut;
      actual.cpu.ptrs.memWLUT = lut;
#endif
      actual.gpr[1] = 0;
      actual.gpr[2] = taken ? 0 : 1;
      actual.gpr[3] = 0x89abcdef;
      actual.ram[0] = 0x10203040;
      actual.ram[1] = 0x50607080;
      TEST_CYCLE(actual) = 0;
      ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
      CHECK(TEST_CYCLE(actual) == 2);
      CHECK(TEST_PC(actual) == (taken ? pc + 12 : pc + 8));
      CHECK(actual.gpr[1] == 0 && actual.gpr[2] == (taken ? 0u : 1u) &&
            actual.gpr[3] == 0x89abcdef);
      CHECK(actual.ram[0] == expected_ram0[s]);
      CHECK(actual.ram[1] == (s == 2 ? 0x89abcdefu : 0x50607080u));
      CHECK(LOAD_DELAY_CLEAR(actual));
    }
  }
  return 0;
}

/* A bounded generated dispatcher, not the production block cache. All block
 * exits write back dirty GPRs; entries rebuild maps from canonical scratch state.
 * The dispatcher stays inside generated code, preserving s0/s1 across tails. */
static int multiblock_selftest(void *code, size_t size, int (*publish)(void *, size_t))
{
  static const uint32_t pcs[] = {0x1000, 0x1008, 0x1010, 0x1014};
  unsigned char *arena = code;
  uint32_t words[2] = {0x24210001, 0x00411021}; /* r1++; r2 += r1 */
  size_t exception_exit, budget_exit, matches[4];
  int result = 0;
  if (size < 16384) return __LINE__;
  test_dispatcher = arena + 8192;
  for (unsigned b = 0; b < 4; b++) {
    linear_pc = pcs[b];
    if (b == 0) result = compile(arena, 2048, words, 2);
    if (b == 1) result = compile_branch(arena + 2048, 2048, pcs[b],
      0x1423fffd, 0x24840001, 0); /* bne r1,r3,1000; r4++ */
    if (b == 2) {
      uint32_t add = 0x24850007; /* r5 = r4 + 7 */
      result = compile(arena + 4096, 2048, &add, 1);
    }
    if (b == 3) result = compile_branch(arena + 6144, 2048, pcs[b],
      0x00c00008, 0x24e70001, 0); /* jr r6; r7++ */
    if (result) break;
  }
  linear_pc = 0;
  test_dispatcher = NULL;
  CHECK(result == 0);
  rv32_init(&rv32_output, arena + 8192, size - 8192);
  rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0, offsetof(struct test_cpu, exception));
  exception_exit = rv32_output.count;
  rv32_branch(&rv32_output, RV_NE, RV_A0, RV_ZERO, 0);
  rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0, offsetof(struct test_cpu, block_budget));
  budget_exit = rv32_output.count;
  rv32_branch(&rv32_output, RV_EQ, RV_A0, RV_ZERO, 0);
  rv32_load(&rv32_output, RV_WORD, RV_A1, RV_S0,
#ifdef ESP_PLATFORM
    offsetof(struct test_cpu, cpu.pc));
#else
    offsetof(struct test_cpu, pc));
#endif
  for (unsigned b = 0; b < 4; b++) {
    rv32_li(&rv32_output, RV_A2, pcs[b]);
    matches[b] = rv32_output.count;
    rv32_branch(&rv32_output, RV_EQ, RV_A1, RV_A2, 0);
  }
  size_t leave = rv32_output.count;
  rv32_patch_relative(&rv32_output, exception_exit, (int32_t)((leave - exception_exit) * 4));
  rv32_patch_relative(&rv32_output, budget_exit, (int32_t)((leave - budget_exit) * 4));
  rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6, (uint32_t)(uintptr_t)ndrc_rv32_leave);
  for (unsigned b = 0; b < 4; b++) {
    rv32_patch_relative(&rv32_output, matches[b], (int32_t)((rv32_output.count - matches[b]) * 4));
    rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_A0, -1);
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0, offsetof(struct test_cpu, block_budget));
    rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6, (uint32_t)(uintptr_t)(arena + b * 2048));
  }
  CHECK(rv32_output.error == RV_OK);
  CHECK(publish(code, 8192 + rv32_output.count * 4) == 0);
  for (unsigned n = 1; n <= 16; n++) for (unsigned budget = 0; budget <= 36; budget++) {
    struct test_cpu actual = {0}, expected;
    uint32_t cycles = 0, expected_cycles = 0;
    actual.gpr[3] = n; actual.gpr[6] = 0x2000;
    TEST_PC(actual) = 0x1000; actual.block_budget = budget;
    expected = actual;
    while (expected.block_budget && TEST_PC(expected) != 0x2000) {
      expected.block_budget--;
      switch (TEST_PC(expected)) {
      case 0x1000:
        expected.gpr[1]++; expected.gpr[2] += expected.gpr[1];
        TEST_PC(expected) = 0x1008; expected_cycles += 2; break;
      case 0x1008:
        TEST_PC(expected) = expected.gpr[1] != n ? 0x1000 : 0x1010;
        expected.gpr[4]++; expected_cycles += 2; break;
      case 0x1010:
        expected.gpr[5] = expected.gpr[4] + 7;
        TEST_PC(expected) = 0x1014; expected_cycles++; break;
      case 0x1014:
        TEST_PC(expected) = expected.gpr[6]; expected.gpr[7]++; expected_cycles += 2; break;
      default: return __LINE__;
      }
    }
    TEST_CYCLE(actual) = 0;
    ndrc_rv32_enter(arena + 8192, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(TEST_CYCLE(actual) == expected_cycles && TEST_PC(actual) == TEST_PC(expected));
    CHECK(actual.exception == 0 && actual.block_budget == expected.block_budget);
    for (unsigned r = 0; r < 32; r++) CHECK(actual.gpr[r] == expected.gpr[r]);
  }
  /* Exception exits must stop dispatch, even with a remaining block budget. */
  struct test_cpu trapped = {0};
  uint32_t cycles = 0;
#ifdef ESP_PLATFORM
  (void)cycles; /* Host-independent build passes this to the linkage shim. */
#endif
  TEST_PC(trapped) = 0x1010; trapped.exception = 2; trapped.block_budget = 10;
  TEST_CYCLE(trapped) = 0;
  ndrc_rv32_enter(arena + 8192, &trapped, TEST_CYCLE_PTR(trapped, &cycles));
  CHECK(TEST_CYCLE(trapped) == 0 && TEST_PC(trapped) == 0x1010 && trapped.block_budget == 10);
  /* Also generate an exception during execution, after a preceding block has
   * committed dirty results, on both taken and untaken delay-slot paths. */
  test_dispatcher = arena + 8192;
  result = compile_branch(arena + 2048, 2048, 0x1008, 0x1423fffd,
    0x20840001, 0);
  test_dispatcher = NULL;
  CHECK(result == 0);
  CHECK(publish(arena + 2048, 256 + rv32_output.count * 4) == 0);
  for (unsigned taken = 0; taken < 2; taken++) {
    struct test_cpu actual = {0};
    cycles = 0;
    TEST_PC(actual) = 0x1000; actual.block_budget = 10;
    actual.gpr[3] = taken ? 2 : 1; actual.gpr[4] = 0x7fffffff;
    TEST_CYCLE(actual) = 0;
    ndrc_rv32_enter(arena + 8192, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(TEST_CYCLE(actual) == 4 && actual.exception == 2 && TEST_PC(actual) == 0x1008);
    CHECK(actual.block_budget == 8 && actual.gpr[1] == 1 && actual.gpr[2] == 1);
    CHECK(actual.gpr[4] == 0x7fffffff && actual.gpr[5] == 0 && actual.gpr[7] == 0);
  }
  return 0;
}
static int memory_selftest(void *code, size_t size, int (*publish)(void *, size_t))
{
  static const uint32_t programs[][4] = {
    {0x8c220000, 0x24430001, 0x24440002, 0},
    {0x8c220000, 0x24420001, 0x24440002, 0},
    {0x8c220000, 0x8c220004, 0x24430001, 0x24440002},
    {0x8c210000, 0x8c220004, 0x24430001, 0x24440002},
    {0x8c220000, 0xac220004, 0, 0},
    {0x8c220000, 0x20630001, 0, 0},
    {0xac220000, 0x8c240000, 0x24850001, 0},
    {0x8c200000, 0x24050001, 0, 0},
    {0x8c220000, 0x8c240001, 0, 0},
    {0x8c220000, 0xac240001, 0, 0},
    {0x80220000, 0x24430001, 0, 0}, /* LB */
    {0x90220000, 0x24430001, 0, 0}, /* LBU */
    {0x84220000, 0x24430001, 0, 0}, /* LH */
    {0x94220000, 0x24430001, 0, 0}, /* LHU */
    {0xa0220000, 0x8c240000, 0, 0}, /* SB */
    {0xa4220000, 0x8c240000, 0, 0}, /* SH */
    {0x88220000, 0x24430001, 0, 0}, /* LWL, every address low bit */
    {0x98220000, 0x24430001, 0, 0}, /* LWR, every address low bit */
    {0x88220000, 0x98220003, 0x24430001, 0}, /* pending LWL/LWR merge */
    {0xa8220000, 0x8c240000, 0, 0}, /* SWL */
    {0xb8220000, 0x8c240000, 0, 0}  /* SWR */
  };
  static const uint32_t addresses[] = {0, 4, 248, 252, 256, 1, 0xfffffffc};
  for (unsigned p = 0; p < ARRAY_SIZE(programs); p++) {
    CHECK(compile(code, size, programs[p], 4) == 0);
    CHECK(publish(code, rv32_output.count * 4) == 0);
    for (unsigned a = 0; a < ARRAY_SIZE(addresses); a++) for (unsigned edge = 0; edge < 4; edge++) {
      struct test_cpu actual = {0}, expected;
      uint32_t cycles = 0, expected_cycles = 0, pending = 0, value = 0;
#ifdef ESP_PLATFORM
      uintptr_t actual_lut[1] = {(uintptr_t)actual.ram};
      actual.cpu.ptrs.memRLUT = actual_lut;
      actual.cpu.ptrs.memWLUT = actual_lut;
#endif
      actual.gpr[1] = addresses[a]; actual.gpr[2] = 0x12345678;
      actual.gpr[3] = edge & 1 ? 0x7fffffff : 0;
      for (unsigned r = 0; r < 64; r++) actual.ram[r] =
        edge & 2 ? 0x80000000u + r : r * 4;
      expected = actual; TEST_PC(expected) = 16;
#ifdef ESP_PLATFORM
      uint32_t oracle_gpr[32], oracle_ram[64], oracle_pc, oracle_cycles;
      memcpy(oracle_gpr, actual.gpr, sizeof(oracle_gpr));
      memcpy(oracle_ram, actual.ram, sizeof(oracle_ram));
      int oracle_status = ndrc_rv32_interpreter_memory(oracle_gpr, oracle_ram,
        programs[p], 4, &oracle_pc, &oracle_cycles);
      CHECK(oracle_status >= 0);
#endif
      for (unsigned i = 0; i < 4; i++) {
        uint32_t w = programs[p][i], op = w >> 26, next_value = 0;
        unsigned dest = op ? (w >> 16) & 31 : (w >> 11) & 31, next = 0;
        int fault = 0;
        expected_cycles++;
        if (memory_load_opcode(op) || memory_store_opcode(op)) {
          uint32_t address = expected.gpr[(w >> 21) & 31] + (uint32_t)(int32_t)(int16_t)w;
          int is_load = memory_load_opcode(op);
          unsigned align = (op == 0x21 || op == 0x25 || op == 0x29) ? 1 :
            (op == 0x23 || op == 0x2b) ? 3 : 0;
          unsigned max = align == 3 ? 252 : align == 1 ? 254 : 255;
          uint8_t *bytes = (uint8_t *)expected.ram;
          if ((address & align) || address > max) fault = is_load ? 3 : 4;
          else if (is_load) {
            uint32_t old_value = pending == dest ? value : expected.gpr[dest];
            next = dest;
            if (op == 0x20) next_value = (uint32_t)(int32_t)(int8_t)bytes[address];
            else if (op == 0x24) next_value = bytes[address];
            else if (op == 0x21) next_value = (uint32_t)(int32_t)(int16_t)
              (uint16_t)(bytes[address] | (uint16_t)bytes[address + 1] << 8);
            else if (op == 0x25) next_value =
              (uint16_t)(bytes[address] | (uint16_t)bytes[address + 1] << 8);
            else if (op == 0x22) {
              unsigned shift = 24u - (address & 3u) * 8u;
              uint32_t memory = expected.ram[(address & ~3u) / 4u];
              next_value = (old_value & ~(UINT32_MAX << shift)) |
                (memory << shift);
            }
            else if (op == 0x26) {
              unsigned shift = (address & 3u) * 8u;
              uint32_t memory = expected.ram[(address & ~3u) / 4u];
              next_value = (old_value & ~(UINT32_MAX >> shift)) |
                (memory >> shift);
            }
            else next_value = expected.ram[address / 4];
          }
          else if (op == 0x28) bytes[address] = (uint8_t)expected.gpr[dest];
          else if (op == 0x29) {
            bytes[address] = (uint8_t)expected.gpr[dest];
            bytes[address + 1] = (uint8_t)(expected.gpr[dest] >> 8);
          }
          else if (op == 0x2a) {
            unsigned count = (address & 3u) + 1u;
            unsigned base = address & ~3u;
            for (unsigned byte = 0; byte < count; byte++)
              bytes[base + byte] = (uint8_t)(expected.gpr[dest] >>
                ((4u - count + byte) * 8u));
          }
          else if (op == 0x2e) {
            unsigned count = 4u - (address & 3u);
            for (unsigned byte = 0; byte < count; byte++)
              bytes[address + byte] =
                (uint8_t)(expected.gpr[dest] >> (byte * 8u));
          }
          else expected.ram[address / 4] = expected.gpr[dest];
          if (pending && (fault || !is_load || dest != pending))
            expected.gpr[pending] = value;
        }
        else {
          fault = reference_step(expected.gpr, w);
          CHECK(fault >= 0);
          if (pending && (fault || dest != pending)) expected.gpr[pending] = value;
        }
        pending = next; value = next_value;
        if (fault) { TEST_PC(expected) = i * 4; expected.exception = (uint32_t)fault; break; }
      }
      TEST_CYCLE(actual) = 0;
      ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
      CHECK(TEST_CYCLE(actual) == expected_cycles && TEST_PC(actual) == TEST_PC(expected));
      CHECK(actual.exception == expected.exception);
      CHECK(LOAD_DELAY_CLEAR(actual));
      for (unsigned r = 0; r < 32; r++) CHECK(actual.gpr[r] == expected.gpr[r]);
      for (unsigned r = 0; r < 64; r++) CHECK(actual.ram[r] == expected.ram[r]);
#ifdef ESP_PLATFORM
      CHECK(oracle_pc == TEST_PC(actual) && oracle_cycles == TEST_CYCLE(actual));
      CHECK((uint32_t)oracle_status == actual.exception);
      CHECK(memcmp(oracle_gpr, actual.gpr, sizeof(oracle_gpr)) == 0);
      CHECK(memcmp(oracle_ram, actual.ram, sizeof(oracle_ram)) == 0);
#endif
      /* The instruction following the second load still sees the original r2. */
      if (p == 2 && !(addresses[a] & 3) && addresses[a] <= 248)
        CHECK(actual.gpr[3] == 0x12345679);
    }
  }
  /* Force dirty allocator mappings through both named memory-temporary slots,
   * then prove the boundary spill preserves them across store/load lowering. */
  {
    uint32_t pressure[16];
    struct test_cpu actual = {0};
    uint32_t cycles = 0;
    for (unsigned i = 0; i < 12; i++) {
      unsigned rt = i + 2;
      pressure[i] = 0x24000000u | (rt << 16) | (0x100u + rt); /* addiu rt,r0,imm */
    }
    pressure[12] = 0xac240000; /* sw r4,0(r1), retained in a saved host slot */
    pressure[13] = 0x8c2e0000; /* lw r14,0(r1) */
    pressure[14] = 0x25cf0001; /* addiu r15,r14,1 (sees old r14) */
    pressure[15] = 0x00448021; /* addu r16,r2,r4 */
    CHECK(compile(code, size, pressure, ARRAY_SIZE(pressure)) == 0);
    CHECK(memory_temp_spills > 0);
    CHECK(memory_preserved_mappings > 0 && memory_mapped_sources > 0);
    CHECK(publish(code, rv32_output.count * 4) == 0);
#ifdef ESP_PLATFORM
    uintptr_t actual_lut[1] = {(uintptr_t)actual.ram};
    actual.cpu.ptrs.memRLUT = actual_lut;
    actual.cpu.ptrs.memWLUT = actual_lut;
#endif
    TEST_CYCLE(actual) = 0;
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(TEST_CYCLE(actual) == ARRAY_SIZE(pressure) && TEST_PC(actual) == 64);
    for (unsigned rt = 2; rt <= 13; rt++) CHECK(actual.gpr[rt] == 0x100u + rt);
    CHECK(actual.ram[0] == 0x104 && actual.gpr[14] == 0x104);
    CHECK(actual.gpr[15] == 1 && actual.gpr[16] == 0x206);
    CHECK(LOAD_DELAY_CLEAR(actual));
  }
  CHECK(compile(code, size, programs[0], 1) != 0); /* unresolved end-of-block load */
  /* Exercise the real dispatch boundary. The shared producer exits with an LW
   * pending; this deliberately small boundary adapter captures the following
   * instruction's source before dynamically committing/cancelling that load. */
  for (unsigned dest = 2; dest <= 3; dest++) {
    unsigned char *arena = code;
    const uint32_t lw = 0x8c220000;
    size_t no_pending, cancel, clear;
    struct test_cpu actual = {0};
    uint32_t cycles = 0;
#ifdef ESP_PLATFORM
    uintptr_t actual_lut[1] = {(uintptr_t)actual.ram};
    actual.cpu.ptrs.memRLUT = actual_lut;
    actual.cpu.ptrs.memWLUT = actual_lut;
    /* An empty interpreter pipeline may leave either selector value.  A
     * terminal native load must export a canonical slot-zero delay. */
    actual.cpu.dloadSel = 1;
#endif
    allow_pending_exit = 1;
    test_dispatcher = arena + 2048;
    linear_pc = 0;
    CHECK(compile(arena, 2048, &lw, 1) == 0);
    allow_pending_exit = 0;
    test_dispatcher = NULL;
    rv32_init(&rv32_output, arena + 2048, size - 2048);
    emit_loadreg(2, 0); /* Capture the architectural pre-load r2. */
    rv32_load(&rv32_output, RV_BYTE_U, RV_A1, RV_S0, PENDING_REG_OFFSET);
    no_pending = rv32_output.count;
    rv32_branch(&rv32_output, RV_EQ, RV_A1, RV_ZERO, 0);
    rv32_li(&rv32_output, RV_A3, dest);
    cancel = rv32_output.count;
    rv32_branch(&rv32_output, RV_EQ, RV_A1, RV_A3, 0);
    rv32_load(&rv32_output, RV_WORD, RV_A2, RV_S0, PENDING_VALUE_OFFSET);
    rv32_shift(&rv32_output, RV_SLL, RV_A1, RV_A1, 2);
    rv32_alu(&rv32_output, RV_ADD, RV_A1, RV_S0, RV_A1);
    rv32_store(&rv32_output, RV_WORD, RV_A2, RV_A1, 0);
    clear = rv32_output.count;
    rv32_patch_relative(&rv32_output, no_pending,
      (int32_t)((clear - no_pending) * 4));
    rv32_patch_relative(&rv32_output, cancel,
      (int32_t)((clear - cancel) * 4));
    cancel_load();
    rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_A0, 1);
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0, (int32_t)(dest * 4));
    rv32_imm(&rv32_output, RV_ADD, RV_S1, RV_S1, 1);
    rv32_li(&rv32_output, RV_A0, 8);
    rv32_store(&rv32_output, RV_WORD, RV_A0, RV_S0,
#ifdef ESP_PLATFORM
      offsetof(struct test_cpu, cpu.pc));
#else
      offsetof(struct test_cpu, pc));
#endif
    rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6, (uint32_t)(uintptr_t)ndrc_rv32_leave);
    CHECK(rv32_output.error == RV_OK);
    CHECK(publish(code, 2048 + rv32_output.count * 4) == 0);
    actual.gpr[1] = 0; actual.gpr[2] = 10; actual.ram[0] = 40;
    TEST_CYCLE(actual) = 0;
    ndrc_rv32_enter(arena, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(TEST_CYCLE(actual) == 2 && TEST_PC(actual) == 8);
    CHECK(actual.gpr[2] == (dest == 2 ? 11 : 40));
    CHECK(actual.gpr[3] == (dest == 3 ? 11 : 0));
    CHECK(LOAD_DELAY_CLEAR(actual));
#ifdef ESP_PLATFORM
    CHECK(actual.cpu.dloadSel == 0 && actual.cpu.dloadReg[1] == 0 &&
          actual.cpu.dloadVal[1] == 0);
#endif
  }
#ifdef ESP_PLATFORM
  /* Inspect the producer boundary itself for both a real delayed destination
   * and LW r0.  The latter has no pending value, but still canonicalizes an
   * otherwise irrelevant stale selector before returning to C. */
  for (unsigned dest = 0; dest <= 2; dest += 2) {
    uint32_t lw = 0x8c200000u | (dest << 16);
    struct test_cpu actual = {0};
    uintptr_t actual_lut[1] = {(uintptr_t)actual.ram};
    actual.cpu.ptrs.memRLUT = actual_lut;
    actual.cpu.ptrs.memWLUT = actual_lut;
    actual.cpu.dloadSel = 1;
    actual.cpu.dloadVal[1] = 0xfeedfaceu;
    actual.gpr[1] = 0;
    actual.gpr[2] = 0x11223344u;
    actual.ram[0] = 0xa5a55a5au;
    allow_pending_exit = 1;
    linear_pc = 0;
    CHECK(compile(code, size, &lw, 1) == 0);
    allow_pending_exit = 0;
    CHECK(publish(code, rv32_output.count * 4) == 0);
    CHECK(ndrc_rv32_enter(code, &actual, &actual.cpu.cycle) == 0);
    CHECK(actual.cpu.pc == 4 && actual.cpu.cycle == 1 &&
          actual.cpu.dloadSel == 0 && actual.cpu.dloadReg[1] == 0 &&
          actual.cpu.dloadVal[1] == 0);
    CHECK(actual.cpu.dloadReg[0] == dest &&
          actual.cpu.dloadVal[0] == (dest ? 0xa5a55a5au : 0));
    CHECK(actual.gpr[2] == 0x11223344u);
  }
#endif
  /* Fuse the common load -> branch -> delay-slot boundary exactly as the live
   * compiler does.  The branch must consume the pre-load value, while its
   * delay slot consumes the loaded value.  Also cover a link write which
   * cancels a pending load to the same architectural register. */
  {
    static const struct {
      uint32_t branch;
      uint32_t old_value;
      uint32_t expected_pc;
      unsigned load_dest;
    } cases[] = {
      {0x10400002, 0x00000000, 0x00000010, 2}, /* taken beq r2,r0,+2 */
      {0x10400002, 0x00000007, 0x0000000c, 2}, /* untaken beq */
      {0x08000008, 0x12345678, 0x00000020, 2}, /* j 0x20 */
      {0x00400008, 0x00000040, 0x00000040, 2}, /* jr r2 */
      {0x0c000008, 0x12345678, 0x00000020, 31} /* jal; link wins r31 */
    };
    unsigned char *arena = code;
    for (unsigned i = 0; i < ARRAY_SIZE(cases); i++) {
      uint32_t load = 0x8c200000u | (cases[i].load_dest << 16);
      uint32_t delay = cases[i].load_dest == 31 ?
        0x27e30001u : 0x24430001u; /* addiu r3,loaded-register,1 */
      struct test_cpu actual = {0};
      uint32_t cycles = 0;
#ifdef ESP_PLATFORM
      uintptr_t actual_lut[1] = {(uintptr_t)actual.ram};
      actual.cpu.ptrs.memRLUT = actual_lut;
      actual.cpu.ptrs.memWLUT = actual_lut;
#endif
      allow_pending_exit = 1;
      test_dispatcher = arena + 2048;
      linear_pc = 0;
      CHECK(compile(arena, 2048, &load, 1) == 0);
      allow_pending_exit = 0;
      test_dispatcher = NULL;
      CHECK(compile_branch(arena + 2048, size - 2048, 4,
        cases[i].branch, delay, cases[i].load_dest) == 0);
      CHECK(publish(code, 2048 + 256 + rv32_output.count * 4) == 0);
      actual.gpr[1] = 0;
      actual.gpr[cases[i].load_dest] = cases[i].old_value;
      actual.ram[0] = 5;
      TEST_CYCLE(actual) = 0;
      ndrc_rv32_enter(arena, &actual, TEST_CYCLE_PTR(actual, &cycles));
      CHECK(TEST_CYCLE(actual) == 3 && TEST_PC(actual) == cases[i].expected_pc);
      CHECK(actual.gpr[cases[i].load_dest] ==
        (cases[i].load_dest == 31 ? 12u : 5u));
      CHECK(actual.gpr[3] == (cases[i].load_dest == 31 ? 13u : 6u));
      CHECK(LOAD_DELAY_CLEAR(actual));
    }
  }
  return 0;
}

#ifdef ESP_PLATFORM
static void delay_mmio_fixture(psxRegisters *cpu, uint32_t address, uint32_t value)
{
  struct test_cpu *fixture = (struct test_cpu *)cpu;
  fixture->ram[0] = cpu->pc;
  fixture->ram[1] = address;
  fixture->ram[2] = value;
  fixture->ram[3]++;
  cpu->cycle += 7; /* Ensure restoring the target does not discard handler time. */
}

static int delay_mmio_selftest(void *code, size_t size,
                               int (*publish)(void *, size_t))
{
  const uint32_t pc = 0x800121a8;
  static const uint32_t branches[] = {0x10400002, 0x0c011c4c, 0x00800008};
  static const uint32_t stores[] = {0xa0230000, 0xa4230000, 0xac230000};
  for (unsigned b = 0; b < ARRAY_SIZE(branches); b++) {
    for (unsigned s = 0; s < ARRAY_SIZE(stores); s++) {
    CHECK(compile_branch(code, size, pc, branches[b], stores[s], 0) == 0);
      CHECK(publish(code, 256 + rv32_output.count * 4) == 0);
      for (unsigned taken = 0; taken < 2; taken++) {
        struct test_cpu actual = {0};
#ifdef PCSX_NDRC_RV32_LIVE_CANARY
        uint32_t source_generation = live_canary_source_generation[0];
        live_canary_compiled_page[0] = 1;
#endif
        uintptr_t lut[1] = {INVALID_PTR_VAL};
        actual.cpu.ptrs.memWLUT = lut;
        actual.gpr[1] = 0x20;
        actual.gpr[2] = taken ? 0 : 1;
        actual.gpr[3] = 0x89abcdef;
        actual.gpr[4] = 0x80047130;
        actual.cpu.cycle = 100;
        CHECK(ndrc_rv32_enter(code, &actual, &actual.cpu.cycle) == 0);
        CHECK(actual.cpu.pc == (b ? 0x80047130u : pc + (taken ? 12u : 8u)));
        CHECK(actual.cpu.cycle == 109);
#ifdef PCSX_NDRC_RV32_LIVE_CANARY
        CHECK(live_canary_source_generation[0] == source_generation + 1);
#endif
        CHECK(actual.ram[0] == pc + 4 && actual.ram[1] == 0x20 &&
              actual.ram[2] == 0x89abcdef && actual.ram[3] == 1);
        CHECK(actual.gpr[31] == (b == 1 ? pc + 8 : 0));
        CHECK(LOAD_DELAY_CLEAR(actual));
      }
    }
  }
  return 0;
}

static int mapped_memory_selftest(void *code, size_t size,
                                  int (*publish)(void *, size_t))
{
  static const uint32_t addresses[] = {0x00000020, 0x80000020,
    0xa0000020, 0x1f800040, 0x1f801000, 0x1fc00000};
  const uint32_t words[] = {0x8c220000, 0x24430001, 0xac230004};
  uintptr_t *lut = rg_alloc(65536 * sizeof(*lut), MEM_SLOW | MEM_NOPANIC);
  uint8_t *scratch = rg_alloc(4096, MEM_SLOW | MEM_NOPANIC);
  if (!lut || !scratch) {
    free(lut); free(scratch);
    return __LINE__;
  }
  memset(lut, 0xff, 65536 * sizeof(*lut));
  for (unsigned a = 0; a < ARRAY_SIZE(addresses); a++) {
    struct test_cpu actual = {0};
    uint32_t cycles = 0;
    int mapped = a < 4;
    uint32_t *base = !mapped ? NULL : addresses[a] == 0x1f800040 ?
      (uint32_t *)(scratch + 0x40) : &actual.ram[8];
    memset(scratch, 0, 4096);
    actual.cpu.ptrs.memRLUT = lut;
    actual.cpu.ptrs.memWLUT = lut;
    actual.cpu.ptrs.psxM = (uint8_t *)actual.ram;
    actual.cpu.ptrs.psxH = scratch;
    lut[0x0000] = (uintptr_t)actual.ram;
    lut[0x8000] = (uintptr_t)actual.ram - 0x80000000u;
    lut[0xa000] = (uintptr_t)actual.ram - 0xa0000000u;
    actual.gpr[1] = addresses[a]; actual.gpr[2] = 10;
    if (mapped) { base[0] = 0x11223344; base[1] = 0; }
    full_address_mode = direct_ram_mode = 1;
    int compile_result = compile(code, size, words, ARRAY_SIZE(words));
    direct_ram_mode = 0;
    if (compile_result != 0) {
      full_address_mode = 0; free(scratch); free(lut); return __LINE__;
    }
    full_address_mode = 0;
    if (publish(code, rv32_output.count * 4) != 0) {
      free(scratch); free(lut); return __LINE__;
    }
    TEST_CYCLE(actual) = 0;
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    if ((mapped && (TEST_CYCLE(actual) != 3 || TEST_PC(actual) != 12 ||
         actual.exception || actual.gpr[2] != 0x11223344 ||
         actual.gpr[3] != 11 || base[1] != 11)) ||
        (!mapped && (TEST_CYCLE(actual) != 1 || TEST_PC(actual) != 0 ||
         actual.exception != 5 || actual.gpr[2] != 10 || actual.gpr[3] != 0)) ||
        !LOAD_DELAY_CLEAR(actual)) {
      free(scratch); free(lut); return __LINE__;
    }
  }
  free(scratch); free(lut);
  return 0;
}
#endif

#ifdef ESP_PLATFORM
/* ndrc_rv32_interpreter_alu() deliberately exposes only the original pure
 * 32-GPR ALU whitelist.  HI/LO instructions need psxRegisters state which its
 * fixed 32-word interface cannot carry, so leave those to reference_step()
 * and the explicit generated HI/LO differential tests below. */
static int interpreter_alu_oracle_supported(uint32_t word)
{
  unsigned op = word >> 26, function = word & 63;

  if (op >= 8 && op <= 15) return 1;
  if (op != 0) return 0;
  switch (function) {
    case 0: case 2: case 3: case 4: case 6: case 7:
    case 0x20: case 0x21: case 0x22: case 0x23:
    case 0x24: case 0x25: case 0x26: case 0x27:
    case 0x2a: case 0x2b:
      return 1;
    default:
      return 0;
  }
}
#endif

static int check_block(void *code, size_t size, int (*publish)(void *, size_t),
                       const uint32_t *words, unsigned count, unsigned seed)
{
  static const uint32_t edge[] = { 0, 1, 0xffffffff, 0x80000000, 0x7fffffff, 0x8000, 31, 32 };
  struct test_cpu actual, expected;
  unsigned i, j;
  uint32_t cycles = 0, expected_cycles = 0;
#ifdef ESP_PLATFORM
  (void)cycles; /* Host-independent build passes this to the linkage shim. */
#endif
  CHECK(compile(code, size, words, count) == 0);
  CHECK(publish(code, rv32_output.count * 4) == 0);
  for (j = 0; j < 16; j++) {
    memset(&actual, 0, sizeof(actual));
    memset(&expected, 0, sizeof(expected));
    for (i = 0; i < 32; i++) {
      seed = seed * 1664525u + 1013904223u;
      actual.gpr[i] = expected.gpr[i] = j < 8 ? edge[(i + j) % 8] : seed;
    }
    actual.gpr[0] = expected.gpr[0] = 0;
    TEST_PC(actual) = actual.exception = 0xdeadbeef;
    TEST_PC(expected) = count * 4; expected.exception = 0;
    cycles = expected_cycles = 0;
    for (i = 0; i < count; i++) {
      int ret;
#ifdef ESP_PLATFORM
      uint32_t oracle[32];
      unsigned reg;
      int oracle_supported = interpreter_alu_oracle_supported(words[i]);
      int oracle_status = 0;
      for (reg = 0; reg < 32; reg++) oracle[reg] = expected.gpr[reg];
      if (oracle_supported)
        oracle_status = ndrc_rv32_interpreter_alu(oracle, words[i]);
#endif
      ret = reference_step(expected.gpr, words[i]);
      CHECK(ret >= 0);
#ifdef ESP_PLATFORM
      if (oracle_supported) {
        CHECK(oracle_status == ret);
        for (reg = 0; reg < 32; reg++) CHECK(oracle[reg] == expected.gpr[reg]);
      }
#endif
      expected_cycles++;
      if (ret) { expected.exception = 1; TEST_PC(expected) = i * 4; break; }
    }
    TEST_CYCLE(actual) = 0;
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(TEST_CYCLE(actual) == expected_cycles);
    CHECK(TEST_PC(actual) == TEST_PC(expected) && actual.exception == expected.exception);
    for (i = 0; i < 32; i++) CHECK(actual.gpr[i] == expected.gpr[i]);
  }
  return 0;
}

static int compiler_selftest(void *code, size_t size, int (*publish)(void *, size_t))
{
  static const unsigned special[] = { 0, 2, 3, 4, 6, 7,
    0x10, 0x11, 0x12, 0x13, 0x18, 0x19, 0x1a, 0x1b,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2a, 0x2b };
  static const unsigned alu_special[] = { 0, 2, 3, 4, 6, 7,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2a, 0x2b };
  static const unsigned immediates[] = { 0, 1, 0x7ff, 0x800, 0x7fff, 0x8000, 0xffff };
  uint32_t words[TEST_INSNS];
  unsigned op, alias, zero, imm, i;
  int result;
  CHECK(code && size >= 16384 && publish);
  for (op = 0; op < sizeof(special) / sizeof(special[0]); op++)
    for (alias = 0; alias < 4; alias++) for (zero = 0; zero < 4; zero++) {
      unsigned s = zero & 1 ? 0 : 1, t = zero & 2 ? 0 : 2;
      words[0] = s << 21 | t << 16 | alias << 11 |
        (special[op] < 4 ? 31u << 6 : 0) | special[op];
      result = check_block(code, size, publish, words, 1, 0x31415926);
      if (result) return result;
    }
  /* HI/LO dependency and divide edge cases in one generated block. */
  {
    static const uint32_t pairs[][2] = {
      {0x80000000u, UINT32_MAX}, {0x80000000u, 0},
      {0xffffffffu, 0}, {0x7fffffffu, 3}, {0x89abcdefu, 0x12345u}
    };
    static const unsigned functions[] = {0x18, 0x19, 0x1a, 0x1b};
    for (unsigned function = 0; function < ARRAY_SIZE(functions); function++)
      for (unsigned pair = 0; pair < ARRAY_SIZE(pairs); pair++) {
        struct test_cpu actual = {0}, expected;
        uint32_t cycles = 0;
        words[0] = 1u << 21 | 2u << 16 | functions[function];
        words[1] = 0x00001812; /* mflo r3 */
        words[2] = 0x00002010; /* mfhi r4 */
        actual.gpr[1] = pairs[pair][0]; actual.gpr[2] = pairs[pair][1];
        expected = actual;
        CHECK(reference_step(expected.gpr, words[0]) == 0);
        CHECK(reference_step(expected.gpr, words[1]) == 0);
        CHECK(reference_step(expected.gpr, words[2]) == 0);
        TEST_PC(expected) = 12;
        CHECK(compile(code, size, words, 3) == 0);
        CHECK(publish(code, rv32_output.count * 4) == 0);
        TEST_CYCLE(actual) = 0;
        ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
        CHECK(TEST_PC(actual) == TEST_PC(expected));
        CHECK(actual.gpr[LOREG] == expected.gpr[LOREG]);
        CHECK(actual.gpr[HIREG] == expected.gpr[HIREG]);
        CHECK(actual.gpr[3] == expected.gpr[3] && actual.gpr[4] == expected.gpr[4]);
        if (functions[function] >= 0x1a)
          CHECK(TEST_CYCLE(actual) == 39u);
        else {
          uint32_t source = pairs[pair][0];
          uint32_t significant = functions[function] == 0x18 ?
            source ^ (uint32_t)((int32_t)source >> 21) : source;
          unsigned leading = (unsigned)__builtin_clz(significant | 1u);
          unsigned latency = 7u + (2u - leading / 11u) * 4u;
          CHECK(TEST_CYCLE(actual) == latency + 2u);
        }
      }
  }
  {
    struct test_cpu actual = {0};
    uint32_t cycles = 0;
    words[0] = 0x00200011; /* mthi r1 */
    words[1] = 0x00001810; /* mfhi r3 */
    words[2] = 0x00400013; /* mtlo r2 */
    words[3] = 0x00002012; /* mflo r4 */
    actual.gpr[1] = 0x89abcdefu;
    actual.gpr[2] = 0x12345678u;
    CHECK(compile(code, size, words, 4) == 0);
    CHECK(publish(code, rv32_output.count * 4) == 0);
    TEST_CYCLE(actual) = 0;
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(TEST_CYCLE(actual) == 4 && TEST_PC(actual) == 16);
    CHECK(actual.gpr[HIREG] == actual.gpr[1] && actual.gpr[3] == actual.gpr[1]);
    CHECK(actual.gpr[LOREG] == actual.gpr[2] && actual.gpr[4] == actual.gpr[2]);
  }
  for (op = 8; op <= 15; op++) for (alias = 0; alias < 3; alias++)
    for (zero = 0; zero < 2; zero++) for (imm = 0; imm < 7; imm++) {
      words[0] = op << 26 | zero << 21 | alias << 16 | immediates[imm];
      result = check_block(code, size, publish, words, 1, 0x27182818);
      if (result) return result;
    }
  for (op = 0; op < 3; op++) for (imm = 0; imm < 32; imm++) {
    words[0] = 1u << 16 | 3u << 11 | imm << 6 | (op ? op + 1 : 0);
    result = check_block(code, size, publish, words, 1, 7);
    if (result) return result;
  }
  /* Register dependencies across an actual multi-instruction generated block. */
  for (i = 0; i < 16; i++)
    words[i] = 9u << 26 | (i + 1) << 21 | (i + 2) << 16 | 0xffff;
  result = check_block(code, size, publish, words, 16, 42);
  if (result) return result;
  CHECK(cached_sources > 0);
  /* A live accumulator must need one final writeback, not 64 stores. */
  for (i = 0; i < TEST_INSNS; i++) words[i] = 0x24210001;
  result = check_block(code, size, publish, words, TEST_INSNS, 43);
  if (result) return result;
  CHECK(writebacks == 1 && folded_results == 0);
  /* Constant creation, propagation, wraparound and shift materialization. */
  words[0] = 0x3c01ffff; /* lui r1,ffff */
  words[1] = 0x3421ffff; /* ori r1,r1,ffff */
  words[2] = 0x24210002; /* addiu r1,r1,2 -> 1 */
  words[3] = 0x00010fc0; /* sll r1,r1,31 */
  result = check_block(code, size, publish, words, 4, 44);
  if (result) return result;
  CHECK(writebacks == 1 && folded_results == 4);
  /* More live guest registers than host slots; then reuse evicted values. */
  for (i = 0; i < TEST_INSNS; i++) {
    unsigned s = i % 31 + 1, t = (i + 11) % 31 + 1, d = (i + 21) % 31 + 1;
    words[i] = s << 21 | t << 16 | d << 11 | (i & 1 ? 0x26u : 0x21u);
  }
  result = check_block(code, size, publish, words, TEST_INSNS, 131);
  if (result) return result;
  CHECK(cached_sources > 0 && evictions > 0);
  /* Mix instruction classes under sustained register pressure, including zero
   * destinations, source/destination aliases and extreme immediate values. */
  for (unsigned block = 0; block < 128; block++) {
    uint32_t random = 0x9e3779b9u ^ block;
    for (i = 0; i < TEST_INSNS; i++) {
      random = random * 1664525u + 1013904223u;
      unsigned s = random >> 27, t = (random >> 22) & 31, d = (random >> 17) & 31;
      unsigned f = alu_special[(random >> 8) & 15];
      if (i & 1)
        words[i] = (8u + ((random >> 5) & 7)) << 26 | s << 21 | d << 16 |
          (random & 0xffff);
      else
        words[i] = s << 21 | t << 16 | d << 11 | (random & 31) << 6 | f;
    }
    result = check_block(code, size, publish, words, TEST_INSNS, block + 197);
    if (result) return result;
  }
  /* Precise overflow exits after full register pressure, even with r0 as dest. */
  for (unsigned zero_dest = 0; zero_dest < 2; zero_dest++) {
    for (i = 0; i < TEST_INSNS - 4; i++)
      words[i] = 9u << 26 | (i % 31 + 1) << 16 | (i + 1);
    words[TEST_INSNS - 4] = 0x3c017fff;
    words[TEST_INSNS - 3] = 0x3421ffff;
    words[TEST_INSNS - 2] = zero_dest ? 0x20200001 : 0x20220001;
    words[TEST_INSNS - 1] = 0x2403002a;
    result = check_block(code, size, publish, words, TEST_INSNS, 73);
    if (result) return result;
    CHECK(evictions > 0);
    CHECK(folded_results > 0);
  }
  /* Overflow after earlier committed writes: later instructions must not run. */
  words[0] = 0x3c017fff; /* lui r1,0x7fff */
  words[1] = 0x3421ffff; /* ori r1,r1,0xffff */
  words[2] = 0x20220001; /* addi r2,r1,1 -> overflow */
  words[3] = 0x2403002a; /* addiu r3,r0,42 (unreachable) */
  result = check_block(code, size, publish, words, 4, 73);
  if (result) return result;
  /* Overflow must preserve a dirty destination which aliases its input. */
  for (unsigned negative = 0; negative < 2; negative++) {
    words[0] = negative ? 0x3c018000 : 0x3c017fff;
    words[1] = negative ? 0x34210000 : 0x3421ffff;
    words[2] = negative ? 0x2021ffff : 0x20210001;
    words[3] = 0x2403002a;
    result = check_block(code, size, publish, words, 4, 74);
    if (result) return result;
  }
  /* A straight non-trapping ALU run pays one cycle-counter update, while its
   * architectural cycle total and dependent GPR results remain unchanged. */
  words[0] = 0x24010001; /* addiu r1,r0,1 */
  words[1] = 0x24220001; /* addiu r2,r1,1 */
  words[2] = 0x24430001; /* addiu r3,r2,1 */
  words[3] = 0x24640001; /* addiu r4,r3,1 */
  result = check_block(code, size, publish, words, 4, 75);
  if (result) return result;
  CHECK(cycle_adds_saved == 3);
  words[0] = 0x8c220000; /* LW is not implemented by this isolated harness. */
  CHECK(compile(code, size, words, 1) != 0);
  words[0] = 0x24210001;
  CHECK(compile(code, size, words, 0) != 0);
  CHECK(compile(code, size, words, TEST_INSNS + 1) != 0);
  ((uint32_t *)code)[1] = 0xdeadbeef;
  CHECK(compile(code, 4, words, 1) != 0);
  CHECK(((uint32_t *)code)[1] == 0xdeadbeef);
  return 0;
}

#ifdef ESP_PLATFORM
static int cop2_selftest(void *code, size_t size,
                         int (*publish)(void *, size_t))
{
  static const uint8_t commands[] = {
    0x01, 0x06, 0x0c, 0x10, 0x11, 0x12, 0x13, 0x14, 0x16,
    0x1b, 0x1c, 0x1e, 0x20, 0x28, 0x29, 0x2a, 0x2d, 0x2e,
    0x30, 0x3d, 0x3e, 0x3f
  };

  /* Exercise every valid command against the same established C handler.
   * This is primarily an RV32 call/state/timing test: the GTE algorithms
   * themselves already have their own implementation coverage. */
  for (unsigned i = 0; i < ARRAY_SIZE(commands); i++) {
    uint32_t word = 0x4a000000u | commands[i];
    struct test_cpu actual = {0}, expected = {0};
    uint32_t cycles = 0, saved_code = psxRegs.code;
    actual.cpu.CP0.n.SR = expected.cpu.CP0.n.SR = 1u << 30;
    for (unsigned r = 0; r < 32; r++) {
      actual.cpu.CP2D.r[r] = expected.cpu.CP2D.r[r] = r * 0x101u;
      actual.cpu.CP2C.r[r] = expected.cpu.CP2C.r[r] = r * 0x10001u;
    }
    /* Keep divider inputs and projection scale finite and deterministic. */
    actual.cpu.CP2C.p[26].w.l = expected.cpu.CP2C.p[26].w.l = 1;
    CHECK(compile(code, size, &word, 1) == 0);
    CHECK(publish(code, rv32_output.count * 4) == 0);
    expected.cpu.cycle = 1;
    if (!Config.DisableStalls)
      expected.cpu.gteBusyCycle = 1 + gte_cycletab[commands[i]];
    psxRegs.code = word;
    psxCP2[commands[i]](&expected.cpu.CP2);
    psxRegs.code = saved_code;
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(actual.cpu.pc == 4 && actual.cpu.cycle == expected.cpu.cycle);
    CHECK(actual.cpu.gteBusyCycle == expected.cpu.gteBusyCycle);
    CHECK(memcmp(&actual.cpu.CP2, &expected.cpu.CP2,
      sizeof(actual.cpu.CP2)) == 0);
  }

  /* The first command's FLAG is dead because the following command replaces
   * it before any CFC2 read. Its no-FLAG handler must remain architecturally
   * identical once the second, full command completes. The surrounding ALU
   * operations also verify that a dirty callee-saved GPR mapping survives both
   * GTE helper calls without a writeback/reload barrier. */
  {
    const uint32_t words[] = {
      0x2402002au, /* ADDIU r2,r0,42 */
      0x4a000001u, /* RTPS */
      0x24430001u, /* ADDIU r3,r2,1 */
      0x4a000030u, /* RTPT */
    };
    struct test_cpu actual = {0}, expected = {0};
    uint32_t cycles = 0, saved_code = psxRegs.code;
    uint32_t no_flags_before = rv32_gte_profile.no_flags_calls;
    actual.cpu.CP0.n.SR = expected.cpu.CP0.n.SR = 1u << 30;
    for (unsigned r = 0; r < 32; r++) {
      actual.cpu.CP2D.r[r] = expected.cpu.CP2D.r[r] = r * 0x101u;
      actual.cpu.CP2C.r[r] = expected.cpu.CP2C.r[r] = r * 0x10001u;
    }
    actual.cpu.CP2C.p[26].w.l = expected.cpu.CP2C.p[26].w.l = 1;
    CHECK(compile(code, size, words, ARRAY_SIZE(words)) == 0);
    CHECK(publish(code, rv32_output.count * 4) == 0);
    for (unsigned i = 0; i < ARRAY_SIZE(words); i++) {
      unsigned op = words[i] >> 26;
      expected.cpu.cycle++;
      if (op == 0x12 && ((words[i] >> 21) & 0x10)) {
        unsigned command = words[i] & 63;
        if (!Config.DisableStalls) {
          uint32_t left = expected.cpu.gteBusyCycle - expected.cpu.cycle;
          if (left <= 44) expected.cpu.cycle = expected.cpu.gteBusyCycle;
          expected.cpu.gteBusyCycle = expected.cpu.cycle +
            gte_cycletab[command];
        }
        psxRegs.code = words[i];
        psxCP2[command](&expected.cpu.CP2);
      }
      else
        CHECK(reference_step(expected.gpr, words[i]) == 0);
    }
    psxRegs.code = saved_code;
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(actual.cpu.pc == sizeof(words) &&
      actual.cpu.cycle == expected.cpu.cycle);
    CHECK(memcmp(&actual.cpu.CP2, &expected.cpu.CP2,
      sizeof(actual.cpu.CP2)) == 0);
    CHECK(actual.gpr[2] == 42 && actual.gpr[3] == 43);
    CHECK(rv32_gte_profile.no_flags_calls == no_flags_before + 1);
  }

  /* CPU<->GTE moves include a real MFC2 load delay. */
  {
    const uint32_t words[] = {
      0x48820800, /* MTC2 r2,c2d1 */
      0x48030800, /* MFC2 r3,c2d1 */
      0x24640001, /* ADDIU r4,r3,1 observes old r3 */
      0
    };
    struct test_cpu actual = {0};
    uint32_t cycles = 0;
    actual.cpu.CP0.n.SR = 1u << 30;
    actual.gpr[2] = 0x00008001u;
    actual.gpr[3] = 9;
    CHECK(compile(code, size, words, ARRAY_SIZE(words)) == 0);
    CHECK(publish(code, rv32_output.count * 4) == 0);
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(actual.cpu.pc == sizeof(words) && actual.cpu.cycle == 4);
    /* MFC2 sign-extends this register and publishes the transformed value
     * back into CP2D as well as into the delayed GPR destination. */
    CHECK(actual.cpu.CP2D.r[1] == 0xffff8001u);
    CHECK(actual.gpr[3] == 0xffff8001u && actual.gpr[4] == 10);
    CHECK(LOAD_DELAY_CLEAR(actual));
  }

  /* LWC2/SWC2 use private LUT-backed RAM and the GTE register semantics,
   * proving that generated helper calls do not touch live device handlers. */
  {
    const uint32_t words[] = {
      0xc8260000, /* LWC2 c2d6,0(r1) */
      0xe8260004, /* SWC2 c2d6,4(r1) */
      0
    };
    struct test_cpu actual = {0};
    uintptr_t lut[1] = {(uintptr_t)actual.ram};
    uint32_t cycles = 0;
    actual.cpu.ptrs.memRLUT = lut;
    actual.cpu.ptrs.memWLUT = lut;
    actual.ram[0] = 0x12345678u;
    CHECK(compile(code, size, words, ARRAY_SIZE(words)) == 0);
    CHECK(publish(code, rv32_output.count * 4) == 0);
    ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
    CHECK(actual.cpu.pc == sizeof(words) && actual.cpu.cycle == 3);
    CHECK(actual.cpu.CP2D.r[6] == 0x12345678u);
    CHECK(actual.ram[1] == 0x12345678u);
  }
  return 0;
}

static int cop0_selftest(void *code, size_t size,
                         int (*publish)(void *, size_t))
{
  const uint32_t words[] = {
    0x40027800u, /* MFC0 r2,PRId */
    0x24430001u, /* ADDIU r3,r2,1 observes old r2 */
    0x40843000u, /* MTC0 r4,Target */
    0x42000010u  /* RFE: terminal control-sensitive operation */
  };
  struct test_cpu actual = {0};
  uint32_t cycles = 0;

  actual.cpu = psxRegs;
  actual.cpu.pc = 0x80001000u;
  actual.cpu.cycle = 0;
  actual.cpu.CP0.n.PRid = 2;
  actual.cpu.CP0.n.SR = (actual.cpu.CP0.n.SR & ~0x3fu) | 0x3cu;
  actual.cpu.CP0.n.Cause &= ~0x0300u;
  actual.gpr[2] = 9;
  actual.gpr[3] = 0xdeadbeefu;
  actual.gpr[4] = 0x12345678u;
  linear_pc = actual.cpu.pc;
  CHECK(compile(code, size, words, ARRAY_SIZE(words)) == 0);
  CHECK(publish(code, rv32_output.count * 4) == 0);
  ndrc_rv32_enter(code, &actual, TEST_CYCLE_PTR(actual, &cycles));
  CHECK(actual.cpu.pc == 0x80001000u + sizeof(words));
  CHECK(actual.cpu.cycle == ARRAY_SIZE(words));
  CHECK(actual.gpr[2] == 2 && actual.gpr[3] == 10);
  CHECK(actual.cpu.CP0.n.Target == 0x12345678u);
  /* RFE replaces bits 0..3 from the prior mode stack while preserving the
   * current mode bits 4..5: 0x3c therefore becomes 0x3f, not 0x0f. */
  CHECK((actual.cpu.CP0.n.SR & 0x3fu) == 0x3fu);
  CHECK(LOAD_DELAY_CLEAR(actual));
  linear_pc = 0;
  return 0;
}
#endif

int ndrc_rv32_compiler_selftest(void *code, size_t size, int (*publish)(void *, size_t))
{
  int result;
#ifdef ESP_PLATFORM
  work = rg_alloc(sizeof(*work), MEM_SLOW | MEM_NOPANIC);
  if (!work) return __LINE__;
#else
  static struct compiler_workspace storage;
  work = &storage;
#endif
  memset(work, 0, sizeof(*work));
  result = compiler_selftest(code, size, publish);
  if (!result) result = long_guard_selftest(code, size, publish);
#ifdef ESP_PLATFORM
  if (!result) result = safe_capped_prefix_selftest();
#endif
  if (!result) result = branch_selftest(code, size, publish);
  if (!result) result = mapped_branch_selftest(code, size, publish);
  if (!result) result = multiblock_selftest(code, size, publish);
  if (!result) {
    memory_mode = 1;
    result = memory_selftest(code, size, publish);
    if (!result) result = branch_store_delay_selftest(code, size, publish);
#ifdef ESP_PLATFORM
    if (!result) result = mapped_memory_selftest(code, size, publish);
    if (!result) {
      runtime_exit_mode = full_address_mode = live_hardware_memory = 1;
      test_memory_helper = (uintptr_t)delay_mmio_fixture;
      result = delay_mmio_selftest(code, size, publish);
      test_memory_helper = 0;
      runtime_exit_mode = full_address_mode = live_hardware_memory = 0;
    }
    if (!result) result = cop2_selftest(code, size, publish);
    if (!result) result = cop0_selftest(code, size, publish);
#endif
    memory_mode = 0;
  }
#ifdef ESP_PLATFORM
  free(work);
#endif
  work = NULL;
  return result;
}

#ifdef ESP_PLATFORM
#ifdef PCSX_NDRC_RV32_LIVE_CANARY
#define LIVE_CANARY_MAX_PATHS 8
#define LIVE_CANARY_MAX_LINKS 32
#define LIVE_CANARY_PREFIX_WORDS TEST_INSNS
#define LIVE_CANARY_FETCH_WORDS (LIVE_CANARY_PREFIX_WORDS + 2)

struct live_canary_link {
  uint32_t source_pc, target_pc;
  uint32_t exit_pc[LIVE_CANARY_MAX_PATHS];
  unsigned path_words[LIVE_CANARY_MAX_PATHS];
  unsigned path_blocks[LIVE_CANARY_MAX_PATHS], path_count;
  uint32_t event_pc[LIVE_CANARY_MAX_PATHS];
  unsigned event_words[LIVE_CANARY_MAX_PATHS];
  unsigned event_blocks[LIVE_CANARY_MAX_PATHS], event_count;
  void *source_code;
  unsigned dispatcher_offset;
};

struct live_canary_edge {
  uint32_t source_pc, target_pc;
  unsigned ready;
};

static struct {
  void *allocation;
  void *arena;
  void *code;
  struct compiler_workspace *workspace;
  struct ndrc_rv32_block_cache cache;
  struct ndrc_rv32_cache_entry *entries;
  /* Exact main-RAM word slots hold an executable pointer tagged with pc[31:29]
   * in its low three bits. Block starts are 64-byte aligned, so the tag costs
   * no address bits and distinguishes the three PSX RAM aliases. */
  uintptr_t *dispatch_table;
  struct live_canary_link *links, *link_backup;
  uint32_t pc, words[LIVE_CANARY_FETCH_WORDS], word_count, code_bytes, exit_pc[2];
  struct live_canary_edge edges[LIVE_CANARY_MAX_LINKS];
  unsigned indirect_reg, indirect_dynamic, has_terminal_branch;
  unsigned has_terminal_cop0;
  unsigned has_terminal_load;
  unsigned linked_active, last_linked_blocks;
  unsigned scans, executions, unsupported, hits, misses, chained;
  unsigned direct_calls;
  unsigned stale_link_skips;
  unsigned links_published, discovery_continuations, direct_discoveries;
  unsigned general_active, general_attempts, general_calls, general_blocks;
  unsigned general_fallbacks, general_unsupported_fallbacks;
  unsigned general_event_deferrals, general_stop_deferrals;
  unsigned general_load_deferrals;
  unsigned general_compile_fallbacks, general_runtime_fallbacks;
  unsigned general_overflow_fallbacks, general_trapping_alu_admissions;
  unsigned general_max_chain, cache_resets;
  unsigned general_reject_opcode, general_reject_delay;
  unsigned general_reject_terminal_load, general_duplicate_load_rejections;
  unsigned general_duplicate_split_admissions;
  unsigned general_duplicate_sample_logged, general_store_delay_admissions;
  unsigned general_empty_branch_admissions;
  unsigned general_straight_admissions;
  unsigned general_terminal_load_blocks;
  unsigned general_load_branch_blocks;
  unsigned general_capped_blocks, general_capped_load_blocks;
  unsigned general_capped_load_splits;
  unsigned general_fast_cache_hits;
  unsigned general_fallback_store_invalidations;
  unsigned general_hit_base, general_miss_base;
  unsigned general_native_cycles, general_links_published;
  unsigned general_linked_blocks;
  unsigned general_linked_event_exits;
  unsigned general_native_returns;
  unsigned general_dynamic_blocks, general_dynamic_words;
  unsigned general_dynamic_misses;
  unsigned general_inline_blocks;
  unsigned general_compiled_memory_retained;
  unsigned general_compiled_memory_sources;
  unsigned general_compiled_branch_mappings;
  unsigned general_compiled_cycle_adds_saved;
  unsigned general_compiled_ram_fastpaths;
  unsigned general_progress_bucket;
  uint32_t profile_rng, profile_lookup_calls, profile_lookup_samples;
  uint64_t profile_lookup_cycles, profile_native_us;
  unsigned general_timing_adjustments, general_timing_cycles;
  unsigned general_interpreter_fallbacks, general_interpreter_cycles;
  unsigned general_hle_calls, general_hle_cycles;
  /* Cumulative instruction mix at native-to-interpreter boundaries.  The
   * primary opcode alone is insufficient for SPECIAL and COP2, so retain
   * their low-six-bit subfunctions as separate histograms. */
  unsigned general_fallback_opcode[64];
  unsigned general_fallback_special[64];
  unsigned general_fallback_cop0[64];
  unsigned general_fallback_cop2[64];
  unsigned general_interpreter_active;
  unsigned general_bridge_pending;
  volatile unsigned state; /* 0 unavailable, 1 armed, 2 consumed, 3 rejected, 4 waiting, 5 general */
} live_canary;

#define LIVE_CANARY_MAX_SCANS 512
#define LIVE_CANARY_MAX_EXECUTIONS 16
#define LIVE_CANARY_GENERAL_MAX_BLOCKS 0x40000000u
#define LIVE_CANARY_GENERAL_MAX_ATTEMPTS 0xffffffffu
#define LIVE_CANARY_GENERAL_CALL_BLOCKS 512
/* This counter is guest cycles, not native block transitions.  A 1M-cycle
 * interval floods the comparatively slow serial logger several times per
 * second and measurably perturbs emulation.  Keep detailed telemetry sparse
 * enough to remain useful without turning it into part of the workload. */
#define LIVE_CANARY_GENERAL_PROGRESS_CYCLES 0x02000000u
#define LIVE_CANARY_GENERAL_DETAIL_BLOCKS 16
#define LIVE_CANARY_BLOCK_RESERVE 24576
/* Measurement is deliberately roomier than the compact cached layout.  The
 * complete memory family plus HI/LO barriers produced a real 18-op Ridge Racer
 * prefix at exactly the former 3,328-byte ceiling. Prefixes may now span the
 * restricted compiler's full 64 guest instructions so a supported load at an
 * artificial boundary does not force an interpreter handoff. Successful
 * blocks still return unused reservation space before insertion, so this
 * changes miss-time scratch capacity rather than steady-state bytes per cached
 * block. */
#define LIVE_CANARY_MEASURE_BRANCH_OFFSET 16384
#define LIVE_CANARY_MEASURE_DISPATCH_OFFSET 24064
#define LIVE_CANARY_ARENA_PREFERRED_SIZE (8u * 1024u * 1024u)
#define LIVE_CANARY_ARENA_FALLBACK_SIZE (4u * 1024u * 1024u)
#define LIVE_CANARY_CACHE_ENTRIES 4093u
#define LIVE_CANARY_DISPATCH_ENTRIES (0x200000u / 4u)
static inline uint32_t live_canary_cycle_count(void)
{
  uint32_t cycles;
  __asm__ volatile("csrr %0, mcycle" : "=r"(cycles));
  return cycles;
}

static void live_canary_top4(const unsigned counts[64], unsigned top[4])
{
  top[0] = top[1] = top[2] = top[3] = 64;
  for (unsigned i = 0; i < 64; i++) {
    for (unsigned rank = 0; rank < 4; rank++) {
      if (top[rank] == 64 || counts[i] > counts[top[rank]]) {
        for (unsigned move = 3; move > rank; move--)
          top[move] = top[move - 1];
        top[rank] = i;
        break;
      }
    }
  }
}

static int live_canary_source_page(uint32_t address)
{
  uint32_t physical = address & 0x1fffffffu;
  if (physical < 0x200000u) return (int)(physical >> 12);
  if ((physical & ~0xfffu) == 0x1f800000u) return LIVE_CANARY_RAM_PAGES;
  return -1;
}

static uint32_t live_canary_page_generation(uint32_t address)
{
  int page = live_canary_source_page(address);
  return page < 0 ? 0 : live_canary_source_generation[page];
}

static void live_canary_clear_dispatch_page(unsigned page)
{
  /* All callers, including DMA/CPU Clear and interpreter notifications, must
   * skip empty pages. After clearing once, no slot can be published for this
   * page (including a crossing block) without stamp_entry setting this bit. */
  if (page >= LIVE_CANARY_SOURCE_PAGES ||
      !live_canary_compiled_page[page]) return;
  /* A changed word can invalidate a block beginning up to one less than the
   * largest published block earlier. Clear this page's starts plus that
   * overlap tail from the preceding page. */
  if (page < LIVE_CANARY_RAM_PAGES && live_canary.dispatch_table) {
    const unsigned per_page = 4096u / 4u;
    const unsigned overlap = LIVE_CANARY_FETCH_WORDS - 1;
    memset(live_canary.dispatch_table + page * per_page, 0,
      per_page * sizeof(*live_canary.dispatch_table));
    if (page)
      memset(live_canary.dispatch_table + page * per_page - overlap, 0,
        overlap * sizeof(*live_canary.dispatch_table));
  }
  if (page < LIVE_CANARY_SOURCE_PAGES)
    live_canary_compiled_page[page] = 0;
}

static void live_canary_invalidate_page(unsigned page)
{
  if (page >= LIVE_CANARY_SOURCE_PAGES) return;
  if (++live_canary_source_generation[page] == 0)
    live_canary_source_generation[page] = 1;
  live_canary_clear_dispatch_page(page);
}

static void live_canary_invalidate_write(uint32_t address, unsigned bytes)
{
  int first = live_canary_source_page(address);
  int last = live_canary_source_page(address + (bytes ? bytes - 1 : 0));
  if (first >= 0) live_canary.general_fallback_store_invalidations++;
  if (first >= 0) live_canary_invalidate_page((unsigned)first);
  if (last >= 0 && last != first)
    live_canary_invalidate_page((unsigned)last);
}

static int live_canary_entry_fresh(const struct ndrc_rv32_cache_entry *entry)
{
  uint32_t end = entry->pc + entry->guest_bytes - 1;
  uint32_t first = live_canary_page_generation(entry->pc);
  uint32_t last = live_canary_page_generation(end);
  /* ROM/other untracked mappings retain the source hash guard. */
  if (!first || !last) {
    uint32_t words[LIVE_CANARY_FETCH_WORDS];
    unsigned count = entry->guest_bytes / 4;
    if (!count || count > ARRAY_SIZE(words)) return 0;
    for (unsigned i = 0; i < count; i++)
      words[i] = intFakeFetch(entry->pc + i * 4);
    return ndrc_rv32_source_hash(words, count) == entry->source_hash;
  }
  return entry->source_generation[0] == first &&
    entry->source_generation[1] == last;
}

static void live_canary_stamp_entry(uint32_t pc)
{
  struct ndrc_rv32_cache_entry *entry = (struct ndrc_rv32_cache_entry *)
    ndrc_rv32_cache_lookup_pc(&live_canary.cache, pc);
  if (!entry) return;
  entry->source_generation[0] = live_canary_page_generation(pc);
  entry->source_generation[1] =
    live_canary_page_generation(pc + entry->guest_bytes - 1);
  {
    int first = live_canary_source_page(pc);
    int last = live_canary_source_page(pc + entry->guest_bytes - 1);
    if (first >= 0) live_canary_compiled_page[first] = 1;
    if (last >= 0) live_canary_compiled_page[last] = 1;
  }
  if (live_canary.dispatch_table &&
      (pc & 0x1fffffffu) < 0x200000u) {
    uintptr_t code = (uintptr_t)live_canary.arena + entry->code_offset;
    if (!(code & 7u))
      live_canary.dispatch_table[(pc & 0x1fffffffu) >> 2] =
        code | (pc >> 29);
  }
}

void ndrc_rv32_live_canary_invalidate(uint32_t address, uint32_t bytes)
{
  uint32_t physical = address & 0x1fffffffu;
  uint64_t end;
  if (!bytes) return;
  end = (uint64_t)physical + bytes - 1;
  if (physical < 0x200000u) {
    unsigned first = physical >> 12;
    unsigned last = end < 0x200000u ? (unsigned)(end >> 12) :
      LIVE_CANARY_RAM_PAGES - 1;
    for (unsigned page = first; page <= last; page++)
      live_canary_invalidate_page(page);
  }
  else if ((physical & ~0xfffu) == 0x1f800000u && end < 0x1f801000u) {
    live_canary_invalidate_page(LIVE_CANARY_RAM_PAGES);
  }
}

void ndrc_rv32_live_canary_invalidate_all(void)
{
  for (unsigned page = 0; page < LIVE_CANARY_SOURCE_PAGES; page++)
    live_canary_invalidate_page(page);
}

static const struct ndrc_rv32_cache_entry *live_canary_cached_pc(uint32_t pc)
{
  const struct ndrc_rv32_cache_entry *entry =
    ndrc_rv32_cache_lookup_pc(&live_canary.cache, pc);
  if (!entry || (entry->guest_bytes & 3)) return NULL;
  return live_canary_entry_fresh(entry) ? entry : NULL;
}

/* PCSX opcode 0x3b is a private HLE trap, not a guest R3000A instruction.
 * Match Lightrec and the original new_dynarec by advancing over the trap and
 * calling its registered host handler directly. The handler may recursively
 * use ExecuteBlock; suppress this experimental dispatcher during that nested
 * call so the established core owns it. A real pending load still goes through
 * execI, whose dloadStep/dloadFlush ordering remains authoritative. */
static int live_canary_hle_call(psxRegisters *cpu, uint32_t word)
{
  unsigned code = word & 0x03ffffffu;
  uint32_t before_cycle;
  if ((word >> 26) != 0x3b || !Config.HLE || code >= hleop_count_ ||
      cpu->dloadReg[0] || cpu->dloadReg[1]) return 0;

  before_cycle = cpu->cycle;
  cpu->cycle++;
  cpu->pc += 4;
  cpu->code = word;
  live_canary.general_interpreter_active = 1;
  psxHLEt[code]();
  live_canary.general_interpreter_active = 0;
  cpu->branchSeen = 1;
  live_canary.general_hle_calls++;
  live_canary.general_hle_cycles += cpu->cycle - before_cycle;
  return 1;
}

/* Conservative generated-to-generated dispatch. Every target is looked up by
 * exact PC and rehashed before it is returned, so an unknown or modified block
 * falls back to the normal C compiler/interpreter bridge. The generated
 * dispatcher has already stored the live cycle before entering this helper. */
static void *live_canary_dispatch_lookup(uint32_t pc)
{
  /* Sample irregularly to avoid repeatedly selecting one PC in a hot loop.
   * Only about one lookup in 1024 pays for two platform timer reads. */
  live_canary.profile_rng = live_canary.profile_rng * 1664525u + 1013904223u;
  int sample = (live_canary.profile_rng >> 22) == 0;
  uint32_t started = sample ? live_canary_cycle_count() : 0;
  const struct ndrc_rv32_cache_entry *entry = live_canary_cached_pc(pc);
  if (sample) {
    live_canary.profile_lookup_cycles +=
      (uint32_t)(live_canary_cycle_count() - started);
    live_canary.profile_lookup_samples++;
  }
  live_canary.profile_lookup_calls++;
  if (!entry) {
    live_canary.general_dynamic_misses++;
    return NULL;
  }
  live_canary.general_dynamic_blocks++;
  live_canary.general_dynamic_words += entry->guest_bytes / 4;
  return (unsigned char *)live_canary.arena + entry->code_offset;
}

static int live_canary_relink(void *source_code, unsigned dispatcher_offset,
    uint32_t source_pc, void *target_code, uint32_t target_pc,
    int dynamic_dispatch)
{
  uint32_t dispatcher[64];
  size_t event_exit, self_miss = 0, link_miss = 0, dynamic_miss = 0;
  size_t direct_miss[8];
  unsigned direct_miss_count = 0;
  size_t check_link = 0, leave;
  rv32_init(&rv32_output, dispatcher, sizeof(dispatcher));
  rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0,
    offsetof(struct test_cpu, cpu.next_interupt));
  rv32_alu(&rv32_output, RV_SUB, RV_A2, RV_S1, RV_A0);
  event_exit = rv32_output.count;
  rv32_branch(&rv32_output, RV_GE, RV_A2, RV_ZERO, 0);
  if (dynamic_dispatch) {
    rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0,
      offsetof(struct test_cpu, cpu.pc));
    /* Main-RAM targets use an authoritative instruction-address table.
     * Source-page writes clear all possibly overlapping slots before another
     * dispatch. The low pointer bits carry pc[31:29], distinguishing physical,
     * KSEG0 and KSEG1 aliases without following PSRAM metadata. Cache resets
     * clear the table before reusing executable arena storage. */
    rv32_li(&rv32_output, RV_A1, 0x1fffffffu);
    rv32_alu(&rv32_output, RV_AND, RV_A1, RV_A0, RV_A1);
    rv32_li(&rv32_output, RV_A2, 0x200000u);
    direct_miss[direct_miss_count++] = rv32_output.count;
    rv32_branch(&rv32_output, RV_GEU, RV_A1, RV_A2, 0);
    rv32_li(&rv32_output, RV_T6,
      (uint32_t)(uintptr_t)live_canary.dispatch_table);
    rv32_alu(&rv32_output, RV_ADD, RV_A2, RV_T6, RV_A1);
    rv32_load(&rv32_output, RV_WORD, RV_A2, RV_A2, 0);
    direct_miss[direct_miss_count++] = rv32_output.count;
    rv32_branch(&rv32_output, RV_EQ, RV_A2, RV_ZERO, 0);
    rv32_imm(&rv32_output, RV_AND, RV_A3, RV_A2, 7);
    rv32_shift(&rv32_output, RV_SRL, RV_A4, RV_A0, 29);
    direct_miss[direct_miss_count++] = rv32_output.count;
    rv32_branch(&rv32_output, RV_NE, RV_A3, RV_A4, 0);
    rv32_imm(&rv32_output, RV_AND, RV_A2, RV_A2, -8);
    rv32_jalr(&rv32_output, RV_ZERO, RV_A2, 0);
    check_link = rv32_output.count;
    for (unsigned i = 0; i < direct_miss_count; i++)
      rv32_patch_relative(&rv32_output, direct_miss[i],
        (int32_t)((check_link - direct_miss[i]) * 4));
    /* Canonical cycle is only required before C observes state. Direct hits
     * retain it in s1 and linkage publishes it when the native chain exits. */
    rv32_store(&rv32_output, RV_WORD, RV_S1, RV_S0,
      offsetof(struct test_cpu, cpu.cycle));
    rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0,
      offsetof(struct test_cpu, cpu.pc));
    rv32_abs_jump(&rv32_output, RV_RA, RV_T6,
      (uint32_t)(uintptr_t)live_canary_dispatch_lookup);
    dynamic_miss = rv32_output.count;
    rv32_branch(&rv32_output, RV_EQ, RV_A0, RV_ZERO, 0);
    rv32_jalr(&rv32_output, RV_ZERO, RV_A0, 0);
  }
  else {
    rv32_load(&rv32_output, RV_WORD, RV_A0, RV_S0,
      offsetof(struct test_cpu, cpu.pc));
    rv32_li(&rv32_output, RV_A1, source_pc);
    self_miss = rv32_output.count;
    rv32_branch(&rv32_output, RV_NE, RV_A0, RV_A1, 0);
    rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6,
      (uint32_t)(uintptr_t)source_code);
    check_link = rv32_output.count;
    if (target_code) {
      rv32_li(&rv32_output, RV_A1, target_pc);
      link_miss = rv32_output.count;
      rv32_branch(&rv32_output, RV_NE, RV_A0, RV_A1, 0);
      rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6,
        (uint32_t)(uintptr_t)target_code);
    }
  }
  leave = rv32_output.count;
  rv32_patch_relative(&rv32_output, event_exit,
    (int32_t)((leave - event_exit) * 4));
  if (dynamic_dispatch)
    rv32_patch_relative(&rv32_output, dynamic_miss,
      (int32_t)((leave - dynamic_miss) * 4));
  else {
    rv32_patch_relative(&rv32_output, self_miss,
      (int32_t)((check_link - self_miss) * 4));
    if (target_code)
      rv32_patch_relative(&rv32_output, link_miss,
        (int32_t)((leave - link_miss) * 4));
  }
  rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_ZERO, 0);
  rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6,
    (uint32_t)(uintptr_t)ndrc_rv32_leave);
  if (rv32_output.error != RV_OK || !dispatcher_offset ||
      rv32_output.count * 4 > 256) return -1;
  memcpy((unsigned char *)source_code + dispatcher_offset,
    dispatcher, rv32_output.count * 4);
  return ndrc_rv32_p4_publish((unsigned char *)source_code +
    dispatcher_offset, rv32_output.count * 4);
}

static int live_canary_drop_links(void)
{
  for (unsigned i = 0; i < LIVE_CANARY_MAX_LINKS; i++) {
    struct live_canary_link *link = &live_canary.links[i];
    if (link->source_code && live_canary_relink(link->source_code,
        link->dispatcher_offset, link->source_pc, NULL, 0, 0) != 0) return -1;
  }
  memset(live_canary.links, 0,
    LIVE_CANARY_MAX_LINKS * sizeof(*live_canary.links));
  memset(live_canary.edges, 0, sizeof(live_canary.edges));
  live_canary.links_published = 0;
  live_canary.linked_active = 0;
  return 0;
}

static int live_canary_safe_alu(uint32_t word)
{
  unsigned op = word >> 26, f = word & 63;
  return (op >= 9 && op <= 15) || (!op &&
    (f == 0 || f == 2 || f == 3 || f == 4 || f == 6 || f == 7 ||
     f == 0x21 || (f >= 0x23 && f <= 0x27) || f == 0x2a || f == 0x2b));
}

static int live_canary_trapping_alu(uint32_t word)
{
  unsigned op = word >> 26, f = word & 63;
  return op == 8 || (!op && (f == 0x20 || f == 0x22));
}

static int live_canary_hilo(uint32_t word)
{
  unsigned op = word >> 26, f = word & 63;
  return !op && ((f >= 0x10 && f <= 0x13) ||
                 (f >= 0x18 && f <= 0x1b));
}

static int live_canary_memory(uint32_t word)
{
  unsigned op = word >> 26;
  return memory_load_opcode(op) || memory_store_opcode(op) ||
    op == 0x32 || op == 0x3a;
}

static int live_canary_store(uint32_t word)
{
  unsigned op = word >> 26;
  return memory_store_opcode(op) || op == 0x3a;
}

static int live_canary_cached_terminal_branch(const uint32_t *words,
    unsigned count)
{
  unsigned op, funct;
  if (count < 2) return 0;
  op = words[count - 2] >> 26;
  funct = words[count - 2] & 63;
  return op == 1 || (op >= 2 && op <= 7) ||
    (!op && (funct == 8 || funct == 9));
}

static unsigned live_canary_written_regs(const uint32_t *words,
    unsigned count)
{
  unsigned written = 0;
  for (unsigned i = 0; i < count; i++) {
    unsigned op = words[i] >> 26;
    unsigned dst = 0;
    if (live_canary_safe_alu(words[i]) || live_canary_trapping_alu(words[i]))
      dst = op ? (words[i] >> 16) & 31 : (words[i] >> 11) & 31;
    else if (live_canary_hilo(words[i]) &&
             ((words[i] & 63) == 0x10 || (words[i] & 63) == 0x12))
      dst = (words[i] >> 11) & 31;
    else if (delayed_load_word(words[i]))
      dst = (words[i] >> 16) & 31;
    if (dst) written |= 1u << dst;
  }
  return written;
}

static int live_canary_prefix_op(uint32_t word)
{
  return live_canary_safe_alu(word) || live_canary_trapping_alu(word) ||
    live_canary_hilo(word) || live_canary_memory(word) || cop0_word(word) ||
    cop2_word(word);
}

static void live_canary_static_exits(uint32_t pc, unsigned count,
    uint32_t branch, uint32_t exits[2])
{
  unsigned op = branch >> 26;
  uint32_t branch_pc = pc + (count - 2) * 4;
  exits[0] = pc + count * 4;
  exits[1] = branch_pc + 4 + ((int32_t)(int16_t)branch << 2);
  if (op == 2 || op == 3)
    exits[0] = exits[1] = ((branch_pc + 4) & 0xf0000000u) |
      ((branch & 0x03ffffffu) << 2);
}

/* Cached general blocks are either a terminal primary branch plus its delay
 * slot, or a straight safe prefix which hands off at pc + guest_bytes. Keep
 * this classification derived from source bytes so cache entries need no
 * diagnostic-only format change. Indirect branches remain unlinked. */
static unsigned live_canary_block_exits(uint32_t pc, unsigned count,
    const uint32_t *words, uint32_t exits[2])
{
  unsigned op = count >= 2 ? words[count - 2] >> 26 : 0;
  unsigned funct = count >= 2 ? words[count - 2] & 63 : 0;
  if (op == 1 || (op >= 2 && op <= 7)) {
    live_canary_static_exits(pc, count, words[count - 2], exits);
    return 2;
  }
  if (!op && (funct == 8 || funct == 9)) return 0;
  exits[0] = pc + count * 4;
  exits[1] = exits[0];
  return 1;
}

/* Rebuild every exit oracle from the current acyclic one-successor graph. A
 * path count describes blocks after the link's source; the source block itself
 * is accounted by the caller. */
static int live_canary_general_build_paths(unsigned active, unsigned visiting,
    unsigned depth)
{
  struct live_canary_link *link = &live_canary.links[active];
  const struct ndrc_rv32_cache_entry *source, *target;
  const struct live_canary_link *next = NULL;
  uint32_t source_words[LIVE_CANARY_FETCH_WORDS];
  uint32_t target_words[LIVE_CANARY_FETCH_WORDS];
  uint32_t source_exit[2], target_exit[2];
  uint32_t exits[LIVE_CANARY_MAX_PATHS];
  unsigned words[LIVE_CANARY_MAX_PATHS], blocks[LIVE_CANARY_MAX_PATHS];
  uint32_t event_pc[LIVE_CANARY_MAX_PATHS];
  unsigned event_words[LIVE_CANARY_MAX_PATHS];
  unsigned event_blocks[LIVE_CANARY_MAX_PATHS];
  unsigned source_count, target_count, source_exits, target_exits, count = 0;
  unsigned event_count = 1;
  int reaches_target = 0;

  if (active >= LIVE_CANARY_MAX_LINKS || depth >= LIVE_CANARY_MAX_PATHS ||
      (visiting & (1u << active))) return -1;
  visiting |= 1u << active;
  source = live_canary_cached_pc(link->source_pc);
  target = live_canary_cached_pc(link->target_pc);
  if (!source || !target || link->source_code !=
      (unsigned char *)live_canary.arena + source->code_offset) return -1;
  source_count = source->guest_bytes / 4;
  target_count = target->guest_bytes / 4;
  if (!source_count || source_count > ARRAY_SIZE(source_words) ||
      !target_count || target_count > ARRAY_SIZE(target_words)) return -1;
  for (unsigned i = 0; i < source_count; i++)
    source_words[i] = intFakeFetch(link->source_pc + i * 4);
  for (unsigned i = 0; i < target_count; i++)
    target_words[i] = intFakeFetch(link->target_pc + i * 4);
  if (cop0_control_word(source_words[source_count - 1]) ||
      cop0_control_word(target_words[target_count - 1])) return -1;
  source_exits = live_canary_block_exits(link->source_pc, source_count,
    source_words, source_exit);
  target_exits = live_canary_block_exits(link->target_pc, target_count,
    target_words, target_exit);
  if (!source_exits || !target_exits) return -1;
  for (unsigned i = 0; i < source_exits; i++) {
    reaches_target |= source_exit[i] == link->target_pc;
    if (source_exit[i] == link->source_pc) return -1;
  }
  if (!reaches_target) return -1;

  /* Every linked dispatcher checks the event deadline before following its
   * edge. Reaching this link's target is therefore a valid early return after
   * the source block, even though it is not a terminal control-flow exit. */
  event_pc[0] = link->target_pc;
  event_words[0] = event_blocks[0] = 0;

  for (unsigned i = 0; i < LIVE_CANARY_MAX_LINKS; i++)
    if (live_canary.links[i].source_pc == link->target_pc) {
      if (live_canary.links[i].source_code !=
          (unsigned char *)live_canary.arena + target->code_offset ||
          live_canary_general_build_paths(i, visiting, depth + 1) != 0)
        return -1;
      next = &live_canary.links[i];
      break;
    }
  if (!next) for (unsigned i = 0; i < target_exits; i++)
    if (target_exit[i] == link->target_pc) return -1;

  for (unsigned i = 0; i < source_exits; i++)
    if (source_exit[i] != link->target_pc) {
      if (count == ARRAY_SIZE(exits)) return -1;
      exits[count] = source_exit[i];
      words[count] = blocks[count] = 0;
      count++;
    }
  if (next) {
    if (count + next->path_count > ARRAY_SIZE(exits)) return -1;
    for (unsigned i = 0; i < next->path_count; i++) {
      exits[count] = next->exit_pc[i];
      words[count] = target_count + next->path_words[i];
      blocks[count] = 1 + next->path_blocks[i];
      count++;
    }
    if (event_count + next->event_count > ARRAY_SIZE(event_pc)) return -1;
    for (unsigned i = 0; i < next->event_count; i++) {
      event_pc[event_count] = next->event_pc[i];
      event_words[event_count] = target_count + next->event_words[i];
      event_blocks[event_count] = 1 + next->event_blocks[i];
      event_count++;
    }
  }
  else {
    if (count + target_exits > ARRAY_SIZE(exits)) return -1;
    for (unsigned i = 0; i < target_exits; i++) {
      exits[count] = target_exit[i];
      words[count] = target_count;
      blocks[count] = 1;
      count++;
    }
  }
  memcpy(link->exit_pc, exits, count * sizeof(exits[0]));
  memcpy(link->path_words, words, count * sizeof(words[0]));
  memcpy(link->path_blocks, blocks, count * sizeof(blocks[0]));
  link->path_count = count;
  memcpy(link->event_pc, event_pc, event_count * sizeof(event_pc[0]));
  memcpy(link->event_words, event_words, event_count * sizeof(event_words[0]));
  memcpy(link->event_blocks, event_blocks,
    event_count * sizeof(event_blocks[0]));
  link->event_count = event_count;
  return 0;
}

static int live_canary_general_refresh_paths(void)
{
  for (unsigned i = 0; i < LIVE_CANARY_MAX_LINKS; i++)
    if (live_canary.links[i].source_pc &&
        live_canary_general_build_paths(i, 0, 0) != 0) return -1;
  return 0;
}

/* Add one observed edge only after both blocks execute successfully alone.
 * One outgoing edge per block, 32 total links, and eight-deep/eight-exit
 * oracles bound the generated graph. Rebuilding every oracle rejects cycles
 * and path explosion before the source dispatcher is patched. */
static int live_canary_general_link(uint32_t source_pc, uint32_t target_pc)
{
  const struct ndrc_rv32_cache_entry *source, *target;
  struct live_canary_link *link = NULL;
  struct live_canary_link *backup = live_canary.link_backup;
  unsigned max_blocks = 0;

  if (!source_pc || !target_pc || source_pc == target_pc) return 0;
  if (!backup) return 0;
  for (unsigned i = 0; i < LIVE_CANARY_MAX_LINKS; i++) {
    const struct live_canary_link *used = &live_canary.links[i];
    if (!used->source_pc) {
      if (!link) link = &live_canary.links[i];
      continue;
    }
    if (used->source_pc == source_pc) return 0;
  }
  if (!link) return 0;
  source = live_canary_cached_pc(source_pc);
  target = live_canary_cached_pc(target_pc);
  if (!source || !target) return 0;
  if (cop0_control_word(intFakeFetch(source_pc + source->guest_bytes - 4)) ||
      cop0_control_word(intFakeFetch(target_pc + target->guest_bytes - 4)))
    return 0;
  /* A terminal native load deliberately returns to the interpreter bridge so
   * its successor can consume the architectural delay.  Never patch that
   * dispatch edge directly until a native pending-entry adapter exists. */
  if (source->guest_bytes >= 4 && delayed_load_word(
      intFakeFetch(source_pc + source->guest_bytes - 4))) return 0;
  memcpy(backup, live_canary.links,
    LIVE_CANARY_MAX_LINKS * sizeof(*backup));
  link->source_pc = source_pc;
  link->target_pc = target_pc;
  link->source_code = (unsigned char *)live_canary.arena + source->code_offset;
  link->dispatcher_offset = source->dispatcher_offset;
  if (live_canary_general_refresh_paths() != 0) {
    memcpy(live_canary.links, backup,
      LIVE_CANARY_MAX_LINKS * sizeof(*backup));
    return 0;
  }
  if (live_canary_relink((unsigned char *)live_canary.arena +
        source->code_offset, source->dispatcher_offset, source_pc,
        (unsigned char *)live_canary.arena + target->code_offset,
        target_pc, 0) != 0) {
    memcpy(live_canary.links, backup,
      LIVE_CANARY_MAX_LINKS * sizeof(*backup));
    return 0;
  }
  for (unsigned i = 0; i < link->path_count; i++)
    if (link->path_blocks[i] > max_blocks) max_blocks = link->path_blocks[i];
  live_canary.links_published++;
  live_canary.general_links_published++;
  if (live_canary.general_links_published <= 16)
    RG_LOGI("new_dynarec RV32 general acyclic link PUBLISHED %u from=%08x to=%08x paths=%u depth=%u",
      live_canary.general_links_published, source_pc, target_pc,
      link->path_count, max_blocks + 1);
  return 1;
}

static int live_canary_link_fresh(unsigned active)
{
  const struct live_canary_link *link = &live_canary.links[active - 1];
  for (unsigned depth = 0; depth < LIVE_CANARY_MAX_PATHS; depth++) {
    const struct ndrc_rv32_cache_entry *target =
      live_canary_cached_pc(link->target_pc);
    void *target_code;
    const struct live_canary_link *next = NULL;
    if (!target) return 0;
    target_code = (unsigned char *)live_canary.arena + target->code_offset;
    for (unsigned i = 0; i < LIVE_CANARY_MAX_LINKS; i++)
      if (live_canary.links[i].source_pc == link->target_pc &&
          live_canary.links[i].source_code == target_code) {
        next = &live_canary.links[i];
        break;
      }
    if (!next) return 1;
    link = next;
  }
  return 0; /* A cycle in link metadata is never safe for this bounded probe. */
}

/* Replace one terminal frontier with both exits of a newly linked target.
 * Retaining all other terminals is important: a conditional predecessor may
 * still leave the chain through its untaken edge. */
static int live_canary_expand_frontier(struct live_canary_link *link,
    uint32_t target_pc, uint32_t exit0, uint32_t exit1,
    unsigned target_words)
{
  uint32_t exits[LIVE_CANARY_MAX_PATHS];
  unsigned paths[LIVE_CANARY_MAX_PATHS], blocks[LIVE_CANARY_MAX_PATHS], count = 0;
  int replaced = 0;
  for (unsigned i = 0; i < link->path_count; i++) {
    if (link->exit_pc[i] == target_pc) {
      if (count + 2 > ARRAY_SIZE(exits)) return -1;
      exits[count] = exit0;
      paths[count] = link->path_words[i] + target_words;
      blocks[count++] = link->path_blocks[i] + 1;
      exits[count] = exit1;
      paths[count] = link->path_words[i] + target_words;
      blocks[count++] = link->path_blocks[i] + 1;
      replaced = 1;
    }
    else {
      if (count == ARRAY_SIZE(exits)) return -1;
      exits[count] = link->exit_pc[i];
      paths[count] = link->path_words[i];
      blocks[count++] = link->path_blocks[i];
    }
  }
  if (!replaced) return 0;
  memcpy(link->exit_pc, exits, count * sizeof(exits[0]));
  memcpy(link->path_words, paths, count * sizeof(paths[0]));
  memcpy(link->path_blocks, blocks, count * sizeof(blocks[0]));
  link->path_count = count;
  return 1;
}

static int live_canary_arm_at(uint32_t pc)
{
  struct compiler_workspace *saved_work = work;
  uint32_t words[LIVE_CANARY_FETCH_WORDS];
  uint32_t hash, code_offset = 0;
  unsigned prefix = 0, size = 0;
  unsigned prefix_bytes = 0, branch_bytes = 0;
  unsigned branch_offset = 0, dispatcher_offset = 0;
  unsigned has_memory = 0, written_regs = 0;
  unsigned has_trapping_alu = 0;
  unsigned final_memory_retained = 0, final_memory_sources = 0;
  unsigned final_branch_mappings = 0;
  unsigned final_cycle_adds_saved = 0, final_ram_fastpaths = 0;
  const struct ndrc_rv32_cache_entry *cached;
  void *code;
  size_t arena_mark;
  int candidate = 0, terminal_branch = 0, terminal_pending = 0, ok = 0;
  int branch_load = 0, terminal_cop0 = 0;
  int prefix_capped = 0, capped_load_split = 0;
  unsigned branch_pending = 0;
  struct regstat prefix_map;
  live_canary.linked_active = 0;
  /* Productive dispatch validates a cache hit directly from its recorded
   * extent. A hit was already fully decoded before insertion, so fetching and
   * decoding the maximum candidate window again is redundant. Source hashing
   * still detects self-modifying code before native execution. */
  if (live_canary.general_active &&
      (cached = ndrc_rv32_cache_lookup_pc(&live_canary.cache, pc)) != NULL &&
      !(cached->guest_bytes & 3)) {
    prefix = cached->guest_bytes / 4;
    if (prefix && prefix <= ARRAY_SIZE(words)) {
      for (unsigned i = 0; i < prefix; i++)
        words[i] = intFakeFetch(pc + i * 4);
      hash = ndrc_rv32_source_hash(words, prefix);
      if (hash == cached->source_hash) {
        live_canary_stamp_entry(pc);
        terminal_branch = live_canary_cached_terminal_branch(words, prefix);
        branch_load = terminal_branch && prefix >= 3 &&
          delayed_load_word(words[prefix - 3]);
        branch_pending = branch_load ? (words[prefix - 3] >> 16) & 31 : 0;
        terminal_pending = !terminal_branch &&
          delayed_load_word(words[prefix - 1]);
        terminal_cop0 = !terminal_branch &&
          cop0_control_word(words[prefix - 1]);
        prefix_capped = !terminal_branch &&
          prefix == LIVE_CANARY_PREFIX_WORDS;
        written_regs = live_canary_written_regs(words,
          terminal_branch ? prefix - 2 : prefix);
        live_canary.general_fast_cache_hits++;
        goto cached_hit;
      }
    }
  }
  /* A stale cache hit left prefix at the old extent. Decode changed source
   * from its first instruction, just as for a previously unseen PC. */
  prefix = 0;
  for (unsigned i = 0; i < ARRAY_SIZE(words); i++)
    words[i] = intFakeFetch(pc + i * 4);
  /* Final handoff phase: a mixed safe prefix ending at a branch. A terminal
   * load is admitted with that branch and resolved by its adapter; standalone
   * terminal loads retain the interpreter handoff below. */
  while (prefix < LIVE_CANARY_PREFIX_WORDS &&
         live_canary_prefix_op(words[prefix])) {
    /* An immediately preceding load is still pending while this instruction
     * runs. Keep a control-changing COP0 operation in the successor block so
     * its exception helper never has to merge that transient state. */
    if (prefix && cop0_control_word(words[prefix]) &&
        delayed_load_word(words[prefix - 1])) break;
    has_memory |= live_canary_memory(words[prefix]);
    has_trapping_alu |= live_canary_trapping_alu(words[prefix]);
    {
      unsigned op = words[prefix] >> 26;
      if (live_canary_safe_alu(words[prefix]) ||
          live_canary_trapping_alu(words[prefix])) {
        unsigned dst = op ? (words[prefix] >> 16) & 31 :
          (words[prefix] >> 11) & 31;
        if (dst) written_regs |= 1u << dst;
      }
      else if (live_canary_hilo(words[prefix]) &&
               ((words[prefix] & 63) == 0x10 ||
                (words[prefix] & 63) == 0x12)) {
        unsigned dst = (words[prefix] >> 11) & 31;
        if (dst) written_regs |= 1u << dst;
      }
      else if (delayed_load_word(words[prefix])) {
        unsigned dst = (words[prefix] >> 16) & 31;
        if (dst) written_regs |= 1u << dst;
      }
    }
    prefix++;
    if (cop0_control_word(words[prefix - 1])) break;
  }
  if (prefix + 1 < ARRAY_SIZE(words)) {
    unsigned branch_op = words[prefix] >> 26;
    int branch_supported;
    int delay_supported;
    int terminal_load = prefix && delayed_load_word(words[prefix - 1]);
    /* Keep the initial live selection memory-backed so the established A/B/C
     * path remains deterministic. Once that three-block chain is published,
     * admit safe ALU-only successors as well; these use the same compiler and
     * branch adapter but previously could only be observed, not executed. */
    branch_supported = (branch_op == 1 || (branch_op >= 2 && branch_op <= 7) ||
      (!branch_op && ((words[prefix] & 63) == 8 ||
                      (words[prefix] & 63) == 9)));
    delay_supported = live_canary_safe_alu(words[prefix + 1]) ||
      live_canary_store(words[prefix + 1]);
    terminal_branch = (has_memory || live_canary.links_published >= 2 ||
      live_canary.general_active) &&
      (prefix || branch_op == 2 || branch_op == 3 ||
       live_canary.general_active) && branch_supported &&
       delay_supported;
    branch_load = terminal_branch && terminal_load;
    branch_pending = branch_load ? (words[prefix - 1] >> 16) & 31 : 0;
    candidate = terminal_branch;
    /* The broad phase may stop before an unsupported opcode and return its
     * exact PC to the interpreter bridge. Terminal loads are now retained:
     * compile() exports normalized interpreter delay state, and this block's
     * dispatcher is kept unlinked so execI resolves the following operation. */
    if (!candidate && live_canary.general_active && prefix) {
      candidate = 1;
      live_canary.general_straight_admissions++;
    }
    if (live_canary.general_active) {
      if (!candidate && terminal_load)
        live_canary.general_reject_terminal_load++;
      else if (!candidate && !branch_supported)
        live_canary.general_reject_opcode++;
      else if (!candidate && !delay_supported)
        live_canary.general_reject_delay++;
      else if (terminal_branch) {
        if (!prefix && branch_op != 2 && branch_op != 3)
          live_canary.general_empty_branch_admissions++;
        if (live_canary_store(words[prefix + 1]))
          live_canary.general_store_delay_admissions++;
      }
    }
  }
  if (!candidate) return 0;
  if (live_canary.general_active && has_trapping_alu)
    live_canary.general_trapping_alu_admissions++;
  if (terminal_branch)
    prefix += 2;
  else if (prefix == LIVE_CANARY_PREFIX_WORDS) {
    unsigned safe_prefix;
    prefix_capped = 1;
    safe_prefix = safe_capped_prefix(words, prefix, 1);
    capped_load_split = safe_prefix != prefix;
    prefix = safe_prefix;
    if (!prefix) return 0;
  }
  terminal_pending = !terminal_branch && prefix &&
    delayed_load_word(words[prefix - 1]);
  terminal_cop0 = !terminal_branch && prefix &&
    cop0_control_word(words[prefix - 1]);
  hash = ndrc_rv32_source_hash(words, prefix);
  cached = ndrc_rv32_cache_lookup(&live_canary.cache, pc, hash);
  if (cached && cached->guest_bytes == prefix * 4) {
    live_canary_stamp_entry(pc);
    goto cached_hit;
  }
  goto cache_miss;

cached_hit:
  {
    const struct ndrc_rv32_cache_entry *target = NULL;
    live_canary.code = (unsigned char *)live_canary.arena + cached->code_offset;
    for (unsigned i = 0; i < LIVE_CANARY_MAX_LINKS; i++)
      if (live_canary.links[i].source_pc == pc &&
          live_canary.links[i].source_code == live_canary.code) {
        live_canary.linked_active = i + 1;
        break;
      }
    if (live_canary.linked_active &&
        !live_canary_link_fresh(live_canary.linked_active)) {
      live_canary.stale_link_skips++;
      if (live_canary.stale_link_skips <= 8 ||
          !(live_canary.stale_link_skips &
            (live_canary.stale_link_skips - 1u)))
        RG_LOGW("new_dynarec RV32 stale links DETACHED count=%u pc=%08x chain=%u",
          live_canary.stale_link_skips, pc, live_canary.linked_active);
      if (live_canary_drop_links() != 0) {
        __atomic_store_n(&live_canary.state, 3, __ATOMIC_RELEASE);
        RG_LOGE("new_dynarec RV32 stale link detach FAILED; Lightrec retained");
        return 0;
      }
    }
    live_canary.pc = pc;
    live_canary.word_count = prefix;
    live_canary.code_bytes = cached->code_bytes;
    live_canary.has_terminal_branch = terminal_branch;
    live_canary.has_terminal_load = terminal_pending;
    live_canary.has_terminal_cop0 = terminal_cop0;
    live_canary.indirect_reg = terminal_branch &&
      (words[prefix - 2] >> 26) == 0 ?
      (words[prefix - 2] >> 21) & 31 : 32;
    live_canary.indirect_dynamic = live_canary.indirect_reg < 32 &&
      (written_regs & (1u << live_canary.indirect_reg));
    if (terminal_branch)
      live_canary_static_exits(pc, prefix, words[prefix - 2],
        live_canary.exit_pc);
    else
      live_canary.exit_pc[0] = live_canary.exit_pc[1] = pc + prefix * 4;
    memcpy(live_canary.words, words, prefix * sizeof(words[0]));
    live_canary.hits++;
    if (!live_canary.edges[0].ready && live_canary.edges[0].source_pc == pc)
      target = live_canary_cached_pc(live_canary.edges[0].target_pc);
    if (target) {
      uint32_t target_words[LIVE_CANARY_FETCH_WORDS];
      unsigned target_count = target->guest_bytes / 4;
      uint32_t target_branch;
      for (unsigned i = 0; i < target_count; i++)
        target_words[i] = intFakeFetch(live_canary.edges[0].target_pc + i * 4);
      target_branch = target_words[target_count - 2];
      /* Keep the first live link's final-PC oracle static. Indirect targets are
       * already executed independently but need a richer linked-state oracle. */
      if (((target_branch >> 26) == 1 ||
           ((target_branch >> 26) >= 4 && (target_branch >> 26) <= 7)) &&
          live_canary_relink(live_canary.code, cached->dispatcher_offset, pc,
            (unsigned char *)live_canary.arena + target->code_offset,
            live_canary.edges[0].target_pc, 0) == 0) {
        uint32_t branch_pc_ = live_canary.edges[0].target_pc + (target_count - 2) * 4;
        struct live_canary_link *link = &live_canary.links[0];
        live_canary.edges[0].ready = live_canary.linked_active = 1;
        live_canary.links_published++;
        link->source_pc = pc;
        link->target_pc = live_canary.edges[0].target_pc;
        link->source_code = live_canary.code;
        link->dispatcher_offset = cached->dispatcher_offset;
        link->path_count = 2;
        link->path_words[0] = target_count;
        link->path_words[1] = target_count;
        link->path_blocks[0] = link->path_blocks[1] = 1;
        link->exit_pc[0] = live_canary.edges[0].target_pc + target_count * 4;
        link->exit_pc[1] = branch_pc_ + 4 +
          ((int32_t)(int16_t)target_branch << 2);
        RG_LOGI("new_dynarec RV32 live cross-block link PUBLISHED from=%08x to=%08x source=%uB target=%uB",
          pc, live_canary.edges[0].target_pc, cached->code_bytes, target->code_bytes);
      }
    }
    __atomic_store_n(&live_canary.state, 1, __ATOMIC_RELEASE);
    if (live_canary.general_active &&
        live_canary.general_blocks < LIVE_CANARY_GENERAL_DETAIL_BLOCKS)
      RG_LOGI("new_dynarec RV32 general cache HIT pc=%08x ops=%u code=%uB",
        pc, prefix, live_canary.code_bytes);
    else if (!live_canary.general_active)
      RG_LOGI("new_dynarec RV32 live mixed-branch cache HIT %u/%u pc=%08x ops=%u code=%uB",
        live_canary.executions + 1, LIVE_CANARY_MAX_EXECUTIONS, pc, prefix,
        live_canary.code_bytes);
    return 1;
  }
cache_miss:
  arena_mark = live_canary.cache.arena_used;
  code = ndrc_rv32_cache_reserve(&live_canary.cache,
    LIVE_CANARY_BLOCK_RESERVE, 64, &code_offset);
  if (!code && live_canary.general_active) {
    size_t exhausted_bytes = live_canary.cache.arena_used;
    unsigned exhausted_entries = live_canary.cache.used_entries;
    ndrc_rv32_cache_reset(&live_canary.cache);
    memset(live_canary.dispatch_table, 0,
      LIVE_CANARY_DISPATCH_ENTRIES * sizeof(*live_canary.dispatch_table));
    memset(live_canary_compiled_page, 0,
      sizeof(live_canary_compiled_page));
    memset(live_canary.links, 0,
      LIVE_CANARY_MAX_LINKS * sizeof(*live_canary.links));
    memset(live_canary.edges, 0, sizeof(live_canary.edges));
    live_canary.links_published = 0;
    live_canary.linked_active = 0;
    live_canary.cache_resets++;
    arena_mark = live_canary.cache.arena_used;
    code = ndrc_rv32_cache_reserve(&live_canary.cache,
      LIVE_CANARY_BLOCK_RESERVE, 64, &code_offset);
    if (live_canary.cache_resets <= 8)
      RG_LOGW("new_dynarec RV32 general cache generation RESET %u: arena=%u/%uKB entries=%u/%u",
        live_canary.cache_resets, (unsigned)(exhausted_bytes / 1024u),
        (unsigned)(live_canary.cache.arena_size / 1024u),
        exhausted_entries, live_canary.cache.entry_count);
  }
  if (!code) {
    __atomic_store_n(&live_canary.state, 3, __ATOMIC_RELEASE);
    RG_LOGE("new_dynarec RV32 live memory cache exhausted after %u misses; Lightrec retained",
      live_canary.misses);
    return -1;
  }
  work = live_canary.workspace;
  if (candidate) {
    /* First compile into the old maximum-size layout to measure this block.
     * Only cache misses pay for this pass. The final pass below places the
     * prefix, branch adapter and patchable dispatcher next to one another. */
    memset(code, 0, LIVE_CANARY_BLOCK_RESERVE);
    memset(work, 0, sizeof(*work));
    memory_mode = 1; full_address_mode = 1; linear_pc = pc;
    live_hardware_memory = live_canary.general_active;
    direct_ram_mode = live_canary.general_active;
    runtime_exit_mode = 1;
    test_dispatcher = (unsigned char *)code +
      (terminal_branch ? LIVE_CANARY_MEASURE_BRANCH_OFFSET :
       LIVE_CANARY_MEASURE_DISPATCH_OFFSET);
    allow_pending_exit = terminal_pending || branch_load;
    if (terminal_branch && prefix == 2) {
      reset_register_map(&prefix_map);
      ok = compile_empty_prefix(code, LIVE_CANARY_MEASURE_BRANCH_OFFSET) == 0;
    }
    else
      ok = compile_mapped(code,
        terminal_branch ? LIVE_CANARY_MEASURE_BRANCH_OFFSET :
        LIVE_CANARY_MEASURE_DISPATCH_OFFSET,
        words, terminal_branch ? prefix - 2 : prefix, NULL,
        terminal_branch ? &prefix_map : NULL, terminal_branch) == 0;
    ok = ok && rv32_output.error == RV_OK;
    allow_pending_exit = 0;
    prefix_bytes = (unsigned)rv32_output.count * 4;
    test_dispatcher = (unsigned char *)code +
      LIVE_CANARY_MEASURE_DISPATCH_OFFSET;
    if (ok && terminal_branch) {
      allow_pending_exit = 1;
      ok = compile_branch_mapped((unsigned char *)code +
        LIVE_CANARY_MEASURE_BRANCH_OFFSET,
        LIVE_CANARY_MEASURE_DISPATCH_OFFSET -
        LIVE_CANARY_MEASURE_BRANCH_OFFSET,
        pc + (prefix - 2) * 4, words[prefix - 2], words[prefix - 1],
        branch_pending, &prefix_map) == 0 &&
        rv32_output.error == RV_OK;
      branch_bytes = 256 + (unsigned)rv32_output.count * 4;
      allow_pending_exit = 0;
    }
    if (ok) {
      /* One extra 64-byte line absorbs any absolute-address materialization
       * difference between measurement and the final executable address. */
      unsigned prefix_region = (prefix_bytes + 127u) & ~63u;
      branch_offset = terminal_branch ? prefix_region : 0;
      if (terminal_branch) {
        unsigned branch_region = (branch_bytes + 127u) & ~63u;
        if (branch_region < 1024) branch_region = 1024;
        dispatcher_offset = branch_offset + branch_region;
      }
      else {
        dispatcher_offset = prefix_region;
      }
      size = dispatcher_offset + 256;
      ok = dispatcher_offset && size <= LIVE_CANARY_BLOCK_RESERVE;
    }
    if (ok) {
      /* Recompile with the compact final addresses embedded in tail jumps. */
      memset(code, 0, LIVE_CANARY_BLOCK_RESERVE);
      memset(work, 0, sizeof(*work));
      test_dispatcher = (unsigned char *)code +
        (terminal_branch ? branch_offset : dispatcher_offset);
      allow_pending_exit = terminal_pending || branch_load;
      if (terminal_branch && prefix == 2) {
        reset_register_map(&prefix_map);
        ok = compile_empty_prefix(code, branch_offset) == 0;
      }
      else
        ok = compile_mapped(code,
          terminal_branch ? branch_offset : dispatcher_offset,
          words, terminal_branch ? prefix - 2 : prefix, NULL,
          terminal_branch ? &prefix_map : NULL, terminal_branch) == 0;
      ok = ok && rv32_output.error == RV_OK;
      allow_pending_exit = 0;
      final_memory_retained = memory_preserved_mappings;
      final_memory_sources = memory_mapped_sources;
      final_branch_mappings = branch_preserved_mappings;
      final_cycle_adds_saved = cycle_adds_saved;
      final_ram_fastpaths = direct_ram_fastpaths;
      test_dispatcher = (unsigned char *)code + dispatcher_offset;
      if (ok && terminal_branch) {
        allow_pending_exit = 1;
        ok = compile_branch_mapped((unsigned char *)code + branch_offset,
          dispatcher_offset - branch_offset,
          pc + (prefix - 2) * 4, words[prefix - 2], words[prefix - 1],
          branch_pending, &prefix_map) == 0 &&
          rv32_output.error == RV_OK;
        final_cycle_adds_saved += cycle_adds_saved;
        final_ram_fastpaths += direct_ram_fastpaths;
        allow_pending_exit = 0;
      }
    }
    if (ok) {
      ok = live_canary_relink(code, dispatcher_offset, pc, NULL, 0,
        live_canary.general_active && !terminal_pending && !terminal_cop0) == 0 &&
        ndrc_rv32_p4_publish(code, size) == 0 &&
        ndrc_rv32_cache_insert(&live_canary.cache, pc, prefix * 4,
          code_offset, size, dispatcher_offset, hash) == 0;
      if (ok) {
        live_canary_stamp_entry(pc);
        live_canary.cache.arena_used = code_offset + size;
        live_canary.general_compiled_memory_retained += final_memory_retained;
        live_canary.general_compiled_memory_sources += final_memory_sources;
        live_canary.general_compiled_branch_mappings +=
          final_branch_mappings;
        live_canary.general_compiled_cycle_adds_saved +=
          final_cycle_adds_saved;
        live_canary.general_compiled_ram_fastpaths += final_ram_fastpaths;
        if (terminal_pending) live_canary.general_terminal_load_blocks++;
        if (branch_load) live_canary.general_load_branch_blocks++;
        if (prefix_capped) live_canary.general_capped_blocks++;
        if (prefix_capped && terminal_pending)
          live_canary.general_capped_load_blocks++;
        if (capped_load_split)
          live_canary.general_capped_load_splits++;
      }
    }
  }
  live_canary.misses++;
  live_canary.code = code;
  test_dispatcher = NULL; allow_pending_exit = 0; runtime_exit_mode = 0;
  memory_mode = 0; full_address_mode = 0; live_hardware_memory = 0;
  direct_ram_mode = 0;
  linear_pc = 0;
  work = saved_work;
  if (!ok) {
    live_canary.cache.arena_used = arena_mark;
    if (live_canary.general_active) {
      live_canary.general_compile_fallbacks++;
      __atomic_store_n(&live_canary.state, 4, __ATOMIC_RELEASE);
      if (live_canary.general_compile_fallbacks <= 16 ||
          !(live_canary.general_compile_fallbacks &
            (live_canary.general_compile_fallbacks - 1u)))
        RG_LOGW("new_dynarec RV32 general compile FALLBACK count=%u pc=%08x ops=%u emitted=%uB error=%u; Lightrec retained",
          live_canary.general_compile_fallbacks, pc, prefix,
          (unsigned)rv32_output.count * 4, (unsigned)rv32_output.error);
      return 0;
    }
    __atomic_store_n(&live_canary.state, 3, __ATOMIC_RELEASE);
    RG_LOGE("new_dynarec RV32 live boundary compile/publish FAILED pc=%08x words=%08x,%08x,%08x; Lightrec retained",
      pc, words[0], words[1], words[2]);
    return -1;
  }
  live_canary.pc = pc;
  live_canary.word_count = prefix;
  live_canary.code_bytes = size;
  live_canary.has_terminal_branch = terminal_branch;
  live_canary.has_terminal_load = terminal_pending;
  live_canary.indirect_reg = terminal_branch &&
    (words[prefix - 2] >> 26) == 0 ?
    (words[prefix - 2] >> 21) & 31 : 32;
  live_canary.indirect_dynamic = live_canary.indirect_reg < 32 &&
    (written_regs & (1u << live_canary.indirect_reg));
  if (terminal_branch)
    live_canary_static_exits(pc, prefix, words[prefix - 2],
      live_canary.exit_pc);
  else
    live_canary.exit_pc[0] = live_canary.exit_pc[1] = pc + prefix * 4;
  memcpy(live_canary.words, words, live_canary.word_count * sizeof(words[0]));
  __atomic_store_n(&live_canary.state, 1, __ATOMIC_RELEASE);
  if (live_canary.general_active &&
      live_canary.general_blocks < LIVE_CANARY_GENERAL_DETAIL_BLOCKS)
    RG_LOGI("new_dynarec RV32 general cache MISS compiled pc=%08x ops=%u code=%uB",
      pc, live_canary.word_count, live_canary.code_bytes);
  else if (!live_canary.general_active)
    RG_LOGI("new_dynarec RV32 live mixed-branch canary ARMED %u/%u at boundary pc=%08x ops=%u code=%uB",
      live_canary.executions + 1, LIVE_CANARY_MAX_EXECUTIONS, pc,
      live_canary.word_count, live_canary.code_bytes);
  if (!live_canary.general_active ||
      live_canary.general_blocks < LIVE_CANARY_GENERAL_DETAIL_BLOCKS) {
    RG_LOGI("new_dynarec RV32 live mixed-branch words: %08x %08x %08x %08x",
      words[0], words[1], words[2], words[3]);
    if (terminal_branch)
      RG_LOGI("new_dynarec RV32 live mixed-branch words: %08x %08x %08x %08x prefix=%u branch=%08x delay=%08x",
        words[4], words[5], words[6], words[7], prefix - 2,
        words[prefix - 2], words[prefix - 1]);
    else
      RG_LOGI("new_dynarec RV32 live straight-prefix words: %08x %08x %08x %08x ops=%u exit=%08x",
        words[4], words[5], words[6], words[7], prefix, pc + prefix * 4);
  }
  return 1;
}

int ndrc_rv32_live_canary_try(void *opaque)
{
  psxRegisters *regs_ = opaque;
  uint32_t current[LIVE_CANARY_FETCH_WORDS];
  uint32_t status, old_pc, old_cycle, cycle_delta;
  uint32_t expected_exit[LIVE_CANARY_MAX_PATHS];
  uint32_t branch_word, expected_ra = 0;
  int armed;
  int valid, pc_valid, link_valid = 1, load_exit_valid = 1;
  int runtime_fallback;
  int linked_event_exit = 0;
  unsigned expected_count = 2;
  unsigned event_count = 0;
  unsigned matched_cycles = 0;
  unsigned *path_words = NULL;
  unsigned *path_blocks = NULL;
  uint32_t *event_pc = NULL;
  unsigned *event_words = NULL;
  unsigned *event_blocks = NULL;
  unsigned expected = 1;
  unsigned dynamic_blocks_before = 0, dynamic_words_before = 0;
  unsigned dynamic_blocks = 0, dynamic_words = 0;
  unsigned inline_blocks = 0;
  unsigned state = __atomic_load_n(&live_canary.state, __ATOMIC_ACQUIRE);
  if (!regs_) return 0;
  /* execI may enter an HLE softcall which recursively invokes the configured
   * CPU's ExecuteBlock. Let that nested Lightrec call run normally instead of
   * recursively entering this diagnostic dispatcher. */
  if (live_canary.general_interpreter_active) return 0;
  if (state == 5) {
    unsigned ran = 0, dispatched = 0, advanced = 0, native_chain = 0;
    unsigned compile_fallbacks = live_canary.general_compile_fallbacks;
    unsigned runtime_fallbacks = live_canary.general_runtime_fallbacks;
    /* ExecuteBlock callers may keep polling while the frontend deliberately
     * holds stop (notably across BIOS/HLE boundaries). That is a suspension,
     * not a general-dispatch attempt: consuming the finite call budget here
     * made 65,536 empty callbacks terminate an otherwise healthy diagnostic. */
    if (regs_->stop) {
      live_canary.general_bridge_pending = 0;
      live_canary.general_stop_deferrals++;
      return 0;
    }
    /* native_batch_advanced() is queried after this pass. Only let the CPU
     * outer loop return directly to this dispatcher when canonical state
     * actually advanced. */
    live_canary.general_bridge_pending = 0;
    live_canary.general_calls++;
    live_canary.general_active = 1;
    if ((int32_t)(regs_->next_interupt - regs_->cycle) < 18)
      live_canary.general_event_deferrals++;
    while (dispatched < LIVE_CANARY_GENERAL_CALL_BLOCKS &&
        live_canary.general_blocks < LIVE_CANARY_GENERAL_MAX_BLOCKS &&
        live_canary.general_attempts < LIVE_CANARY_GENERAL_MAX_ATTEMPTS &&
        !regs_->stop &&
        (int32_t)(regs_->next_interupt - regs_->cycle) >= 18) {
      uint32_t before_pc = regs_->pc, before_cycle = regs_->cycle;
      unsigned had_load_delay = regs_->dloadReg[0] || regs_->dloadReg[1];
      unsigned linked_blocks;
      int native_result;
      dispatched++;
      live_canary.general_attempts++;
      if (live_canary_hle_call(regs_, intFakeFetch(before_pc))) {
        advanced = 1;
        __atomic_store_n(&live_canary.state, 5, __ATOMIC_RELEASE);
        continue;
      }
      __atomic_store_n(&live_canary.state, 4, __ATOMIC_RELEASE);
      native_result = ndrc_rv32_live_canary_try(regs_);
      if (!native_result ||
          __atomic_load_n(&live_canary.state, __ATOMIC_ACQUIRE) == 3 ||
          (regs_->pc == before_pc && regs_->cycle == before_cycle)) {
        unsigned fatal =
          __atomic_load_n(&live_canary.state, __ATOMIC_ACQUIRE) == 3;
        if (!fatal &&
            live_canary.general_compile_fallbacks == compile_fallbacks &&
            live_canary.general_runtime_fallbacks == runtime_fallbacks) {
          if (had_load_delay) live_canary.general_load_deferrals++;
          else live_canary.general_unsupported_fallbacks++;
        }
        if (fatal || (regs_->pc == before_pc && regs_->cycle == before_cycle &&
                      native_result))
          break;

        /* Use the interpreter entry intended for dynarec fallback, then offer
         * its exact resulting state back to the native compiler. execI may
         * consume extra instructions to resolve a load delay or branch. */
        before_pc = regs_->pc;
        before_cycle = regs_->cycle;
        uint32_t fallback_word = intFakeFetch(before_pc);
        unsigned fallback_op = fallback_word >> 26;
        live_canary.general_fallback_opcode[fallback_op]++;
        if (!fallback_op)
          live_canary.general_fallback_special[fallback_word & 63]++;
        else if (fallback_op == 0x10)
          live_canary.general_fallback_cop0[(fallback_word >> 21) & 31]++;
        else if (fallback_op == 0x12)
          live_canary.general_fallback_cop2[fallback_word & 63]++;
        uint32_t fallback_address =
          regs_->GPR.r[(fallback_word >> 21) & 31] + (int16_t)fallback_word;
        live_canary.general_interpreter_active = 1;
        execI(regs_);
        live_canary.general_interpreter_active = 0;
        if (fallback_op == 0x28)
          live_canary_invalidate_write(fallback_address, 1);
        else if (fallback_op == 0x29)
          live_canary_invalidate_write(fallback_address, 2);
        else if (fallback_op == 0x2a || fallback_op == 0x2b ||
                 fallback_op == 0x2e || fallback_op == 0x3a)
          live_canary_invalidate_write(fallback_address & ~3u, 4);
        if (regs_->pc == before_pc && regs_->cycle == before_cycle) {
          __atomic_store_n(&live_canary.state, 3, __ATOMIC_RELEASE);
          RG_LOGE("new_dynarec RV32 interpreter fallback made no progress pc=%08x cycle=%u",
            before_pc, before_cycle);
          break;
        }
        live_canary.general_interpreter_fallbacks++;
        live_canary.general_interpreter_cycles += regs_->cycle - before_cycle;
        compile_fallbacks = live_canary.general_compile_fallbacks;
        runtime_fallbacks = live_canary.general_runtime_fallbacks;
        advanced = 1;
        __atomic_store_n(&live_canary.state, 5, __ATOMIC_RELEASE);
        continue;
      }
      linked_blocks = live_canary.last_linked_blocks;
      live_canary.general_linked_blocks += linked_blocks;
      live_canary.general_native_cycles +=
        (uint32_t)(regs_->cycle - before_cycle);
      compile_fallbacks = live_canary.general_compile_fallbacks;
      runtime_fallbacks = live_canary.general_runtime_fallbacks;
      live_canary.general_blocks++;
      ran++;
      advanced = 1;
      native_chain += 1 + linked_blocks;
      unsigned progress_bucket = live_canary.general_native_cycles /
        LIVE_CANARY_GENERAL_PROGRESS_CYCLES;
      if (progress_bucket != live_canary.general_progress_bucket) {
        live_canary.general_progress_bucket = progress_bucket;
        RG_LOGI("new_dynarec RV32 continuous progress: entries=%u selections=%u pc=%08x cycle=%u",
          live_canary.general_blocks, live_canary.general_attempts,
          regs_->pc, regs_->cycle);
        RG_LOGI("new_dynarec RV32 continuous totals: native_cycles=%u resets=%u runtime=%u interpreter=%u hle=%u",
          live_canary.general_native_cycles, live_canary.cache_resets,
          live_canary.general_runtime_fallbacks,
          live_canary.general_interpreter_fallbacks,
          live_canary.general_hle_calls);
        RG_LOGI("new_dynarec RV32 continuous timing: event_exits=%u adjusted=%u extra_cycles=%u",
          live_canary.general_linked_event_exits,
          live_canary.general_timing_adjustments,
          live_canary.general_timing_cycles);
        RG_LOGI("new_dynarec RV32 continuous cache: fast_hits=%u hits=%u misses=%u stale=%u fallback_writes=%u",
          live_canary.general_fast_cache_hits,
          live_canary.hits - live_canary.general_hit_base,
          live_canary.misses - live_canary.general_miss_base,
          live_canary.stale_link_skips,
          live_canary.general_fallback_store_invalidations);
        RG_LOGI("new_dynarec RV32 continuous chaining: c_blocks=%u inline_blocks=%u lookup_misses=%u linked_blocks=%u native_returns=%u",
          live_canary.general_dynamic_blocks,
          live_canary.general_inline_blocks,
          live_canary.general_dynamic_misses,
          live_canary.general_linked_blocks,
          live_canary.general_native_returns);
        RG_LOGI("new_dynarec RV32 compiled reuse: memory_retained=%u operand_hits=%u branch_maps=%u cycle_adds_saved=%u ram_fastpaths=%u terminal_loads=%u load_branches=%u load_fallbacks=%u",
          live_canary.general_compiled_memory_retained,
          live_canary.general_compiled_memory_sources,
          live_canary.general_compiled_branch_mappings,
          live_canary.general_compiled_cycle_adds_saved,
          live_canary.general_compiled_ram_fastpaths,
          live_canary.general_terminal_load_blocks,
          live_canary.general_load_branch_blocks,
          live_canary.general_load_deferrals);
        RG_LOGI("new_dynarec RV32 compiled prefix limits: capped=%u capped_loads=%u split_loads=%u max_words=%u",
          live_canary.general_capped_blocks,
          live_canary.general_capped_load_blocks,
          live_canary.general_capped_load_splits,
          LIVE_CANARY_PREFIX_WORDS);
        {
          unsigned op[4], special[4], cop0[4], cop2[4];
          live_canary_top4(live_canary.general_fallback_opcode, op);
          live_canary_top4(live_canary.general_fallback_special, special);
          live_canary_top4(live_canary.general_fallback_cop0, cop0);
          live_canary_top4(live_canary.general_fallback_cop2, cop2);
          RG_LOGI("new_dynarec RV32 fallback opcode top: %02x=%u %02x=%u %02x=%u %02x=%u",
            op[0], live_canary.general_fallback_opcode[op[0]],
            op[1], live_canary.general_fallback_opcode[op[1]],
            op[2], live_canary.general_fallback_opcode[op[2]],
            op[3], live_canary.general_fallback_opcode[op[3]]);
          RG_LOGI("new_dynarec RV32 fallback detail top: special %02x=%u %02x=%u %02x=%u; cop2 %02x=%u %02x=%u %02x=%u",
            special[0], live_canary.general_fallback_special[special[0]],
            special[1], live_canary.general_fallback_special[special[1]],
            special[2], live_canary.general_fallback_special[special[2]],
            cop2[0], live_canary.general_fallback_cop2[cop2[0]],
            cop2[1], live_canary.general_fallback_cop2[cop2[1]],
            cop2[2], live_canary.general_fallback_cop2[cop2[2]]);
          RG_LOGI("new_dynarec RV32 fallback COP0 rs top: %02x=%u %02x=%u %02x=%u %02x=%u",
            cop0[0], live_canary.general_fallback_cop0[cop0[0]],
            cop0[1], live_canary.general_fallback_cop0[cop0[1]],
            cop0[2], live_canary.general_fallback_cop0[cop0[2]],
            cop0[3], live_canary.general_fallback_cop0[cop0[3]]);
        }
        /* Wall time includes preemption and helper work. The scaled lookup
         * share is an estimate, not a measurement of pure instruction cost. */
        unsigned dispatch_cycles = live_canary.profile_lookup_samples ?
          (unsigned)(live_canary.profile_lookup_cycles /
            live_canary.profile_lookup_samples) : 0;
        RG_LOGI("new_dynarec RV32 profile: native_ms=%u dispatch_cycles=%u samples=%u calls=%u",
          (unsigned)(live_canary.profile_native_us / 1000), dispatch_cycles,
          live_canary.profile_lookup_samples,
          live_canary.profile_lookup_calls);
        RG_LOGI("new_dynarec RV32 GTE profile: calls=%u no_flags=%u sampled=%u cycles=%u",
          rv32_gte_profile.calls, rv32_gte_profile.no_flags_calls,
          rv32_gte_profile.sampled_calls,
          (unsigned)(rv32_gte_profile.sampled_cycles * 16u));
        live_canary.profile_native_us = live_canary.profile_lookup_cycles = 0;
        live_canary.profile_lookup_calls = live_canary.profile_lookup_samples = 0;
        rv32_gte_profile.sampled_cycles = 0;
        rv32_gte_profile.calls = rv32_gte_profile.no_flags_calls = 0;
        rv32_gte_profile.sampled_calls = 0;
      }
    }
    if (native_chain > live_canary.general_max_chain)
      live_canary.general_max_chain = native_chain;
    live_canary.general_active = 0;
    if (__atomic_load_n(&live_canary.state, __ATOMIC_ACQUIRE) != 3) {
      if (!ran) live_canary.general_fallbacks++;
      if (live_canary.general_blocks >= LIVE_CANARY_GENERAL_MAX_BLOCKS ||
          live_canary.general_attempts >= LIVE_CANARY_GENERAL_MAX_ATTEMPTS ||
          live_canary.general_calls >= LIVE_CANARY_GENERAL_MAX_ATTEMPTS) {
        __atomic_store_n(&live_canary.state, 3, __ATOMIC_RELEASE);
        /* Keep completion records short.  The ESP32-P4 logging path faulted
         * while formatting the former all-in-one record after a successful
         * 2,048-entry run.  Apart from avoiding that logger boundary, split
         * records make any latent post-run ABI fault much easier to locate. */
        RG_LOGI("new_dynarec RV32 sustained COMPLETE: calls=%u selections=%u entries=%u",
          live_canary.general_calls, live_canary.general_attempts,
          live_canary.general_blocks);
        RG_LOGI("new_dynarec RV32 sustained native: linked=%u cycles=%u max_chain=%u native_returns=%u",
          live_canary.general_linked_blocks,
          live_canary.general_native_cycles,
          live_canary.general_max_chain,
          live_canary.general_native_returns);
        RG_LOGI("new_dynarec RV32 sustained dynamic: c_blocks=%u inline_blocks=%u lookup_misses=%u words=%u",
          live_canary.general_dynamic_blocks,
          live_canary.general_inline_blocks,
          live_canary.general_dynamic_misses,
          live_canary.general_dynamic_words);
        RG_LOGI("new_dynarec RV32 sustained deferrals: no_native=%u unsupported=%u event=%u stop=%u load=%u",
          live_canary.general_fallbacks,
          live_canary.general_unsupported_fallbacks,
          live_canary.general_event_deferrals,
          live_canary.general_stop_deferrals,
          live_canary.general_load_deferrals);
        RG_LOGI("new_dynarec RV32 sustained fallbacks: compile=%u runtime=%u overflow=%u; Lightrec retained",
          live_canary.general_compile_fallbacks,
          live_canary.general_runtime_fallbacks,
          live_canary.general_overflow_fallbacks);
        RG_LOGI("new_dynarec RV32 sustained interpreter: calls=%u cycles=%u",
          live_canary.general_interpreter_fallbacks,
          live_canary.general_interpreter_cycles);
        RG_LOGI("new_dynarec RV32 sustained HLE: calls=%u cycles=%u",
          live_canary.general_hle_calls,
          live_canary.general_hle_cycles);
        RG_LOGI("new_dynarec RV32 sustained timing: event_exits=%u adjusted=%u extra_cycles=%u",
          live_canary.general_linked_event_exits,
          live_canary.general_timing_adjustments,
          live_canary.general_timing_cycles);
        RG_LOGI("new_dynarec RV32 sustained cache: links=%u fast_hits=%u hits=%u misses=%u resets=%u",
          live_canary.general_links_published,
          live_canary.general_fast_cache_hits,
          live_canary.hits - live_canary.general_hit_base,
          live_canary.misses - live_canary.general_miss_base,
          live_canary.cache_resets);
        RG_LOGI("new_dynarec RV32 sustained memory reuse: retained=%u operand_hits=%u ram_fastpaths=%u capped=%u capped_loads=%u split_loads=%u",
          live_canary.general_compiled_memory_retained,
          live_canary.general_compiled_memory_sources,
          live_canary.general_compiled_ram_fastpaths,
          live_canary.general_capped_blocks,
          live_canary.general_capped_load_blocks,
          live_canary.general_capped_load_splits);
        RG_LOGI("new_dynarec RV32 sustained rejects: opcode=%u delay=%u terminal_load=%u duplicate_load=%u",
          live_canary.general_reject_opcode,
          live_canary.general_reject_delay,
          live_canary.general_reject_terminal_load,
          live_canary.general_duplicate_load_rejections);
        RG_LOGI("new_dynarec RV32 sustained admissions: trap_alu=%u duplicate_split=%u straight=%u empty_branch=%u store_delay=%u terminal_load=%u load_branch=%u",
          live_canary.general_trapping_alu_admissions,
          live_canary.general_duplicate_split_admissions,
          live_canary.general_straight_admissions,
          live_canary.general_empty_branch_admissions,
          live_canary.general_store_delay_admissions,
          live_canary.general_terminal_load_blocks,
          live_canary.general_load_branch_blocks);
      }
      else {
        live_canary.general_bridge_pending = advanced && !regs_->stop;
        __atomic_store_n(&live_canary.state, 5, __ATOMIC_RELEASE);
      }
    }
    return advanced != 0;
  }
  if (state != 1 && state != 4) return 0;
  if (regs_->dloadReg[0] || regs_->dloadReg[1] ||
      (int32_t)(regs_->next_interupt - regs_->cycle) < 18) return 0;
  if (state == 4) {
    live_canary.scans++;
    armed = live_canary_arm_at(regs_->pc);
    if (armed <= 0) {
      if (!armed) live_canary.unsupported++;
      if (!armed && !live_canary.general_active &&
          live_canary.scans >= LIVE_CANARY_MAX_SCANS) {
        __atomic_store_n(&live_canary.state, 3, __ATOMIC_RELEASE);
        RG_LOGW("new_dynarec RV32 live canary scan limit: executed=%u unsupported=%u/%u; Lightrec retained",
          live_canary.executions, live_canary.unsupported, live_canary.scans);
      }
      return 0;
    }
  }
  if (regs_->pc != live_canary.pc) {
    return 0;
  }
  /* Productive dispatch validated a cached block's source hash while arming
   * it immediately above; no guest code can run between that check and this
   * entry. The bounded/fixed canary may remain armed across Lightrec callbacks,
   * so retain its independent pre-entry stale-code guard. */
  if (!live_canary.general_active) {
    for (unsigned i = 0; i < live_canary.word_count; i++)
      current[i] = intFakeFetch(live_canary.pc + i * 4);
    if (ndrc_rv32_source_hash(current, live_canary.word_count) !=
        ndrc_rv32_source_hash(live_canary.words, live_canary.word_count)) {
      __atomic_store_n(&live_canary.state, 3, __ATOMIC_RELEASE);
      RG_LOGW("new_dynarec RV32 live canary stale at pc=%08x; Lightrec retained",
        live_canary.pc);
      return 0;
    }
  }
  if (!__atomic_compare_exchange_n(&live_canary.state, &expected, 2, 0,
      __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) return 0;
  old_pc = regs_->pc;
  old_cycle = regs_->cycle;
  branch_word = live_canary.has_terminal_branch ?
    live_canary.words[live_canary.word_count - 2] : 0;
  if (live_canary.has_terminal_branch && !live_canary.linked_active &&
      (branch_word >> 26) == 3)
    expected_ra = old_pc + live_canary.word_count * 4;
  expected_exit[0] = live_canary.exit_pc[0];
  expected_exit[1] = live_canary.exit_pc[1];
  if (live_canary.indirect_reg < 32)
    expected_exit[0] = expected_exit[1] =
      regs_->GPR.r[live_canary.indirect_reg];
  if (live_canary.linked_active) {
    struct live_canary_link *link =
      &live_canary.links[live_canary.linked_active - 1];
    uint32_t *linked_exit = link->exit_pc;
    path_words = link->path_words;
    path_blocks = link->path_blocks;
    expected_count = link->path_count;
    event_pc = link->event_pc;
    event_words = link->event_words;
    event_blocks = link->event_blocks;
    event_count = link->event_count;
    for (unsigned i = 0; i < expected_count; i++)
      expected_exit[i] = linked_exit[i];
  }
  live_canary.last_linked_blocks = 0;
  dynamic_blocks_before = live_canary.general_dynamic_blocks;
  dynamic_words_before = live_canary.general_dynamic_words;
  uint64_t native_started = live_canary.general_active ? rg_system_timer() : 0;
  status = ndrc_rv32_enter(live_canary.code, regs_, &regs_->cycle);
  if (live_canary.general_active)
    live_canary.profile_native_us += rg_system_timer() - native_started;
  dynamic_blocks = live_canary.general_dynamic_blocks - dynamic_blocks_before;
  dynamic_words = live_canary.general_dynamic_words - dynamic_words_before;
  cycle_delta = regs_->cycle - old_cycle;
  /* Status 1 is a prefix ADD/ADDI/SUB overflow. Its stub preserves prior
   * architectural writes and identifies the exact trapping instruction, so
   * Lightrec can deliver the real exception once. Status 2 is delay-slot
   * overflow and remains outside the live selector because replaying only the
   * delay instruction would lose the branch exception context. */
  runtime_fallback = status == 1 || (status >= 3 && status <= 5);
  pc_valid = 0;
  if (runtime_fallback) {
    if (live_canary.general_active || dynamic_blocks || inline_blocks ||
        live_canary.linked_active)
      pc_valid = !(regs_->pc & 3) && cycle_delta != 0;
    else
      pc_valid = regs_->pc >= old_pc &&
        regs_->pc < old_pc + live_canary.word_count * 4 &&
        !((regs_->pc - old_pc) & 3) &&
        cycle_delta == (regs_->pc - old_pc) / 4 + 1;
  }
  else if (live_canary.general_active || dynamic_blocks || inline_blocks) {
    int event_due = (int32_t)(regs_->cycle - regs_->next_interupt) >= 0;
    int terminal_load_exit = live_canary.has_terminal_load &&
      !live_canary.linked_active && !dynamic_blocks && !inline_blocks;
    matched_cycles = live_canary.word_count + dynamic_words;
    if (terminal_load_exit) {
      unsigned dest = (live_canary.words[live_canary.word_count - 1] >> 16) & 31;
      /* The producer's forced return is valid even when its successor already
       * exists in the native cache. Verify the complete bridge ABI here so a
       * malformed pending state cannot be mistaken for a legitimate exit. */
      load_exit_valid = dest ?
        (regs_->dloadSel == 0 && regs_->dloadReg[0] == dest &&
         regs_->dloadReg[1] == 0) :
        (regs_->dloadReg[0] == 0 && regs_->dloadReg[1] == 0);
    }
    pc_valid = !(regs_->pc & 3) && cycle_delta >= matched_cycles &&
      (terminal_load_exit || live_canary.has_terminal_cop0 || event_due ||
       live_canary_cached_pc(regs_->pc) == NULL);
    live_canary.last_linked_blocks = dynamic_blocks + inline_blocks;
    linked_event_exit = event_due;
  }
  else if (live_canary.linked_active) {
    for (unsigned i = 0; i < expected_count; i++) {
      unsigned path_cycles = live_canary.word_count + path_words[i];
      /* Slow psxMem* helpers may advance canonical time for DMA, MDEC or CD
       * hardware. The compiled instruction count is therefore a strict
       * minimum. The finite exit-PC oracle still validates control flow. If
       * paths converge on one PC, use the longest compatible path. */
      if (regs_->pc == expected_exit[i] && cycle_delta >= path_cycles &&
          path_cycles >= matched_cycles) {
        pc_valid = 1;
        matched_cycles = path_cycles;
        live_canary.last_linked_blocks = path_blocks[i];
      }
    }
    /* A linked dispatcher may leave at any intermediate successor when the
     * event deadline becomes due. These frontiers are distinct from the
     * chain's terminal exits and carry their own exact path-prefix minima. */
    if ((int32_t)(regs_->cycle - regs_->next_interupt) >= 0)
      for (unsigned i = 0; i < event_count; i++) {
        unsigned path_cycles = live_canary.word_count + event_words[i];
        if (regs_->pc == event_pc[i] && cycle_delta >= path_cycles &&
            path_cycles >= matched_cycles) {
          pc_valid = 1;
          linked_event_exit = 1;
          matched_cycles = path_cycles;
          live_canary.last_linked_blocks = event_blocks[i];
        }
      }
  }
  else {
    pc_valid = regs_->pc == expected_exit[0] || regs_->pc == expected_exit[1];
    matched_cycles = live_canary.word_count;
  }
  if (expected_ra && !live_canary.general_active &&
      !dynamic_blocks && !inline_blocks)
    link_valid = regs_->GPR.r[31] == expected_ra;
  if (!live_canary.linked_active && live_canary.indirect_dynamic)
    pc_valid = (regs_->pc & 3) == 0;
  valid = (runtime_fallback ? pc_valid :
    (status || (pc_valid && link_valid && (live_canary.general_active ||
     dynamic_blocks || inline_blocks ||
     live_canary.linked_active ||
     (cycle_delta >= live_canary.word_count &&
      (live_canary.general_active ||
       cycle_delta % live_canary.word_count == 0)))))) && load_exit_valid;
  if (!status && valid && live_canary.general_active &&
      cycle_delta > matched_cycles) {
    live_canary.general_timing_adjustments++;
    live_canary.general_timing_cycles += cycle_delta - matched_cycles;
    if (live_canary.general_timing_adjustments <= 8) {
      RG_LOGI("new_dynarec RV32 live timing adjustment ACCEPTED %u pc=%08x->%08x",
        live_canary.general_timing_adjustments, old_pc, regs_->pc);
      RG_LOGI("new_dynarec RV32 timing cycles: minimum=%u actual=%u extra=%u linked=%u",
        matched_cycles, cycle_delta, cycle_delta - matched_cycles,
        live_canary.linked_active);
    }
  }
  if (!status && valid && live_canary.general_active && linked_event_exit)
    live_canary.general_linked_event_exits++;
  if (!status && valid && expected_ra && !live_canary.general_active &&
      !dynamic_blocks && !inline_blocks)
  {
    live_canary.direct_calls++;
    if (!live_canary.general_active ||
        live_canary.general_blocks < LIVE_CANARY_GENERAL_DETAIL_BLOCKS)
      RG_LOGI("new_dynarec RV32 live direct-call link PASSED %u pc=%08x ra=%08x",
        live_canary.direct_calls, old_pc, regs_->GPR.r[31]);
  }
  if (!status && valid) {
    unsigned linked = live_canary.general_active && live_canary.linked_active ?
      live_canary.last_linked_blocks :
      cycle_delta / live_canary.word_count - 1;
    live_canary.chained += linked;
    if (!live_canary.general_active && !live_canary.edges[0].source_pc &&
        regs_->pc != old_pc) {
      live_canary.edges[0].source_pc = old_pc;
      live_canary.edges[0].target_pc = regs_->pc;
      RG_LOGI("new_dynarec RV32 live cross-block edge OBSERVED from=%08x to=%08x",
        old_pc, regs_->pc);
    }
    if (!live_canary.general_active && live_canary.links_published == 2 &&
        live_canary.linked_active == 1 &&
        !live_canary.edges[2].source_pc) {
      live_canary.edges[2].source_pc = live_canary.edges[1].target_pc;
      live_canary.edges[2].target_pc = regs_->pc;
      RG_LOGI("new_dynarec RV32 live cross-block third edge OBSERVED from=%08x to=%08x",
        live_canary.edges[2].source_pc, live_canary.edges[2].target_pc);
    }
    if (linked && (!live_canary.general_active ||
        live_canary.general_blocks < LIVE_CANARY_GENERAL_DETAIL_BLOCKS)) {
      uint32_t successor[LIVE_CANARY_FETCH_WORDS];
      for (unsigned i = 0; i < ARRAY_SIZE(successor); i++)
        successor[i] = intFakeFetch(regs_->pc + i * 4);
      RG_LOGI("new_dynarec RV32 live successor OBSERVED pc=%08x words=%08x %08x %08x %08x %08x %08x",
        regs_->pc, successor[0], successor[1], successor[2], successor[3],
        successor[4], successor[5]);
      RG_LOGI("new_dynarec RV32 live successor words: %08x %08x %08x %08x %08x %08x",
        successor[6], successor[7], successor[8], successor[9],
        successor[10], successor[11]);
      RG_LOGI("new_dynarec RV32 live successor words: %08x %08x %08x %08x %08x %08x",
        successor[12], successor[13], successor[14], successor[15],
        successor[16], successor[17]);
    }
  }
  live_canary.executions++;
  if (live_canary.general_active &&
      live_canary.general_blocks < LIVE_CANARY_GENERAL_DETAIL_BLOCKS)
    RG_LOGI("new_dynarec RV32 general block EXECUTED %u/%u pc=%08x->%08x cycle=%u->%u status=%u code=%uB",
      live_canary.general_blocks + 1, LIVE_CANARY_GENERAL_MAX_BLOCKS,
      old_pc, regs_->pc, old_cycle, regs_->cycle, status,
      live_canary.code_bytes);
  else if (!live_canary.general_active)
    RG_LOGI("new_dynarec RV32 live mixed-branch canary EXECUTED %u/%u pc=%08x->%08x cycle=%u->%u status=%u code=%uB; resuming Lightrec",
      live_canary.executions, LIVE_CANARY_MAX_EXECUTIONS, old_pc, regs_->pc,
      old_cycle, regs_->cycle, status, live_canary.code_bytes);
  if (runtime_fallback) {
    uint32_t attempted_cycle = regs_->cycle;
    if (valid) {
      /* The guard follows the per-instruction cycle increment, but the memory
       * operation itself has not run. Roll back that increment so Lightrec
       * executes the exact instruction once. */
      regs_->cycle--;
      if (live_canary.general_active)
        live_canary.general_runtime_fallbacks++;
      if (live_canary.general_active && status == 1)
        live_canary.general_overflow_fallbacks++;
      __atomic_store_n(&live_canary.state, 4, __ATOMIC_RELEASE);
      if (!live_canary.general_active ||
          live_canary.general_runtime_fallbacks <= 8)
        RG_LOGI("new_dynarec RV32 exact-PC runtime fallback: pc=%08x status=%u native_cycles=%u resumed_cycle=%u; Lightrec retained",
          regs_->pc, status, attempted_cycle - old_cycle - 1, regs_->cycle);
    }
    else {
      __atomic_store_n(&live_canary.state, 3, __ATOMIC_RELEASE);
      RG_LOGE("new_dynarec RV32 runtime fallback accounting FAILED: pc=%08x status=%u cycle_delta=%u base=%08x/%u; Lightrec retained",
        regs_->pc, status, regs_->cycle - old_cycle, old_pc,
        live_canary.word_count);
    }
    return 0;
  }
  if (!status && valid && live_canary.edges[2].source_pc &&
      !live_canary.edges[2].ready && old_pc == live_canary.edges[2].target_pc) {
    const struct ndrc_rv32_cache_entry *source3 =
      live_canary_cached_pc(live_canary.edges[2].source_pc);
    const struct ndrc_rv32_cache_entry *target3 =
      live_canary_cached_pc(live_canary.edges[2].target_pc);
    if (source3 && target3) {
      uint32_t target_words[LIVE_CANARY_FETCH_WORDS];
      uint32_t target_branch, branch_pc_, alternate;
      unsigned target_count = target3->guest_bytes / 4;
      live_canary.edges[2].ready = 1;
      RG_LOGI("new_dynarec RV32 live cross-block third link READY from=%08x to=%08x source=%uB target=%uB (not published)",
        live_canary.edges[2].source_pc, live_canary.edges[2].target_pc,
        source3->code_bytes, target3->code_bytes);
      for (unsigned i = 0; i < target_count; i++)
        target_words[i] = intFakeFetch(live_canary.edges[2].target_pc + i * 4);
      target_branch = target_words[target_count - 2];
      branch_pc_ = live_canary.edges[2].target_pc + (target_count - 2) * 4;
      alternate = live_canary.links[1].exit_pc[0] == live_canary.edges[2].target_pc ?
        live_canary.links[1].exit_pc[1] : live_canary.links[1].exit_pc[0];
      if (((target_branch >> 26) == 1 ||
           ((target_branch >> 26) >= 4 && (target_branch >> 26) <= 7)) &&
          live_canary_relink((unsigned char *)live_canary.arena +
            source3->code_offset, source3->dispatcher_offset,
            live_canary.edges[2].source_pc,
            (unsigned char *)live_canary.arena + target3->code_offset,
            live_canary.edges[2].target_pc, 0) == 0) {
        uint32_t target_exit0 = live_canary.edges[2].target_pc + target_count * 4;
        uint32_t target_exit1 = branch_pc_ + 4 +
          ((int32_t)(int16_t)target_branch << 2);
        struct live_canary_link *link3 = &live_canary.links[2];
        link3->source_pc = live_canary.edges[2].source_pc;
        link3->target_pc = live_canary.edges[2].target_pc;
        link3->source_code = (unsigned char *)live_canary.arena +
          source3->code_offset;
        link3->dispatcher_offset = source3->dispatcher_offset;
        link3->path_count = 3;
        link3->exit_pc[0] = alternate;
        link3->exit_pc[1] = target_exit0;
        link3->exit_pc[2] = target_exit1;
        link3->path_words[0] = 0;
        link3->path_words[1] = target_count;
        link3->path_words[2] = target_count;
        link3->path_blocks[0] = 0;
        link3->path_blocks[1] = link3->path_blocks[2] = 1;
        for (unsigned i = 0; i < 2; i++)
          if (live_canary_expand_frontier(&live_canary.links[i],
              live_canary.edges[2].target_pc, target_exit0, target_exit1,
              target_count) <= 0) {
            RG_LOGE("new_dynarec RV32 live third-link frontier expansion FAILED chain=%u", i + 1);
            valid = 0;
          }
        live_canary.links_published++;
        RG_LOGI("new_dynarec RV32 live cross-block third link PUBLISHED from=%08x to=%08x local=%08x@%u,%08x@%u,%08x@%u root_paths=%u",
          live_canary.edges[2].source_pc, live_canary.edges[2].target_pc, alternate,
          0, target_exit0, target_count, target_exit1, target_count,
          live_canary.links[0].path_count);
      }
      else {
        RG_LOGE("new_dynarec RV32 live cross-block third link publish FAILED; Lightrec retained");
      }
    }
  }
  if (!valid) {
    RG_LOGE("new_dynarec RV32 live mixed-branch accounting FAILED: actual pc=%08x cycle_delta=%u base=%u linked=%u paths=%u",
      regs_->pc, cycle_delta, live_canary.word_count,
      live_canary.linked_active, expected_count);
    RG_LOGE("new_dynarec RV32 accounting oracle: expected pc=%08x/%08x/%08x dynamic=%u link_ok=%u ra=%08x/%08x",
      expected_exit[0], expected_exit[1],
      expected_count > 2 ? expected_exit[2] : 0,
      live_canary.indirect_dynamic, link_valid, regs_->GPR.r[31], expected_ra);
    RG_LOGE("new_dynarec RV32 accounting delay: terminal=%u word=%08x sel=%u reg=%u/%u val=%08x/%08x valid=%d",
      live_canary.has_terminal_load,
      live_canary.words[live_canary.word_count - 1], regs_->dloadSel,
      regs_->dloadReg[0], regs_->dloadReg[1], regs_->dloadVal[0],
      regs_->dloadVal[1], load_exit_valid);
    if (live_canary.linked_active)
      RG_LOGE("new_dynarec RV32 accounting minima: cycles=%u/%u/%u/%u",
        live_canary.word_count + path_words[0],
        expected_count > 1 ? live_canary.word_count + path_words[1] : 0,
        expected_count > 2 ? live_canary.word_count + path_words[2] : 0,
        expected_count > 3 ? live_canary.word_count + path_words[3] : 0);
    if (event_count)
      RG_LOGE("new_dynarec RV32 event oracle: pc=%08x/%08x/%08x due=%u count=%u",
        event_pc[0], event_count > 1 ? event_pc[1] : 0,
        event_count > 2 ? event_pc[2] : 0,
        (int32_t)(regs_->cycle - regs_->next_interupt) >= 0, event_count);
  }
  if (!status && valid &&
      (live_canary.general_active ||
       (live_canary.executions < LIVE_CANARY_MAX_EXECUTIONS &&
        live_canary.scans < LIVE_CANARY_MAX_SCANS))) {
    __atomic_store_n(&live_canary.state, 4, __ATOMIC_RELEASE);
    if (live_canary.linked_active && !live_canary.discovery_continuations) {
      const struct ndrc_rv32_cache_entry *source2, *target2;
      live_canary.discovery_continuations++;
      live_canary.edges[1].source_pc = live_canary.edges[0].target_pc;
      live_canary.edges[1].target_pc = regs_->pc;
      RG_LOGI("new_dynarec RV32 live cross-block successor DISCOVERY at pc=%08x",
        regs_->pc);
      (void)ndrc_rv32_live_canary_try(regs_);
      source2 = live_canary_cached_pc(live_canary.edges[1].source_pc);
      target2 = live_canary_cached_pc(live_canary.edges[1].target_pc);
      if (source2 && target2) {
        uint32_t target_words[LIVE_CANARY_FETCH_WORDS];
        uint32_t source_words[LIVE_CANARY_FETCH_WORDS];
        unsigned target_count = target2->guest_bytes / 4;
        unsigned source_count = source2->guest_bytes / 4;
        uint32_t target_branch, branch_pc_, source_branch_pc, source_branch;
        uint32_t source_exit0, source_exit1, alternate;
        live_canary.edges[1].ready = 1;
        RG_LOGI("new_dynarec RV32 live cross-block second link READY from=%08x to=%08x source=%uB target=%uB",
          live_canary.edges[1].source_pc, live_canary.edges[1].target_pc,
          source2->code_bytes, target2->code_bytes);
        for (unsigned i = 0; i < target_count; i++)
          target_words[i] = intFakeFetch(live_canary.edges[1].target_pc + i * 4);
        for (unsigned i = 0; i < source_count; i++)
          source_words[i] = intFakeFetch(live_canary.edges[1].source_pc + i * 4);
        target_branch = target_words[target_count - 2];
        branch_pc_ = live_canary.edges[1].target_pc + (target_count - 2) * 4;
        source_branch = source_words[source_count - 2];
        source_branch_pc = live_canary.edges[1].source_pc + (source_count - 2) * 4;
        source_exit0 = live_canary.edges[1].source_pc + source_count * 4;
        source_exit1 = source_branch_pc + 4 +
          ((int32_t)(int16_t)source_branch << 2);
        alternate = source_exit0 == live_canary.edges[1].target_pc ?
          source_exit1 : source_exit0;
        if (((target_branch >> 26) == 1 ||
             ((target_branch >> 26) >= 4 && (target_branch >> 26) <= 7)) &&
            live_canary_relink((unsigned char *)live_canary.arena +
              source2->code_offset, source2->dispatcher_offset,
              live_canary.edges[1].source_pc,
              (unsigned char *)live_canary.arena + target2->code_offset,
              live_canary.edges[1].target_pc, 0) == 0) {
          struct live_canary_link *link2 = &live_canary.links[1];
          struct live_canary_link *link1 = &live_canary.links[0];
          link2->source_pc = live_canary.edges[1].source_pc;
          link2->target_pc = live_canary.edges[1].target_pc;
          link2->source_code = (unsigned char *)live_canary.arena +
            source2->code_offset;
          link2->dispatcher_offset = source2->dispatcher_offset;
          link2->path_count = 3;
          link2->path_words[0] = 0;
          link2->path_words[1] = target_count;
          link2->path_words[2] = target_count;
          link2->path_blocks[0] = 0;
          link2->path_blocks[1] = link2->path_blocks[2] = 1;
          link2->exit_pc[0] = alternate;
          link2->exit_pc[1] = live_canary.edges[1].target_pc + target_count * 4;
          link2->exit_pc[2] = branch_pc_ + 4 +
            ((int32_t)(int16_t)target_branch << 2);
          if (live_canary_expand_frontier(link1,
              live_canary.edges[1].target_pc, link2->exit_pc[1],
              link2->exit_pc[2], target_count) <= 0) {
            RG_LOGE("new_dynarec RV32 live second-link frontier expansion FAILED");
            valid = 0;
          }
          live_canary.links_published++;
          RG_LOGI("new_dynarec RV32 live cross-block second link PUBLISHED from=%08x to=%08x local_paths=%u root_paths=%u",
            live_canary.edges[1].source_pc, live_canary.edges[1].target_pc,
            link2->path_count, link1->path_count);
        }
        else {
          RG_LOGE("new_dynarec RV32 live cross-block second link publish FAILED; Lightrec retained");
        }
      }
    }
    else if (live_canary.linked_active && live_canary.links_published >= 3 &&
        live_canary.direct_discoveries == 0 &&
        (intFakeFetch(regs_->pc) >> 26) >= 2 &&
        (intFakeFetch(regs_->pc) >> 26) <= 3 &&
        live_canary_safe_alu(intFakeFetch(regs_->pc + 4))) {
      uint32_t frontier_pc = regs_->pc;
      RG_LOGI("new_dynarec RV32 live direct-call frontier DISCOVERY at pc=%08x",
        frontier_pc);
      if (ndrc_rv32_live_canary_try(regs_)) {
        live_canary.direct_discoveries++;
        RG_LOGI("new_dynarec RV32 live direct-call frontier EXECUTED pc=%08x",
          frontier_pc);
      }
    }
  }
  else {
    if (!live_canary.general_active && !status && valid &&
        live_canary.executions >= LIVE_CANARY_MAX_EXECUTIONS) {
      /* The fixed canary contains deliberately patched, path-specific links.
       * Start broad discovery in a fresh generation so a later dynamic exit
       * cannot inherit the deterministic Ridge Racer oracle. */
      RG_LOGI("new_dynarec RV32 bounded core loop COMPLETE: executed=%u chained=%u scans=%u unsupported=%u hits=%u misses=%u direct_calls=%u link_ready=%u/%u/%u published=%u stale_skips=%u discovery=%u/%u status=%u; general dispatcher enabled",
        live_canary.executions, live_canary.chained, live_canary.scans,
        live_canary.unsupported, live_canary.hits, live_canary.misses,
        live_canary.direct_calls, live_canary.edges[0].ready,
        live_canary.edges[1].ready, live_canary.edges[2].ready,
        live_canary.links_published, live_canary.stale_link_skips,
        live_canary.discovery_continuations, live_canary.direct_discoveries,
        status);
      ndrc_rv32_cache_reset(&live_canary.cache);
      memset(live_canary.dispatch_table, 0,
        LIVE_CANARY_DISPATCH_ENTRIES * sizeof(*live_canary.dispatch_table));
      memset(live_canary_compiled_page, 0,
        sizeof(live_canary_compiled_page));
      memset(live_canary.links, 0,
        LIVE_CANARY_MAX_LINKS * sizeof(*live_canary.links));
      memset(live_canary.edges, 0, sizeof(live_canary.edges));
      live_canary.links_published = 0;
      live_canary.linked_active = 0;
      live_canary.general_hit_base = live_canary.hits;
      live_canary.general_miss_base = live_canary.misses;
      __atomic_store_n(&live_canary.state, 5, __ATOMIC_RELEASE);
      RG_LOGI("new_dynarec RV32 general dispatcher cache generation isolated and empty");
    }
    else {
      __atomic_store_n(&live_canary.state, 3, __ATOMIC_RELEASE);
      RG_LOGE("new_dynarec RV32 execution phase STOPPED: general=%u valid=%u status=%u executed=%u; Lightrec retained",
        live_canary.general_active, valid, status, live_canary.executions);
    }
  }
  return 1;
}

/* Consume the indication that the finite general gate advanced canonical
 * state. The CPU adapter may return directly to its outer loop and offer the
 * new PC to this dispatcher again. If the gate cannot advance near an event
 * boundary this remains false, allowing normal Lightrec/event processing. */
int ndrc_rv32_live_canary_native_batch_advanced(void)
{
  if (live_canary.general_bridge_pending &&
      __atomic_load_n(&live_canary.state, __ATOMIC_ACQUIRE) == 5) {
    live_canary.general_bridge_pending = 0;
    live_canary.general_native_returns++;
    return 1;
  }
  return 0;
}

void ndrc_rv32_live_canary_shutdown(void)
{
  free(live_canary.allocation);
  free(live_canary.workspace);
  free(live_canary.entries);
  free(live_canary.dispatch_table);
  free(live_canary.links);
  free(live_canary.link_backup);
  memset(&live_canary, 0, sizeof(live_canary));
}
#endif

static int mixed_prefix_info(const uint32_t *words, unsigned count,
    unsigned *base_out, int32_t *anchor_out)
{
  uint32_t written = 0;
  unsigned common_base = 32, memory_count = 0, nonmemory_count = 0;
  int32_t anchor = 0;
  if (count < 2) return 0;
  for (unsigned i = 0; i < count; i++) {
    uint32_t word = words[i];
    unsigned op = word >> 26;
    int is_memory = memory_load_opcode(op) || memory_store_opcode(op);
    if (is_memory) {
      unsigned base = (word >> 21) & 31;
      int32_t imm = (int16_t)word;
      unsigned align = (op == 0x21 || op == 0x25 || op == 0x29) ? 1 :
        (op == 0x23 || op == 0x2b) ? 3 : 0;
      int32_t address;
      if (!base || ((written >> base) & 1u)) return 0;
      if (common_base == 32) { common_base = base; anchor = imm; }
      else if (base != common_base) return 0;
      address = 0x80 + imm - anchor;
      if (address < 0 || address > (int32_t)(255 - align) ||
          ((unsigned)address & align)) return 0;
      memory_count++;
    }
    else nonmemory_count++;
    if (op >= 8 && op <= 15)
      written |= 1u << ((word >> 16) & 31);
    else if (memory_load_opcode(op))
      written |= 1u << ((word >> 16) & 31);
    else if (!op)
      written |= 1u << ((word >> 11) & 31);
  }
  {
    unsigned last_op = words[count - 1] >> 26;
    if (memory_load_opcode(last_op)) return 0;
  }
  if (!memory_count || !nonmemory_count) return 0;
  *base_out = common_base;
  *anchor_out = anchor;
  return 1;
}

/* Observe a bounded sample of real game blocks without publishing or executing
 * its output. This measures the restricted compiler's current useful prefix and
 * first blocker; it cannot alter Lightrec or guest state. */
void ndrc_rv32_shadow_observe(uint32_t pc, const uint32_t *words, unsigned count)
{
  static struct compiler_workspace *shadow_work;
  static void *shadow_allocation;
  static void *shadow_code;
  static size_t shadow_arena_size;
  /* Keys 0..63 are primary opcodes; 64..127 are SPECIAL funct values. */
  static uint32_t blockers[128];
  static unsigned blocks, full, compiled, failed, source_ops, prefix_ops, code_bytes;
  static unsigned pending_boundary, capped, branch_blocks;
  static uint32_t execute_words[TEST_INSNS], execute_pc;
  static unsigned execute_count;
  static uint32_t execute_branch, execute_delay, execute_branch_pc;
  static uint32_t execute_memory[2], execute_memory_pc;
  static uint32_t execute_store[2], execute_store_pc;
  static uint32_t execute_mixed[16], execute_mixed_pc;
  static unsigned execute_mixed_count, execute_mixed_base;
  static int32_t execute_mixed_anchor;
  static uint32_t execute_whole[16], execute_whole_branch, execute_whole_delay;
  static uint32_t execute_whole_pc;
  static unsigned execute_whole_count, execute_whole_base;
  static int32_t execute_whole_anchor;
  unsigned prefix = 0, limit, blocker = 0, blocked = 0, trimmed_load = 0;
  unsigned covered, complete = 0, success = 0;
  if (blocks >= 512 || !words || !count) return;
  if (!shadow_work) {
    shadow_work = rg_alloc(sizeof(*shadow_work), MEM_SLOW | MEM_NOPANIC);
    shadow_arena_size = LIVE_CANARY_ARENA_PREFERRED_SIZE;
    shadow_allocation = rg_alloc(shadow_arena_size + 63,
      MEM_SLOW | MEM_NOPANIC);
    if (!shadow_allocation) {
      shadow_arena_size = LIVE_CANARY_ARENA_FALLBACK_SIZE;
      shadow_allocation = rg_alloc(shadow_arena_size + 63,
        MEM_SLOW | MEM_NOPANIC);
    }
    shadow_code = shadow_allocation ? (void *)(((uintptr_t)shadow_allocation + 63) & ~(uintptr_t)63) : NULL;
    if (!shadow_work || !shadow_code) {
      free(shadow_work); free(shadow_allocation);
      shadow_work = NULL; shadow_allocation = NULL; shadow_code = NULL;
      shadow_arena_size = 0; blocks = 512;
      RG_LOGE("new_dynarec RV32 shadow sample disabled: no diagnostic PSRAM");
      return;
    }
  }
  limit = count < TEST_INSNS ? count : TEST_INSNS;
  while (prefix < limit) {
    uint32_t word = words[prefix];
    unsigned op = word >> 26, f = word & 63;
    if (!(memory_load_opcode(op) || memory_store_opcode(op) ||
        (op >= 8 && op <= 15) || (!op &&
        (f == 0 || f == 2 || f == 3 || f == 4 || f == 6 || f == 7 ||
         (f >= 0x20 && f <= 0x27) || f == 0x2a || f == 0x2b)))) {
      blocker = op ? op : 64 + f;
      blocked = 1;
      break;
    }
    prefix++;
  }
  /* A terminal LW requires the next block's entry adapter, not this isolated
   * straight-line shadow compile. Keep it in coverage, but trim it from output. */
  unsigned end_op = prefix ? words[prefix - 1] >> 26 : 0;
  if (prefix && memory_load_opcode(end_op)) {
    prefix--;
    pending_boundary++;
    trimmed_load = 1;
  }
  if (count > TEST_INSNS && !blocked) capped++;
  blocks++;
  source_ops += count;
  covered = prefix;
  /* Candidate execution stops before memory, control flow, or a trapping ALU
   * operation. Select it independently of how the containing block ends. */
  {
    unsigned alu_prefix = 0;
    while (alu_prefix < limit) {
      unsigned op_i = words[alu_prefix] >> 26, f_i = words[alu_prefix] & 63;
      int safe = (op_i >= 9 && op_i <= 15) || (!op_i &&
        (f_i == 0 || f_i == 2 || f_i == 3 || f_i == 4 || f_i == 6 ||
         f_i == 7 || f_i == 0x21 || (f_i >= 0x23 && f_i <= 0x27) ||
         f_i == 0x2a || f_i == 0x2b));
      if (!safe) break;
      alu_prefix++;
    }
    if (alu_prefix > execute_count) {
      memcpy(execute_words, words, alu_prefix * sizeof(words[0]));
      execute_pc = pc;
      execute_count = alu_prefix;
    }
  }
  /* Retain a real aligned load plus a safe following ALU instruction so the
   * load-delay result is resolved inside the isolated generated sequence. */
  if (!execute_memory[0]) {
    for (unsigned i = 0; i + 1 < limit; i++) {
      unsigned mop = words[i] >> 26;
      unsigned nop = words[i + 1] >> 26, nf = words[i + 1] & 63;
      unsigned base = (words[i] >> 21) & 31;
      int load = memory_load_opcode(mop);
      int safe_next = (nop >= 9 && nop <= 15) || (!nop &&
        (nf == 0 || nf == 2 || nf == 3 || nf == 4 || nf == 6 ||
         nf == 7 || nf == 0x21 || (nf >= 0x23 && nf <= 0x27) ||
         nf == 0x2a || nf == 0x2b));
      if (load && base && safe_next) {
        execute_memory[0] = words[i];
        execute_memory[1] = words[i + 1];
        execute_memory_pc = pc + i * 4;
        break;
      }
    }
  }
  if (!execute_store[0]) {
    for (unsigned i = 0; i + 1 < limit; i++) {
      unsigned mop = words[i] >> 26;
      unsigned nop = words[i + 1] >> 26, nf = words[i + 1] & 63;
      unsigned base = (words[i] >> 21) & 31;
      int store = memory_store_opcode(mop);
      int safe_next = (nop >= 9 && nop <= 15) || (!nop &&
        (nf == 0 || nf == 2 || nf == 3 || nf == 4 || nf == 6 ||
         nf == 7 || nf == 0x21 || (nf >= 0x23 && nf <= 0x27) ||
         nf == 0x2a || nf == 0x2b));
      if (store && base && safe_next) {
        execute_store[0] = words[i];
        execute_store[1] = words[i + 1];
        execute_store_pc = pc + i * 4;
        break;
      }
    }
  }
  /* Keep the longest short straight-line prefix which mixes memory and ALU
   * while using one stable address base. That lets the hardware diagnostic
   * relocate every access into its private 256-byte RAM without changing the
   * real instruction words. It is the first whole-prefix execution probe,
   * beyond the deliberately narrow load/store pairs above. */
  for (unsigned n = 4; n <= prefix && n <= ARRAY_SIZE(execute_mixed); n++) {
    unsigned candidate_base;
    int32_t candidate_anchor;
    if (mixed_prefix_info(words, n, &candidate_base, &candidate_anchor) &&
        n > execute_mixed_count) {
      memcpy(execute_mixed, words, n * sizeof(words[0]));
      execute_mixed_pc = pc;
      execute_mixed_count = n;
      execute_mixed_base = candidate_base;
      execute_mixed_anchor = candidate_anchor;
    }
  }
  if (prefix || (blocked && prefix + 1 < count)) {
    struct compiler_workspace *saved = work;
    unsigned op = blocked ? words[prefix] >> 26 : 0;
    unsigned f = blocked ? words[prefix] & 63 : 0;
    int is_branch = blocked && !trimmed_load && prefix + 1 < count &&
      ((op >= 1 && op <= 7) || (!op && (f == 8 || f == 9)));
    int result = 0;
    work = shadow_work;
    memset(work, 0, sizeof(*work));
    memory_mode = 1; full_address_mode = 1;
    allow_pending_exit = 0; branch_pair = 0;
    linear_pc = pc; test_dispatcher = NULL;
    if (is_branch) {
      void *branch_code = (unsigned char *)shadow_code + 8192;
      unsigned prefix_bytes = 0;
      if (prefix) {
        test_dispatcher = branch_code;
        result = compile(shadow_code, 8192, words, prefix);
        prefix_bytes = (unsigned)rv32_output.count * 4;
      }
      test_dispatcher = NULL;
      if (!result) {
        allow_pending_exit = 1;
        result = compile_branch(branch_code, 8192, pc + prefix * 4,
          words[prefix], words[prefix + 1], 0);
        allow_pending_exit = 0;
      }
      if (!result && rv32_output.error == RV_OK) {
        covered = prefix + 2;
        complete = covered == count;
        branch_blocks++;
        code_bytes += prefix_bytes + 256 + (unsigned)rv32_output.count * 4;
        success = 1;
      }
    }
    else if (prefix && compile(shadow_code, 16384, words, prefix) == 0 &&
        rv32_output.error == RV_OK) {
      complete = prefix == count;
      code_bytes += (unsigned)rv32_output.count * 4;
      success = 1;
    }
    if (success) {
      compiled++;
      if (!execute_branch && is_branch && op != 2 && op != 3 && op != 0) {
        uint32_t delay = words[prefix + 1];
        unsigned dop = delay >> 26, df = delay & 63;
        unsigned rs = (words[prefix] >> 21) & 31;
        unsigned rt = (words[prefix] >> 16) & 31;
        int safe_delay = (dop >= 9 && dop <= 15) || (!dop &&
          (df == 0 || df == 2 || df == 3 || df == 4 || df == 6 ||
           df == 7 || df == 0x21 || (df >= 0x23 && df <= 0x27) ||
           df == 0x2a || df == 0x2b));
        int controllable = rs && (op != 4 && op != 5 ? 1 : rt && rt != rs);
        if (safe_delay && controllable) {
          execute_branch = words[prefix];
          execute_delay = delay;
          execute_branch_pc = pc + prefix * 4;
        }
      }
      if (is_branch && prefix <= ARRAY_SIZE(execute_whole)) {
        unsigned candidate_base, bop = words[prefix] >> 26;
        unsigned dop = words[prefix + 1] >> 26, df = words[prefix + 1] & 63;
        int32_t candidate_anchor;
        int safe_delay = (dop >= 9 && dop <= 15) || (!dop &&
          (df == 0 || df == 2 || df == 3 || df == 4 || df == 6 ||
           df == 7 || df == 0x21 || (df >= 0x23 && df <= 0x27) ||
           df == 0x2a || df == 0x2b));
        int conditional = bop == 1 || bop == 4 || bop == 5 ||
          bop == 6 || bop == 7;
        if (safe_delay && conditional &&
            mixed_prefix_info(words, prefix, &candidate_base, &candidate_anchor) &&
            prefix > execute_whole_count) {
          memcpy(execute_whole, words, prefix * sizeof(words[0]));
          execute_whole_branch = words[prefix];
          execute_whole_delay = words[prefix + 1];
          execute_whole_pc = pc;
          execute_whole_count = prefix;
          execute_whole_base = candidate_base;
          execute_whole_anchor = candidate_anchor;
        }
      }
    }
    else failed++;
    memory_mode = 0; full_address_mode = 0; linear_pc = 0;
    work = saved;
  }
  prefix_ops += covered;
  if (complete) full++;
  else if (blocked) blockers[blocker]++;
  if (blocks == 64 || blocks == 256 || blocks == 512) {
    unsigned top[4] = {0, 0, 0, 0};
    for (unsigned rank = 0; rank < 4; rank++) {
      uint32_t best_count = 0;
      for (unsigned i = 0; i < 128; i++) {
        int used = 0;
        for (unsigned prior = 0; prior < rank; prior++) used |= top[prior] == i;
        if (!used && blockers[i] > best_count) {
          top[rank] = i;
          best_count = blockers[i];
        }
      }
    }
    RG_LOGI("new_dynarec RV32 shadow (inline map): blocks=%u full=%u compiled=%u branch=%u failed=%u covered=%u/%u code=%uB alu_run=%u pending_end=%u capped=%u blockers=%02x/%02x:%u,%02x/%02x:%u,%02x/%02x:%u,%02x/%02x:%u",
      blocks, full, compiled, branch_blocks, failed, prefix_ops, source_ops, code_bytes,
      execute_count, pending_boundary, capped,
      top[0] < 64 ? top[0] : 0, top[0] < 64 ? 0xff : top[0] - 64, blockers[top[0]],
      top[1] < 64 ? top[1] : 0, top[1] < 64 ? 0xff : top[1] - 64, blockers[top[1]],
      top[2] < 64 ? top[2] : 0, top[2] < 64 ? 0xff : top[2] - 64, blockers[top[2]],
      top[3] < 64 ? top[3] : 0, top[3] < 64 ? 0xff : top[3] - 64, blockers[top[3]]);
  }
  if (blocks == 512) {
    int probe_ok = 0;
    struct compiler_workspace *saved_work = work;
    if (execute_count) {
      struct test_cpu actual = {0}, expected;
      work = shadow_work;
      memset(work, 0, sizeof(*work));
      memory_mode = 1; full_address_mode = 1; linear_pc = execute_pc;
      actual.cpu = psxRegs; actual.exception = 0;
      expected = actual;
      probe_ok = compile(shadow_code, 16384, execute_words, execute_count) == 0 &&
        rv32_output.error == RV_OK;
      for (unsigned i = 0; probe_ok && i < execute_count; i++)
        probe_ok = reference_step(expected.gpr, execute_words[i]) == 0;
      expected.cpu.pc = execute_pc + execute_count * 4;
      expected.cpu.cycle += execute_count;
      if (probe_ok) probe_ok = ndrc_rv32_p4_publish(shadow_code,
        rv32_output.count * 4) == 0;
      if (probe_ok) ndrc_rv32_enter(shadow_code, &actual, &actual.cpu.cycle);
      probe_ok = probe_ok && actual.exception == 0 &&
        actual.cpu.pc == expected.cpu.pc && actual.cpu.cycle == expected.cpu.cycle &&
        memcmp(actual.gpr, expected.gpr, sizeof(actual.gpr)) == 0;
      if (probe_ok)
        RG_LOGI("new_dynarec RV32 longest initial ALU run clone execute PASSED pc=%08x ops=%u",
          execute_pc, execute_count);
      else
        RG_LOGE("new_dynarec RV32 longest initial ALU run clone execute FAILED pc=%08x ops=%u",
          execute_pc, execute_count);
      memory_mode = 0; full_address_mode = 0; linear_pc = 0;
    }
    if (execute_branch) {
      int branch_ok = 1;
      unsigned op = execute_branch >> 26;
      unsigned rs = (execute_branch >> 21) & 31;
      unsigned rt = (execute_branch >> 16) & 31;
      unsigned link = op == 1 && (execute_branch & 0x100000) ? 31 : 0;
      int32_t displacement = (int32_t)(int16_t)execute_branch * 4;
      uint32_t target = execute_branch_pc + 4u + (uint32_t)displacement;
      memset(work, 0, sizeof(*work));
      memory_mode = 1;
      branch_ok = compile_branch(shadow_code, 16384, execute_branch_pc,
        execute_branch, execute_delay, 0) == 0 && rv32_output.error == RV_OK;
      if (branch_ok) branch_ok = ndrc_rv32_p4_publish(shadow_code,
        256 + rv32_output.count * 4) == 0;
      for (unsigned want_taken = 0; branch_ok && want_taken < 2; want_taken++) {
        struct test_cpu actual = {0}, expected;
        uint32_t source, second = 0;
        int taken;
        actual.cpu = psxRegs; actual.exception = 0;
        if (op == 4 || op == 5) {
          actual.gpr[rs] = 0x13579bdf;
          actual.gpr[rt] = (want_taken == (op == 4)) ? 0x13579bdf : 0x2468ace0;
        }
        else if (op == 6) actual.gpr[rs] = want_taken ? 0 : 1;
        else if (op == 7) actual.gpr[rs] = want_taken ? 1 : 0;
        else if (op == 1) {
          int branch_on_ge = (execute_branch & 0x10000) != 0;
          actual.gpr[rs] = (want_taken == (unsigned)branch_on_ge) ? 1 : 0x80000000u;
        }
        expected = actual;
        source = expected.gpr[rs];
        if (op == 4 || op == 5) second = expected.gpr[rt];
        taken = op == 4 ? source == second : op == 5 ? source != second :
          op == 6 ? (int32_t)source <= 0 : op == 7 ? (int32_t)source > 0 :
          (execute_branch & 0x10000) ? (int32_t)source >= 0 : (int32_t)source < 0;
        if (link) expected.gpr[link] = execute_branch_pc + 8u;
        branch_ok = reference_step(expected.gpr, execute_delay) == 0;
        expected.cpu.pc = taken ? target : execute_branch_pc + 8u;
        expected.cpu.cycle += 2;
        if (branch_ok) ndrc_rv32_enter(shadow_code, &actual, &actual.cpu.cycle);
        branch_ok = branch_ok && actual.exception == 0 &&
          actual.cpu.pc == expected.cpu.pc && actual.cpu.cycle == expected.cpu.cycle &&
          memcmp(actual.gpr, expected.gpr, sizeof(actual.gpr)) == 0;
      }
      if (branch_ok)
        RG_LOGI("new_dynarec RV32 real conditional branch clone execute PASSED pc=%08x op=%08x delay=%08x cases=2",
          execute_branch_pc, execute_branch, execute_delay);
      else
        RG_LOGE("new_dynarec RV32 real conditional branch clone execute FAILED pc=%08x op=%08x delay=%08x",
          execute_branch_pc, execute_branch, execute_delay);
      memory_mode = 0;
    }
    if (execute_memory[0]) {
      uintptr_t *lut = rg_alloc(65536 * sizeof(*lut), MEM_SLOW | MEM_NOPANIC);
      int memory_ok = lut != NULL;
      struct test_cpu actual = {0};
      uint32_t oracle_gpr[32], oracle_ram[64], oracle_pc = 0, oracle_cycles = 0;
      int oracle_status = -1;
      if (memory_ok) {
        unsigned base = (execute_memory[0] >> 21) & 31;
        int32_t imm = (int16_t)execute_memory[0];
        memset(lut, 0xff, 65536 * sizeof(*lut));
        actual.cpu = psxRegs;
        actual.cpu.cycle = 0;
        actual.cpu.dloadReg[0] = 0;
        actual.cpu.dloadVal[0] = 0;
        actual.exception = 0;
        for (unsigned i = 0; i < 32; i++) actual.gpr[i] = 0x10203040u + i * 0x01010101u;
        actual.gpr[0] = 0;
        actual.gpr[base] = 0x20u - (uint32_t)imm;
        for (unsigned i = 0; i < 64; i++) actual.ram[i] = 0x80010000u + i * 0x010203u;
        actual.cpu.ptrs.memRLUT = lut;
        actual.cpu.ptrs.memWLUT = lut;
        lut[0] = (uintptr_t)actual.ram;
        memcpy(oracle_gpr, actual.gpr, sizeof(oracle_gpr));
        memcpy(oracle_ram, actual.ram, sizeof(oracle_ram));
        oracle_status = ndrc_rv32_interpreter_memory(oracle_gpr, oracle_ram,
          execute_memory, 2, &oracle_pc, &oracle_cycles);
        memset(work, 0, sizeof(*work));
        memory_mode = 1; full_address_mode = 1; linear_pc = execute_memory_pc;
        memory_ok = oracle_status >= 0 &&
          compile(shadow_code, 16384, execute_memory, 2) == 0 &&
          rv32_output.error == RV_OK;
        if (memory_ok) memory_ok = ndrc_rv32_p4_publish(shadow_code,
          rv32_output.count * 4) == 0;
        if (memory_ok) ndrc_rv32_enter(shadow_code, &actual, &actual.cpu.cycle);
        memory_ok = memory_ok && actual.exception == (uint32_t)oracle_status &&
          actual.cpu.pc == execute_memory_pc + oracle_pc &&
          actual.cpu.cycle == oracle_cycles && LOAD_DELAY_CLEAR(actual) &&
          memcmp(actual.gpr, oracle_gpr, sizeof(oracle_gpr)) == 0 &&
          memcmp(actual.ram, oracle_ram, sizeof(oracle_ram)) == 0;
      }
      if (memory_ok)
        RG_LOGI("new_dynarec RV32 real load-delay pair clone execute PASSED pc=%08x load=%08x next=%08x",
          execute_memory_pc, execute_memory[0], execute_memory[1]);
      else
        RG_LOGE("new_dynarec RV32 real load-delay pair clone execute FAILED pc=%08x load=%08x next=%08x",
          execute_memory_pc, execute_memory[0], execute_memory[1]);
      free(lut);
      memory_mode = 0; full_address_mode = 0; linear_pc = 0;
    }
    if (execute_store[0]) {
      uintptr_t *lut = rg_alloc(65536 * sizeof(*lut), MEM_SLOW | MEM_NOPANIC);
      int store_ok = lut != NULL;
      struct test_cpu actual = {0};
      uint32_t oracle_gpr[32], oracle_ram[64], oracle_pc = 0, oracle_cycles = 0;
      int oracle_status = -1;
      if (store_ok) {
        unsigned base = (execute_store[0] >> 21) & 31;
        int32_t imm = (int16_t)execute_store[0];
        memset(lut, 0xff, 65536 * sizeof(*lut));
        actual.cpu = psxRegs;
        actual.cpu.cycle = 0;
        actual.cpu.dloadReg[0] = 0;
        actual.cpu.dloadVal[0] = 0;
        actual.exception = 0;
        for (unsigned i = 0; i < 32; i++) actual.gpr[i] = 0x50607080u + i * 0x01020304u;
        actual.gpr[0] = 0;
        actual.gpr[base] = 0x20u - (uint32_t)imm;
        for (unsigned i = 0; i < 64; i++) actual.ram[i] = 0xa0b00000u + i * 0x010305u;
        actual.cpu.ptrs.memRLUT = lut;
        actual.cpu.ptrs.memWLUT = lut;
        lut[0] = (uintptr_t)actual.ram;
        memcpy(oracle_gpr, actual.gpr, sizeof(oracle_gpr));
        memcpy(oracle_ram, actual.ram, sizeof(oracle_ram));
        oracle_status = ndrc_rv32_interpreter_memory(oracle_gpr, oracle_ram,
          execute_store, 2, &oracle_pc, &oracle_cycles);
        memset(work, 0, sizeof(*work));
        memory_mode = 1; full_address_mode = 1; linear_pc = execute_store_pc;
        store_ok = oracle_status >= 0 &&
          compile(shadow_code, 16384, execute_store, 2) == 0 &&
          rv32_output.error == RV_OK;
        if (store_ok) store_ok = ndrc_rv32_p4_publish(shadow_code,
          rv32_output.count * 4) == 0;
        if (store_ok) ndrc_rv32_enter(shadow_code, &actual, &actual.cpu.cycle);
        store_ok = store_ok && actual.exception == (uint32_t)oracle_status &&
          actual.cpu.pc == execute_store_pc + oracle_pc &&
          actual.cpu.cycle == oracle_cycles && LOAD_DELAY_CLEAR(actual) &&
          memcmp(actual.gpr, oracle_gpr, sizeof(oracle_gpr)) == 0 &&
          memcmp(actual.ram, oracle_ram, sizeof(oracle_ram)) == 0;
      }
      if (store_ok)
        RG_LOGI("new_dynarec RV32 real store pair clone execute PASSED pc=%08x store=%08x next=%08x",
          execute_store_pc, execute_store[0], execute_store[1]);
      else
        RG_LOGE("new_dynarec RV32 real store pair clone execute FAILED pc=%08x store=%08x next=%08x",
          execute_store_pc, execute_store[0], execute_store[1]);
      free(lut);
      memory_mode = 0; full_address_mode = 0; linear_pc = 0;
    }
    if (execute_mixed_count) {
      uintptr_t *lut = rg_alloc(65536 * sizeof(*lut), MEM_SLOW | MEM_NOPANIC);
      int mixed_ok = lut != NULL;
      struct test_cpu actual = {0};
      uint32_t oracle_gpr[32], oracle_ram[64], oracle_pc = 0, oracle_cycles = 0;
      int oracle_status = -1;
      if (mixed_ok) {
        memset(lut, 0xff, 65536 * sizeof(*lut));
        actual.cpu = psxRegs;
        actual.cpu.cycle = 0;
        actual.cpu.dloadReg[0] = 0;
        actual.cpu.dloadVal[0] = 0;
        actual.exception = 0;
        for (unsigned i = 0; i < 32; i++)
          actual.gpr[i] = 0x31415926u ^ (i * 0x01020408u);
        actual.gpr[0] = 0;
        actual.gpr[execute_mixed_base] =
          0x80u - (uint32_t)execute_mixed_anchor;
        for (unsigned i = 0; i < 64; i++)
          actual.ram[i] = 0x5a000000u ^ (i * 0x00010203u);
        actual.cpu.ptrs.memRLUT = lut;
        actual.cpu.ptrs.memWLUT = lut;
        lut[0] = (uintptr_t)actual.ram;
        memcpy(oracle_gpr, actual.gpr, sizeof(oracle_gpr));
        memcpy(oracle_ram, actual.ram, sizeof(oracle_ram));
        oracle_status = ndrc_rv32_interpreter_memory(oracle_gpr, oracle_ram,
          execute_mixed, execute_mixed_count, &oracle_pc, &oracle_cycles);
        memset(work, 0, sizeof(*work));
        memory_mode = 1; full_address_mode = 1; linear_pc = execute_mixed_pc;
        mixed_ok = oracle_status >= 0 &&
          compile(shadow_code, 16384, execute_mixed, execute_mixed_count) == 0 &&
          rv32_output.error == RV_OK;
        if (mixed_ok) mixed_ok = ndrc_rv32_p4_publish(shadow_code,
          rv32_output.count * 4) == 0;
        if (mixed_ok) ndrc_rv32_enter(shadow_code, &actual, &actual.cpu.cycle);
        mixed_ok = mixed_ok && actual.exception == (uint32_t)oracle_status &&
          actual.cpu.pc == execute_mixed_pc + oracle_pc &&
          actual.cpu.cycle == oracle_cycles && LOAD_DELAY_CLEAR(actual) &&
          memcmp(actual.gpr, oracle_gpr, sizeof(oracle_gpr)) == 0 &&
          memcmp(actual.ram, oracle_ram, sizeof(oracle_ram)) == 0;
      }
      if (mixed_ok)
        RG_LOGI("new_dynarec RV32 real mixed prefix clone execute PASSED pc=%08x ops=%u base=r%u",
          execute_mixed_pc, execute_mixed_count, execute_mixed_base);
      else
        RG_LOGE("new_dynarec RV32 real mixed prefix clone execute FAILED pc=%08x ops=%u base=r%u status=%d",
          execute_mixed_pc, execute_mixed_count, execute_mixed_base, oracle_status);
      free(lut);
      memory_mode = 0; full_address_mode = 0; linear_pc = 0;
    }
    if (execute_whole_count) {
      uintptr_t *lut = rg_alloc(65536 * sizeof(*lut), MEM_SLOW | MEM_NOPANIC);
      int whole_ok = lut != NULL;
      struct ndrc_rv32_block_cache cache;
      struct ndrc_rv32_cache_entry cache_entries[17];
      const struct ndrc_rv32_cache_entry *cached = NULL;
      uint32_t cache_offset = UINT32_MAX;
      uint32_t source_words[LIVE_CANARY_FETCH_WORDS], source_hash;
      void *cached_code = NULL;
      unsigned bop = execute_whole_branch >> 26;
      unsigned rs = (execute_whole_branch >> 21) & 31;
      unsigned rt = (execute_whole_branch >> 16) & 31;
      int32_t displacement = (int32_t)(int16_t)execute_whole_branch * 4;
      uint32_t branch_pc = execute_whole_pc + execute_whole_count * 4;
      uint32_t target = branch_pc + 4u + (uint32_t)displacement;
      void *branch_code = (unsigned char *)shadow_code + 8192;
      unsigned prefix_bytes = 0, branch_offset = 0;
      unsigned published_size = 0;
      memcpy(source_words, execute_whole,
        execute_whole_count * sizeof(source_words[0]));
      source_words[execute_whole_count] = execute_whole_branch;
      source_words[execute_whole_count + 1] = execute_whole_delay;
      source_hash = ndrc_rv32_source_hash(source_words, execute_whole_count + 2);
      if (whole_ok) {
        runtime_exit_mode = 1;
        whole_ok = ndrc_rv32_cache_init(&cache, cache_entries,
          ARRAY_SIZE(cache_entries), shadow_code, 16384) == 0;
        cached_code = shadow_code;
        memset(work, 0, sizeof(*work));
        memory_mode = 1; full_address_mode = 1; linear_pc = execute_whole_pc;
        /* Measurement pass: the prefix's absolute tail jump has fixed size,
         * so it tells us where its final aligned branch adapter can begin. */
        test_dispatcher = branch_code;
        whole_ok = whole_ok && compile(cached_code, 8192, execute_whole,
          execute_whole_count) == 0 && rv32_output.error == RV_OK;
        prefix_bytes = (unsigned)rv32_output.count * 4;
        test_dispatcher = NULL;
        if (whole_ok) {
          branch_offset = (prefix_bytes + 63u) & ~63u;
          branch_code = (unsigned char *)shadow_code + branch_offset;
          allow_pending_exit = 1;
          whole_ok = branch_offset < 16384 &&
            compile_branch(branch_code, 16384 - branch_offset, branch_pc,
            execute_whole_branch, execute_whole_delay, 0) == 0 &&
            rv32_output.error == RV_OK;
          allow_pending_exit = 0;
          published_size = branch_offset + 256 + (unsigned)rv32_output.count * 4;
          whole_ok = whole_ok && published_size <= 16384;
        }
        if (whole_ok) {
          cached_code = ndrc_rv32_cache_reserve(&cache, published_size, 64,
            &cache_offset);
          whole_ok = cached_code == shadow_code && cache_offset == 0;
        }
        if (whole_ok) {
          /* Final pass uses the compact adapter address recorded in the code. */
          memset(work, 0, sizeof(*work));
          test_dispatcher = branch_code;
          whole_ok = compile(cached_code, branch_offset, execute_whole,
            execute_whole_count) == 0 && rv32_output.error == RV_OK &&
            (unsigned)rv32_output.count * 4 == prefix_bytes;
          test_dispatcher = NULL;
        }
        if (whole_ok) {
          allow_pending_exit = 1;
          whole_ok = compile_branch(branch_code, 16384 - branch_offset,
            branch_pc, execute_whole_branch, execute_whole_delay, 0) == 0 &&
            rv32_output.error == RV_OK;
          allow_pending_exit = 0;
        }
        if (whole_ok)
          whole_ok = ndrc_rv32_p4_publish(shadow_code, published_size) == 0;
        if (whole_ok)
          whole_ok = ndrc_rv32_cache_insert(&cache, execute_whole_pc,
            (execute_whole_count + 2) * 4, cache_offset, published_size,
            0, source_hash) == 0;
        if (whole_ok) {
          cached = ndrc_rv32_cache_lookup(&cache, execute_whole_pc, source_hash);
          whole_ok = cached && cached->code_offset == cache_offset &&
            cached->code_bytes == published_size;
        }
      }
      for (unsigned run = 0; whole_ok && run < 1; run++) {
        struct test_cpu actual = {0};
        uint32_t oracle_gpr[32], oracle_ram[64], oracle_pc = 0, oracle_cycles = 0;
        int oracle_status;
        int taken;
        uint32_t runtime_status;
        memset(lut, 0xff, 65536 * sizeof(*lut));
        actual.cpu = psxRegs;
        actual.cpu.cycle = 0;
        actual.cpu.dloadReg[0] = 0;
        actual.cpu.dloadVal[0] = 0;
        actual.exception = 0xdeadbeefu;
        actual.branch_target = 0xcafebabeu;
        for (unsigned i = 0; i < 32; i++)
          actual.gpr[i] = 0x27182818u ^ (i * 0x02040810u);
        actual.gpr[0] = 0;
        actual.gpr[execute_whole_base] =
          0x80u - (uint32_t)execute_whole_anchor;
        for (unsigned i = 0; i < 64; i++)
          actual.ram[i] = 0x6b000000u ^ (i * 0x00030201u);
        actual.cpu.ptrs.memRLUT = lut;
        actual.cpu.ptrs.memWLUT = lut;
        lut[0] = (uintptr_t)actual.ram;
        memcpy(oracle_gpr, actual.gpr, sizeof(oracle_gpr));
        memcpy(oracle_ram, actual.ram, sizeof(oracle_ram));
        oracle_status = ndrc_rv32_interpreter_memory(oracle_gpr, oracle_ram,
          execute_whole, execute_whole_count, &oracle_pc, &oracle_cycles);
        if (oracle_status < 0) {
          whole_ok = 0;
          break;
        }
        taken = bop == 4 ? oracle_gpr[rs] == oracle_gpr[rt] :
          bop == 5 ? oracle_gpr[rs] != oracle_gpr[rt] :
          bop == 6 ? (int32_t)oracle_gpr[rs] <= 0 :
          bop == 7 ? (int32_t)oracle_gpr[rs] > 0 :
          (execute_whole_branch & 0x10000) ? (int32_t)oracle_gpr[rs] >= 0 :
          (int32_t)oracle_gpr[rs] < 0;
        if (bop == 1 && (execute_whole_branch & 0x100000))
          oracle_gpr[31] = branch_pc + 8u;
        if (reference_step(oracle_gpr, execute_whole_delay) != 0) {
          whole_ok = 0;
          break;
        }
        oracle_cycles += 2;
        runtime_status = ndrc_rv32_enter(cache.arena + cached->code_offset,
          &actual, &actual.cpu.cycle);
        whole_ok = runtime_status == 0 &&
          actual.exception == 0xdeadbeefu &&
          actual.branch_target == 0xcafebabeu &&
          actual.cpu.pc == (taken ? target : branch_pc + 8u) &&
          actual.cpu.cycle == oracle_cycles && LOAD_DELAY_CLEAR(actual) &&
          memcmp(actual.gpr, oracle_gpr, sizeof(oracle_gpr)) == 0 &&
          memcmp(actual.ram, oracle_ram, sizeof(oracle_ram)) == 0;
      }
      if (whole_ok)
        RG_LOGI("new_dynarec RV32 runtime-safe cached block clone execute PASSED pc=%08x ops=%u branch=%08x code=%uB status=0",
          execute_whole_pc, execute_whole_count + 2, execute_whole_branch,
          published_size);
      else
        RG_LOGE("new_dynarec RV32 runtime-safe cached block clone execute FAILED pc=%08x ops=%u branch=%08x",
          execute_whole_pc, execute_whole_count + 2, execute_whole_branch);
      if (whole_ok) {
        static const uint32_t fallback_words[2] = {
          0x8c220000u, /* LW r2, 0(r1): forced MMIO below. */
          0x24030001u  /* ADDIU r3, r0, 1: must not execute. */
        };
        struct test_cpu fallback = {0};
        uint32_t fallback_status = UINT32_MAX;
        unsigned fallback_size = 0;
        int fallback_ok;
        memset(lut, 0xff, 65536 * sizeof(*lut));
        fallback.cpu = psxRegs;
        fallback.cpu.pc = 0x80001000u;
        fallback.cpu.cycle = 0;
        fallback.cpu.dloadReg[0] = 0;
        fallback.cpu.dloadVal[0] = 0;
        fallback.gpr[1] = 0x1f801000u;
        fallback.gpr[2] = 0x11223344u;
        fallback.gpr[3] = 0x55667788u;
        fallback.cpu.ptrs.memRLUT = lut;
        fallback.cpu.ptrs.memWLUT = lut;
        fallback.exception = 0xdeadbeefu;
        fallback.branch_target = 0xcafebabeu;
        memset(work, 0, sizeof(*work));
        memory_mode = 1; full_address_mode = 1; linear_pc = fallback.cpu.pc;
        runtime_exit_mode = 1;
        /* Install a normal dispatcher which returns zero. A guarded memory
         * exit must bypass it and preserve status 5 in a0. */
        test_dispatcher = (unsigned char *)shadow_code + 8192;
        fallback_ok = compile(shadow_code, 8192, fallback_words, 2) == 0 &&
          rv32_output.error == RV_OK;
        test_dispatcher = NULL;
        if (fallback_ok) {
          rv32_init(&rv32_output, (unsigned char *)shadow_code + 8192,
            16384 - 8192);
          rv32_imm(&rv32_output, RV_ADD, RV_A0, RV_ZERO, 0);
          rv32_abs_jump(&rv32_output, RV_ZERO, RV_T6,
            (uint32_t)(uintptr_t)ndrc_rv32_leave);
          fallback_size = 8192 + (unsigned)rv32_output.count * 4;
          fallback_ok = rv32_output.error == RV_OK;
        }
        if (fallback_ok)
          fallback_ok = ndrc_rv32_p4_publish(shadow_code, fallback_size) == 0;
        if (fallback_ok)
          fallback_status = ndrc_rv32_enter(shadow_code, &fallback,
            &fallback.cpu.cycle);
        fallback_ok = fallback_ok && fallback_status == 5 &&
          fallback.cpu.pc == 0x80001000u &&
          fallback.gpr[2] == 0x11223344u &&
          fallback.gpr[3] == 0x55667788u &&
          fallback.exception == 0xdeadbeefu &&
          fallback.branch_target == 0xcafebabeu;
        if (fallback_ok)
          RG_LOGI("new_dynarec RV32 runtime-safe exact-PC fallback PASSED pc=%08x status=%u",
            fallback.cpu.pc, fallback_status);
        else
          RG_LOGE("new_dynarec RV32 runtime-safe exact-PC fallback FAILED pc=%08x status=%u",
            fallback.cpu.pc, fallback_status);
        if (fallback_ok) {
          static const uint32_t overflow_words[2] = {
            0x20220001u, /* ADDI r2, r1, 1: overflows for INT32_MAX. */
            0x24030001u  /* ADDIU r3, r0, 1: must not execute. */
          };
          uint32_t overflow_status = UINT32_MAX;
          memset(&fallback, 0, sizeof(fallback));
          fallback.cpu = psxRegs;
          fallback.cpu.pc = 0x80002000u;
          fallback.cpu.cycle = 0;
          fallback.cpu.dloadReg[0] = 0;
          fallback.cpu.dloadVal[0] = 0;
          fallback.gpr[1] = 0x7fffffffu;
          fallback.gpr[2] = 0x11223344u;
          fallback.gpr[3] = 0x55667788u;
          fallback.exception = 0xdeadbeefu;
          fallback.branch_target = 0xcafebabeu;
          memset(work, 0, sizeof(*work));
          memory_mode = 1; full_address_mode = 1;
          linear_pc = fallback.cpu.pc; runtime_exit_mode = 1;
          test_dispatcher = (unsigned char *)shadow_code + 8192;
          fallback_ok = compile(shadow_code, 8192, overflow_words,
            ARRAY_SIZE(overflow_words)) == 0 && rv32_output.error == RV_OK;
          test_dispatcher = NULL;
          if (fallback_ok)
            fallback_ok = ndrc_rv32_p4_publish(shadow_code,
              fallback_size) == 0;
          if (fallback_ok)
            overflow_status = ndrc_rv32_enter(shadow_code, &fallback,
              &fallback.cpu.cycle);
          fallback_ok = fallback_ok && overflow_status == 1 &&
            fallback.cpu.pc == 0x80002000u && fallback.cpu.cycle == 1 &&
            fallback.gpr[2] == 0x11223344u &&
            fallback.gpr[3] == 0x55667788u &&
            fallback.exception == 0xdeadbeefu &&
            fallback.branch_target == 0xcafebabeu;
          if (fallback_ok)
            RG_LOGI("new_dynarec RV32 runtime-safe overflow fallback PASSED pc=%08x status=%u",
              fallback.cpu.pc, overflow_status);
          else
            RG_LOGE("new_dynarec RV32 runtime-safe overflow fallback FAILED pc=%08x status=%u cycle=%u",
              fallback.cpu.pc, overflow_status, fallback.cpu.cycle);
        }
#ifdef PCSX_NDRC_RV32_LIVE_CANARY
        if (fallback_ok) {
          struct ndrc_rv32_cache_entry *live_entries = rg_alloc(
            LIVE_CANARY_CACHE_ENTRIES * sizeof(*live_entries),
            MEM_SLOW | MEM_NOPANIC);
          uintptr_t *live_dispatch_table = rg_alloc(
            LIVE_CANARY_DISPATCH_ENTRIES * sizeof(*live_dispatch_table),
            MEM_SLOW | MEM_NOPANIC);
          struct live_canary_link *live_links = rg_alloc(
            LIVE_CANARY_MAX_LINKS * sizeof(*live_links),
            MEM_SLOW | MEM_NOPANIC);
          struct live_canary_link *live_link_backup = rg_alloc(
            LIVE_CANARY_MAX_LINKS * sizeof(*live_link_backup),
            MEM_SLOW | MEM_NOPANIC);
          if (live_entries && live_dispatch_table && live_links &&
              live_link_backup) {
            live_canary.allocation = shadow_allocation;
            live_canary.arena = shadow_code;
            live_canary.code = NULL;
            live_canary.workspace = shadow_work;
            live_canary.entries = live_entries;
            live_canary.dispatch_table = live_dispatch_table;
            live_canary.links = live_links;
            live_canary.link_backup = live_link_backup;
            memset(live_links, 0,
              LIVE_CANARY_MAX_LINKS * sizeof(*live_links));
          }
          if (live_entries && live_dispatch_table && live_links &&
              live_link_backup &&
              ndrc_rv32_cache_init(&live_canary.cache,
              live_canary.entries, LIVE_CANARY_CACHE_ENTRIES, shadow_code,
              shadow_arena_size) == 0) {
            for (unsigned page = 0; page < LIVE_CANARY_SOURCE_PAGES; page++)
              live_canary_source_generation[page] = 1;
            __atomic_store_n(&live_canary.state, 4, __ATOMIC_RELEASE);
            shadow_allocation = NULL;
            shadow_work = NULL;
            RG_LOGI("new_dynarec RV32 live canary WAITING: cache=%uKB entries=%u",
              (unsigned)(shadow_arena_size / 1024u),
              LIVE_CANARY_CACHE_ENTRIES);
          }
          else {
            free(live_entries);
            free(live_dispatch_table);
            free(live_links);
            free(live_link_backup);
            memset(&live_canary, 0, sizeof(live_canary));
            RG_LOGE("new_dynarec RV32 live cache initialization FAILED; Lightrec retained");
          }
        }
#endif
      }
      free(lut);
      test_dispatcher = NULL; allow_pending_exit = 0; runtime_exit_mode = 0;
      memory_mode = 0; full_address_mode = 0; linear_pc = 0;
    }
    else {
      RG_LOGW("new_dynarec RV32 real whole-block clone execute NOT FOUND in blocks=%u",
        blocks);
    }
    work = saved_work;
    free(shadow_allocation); free(shadow_work);
    shadow_allocation = NULL; shadow_code = NULL; shadow_work = NULL;
    shadow_arena_size = 0;
  }
}
#endif
