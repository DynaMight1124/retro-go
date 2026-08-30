// sfx.c
// ============================================================================
// Engine-TU audio mixer and synth driver.
//
// Compiled inside engine.cpp's TU (engine.cpp text-includes this file and
// itself is compiled as C++). The `struct Channel` layout is owned by sfx.h
// (see that header for the C-vs-C++ split on the `phi` accumulator field);
// this file just uses the struct and does NOT redefine it.
// ============================================================================

#include "sfx.h"
#include "data.h"        // SFX, Note, SAMPLE_RATE, SAMPLES_PER_DURATION, NOTES_PER_SFX
#include "lua/fix32.h"   // z8::fix32 — C++ type, requires C++ compilation
#include <climits>       // USHRT_MAX — used in uint16_t wrap-around guard
#include <stdint.h>      // int32_t — disambiguates fix32 ctor calls (see below)

// (struct Channel is fully defined in sfx.h with conditional `phi` type —
//  z8::fix32 in C++, int32_t in C, same 4-byte layout either way.)

const z8::fix32 VOL_NORMALIZER = 32767.99f / 7.0f;

static SFX sfx[64];
static MusicPattern music_patterns[64];
struct Channel channels[4];

// A few legacy carts contain populated speed-zero effects. Keep one tiny
// channel-local copy for direct sfx() playback so those can use speed 1
// without rewriting the cartridge's shared SFX table. The distinction is
// important for carts such as Snekburd, which use speed-zero slots as nested
// custom-instrument data rather than as ordinary effects.
static SFX direct_zero_speed_sfx[4];

static volatile int music_pending_pattern = -2;
static int music_pattern = -1;
// music()'s mask reserves channels from automatic sfx() allocation. It does
// not select which pattern voices play; every enabled pattern channel plays.
// PICO-8's omitted-mask default is zero (no reserved channels).
static volatile int music_channel_mask = 0;
static uint32_t music_elapsed = 0;
static uint32_t music_duration = 0;
static uint32_t music_patterns_played = 0;

// PICO-8 waveform 6 is pitch-shaped noise. The inherited synth called
// rand() for every sample and returned unfiltered, positive-only white
// noise, producing a constant hiss. Keep independent state per channel and
// use a cheap one-pole filter: no libc RNG, locks, or divisions in the hot
// path. The polynomial approximates scale/(1+scale), where scale is the
// current pitch frequency relative to key 63.
static const z8::fix32 NOISE_INV_MAX_FREQ = z8::fix32(1.0f / 2489.0158698f);
static const z8::fix32 NOISE_INV_63 = z8::fix32(1.0f / 63.0f);
static const z8::fix32 NOISE_GAIN = z8::fix32(0.375f);

static inline void reset_channel_noise(struct Channel *c, uint32_t salt) {
    c->noise_sample = int32_t{0};
    c->noise_rng = 0x6d2b79f5u ^ (salt * 0x9e3779b9u);
    if (c->noise_rng == 0) c->noise_rng = 1;
}

static inline void reset_channel_instrument(struct Channel *c) {
    c->instrument_offset = 0;
    c->instrument_id = 0xff;
    c->instrument_parent_key = 0xff;
}

static inline z8::fix32 filtered_noise_waveform(struct Channel *c,
                                                z8::fix32 effective_freq,
                                                uint8_t pitch) {
    uint32_t x = c->noise_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    c->noise_rng = x;

    // A signed upper halfword mapped directly to Q16.16 yields [-1, 1).
    z8::fix32 random = z8::fix32::frombits(
        (int32_t)(int16_t)(x >> 16) * 2);
    z8::fix32 scale = effective_freq * NOISE_INV_MAX_FREQ;
    if (scale > ONE) scale = ONE;
    z8::fix32 alpha = scale * (ONE - HALF * scale);
    c->noise_sample += alpha * (random - c->noise_sample);

    uint8_t key = pitch & 0x3f;
    z8::fix32 darkness = z8::fix32(int32_t{63 - key}) * NOISE_INV_63;
    return c->noise_sample * NOISE_GAIN * (ONE + darkness * darkness);
}

