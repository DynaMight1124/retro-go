/* xjit_emit.h - Xtensa LX7 (ESP32-S3) instruction emitter for the shared JIT core.
 *
 * Writes little-endian instruction words into a byte buffer (normal DRAM or
 * PSRAM); xjit_block.c lays the finished block out with its literal pool and
 * resolves label branches. Every encoding here is checked against
 * xtensa-esp32s3-elf-as by components/xjit/test (run_emit_test.sh).
 *
 * Operands are register numbers 0..15 (a0..a15). An operand that does not fit
 * its field sets e->bad instead of emitting a wrong instruction; callers check
 * xj_ok() once per block. Branch displacements are byte offsets relative to
 * the branch as the ISA defines them (target - (pc + 4) for most branches);
 * the label helpers in xjit_block.h compute them.
 *
 * Encodings follow the Xtensa ISA Reference Manual; the density (.n) forms
 * are used only where the caller asks for them. Reference for a few encodings:
 * Dragonfruit's m68k_jit_backend_xtensa.c (MIT, Kevin Fisher).
 */
#ifndef XJIT_EMIT_H
#define XJIT_EMIT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t *buf;   /* code buffer (byte addressable) */
    int pos;        /* next byte */
    int cap;        /* buffer size */
    bool overflow;  /* ran past cap: the block must be dropped */
    bool bad;       /* an operand did not fit its field */
} xj_emit_t;

static inline void xj_init(xj_emit_t *e, uint8_t *buf, int cap)
{
    e->buf = buf; e->pos = 0; e->cap = cap; e->overflow = false; e->bad = false;
}
static inline bool xj_ok(const xj_emit_t *e) { return !e->overflow && !e->bad; }

static inline void xj_byte(xj_emit_t *e, uint32_t b)
{
    if (e->pos < e->cap) e->buf[e->pos] = (uint8_t)b;
    else e->overflow = true;
    e->pos++;
}
static inline void xj_16(xj_emit_t *e, uint32_t w) { xj_byte(e, w); xj_byte(e, w >> 8); }
static inline void xj_24(xj_emit_t *e, uint32_t w) { xj_byte(e, w); xj_byte(e, w >> 8); xj_byte(e, w >> 16); }

#define XJ_CHECK(e, cond) do { if (!(cond)) (e)->bad = true; } while (0)
#define XJ_REG(e, r) XJ_CHECK(e, (unsigned)(r) < 16)

/* ---- instruction formats ------------------------------------------------ */
/* RRR: op2 op1 r s t op0 */
static inline void xj_rrr(xj_emit_t *e, int op2, int op1, int r, int s, int t, int op0)
{
    xj_24(e, ((op2 & 15) << 20) | ((op1 & 15) << 16) | ((r & 15) << 12) | ((s & 15) << 8) | ((t & 15) << 4) | (op0 & 15));
}
/* RRI8: imm8 r s t op0 */
static inline void xj_rri8(xj_emit_t *e, int imm8, int r, int s, int t, int op0)
{
    xj_24(e, ((imm8 & 0xFF) << 16) | ((r & 15) << 12) | ((s & 15) << 8) | ((t & 15) << 4) | (op0 & 15));
}

/* ---- ALU, register-register (op0 0, op1 0) ------------------------------- */
#define XJ_RST0(name, op2) \
    static inline void xj_##name(xj_emit_t *e, int ar, int as, int at) \
    { XJ_REG(e, ar); XJ_REG(e, as); XJ_REG(e, at); xj_rrr(e, op2, 0, ar, as, at, 0); }
XJ_RST0(and, 0x1)
XJ_RST0(or, 0x2)
XJ_RST0(xor, 0x3)
XJ_RST0(add, 0x8)
XJ_RST0(addx2, 0x9)
XJ_RST0(addx4, 0xA)
XJ_RST0(addx8, 0xB)
XJ_RST0(sub, 0xC)
XJ_RST0(subx2, 0xD)
XJ_RST0(subx4, 0xE)
XJ_RST0(subx8, 0xF)
#undef XJ_RST0

/* mov ar, as = or ar, as, as */
static inline void xj_mov(xj_emit_t *e, int ar, int as) { xj_or(e, ar, as, as); }
static inline void xj_neg(xj_emit_t *e, int ar, int at) { XJ_REG(e, ar); XJ_REG(e, at); xj_rrr(e, 6, 0, ar, 0, at, 0); }
static inline void xj_abs(xj_emit_t *e, int ar, int at) { XJ_REG(e, ar); XJ_REG(e, at); xj_rrr(e, 6, 0, ar, 1, at, 0); }

