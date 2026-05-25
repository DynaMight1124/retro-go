#include "pico_int.h"
#include "memory.h"

#ifdef EMU_F68K
M68K_CONTEXT PicoCpuFM68k;
M68K_CONTEXT PicoCpuFS68k;

static void update_fame_irq_master(M68K_CONTEXT *ctx)
{
  int i;
  ctx->interrupts[0] = 0;
  for (i = 7; i > 0; i--) {
    if (ctx->interrupts[i]) {
      ctx->interrupts[0] = i;
      break;
    }
  }
}
#endif

static int do_ack(int level)
{
  struct PicoVideo *pv = &Pico.video;
  if (pv->pending_ints & pv->reg[1] & 0x20) {
    pv->pending_ints &= ~0x20;
    pv->status &= ~SR_F;
    if (pv->reg[0] & pv->pending_ints & 0x10)
      return pv->hint_irq;
  }
  else if (pv->pending_ints & pv->reg[0] & 0x10)
    pv->pending_ints &= ~0x10;

  return (PicoIn.AHW & PAHW_PICO ? PicoPicoIrqAck(level) : 0);
}

#ifdef EMU_F68K
static void SekIntAckF68K(unsigned level) {
  if (level > 0 && level < 8) {
    PicoCpuFM68k.interrupts[level] = 0;
    int next = do_ack(level);
    if (next > 0 && next < 8) PicoCpuFM68k.interrupts[next] = 1;
    update_fame_irq_master(&PicoCpuFM68k);
  }
}
static void SekIntAckFS68K(unsigned level) {
  Pico_mcd->m.s68k_pend_ints &= ~(1 << level);
  if (level == 2) {
    Pico_mcd->m.state_flags &= ~PCD_ST_S68K_IFL2;
  }
  int irqs = Pico_mcd->m.s68k_pend_ints & Pico_mcd->s68k_regs[0x33];
  int next_level = 0;
  for (int i = 7; i > 0; i--) {
    if (irqs & (1 << i)) {
      next_level = i;
      break;
    }
  }
  PicoCpuFS68k.interrupts[0] = next_level;
}
#endif

PICO_INTERNAL void SekInit(void)
{
#ifdef EMU_F68K
  memset(&PicoCpuFM68k, 0, sizeof(PicoCpuFM68k));
  fm68k_init();
  PicoCpuFM68k.iack_handler = SekIntAckF68K;
  PicoCpuFM68k.sr = 0x2700; 
#endif
}

PICO_INTERNAL void SekInitS68k(void)
{
#ifdef EMU_F68K
  memset(&PicoCpuFS68k, 0, sizeof(PicoCpuFS68k));
  PicoCpuFS68k.iack_handler = SekIntAckFS68K;
  PicoCpuFS68k.sr = 0x2700; 

  PicoCpuFS68k.read_byte  = (void *)s68k_read8;
  PicoCpuFS68k.read_word  = (void *)s68k_read16;
  PicoCpuFS68k.read_long  = (void *)s68k_read32;
  PicoCpuFS68k.write_byte = (void *)s68k_write8;
  PicoCpuFS68k.write_word = (void *)s68k_write16;
  PicoCpuFS68k.write_long = (void *)s68k_write32;
#endif
}

PICO_INTERNAL int SekReset(void)
{
#ifdef EMU_F68K
  fm68k_reset(&PicoCpuFM68k);
  PicoCpuFM68k.sr = 0x2700;
#endif
  return 0;
}

PICO_INTERNAL int SekResetS68k(void)
{
#ifdef EMU_F68K
  fm68k_reset(&PicoCpuFS68k);
  PicoCpuFS68k.sr = 0x2700; 
#endif
  return 0;
}