// SAMPLES_PER_DURATION (183) and NOTES_PER_SFX (32) are forward-declared in
// data.h as `extern const`. Define them here with matching external linkage
// (the explicit `extern` keyword forces external linkage even though a
// file-scope `const` in C++ defaults to internal). Without this, the engine
// TU's only definition would not satisfy the header's `extern` lookups from
// any other TU that picks them up transitively via pico8_globals.h.
extern const uint8_t SAMPLES_PER_DURATION = 183;
extern const uint8_t NOTES_PER_SFX        = 32;

// this is can fit an SFX of duration 1;
// so filling this buffer $duration times will play an entire SFX
// it could be anywhere from 1x to 32x
// at 1x, this buffers 5.46ms of data
// and at 32x; 174ms
// 4x sounds reasonable; 21.8ms             (1464 bytes)
// 6x is 32.76 ~ 1 frame (33ms)             (2196 bytes)
// 8x is 43.6ms which is fairly noticeable  (2928 bytes)
// --------
// this total should not exceed 4092 bytes; which is the max supported by ESP32
// which means choices are between 4 and 8
const uint8_t SAMPLES_PER_BUFFER = 6;
static int32_t audiobuf[SAMPLES_PER_DURATION * SAMPLES_PER_BUFFER];

static void music_begin_pattern(int pattern) {
    if (pattern < 0 || pattern >= 64) {
        music_pattern = -1;
        music_channel_mask = 0;
        for (int c = 0; c < 4; ++c) {
            if (channels[c].is_music) {
                channels[c].sfx = NULL;
                channels[c].is_music = 0;
            }
        }
        return;
    }

    music_pattern = pattern;
    music_patterns_played++;
    MusicPattern *p = &music_patterns[pattern];
    music_elapsed = 0;
    music_duration = 0;
    uint32_t longest_loop_duration = 0;

    for (int c = 0; c < 4; ++c) {
        uint8_t id = p->channels[c];
        if (id >= 64 || sfx[id].duration == 0) {
            if (channels[c].is_music) {
                channels[c].sfx = NULL;
                channels[c].is_music = 0;
            }
            continue;
        }
        // A user sfx() explicitly placed on this channel takes priority
        // until it finishes. The next music pattern can reclaim it.
        if (channels[c].sfx != NULL && !channels[c].is_music) continue;
        channels[c].sfx = &sfx[id];
        channels[c].sfx_id = id;
        channels[c].offset = 0;
        channels[c].end_note = NOTES_PER_SFX;
        channels[c].is_music = 1;
        channels[c].loop_released = 0;
        channels[c].phi = int32_t{0};
        reset_channel_instrument(&channels[c]);
        reset_channel_noise(&channels[c], ((uint32_t)pattern << 8) | id | c);
        channels[c].prev_key = sfx[id].notes[0].key;
        channels[c].prev_vol = sfx[id].notes[0].volume;
        // PICO-8 ends a music pattern when the left-most non-looping channel
        // finishes. A loop-start with loop-end zero is instead a shortened
        // track length, used for time signatures with fewer than 32 rows.
        bool looping = sfx[id].loop_start < sfx[id].loop_end;
        if (looping) {
            // If every active channel loops, the pattern still lasts for a
            // complete 32-note pass at the slowest/longest active channel's
            // speed before its pattern flags are evaluated. The previous
            // fallback omitted `duration`, making Celeste 2 pattern 38
            // restart eleven times too quickly (5856 vs 64416 samples).
            uint32_t duration = (uint32_t)SAMPLES_PER_DURATION
                              * NOTES_PER_SFX * sfx[id].duration;
            if (duration > longest_loop_duration)
                longest_loop_duration = duration;
        } else if (music_duration == 0) {
            uint32_t notes = (sfx[id].loop_start > 0 && sfx[id].loop_end == 0)
                           ? sfx[id].loop_start : NOTES_PER_SFX;
            channels[c].end_note = (uint8_t)notes;
            music_duration = (uint32_t)SAMPLES_PER_DURATION * notes
                           * sfx[id].duration;
        }
    }
    if (music_duration == 0) {
        music_duration = longest_loop_duration;
        if (music_duration == 0)
            music_duration = (uint32_t)SAMPLES_PER_DURATION * NOTES_PER_SFX;
    }
}