/* op1 2: multiply / divide */
#define XJ_RST2(name, op2) \
    static inline void xj_##name(xj_emit_t *e, int ar, int as, int at) \
    { XJ_REG(e, ar); XJ_REG(e, as); XJ_REG(e, at); xj_rrr(e, op2, 2, ar, as, at, 0); }
XJ_RST2(saltu, 0x6)   /* ar = as < at (unsigned) ? 1 : 0 */
XJ_RST2(salt, 0x7)    /* ar = as < at (signed) ? 1 : 0 */
XJ_RST2(mull, 0x8)
XJ_RST2(muluh, 0xA)
XJ_RST2(mulsh, 0xB)
XJ_RST2(quou, 0xC)
XJ_RST2(quos, 0xD)
XJ_RST2(remu, 0xE)
XJ_RST2(rems, 0xF)
#undef XJ_RST2

/* op1 3: min/max, conditional moves */
#define XJ_RST3(name, op2) \
    static inline void xj_##name(xj_emit_t *e, int ar, int as, int at) \
    { XJ_REG(e, ar); XJ_REG(e, as); XJ_REG(e, at); xj_rrr(e, op2, 3, ar, as, at, 0); }
XJ_RST3(min, 0x4)
XJ_RST3(max, 0x5)
XJ_RST3(minu, 0x6)
XJ_RST3(maxu, 0x7)
XJ_RST3(moveqz, 0x8)
XJ_RST3(movnez, 0x9)
XJ_RST3(movltz, 0xA)
XJ_RST3(movgez, 0xB)
#undef XJ_RST3

/* sext ar, as, bit (7..22): sign-extend from bit */
static inline void xj_sext(xj_emit_t *e, int ar, int as, int bit)
{ XJ_REG(e, ar); XJ_REG(e, as); XJ_CHECK(e, bit >= 7 && bit <= 22); xj_rrr(e, 2, 3, ar, as, bit - 7, 0); }
/* nsau at, as: leading zeros (32 for 0); nsa: redundant sign bits */
static inline void xj_nsau(xj_emit_t *e, int at, int as) { XJ_REG(e, at); XJ_REG(e, as); xj_rrr(e, 4, 0, 0xF, as, at, 0); }
static inline void xj_nsa(xj_emit_t *e, int at, int as) { XJ_REG(e, at); XJ_REG(e, as); xj_rrr(e, 4, 0, 0xE, as, at, 0); }

/* ---- shifts --------------------------------------------------------------- */
/* SAR setup: ssr as (right by as&31), ssl as (left by as&31), ssai sa, ssa8l as */
static inline void xj_ssr(xj_emit_t *e, int as) { XJ_REG(e, as); xj_rrr(e, 4, 0, 0, as, 0, 0); }
static inline void xj_ssl(xj_emit_t *e, int as) { XJ_REG(e, as); xj_rrr(e, 4, 0, 1, as, 0, 0); }
static inline void xj_ssa8l(xj_emit_t *e, int as) { XJ_REG(e, as); xj_rrr(e, 4, 0, 2, as, 0, 0); }
static inline void xj_ssai(xj_emit_t *e, int sa)
{ XJ_CHECK(e, sa >= 0 && sa <= 31); xj_rrr(e, 4, 0, 4, sa & 15, sa >> 4, 0); }
/* shifts by SAR: srl ar, at / sra ar, at / sll ar, as / src ar, as, at */
static inline void xj_srl(xj_emit_t *e, int ar, int at) { XJ_REG(e, ar); XJ_REG(e, at); xj_rrr(e, 9, 1, ar, 0, at, 0); }
static inline void xj_sra(xj_emit_t *e, int ar, int at) { XJ_REG(e, ar); XJ_REG(e, at); xj_rrr(e, 0xB, 1, ar, 0, at, 0); }
static inline void xj_sll(xj_emit_t *e, int ar, int as) { XJ_REG(e, ar); XJ_REG(e, as); xj_rrr(e, 0xA, 1, ar, as, 0, 0); }
static inline void xj_src(xj_emit_t *e, int ar, int as, int at) { XJ_REG(e, ar); XJ_REG(e, as); XJ_REG(e, at); xj_rrr(e, 8, 1, ar, as, at, 0); }
/* immediate shifts */
static inline void xj_slli(xj_emit_t *e, int ar, int as, int sa)   /* sa 1..31 */
{
    XJ_REG(e, ar); XJ_REG(e, as); XJ_CHECK(e, sa >= 1 && sa <= 31);
    int x = 32 - sa;
    xj_rrr(e, (x >> 4) & 1, 1, ar, as, x & 15, 0);
}
static inline void xj_srai(xj_emit_t *e, int ar, int at, int sa)   /* sa 0..31 */
{ XJ_REG(e, ar); XJ_REG(e, at); XJ_CHECK(e, sa >= 0 && sa <= 31); xj_rrr(e, 2 | (sa >> 4), 1, ar, sa & 15, at, 0); }
static inline void xj_srli(xj_emit_t *e, int ar, int at, int sa)   /* sa 0..15 */
{ XJ_REG(e, ar); XJ_REG(e, at); XJ_CHECK(e, sa >= 0 && sa <= 15); xj_rrr(e, 4, 1, ar, sa, at, 0); }
/* extui ar, at, shift, bits: (at >> shift) & ((1 << bits) - 1), bits 1..16 */
static inline void xj_extui(xj_emit_t *e, int ar, int at, int shift, int bits)
{
    XJ_REG(e, ar); XJ_REG(e, at); XJ_CHECK(e, shift >= 0 && shift <= 31 && bits >= 1 && bits <= 16 && shift + bits <= 32);
    xj_rrr(e, bits - 1, 4 | (shift >> 4), ar, shift & 15, at, 0);
}

