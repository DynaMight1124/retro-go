/* See mix_jobs.h. */
#define MIXER_INTERNAL
#include <string.h>
#include <stdio.h>
#include "mixer.h"
#include "mix_jobs.h"
#include "port.h"

extern unsigned int port_time_us(void);

_Static_assert(sizeof(MixJob) % 64 == 0 && sizeof(MixShared) == 64, "whole cache lines");

#ifdef RETRO_GO
#include "port/rg/audio_rg.h"
#include <rg_system.h>
__attribute__((section(".ext_ram.bss")))
#endif
MixJob gMixJobs[MIXJ_JOBS] __attribute__((aligned(64)));
MixShared gMixShared __attribute__((aligned(64)));

enum {
    OP_CLEAR, OP_LOADADPCM, OP_SETBUFFER, OP_DMEMMOVE, OP_SETLOOP, OP_ADPCMDEC, OP_RESAMPLE, OP_LOADBUFFER,
    OP_SAVEBUFFER, OP_INTERLEAVEOLD, OP_MIX, OP_ENVSETUP1, OP_ENVSETUP2, OP_ENVMIXER, OP_S8DEC, OP_ADDMIXER,
    OP_DUPLICATE, OP_DMEMMOVE2, OP_RESAMPLEZOH, OP_DOWNSAMPLEHALF, OP_FILTER, OP_HILOGAIN, OP_UNKNOWN25, OP_INTERLEAVE
};

static MixJob* sRec;      /* the job being recorded */
static uint32_t sRecId;
static uint32_t sJobOfAiBuffer[8];
static int sOverflowLogged;
#ifndef RETRO_GO
static int sUseMe = -1;   /* -1: not decided yet */
#endif

static void rec(uint32_t op, int n, const uint32_t* a) {
    int i;
    if (sRec == NULL) return;
    if (sRec->nwords + 1 + (uint32_t) n > MIXJ_WORDS) {
#ifdef RETRO_GO
        RG_PANIC("MK64 mixer command overflow");
#endif
        if (!sOverflowLogged) { sOverflowLogged = 1; PORT_LOG("audio: mixer job overflow (%u words)\n", (unsigned) sRec->nwords); }
        return;
    }
    sRec->words[sRec->nwords++] = op | ((uint32_t) n << 8);
    for (i = 0; i < n; i++) sRec->words[sRec->nwords++] = a[i];
}
#define REC(op, ...) do { const uint32_t a_[] = { __VA_ARGS__ }; rec(op, (int) (sizeof(a_) / sizeof(a_[0])), a_); } while (0)
#define U(x) ((uint32_t) (x))
#define P(x) ((uint32_t) (uintptr_t) (x))

void mixq_aClearBuffer(uint16_t addr, int nbytes) { REC(OP_CLEAR, U(addr), U(nbytes)); }
void mixq_aLoadADPCM(int n, const int16_t* book) { REC(OP_LOADADPCM, U(n), P(book)); }
void mixq_aSetBuffer(uint8_t f, uint16_t in, uint16_t out, uint16_t nbytes) { REC(OP_SETBUFFER, U(f), U(in), U(out), U(nbytes)); }
void mixq_aDMEMMove(uint16_t in, uint16_t out, int nbytes) { REC(OP_DMEMMOVE, U(in), U(out), U(nbytes)); }
void mixq_aSetLoop(ADPCM_STATE* s) { REC(OP_SETLOOP, P(s)); }
void mixq_aADPCMdec(uint8_t f, ADPCM_STATE s) { REC(OP_ADPCMDEC, U(f), P(s)); }
void mixq_aResample(uint8_t f, uint16_t pitch, RESAMPLE_STATE s) { REC(OP_RESAMPLE, U(f), U(pitch), P(s)); }
void mixq_aLoadBuffer(const void* src, uint16_t dest, uint16_t nbytes) { REC(OP_LOADBUFFER, P(src), U(dest), U(nbytes)); }
void mixq_aSaveBuffer(uint16_t src, int16_t* dest, uint16_t nbytes) { REC(OP_SAVEBUFFER, U(src), P(dest), U(nbytes)); }
void mixq_aInterleaveOld(uint16_t l, uint16_t r) { REC(OP_INTERLEAVEOLD, U(l), U(r)); }
void mixq_aInterleave(uint16_t d, uint16_t l, uint16_t r, uint16_t c) { REC(OP_INTERLEAVE, U(d), U(l), U(r), U(c)); }
void mixq_aMix(int16_t gain, uint16_t in, uint16_t out, uint16_t count) { REC(OP_MIX, U((uint16_t) gain), U(in), U(out), U(count)); }
void mixq_aEnvSetup1(uint8_t a, uint16_t b, uint16_t c, uint16_t d) { REC(OP_ENVSETUP1, U(a), U(b), U(c), U(d)); }
void mixq_aEnvSetup2(uint16_t l, uint16_t r) { REC(OP_ENVSETUP2, U(l), U(r)); }
void mixq_aEnvMixer(uint16_t in, uint16_t n, bool swap, bool negl, bool negr, uint16_t dl, uint16_t dr, uint16_t wl, uint16_t wr) {
    REC(OP_ENVMIXER, U(in), U(n), U(swap), U(negl), U(negr), U(dl), U(dr), U(wl), U(wr));
}
void mixq_aS8Dec(uint8_t f, ADPCM_STATE s) { REC(OP_S8DEC, U(f), P(s)); }
void mixq_aAddMixer(uint16_t in, uint16_t out, uint16_t c) { REC(OP_ADDMIXER, U(in), U(out), U(c)); }
void mixq_aDuplicate(uint16_t in, uint16_t out, uint16_t c) { REC(OP_DUPLICATE, U(in), U(out), U(c)); }
void mixq_aDMEMMove2(uint8_t t, uint16_t in, uint16_t out, uint16_t c) { REC(OP_DMEMMOVE2, U(t), U(in), U(out), U(c)); }
void mixq_aResampleZoh(uint16_t pitch, uint16_t frac) { REC(OP_RESAMPLEZOH, U(pitch), U(frac)); }
void mixq_aDownsampleHalf(uint16_t n, uint16_t in, uint16_t out) { REC(OP_DOWNSAMPLEHALF, U(n), U(in), U(out)); }
void mixq_aFilter(uint8_t f, uint16_t c, int16_t* s) { REC(OP_FILTER, U(f), U(c), P(s)); }
void mixq_aHiLoGain(uint8_t g, uint16_t c, uint16_t addr) { REC(OP_HILOGAIN, U(g), U(c), U(addr)); }
void mixq_aUnknown25(uint8_t f, uint16_t c, uint16_t out, uint16_t in) { REC(OP_UNKNOWN25, U(f), U(c), U(out), U(in)); }

