#ifndef _GNU_SOURCE
#define _GNU_SOURCE // *_np
#endif
#ifdef _3DS
#include <3ds/svc.h>
#include <3ds/os.h>
#include <3ds/services/apt.h>
#include <sys/time.h>
#include "../libpcsxcore/new_dynarec/new_dynarec.h"
#endif

#ifdef ESP_PLATFORM
#include <stdlib.h>
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_pthread.h"
#include "freertos/FreeRTOS.h"
#else
#include "../deps/libretro-common/rthreads/rthreads.c"
#include "features/features_cpu.h"
#endif
#include "pcsxr-threads.h"

// pcsxr "extensions"
extern void SysPrintf(const char *fmt, ...);

#ifdef _3DS
static bool is_new_3ds;
#endif

#ifdef ESP_PLATFORM
struct esp_sthread_start {
	void (*func)(void *);
	void *userdata;
};

static void *esp_sthread_entry(void *arg)
{
	struct esp_sthread_start start = *(struct esp_sthread_start *)arg;
	free(arg);
	start.func(start.userdata);
	return NULL;
}

sthread_t *sthread_create(void (*thread_func)(void *), void *userdata)
{
	struct esp_sthread_start *start = malloc(sizeof(*start));
	sthread_t *thread = malloc(sizeof(*thread));
	if (!start || !thread) {
		free(start);
		free(thread);
		return NULL;
	}
	start->func = thread_func;
	start->userdata = userdata;
	if (pthread_create(&thread->id, NULL, esp_sthread_entry, start) != 0) {
		free(start);
		free(thread);
		return NULL;
	}
	return thread;
}

void sthread_join(sthread_t *thread)
{
	if (thread) {
		pthread_join(thread->id, NULL);
		free(thread);
	}
}

slock_t *slock_new(void)
{
	slock_t *lock = malloc(sizeof(*lock));
	if (lock && pthread_mutex_init(lock, NULL) != 0) {
		free(lock);
		lock = NULL;
	}
	return lock;
}

void slock_free(slock_t *lock)
{
	if (lock) {
		pthread_mutex_destroy(lock);
		free(lock);
	}
}

void slock_lock(slock_t *lock) { pthread_mutex_lock(lock); }
void slock_unlock(slock_t *lock) { pthread_mutex_unlock(lock); }

scond_t *scond_new(void)
{
	scond_t *cond = malloc(sizeof(*cond));
	if (cond && pthread_cond_init(cond, NULL) != 0) {
		free(cond);
		cond = NULL;
	}
	return cond;
}

void scond_free(scond_t *cond)
{
	if (cond) {
		pthread_cond_destroy(cond);
		free(cond);
	}
}

void scond_wait(scond_t *cond, slock_t *lock) { pthread_cond_wait(cond, lock); }
void scond_signal(scond_t *cond) { pthread_cond_signal(cond); }
#endif

void pcsxr_sthread_init(void)
{
	#ifdef ESP_PLATFORM
	SysPrintf("%d cpu core(s) detected\n", CONFIG_FREERTOS_NUMBER_OF_CORES);
	#else
	SysPrintf("%d cpu core(s) detected\n", cpu_features_get_core_amount());
	#endif
#ifdef _3DS
	int64_t version = 0;
	int fpscr = -1;

	APT_CheckNew3DS(&is_new_3ds);
	svcGetSystemInfo(&version, 0x10000, 0);

	APT_SetAppCpuTimeLimit(35);
	u32 percent = -1;
	APT_GetAppCpuTimeLimit(&percent);

	__asm__ volatile("fmrx %0, fpscr" : "=r"(fpscr));
	SysPrintf("%s3ds detected, v%d.%d, AppCpuTimeLimit=%ld fpscr=%08x\n",
		is_new_3ds ? "new" : "old", (int)GET_VERSION_MAJOR(version),
		(int)GET_VERSION_MINOR(version), percent, fpscr);
#endif
}

sthread_t *pcsxr_sthread_create(void (*thread_func)(void *),
	enum pcsxr_thread_type type)
{
	sthread_t *h = NULL;
#ifdef _3DS
	size_t stack_size = 64*1024;
	Thread ctr_thread;
	int core_id = 0;
	s32 prio = 0x30;

	h = calloc(1, sizeof(*h));
	if (!h)
		return NULL;

	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);

	switch (type) {
	case PCSXRT_CDR:
	case PCSXRT_SPU:
		core_id = 1;
		break;
	case PCSXRT_DRC:
		stack_size = new_dynarec_estimate_stack_size();
		// fallthrough
	case PCSXRT_GPU:
		core_id = is_new_3ds ? 2 : 1;
		break;
	case PCSXRT_COUNT:
		break;
	}

	ctr_thread = threadCreate(thread_func, NULL, stack_size, prio, core_id, false);
	if (!ctr_thread) {
		if (core_id == 1) {
			SysPrintf("threadCreate pcsxt %d core %d failed\n",
				type, core_id);
			core_id = is_new_3ds ? 2 : -1;
			ctr_thread = threadCreate(thread_func, NULL, stack_size,
				prio, core_id, false);
		}
	}
	SysPrintf("threadCreate: pcsxt %d core %d stack %zd: %p\n",
		type, core_id, stack_size, ctr_thread);
	if (!ctr_thread) {
		free(h);
		return NULL;
	}
	h->id = (pthread_t)ctr_thread;
#else
	#ifdef ESP_PLATFORM
	esp_pthread_cfg_t previous_cfg;
	esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
	int have_previous_cfg = esp_pthread_get_cfg(&previous_cfg) == ESP_OK;

	if (have_previous_cfg)
		cfg = previous_cfg;
	cfg.stack_size = type == PCSXRT_GPU ? 12 * 1024 : 8 * 1024;
	cfg.inherit_cfg = false;
	cfg.thread_name = type == PCSXRT_GPU ? "pcsxr-gpu" : "pcsxr-worker";
	cfg.pin_to_core = type == PCSXRT_GPU ? 0 : -1;
	cfg.stack_alloc_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
	if (esp_pthread_set_cfg(&cfg) != ESP_OK)
		return NULL;

	h = sthread_create(thread_func, NULL);
	if (have_previous_cfg)
		esp_pthread_set_cfg(&previous_cfg);
	else {
		cfg = esp_pthread_get_default_config();
		esp_pthread_set_cfg(&cfg);
	}
	SysPrintf("threadCreate: pcsxt %d core %d stack %u: %p\n",
		type, type == PCSXRT_GPU ? 0 : -1,
		type == PCSXRT_GPU ? 12 * 1024u : 8 * 1024u, h);
	#else
	h = sthread_create(thread_func, NULL);
 #if defined(__GLIBC__) || \
    (defined(__ANDROID_API__) && __ANDROID_API__ >= 26)
	if (h && (unsigned int)type < (unsigned int)PCSXRT_COUNT)
	{
		const char * const pcsxr_tnames[PCSXRT_COUNT] = {
			"pcsxr-cdrom", "pcsxr-drc", "pcsxr-gpu", "pcsxr-spu"
		};
		pthread_setname_np(h->id, pcsxr_tnames[type]);
	}
 #endif
	#endif
#endif
	return h;
}