/* ---- immediates ----------------------------------------------------------- */
static inline void xj_movi(xj_emit_t *e, int at, int imm)           /* -2048..2047 */
{
    XJ_REG(e, at); XJ_CHECK(e, imm >= -2048 && imm <= 2047);
    xj_rri8(e, imm & 0xFF, 0xA, (imm >> 8) & 15, at, 2);
}
static inline void xj_addi(xj_emit_t *e, int at, int as, int imm)   /* -128..127 */
{ XJ_REG(e, at); XJ_REG(e, as); XJ_CHECK(e, imm >= -128 && imm <= 127); xj_rri8(e, imm & 0xFF, 0xC, as, at, 2); }
static inline void xj_addmi(xj_emit_t *e, int at, int as, int imm)  /* multiple of 256, -32768..32512 */
{ XJ_REG(e, at); XJ_REG(e, as); XJ_CHECK(e, !(imm & 255) && imm >= -32768 && imm <= 32512); xj_rri8(e, (imm >> 8) & 0xFF, 0xD, as, at, 2); }

/* ---- loads and stores (offset in bytes, scaled by the access size) -------- */
#define XJ_LS(name, r, shift) \
    static inline void xj_##name(xj_emit_t *e, int at, int as, int off) \
    { XJ_REG(e, at); XJ_REG(e, as); XJ_CHECK(e, off >= 0 && !(off & ((1 << shift) - 1)) && (off >> shift) <= 255); \
      xj_rri8(e, off >> shift, r, as, at, 2); }
XJ_LS(l8ui, 0x0, 0)
XJ_LS(l16ui, 0x1, 1)
XJ_LS(l32i, 0x2, 2)
XJ_LS(s8i, 0x4, 0)
XJ_LS(s16i, 0x5, 1)
XJ_LS(s32i, 0x6, 2)
XJ_LS(l16si, 0x9, 1)
#undef XJ_LS

/* l32r at, <literal>: disp = literal - ((pc + 3) & ~3), a multiple of 4 in
 * -262144..-4 (literals always sit below the code). */
static inline void xj_l32r(xj_emit_t *e, int at, int disp)
{
    XJ_REG(e, at); XJ_CHECK(e, !(disp & 3) && disp < 0 && disp >= -262144);
    xj_24(e, (((uint32_t)(disp >> 2) & 0xFFFF) << 8) | (at << 4) | 1);
}

/* ---- branches ------------------------------------------------------------- */
/* RRI8 compare-and-branch, disp = target - (pc + 4), -128..127 */
#define XJ_B(name, r) \
    static inline void xj_##name(xj_emit_t *e, int as, int at, int disp) \
    { XJ_REG(e, as); XJ_REG(e, at); XJ_CHECK(e, disp >= -128 && disp <= 127); xj_rri8(e, disp & 0xFF, r, as, at, 7); }
