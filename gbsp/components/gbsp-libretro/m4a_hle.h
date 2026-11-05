/* esp32-emu-turbo: native m4a mixer loop (included by cpu.cpp, RETRO_GO only).
 *
 * Nintendo's m4a sound engine mixes every PCM channel with a small ARM loop
 * copied to IWRAM (SoundMainRAM): read a signed 8-bit sample, interpolate,
 * scale by the channel's left/right volume, accumulate into the PCM buffer.
 * It is 30-45% of all instructions in Sonic Advance, Metal Slug Advance and
 * TMNT. The loop is found in IWRAM by its code (stereo and mono versions)
 * and run here natively, one ARM instruction at a time with the exact
 * register, flag, memory and cycle effects of the interpreter, returning to
 * it at the same instruction when the cycle budget runs out or the loop
 * exits, so the emulation stays bit-identical.
 */

/* stereo: Sonic Advance, TMNT; mono: Metal Slug Advance. Word 15/11 is the
   loop-exit "ble" whose offset depends on the build: only its top byte is
   compared. */
static const u32 m4a_sig_stereo[27] = {
  0xe5956000, 0xe5957630, 0xe009019e, 0xe0809bc9, 0xe00c099a, 0xe3ccc8ff,
  0xe08c6466, 0xe00c099b, 0xe3ccc8ff, 0xe08c7467, 0xe08ee004, 0xe1b09bae,
  0x0a000007, 0xe3cee5fe, 0xe0522009, 0xdaffffce, 0xe2599001, 0x00800001,
  0x11b300d9, 0xe1f310d1, 0xe0411000, 0xe2955101, 0x3affffea, 0xe5857630,
  0xe4856004, 0xe2588004, 0xcaffffe4 };
static const u32 m4a_sig_mono[22] = {
  0xe5956000, 0xe009019e, 0xe0809bc9, 0xe00c099a, 0xe3ccc8ff, 0xe08c6466,
  0xe08ee004, 0xe1b09bae, 0x0a000007, 0xe3cee5fe, 0xe0522009, 0xdaffffd4,
  0xe2599001, 0x00800001, 0x11b300d9, 0xe1f310d1, 0xe0411000, 0xe2955101,
  0x3affffed, 0xe4856004, 0xe2588004, 0xcaffffe9 };

static u32 m4a_kind;                    /* 0 none, 1 stereo, 2 mono */
static u32 m4a_base;                    /* address of "ldr r6, [r5]" */
static u32 m4a_exit;                    /* target of the "ble" */
static u32 m4a_pc_out = 0xFFFFFFFF;     /* outer loop head */
static u32 m4a_pc_in = 0xFFFFFFFF;      /* inner loop head */
static u32 m4a_scan_count;
extern "C" { int m4a_disable; }   /* PC harness A/B */

static bool m4a_match(const u32 *w, const u32 *sig, int n, int ble)
{
  for (int i = 0; i < n; i++)
    if (i == ble ? (w[i] >> 24) != (sig[i] >> 24) : w[i] != sig[i])
      return false;
  return true;
}

static void m4a_set(u32 kind, u32 base, const u32 *w, int ble)
{
  u32 ble_addr = base + ble * 4;
  m4a_kind = kind;
  m4a_base = base;
  m4a_exit = ble_addr + 8 + ((s32)(w[ble] << 8) >> 6);
  m4a_pc_out = base;
  m4a_pc_in = base + (kind == 1 ? 0x08 : 0x04);
}

/* once per frame: keep the loop's address current (the code is copied to
   IWRAM at boot and could be replaced); a full scan every 32 frames while
   it is not found */
static void m4a_check(void)
{
  const u32 *iw = (const u32 *)(iwram + 0x8000 * SMC_DETECTION);   /* the code, past the dynarec's SMC tags */
  if (m4a_disable)
    return;
  if (m4a_kind)
  {
    const u32 *w = iw + ((m4a_base & 0x7FFF) >> 2);
    if (m4a_kind == 1 ? m4a_match(w, m4a_sig_stereo, 27, 15) : m4a_match(w, m4a_sig_mono, 22, 11))
      return;
    m4a_kind = 0;
    m4a_pc_out = m4a_pc_in = 0xFFFFFFFF;
  }
  if (m4a_scan_count++ & 31)
    return;
  for (u32 i = 0; i + 27 <= 0x8000 / 4; i++)
  {
    if (iw[i] != 0xe5956000)
      continue;
    if (m4a_match(iw + i, m4a_sig_stereo, 27, 15))
      return m4a_set(1, 0x03000000 + i * 4, iw + i, 15);
    if (m4a_match(iw + i, m4a_sig_mono, 22, 11))
      return m4a_set(2, 0x03000000 + i * 4, iw + i, 11);
  }
}

