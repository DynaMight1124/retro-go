#include "snd.h"
#include "radio_stream.h"
#include "assets.h"
#include "fx.h"
#include "j2me.h"
#include "gtavc_profile.h"
#include <rg_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static rg_mutex_t* snd_lock = NULL;

static inline void snd_lock_acquire(void) {
    if (!snd_lock) return;
    /* The mixer must never write the game task's counters or race report reset. */
    if (rg_task_current() == gtavc_profile_game_task) {
        GTAVC_TIME(PROF_AUDIO_WAIT, rg_mutex_take(snd_lock, -1));
    } else {
        rg_mutex_take(snd_lock, -1);
    }
}

static inline void snd_lock_release(void) {
    if (snd_lock) rg_mutex_give(snd_lock);
}

bool Snd_on = true;
bool Snd_ok = false;

#define SND_OUT_RATE 22050
#define SND_MAX_MIX 12
#define SND_VOICE_TAG 1000
#define SND_RADIO_TRACKS 15
#define SND_MASTER 70         /* percent: leave headroom before the mono hardware limiter */

static const char* FILE_NAMES[SND_N] = {
    "punch", "gun", "bat", "swing", "crash", "dooropen", "doorclose", "horn", "pickup",
    "explode", "splash", "hitped", "siren", "idle", "rev"
};

static const int VOICE_COUNT[5] = { 3, 2, 2, 3, 2 };
static const char* VOICE_M[5][3] = {
    { "v_mpanic0", "v_mpanic1", "v_mpanic2" },
    { "v_mdeath0", "v_mdeath1", NULL },
    { "v_fight0", "v_fight1", NULL },
    { "v_chat0", "v_chat1", "v_chat2" },
    { "v_tommy0", "v_tommy1", NULL }
};
static const char* VOICE_F[5][3] = {
    { "v_fpanic0", "v_fpanic1", "v_fpanic2" },
    { "v_fdeath0", "v_fdeath1", NULL },
    { "v_fight0", "v_fight1", NULL },
    { "v_chat0", "v_chat1", "v_chat2" },
    { "v_tommy0", "v_tommy1", NULL }
};

typedef struct {
    int16_t* pcm;      /* mono, signed 16 bit */
    int len;           /* samples */
    int rate;          /* source Hz */
} SndClip;

static SndClip clips[SND_N];
static SndClip voiceM[5][3];
static SndClip voiceF[5][3];
static int radioLoaded = -1;
static bool radioPlaying = false;
static bool radioPrepared = false;
static int radioRetryMs = 0;
static uint16_t radioTracks = 0;
static char radioDir[600];
static int64_t radioRetryAfterUs = 0;

/* Scan only during sound initialization, never per gameplay tick. */
static void radioDiscover(void) {
    char configured[600];
    snprintf(configured, sizeof(configured), "%s/radio", platform_assets_dir());
    const char *dirs[] = {configured, "assets/radio", "res/radio",
                         RG_BASE_PATH_ROMS "/gtavc/radio"};
    radioTracks = 0;
    radioDir[0] = 0;
    for (unsigned i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
        DIR *dir = opendir(dirs[i]);
        if (!dir) continue;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            for (int track = 0; track < SND_RADIO_TRACKS; track++) {
                char name[16];
                snprintf(name, sizeof(name), "%02d.wav", track + 1);
                if (strcmp(entry->d_name, name) == 0)
                    radioTracks |= (uint16_t)(1u << track);
            }
        }
        closedir(dir);
        if (radioTracks) {
            snprintf(radioDir, sizeof(radioDir), "%s", dirs[i]);
            break;
        }
    }
    if (!radioTracks) {
        RG_LOGD("[RADIO] no optional station files found; skipping radio loads this session (effects enabled)");
    } else {
        RG_LOGD("[RADIO] station_mask=0x%04x dir=%.120s", (unsigned)radioTracks, radioDir);
    }
}

