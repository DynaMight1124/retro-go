// sfx.h
// ============================================================================
// Public surface of sfx.c.
//
// Exposes the four PICO-8 audio channels (declared here as
// `extern struct Channel channels[4]` together with their full layout) and
// the fill_buffer() entry point that retro_go_backend.c's audio task calls.
//
// Why the struct is fully visible here (not just forward-declared opaque):
// the audio task in retro_go_backend.c (a C TU) takes `&channels[i]` and the
// C language does not allow arrays of incomplete struct types, so a bare
// `struct Channel;` forward-declared in front of `extern struct Channel
// channels[4];` would fail to compile with "array type has incomplete
// element type 'struct Channel'". Hiding the layout behind a `void*`
// would force callers to do pointer-casts over a raw byte buffer, which is
// more brittle than the symmetry we get here.
//
// The struct's `phi` accumulator field is `z8::fix32` in C++ (type-safe
// operators +=, abs, etc.) and `int32_t` in C (because the C TU doesn't see
// `z8`). Both occupy exactly 4 bytes (`z8::fix32` is a single `int32_t`
// `m_bits` plus a couple of static methods), so the runtime memory layout
// is identical across TUs — the engine TU interprets `phi` as `fix32`,
// the backend TU only ever takes the address and forwards it to fill_buffer,
// which doesn't touch the field from C.
// ============================================================================

#pragma once

#include <stdint.h>
#include "data.h"           // SFX type (used by the `sfx` pointer field)

#ifdef __cplusplus
#include "lua/fix32.h"      // z8::fix32 — C++-only; C TU doesn't see it
#endif

struct Channel {
    uint8_t  id;
    SFX*     sfx;
    uint8_t  sfx_id;
    uint32_t offset;        // in samples
    uint8_t  end_note;      // exclusive note limit (1..32)
    uint8_t  is_music;      // channel currently owned by music()
    uint8_t  loop_released; // sfx(-2): finish without looping again
#ifdef __cplusplus
    z8::fix32 phi;          // fractional-sample phase; type-safe operators
    z8::fix32 noise_sample; // filtered waveform-6 state
#else
    int32_t  phi;           // raw `z8::fix32::m_bits` storage; opaque to C
    int32_t  noise_sample;  // raw fixed-point filtered-noise state
#endif
    uint32_t noise_rng;     // per-channel xorshift state
    uint8_t  prev_key;      // FX_SLIDE source key (captured at note transition)
    uint8_t  prev_vol;      // FX_SLIDE source volume (captured at note transition)
    // Playback cursor for PICO-8 SFX instruments (note waveform 8..15).
    // Kept per output channel because an instrument can continue across
    // several parent notes without being retriggered.
    uint32_t instrument_offset;
    uint8_t  instrument_id;         // 0..7, or 0xff when inactive
    uint8_t  instrument_parent_key;
};

// Four PICO-8 audio channels. Mutated by:
//   - pico8api.c::_lua_sfx   (Lua thread, main)
//   - sfx.c::fill_buffer     (audio task, ~once per 5ms)
//
// The `channels` array and `fill_buffer()` symbol cross the engine TU
// (C++-compiled) and the retro_go_backend.c TU (C-compiled). Without this
// `extern "C"` wrapper, the C++ TU mangles `fill_buffer`'s reference while
// the C TU looks up an unmangled name, producing an "undefined reference"
// linker error. The struct Channel layout above is shared by both worlds —
// the `phi` field is a 4-byte type either way (z8::fix32 in C++, raw int32_t
// in C, same storage) — so only the linker-visible declarations below need
// the C-linkage guard.
#ifdef __cplusplus
extern "C" {
#endif
extern struct Channel channels[4];

// Render `samples` mono samples into `buf` (signed 32-bit accumulator) for one
// channel. The buffer is `+=` accumulated so multiple channels can be mixed
// into the same buffer per tick. buf must hold at least `samples` uint16_t
// entries.
void fill_buffer(int32_t *buf, struct Channel *c, uint16_t samples);
void music_request(int pattern, int fade_ms, int channel_mask);
void music_update(uint16_t samples);
void music_reset(void);
#ifdef __cplusplus
}
#endif
