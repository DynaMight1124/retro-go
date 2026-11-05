/* xjit_block.h - one translated block: code with labels and a literal pool.
 *
 * A block is emitted into a scratch buffer with xjit_emit.h; branches to
 * labels and l32r loads of 32-bit constants are recorded as fixups. The
 * finished block is laid out as
 *
 *     [literal pool, 4-byte words][code, 4-byte aligned]
 *
 * (l32r only reads literals below itself) and copied to executable memory by
 * xjb_finalize(). Branch displacements depend only on code offsets; l32r
 * displacements are fixed at layout time.
 */
#ifndef XJIT_BLOCK_H
#define XJIT_BLOCK_H

#include "xjit_emit.h"

#define XJB_MAX_LITS   128
#define XJB_MAX_LABELS 256
#define XJB_MAX_FIX    512

typedef enum
{
    XJB_FIX_B8,     /* RRI8/BRI8 branch: imm8 in bits 16..23 */
    XJB_FIX_B12,    /* beqz/bnez/bltz/bgez: imm12 in bits 12..23 */
    XJB_FIX_J18,    /* j: offset18 in bits 6..23 */
    XJB_FIX_BN6,    /* beqz.n/bnez.n: imm6 split, forward only */
} xjb_fix_kind_t;

typedef struct
{
    xj_emit_t e;
    uint32_t lits[XJB_MAX_LITS];
    int n_lits;
    struct { int pos, lit; } lfix[XJB_MAX_FIX];
    int n_lfix;
    int labels[XJB_MAX_LABELS];     /* code offset, -1 while unbound */
    int n_labels;
    struct { int pos, label; uint8_t kind; } bfix[XJB_MAX_FIX];
    int n_bfix;
    bool full;                      /* a table overflowed: drop the block */
} xj_block_t;

void xjb_init(xj_block_t *b, uint8_t *scratch, int cap);
static inline bool xjb_ok(const xj_block_t *b) { return xj_ok(&b->e) && !b->full; }

/* labels */
int xjb_label(xj_block_t *b);               /* a new unbound label */
void xjb_bind(xj_block_t *b, int label);    /* the label is the current position */
static inline int xjb_here(xj_block_t *b) { int l = xjb_label(b); xjb_bind(b, l); return l; }

/* branches and jumps to labels */
void xjb_b8(xj_block_t *b, void (*emit)(xj_emit_t *, int, int, int), int s, int t, int label);
void xjb_bz(xj_block_t *b, void (*emit)(xj_emit_t *, int, int), int s, int label);
void xjb_bz_n(xj_block_t *b, bool nez, int s, int label);   /* beqz.n / bnez.n: forward, 0..63 bytes */
void xjb_bi(xj_block_t *b, void (*emit)(xj_emit_t *, int, int, int), int s, int imm, int label);
void xjb_bbi(xj_block_t *b, void (*emit)(xj_emit_t *, int, int, int), int s, int bit, int label);
void xjb_j(xj_block_t *b, int label);

/* constants: l32r from the pool, or movi when the value fits 12 bits */
void xjb_lit(xj_block_t *b, int at, uint32_t value);
void xjb_imm(xj_block_t *b, int at, uint32_t value);

/* Lay the block out at exec_addr (the address the code will run at) through
 * write_word(dst, word_index, word) - executable internal RAM takes only
 * 32-bit stores. Returns the block size in bytes and sets *entry_off to the
 * offset of the code, or -1 if the block is invalid or larger than cap. */
typedef void (*xjb_write_fn)(void *dst, int word_index, uint32_t word);
int xjb_finalize(xj_block_t *b, void *dst, uint32_t exec_addr, int cap, xjb_write_fn write_word, int *entry_off);

#endif
