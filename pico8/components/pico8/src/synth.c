#include "synth.h"
#include <stdint.h>     // int32_t — used to disambiguate fix32 ctor calls below

const z8::fix32 QUARTER = z8::fix32(0.25f);
const z8::fix32 THIRD   = z8::fix32(0.3333f);
const z8::fix32 HALF    = z8::fix32(0.5f);
// NOTE: z8::fix32 has 8 separate integer ctors (int8_t..uint64_t), so calling
// e.g. z8::fix32(1) with an `int` literal is ambiguous in C++. The original
// code worked under C, but engine.cpp now text-includes synth.c as C++. We
// force-typed the literal through `int32_t{...}` which selects the
// `fix32(int32_t)` ctor unambiguously.
const z8::fix32 ONE     = z8::fix32(int32_t{1});
const z8::fix32 TWO     = z8::fix32(int32_t{2});
const z8::fix32 THREE   = z8::fix32(int32_t{3});
const z8::fix32 FOUR    = z8::fix32(int32_t{4});
const z8::fix32 SIX     = z8::fix32(int32_t{6});

const z8::fix32 SAW_FACTOR  = z8::fix32(0.653f);

z8::fix32 waveform(int instrument, z8::fix32 advance)
{
    z8::fix32 t = z8::fix32::decimals(advance);
    // Initialise `ret` as int32_t{0} so the implicit conversion to z8::fix32
    // is unambiguous (single fix32(int32_t) candidate).
    z8::fix32 ret = int32_t{0};

    // Multipliers were measured from PICO-8 WAV exports. Waveforms are
    // inferred from those exports by guessing what the original formulas
    // could be.
    switch (instrument)
    {
        case INST_TRIANGLE:
            return (z8::fix32::abs(z8::fix32::fast_shl(t, 2) - TWO) - ONE) >> 1;
            //return z8::fix32::fast_shr(z8::fix32::abs(z8::fix32::fast_shl(t, 2) - TWO) - ONE, 1);
        case INST_TILTED_SAW:
        {
            static z8::fix32 const a = 0.9f;
            ret = t < a ? 2 * t / a - ONE
                        : 2 * (ONE - t) / (ONE - a) - ONE;
            return ret >> 1;
        }
        case INST_SAW:
            return SAW_FACTOR * (t < HALF ? t : t - ONE);
        case INST_SQUARE:
            return t < HALF ? QUARTER : -QUARTER;
        case INST_PULSE:
            return t < THIRD ? QUARTER : -QUARTER;
        case INST_ORGAN:
            ret = t < HALF ? THREE  - z8::fix32::abs(24 * t - SIX)
                           : ONE    - z8::fix32::abs(16 * t - 12);
            // fix32 has no operator/(int32_t); cast the divisor explicitly.
            return ret / z8::fix32(int32_t{9});
        case INST_NOISE:
#if 0
        {
            // Spectral analysis indicates this is some kind of brown noise,
            // but losing almost 10dB per octave. I thought using Perlin noise
            // would be fun, but it’s definitely not accurate.
            //
            // This may help us create a correct filter:
            // http://www.firstpr.com.au/dsp/pink-noise/

            /*
            static lol::perlin_noise<1> noise;
            for (float m = 1.75f, d = 1.f; m <= 128; m *= 2.25f, d *= 0.75f)
                ret += d * noise.eval(lol::vec_t<float, 1>(m * advance));
            return ret * 0.4f;
            */
            // FIXME: this is now more broken )) it gives _some_ noise
            // but obviously the noise profile is terrible
            return (z8::fix32(int32_t{rand() >> 16})/z8::fix32(int32_t{RAND_MAX >> 17})) * THIRD;
        }
#else
            // Stateful, pitch-filtered waveform 6 is generated per channel
            // by filtered_noise_waveform() in sfx.c.
            return int32_t{0};
#endif
        case INST_PHASER:
        {   // This one has a subfrequency of freq/128 that appears
            // to modulate two signals using a triangle wave
            // FIXME: amplitude seems to be affected, too
            z8::fix32 k = z8::fix32::abs(TWO * z8::fix32::decimals(advance >> 7) - ONE);
            z8::fix32 u = z8::fix32::decimals(t + HALF * k);
            ret = z8::fix32::abs((u<<2) - TWO) - z8::fix32::abs((t<<3) - FOUR);
            return ret / SIX;
        }
    }

    return int32_t{0};
}

