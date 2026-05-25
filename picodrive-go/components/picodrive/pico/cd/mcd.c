#include <rg_system.h>
#include "../pico_int.h"
#include "../memory.h"

mcd_state *Pico_mcd;
u32 pcd_base_address;
unsigned int pcd_event_times[PCD_EVENT_COUNT];

static unsigned int mcd_m68k_cycle_mult;
static unsigned int mcd_s68k_cycle_mult;

extern unsigned char formatted_bram[4*0x10];

PICO_INTERNAL void PicoCreateMCD(unsigned char *bios_data, int bios_size)
{
  if (Pico_mcd == NULL) {
    Pico_mcd = rg_alloc(sizeof(mcd_state), MEM_SLOW);
    if (Pico_mcd == NULL) return;
    memset(Pico_mcd, 0, sizeof(mcd_state));
  }

  if (bios_data && bios_size > 0) {
    if (bios_size > sizeof(Pico_mcd->bios)) bios_size = sizeof(Pico_mcd->bios);
    memcpy(Pico_mcd->bios, bios_data, bios_size);

    u8 *b = Pico_mcd->bios + 0x100;
    // Check for standard Big-Endian 'SEGA' header
    if (b[0] == 0x53 && b[1] == 0x45 && b[2] == 0x47 && b[3] == 0x41) {
      int i;
      u16 *d = (u16 *)Pico_mcd->bios;
      for (i = 0; i < bios_size/2; i++) d[i] = (d[i] << 8) | (d[i] >> 8);
      RG_LOGI("BIOS Header detected as Big-Endian. Swapped for Little-Endian host.\n");
    } else {
      RG_LOGI("BIOS Header is already swapped or unknown: %02x%02x%02x%02x\n",
              b[0], b[1], b[2], b[3]);
    }
  }
}

static unsigned int event_time_next;

PICO_INTERNAL void PicoInitMCD(void) { SekInitS68k(); }

PICO_INTERNAL void PicoExitMCD(void) {
  cdd_unload();
  if (Pico_mcd) { free(Pico_mcd); Pico_mcd = NULL; }
}

PICO_INTERNAL void PicoPowerMCD(void) {
  int fmt_size = sizeof(formatted_bram);

  memset(Pico_mcd->prg_ram,    0, sizeof(Pico_mcd->prg_ram));
  memset(Pico_mcd->word_ram2M, 0, sizeof(Pico_mcd->word_ram2M));
  memset(Pico_mcd->pcm_ram,    0, sizeof(Pico_mcd->pcm_ram));
  memset(Pico_mcd->bram, 0, sizeof(Pico_mcd->bram));
  memcpy(Pico_mcd->bram + sizeof(Pico_mcd->bram) - fmt_size, formatted_bram, fmt_size);
  memset(Pico_mcd->s68k_regs, 0, sizeof(Pico_mcd->s68k_regs));
  memset(&Pico_mcd->pcm, 0, sizeof(Pico_mcd->pcm));
  memset(&Pico_mcd->m, 0, sizeof(Pico_mcd->m));

  cdc_init();
  gfx_init();

  Pico_mcd->m.state_flags = PCD_ST_S68K_RST;
  Pico_mcd->m.busreq = 2;
  Pico_mcd->s68k_regs[3] = 1;
  
  if (Pico.romsize == 0) 
    memset(Pico_mcd->bios + 0x70, 0xff, 4);

  cdc_reset();
  cdd_reset();

  memset(pcd_event_times, 0, sizeof(pcd_event_times));
  event_time_next = 0;
  pcd_event_schedule(0, PCD_EVENT_CDC, 12500000/75);
}

PICO_INTERNAL int PicoResetMCD(void) {
  PicoMemSetupCD();
  PicoPowerMCD();

  SekResetS68k();

  SekCycleCntS68k = SekCycleAimS68k = 0;
  RG_LOGI("Sub-CPU Reset Complete.\n");
  return 0;
}
void pcd_irq_s68k(int irq, int state) {
  if (state) Pico_mcd->m.s68k_pend_ints |= (1 << irq);
  else       Pico_mcd->m.s68k_pend_ints &= ~(1 << irq);
  if (Pico_mcd->m.s68k_pend_ints & Pico_mcd->s68k_regs[0x33]) {
    int i = 6;
    for (; i > 0; i--) if ((Pico_mcd->m.s68k_pend_ints & Pico_mcd->s68k_regs[0x33]) & (1 << i)) break;
    SekInterruptS68k(i);
  } else {
    SekInterruptS68k(0);
  }
}