/* No OS call, no libc beyond what the mixer itself uses: this runs on the ME too. */
void mix_job_execute(const MixJob* job, uint32_t m) {
    const uint32_t* w = job->words;
    const uint32_t* end = w + job->nwords;
    while (w < end) {
        uint32_t op = w[0] & 0xFF, n = (w[0] >> 8) & 0xFF;
        const uint32_t* a = w + 1;
#define PTR(T, v) ((T) (uintptr_t) ((v) | m))
        switch (op) {
            case OP_CLEAR: aClearBufferImpl((uint16_t) a[0], (int) a[1]); break;
            case OP_LOADADPCM: aLoadADPCMImpl((int) a[0], PTR(const int16_t*, a[1])); break;
            case OP_SETBUFFER: aSetBufferImpl((uint8_t) a[0], (uint16_t) a[1], (uint16_t) a[2], (uint16_t) a[3]); break;
            case OP_DMEMMOVE: aDMEMMoveImpl((uint16_t) a[0], (uint16_t) a[1], (int) a[2]); break;
            case OP_SETLOOP: aSetLoopImpl(PTR(ADPCM_STATE*, a[0])); break;
            case OP_ADPCMDEC: aADPCMdecImpl((uint8_t) a[0], PTR(int16_t*, a[1])); break;
            case OP_RESAMPLE: aResampleImpl((uint8_t) a[0], (uint16_t) a[1], PTR(int16_t*, a[2])); break;
            case OP_LOADBUFFER: aLoadBufferImpl(PTR(const void*, a[0]), (uint16_t) a[1], (uint16_t) a[2]); break;
            case OP_SAVEBUFFER: aSaveBufferImpl((uint16_t) a[0], PTR(int16_t*, a[1]), (uint16_t) a[2]); break;
            case OP_INTERLEAVEOLD: aInterleaveOldImpl((uint16_t) a[0], (uint16_t) a[1]); break;
            case OP_INTERLEAVE: aInterleaveImpl((uint16_t) a[0], (uint16_t) a[1], (uint16_t) a[2], (uint16_t) a[3]); break;
            case OP_MIX: aMixImpl((int16_t) a[0], (uint16_t) a[1], (uint16_t) a[2], (uint16_t) a[3]); break;
            case OP_ENVSETUP1: aEnvSetup1Impl((uint8_t) a[0], (uint16_t) a[1], (uint16_t) a[2], (uint16_t) a[3]); break;
            case OP_ENVSETUP2: aEnvSetup2Impl((uint16_t) a[0], (uint16_t) a[1]); break;
            case OP_ENVMIXER: aEnvMixerImpl((uint16_t) a[0], (uint16_t) a[1], a[2] != 0, a[3] != 0, a[4] != 0, (uint16_t) a[5], (uint16_t) a[6], (uint16_t) a[7], (uint16_t) a[8]); break;
            case OP_S8DEC: aS8DecImpl((uint8_t) a[0], PTR(int16_t*, a[1])); break;
            case OP_ADDMIXER: aAddMixerImpl((uint16_t) a[0], (uint16_t) a[1], (uint16_t) a[2]); break;
            case OP_DUPLICATE: aDuplicateImpl((uint16_t) a[0], (uint16_t) a[1], (uint16_t) a[2]); break;
            case OP_DMEMMOVE2: aDMEMMove2Impl((uint8_t) a[0], (uint16_t) a[1], (uint16_t) a[2], (uint16_t) a[3]); break;
            case OP_RESAMPLEZOH: aResampleZohImpl((uint16_t) a[0], (uint16_t) a[1]); break;
            case OP_DOWNSAMPLEHALF: aDownsampleHalfImpl((uint16_t) a[0], (uint16_t) a[1], (uint16_t) a[2]); break;
            case OP_FILTER: aFilterImpl((uint8_t) a[0], (uint16_t) a[1], PTR(int16_t*, a[2])); break;
            case OP_HILOGAIN: aHiLoGainImpl((uint8_t) a[0], (uint16_t) a[1], (uint16_t) a[2]); break;
            case OP_UNKNOWN25: aUnknown25Impl((uint8_t) a[0], (uint16_t) a[1], (uint16_t) a[2], (uint16_t) a[3]); break;
        }
#undef PTR
        w += 1 + n;
    }
}