void apply_fx(SFX* s, Note* n, z8::fix32* volume, z8::fix32* freq, uint16_t offset, uint16_t speed, uint16_t note_id, uint8_t prev_key, uint8_t prev_vol) {
    switch(n->effect) {
        case FX_NO_EFFECT:
            return;        case FX_SLIDE:
            {
                // Per PICO-8 wiki: at offset=0 within this note, freq+vol
                // equal the previous note's; at offset=duration*SPD-1
                // (last sample of this note), they equal this note's.
                // Linear interp in between. Works for both pitch-only
                // slides (prev_key != n->key, prev_vol == n->volume) and
                // volume-only slides (the opposite) without branching.
                z8::fix32 t = z8::fix32(int32_t{offset}) /
                              z8::fix32(int32_t{s->duration * SAMPLES_PER_DURATION});
                if (t > ONE) t = ONE;
                // Lerp freq from prev_key to n->key.
                z8::fix32 prev_f = key_to_freq[prev_key & 0x3F];
                z8::fix32 cur_f  = key_to_freq[n->key  & 0x3F];
                *freq = prev_f + t * (cur_f - prev_f);
                // Lerp vol from prev_vol to n->volume. (0..7 stored in
                // Note.volume; we mask the side just in case.)
                z8::fix32 prev_v = z8::fix32(int32_t{prev_vol & 0x07});
                z8::fix32 cur_v  = z8::fix32(int32_t{n->volume & 0x07});
                *volume = prev_v + t * (cur_v - prev_v);
                break;
            }
        case FX_VIBRATO:
            {
                // Per PICO-8 wiki: half-semitone freq wobble at 7.5 Hz.
                // amplitude ~ sqrt(2^(1/12))-1 ~= 0.05946, sin argument
                // is 2*pi*7.5*offset / SAMPLE_RATE because offset is in
                // samples. `speed` is unused -- vibrato tempo is fixed.
                const z8::fix32 amp = z8::fix32(int32_t{1}) * z8::fix32(0.05946f);
                const z8::fix32 t   = z8::fix32::sin(
                    TWO * z8::fix32(3.14159265f) * z8::fix32(7.5f) *
                    z8::fix32(int32_t{offset}) /
                    z8::fix32(int32_t{SAMPLE_RATE}));
                *freq *= ONE + amp * t;
                break;
            }
        case FX_DROP:
            // TODO stub -- kept as a no-op. The original body was
            //   *freq *= 1.f - fmod(offset, 1.f);
            // which was permanently a no-op since uint16_t % 1.f = 0
            // -- FX_DROP never actually dropped anything before this
            // commit. Replace with the spec's multiplicative pitch decay
            // over note duration when the engine needs it.
            break;
        case FX_FADE_IN:
            // offset is per-note remainder (set by sfx.c::fill_buffer) so the
            // coefficient ramps 0 -> 1 across the note's lifetime. z8::fix32 needs
            // explicit fix32 operands; the int32_t{} cast disambiguates the 8
            // fix32 integer ctors that this file's top comment warns about.
            *volume *= z8::fix32(int32_t{offset}) /
                       z8::fix32(int32_t{s->duration * SAMPLES_PER_DURATION});
            break;
        case FX_FADE_OUT:
            *volume *= ONE -
                       z8::fix32(int32_t{offset}) /
                       z8::fix32(int32_t{s->duration * SAMPLES_PER_DURATION});
            break;
        case FX_ARP_FAST:
        case FX_ARP_SLOW:
            {
                // Per PICO-8 wiki: arpeggio cycles groups of 4 contiguous
                // notes (note_id & ~3) at speed 4 ticks (FAST) or 8 ticks
                // (SLOW); halved to 2 / 4 when sfx.speed (== s->duration ==
                // ticks/note == `speed` here) <= 8. arp_idx picks which
                // note in the group is currently sounding -- CEIL-based, so
                // note 0 plays for arp_speed ticks before advancing.
                // `offset` here is in SAMPLES_PER_DURATION-tick units (per
                // sfx.c::fill_buffer's per-sample caller).
                uint16_t ticks     = (uint16_t)(offset / (uint32_t)SAMPLES_PER_DURATION);
                uint8_t  arp_speed = (speed <= 8)
                                     ? (uint8_t)((n->effect == FX_ARP_FAST) ? 2 : 4)
                                     : (uint8_t)((n->effect == FX_ARP_FAST) ? 4 : 8);
                uint8_t  arp_idx     = (uint8_t)((ticks / arp_speed) & 3);
                uint8_t  arp_note_id = (uint8_t)((note_id & ~(uint8_t)3) | arp_idx);
                *freq = key_to_freq[s->notes[arp_note_id].key];
                break;
            }
    }
}
