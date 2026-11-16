/* Minimal libm subset for the freestanding SF2000/GB300 core (no -lm there). */
#ifdef SF2000
#include <stdint.h>

double sqrt(double x) {
    if (x <= 0.0) return 0.0;
    double r = x > 1.0 ? x : 1.0;
    for (int i = 0; i < 24; ++i) r = 0.5 * (r + x / r);
    return r;
}

double sin(double x) {
    const double pi = 3.14159265358979323846;
    const double twopi = 6.28318530717958647692;
    while (x > pi) x -= twopi;
    while (x < -pi) x += twopi;
    double x2 = x * x;
    return x * (1.0 - x2 / 6.0 + x2 * x2 / 120.0 - x2 * x2 * x2 / 5040.0
                + x2 * x2 * x2 * x2 / 362880.0 - x2 * x2 * x2 * x2 * x2 / 39916800.0);
}

double cos(double x) {
    return sin(x + 1.57079632679489661923);
}

double ldexp(double x, int e) {
    if (e > 0) { while (e--) x *= 2.0; }
    else if (e < 0) { while (e++) x *= 0.5; }
    return x;
}

double floor(double x) {
    long long i = (long long)x;
    if (x < 0.0 && (double)i != x) i--;
    return (double)i;
}

double fabs(double x) {
    return x < 0.0 ? -x : x;
}
#endif