static bool radioCanLoad(void) {
    if (RadioStream_failed()) {
        RadioStream_cancel();
        radioLoaded = -1;
        radioRetryAfterUs = rg_system_timer() + 30000000;
    }
    return radioTracks && rg_system_timer() >= radioRetryAfterUs;
}
static uint32_t radioSeed = 0x13579BDFu;

typedef struct {
    const SndClip* clip;
    uint64_t pos;      /* source position, 16.16 (radio clips exceed 32 bit) */
    uint32_t step;     /* source samples per output sample, 16.16 */
    uint64_t end;      /* len << 16 */
    int vol;           /* 0..256, Q8 */
    int env;           /* 0..256 attack envelope */
    bool loop;
    bool active;
    int tag;
    uint32_t age;
} MixVoice;

static MixVoice mixv[SND_MAX_MIX];
static uint32_t mixClock;
static int engineState = -1;
static int wantEngine = -1;
static long long wantSince = 0;
static bool sirenOn = false;

static uint16_t rd16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t rd32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void clipFree(SndClip* c) {
    if (!c) return;
    free(c->pcm);
    c->pcm = NULL;
    c->len = 0;
    c->rate = 0;
}

/* Minimal RIFF/WAVE PCM decoder: 8/16-bit, mono or stereo.  The exported
   effects are 11025 Hz 16-bit mono; voices are 8000 Hz 8-bit mono. */
static bool clipLoad(const char* path, SndClip* c) {
    int len = 0;
    uint8_t* b = Assets_readAll(path, &len);
    if (!b || len < 12) {
        free(b);
        return false;
    }
    if (memcmp(b, "RIFF", 4) != 0 || memcmp(b + 8, "WAVE", 4) != 0) {
        free(b);
        return false;
    }

    int fmt = 0, ch = 0, bits = 0, rate = 0;
    size_t dataOff = 0, dataLen = 0;
    size_t p = 12;
    while (p + 8 <= (size_t)len) {
        uint32_t sz = rd32(b + p + 4);
        size_t payload = p + 8;
        if (memcmp(b + p, "fmt ", 4) == 0) {
            if (sz >= 16 && payload + 16 <= (size_t)len) {
                fmt = rd16(b + payload);
                ch = rd16(b + payload + 2);
                rate = (int)rd32(b + payload + 4);
                bits = rd16(b + payload + 14);
            }
        } else if (memcmp(b + p, "data", 4) == 0) {
            dataOff = payload;
            dataLen = sz;
            if (dataOff > (size_t)len) dataOff = (size_t)len;
            if (dataOff + dataLen > (size_t)len) dataLen = (size_t)len - dataOff;
        }
        size_t next = payload + (size_t)sz + (sz & 1u);
        if (next <= p || next > (size_t)len) break;
        p = next;
    }

    if (fmt != 1 || ch < 1 || ch > 2 || (bits != 8 && bits != 16) ||
        rate < 4000 || rate > 48000 || dataLen == 0) {
        free(b);
        return false;
    }

    int bytes = bits / 8;
    int frames = (int)(dataLen / (size_t)(bytes * ch));
    if (frames <= 0) {
        free(b);
        return false;
    }
    int16_t* pcm = (int16_t*)malloc((size_t)frames * sizeof(int16_t));
    if (!pcm) {
        free(b);
        return false;
    }
    for (int i = 0; i < frames; i++) {
        int32_t acc = 0;
        for (int cc = 0; cc < ch; cc++) {
            size_t off = dataOff + ((size_t)i * ch + cc) * (size_t)bytes;
            int32_t v;
            if (bits == 16) v = (int16_t)rd16(b + off);
            else v = ((int32_t)b[off] - 128) << 8;
            acc += v;
        }
        pcm[i] = (int16_t)(acc / ch);
    }
    free(b);

    clipFree(c);
    c->pcm = pcm;
    c->len = frames;
    c->rate = rate;
    return true;
}

