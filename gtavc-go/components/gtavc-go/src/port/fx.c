#include "fx.h"
#include "fx_math.h"
#include "fx_tables.h"
#include <rg_system.h>

DRAM_ATTR int Fx_SIN[1024];
int Fx_seed = 0x2F6E2B1;
DRAM_ATTR int Fx_ATAN[258];

void Fx_initAtan(void) {
    for (int i = 0; i < 258; i++) Fx_ATAN[i] = FX_ATAN_TABLE[i];
}

void Fx_init(void) {
    for (int i = 0; i < 1024; i++) Fx_SIN[i] = FX_SIN_TABLE[i];
    Fx_initAtan();
}

int Fx_rnd(void) {
    Fx_seed = (int)((uint32_t)Fx_seed * 1103515245u + 12345u);
    return ((uint32_t)Fx_seed >> 16) & 0x7FFF;
}

int Fx_rndBound(int n) {
    return n <= 0 ? 0 : Fx_rnd() % n;
}

int Fx_rndRange(int a, int b) {
    return a + Fx_rndBound(b - a + 1);
}

IRAM_ATTR int Fx_yawOf(int dx, int dz) {
    if (dx == 0 && dz == 0) return 0;
    int ax = dx < 0 ? -dx : dx;
    int az = dz < 0 ? -dz : dz;
    int ang;
    if (az >= ax) {
        ang = Fx_ATAN[(ax * 256) / az];
    } else {
        ang = 256 - Fx_ATAN[(az * 256) / ax];
    }
    if (dx >= 0) return dz >= 0 ? ang : 512 - ang;
    return dz < 0 ? 512 + ang : (1024 - ang) & 1023;
}

int Fx_rf(float f) {
    return (int)Fx_floorD((double)f + 0.5);
}

IRAM_ATTR int Fx_dist(int dx, int dz) {
    uint64_t d2 = (uint64_t)((int64_t)dx * (int64_t)dx) + (uint64_t)((int64_t)dz * (int64_t)dz);
    return (int)Fx_isqrtFast(d2);
}

IRAM_ATTR int Fx_angDiff(int a, int b) {
    int d = (b - a) & 1023;
    return d >= 512 ? d - 1024 : d;
}

IRAM_ATTR void Fx_ident(int* m) {
    m[0] = 16384; m[1] = 0;     m[2] = 0;
    m[3] = 0;     m[4] = 16384; m[5] = 0;
    m[6] = 0;     m[7] = 0;     m[8] = 16384;
}

IRAM_ATTR void Fx_rotX(int* m, int a) {
    int c = Fx_cos(a), s = Fx_sin(a);
    m[0] = 16384; m[1] = 0; m[2] = 0;
    m[3] = 0;     m[4] = c; m[5] = -s;
    m[6] = 0;     m[7] = s; m[8] = c;
}

IRAM_ATTR void Fx_rotY(int* m, int a) {
    int c = Fx_cos(a), s = Fx_sin(a);
    m[0] = c;  m[1] = 0;     m[2] = s;
    m[3] = 0;  m[4] = 16384; m[5] = 0;
    m[6] = -s; m[7] = 0;     m[8] = c;
}

IRAM_ATTR void Fx_rotZ(int* m, int a) {
    int c = Fx_cos(a), s = Fx_sin(a);
    m[0] = c; m[1] = -s; m[2] = 0;
    m[3] = s; m[4] = c;  m[5] = 0;
    m[6] = 0; m[7] = 0;  m[8] = 16384;
}

IRAM_ATTR void Fx_mul(const int* a, const int* b, int* o) {
    o[0] = (a[0] * b[0] + a[1] * b[3] + a[2] * b[6]) >> 14;
    o[1] = (a[0] * b[1] + a[1] * b[4] + a[2] * b[7]) >> 14;
    o[2] = (a[0] * b[2] + a[1] * b[5] + a[2] * b[8]) >> 14;
    o[3] = (a[3] * b[0] + a[4] * b[3] + a[5] * b[6]) >> 14;
    o[4] = (a[3] * b[1] + a[4] * b[4] + a[5] * b[7]) >> 14;
    o[5] = (a[3] * b[2] + a[4] * b[5] + a[5] * b[8]) >> 14;
    o[6] = (a[6] * b[0] + a[7] * b[3] + a[8] * b[6]) >> 14;
    o[7] = (a[6] * b[1] + a[7] * b[4] + a[8] * b[7]) >> 14;
    o[8] = (a[6] * b[2] + a[7] * b[5] + a[8] * b[8]) >> 14;
}
