#ifndef GTAVC_FX_H
#define GTAVC_FX_H

#include <stdint.h>
#include <stdbool.h>
#include <rg_system.h>

#define FX_PI 3.14159265358979323846

extern int Fx_SIN[1024];
extern int Fx_seed;
extern int Fx_ATAN[258];

void Fx_init(void);
void Fx_initAtan(void);

static inline IRAM_ATTR int Fx_sin(int a) {
    return Fx_SIN[a & 1023];
}

static inline IRAM_ATTR int Fx_cos(int a) {
    return Fx_SIN[(a + 256) & 1023];
}

int Fx_rnd(void);
int Fx_rndBound(int n);
int Fx_rndRange(int a, int b);

int Fx_yawOf(int dx, int dz);
int Fx_rf(float f);
int Fx_dist(int dx, int dz);
int Fx_angDiff(int a, int b);

static inline IRAM_ATTR int Fx_clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline IRAM_ATTR int Fx_abs(int v) {
    return v < 0 ? -v : v;
}

/* 3x3 matrices in 2.14 (16384 = 1.0), row-major */
void Fx_ident(int* m);
void Fx_rotX(int* m, int a);
void Fx_rotY(int* m, int a);
void Fx_rotZ(int* m, int a);
void Fx_mul(const int* a, const int* b, int* o);

#endif /* GTAVC_FX_H */
