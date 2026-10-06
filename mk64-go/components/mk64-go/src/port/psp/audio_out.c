/**
 * PSP audio output for the port: replaces the N64 AI.
 *
 * The game's audio engine (create_next_audio_frame_task) hands the AI a
 * variable-length buffer of 32 kHz stereo s16 samples every VI retrace via
 * osAiSetNextBuffer() and sizes the next one from osAiGetLength().  Here the
 * samples go into a FIFO that is drained to a PSP SRC channel in fixed 512
 * sample blocks; the block output blocks until the previous block finished
 * playing, so the game loop is paced by the audio clock.
 */
#include <PR/ultratypes.h>
#include <string.h>
#include <pspkernel.h>
#include <pspaudio.h>
#include <stdio.h>
#include "port.h"
#include <macros.h>

#ifdef PORT_AUDIO_CAPTURE
/* Dump the raw s16 stereo stream to data/audio.raw next to the EBOOT (a few seconds),
 * so the exact samples the game produces can be pulled off and analysed. */
static FILE* sCapFile;
static u32 sCapBytes;
#define CAP_LIMIT (32000 * 2 * 2 * 8) /* ~8 seconds */
#endif

#define OUT_FREQ 32000
/* The game mixes at its audio-session frequency (every MK64 preset: 26800 Hz,
 * see audio_session_presets.c) but the PSP SRC channel only takes a fixed set
 * of rates, so the stream is resampled (linear, stateful) to OUT_FREQ here.
 * Playing the 26.8 kHz mix straight at 32 kHz ran everything 19% fast. */
static u32 sInRate = OUT_FREQ;
static s16 sPrevL, sPrevR;   // last input sample of the previous push
static u32 sPhase = 0x10000; // 16.16 position of the next output sample, measured from sPrev
static u32 sStep = 0x10000;  // input samples per output sample, 16.16
void port_audio_out_set_rate(u32 freq) {
    if (freq < 8000 || freq > 48000) freq = OUT_FREQ;
    sInRate = freq;
    sStep = (u32) (((u64) freq << 16) / OUT_FREQ);
    sPhase = 0x10000;
    sPrevL = sPrevR = 0;
}
#define OUT_BLOCK 512                 // samples per sceAudio output (multiple of 64)
#define FIFO_SAMPLES (OUT_BLOCK * 16) // ring buffer capacity in stereo samples

static s16 sFifo[FIFO_SAMPLES * 2];
static u32 sFifoRead;
static u32 sFifoWrite; // total samples written (mod FIFO_SAMPLES when indexing)
static s16 sBlock[2][OUT_BLOCK * 2] __attribute__((aligned(64)));
static int sBlockIdx;
static int sChannel = -1;
static int sMuted;

static u32 fifo_count(void);
static volatile u32 sUnderruns;
static volatile u32 sBlocksOut;

/* Output thread: the only caller of the blocking sceAudio output, so the game
 * thread never stalls on the audio hardware.  Single producer / single
 * consumer ring: the game thread only advances sFifoWrite, this thread only
 * advances sFifoRead.  On underrun a block of silence keeps the stream timed. */
static int audio_thread(UNUSED SceSize args, UNUSED void* argp) {
    for (;;) {
        s16* blk = sBlock[sBlockIdx];
        u32 i;
        if (fifo_count() >= OUT_BLOCK) {
            u32 base = sFifoRead % FIFO_SAMPLES;
            for (i = 0; i < OUT_BLOCK; i++) {
                u32 idx = (base + i) % FIFO_SAMPLES;
                blk[i * 2] = sMuted ? 0 : sFifo[idx * 2];
                blk[i * 2 + 1] = sMuted ? 0 : sFifo[idx * 2 + 1];
            }
            sFifoRead += OUT_BLOCK;
        } else {
            memset(blk, 0, OUT_BLOCK * 4);
            sUnderruns++;
        }
        sBlocksOut++;
        sceKernelDcacheWritebackRange(blk, OUT_BLOCK * 4);
        if (sChannel >= 0) {
            sceAudioOutput2OutputBlocking(PSP_AUDIO_VOLUME_MAX, blk); // returns as the previous block finishes
        } else {
            sceKernelDelayThread(16000);
        }
        sBlockIdx ^= 1;
    }
    return 0;
}

u32 port_audio_out_underruns(void) {
    return sUnderruns;
}