static MixVoice* findTag(int tag) {
    for (int i = 0; i < SND_MAX_MIX; i++) {
        if (mixv[i].active && mixv[i].tag == tag) return &mixv[i];
    }
    return NULL;
}

static MixVoice* allocVoice(void) {
    MixVoice* steal = NULL;
    for (int i = 0; i < SND_MAX_MIX; i++) {
        if (!mixv[i].active) return &mixv[i];
        if (steal == NULL || mixv[i].age < steal->age) steal = &mixv[i];
    }
    return steal;
}

static void startClipAt(int tag, const SndClip* c, int vol, bool loop, uint32_t startSample) {
    if (!c || !c->pcm || c->len <= 0) return;
    if (vol <= 0) return;
    if (vol > 100) vol = 100;
    snd_lock_acquire();
    MixVoice* v = findTag(tag);
    if (!v) v = allocVoice();
    if (!v) {
        snd_lock_release();
        return;
    }
    v->clip = c;
    if (startSample >= (uint32_t)c->len) startSample = 0;
    v->pos = (uint64_t)startSample << 16;
    v->step = (uint32_t)(((uint64_t)(uint32_t)c->rate << 16) / SND_OUT_RATE);
    if (v->step == 0) v->step = 1;
    v->end = (uint64_t)c->len << 16;
    v->vol = vol * 256 / 100 * SND_MASTER / 100;
    v->env = 0;
    v->loop = loop;
    v->active = true;
    v->tag = tag;
    v->age = ++mixClock;
    snd_lock_release();
}

static void startClip(int tag, const SndClip* c, int vol, bool loop) {
    startClipAt(tag, c, vol, loop, 0);
}

static bool voiceActive(void) {
    snd_lock_acquire();
    bool active = (findTag(SND_VOICE_TAG) != NULL);
    snd_lock_release();
    return active;
}

void Snd_init(void) {
    if (!snd_lock) {
        snd_lock = rg_mutex_create();
    }
    Snd_cleanup();
    for (int i = 0; i < SND_N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/s_%s.wav", FILE_NAMES[i]);
        clipLoad(path, &clips[i]);
    }
    for (int k = 0; k < 5; k++) {
        for (int i = 0; i < VOICE_COUNT[k]; i++) {
            char path[64];
            if (VOICE_M[k][i]) {
                snprintf(path, sizeof(path), "/%s.wav", VOICE_M[k][i]);
                clipLoad(path, &voiceM[k][i]);
            }
            if (VOICE_F[k][i]) {
                snprintf(path, sizeof(path), "/%s.wav", VOICE_F[k][i]);
                clipLoad(path, &voiceF[k][i]);
            }
        }
    }
    radioDiscover();
    if (radioTracks && !RadioStream_init()) {
        RG_LOGW("Radio control allocation failed; effects remain enabled");
        radioTracks = 0;
    }
    /* Radio randomness is deliberately independent of Fx_seed so the
       simulation stays bit-identical to the J2ME reference. */
    radioSeed ^= (uint32_t)System_currentTimeMillis() * 2654435761u;
    if (radioSeed == 0) radioSeed = 0x13579BDFu;
    radioPlaying = false;
    radioPrepared = false;
    radioRetryMs = 0;
    /* Keep the RNG sequence identical to the original Snd.voice even when a
       WAV is missing or playback is disabled. */
    Snd_ok = true;
    Snd_on = true;
    engineState = -1;
    wantEngine = -1;
    mixClock = 0;
}

void Snd_playVol(int id, int vol) {
    if (!Snd_ok || !Snd_on) return;
    if (id < 0 || id >= SND_N) return;
    bool loop = (id == SND_SIREN || id == SND_IDLE || id == SND_REV);
    startClip(id, &clips[id], vol, loop);
}

void Snd_play(int id) {
    Snd_playVol(id, 100);
}