XJ_B(bnone, 0x0)
XJ_B(beq, 0x1)
XJ_B(blt, 0x2)
XJ_B(bltu, 0x3)
XJ_B(ball, 0x4)
XJ_B(bbc, 0x5)
XJ_B(bany, 0x8)
XJ_B(bne, 0x9)
XJ_B(bge, 0xA)
XJ_B(bgeu, 0xB)
XJ_B(bnall, 0xC)
XJ_B(bbs, 0xD)
#undef XJ_B
/* bbci/bbsi as, bit, disp: branch if bit clear / set */
static inline void xj_bbci(xj_emit_t *e, int as, int bit, int disp)
{ XJ_REG(e, as); XJ_CHECK(e, bit >= 0 && bit <= 31 && disp >= -128 && disp <= 127); xj_rri8(e, disp & 0xFF, 0x6 | (bit >> 4), as, bit & 15, 7); }
static inline void xj_bbsi(xj_emit_t *e, int as, int bit, int disp)
{ XJ_REG(e, as); XJ_CHECK(e, bit >= 0 && bit <= 31 && disp >= -128 && disp <= 127); xj_rri8(e, disp & 0xFF, 0xE | (bit >> 4), as, bit & 15, 7); }

/* BRI12 compare-with-zero, disp -2048..2047 */
#define XJ_BZ(name, m) \
    static inline void xj_##name(xj_emit_t *e, int as, int disp) \
    { XJ_REG(e, as); XJ_CHECK(e, disp >= -2048 && disp <= 2047); \
      xj_24(e, (((uint32_t)disp & 0xFFF) << 12) | (as << 8) | (m << 6) | (1 << 4) | 6); }
XJ_BZ(beqz, 0)
XJ_BZ(bnez, 1)
XJ_BZ(bltz, 2)
XJ_BZ(bgez, 3)
#undef XJ_BZ

/* compare with a b4const / b4constu immediate: -1 if the value has no code */
static inline int xj_b4const(int v)
{
    static const int t[16] = {-1, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 32, 64, 128, 256};
    for (int i = 0; i < 16; i++) if (t[i] == v) return i;
    return -1;
}
static inline int xj_b4constu(unsigned v)
{
    static const unsigned t[16] = {32768, 65536, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 32, 64, 128, 256};
    for (int i = 0; i < 16; i++) if (t[i] == v) return i;
    return -1;
}
/* BRI8 (op0 6, n 2 / n 3): beqi bnei blti bgei (b4const), bltui bgeui (b4constu) */
static inline void xj_bri8(xj_emit_t *e, int n, int m, int as, int code, int disp)
{
    XJ_REG(e, as); XJ_CHECK(e, code >= 0 && disp >= -128 && disp <= 127);
    xj_24(e, ((disp & 0xFF) << 16) | ((code & 15) << 12) | (as << 8) | (m << 6) | (n << 4) | 6);
}
static inline void xj_beqi(xj_emit_t *e, int as, int imm, int disp) { xj_bri8(e, 2, 0, as, xj_b4const(imm), disp); }
static inline void xj_bnei(xj_emit_t *e, int as, int imm, int disp) { xj_bri8(e, 2, 1, as, xj_b4const(imm), disp); }
static inline void xj_blti(xj_emit_t *e, int as, int imm, int disp) { xj_bri8(e, 2, 2, as, xj_b4const(imm), disp); }
static inline void xj_bgei(xj_emit_t *e, int as, int imm, int disp) { xj_bri8(e, 2, 3, as, xj_b4const(imm), disp); }
static inline void xj_bltui(xj_emit_t *e, int as, unsigned imm, int disp) { xj_bri8(e, 3, 2, as, xj_b4constu(imm), disp); }
static inline void xj_bgeui(xj_emit_t *e, int as, unsigned imm, int disp) { xj_bri8(e, 3, 3, as, xj_b4constu(imm), disp); }

/* ---- jumps and calls ------------------------------------------------------ */
/* j: disp = target - (pc + 4), -131072..131071 */
static inline void xj_j(xj_emit_t *e, int disp)
{ XJ_CHECK(e, disp >= -131072 && disp <= 131071); xj_24(e, (((uint32_t)disp & 0x3FFFF) << 6) | 6); }
/* call0/call8: target = ((pc & ~3) + 4) + disp, disp a multiple of 4 */
static inline void xj_calln(xj_emit_t *e, int n, int disp)
{ XJ_CHECK(e, !(disp & 3) && disp >= -524288 && disp <= 524284); xj_24(e, (((uint32_t)(disp >> 2) & 0x3FFFF) << 6) | (n << 4) | 5); }
static inline void xj_call0(xj_emit_t *e, int disp) { xj_calln(e, 0, disp); }
static inline void xj_call8(xj_emit_t *e, int disp) { xj_calln(e, 2, disp); }
/* register-indirect: jx, callx0, callx8 (args in a10.., result in a10 for callx8) */
static inline void xj_jx(xj_emit_t *e, int as) { XJ_REG(e, as); xj_rrr(e, 0, 0, 0, as, 0xA, 0); }
static inline void xj_callx0(xj_emit_t *e, int as) { XJ_REG(e, as); xj_rrr(e, 0, 0, 0, as, 0xC, 0); }
static inline void xj_callx8(xj_emit_t *e, int as) { XJ_REG(e, as); xj_rrr(e, 0, 0, 0, as, 0xE, 0); }
static inline void xj_ret(xj_emit_t *e) { xj_rrr(e, 0, 0, 0, 0, 0x8, 0); }
static inline void xj_retw(xj_emit_t *e) { xj_rrr(e, 0, 0, 0, 0, 0x9, 0); }
/* entry as, framesize (multiple of 8, <= 32760); must sit at a 4-byte aligned address */
static inline void xj_entry(xj_emit_t *e, int as, int frame)
{
    XJ_REG(e, as); XJ_CHECK(e, !(frame & 7) && frame >= 0 && frame <= 32760 && !(e->pos & 3));
    xj_24(e, (((uint32_t)frame >> 3) << 12) | (as << 8) | (3 << 4) | 6);
}

