# PICO-8 Save-State Status

## Current status and decision

Retro-Go save slots are intentionally unsupported by this port. The shared
Save and Load actions display an explanatory message and return failure. Cart
`cartdata()` persistence remains supported and is flushed before menus,
cartridge transitions, reset, sleep, and shutdown.

This is preferable to a partial state file that appears to work but silently
restores an inconsistent game.

## Why machine RAM is insufficient

The native state includes 64 KB of PICO-8 RAM, drawing registers and palettes,
the PRNG, audio channels, music playback, input repeat state, multicart path and
breadcrumb data, and host timing. Those values can be serialized explicitly.

The authoritative game state, however, lives in Lua 5.2. Typical carts store
entities, level state, timers, and object methods in top-level `local` values.
Those become closure upvalues and are not reachable by copying `_ENV` or PICO-8
RAM. Other carts use shared or cyclic tables, dynamically created closures,
registry-held pause-menu callbacks, and coroutines with suspended stacks.

`lua_dump()` serializes a function prototype, not a running Lua heap. Saving
allocator memory verbatim would persist absolute pointers and cannot survive a
cold boot or a different allocation layout.

## Requirements for a complete implementation

A future state format must be versioned, length checked, written through the
filename supplied by Retro-Go, and bound to the launcher ROM plus the currently
active companion cart. At minimum it needs:

1. A stable ID graph for Lua strings, tables, Lua closures, shared upvalues,
   userdata with explicitly supported serializers, and threads.
2. Preservation of table keys, metatables, cycles, shared references, closure
   prototypes, upvalue identity, coroutine stacks, program counters, and
   protected-call/continuation state.
3. Reconstruction of native C functions by registered symbolic ID rather than
   persisted addresses.
4. Explicit native chunks for RAM, draw state, RNG, audio/music, input repeat,
   frame cadence, cart parameters, and multicart breadcrumbs.
5. A cart-content fingerprint, format version, per-chunk sizes, and checksum so
   incompatible or truncated files fail without modifying the running VM.
6. Transactional load: construct and validate a replacement VM first, then
   swap it in only after every chunk succeeds.

Acceptance testing must cover cold-boot resume, all save slots, corrupted and
wrong-cart files, repeated save/load cycles, multicarts, closure-local state,
cyclic tables, active music/SFX, coroutines, screenshots, and original ESP32
memory limits.

Until that serializer exists, the correct Retro-Go contract is to report the
operation as unsupported rather than claim success.

## Fake-08 reference implementation

This materially changes the feasibility assessment: Fake-08 demonstrates working, versioned
save states for another PICO-8-compatible engine using a related z8lua VM.
Save states are therefore credible future work, not something that must be
designed entirely from scratch.

Fake-08 contains the difficult Lua-heap groundwork:

- `fake-08/components/fake-08/libs/z8lua/eris.c` and `eris.h` provide Eris,
  a Lua 5.2 persistence extension.
- Supporting Lua changes register Eris and expose the VM internals it needs;
  copying only `eris.c` is not sufficient.
- `p8GlobalLuaFunctions.h` constructs permanent-value mappings for native
  globals and serializes the game-created global graph.
- `Vm::serializeLuaState()` and `Vm::deserializeLuaState()` in `vm.cpp`
  provide the engine-facing wrapper.
- `fake-08/main/main.cpp` demonstrates the Retro-Go state container,
  validation, cold-start restoration, and deferred in-session loading.

Eris can preserve graphs, cycles, Lua closures, shared upvalues, prototypes,
references, and suspended threads. This addresses the main reason a RAM-only
PicoPico state would be incomplete.

Fake-08 remains a reference, not a drop-in patch. PicoPico has its own z8lua
fork and extensive compatibility modifications. Eris reaches into VM internals
and requires stable permanent mappings for C functions, so the complete patch
must be reconciled with this exact Lua tree. Replacing the Lua directory
wholesale would risk losing existing compatibility work.

## Additional PicoPico state required

An Eris snapshot is necessary but not sufficient. A complete implementation
must also preserve:

- complete PICO-8 RAM and correctness-sensitive expanded sprite/map data;
- palette, transparency, camera, clip, fill-pattern, font, and register state;
- PRNG state and emulated frame/timing counters;
- music position, SFX channels, oscillator/effect/noise state, and audio timing;
- current/previous input and button-repeat counters;
- launch ROM, resolved active cart, cart parameters, and multicart breadcrumbs;
- pending cart transitions and state affecting the next emulated tick.

Derived caches should normally be rebuilt. Retro-Go surfaces, tasks, locks,
file handles, and raw pointers must never be persisted.

## File format and load lifecycle

The container needs a magic value and version, engine compatibility data, cart
fingerprints, independently sized chunks, strict bounds checks, and a checksum.
Unknown, truncated, corrupt, oversized, old-version, or wrong-cart states must
be rejected before changing the running game.

Fake-08 defers an in-session load until its game loop unwinds and then recreates
the VM; Eris cannot safely replace closures while the old VM remains on the C
stack. PicoPico should use the same lifecycle and validate replacement Lua and
native state before committing the load.

## Recommended future plan

1. Diff Fake-08's z8lua/Eris changes against PicoPico's Lua fork.
2. Add Eris under a development guard without exposing release Save/Load.
3. Prove bounded snapshots with upvalues, cycles, closures, and a coroutine.
4. Add a versioned container and explicit native-state chunks.
5. Implement cold-boot restore before in-session restore.
6. Add deferred VM recreation based on Fake-08.
7. Add multicart identity and breadcrumb restoration.
8. Measure peak allocation, largest free block, state size, and latency on the
   original ESP32.
9. Expose Save/Load only after representative carts and failure cases pass.

Each stage should remain reversible and preserve existing compatibility fixes.
Testing should include Celeste, Snekburd, Pigments, a multicart, supported cart
formats, active audio, reset, screenshots, speed changes, sleep, and
`cartdata()`. Test the original ESP32 before S3 and P4.

A restore succeeds only if the next frame, simulation tick, and audio block
continue coherently; reopening approximately the right screen is insufficient.

## Present conclusion

Fake-08 greatly reduces architectural uncertainty, but this remains a
substantial, memory-sensitive VM integration with broad regression risk.
Deferring it while retaining explicit **Save states unavailable** behaviour is
the appropriate release decision.