void music_request(int pattern, int fade_ms, int channel_mask) {
    (void)fade_ms;
    // Publish the reservation immediately so an sfx() later in this Lua tick
    // cannot steal a newly reserved channel before Core 1 consumes PATTERN.
    music_channel_mask = pattern >= 0 ? (channel_mask & 0x0f) : 0;
    music_pending_pattern = pattern;
}

void music_reset(void) {
    music_pending_pattern = -2;
    music_pattern = -1;
    music_channel_mask = 0;
    music_elapsed = music_duration = 0;
    music_patterns_played = 0;
}

void music_update(uint16_t samples) {
    int pending = music_pending_pattern;
    if (pending != -2) {
        music_pending_pattern = -2;
        music_begin_pattern(pending);
    }
    if (music_pattern < 0) return;

    music_elapsed += samples;
    if (music_elapsed < music_duration) return;

    MusicPattern *finished = &music_patterns[music_pattern];
    if (finished->flags & 4) music_begin_pattern(-1);
    else if (finished->flags & 2) {
        // PICO-8 resolves a loop-back from the pattern table, not from
        // playback history. This matters when music() begins in the middle
        // of a loop: Snekburd deliberately starts at rnd(9), so the marked
        // loop-start pattern may not have played during the first pass.
        // Search backward to the closest marker and fall back to pattern 0
        // when none exists, matching the console/reference sequencers.
        int loop_pattern = music_pattern;
        bool found_loop_start = false;
        while (loop_pattern > 0) {
            --loop_pattern;
            if (music_patterns[loop_pattern].flags & 1) {
                found_loop_start = true;
                break;
            }
        }
        if (!found_loop_start) loop_pattern = 0;
        music_begin_pattern(loop_pattern);
    } else music_begin_pattern((music_pattern + 1) & 63);
}

