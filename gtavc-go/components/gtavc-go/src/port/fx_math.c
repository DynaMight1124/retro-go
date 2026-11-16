#include "fx_math.h"

/* Software sine (Taylor, range reduced). Same code on every platform. */
double Fx_sinD(double x) {
    const double pi = 3.14159265358979323846;
    const double twopi = 6.28318530717958647692;
    while (x > pi) x -= twopi;
    while (x < -pi) x += twopi;
    double x2 = x * x;
    return x * (1.0 - x2 / 6.0 + x2 * x2 / 120.0 - x2 * x2 * x2 / 5040.0
                + x2 * x2 * x2 * x2 / 362880.0
                - x2 * x2 * x2 * x2 * x2 / 39916800.0
                + x2 * x2 * x2 * x2 * x2 * x2 / 6227020800.0);
}

double Fx_sqrtD(double x) {
    if (x <= 0.0) return 0.0;
    double r = x > 1.0 ? x : 1.0;
    for (int i = 0; i < 28; ++i) r = 0.5 * (r + x / r);
    return r;
}

double Fx_floorD(double x) {
    long long i = (long long)x;
    if (x < 0.0 && (double)i != x) i--;
    return (double)i;
}

IRAM_ATTR uint32_t Fx_isqrt(uint64_t v) {
    uint64_t r = 0;
    uint64_t bit = (uint64_t)1 << 62;
    while (bit > v) bit >>= 2;
    while (bit != 0) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)r;
}

static IRAM_ATTR uint32_t isqrt32(uint32_t n) {
    uint32_t r = 0;
    uint32_t bit = (uint32_t)1 << 30;
    while (bit > n) bit >>= 2;
    while (bit != 0) {
        if (n >= r + bit) {
            n -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

IRAM_ATTR uint32_t Fx_isqrtFast(uint64_t v) {
    int shift = 0;
    while (v >= ((uint64_t)1 << 32)) {
        v >>= 2;
        shift++;
    }
    return isqrt32((uint32_t)v) << shift;
}
