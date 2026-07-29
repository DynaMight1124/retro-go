#include <string.h>
#include <esp_cpu.h>

#include "pprof.h"

struct pp_counters pprof_counters;
unsigned int pprof_refcounts[pp_total_points];

uint32_t pprof_get_one(void)
{
  return (uint32_t)esp_cpu_get_cycle_count();
}

void pprof_reset(void)
{
  memset(&pprof_counters, 0, sizeof(pprof_counters));
  memset(pprof_refcounts, 0, sizeof(pprof_refcounts));
}

void pprof_init(void)
{
  pprof_reset();
}

void pprof_finish(void)
{
}