#ifndef RETRO_GO
#define SHARED ((MixShared*) ((uintptr_t) &gMixShared | 0x40000000u))

void port_mix_begin(int ai_buffer) {
    if (sUseMe < 0) {
        FILE* f = NULL;
#ifdef PORT_DEBUG_KNOBS
        f = fopen(port_save_path("nome"), "rb"); /* test knob: data/nome keeps the mixer on the main CPU */
#endif
        if (f != NULL) { fclose(f); sUseMe = 0; PORT_LOG("audio: mixer on the main CPU (data/nome)\n"); }
        else {
            sUseMe = port_me_start();
            PORT_LOG("audio: mixer on the %s\n", sUseMe ? "Media Engine" : "main CPU (no Media Engine)");
        }
    }
    sRecId = SHARED->submitted;
    sRec = &gMixJobs[sRecId % MIXJ_JOBS];
    sRec->nwords = 0;
    sJobOfAiBuffer[ai_buffer & 7] = sRecId + 1; /* done once completed reaches this */
}

void port_mix_submit(void) {
    if (sRec == NULL) return;
    if (sUseMe) {
        extern void sceKernelDcacheWritebackAll(void);
        sceKernelDcacheWritebackAll(); /* the job, and any sample data loaded this frame, reach RAM before the ME reads them */
        SHARED->submitted = sRecId + 1;
        port_me_kick(); /* a running task picks the job up by itself */
    } else {
        mix_job_execute(sRec, 0);
        SHARED->submitted = sRecId + 1;
        SHARED->completed = sRecId + 1;
    }
    sRec = NULL;
}

static void wait_for(uint32_t target) {
    extern int sceKernelDelayThread(unsigned int us);
    unsigned int t0;
    if (!sUseMe || (int32_t) (SHARED->completed - target) >= 0) return;
    t0 = port_time_us();
    while ((int32_t) (SHARED->completed - target) < 0) {
        port_me_kick(); /* the task may have ended just as the job was submitted */
        if (port_time_us() - t0 > 500000u) {
            /* The ME stopped answering: finish on the main CPU and stay there. */
            PORT_LOG("audio: the Media Engine timed out at job %u of %u: back to the main CPU\n", (unsigned) SHARED->completed, (unsigned) target);
            port_me_stop();
            sUseMe = 0;
            while ((int32_t) (SHARED->completed - target) < 0) {
                mix_job_execute(&gMixJobs[SHARED->completed % MIXJ_JOBS], 0);
                SHARED->completed = SHARED->completed + 1;
            }
            return;
        }
        sceKernelDelayThread(200);
    }
}

void* port_mix_wait(int ai_buffer, void* buffer) {
    wait_for(sJobOfAiBuffer[ai_buffer & 7]);
    return sUseMe ? (void*) ((uintptr_t) buffer | 0x40000000u) : buffer; /* the ME wrote it past our cache */
}

void port_mix_drain(void) {
    wait_for(SHARED->submitted);
}

#else
void port_mix_begin(int ai_buffer) {
    sRecId = __atomic_load_n(&gMixShared.submitted, __ATOMIC_ACQUIRE);
    mk64_rg_mix_wait(sRecId);
    sRec = &gMixJobs[sRecId % MIXJ_JOBS];
    sRec->nwords = 0;
    sJobOfAiBuffer[ai_buffer & 7] = sRecId + 1;
}
void port_mix_submit(void) {
    if (!sRec) return;
    sRec = NULL;
    mk64_rg_mix_submit(sRecId);
}
void* port_mix_wait(int ai_buffer, void* buffer) {
    mk64_rg_mix_wait(sJobOfAiBuffer[ai_buffer & 7]);
    return buffer;
}
void port_mix_drain(void) {
    mk64_rg_mix_wait(__atomic_load_n(&gMixShared.submitted, __ATOMIC_ACQUIRE));
}
#endif
