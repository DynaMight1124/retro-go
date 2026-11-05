/* emit_test.c - check xjit_emit.h against the GNU assembler.
 *
 * For every emitter function, random operands (and the edges of every range)
 * are emitted into a buffer and written as assembly text. run_emit_test.sh
 * assembles the text with xtensa-esp32s3-elf-as (inside .begin no-transform,
 * so the assembler keeps each instruction as written) and compares the bytes.
 *   emit_test <out.S> <out.bin> <out.map> [seed]
 * out.map lists "offset length text" for every instruction, to name the first
 * mismatch. Also checks that out-of-range operands set e->bad. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "xjit_emit.h"

static xj_emit_t E;
static FILE *fs, *fm;
static uint8_t buf[1 << 20];
static int fails;

static int rnd(int lo, int hi) { return lo + (int)(((unsigned)rand() * 2654435761u >> 7) % (unsigned)(hi - lo + 1)); }
static int reg(void) { return rnd(0, 15); }

/* after emitting one instruction: its text, and where it is */
static int start;
static void begin(void) { start = E.pos; }
static void end(const char *fmt, ...)
{
    char txt[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(txt, sizeof(txt), fmt, ap);
    va_end(ap);
    if (E.bad) { fprintf(stderr, "emitter refused a valid operand: %s\n", txt); fails++; E.bad = false; }
    fprintf(fs, "    %s\n", txt);
    fprintf(fm, "%d %d %s\n", start, E.pos - start, txt);
}
/* pad to a 4-byte boundary with nop.n / nop (for entry, calls, l32r) */
static void align4(void)
{
    while (E.pos & 3)
    {
        if ((E.pos & 3) == 1) { begin(); xj_nop(&E); end("nop"); }
        else { begin(); xj_nop_n(&E); end("nop.n"); }
    }
}


static void expect_bad(const char *what, void (*f)(void))
{
    xj_emit_t save = E;
    E.bad = false;
    f();
    if (!E.bad) { fprintf(stderr, "out-of-range operand accepted: %s\n", what); fails++; }
    E = save;   /* drop whatever was written */
}
static void bad_movi(void) { xj_movi(&E, 2, 2048); }
static void bad_addi(void) { xj_addi(&E, 2, 3, 128); }
static void bad_l32i(void) { xj_l32i(&E, 2, 3, 1022); }
static void bad_l32i_off(void) { xj_l32i(&E, 2, 3, 6); }
static void bad_beq(void) { xj_beq(&E, 2, 3, 128); }
static void bad_beqz(void) { xj_beqz(&E, 2, 2048); }
static void bad_beqi(void) { xj_beqi(&E, 2, 9, 0); }
static void bad_slli(void) { xj_slli(&E, 2, 3, 0); }
static void bad_reg(void) { xj_add(&E, 16, 3, 4); }
static void bad_j(void) { xj_j(&E, 131072); }

int main(int argc, char **argv)
{
    if (argc < 4) { fprintf(stderr, "usage: emit_test out.S out.bin out.map [seed]\n"); return 2; }
    srand(argc > 4 ? atoi(argv[4]) : 1);
    fs = fopen(argv[1], "w");
    fm = fopen(argv[3], "w");
    xj_init(&E, buf, sizeof(buf));
    fprintf(fs, "    .text\n    .align 4\n    .begin no-transform\n");

    static const char *rst0[] = {"and", "or", "xor", "add", "addx2", "addx4", "addx8", "sub", "subx2", "subx4", "subx8"};
    static void (*const rst0f[])(xj_emit_t *, int, int, int) = {xj_and, xj_or, xj_xor, xj_add, xj_addx2, xj_addx4, xj_addx8, xj_sub, xj_subx2, xj_subx4, xj_subx8};
    static const char *rst2[] = {"saltu", "salt", "mull", "muluh", "mulsh", "quou", "quos", "remu", "rems", "min", "max", "minu", "maxu", "moveqz", "movnez", "movltz", "movgez", "src"};
    static void (*const rst2f[])(xj_emit_t *, int, int, int) = {xj_saltu, xj_salt, xj_mull, xj_muluh, xj_mulsh, xj_quou, xj_quos, xj_remu, xj_rems, xj_min, xj_max, xj_minu, xj_maxu, xj_moveqz, xj_movnez, xj_movltz, xj_movgez, xj_src};
    static const char *br[] = {"bnone", "beq", "blt", "bltu", "ball", "bbc", "bany", "bne", "bge", "bgeu", "bnall", "bbs"};
    static void (*const brf[])(xj_emit_t *, int, int, int) = {xj_bnone, xj_beq, xj_blt, xj_bltu, xj_ball, xj_bbc, xj_bany, xj_bne, xj_bge, xj_bgeu, xj_bnall, xj_bbs};
    static const char *bz[] = {"beqz", "bnez", "bltz", "bgez"};
    static void (*const bzf[])(xj_emit_t *, int, int) = {xj_beqz, xj_bnez, xj_bltz, xj_bgez};
    static const int b4c[] = {-1, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 32, 64, 128, 256};
    static const unsigned b4cu[] = {32768, 65536, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 32, 64, 128, 256};
    static const char *bi[] = {"beqi", "bnei", "blti", "bgei"};
    static void (*const bif[])(xj_emit_t *, int, int, int) = {xj_beqi, xj_bnei, xj_blti, xj_bgei};
    static const char *ls[] = {"l8ui", "l16ui", "l32i", "s8i", "s16i", "s32i", "l16si"};
    static void (*const lsf[])(xj_emit_t *, int, int, int) = {xj_l8ui, xj_l16ui, xj_l32i, xj_s8i, xj_s16i, xj_s32i, xj_l16si};
    static const int lssh[] = {0, 1, 2, 0, 1, 2, 1};

    for (int it = 0; it < 400; it++)
    {
        int r = reg(), s = reg(), t = reg(), k;
        for (k = 0; k < 11; k++) { begin(); rst0f[k](&E, r, s, t); end("%s a%d, a%d, a%d", rst0[k], r, s, t); r = reg(); s = reg(); t = reg(); }
        for (k = 0; k < 18; k++) { begin(); rst2f[k](&E, r, s, t); end("%s a%d, a%d, a%d", rst2[k], r, s, t); r = reg(); s = reg(); t = reg(); }
        begin(); xj_mov(&E, r, s); end("or a%d, a%d, a%d", r, s, s);
        begin(); xj_neg(&E, r, t); end("neg a%d, a%d", r, t);
        begin(); xj_abs(&E, s, t); end("abs a%d, a%d", s, t);
        k = rnd(7, 22); begin(); xj_sext(&E, r, s, k); end("sext a%d, a%d, %d", r, s, k);
        begin(); xj_nsau(&E, t, s); end("nsau a%d, a%d", t, s);
        begin(); xj_nsa(&E, r, t); end("nsa a%d, a%d", r, t);
        begin(); xj_ssr(&E, s); end("ssr a%d", s);
        begin(); xj_ssl(&E, t); end("ssl a%d", t);
        begin(); xj_ssa8l(&E, r); end("ssa8l a%d", r);
        k = rnd(0, 31); begin(); xj_ssai(&E, k); end("ssai %d", k);
        begin(); xj_srl(&E, r, t); end("srl a%d, a%d", r, t);
        begin(); xj_sra(&E, s, t); end("sra a%d, a%d", s, t);
        begin(); xj_sll(&E, r, s); end("sll a%d, a%d", r, s);
        k = rnd(1, 31); begin(); xj_slli(&E, r, s, k); end("slli a%d, a%d, %d", r, s, k);
        k = rnd(0, 31); begin(); xj_srai(&E, r, t, k); end("srai a%d, a%d, %d", r, t, k);
        k = rnd(0, 15); begin(); xj_srli(&E, s, t, k); end("srli a%d, a%d, %d", s, t, k);
        { int sh = rnd(0, 31), b = rnd(1, sh > 16 ? 32 - sh : 16); begin(); xj_extui(&E, r, t, sh, b); end("extui a%d, a%d, %d, %d", r, t, sh, b); }
        k = rnd(-2048, 2047); begin(); xj_movi(&E, t, k); end("movi a%d, %d", t, k);
        k = rnd(-128, 127); begin(); xj_addi(&E, r, s, k); end("addi a%d, a%d, %d", r, s, k);
        k = rnd(-128, 127) * 256; begin(); xj_addmi(&E, r, s, k); end("addmi a%d, a%d, %d", r, s, k);
        for (k = 0; k < 7; k++) { int off = rnd(0, 255) << lssh[k]; begin(); lsf[k](&E, t, s, off); end("%s a%d, a%d, %d", ls[k], t, s, off); t = reg(); s = reg(); }
        for (k = 0; k < 12; k++) { int d = rnd(-128, 127); begin(); brf[k](&E, s, t, d); end("%s a%d, a%d, . + %d", br[k], s, t, d + 4); s = reg(); t = reg(); }
        { int b = rnd(0, 31), d = rnd(-128, 127); begin(); xj_bbci(&E, s, b, d); end("bbci a%d, %d, . + %d", s, b, d + 4); }
        { int b = rnd(0, 31), d = rnd(-128, 127); begin(); xj_bbsi(&E, t, b, d); end("bbsi a%d, %d, . + %d", t, b, d + 4); }
        for (k = 0; k < 4; k++) { int d = rnd(-2048, 2047); if (E.pos + 4 + d < 0) d = -d; begin(); bzf[k](&E, s, d); end("%s a%d, . + %d", bz[k], s, d + 4); s = reg(); }
        for (k = 0; k < 4; k++) { int c = b4c[rnd(0, 15)], d = rnd(-128, 127); begin(); bif[k](&E, s, c, d); end("%s a%d, %d, . + %d", bi[k], s, c, d + 4); s = reg(); }
        { unsigned c = b4cu[rnd(0, 15)]; int d = rnd(-128, 127); begin(); xj_bltui(&E, s, c, d); end("bltui a%d, %u, . + %d", s, c, d + 4); }
        { unsigned c = b4cu[rnd(0, 15)]; int d = rnd(-128, 127); begin(); xj_bgeui(&E, t, c, d); end("bgeui a%d, %u, . + %d", t, c, d + 4); }
        k = rnd(-131072, 131071); if (E.pos + 4 + k < 0) k = -k - 8; begin(); xj_j(&E, k); end("j . + %d", k + 4);
        align4(); k = rnd(-131072, 131071) * 4; if (E.pos + 4 + k < 0) k = -k; begin(); xj_call0(&E, k); end("call0 . + %d", k + 4);
        align4(); k = rnd(-131072, 131071) * 4; if (E.pos + 4 + k < 0) k = -k; begin(); xj_call8(&E, k); end("call8 . + %d", k + 4);
        align4(); k = -4 * rnd(1, 65536); if (E.pos + k < 0) k = -4 * rnd(1, E.pos / 4 > 0 ? E.pos / 4 : 1); begin(); xj_l32r(&E, t, k); end("l32r a%d, . + %d", t, k);
        begin(); xj_jx(&E, s); end("jx a%d", s);
        begin(); xj_callx0(&E, s); end("callx0 a%d", s);
        begin(); xj_callx8(&E, t); end("callx8 a%d", t);
        begin(); xj_ret(&E); end("ret");
        begin(); xj_retw(&E); end("retw");
        align4(); k = rnd(0, 4095) * 8; begin(); xj_entry(&E, s, k); end("entry a%d, %d", s, k);
        begin(); xj_memw(&E); end("memw");
        begin(); xj_isync(&E); end("isync");
        begin(); xj_nop(&E); end("nop");
        begin(); xj_add_n(&E, r, s, t); end("add.n a%d, a%d, a%d", r, s, t);
        k = rnd(0, 15); if (k == 0) k = -1; begin(); xj_addi_n(&E, r, s, k); end("addi.n a%d, a%d, %d", r, s, k);
        begin(); xj_mov_n(&E, t, s); end("mov.n a%d, a%d", t, s);
        k = rnd(-32, 95); begin(); xj_movi_n(&E, s, k); end("movi.n a%d, %d", s, k);
        k = rnd(0, 15) * 4; begin(); xj_l32i_n(&E, t, s, k); end("l32i.n a%d, a%d, %d", t, s, k);
        k = rnd(0, 15) * 4; begin(); xj_s32i_n(&E, r, s, k); end("s32i.n a%d, a%d, %d", r, s, k);
        k = rnd(0, 63); begin(); xj_beqz_n(&E, s, k); end("beqz.n a%d, . + %d", s, k + 4);
        k = rnd(0, 63); begin(); xj_bnez_n(&E, t, k); end("bnez.n a%d, . + %d", t, k + 4);
        begin(); xj_ret_n(&E); end("ret.n");
        begin(); xj_retw_n(&E); end("retw.n");
        begin(); xj_nop_n(&E); end("nop.n");
    }
    fprintf(fs, "    .end no-transform\n");

    expect_bad("movi 2048", bad_movi);
    expect_bad("addi 128", bad_addi);
    expect_bad("l32i 1022", bad_l32i);
    expect_bad("l32i 6", bad_l32i_off);
    expect_bad("beq 128", bad_beq);
    expect_bad("beqz 2048", bad_beqz);
    expect_bad("beqi 9", bad_beqi);
    expect_bad("slli 0", bad_slli);
    expect_bad("a16", bad_reg);
    expect_bad("j 131072", bad_j);

    FILE *fb = fopen(argv[2], "wb");
    fwrite(buf, 1, E.pos, fb);
    fclose(fb);
    fclose(fs);
    fclose(fm);
    printf("emit_test: %d bytes emitted, %d emitter self-check failures\n", E.pos, fails);
    return fails != 0;
}
