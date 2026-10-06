/* Coarse whole-frame CPU timings for the Retro-Go diagnostic build. */
#ifndef MK64_RG_PHASE_PROFILE_H
#define MK64_RG_PHASE_PROFILE_H

enum { RG_PHASE_SIM, RG_PHASE_KART, RG_PHASE_OBJECTS, RG_PHASE_LIST,
       RG_PHASE_OTHER, RG_PHASE_DRAW, RG_PHASE_COUNT };
#if MK64_RG_PHASE_PROFILE
void port_rg_profile_begin(int racing, int course);
void port_rg_profile_mark(int phase);
void port_rg_profile_finish(void);
#else
#define port_rg_profile_begin(racing, course) ((void)0)
#define port_rg_profile_mark(phase) ((void)0)
#define port_rg_profile_finish() ((void)0)
#endif
#endif
