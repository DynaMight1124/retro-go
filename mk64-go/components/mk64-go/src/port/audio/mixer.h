#ifndef MIXER_H
#define MIXER_H

#include <stdbool.h>
#include <stdint.h>
#include <ultra64.h>

/* Mario Kart 64 uses the newer audio microcode (same ABI family as Shindou SM64). */
#define NEW_AUDIO_UCODE

#undef aSegment
#undef aClearBuffer
#undef aSetBuffer
#undef aLoadBuffer
#undef aSaveBuffer
#undef aDMEMMove
#undef aMix
#undef aEnvMixer
#undef aResample
#undef aInterleave
#undef aSetVolume
#undef aSetVolume32
#undef aSetLoop
#undef aLoadADPCM
#undef aADPCMdec
#undef aS8Dec
#undef aAddMixer
#undef aDuplicate
#undef aDMEMMove2
#undef aResampleZoh
#undef aDownsampleHalf
#undef aEnvSetup1
#undef aEnvSetup2
#undef aFilter
#undef aHiLoGain
#undef aUnknown25

void aClearBufferImpl(uint16_t addr, int nbytes);
void aLoadADPCMImpl(int num_entries_times_16, const int16_t *book_source_addr);
void aSetBufferImpl(uint8_t flags, uint16_t in, uint16_t out, uint16_t nbytes);
void aDMEMMoveImpl(uint16_t in_addr, uint16_t out_addr, int nbytes);
void aSetLoopImpl(ADPCM_STATE *adpcm_loop_state);
void aADPCMdecImpl(uint8_t flags, ADPCM_STATE state);
void aResampleImpl(uint8_t flags, uint16_t pitch, RESAMPLE_STATE state);

#ifndef NEW_AUDIO_UCODE
void aSetVolumeImpl(uint8_t flags, int16_t v, int16_t t, int16_t r);
void aLoadBufferImpl(const void *source_addr);
void aSaveBufferImpl(int16_t *dest_addr);
void aInterleaveImpl(uint16_t left, uint16_t right);
void aMixImpl(int16_t gain, uint16_t in_addr, uint16_t out_addr);
void aEnvMixerImpl(uint8_t flags, ENVMIX_STATE state);
#else
void aLoadBufferImpl(const void *source_addr, uint16_t dest_addr, uint16_t nbytes);
void aSaveBufferImpl(uint16_t source_addr, int16_t *dest_addr, uint16_t nbytes);
void aInterleaveImpl(uint16_t dest, uint16_t left, uint16_t right, uint16_t c);
void aMixImpl(int16_t gain, uint16_t in_addr, uint16_t out_addr, uint16_t count);
void aEnvSetup1Impl(uint8_t initial_vol_wet, uint16_t rate_wet, uint16_t rate_left, uint16_t rate_right);
void aEnvSetup2Impl(uint16_t initial_vol_left, uint16_t initial_vol_right);
void aEnvMixerImpl(uint16_t in_addr, uint16_t n_samples, bool swap_reverb,
                   bool neg_left, bool neg_right,
                   uint16_t dry_left_addr, uint16_t dry_right_addr,
                   uint16_t wet_left_addr, uint16_t wet_right_addr);
void aS8DecImpl(uint8_t flags, ADPCM_STATE state);
void aAddMixerImpl(uint16_t in_addr, uint16_t out_addr, uint16_t count);
void aDuplicateImpl(uint16_t in_addr, uint16_t out_addr, uint16_t count);
void aDMEMMove2Impl(uint8_t t, uint16_t in_addr, uint16_t out_addr, uint16_t count);
void aResampleZohImpl(uint16_t pitch, uint16_t start_fract);
void aDownsampleHalfImpl(uint16_t n_samples, uint16_t in_addr, uint16_t out_addr);
void aFilterImpl(uint8_t flags, uint16_t count_or_buf, int16_t state_or_filter[8]);
void aHiLoGainImpl(uint8_t g, uint16_t count, uint16_t addr);
void aUnknown25Impl(uint8_t f, uint16_t count, uint16_t out_addr, uint16_t in_addr);
#endif