static inline u32 m4a_rd32(u32 a)
{
  u8 *map = memory_map_read[a >> 15];
  return map ? readaddress32(map, a & 0x7FFF) : read_memory32(a);
}

static inline u32 m4a_rd8s(u32 a)
{
  u8 *map = memory_map_read[a >> 15];
  return (u32)(s32)(s8)(map ? map[a & 0x7FFF] : read_memory8(a));
}

static inline void m4a_wr32(u32 a, u32 v)
{
  if ((a >> 24) == 0x02)
    address32(ewram, a & 0x3FFFF) = eswap32(v);
  else
    address32(iwram, (a & 0x7FFF) + 0x8000 * SMC_DETECTION) = eswap32(v);
}

#ifdef GBAPROF
#define M4A_COUNT() gbaprof_instr++
#else
#define M4A_COUNT()
#endif

/* one instruction retired at offset "off" of the loop: the interpreter's
   tail cost, and a stop at the next instruction when the budget is spent */
#define M4A_STEP(next)                                                        \
  { cycles -= seq; M4A_COUNT();                                               \
    if (cycles <= 0) { pc = base + (next); goto out; } }
#define M4A_MEM(a, sz)                                                        \
  if ((a) < 0x10000000) cycles -= ws_cyc_nseq[(a) >> 24][sz]
#define M4A_ROR8(x) (((x) >> 8) | ((x) << 24))

/* Runs the loop from pc (m4a_pc_out or m4a_pc_in) until it exits or the
   cycles run out, pc becomes the next instruction. False (nothing done) for
   a PCM buffer outside work RAM or unaligned: the interpreter keeps it. */
