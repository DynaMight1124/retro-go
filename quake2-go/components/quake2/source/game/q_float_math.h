#pragma once
// Enabled only for renderer translation units and shared vector math.
// Keep double precision in game logic, timing, cvars and save handling.
#if defined(Q2_FLOAT_MATH) && Q2_FLOAT_MATH
#define sin(x) sinf(x)
#define cos(x) cosf(x)
#define tan(x) tanf(x)
#define asin(x) asinf(x)
#define acos(x) acosf(x)
#define atan(x) atanf(x)
#define atan2(y,x) atan2f(y,x)
#define sqrt(x) sqrtf(x)
#define floor(x) floorf(x)
#define ceil(x) ceilf(x)
#define fabs(x) fabsf(x)
#define fmod(x,y) fmodf(x,y)
#define pow(x,y) powf(x,y)
#define exp(x) expf(x)
#define log(x) logf(x)
#endif
