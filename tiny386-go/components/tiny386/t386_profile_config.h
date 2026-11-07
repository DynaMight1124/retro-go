#pragma once
/* Detailed instrumentation is retained for focused host regression tests only.
 * Normal debug and release firmware both omit it. Release always forces it off. */
#ifndef T386_ENABLE_PROFILING
#define T386_ENABLE_PROFILING 0
#endif
#if defined(RG_BUILD_RELEASE) && RG_BUILD_RELEASE
#undef T386_ENABLE_PROFILING
#define T386_ENABLE_PROFILING 0
#endif