void pcd_soft_reset(void) {
  elprintf(EL_CD, "cd: soft reset");

  Pico_mcd->m.s68k_pend_ints = 0;
  cdc_reset();
  cdd_reset();

  memset(&Pico_mcd->s68k_regs[0x38], 0, 9);
  Pico_mcd->s68k_regs[0x38+9] = 0x0f;  // default checksum

  pcd_event_schedule_s68k(PCD_EVENT_CDC, 12500000/75);
}

void PicoMCDPrepare(void) {
  unsigned int osc = (Pico.m.pal ? OSC_PAL : OSC_NTSC);
  mcd_m68k_cycle_mult = ((7 * 12500000ull << 16) + osc/2) / osc;
  mcd_s68k_cycle_mult = ((1ull * osc << 16) + (7 * 12500000)/2) / (7 * 12500000);
}

unsigned int mcd_m68k_cycle_base;
unsigned int mcd_s68k_cycle_base;

unsigned int pcd_cycles_m68k_to_s68k(unsigned int c) {
  if (c < 40000) // fast path: 32-bit multiply fits without overflow
    return (c * mcd_m68k_cycle_mult) >> 16;
  return (unsigned long long)c * mcd_m68k_cycle_mult >> 16;
}

extern void cdd_update(void);
extern void cdc_dma_update(void);
extern void msd_update(void);
extern void gfx_update(unsigned int now);

extern void pcd_pcm_update(s32 *buffer, int length, int stereo);

static void pcd_cdc_event(unsigned int now)
{
  // 75Hz CDC update
  cdd_update();

  /* check if a new CDD command has been processed */
  if (!(Pico_mcd->s68k_regs[0x4b] & 0xf0))
  {
    /* reset CDD command wait flag */
    Pico_mcd->s68k_regs[0x4b] = 0xf0;
  }

  if ((Pico_mcd->s68k_regs[0x33] & PCDS_IEN4) && (Pico_mcd->s68k_regs[0x37] & 4)) {
    pcd_irq_s68k(4, 1);
  }

  msd_update();
  
  // Tick the PCM chip status
  pcd_pcm_update(NULL, 0, 0);

  pcd_event_schedule(now, PCD_EVENT_CDC, 12500000/75);
}

static void pcd_int3_timer_event(unsigned int now)
{
  if (Pico_mcd->s68k_regs[0x33] & PCDS_IEN3) {
    pcd_irq_s68k(3, 1);
  }

  if (Pico_mcd->s68k_regs[0x31] != 0)
    pcd_event_schedule(now, PCD_EVENT_TIMER3,
      (Pico_mcd->s68k_regs[0x31]+1) * 384);
}

static void pcd_dma_event(unsigned int now)
{
  cdc_dma_update();
}

static void (*pcd_event_cbs[PCD_EVENT_COUNT])(unsigned int now) = {
  pcd_cdc_event,            // PCD_EVENT_CDC
  pcd_int3_timer_event,     // PCD_EVENT_TIMER3
  gfx_update,               // PCD_EVENT_GFX
  pcd_dma_event,            // PCD_EVENT_DMA
};

static void pcd_run_events(unsigned int until)
{
  int oldest, oldest_diff, time;
  int i, diff;

  while (1) {
    oldest = -1, oldest_diff = 0x7fffffff;

    for (i = 0; i < PCD_EVENT_COUNT; i++) {
      if (pcd_event_times[i]) {
        diff = pcd_event_times[i] - until;
        if (diff < oldest_diff) {
          oldest_diff = diff;
          oldest = i;
        }
      }
    }

    if (oldest_diff <= 0) {
      time = pcd_event_times[oldest];
      pcd_event_times[oldest] = 0;
      pcd_event_cbs[oldest](time);
    }
    else if (oldest_diff < 0x7fffffff) {
      event_time_next = pcd_event_times[oldest];
      break;
    }
    else {
      event_time_next = 0;
      break;
    }
  }
}

int pcd_sync_s68k(unsigned int m68k_target, int m68k_poll_sync) {
  #define now SekCycleCntS68k
  unsigned int s68k_target = pcd_cycles_m68k_to_s68k(m68k_target - mcd_m68k_cycle_base) + mcd_s68k_cycle_base;

  if (Pico_mcd->m.busreq != 1) { /* busreq/reset */
    SekCycleCntS68k = SekCycleAimS68k = s68k_target;
    pcd_run_events(s68k_target);
    return 0;
  }

  while (CYCLES_GT(s68k_target, now)) {
    if (event_time_next && CYCLES_GE(now, event_time_next))
      pcd_run_events(now);

    unsigned int target = s68k_target;
    if (event_time_next && CYCLES_GT(target, event_time_next))
      target = event_time_next;

    if (Pico_mcd->m.state_flags & (PCD_ST_S68K_POLL|PCD_ST_S68K_SLEEP)) {
      SekCycleCntS68k = SekCycleAimS68k = target;
    } else {
      SekRunS68k(target);
    }

    if (m68k_poll_sync && Pico_mcd->m.m68k_poll_cnt == 0)
      break;
  }
  return s68k_target - now;
  #undef now
}