void port_audio_out_init(void) {
    SceUID thid;
    sChannel = sceAudioSRCChReserve(OUT_BLOCK, OUT_FREQ, 2);
    if (sChannel < 0) {
        PORT_LOG("audio: sceAudioSRCChReserve failed (%08X)\n", sChannel);
    }
    sFifoRead = sFifoWrite = 0;
    thid = sceKernelCreateThread("audio_out", audio_thread, 0x12, 0x4000, THREAD_ATTR_USER, NULL);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, NULL);
    } else {
        PORT_LOG("audio: thread create failed (%08X)\n", thid);
    }
}

static u32 fifo_count(void) {
    return sFifoWrite - sFifoRead;
}

/* osAiGetLength(): bytes of audio still queued (FIFO + the block playing). */
u32 port_audio_out_queued_bytes(void) {
    int rest = sChannel >= 0 ? sceAudioOutput2GetRestSample() : 0;
    u32 queued;
    if (rest < 0) {
        rest = 0;
    }
    queued = fifo_count() + (u32) rest; // output-rate samples
    if (sInRate != OUT_FREQ) {
        queued = (u32) (((u64) queued * sInRate) / OUT_FREQ); // in the game's sample units
    }
    return queued * 4;
}

static void fifo_push(const s16* samples, u32 n) {
    u32 i;
    if (n > FIFO_SAMPLES - fifo_count()) {
        n = FIFO_SAMPLES - fifo_count(); // drop what does not fit
    }
    for (i = 0; i < n; i++) {
        u32 idx = (sFifoWrite + i) % FIFO_SAMPLES;
        sFifo[idx * 2] = samples[i * 2];
        sFifo[idx * 2 + 1] = samples[i * 2 + 1];
    }
    sFifoWrite += n;
}

/* osAiSetNextBuffer(): queue `bytes` of stereo s16 samples. */
void port_audio_out_push(const s16* samples, u32 bytes) {
    u32 n = bytes / 4;
    u32 i;
#ifdef PORT_AUDIO_CAPTURE
    extern volatile s32 gAudioFrameCount;
    if (gAudioFrameCount > 900 && (u32)sCapBytes < CAP_LIMIT) {
        if (sCapFile == NULL) {
            sCapFile = fopen(port_save_path("audio.raw"), "wb");
        }
        if (sCapFile != NULL) {
            fwrite(samples, 1, bytes, sCapFile);
            sCapBytes += bytes;
            if (sCapBytes >= CAP_LIMIT) {
                fflush(sCapFile);
                fclose(sCapFile);
                sCapFile = NULL;
                PORT_LOG("audio capture done: %u bytes\n", sCapBytes);
            }
        }
    }
#endif
#ifdef PORT_AUDIO_DEBUG
    { static int c; s32 pk = 0; for (i = 0; i < n * 2; i++) { s32 a = samples[i] < 0 ? -samples[i] : samples[i]; if (a > pk) pk = a; } if (c++ < 30) PORT_LOG("audio peak %d (%u)\n", (int) pk, n); }
#endif
    (void) i;
    if (sInRate == OUT_FREQ) {
        fifo_push(samples, n);
        return;
    }
    {
        // Linear interpolation between x[i-1] and x[i], x[-1] = previous push's last sample.
        s16 tmp[256 * 2];
        u32 k = 0;
        for (;;) {
            u32 idx = sPhase >> 16;
            s32 frac, a, b;
            if (idx >= n) {
                break; // needs input that has not arrived yet
            }
            frac = (s32) (sPhase & 0xFFFF);
            a = idx == 0 ? sPrevL : samples[(idx - 1) * 2];
            b = samples[idx * 2];
            tmp[k * 2] = (s16) (a + (((b - a) * frac) >> 16));
            a = idx == 0 ? sPrevR : samples[(idx - 1) * 2 + 1];
            b = samples[idx * 2 + 1];
            tmp[k * 2 + 1] = (s16) (a + (((b - a) * frac) >> 16));
            sPhase += sStep;
            if (++k == 256) {
                fifo_push(tmp, k);
                k = 0;
            }
        }
        if (k) {
            fifo_push(tmp, k);
        }
        sPhase -= n << 16;
        sPrevL = samples[(n - 1) * 2];
        sPrevR = samples[(n - 1) * 2 + 1];
    }
}

void port_audio_out_set_muted(int muted) {
    sMuted = muted;
}