void Snd_play3d(int id, int x, int z, int lx, int lz) {
    int d = Fx_dist(x - lx, z - lz) / 1024;
    if (d > 70) return;
    Snd_playVol(id, 100 - d * 100 / 75);
}

void Snd_voice(int kind, bool female) {
    if (!Snd_ok || !Snd_on) return;
    if (kind < 0 || kind > 4) return;
    int idx = Fx_rndBound(VOICE_COUNT[kind]);
    if (voiceActive()) return;
    const SndClip* c = female ? &voiceF[kind][idx] : &voiceM[kind][idx];
    startClip(SND_VOICE_TAG, c, 90, false);
}

void Snd_engine(bool running, int pct) {
    int s = !running ? 0 : (pct < 22 ? 1 : (pct > 30 ? 2 : (engineState > 0 ? engineState : 1)));
    long long now = System_currentTimeMillis();
    if (s != wantEngine) {
        wantEngine = s;
        wantSince = now;
    }
    if (s == engineState) return;
    /* Hold a change for 0.7 s so the loops do not restart constantly.  A stop
       is immediate, matching the original worker. */
    if (s != 0 && engineState > 0 && now - wantSince < 700) return;
    if (engineState == 1) Snd_stop(SND_IDLE);
    if (engineState == 2) Snd_stop(SND_REV);
    engineState = s;
    if (s == 1) startClip(SND_IDLE, &clips[SND_IDLE], 60, true);
    else if (s == 2) startClip(SND_REV, &clips[SND_REV], 70, true);
}

void Snd_siren(bool onNow) {
    if (onNow == sirenOn) return;
    sirenOn = onNow;
    if (onNow) startClip(SND_SIREN, &clips[SND_SIREN], 70, true);
    else Snd_stop(SND_SIREN);
}

/* Radio uses bounded asynchronous PCM streaming; effects remain resident. */
static int radioPickTrack(void) {
    radioSeed = radioSeed * 1664525u + 1013904223u;
    int first = (int)((radioSeed >> 8) % SND_RADIO_TRACKS);
    for (int n = 0; n < SND_RADIO_TRACKS; n++) {
        int idx = (first + n) % SND_RADIO_TRACKS;
        if (radioTracks & (1u << idx)) return idx;
    }
    return -1;
}

static bool radioLoadTrack(int idx) {
    if (idx < 0 || !radioCanLoad()) return false;
    if (idx == radioLoaded && RadioStream_active()) return true;
    char path[640];
    int n = snprintf(path, sizeof(path), "%s/%02d.wav", radioDir, idx + 1);
    if (n <= 0 || (size_t)n >= sizeof(path)) return false;
    radioSeed = radioSeed * 1664525u + 1013904223u;
    if (!RadioStream_open(path, radioSeed)) {
        radioRetryAfterUs = rg_system_timer() + 30000000;
        RG_LOGW("Radio stream could not start; retry in 30s (effects enabled)");
        return false;
    }
    radioLoaded = idx;
    return true;
}

static void radioStartLoaded(void) {
    RadioStream_play();
}

static void radioPlayOne(void) {
    int idx = radioPickTrack();
    if (!radioLoadTrack(idx)) return;
    radioStartLoaded();
}

void Snd_radioPrepare(void) {
    if (radioPrepared || !Snd_ok || !Snd_on || !radioCanLoad()) return;
    int idx = radioPickTrack();
    if (radioLoadTrack(idx)) radioPrepared = true;
}

void Snd_radioStart(void) {
    radioPlaying = true;
    radioRetryMs = 0;
    if (!Snd_ok || !Snd_on) return;
    if (!radioPrepared || !RadioStream_active()) {
        int idx = radioPickTrack();
        if (!radioLoadTrack(idx)) return;
    }
    radioPrepared = false;
    radioStartLoaded();
}

void Snd_radioStop(void) {
    radioPlaying = false;
    radioRetryMs = 0;
    RadioStream_cancel();
    radioPrepared = false;
    radioLoaded = -1;
}