#define aSegment(pkt, s, b) do { } while(0)
#define aClearBuffer(pkt, d, c) aClearBufferImpl(d, c)
#define aLoadADPCM(pkt, c, d) aLoadADPCMImpl(c, d)
#define aSetBuffer(pkt, f, i, o, c) aSetBufferImpl(f, i, o, c)
#define aDMEMMove(pkt, i, o, c) aDMEMMoveImpl(i, o, c)
#define aSetLoop(pkt, a) aSetLoopImpl(a)
#ifdef PORT_PROFILE_MIX
extern unsigned int port_time_us(void);
extern void port_profile_add(int slot, unsigned int us);
#define MIXPROF(slot, call) do { unsigned int _t0 = port_time_us(); call; port_profile_add(slot, port_time_us() - _t0); } while (0)
#else
#define MIXPROF(slot, call) call
#endif
#define aADPCMdec(pkt, f, s) MIXPROF(6, aADPCMdecImpl(f, s))
#define aResample(pkt, f, p, s) MIXPROF(4, aResampleImpl(f, p, s))

#ifndef NEW_AUDIO_UCODE
#define aSetVolume(pkt, f, v, t, r) aSetVolumeImpl(f, v, t, r)
#define aSetVolume32(pkt, f, v, tr) aSetVolume(pkt, f, v, (int16_t)((tr) >> 16), (int16_t)(tr))
#define aLoadBuffer(pkt, s) aLoadBufferImpl(s)
#define aSaveBuffer(pkt, s) aSaveBufferImpl(s)
#define aInterleave(pkt, l, r) aInterleaveImpl(l, r)
#define aMix(pkt, f, g, i, o) MIXPROF(7, aMixImpl(g, i, o))
#define aEnvMixer(pkt, f, s) MIXPROF(5, aEnvMixerImpl(f, s))
#else
#define aLoadBuffer(pkt, s, d, c) aLoadBufferImpl(s, d, c)
#define aSaveBuffer(pkt, s, d, c) aSaveBufferImpl(s, d, c)
/* MK64 uses the old 2-argument form: output/count come from aSetBuffer. */
void aInterleaveOldImpl(uint16_t left, uint16_t right);
#define aInterleave(pkt, l, r) aInterleaveOldImpl(l, r)
#define aMix(pkt, g, i, o, c) MIXPROF(7, aMixImpl(g, i, o, c))
#define aEnvSetup1(pkt, initialVolReverb, rampReverb, rampLeft, rampRight) \
    aEnvSetup1Impl(initialVolReverb, rampReverb, rampLeft, rampRight)
#undef aEnvSetup1Alt
#define aEnvSetup1Alt(pkt, initialVolReverb, rampReverbL, rampReverbR, rampLeft, rampRight) \
    aEnvSetup1Impl(initialVolReverb, (uint16_t) ((((rampReverbL) & 0xFF) << 8) | ((rampReverbR) & 0xFF)), rampLeft, rampRight)
#define aEnvSetup2(pkt, initialVolLeft, initialVolRight) \
    aEnvSetup2Impl(initialVolLeft, initialVolRight)
#define aEnvMixer(pkt, inBuf, nSamples, swapReverb, negLeft, negRight,  \
                  dryLeft, dryRight, wetLeft, wetRight) \
    MIXPROF(5, aEnvMixerImpl(inBuf, nSamples, swapReverb, negLeft, negRight, \
                  dryLeft, dryRight, wetLeft, wetRight))
#define aS8Dec(pkt, f, s) aS8DecImpl(f, s)
#define aAddMixer(pkt, s, d, c) aAddMixerImpl(s, d, c)
#define aDuplicate(pkt, s, d, c) aDuplicateImpl(s, d, c)
#define aDMEMMove2(pkt, t, i, o, c) aDMEMMove2Impl(t, i, o, c)
#define aResampleZoh(pkt, pitch, startFract) aResampleZohImpl(pitch, startFract)
#define aDownsampleHalf(pkt, nSamples, i, o) aDownsampleHalfImpl(nSamples, i, o)
#define aFilter(pkt, f, countOrBuf, addr) aFilterImpl(f, countOrBuf, addr)
#define aHiLoGain(pkt, g, buflen, i) aHiLoGainImpl(g, buflen, i)
#define aUnknown25(pkt, f, c, o, i) aUnknown25Impl(f, c, o, i)
#endif