PICO_INTERNAL void SekRunS68k(unsigned int to)
{
#ifdef EMU_F68K
  int cyc_do;
  SekCycleAimS68k = to;
  if ((cyc_do = SekCycleAimS68k - SekCycleCntS68k) <= 0)
    return;

  if (Pico_mcd->m.state_flags & (PCD_ST_S68K_SLEEP | PCD_ST_S68K_RST)) {
      SekCycleCntS68k += cyc_do;
      return;
  }

  SekCycleCntS68k += cyc_do;
  fm68k_emulate(&PicoCpuFS68k, cyc_do, fm68k_reason_emulate);
  SekCycleCntS68k -= PicoCpuFS68k.io_cycle_counter;
  PicoCpuFS68k.io_cycle_counter = 0;
#endif
}

void SekStepM68k(void)
{
#if defined(EMU_F68K)
  Pico.t.m68c_cnt++;
  fm68k_emulate(&PicoCpuFM68k, 1, fm68k_reason_emulate);
  Pico.t.m68c_cnt -= PicoCpuFM68k.io_cycle_counter;
  PicoCpuFM68k.io_cycle_counter = 0;
#endif
}

#ifdef EMU_F68K
PICO_INTERNAL void SekRun(int cycles)
{
  Pico.t.m68c_cnt += cycles;
  fm68k_emulate(&PicoCpuFM68k, cycles, fm68k_reason_emulate);
  Pico.t.m68c_cnt -= PicoCpuFM68k.io_cycle_counter;
  PicoCpuFM68k.io_cycle_counter = 0;
}

PICO_INTERNAL int SekInterruptM68k(int irq)
{
#ifdef EMU_F68K
  if (irq > 0 && irq <= 7) {
    PicoCpuFM68k.interrupts[irq] = 1;
    update_fame_irq_master(&PicoCpuFM68k);
  }
#endif
  return 0;
}

PICO_INTERNAL int SekInterruptS68k(int irq)
{
#ifdef EMU_F68K
  PicoCpuFS68k.interrupts[0] = irq;
#endif
  return 0;
}

void SekInterruptClearS68k(int irq)
{
#ifdef EMU_F68K
  Pico_mcd->m.s68k_pend_ints &= ~(1 << irq);
  if (irq == 2) {
    Pico_mcd->m.state_flags &= ~PCD_ST_S68K_IFL2;
  }
  int irqs = Pico_mcd->m.s68k_pend_ints & Pico_mcd->s68k_regs[0x33];
  int next_level = 0;
  for (int i = 7; i > 0; i--) {
    if (irqs & (1 << i)) {
      next_level = i;
      break;
    }
  }
  PicoCpuFS68k.interrupts[0] = next_level;
#endif
}
#endif

PICO_INTERNAL void SekSetRealTAS(int use_real) {}

PICO_INTERNAL void SekPackCpu(unsigned char *cpu, int is_sub)
{
#if defined(EMU_F68K)
  M68K_CONTEXT *context = is_sub ? &PicoCpuFS68k : &PicoCpuFM68k;
  memcpy(cpu,context->dreg,0x40);
  *(u32  *)(cpu+0x40)=context->pc;
  *(u32  *)(cpu+0x44)=context->sr;
  *(u32  *)(cpu+0x48)=context->asp;
  cpu[0x4c] = context->interrupts[0];
  cpu[0x4d] = (context->execinfo & FM68K_HALTED) ? 1 : 0;
#endif
}

PICO_INTERNAL void SekUnpackCpu(const unsigned char *cpu, int is_sub)
{
#if defined(EMU_F68K)
  M68K_CONTEXT *context = is_sub ? &PicoCpuFS68k : &PicoCpuFM68k;
  memcpy(context->dreg,cpu,0x40);
  context->pc =*(u32 *)(cpu+0x40);
  context->sr =*(u32 *)(cpu+0x44);
  context->asp=*(u32 *)(cpu+0x48);
  context->interrupts[0] = cpu[0x4c];
  context->execinfo &= ~FM68K_HALTED;
  if (cpu[0x4d]&1) context->execinfo |= FM68K_HALTED;
#endif
}

void SekInitIdleDet(void) {}
int SekIsIdleReady(void) { return 0; }
int SekIsIdleCode(unsigned short *dst, int bytes) { return 0; }
int SekRegisterIdlePatch(unsigned int pc, int oldop, int newop, void *ctx) { return 0; }
void SekFinishIdleDet(void) {}
