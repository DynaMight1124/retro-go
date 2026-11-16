#ifndef GTAVC_FX_MATH_H
#define GTAVC_FX_MATH_H

#include <stdint.h>
#include <rg_system.h>

/*
 * Deterministic math helpers.
 *
 * The target (SF2000/GB300) has no FPU and its libm differs from the host's,
 * so the few places that need trigonometry/square roots use these software
 * implementations.  They are compiled identically for host and MIPS, which
 * makes the port bit-for-bit reproducible on both.
 */

double Fx_sinD(double x);
double Fx_sqrtD(double x);
double Fx_floorD(double x);

/* exact floor(sqrt(v)) for a non-negative 64-bit integer */
uint32_t Fx_isqrt(uint64_t v);

/* fast floor(sqrt(v)); exact below 2^32, at most a few mm error higher up */
uint32_t Fx_isqrtFast(uint64_t v);

#endif /* GTAVC_FX_MATH_H */