void pcd_event_schedule(unsigned int now, enum pcd_event event, int after) {
  unsigned int t = now + after;
  if (t == 0) t = 1;
  pcd_event_times[event] = t;
  if (event_time_next == 0 || CYCLES_GT(event_time_next, t))
    event_time_next = t;
}

void pcd_event_schedule_s68k(enum pcd_event event, int after) { pcd_event_schedule(SekCycleAimS68k, event, after); }

#define PICO_CD

// Define CPUS_RUN so that pico_cmn.c knows what to use, but we need pcd_run_cpus
// Since pcd_run_cpus uses functions from pico_cmn.c, we must include it first.
// Wait, pico_cmn.c uses CPUS_RUN! So we need to forward declare pcd_run_cpus.

void pcd_run_cpus(int m68k_cycles);

#define CPUS_RUN(m68k_cycles) pcd_run_cpus(m68k_cycles)
#include "../pico_cmn.c"

void pcd_run_cpus(int m68k_cycles) {
  SekAimM68k(m68k_cycles, 0x108);

  while (CYCLES_GT(Pico.t.m68c_aim, Pico.t.m68c_cnt)) {
    if (SekShouldInterrupt()) {
      Pico_mcd->m.state_flags &= ~PCD_ST_M68K_POLL;
      Pico_mcd->m.m68k_poll_cnt = 0;
    }

    if (Pico_mcd->m.state_flags & PCD_ST_M68K_POLL) {
      int s68k_left;
      // main CPU is polling, (wake and) run sub only
      if (Pico_mcd->m.state_flags & (PCD_ST_S68K_POLL|PCD_ST_S68K_SLEEP)) {
        Pico_mcd->m.state_flags &= ~(PCD_ST_S68K_POLL|PCD_ST_S68K_SLEEP);
        Pico_mcd->m.s68k_poll_cnt = 0;
      }
      s68k_left = pcd_sync_s68k(Pico.t.m68c_aim, 1);

      Pico.t.m68c_cnt = Pico.t.m68c_aim;
      if (s68k_left > 0)
        Pico.t.m68c_cnt -= ((long long)s68k_left * mcd_s68k_cycle_mult >> 16);
      if (Pico_mcd->m.state_flags & (PCD_ST_S68K_POLL|PCD_ST_S68K_SLEEP)) {
        // slave has stopped, wake master to avoid lockups
        Pico_mcd->m.state_flags &= ~PCD_ST_M68K_POLL;
        Pico_mcd->m.m68k_poll_cnt = 0;
      }

      elprintf(EL_CDPOLL, "m68k poll [%02x] x%d @%06x",
        Pico_mcd->m.m68k_poll_a, Pico_mcd->m.m68k_poll_cnt, SekPc);
    } else {
      SekSyncM68k(1);
      // make sure sub doesn't get too far out of sync with main
      if (!(Pico_mcd->m.state_flags & (PCD_ST_S68K_POLL|PCD_ST_S68K_SLEEP)) &&
          pcd_cycles_m68k_to_s68k(Pico.t.m68c_aim - mcd_m68k_cycle_base) >
                           10000 + SekCycleAimS68k - mcd_s68k_cycle_base)
        pcd_sync_s68k(Pico.t.m68c_cnt, 0);
    }
    if (Pico_mcd->m.state_flags & PCD_ST_S68K_SYNC) {
      Pico_mcd->m.state_flags &= ~PCD_ST_S68K_SYNC;
      pcd_sync_s68k(Pico.t.m68c_cnt, 0);
    }
  }
}

void pcd_prepare_frame(void)
{
  mcd_m68k_cycle_base = Pico.t.m68c_cnt;
  mcd_s68k_cycle_base = SekCycleCntS68k;
}

PICO_INTERNAL void PicoFrameMCD(void) {
  PicoFrameStart();
  PicoMCDPrepare();
  pcd_prepare_frame();
  PicoFrameHints();
  PsndGetSamples(0);
}void pcd_state_loaded(void) { PicoMemSetupCD(); }