/* Called once per game frame while driving.  When a clip reaches its end a
   new random clip/offset starts, so long drives keep the station playing. */
void Snd_radioTick(int dtMs) {
    if (!radioPlaying || !Snd_ok || !Snd_on || !radioCanLoad()) return;
    if (Snd_radioActive()) return;
    if (radioRetryMs > 0) {
        radioRetryMs -= dtMs;
        return;
    }
    radioPlayOne();
    if (!Snd_radioActive()) radioRetryMs = 1000;
}

bool Snd_radioActive(void) {
    return radioPlaying && RadioStream_active();
}

void Snd_stop(int id) {
    snd_lock_acquire();
    for (int i = 0; i < SND_MAX_MIX; i++) {
        if (mixv[i].active && mixv[i].tag == id) mixv[i].active = false;
    }
    snd_lock_release();
}

void Snd_stopAll(void) {
    Snd_radioStop();
    snd_lock_acquire();
    for (int i = 0; i < SND_MAX_MIX; i++) mixv[i].active = false;
    engineState = -1;
    wantEngine = -1;
    sirenOn = false;
    snd_lock_release();
}

/* Mix into interleaved stereo s16 at the Retro-Go output rate.  All integer, no FPU. */
void Snd_mix(int16_t* out, int frames, int rate) {
    if (!out || frames <= 0) return;
    snd_lock_acquire();
    for (int i = 0; i < frames; i++) {
        int32_t acc = 0;
        for (int j = 0; j < SND_MAX_MIX; j++) {
            MixVoice* v = &mixv[j];
            if (!v->active) continue;
            const SndClip* c = v->clip;
            uint32_t idx = (uint32_t)(v->pos >> 16);
            if (idx >= (uint32_t)c->len) {
                if (v->loop) {
                    v->pos = 0;
                    idx = 0;
                } else {
                    v->active = false;
                    continue;
                }
            }
            int32_t s0 = c->pcm[idx];
            int32_t s1 = (idx + 1 < (uint32_t)c->len) ? c->pcm[idx + 1] :
                         (v->loop ? c->pcm[0] : s0);
            int32_t frac = (int32_t)(v->pos & 0xFFFF);
            int32_t s = s0 + ((((s1 - s0) >> 8) * (frac >> 8)) >> 8);
            int env = v->env;
            if (!v->loop) {
                uint64_t left = v->end > v->pos ? v->end - v->pos : 0;
                uint32_t fade = (uint32_t)v->step * 96u;
                if (left < (uint64_t)fade) env = (int)((uint32_t)left * 256u / (fade ? fade : 1u));
            }
            acc += (s * ((v->vol * env) >> 8)) >> 8;
            if (v->env < 256) v->env += 8;

            v->pos += v->step;
            if (v->pos >= v->end) {
                if (v->loop) v->pos -= v->end;
                else v->active = false;
            }
        }
        if (acc > 32767) acc = 32767;
        else if (acc < -32768) acc = -32768;
        out[i * 2] = (int16_t)acc;
        out[i * 2 + 1] = (int16_t)acc;
    }
    snd_lock_release();
    RadioStream_mix(out, frames, rate);
}

void Snd_cleanup(void) {
    RadioStream_close();
    snd_lock_acquire();
    for (int i = 0; i < SND_MAX_MIX; i++) mixv[i].active = false;
    engineState = -1;
    wantEngine = -1;
    sirenOn = false;
    for (int i = 0; i < SND_N; i++) clipFree(&clips[i]);
    for (int k = 0; k < 5; k++) {
        for (int i = 0; i < 3; i++) {
            clipFree(&voiceM[k][i]);
            clipFree(&voiceF[k][i]);
        }
    }
    radioLoaded = -1;
    radioPlaying = false;
    radioPrepared = false;
    radioRetryMs = 0;
    radioTracks = 0;
    radioDir[0] = 0;
    radioRetryAfterUs = 0;
    Snd_ok = false;
    snd_lock_release();
}
