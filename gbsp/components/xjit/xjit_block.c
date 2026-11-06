/* xjit_block.c - labels, literal pool and layout of one translated block
 * (see xjit_block.h). Host-compilable: no ESP-IDF dependency. */
#include <string.h>
#include "xjit_block.h"

void xjb_init(xj_block_t *b, uint8_t *scratch, int cap)
{
    xj_init(&b->e, scratch, cap);
    b->n_lits = b->n_lfix = b->n_labels = b->n_bfix = 0;
    b->full = false;
}

int xjb_label(xj_block_t *b)
{
    if (b->n_labels >= XJB_MAX_LABELS) { b->full = true; return 0; }
    b->labels[b->n_labels] = -1;
    return b->n_labels++;
}

void xjb_bind(xj_block_t *b, int label)
{
    if (label >= 0 && label < b->n_labels)
        b->labels[label] = b->e.pos;
}

static void add_bfix(xj_block_t *b, int pos, int label, xjb_fix_kind_t kind)
{
    if (b->n_bfix >= XJB_MAX_FIX || label < 0 || label >= b->n_labels) { b->full = true; return; }
    b->bfix[b->n_bfix].pos = pos;
    b->bfix[b->n_bfix].label = label;
    b->bfix[b->n_bfix].kind = kind;
    b->n_bfix++;
}

/* each branch is emitted with displacement 0 and patched at layout time */
void xjb_b8(xj_block_t *b, void (*emit)(xj_emit_t *, int, int, int), int s, int t, int label)
{ add_bfix(b, b->e.pos, label, XJB_FIX_B8); emit(&b->e, s, t, 0); }
void xjb_bi(xj_block_t *b, void (*emit)(xj_emit_t *, int, int, int), int s, int imm, int label)
{ add_bfix(b, b->e.pos, label, XJB_FIX_B8); emit(&b->e, s, imm, 0); }
void xjb_bbi(xj_block_t *b, void (*emit)(xj_emit_t *, int, int, int), int s, int bit, int label)
{ add_bfix(b, b->e.pos, label, XJB_FIX_B8); emit(&b->e, s, bit, 0); }
void xjb_bz(xj_block_t *b, void (*emit)(xj_emit_t *, int, int), int s, int label)
{ add_bfix(b, b->e.pos, label, XJB_FIX_B12); emit(&b->e, s, 0); }
void xjb_bz_n(xj_block_t *b, bool nez, int s, int label)
{
    add_bfix(b, b->e.pos, label, XJB_FIX_BN6);
    if (nez) xj_bnez_n(&b->e, s, 0);
    else xj_beqz_n(&b->e, s, 0);
}
void xjb_j(xj_block_t *b, int label) { add_bfix(b, b->e.pos, label, XJB_FIX_J18); xj_j(&b->e, 0); }

void xjb_lit(xj_block_t *b, int at, uint32_t value)
{
    int i;
    for (i = 0; i < b->n_lits; i++)
        if (b->lits[i] == value)
            break;
    if (i == b->n_lits)
    {
        if (b->n_lits >= XJB_MAX_LITS) { b->full = true; return; }
        b->lits[b->n_lits++] = value;
    }
    if (b->n_lfix >= XJB_MAX_FIX) { b->full = true; return; }
    b->lfix[b->n_lfix].pos = b->e.pos;
    b->lfix[b->n_lfix].lit = i;
    b->n_lfix++;
    xj_l32r(&b->e, at, -4);   /* placeholder, patched in xjb_finalize() */
}

void xjb_imm(xj_block_t *b, int at, uint32_t value)
{
    int32_t v = (int32_t)value;
    if (v >= -2048 && v <= 2047)
        xj_movi(&b->e, at, v);
    else
        xjb_lit(b, at, value);
}

int xjb_finalize(xj_block_t *b, void *dst, uint32_t exec_addr, int cap, xjb_write_fn write_word, int *entry_off)
{
    uint8_t *code = b->e.buf;
    int code_len = b->e.pos;
    int code_off = b->n_lits * 4;
    int total = (code_off + code_len + 3) & ~3;

    if (!xjb_ok(b) || total > cap || (exec_addr & 3))
        return -1;

    for (int i = 0; i < b->n_bfix; i++)
    {
        int pos = b->bfix[i].pos, target = b->labels[b->bfix[i].label];
        int d = target - (pos + 4);
        uint32_t w = code[pos] | (code[pos + 1] << 8) | (code[pos + 2] << 16);
        if (target < 0)
            return -1;   /* label never bound */
        switch (b->bfix[i].kind)
        {
        case XJB_FIX_B8:
            if (d < -128 || d > 127) return -1;
            w = (w & 0x00FFFF) | ((uint32_t)(d & 0xFF) << 16);
            break;
        case XJB_FIX_B12:
            if (d < -2048 || d > 2047) return -1;
            w = (w & 0x000FFF) | ((uint32_t)(d & 0xFFF) << 12);
            break;
        case XJB_FIX_J18:
            if (d < -131072 || d > 131071) return -1;
            w = (w & 0x3F) | ((uint32_t)(d & 0x3FFFF) << 6);
            break;
        case XJB_FIX_BN6:
            if (d < 0 || d > 63) return -1;
            w = (w & 0x0FCF) | ((uint32_t)(d & 15) << 12) | ((uint32_t)(d >> 4) << 4);   /* keep s, the branch kind (t bits 3:2), op0 */
            code[pos] = w;
            code[pos + 1] = w >> 8;
            continue;   /* 16-bit instruction */
        }
        code[pos] = w;
        code[pos + 1] = w >> 8;
        code[pos + 2] = w >> 16;
    }

    for (int i = 0; i < b->n_lfix; i++)
    {
        int pos = b->lfix[i].pos;
        uint32_t pc = exec_addr + code_off + pos;
        uint32_t lit = exec_addr + 4 * b->lfix[i].lit;
        int32_t d = (int32_t)(lit - ((pc + 3) & ~3u));
        if (d >= 0 || d < -262144)
            return -1;
        uint32_t w = code[pos] | (code[pos + 1] << 8) | (code[pos + 2] << 16);
        w = (w & 0xFF) | (((uint32_t)(d >> 2) & 0xFFFF) << 8);
        code[pos] = w;
        code[pos + 1] = w >> 8;
        code[pos + 2] = w >> 16;
    }

    for (int i = 0; i < b->n_lits; i++)
        write_word(dst, i, b->lits[i]);
    for (int off = 0; off < code_len; off += 4)
    {
        uint32_t w = 0;
        for (int k = 0; k < 4 && off + k < code_len; k++)
            w |= (uint32_t)code[off + k] << (8 * k);
        write_word(dst, (code_off + off) / 4, w);
    }
    *entry_off = code_off;
    return total;
}