/* ---- sync ----------------------------------------------------------------- */
static inline void xj_memw(xj_emit_t *e) { xj_rrr(e, 0, 0, 2, 0, 0xC, 0); }
static inline void xj_isync(xj_emit_t *e) { xj_rrr(e, 0, 0, 2, 0, 0x0, 0); }
static inline void xj_nop(xj_emit_t *e) { xj_rrr(e, 0, 0, 2, 0, 0xF, 0); }

/* ---- density (16-bit) forms ---------------------------------------------- */
static inline void xj_add_n(xj_emit_t *e, int ar, int as, int at)
{ XJ_REG(e, ar); XJ_REG(e, as); XJ_REG(e, at); xj_16(e, (ar << 12) | (as << 8) | (at << 4) | 0xA); }
static inline void xj_addi_n(xj_emit_t *e, int ar, int as, int imm)  /* -1, 1..15 */
{
    XJ_REG(e, ar); XJ_REG(e, as); XJ_CHECK(e, imm == -1 || (imm >= 1 && imm <= 15));
    xj_16(e, (ar << 12) | (as << 8) | ((imm == -1 ? 0 : imm) << 4) | 0xB);
}
static inline void xj_mov_n(xj_emit_t *e, int at, int as)
{ XJ_REG(e, at); XJ_REG(e, as); xj_16(e, (0 << 12) | (as << 8) | (at << 4) | 0xD); }
static inline void xj_movi_n(xj_emit_t *e, int as, int imm)         /* -32..95 */
{
    XJ_REG(e, as); XJ_CHECK(e, imm >= -32 && imm <= 95);
    xj_16(e, ((imm & 15) << 12) | (as << 8) | (((imm >> 4) & 7) << 4) | 0xC);
}
static inline void xj_l32i_n(xj_emit_t *e, int at, int as, int off) /* 0..60 */
{ XJ_REG(e, at); XJ_REG(e, as); XJ_CHECK(e, off >= 0 && off <= 60 && !(off & 3)); xj_16(e, ((off >> 2) << 12) | (as << 8) | (at << 4) | 0x8); }
static inline void xj_s32i_n(xj_emit_t *e, int at, int as, int off)
{ XJ_REG(e, at); XJ_REG(e, as); XJ_CHECK(e, off >= 0 && off <= 60 && !(off & 3)); xj_16(e, ((off >> 2) << 12) | (as << 8) | (at << 4) | 0x9); }
/* beqz.n / bnez.n as, disp: forward only, target = pc + 4 + disp, disp 0..63 */
static inline void xj_beqz_n(xj_emit_t *e, int as, int disp)
{ XJ_REG(e, as); XJ_CHECK(e, disp >= 0 && disp <= 63); xj_16(e, ((disp & 15) << 12) | (as << 8) | ((0x8 | (disp >> 4)) << 4) | 0xC); }
static inline void xj_bnez_n(xj_emit_t *e, int as, int disp)
{ XJ_REG(e, as); XJ_CHECK(e, disp >= 0 && disp <= 63); xj_16(e, ((disp & 15) << 12) | (as << 8) | ((0xC | (disp >> 4)) << 4) | 0xC); }
static inline void xj_ret_n(xj_emit_t *e) { xj_16(e, 0xF00D); }
static inline void xj_retw_n(xj_emit_t *e) { xj_16(e, 0xF01D); }
static inline void xj_nop_n(xj_emit_t *e) { xj_16(e, 0xF03D); }

#endif
