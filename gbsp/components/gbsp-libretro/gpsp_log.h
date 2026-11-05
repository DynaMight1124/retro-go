#pragma once
#ifdef RG_LOG_TAG
#undef RG_LOG_TAG
#endif
#define RG_LOG_TAG "gbsp"
#include <rg_system.h>

#ifndef RG_BUILD_RELEASE
#define RG_BUILD_RELEASE 0
#endif

/* Instruction/register dumps are deliberately opt-in even in debug builds. */
#ifndef GPSP_DYNAREC_TRACE
#define GPSP_DYNAREC_TRACE 0
#endif
#define GPSP_TRACE_ENABLED (GPSP_DYNAREC_TRACE && !RG_BUILD_RELEASE)
#if GPSP_TRACE_ENABLED
#define GPSP_TRACE(...) RG_LOGD(__VA_ARGS__)
#else
#define GPSP_TRACE(...) ((void)0)
#endif