void fill_buffer(int32_t* buf, struct Channel* c, uint16_t samples) {
    // Guard against an uninitialized `buf` from the audio-task backend.
    // Symptom: Core 1 StoreProhibited with EXCVADDR=0xcccccccc (the
    // Xtensa uninitialized-fill pattern) on the first `sfx()` call from
    // Lua after cart init. The previous NULL-only guard on `_sfx` was
    // insufficient because once Lua calls _lua_sfx() and assigns
    // channels[i].sfx, the loop body starts dereferencing `buf` for
    // sample writes; if the backend DMA buffer hadn't been wired up
    // yet, that pointer is still 0xcccccccc. Bail without touching
    // memory to keep the synth task alive and let audio resume once
    // the backend provides a valid buffer.
    if (buf == NULL || (uintptr_t)buf == 0xCCCCCCCCUL) {
        return;
    }
    SFX* _sfx = c->sfx;
    if (_sfx == NULL) {
        return;
    }

    // Guard against zero-duration SFX (can occur in PNG-decoded carts where
    // the SFX data hasn't been fully populated). A zero duration causes
    // IntegerDivideByZero on Core 1 at `c->offset / (SPD * duration)`.
    if (_sfx->duration == 0) {
        c->sfx    = NULL;
        c->sfx_id = 0;
        c->offset = 0;
        c->is_music = 0;
        c->loop_released = 0;
        c->phi    = int32_t{0};
        reset_channel_instrument(c);
        return;
    }

    // buffer sizes are always multiples of SAMPLES_PER_DURATION
    // which ensures the notes will always play _entire_ "duration" blocks

    // FX_SLIDE needs the previous note's key+volume. We capture them
    // into c->prev_* when note_id changes between outer iterations.
    // The UINT16_MAX sentinel on the first iteration suppresses capture
    // until at least one note has actually played -- so the first note's
    // FX_SLIDE reads whatever _lua_sfx / engine_init last wrote for
    // c->prev_*, which (per those init paths) is notes[0]'s own key/vol,
    // making FX_SLIDE on the first note a self-slide rather than a
    // phantom slide-from-zero.
    uint16_t last_note_id = UINT16_MAX;

    for (uint16_t s = 0; s < samples; s++) {
        uint16_t note_id = c->offset / (SAMPLES_PER_DURATION * _sfx->duration);
        if (!c->loop_released && _sfx->loop_start < _sfx->loop_end
            && note_id >= _sfx->loop_end) {
            c->offset = (uint32_t)_sfx->loop_start * SAMPLES_PER_DURATION
                      * _sfx->duration;
            note_id = _sfx->loop_start;
            last_note_id = UINT16_MAX;
        }
        if (note_id >= c->end_note || note_id >= NOTES_PER_SFX) {
            c->sfx = NULL;
            c->sfx_id = 0;
            c->offset = 0;
            c->is_music = 0;
            c->loop_released = 0;
            c->phi = int32_t{0};
            reset_channel_instrument(c);
            break;
        }
        if (note_id != last_note_id && last_note_id != UINT16_MAX) {
            c->prev_key = _sfx->notes[last_note_id].key;
            c->prev_vol = _sfx->notes[last_note_id].volume;
        }
        last_note_id = note_id;

        Note n = _sfx->notes[note_id];
        z8::fix32 freq = key_to_freq[n.key];
        // fix32 has no operator/(int32_t); cast the int divisor explicitly.
        const z8::fix32 delta = freq / z8::fix32(int32_t{SAMPLE_RATE});

        c->offset += SAMPLES_PER_DURATION;
        if (n.volume == 0) {
            c->phi += SAMPLES_PER_DURATION * delta;
            s += SAMPLES_PER_DURATION - 1;
            continue;
        }
        // Per-sample apply_fx() -- it scales *volume (FADE) and tweaks
        // *freq (VIBRATO/ARP) per individual sample. This means every
        // effect ramps smoothly with no tick-block-edge clicks, AND
        // VIBRATO can apply a continuous 7.5 Hz sine wobble rather than
        // a stepwise approximation. apply_fx expects the base (raw)
        // n.volume && freq as inputs -- we re-seed both at the top of
        // every iteration so slider effects don't compound across samples.
        const uint16_t n_waveform = n.waveform; // alias for memory access?
        const bool custom_instrument = (n_waveform & 8u) != 0;

        // Waveforms 8..15 are PICO-8 SFX instruments 0..7. They are
        // retriggered when the parent pitch changes, after a silent parent
        // note, or when effect 3 explicitly requests retriggering. Otherwise
        // their envelope/loop continues across parent-note boundaries.
        if (custom_instrument) {
            const uint8_t instrument_id = (uint8_t)(n_waveform & 7u);
            const bool parent_note_start =
                c->offset == (uint32_t)note_id * SAMPLES_PER_DURATION
                           * _sfx->duration + SAMPLES_PER_DURATION;
            uint8_t previous_volume = 0;
            if (note_id > 0) previous_volume = _sfx->notes[note_id - 1].volume;
            if (c->instrument_id != instrument_id ||
                c->instrument_parent_key != n.key ||
                (parent_note_start && previous_volume == 0) ||
                (parent_note_start && n.effect == FX_DROP)) {
                c->instrument_offset = 0;
                c->instrument_id = instrument_id;
                c->instrument_parent_key = n.key;
                c->phi = int32_t{0};
            }
        } else {
            reset_channel_instrument(c);
        }

        for (uint16_t _s = 0; _s < SAMPLES_PER_DURATION; _s++) {
            z8::fix32 volume = n.volume;
            z8::fix32 effective_freq = freq;
            // Per-sample in-note offset:
            //   c->offset was advanced by SAMPLES_PER_DURATION at sfx.c:64,
            //   so subtract SPD to rewind to the START of the current
            //   tick-block, add the inner sample index _s, then subtract
            //   the note base for a clean per-note ramp domain [0,
            //   duration*SAMPLES_PER_DURATION).
            uint16_t in_note_offset = (uint16_t)(
                c->offset - (uint32_t)SAMPLES_PER_DURATION + (uint32_t)_s
                - (uint32_t)note_id * (uint32_t)SAMPLES_PER_DURATION * (uint32_t)_sfx->duration);
            // speed = ticks/note == _sfx->duration per PICO-8 naming;
            // FX_ARP uses it to halve its arpeggio rate when speed<=8.
            apply_fx(_sfx, &n, &volume, &effective_freq, in_note_offset,
                     _sfx->duration, note_id, c->prev_key, c->prev_vol);

            uint8_t output_waveform = (uint8_t)n_waveform;
            uint8_t output_pitch = n.key;
            if (custom_instrument) {
                SFX *instrument = &sfx[c->instrument_id];
                if (instrument->duration == 0) {
                    ++c->instrument_offset;
                    continue;
                }

                const uint32_t instrument_note_samples =
                    (uint32_t)SAMPLES_PER_DURATION * instrument->duration;
                uint16_t instrument_note =
                    (uint16_t)(c->instrument_offset / instrument_note_samples);
                if (instrument->loop_start < instrument->loop_end &&
                    instrument_note >= instrument->loop_end) {
                    c->instrument_offset =
                        (uint32_t)instrument->loop_start * instrument_note_samples;
                    instrument_note = instrument->loop_start;
                }
                if (instrument_note >= NOTES_PER_SFX) {
                    ++c->instrument_offset;
                    continue;
                }

                Note instrument_note_data = instrument->notes[instrument_note];
                uint16_t instrument_in_note = (uint16_t)(
                    c->instrument_offset -
                    (uint32_t)instrument_note * instrument_note_samples);
                uint8_t previous_instrument_note = instrument_note > 0
                    ? (uint8_t)(instrument_note - 1) : (uint8_t)instrument_note;
                z8::fix32 instrument_volume = instrument_note_data.volume;
                z8::fix32 instrument_freq =
                    key_to_freq[instrument_note_data.key & 0x3f];
                apply_fx(instrument, &instrument_note_data,
                         &instrument_volume, &instrument_freq,
                         instrument_in_note, instrument->duration,
                         instrument_note,
                         instrument->notes[previous_instrument_note].key,
                         instrument->notes[previous_instrument_note].volume);

                // SFX-instrument pitches are offsets from C2 (key 24), and
                // their volume multiplies the parent note. Poom's instruments
                // use key 24, so the common path avoids an extra fixed-point
                // division in every generated sample.
                if (instrument_note_data.key != 24)
                    effective_freq *= instrument_freq / key_to_freq[24];
                volume *= instrument_volume / z8::fix32(int32_t{7});
                output_waveform = instrument_note_data.waveform & 7u;
                output_pitch = instrument_note_data.key;
                ++c->instrument_offset;
            }

            const z8::fix32 norm_vol = VOL_NORMALIZER * volume;
            const z8::fix32 w = output_waveform == INST_NOISE
                ? filtered_noise_waveform(c, effective_freq, output_pitch)
                : waveform(output_waveform, c->phi);
            const int16_t  sample = (int16_t)(norm_vol * w);
            uint16_t _offset = (_s + s);

            // NOTE: this is += so that all sfx can be played in parallel
            buf[_offset] += sample;

            // Recompute phi step from effective_freq per sample so
            // FX_VIBRATO and FX_ARP modulate the phase rate in lockstep.
            // The pre-stored `delta` was derived from raw freq at the top
            // of the iteration and would not reflect per-sample freq
            // changes; per-sample division is ~4 fix32 ops x 183 samples
            // x 4 channels x ~60 ticks/sec ~= 700k ops/sec on ESP32 Core 1.
            c->phi += effective_freq / z8::fix32(int32_t{SAMPLE_RATE});
        }

        s += SAMPLES_PER_DURATION - 1;
    }

    if (c->sfx != NULL &&
        (_sfx->loop_start >= _sfx->loop_end || c->loop_released) &&
        c->offset >= ((uint32_t)SAMPLES_PER_DURATION * c->end_note
                   * _sfx->duration)) {
        c->sfx    = NULL;
        c->sfx_id = 0;
        c->offset = 0;
        c->is_music = 0;
        c->loop_released = 0;
        // `phi` is `z8::fix32`; force the literal through int32_t to pick the
        // single fix32(int32_t) ctor unambiguously in C++.
        c->phi    = int32_t{0};
        reset_channel_instrument(c);
    }
}