/* The game records its commands into a job (mix_jobs.h); the mixer and the job
 * executor keep the real entry points. */
#ifndef MIXER_INTERNAL
#define aClearBufferImpl mixq_aClearBuffer
#define aLoadADPCMImpl mixq_aLoadADPCM
#define aSetBufferImpl mixq_aSetBuffer
#define aDMEMMoveImpl mixq_aDMEMMove
#define aSetLoopImpl mixq_aSetLoop
#define aADPCMdecImpl mixq_aADPCMdec
#define aResampleImpl mixq_aResample
#define aLoadBufferImpl mixq_aLoadBuffer
#define aSaveBufferImpl mixq_aSaveBuffer
#define aInterleaveOldImpl mixq_aInterleaveOld
#define aInterleaveImpl mixq_aInterleave
#define aMixImpl mixq_aMix
#define aEnvSetup1Impl mixq_aEnvSetup1
#define aEnvSetup2Impl mixq_aEnvSetup2
#define aEnvMixerImpl mixq_aEnvMixer
#define aS8DecImpl mixq_aS8Dec
#define aAddMixerImpl mixq_aAddMixer
#define aDuplicateImpl mixq_aDuplicate
#define aDMEMMove2Impl mixq_aDMEMMove2
#define aResampleZohImpl mixq_aResampleZoh
#define aDownsampleHalfImpl mixq_aDownsampleHalf
#define aFilterImpl mixq_aFilter
#define aHiLoGainImpl mixq_aHiLoGain
#define aUnknown25Impl mixq_aUnknown25
void mixq_aClearBuffer(uint16_t addr, int nbytes);
void mixq_aLoadADPCM(int n, const int16_t* book);
void mixq_aSetBuffer(uint8_t f, uint16_t in, uint16_t out, uint16_t nbytes);
void mixq_aDMEMMove(uint16_t in, uint16_t out, int nbytes);
void mixq_aSetLoop(ADPCM_STATE* s);
void mixq_aADPCMdec(uint8_t f, ADPCM_STATE s);
void mixq_aResample(uint8_t f, uint16_t pitch, RESAMPLE_STATE s);
void mixq_aLoadBuffer(const void* src, uint16_t dest, uint16_t nbytes);
void mixq_aSaveBuffer(uint16_t src, int16_t* dest, uint16_t nbytes);
void mixq_aInterleaveOld(uint16_t l, uint16_t r);
void mixq_aInterleave(uint16_t d, uint16_t l, uint16_t r, uint16_t c);
void mixq_aMix(int16_t gain, uint16_t in, uint16_t out, uint16_t count);
void mixq_aEnvSetup1(uint8_t a, uint16_t b, uint16_t c, uint16_t d);
void mixq_aEnvSetup2(uint16_t l, uint16_t r);
void mixq_aEnvMixer(uint16_t in, uint16_t n, bool swap, bool negl, bool negr, uint16_t dl, uint16_t dr, uint16_t wl, uint16_t wr);
void mixq_aS8Dec(uint8_t f, ADPCM_STATE s);
void mixq_aAddMixer(uint16_t in, uint16_t out, uint16_t c);
void mixq_aDuplicate(uint16_t in, uint16_t out, uint16_t c);
void mixq_aDMEMMove2(uint8_t t, uint16_t in, uint16_t out, uint16_t c);
void mixq_aResampleZoh(uint16_t pitch, uint16_t frac);
void mixq_aDownsampleHalf(uint16_t n, uint16_t in, uint16_t out);
void mixq_aFilter(uint8_t f, uint16_t c, int16_t* s);
void mixq_aHiLoGain(uint8_t g, uint16_t c, uint16_t addr);
void mixq_aUnknown25(uint8_t f, uint16_t c, uint16_t out, uint16_t in);
#endif

#endif