XT_HOT static bool m4a_run(u32 &pc, s32 &cycles_io, u32 &n_io, u32 &z_io, u32 &c_io, u32 &v_io)
{
  /* locals: through the references every step was a load and a store */
  s32 cycles = cycles_io;
  u32 n_flag = n_io, z_flag = z_io, c_flag = c_io, v_flag = v_io;
  const u32 base = m4a_base, stereo = (m4a_kind == 1);
  const s32 seq = ws_cyc_seq[3][1], nseq_b = ws_cyc_nseq[3][1];
  u32 r0 = reg[0], r1 = reg[1], r2 = reg[2], r3 = reg[3], r4 = reg[4], r5 = reg[5];
  u32 r6 = reg[6], r7 = reg[7], r8 = reg[8], r9 = reg[9], r10 = reg[10], r11 = reg[11];
  u32 r12 = reg[12], r14 = reg[14], a, res;
  /* offsets: the stereo loop has 4 more instructions before "add lr" (d)
     and one more store after "blo" (d2) */
  const u32 o_in = stereo ? 0x08 : 0x04, d = stereo ? 0x10 : 0x00, d2 = stereo ? 0x14 : 0x00;

  if (((r5 >> 24) != 0x02 && (r5 >> 24) != 0x03) || (r5 & 3))
    return false;                            /* not ours: interpret */
  if (pc != base)
    goto inner;

outer:
  a = r5; M4A_MEM(a, 1); r6 = m4a_rd32(a);                      M4A_STEP(0x04);
  if (stereo) { a = r5 + 0x630; M4A_MEM(a, 1); r7 = m4a_rd32(a); M4A_STEP(0x08); }
inner:
  r9 = r14 * r1;                                                M4A_STEP(o_in + 0x04);
  r9 = r0 + ((s32)r9 >> 23);                                    M4A_STEP(o_in + 0x08);
  r12 = r10 * r9;                                               M4A_STEP(o_in + 0x0C);
  r12 &= ~0xFF0000;                                             M4A_STEP(o_in + 0x10);
  r6 = r12 + M4A_ROR8(r6);                                      M4A_STEP(o_in + 0x14);
  if (stereo) {
    r12 = r11 * r9;                                             M4A_STEP(0x20);
    r12 &= ~0xFF0000;                                           M4A_STEP(0x24);
    r7 = r12 + M4A_ROR8(r7);                                    M4A_STEP(0x28);
  }
  r14 += r4;                                                    M4A_STEP(d + 0x1C);
  /* lsrs sb, lr, #23 */
  c_flag = (r14 >> 22) & 1; r9 = r14 >> 23; n_flag = 0; z_flag = (r9 == 0);
                                                                M4A_STEP(d + 0x20);
  /* beq -> adds */
  if (z_flag) { cycles -= nseq_b;                               M4A_STEP(d + 0x44); goto adds; }
                                                                M4A_STEP(d + 0x24);
  r14 &= ~0x3F800000;                                           M4A_STEP(d + 0x28);
  /* subs r2, r2, sb */
  res = r2 - r9; n_flag = res >> 31; z_flag = (res == 0); c_flag = (r2 >= r9);
  v_flag = ((r2 ^ r9) & (r2 ^ res)) >> 31; r2 = res;           M4A_STEP(d + 0x2C);
  /* ble exit: back to the interpreter */
  if (z_flag || (n_flag != v_flag)) {
    cycles -= ws_cyc_nseq[m4a_exit >> 24][1];
    cycles -= ws_cyc_seq[(m4a_exit >> 24) & 0xF][1]; M4A_COUNT();
    pc = m4a_exit; goto out;
  }
                                                                M4A_STEP(d + 0x30);
  /* subs sb, sb, #1 */
  res = r9 - 1; n_flag = res >> 31; z_flag = (res == 0); c_flag = (r9 >= 1);
  v_flag = (r9 & (r9 ^ res)) >> 31; r9 = res;                   M4A_STEP(d + 0x34);
  /* addeq r0, r0, r1 */
  if (z_flag) { r0 += r1; }
                                                                M4A_STEP(d + 0x38);
  /* ldrsbne r0, [r3, sb]! */
  if (!z_flag) { a = r3 + r9; M4A_MEM(a, 0); r0 = m4a_rd8s(a); r3 = a; }
                                                                M4A_STEP(d + 0x3C);
  /* ldrsb r1, [r3, #1]! */
  a = r3 + 1; M4A_MEM(a, 0); r1 = m4a_rd8s(a); r3 = a;          M4A_STEP(d + 0x40);
  r1 -= r0;                                                     M4A_STEP(d + 0x44);
adds:
  /* adds r5, r5, #0x40000000 */
  res = r5 + 0x40000000; c_flag = (res < r5); v_flag = (~(r5 ^ 0x40000000) & (r5 ^ res)) >> 31;
  n_flag = res >> 31; z_flag = (res == 0); r5 = res;            M4A_STEP(d + 0x48);
  /* blo inner */
  if (!c_flag) { cycles -= nseq_b;                              M4A_STEP(o_in); goto inner; }
                                                                M4A_STEP(d + 0x4C);
  if (stereo) { a = r5 + 0x630; M4A_MEM(a, 1); m4a_wr32(a, r7);  M4A_STEP(0x60); }
  /* str r6, [r5], #4 */
  a = r5; M4A_MEM(a, 1); m4a_wr32(a, r6); r5 = a + 4;           M4A_STEP(d2 + 0x50);
  /* subs r8, r8, #4 */
  res = r8 - 4; n_flag = res >> 31; z_flag = (res == 0); c_flag = (r8 >= 4);
  v_flag = (r8 & (r8 ^ res)) >> 31; r8 = res;                   M4A_STEP(d2 + 0x54);
  /* bgt outer */
  if (!z_flag && (n_flag == v_flag)) { cycles -= nseq_b;        M4A_STEP(0x00); goto outer; }
                                                                M4A_STEP(d2 + 0x58);
  pc = base + d2 + 0x58;

out:
  reg[0] = r0; reg[1] = r1; reg[2] = r2; reg[3] = r3; reg[5] = r5;
  reg[6] = r6; reg[7] = r7; reg[8] = r8; reg[9] = r9; reg[12] = r12; reg[14] = r14;
  cycles_io = cycles;
  n_io = n_flag; z_io = z_flag; c_io = c_flag; v_io = v_flag;
  return true;
}
