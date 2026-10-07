# Experimental new_dynarec RV32 backend

Current status (September 2026): ESP32-P4 builds include Lightrec and the RV32
core. Emulator Options selects the CPU core for the next launch; Lightrec is
the default. Hold SELECT while launching the app to clear an RV32 preference
if the experimental core prevents the menu from opening. The RV32 core is not
yet suitable as the general-purpose PSX core: the real-BIOS Resident Evil
memory-card transaction completes under Lightrec but times out under RV32, and
broad game compatibility has not been established. An isolated RV32-only build
remains available with `-DPCSX_NDRC_RV32_FULL_CORE=ON`. Normal builds omit the
temporary SIO/card trace; `-DPCSX_PORT_PROFILE=ON` enables periodic frontend
and renderer diagnostics when needed. The sections below document historical
RV32 work, not current default behavior.

### Alias-tagged direct native dispatch

The direct main-RAM dispatch table previously stored a pointer to PSRAM cache
metadata. Every successful generated block transition therefore performed
three dependent PSRAM reads: the table slot, the cached guest PC, and the code
offset. The updated table stores the executable address itself. Because block
starts are 64-byte aligned, its low three bits carry guest PC bits 31:29; an
exact physical-word table index plus that tag distinguishes physical, KSEG0,
and KSEG1 RAM aliases without another metadata access. Generated dispatch now
loads the tagged pointer once, validates the alias, clears the tag, and jumps.

Source-page writes and DMA/HLE invalidations still clear every affected table
slot, and a cache-generation reset clears the complete table before executable
arena storage is reused. Metadata collision replacement no longer disables an
otherwise valid direct entry because its executable bytes remain owned until
that reset. Host debug/release tests and the P4 firmware build pass; comparable
hardware timing and extended gameplay remain the acceptance gate.

### Capacity-aware live code arena

The first alias-tagged hardware run was correct and improved Ridge Racer by a
repeatable 0.7--0.8 ms/tick (about 1.1--1.3%). It also exposed a different
bottleneck in Destruction Derby: entering 3D filled the 4 MiB compact-code arena
fourteen times in roughly thirty seconds, growing cache misses past 30,000 and
interpreter handoffs to nearly 28,000. Ridge Racer filled it only once over the
comparable test.

The app-local arena now attempts an 8 MiB PSRAM allocation and safely falls back
to 4 MiB when that contiguous block is unavailable. No target sdkconfig or
shared Retro-Go allocation policy changes are required. Generation-reset logs
include occupied arena bytes and metadata entries, allowing the next hardware
run to distinguish executable-byte exhaustion from metadata pressure and to
measure whether Destruction Derby's active code set fits in the larger arena.

The first firmware built for this comparison accidentally retained CMake's
cached `PCSX_NDRC_RV32_SELFTEST=OFF` and `PCSX_NDRC_RV32_LIVE_CANARY=OFF`
values. Its Lightrec-only performance is therefore a useful control, not an
8 MiB new_dynarec result. At that historical checkpoint both options were
enabled for P4; they are now off by default, as noted above.

### Opening-scene comparison capture

The corrected invalidation run restored loop 120 to 70 ms and loop 300 to
93 ms, versus the earlier 75/98 ms baseline. It remains about 10 FPS in the
opening compared with the user's roughly 30 FPS Lightrec reference.
Source inspection shows Lightrec's RV32 disassembler admits up to 64 guest
instructions (plus a required delay slot), and its optimizer supports local
branches, memory-map inference and a specific memset replacement. The RV32
harness admits at most a 16-instruction prefix and canonicalizes registers
before its separate branch adapter. These are opportunities, not measured
explanations of the complete performance gap.

The bounded capture found repeated calls from `8002dddc` to `8004e824`, returns
through `8004e954`, and a counter branch at `8002dde4`. One native invocation
crossed about 13,107 generated blocks in 5.7--6.3 ms. This confirms that short
call/return/branch blocks dominate at least one opening loop; simply raising the
16-instruction straight-prefix limit would not combine that control flow.

The capture is removed after collecting its evidence. The direct-table hit path
also no longer increments the global inline-block diagnostic on every edge or
loads/compares cache generation. Cache resets now clear all 2 MiB of direct
table slots and the compiled-page bitmap before arena metadata is reused, while
exact-PC validation still protects collision replacement. This removes ten
RV32 instructions from each main-RAM hit. General exit validation and progress
use guest cycle advancement, so `inline_blocks=0` is expected in subsequent
logs and does not mean that generated chaining stopped.

### Delay-slot MMIO target preservation

The longer gameplay capture stopped at PC `800121ac`, with eight cycles and
the correct JAL return address `800121b0`, instead of target `80047130`.
The slow memory path published the delay-slot instruction PC over the selected
branch target in canonical state. Runtime branch delay-slot helpers now save
that target in a 16-byte-aligned stack slot and restore it after the call,
while retaining any helper cycle adjustment. Direct LUT accesses are unchanged.
An ESP startup regression uses a private helper (no device access) to exercise
SB/SH/SW with taken/untaken BEQ, JAL and JR, checking the helper-visible PC,
address/value, final destination, link register and adjusted cycles.
Hardware confirmation through the first 3D race is still required.

The long capture also reports sampled lookup estimates above 100%; those
extrapolated percentages are not trustworthy performance shares and must not
be used to justify an optimization until timer/sampling overhead is addressed.

### Page-generation source validation

The first complete 3D run remained native but was far slower than Lightrec.
Its counters showed about one million generated-to-C-to-generated dispatches
per 1.5 seconds, while interpreter fallback represented only about 1.4% of
guest cycles. The hot dispatcher had been fetching and hashing every guest
word at every block boundary.

RAM and scratchpad blocks now record the generation of their source page(s),
making ordinary dispatch validation independent of block length. Generated
stores update the affected page after both direct LUT and slow-helper writes;
interpreter store fallbacks do the same. DMA/HLE clears, reset and state-load
notifications are forwarded by the app-local CPU adapter. Untracked mappings,
including ROM, retain source hashing. A changed page forces the outer compiler
path to hash/decode before reuse, so self-modifying code is not accepted merely
by PC. Profiling now reports sampled dispatcher CPU cycles instead of the
invalid timer-derived percentage. Hardware results should compare the same 3D
scene's `Time=`, real FPS, `dispatch_cycles`, and cache `writes` counters.

The first hardware result reduced native-window time from roughly 1.5 seconds
to 1.0 second, but the comparable 3D tick improved only from about 286 ms to
255--266 ms. The initial write tracker also revealed roughly 0.5 million
generated stores per profiling window. Its C helper call therefore replaced a
large part of the dispatch saving. RAM/scratchpad epoch increments are now
emitted inline after successful stores; non-RAM hardware writes skip them with
no additional C call. The startup delay-slot store test also verifies that the
generated increment executes. Interpreter fallback writes retain the C helper
and are reported separately as `fallback_writes`.

The first inline-invalidation hardware run exposed obsolete fixed compiler
layout limits rather than new unsupported guest instructions: hot 17--18 word
blocks filled the 2,048-byte prefix measurement region (`RV_NO_SPACE`), while
other successfully measured blocks exceeded the 3,328-byte total reservation.
Those failures were retried and logged on every visit, materially perturbing
the run. Measurement space is now 3,072 bytes for prefixes and 1,536 bytes for
branch adapters, with a 5,120-byte maximum compact reservation. The executable
arena and metadata table are doubled to 4 MiB/4,093 entries to offset the larger
generated store sequences. Repeated compile failures are logged for the first
16 occurrences and then at power-of-two totals.

The corrected hardware run had zero compile fallbacks, one cache reset, about
190 ms/tick before 3D and 233 ms/tick at the 3D transition. This remains far
from the roughly 67 ms/tick implied by the 15 FPS Lightrec comparison. Main-RAM
generated dispatch now has a 2 MiB instruction-address table. Its first emitted
RV32 path validated exact PC, cache generation and both source-page generations,
then jumped directly to the cached code offset. Missing, replaced, stale,
aliased, scratchpad and ROM entries retained the C lookup path. An inline block
counter preserves broad runtime accounting; the log distinguishes
`inline_blocks` from `c_blocks`.

The first instruction-address-table run confirmed that path is genuinely hot:
`c_blocks` stayed at zero while inline dispatch advanced by about one million
blocks per report, and sampled C lookup calls fell to roughly 50--90 thousand.
The representative pre-3D tick improved from about 190 ms to 171--173 ms. That
is useful but also rules out dispatch as the remaining explanation for the
roughly threefold Lightrec gap.

The next dispatcher revision makes populated RAM table slots authoritative.
Stores first test a 513-byte compiled-page bitmap; ordinary data-page writes do
no epoch work. A rare write to generated source advances its epoch and clears
all table entries beginning on that page plus the 17 possible cross-page block
starts immediately before it. DMA/HLE invalidation uses the same clearing rule.
The hot dispatcher therefore retains exact-PC and cache-generation guards but
removes both page-generation checks. It also keeps the live cycle in `s1` on a
successful native transition, publishing it only before C observes state or
when linkage exits. Hardware confirmation of this authoritative-table revision
is pending.

The first hardware run regressed loop 120 from 75 ms/tick to 717 ms/tick,
although native profiling windows fell from roughly 515 ms to 460 ms. Review
found that CPU/DMA invalidation cleared 4 KiB of dispatch pointers even for
empty pages; only generated stores had checked the compiled-page bitmap.
The common clearing function now skips empty pages for every caller. Epoch
updates remain intact. This correction requires hardware confirmation; native
window timings alone do not cover the total cost of the surrounding adapter.
Lightrec performance parity remains unproven, and earlier 50--100% speedup
expectations are not supported by the measurements collected so far.

### Register persistence across memory operations

The restricted compiler previously flushed and discarded every guest mapping
before every load or store, even though allocator slots backed by RV32
`s4`--`s11` survive both direct LUT accesses and ABI-conforming C helpers. It
now spills only the caller-clobbered slots plus the reserved `s2`/`s3` memory
temporaries, while retaining mappings in `s4`--`s11`. Memory base/store operands
are consumed directly from a retained mapping when available. Delayed-load
commit updates both canonical state and any retained host mapping; exception
stubs carry the dirty-map snapshot needed to publish earlier results before an
exact-PC fallback. Debug/release tests cover register pressure, retained store
operands, consecutive loads, load-delay cancellation, and guarded memory
faults. Runtime logs report compiled `retained` mappings and `operand_hits` so
hardware results can distinguish an inactive opportunity from an ineffective
one. Register mappings are still canonicalized at generated block boundaries.

## Status: shared decoder/allocator/ALU harness, not a selectable PSX CPU yet

A bounded block-cache foundation now has caller-owned metadata and executable
arena contracts, direct PC/source-hash lookup, aligned code reservation,
collision replacement, guest-range invalidation, and generation-based reset.
Its lifecycle self-test runs in QEMU and on P4. It is deliberately not connected
to Lightrec's live arena or selected as a CPU core yet.

The cloned whole-block probe now reserves its executable scratch region through
this cache, hashes the real guest words, compiles and publishes into the reserved
region, inserts metadata, verifies lookup, and dispatches through the returned
entry. This validates the complete cache path without retaining the temporary
entry or executing against live guest state.

Joined cached blocks use a two-pass compact layout. A measurement compile finds
the prefix size, the branch adapter is placed at the next 64-byte boundary, and
the prefix is recompiled with that final target before publication. The cache
reservation therefore covers the actual emitted span instead of the earlier
fixed 8 KiB diagnostic gap.

The cached whole-block probe now uses the production-safe exit convention: the
selected PC and cycle count stay in `psxRegisters`, while normalized fallback
status is returned in RV32 `a0`. Generated code no longer writes diagnostic-only
fields beyond `psxRegisters`. Sentinel words in those former clone fields must
remain unchanged after execution, directly guarding against adjacent-state
corruption before any live canary is permitted.

A second runtime-exit probe forces an inline load to the MMIO range. Generated
code must return fallback status 5 at the exact guest PC, leave the following
instruction unexecuted, preserve its destination registers, and retain the same
adjacent-state sentinels. This proves both success and conservative fallback
return paths use the production-safe ABI.

`PCSX_NDRC_RV32_LIVE_CANARY` enables the first live integration step and depends
on the self-test build. After all cloned probes pass, the observer retains its
scratch executable arena and compiler workspace in a waiting state. At real
PCSX/Lightrec synchronization boundaries after sampling, it compiles and
publishes blocks starting at the exact live PC, then executes them immediately.
It runs only when no load is pending, the guest source hash remains unchanged,
and the next event is far enough away. The one-shot state is claimed atomically;
Lightrec is then synchronized from each resulting PC and cannot execute the same
block twice. After the eight-block register/branch phase passed on hardware, the
current diagnostic phase selects a safe mixed ALU/memory prefix followed by a
conditional branch and safe delay slot. At that stage a load could not terminate
the prefix, so its delay was always resolved before the branch adapter. Compiled blocks occupy
fixed bounded slots in the tested app-local cache; a matching PC and source hash
reuses published code instead of recompiling. The live subset currently permits
at most one load per complete block: a real two-load/same-destination sequence
exposed a remaining load-delay/control-flow defect and is kept out of dispatch
until its cloned regression is fixed. The phase is bounded to four executions
and 512 candidate scans, and stops
immediately after a compile, publish, or runtime-status failure. This is not
general RV32 core selection.

The mixed-branch slot deliberately uses fixed non-overlapping prefix, branch,
and generated-dispatcher regions (1 KiB, 1792 bytes, and 256 bytes).
An earlier compact measurement used a different absolute dispatcher address
than final emission; variable-length address materialization let the final
prefix overlap the branch adapter. Per-instruction hardware tracing identified
the overwritten tail, and the temporary tracing was then removed.

After the four-block C-dispatched loop passed on hardware, the current diagnostic
adds a generated self-link trampoline to each complete block. It compares the
selected PC with that block's entry and tail-jumps directly back on a self-loop,
while checking the event deadline before every iteration. Other targets and due
events leave to C/Lightrec. After the first generated-link entry passed, the live
probe was expanded to four independently selected entries; each total cycle
delta must be an exact positive multiple of that entry's block length.
The four-entry summary accumulates generated self-links across all entries. A
successful linked exit also captures the successor's first eighteen words once
per such entry, providing the exact input for the next cross-block link without
executing unvalidated successor code.
The first captured successor ends in `JR ra` after three loads to distinct guest
registers. Live admission therefore now accepts the already-tested `JR/JALR`
emitter path and multiple loads only when their destinations do not repeat. The
expected indirect target is captured from canonical guest state before entry.
The next readiness gate retains the first validated A-to-B edge and reports it
linkable only after B exists in the current cache generation and a fresh fetch
of all B guest words still matches its cached source hash. This does not yet
mutate either published dispatcher.
After this gate passed on P4, the first live relinker was enabled for a cached
source and cached conditional-branch destination. It builds the replacement in
a temporary buffer, then publishes only the source's reserved dispatcher region.
The dispatcher retains the event check and self-loop edge, adds one exact-PC
tail jump to B, and otherwise returns to Lightrec. Linked execution validates
B's two static exits and the combined A-plus-B cycle count.
The first live A-to-B execution passed with the expected 12-cycle delta and B
fall-through PC. The bounded run is now eight entries and counts publications,
testing repeated linked execution while admitting B's observed successor into
the cache; link depth remains capped at two blocks.
Repeated hardware execution confirmed the generated A-to-B link itself remained
correct, but exposed that per-entry validation forgot a previously published
link on later cache hits. Published source PC and target length are now retained
separately and restore the linked accounting oracle whenever A is selected.
An eight-entry run then passed every repeated A-to-B link but found an indirect
oracle edge case: a later epilogue loaded `ra` inside its prefix before `JR ra`.
Such blocks cannot use the pre-entry `ra` as their expected target. The live
checker now detects prefix writes to the indirect source; it requires an aligned
result PC plus exact cycle accounting instead, while unchanged sources retain
exact target comparison.
Because Lightrec otherwise consumes the post-link PC before the next canary call,
one bounded C-side discovery continuation now scans and executes C immediately
after a successful generated A-to-B return. This is explicitly a cache-population
scaffold for the next B-to-C generated link, not the final dispatch design.
After C executes successfully, the probe retains B-to-C as a second edge and
requires both blocks to remain in the current cache generation with freshly
matching source hashes before reporting the second link ready. The bounded
hardware canary then patches B to C and extends A's final-PC and cycle oracle
across the complete A-to-B-to-C generated chain; production execution remains
with Lightrec.
After that chain is proven, the bounded selector also admits safe ALU-only
conditional successors. Initial selection remains memory-backed and therefore
deterministic; this expansion is used to validate the next observed block at
the C exit before extending linking any further.
The probe allows ten entries so this ALU-only successor can be reached again as
a cache hit. It is deliberately not linked yet: C has an unlinked alternate
exit, so the next link stage needs terminal-exit-specific cycle oracles rather
than the linear chain's single downstream count.
Link metadata is bound to the exact generated-code address as well as the guest
PC. A cache eviction followed by recompilation therefore cannot inherit stale
link expectations from an older patched instance at the same guest address.
The next observed C-to-D edge is retained only as a readiness probe: D must
execute independently and both cache entries must pass fresh source hashing.
C-to-D is not patched until the validation oracle can represent C's unlinked
alternate exit and D's two exits with their distinct cycle totals.
The branch-aware oracle now stores up to eight terminal PCs with an exact
downstream instruction count for each linked source. Once the third readiness
gate passes, C-to-D is patched and the corresponding C, B-to-C, and A-to-B-to-C
oracles are extended without treating C's alternate exit as though D executed.
Frontier extension replaces only the terminal matching the new link target and
retains every other conditional exit. Thus B's untaken exit remains valid after
B-to-C is published, and both B's and C's alternatives remain valid after the
C-to-D extension; the observed Ridge Racer direction is not assumed globally.
Before entering any patched source, the canary now revalidates every downstream
cache entry and its guest-source hash. If eviction or self-modifying code makes
any dependency stale, that linked instance is skipped and execution remains in
Lightrec. The bounded run is twelve entries so the observed cache displacement
can exercise this invalidation guard.
The application-local diagnostic arena began as 32 KiB in PSRAM, enough for ten
reserved live blocks. It is now 128 KiB for the bounded general phase while
remaining isolated from target sdkconfig and shared Retro-Go memory policy.
The canary uses 257 direct-map metadata slots for this arena. The prior
17-slot table caused the known B and D guest PCs to collide even while both code
allocations remained resident, obscuring the difference between table pressure
and genuine code/source invalidation.
Live link state is now stored as indexed records containing source identity,
target PC, and terminal PC/instruction-count paths. Entry selection and
downstream freshness traversal iterate these records rather than naming the
three proven links individually; bounded edge discovery is still staged.
Observed edge PCs and readiness state now use a parallel indexed record array.
The three-stage timing remains unchanged for hardware comparison, but later
dispatcher work no longer needs additional named edge fields.
Both arrays now reserve eight link records. Only the first three are populated
by the current deterministic canary; the extra capacity removes the metadata
ceiling before edge discovery and conditional publication are made iterative.
Once two links are proven, live admission also accepts a direct J/JAL with no
preceding ALU or memory prefix and a safe delay slot. Its static oracle uses the
MIPS absolute target calculation rather than the conditional-branch offset
formula. This covers the real `JAL` block observed at the D fall-through while
remaining inside the bounded diagnostic and returning to Lightrec afterward.
Zero-prefix conditional and indirect branches remain excluded; the first P4
trial showed such a conditional could otherwise be admitted by this relaxation
and rejected during compilation before the intended direct-call probe ran.
The next P4 run reached a real zero-prefix `JAL` at `8004b0b0` and showed that
the restricted prefix compiler deliberately rejects an empty instruction list.
The live path now emits a dedicated empty-prefix entry adapter before the normal
tested branch adapter, leaving the host suite's invalid-zero-length contract
unchanged.
After the first direct-call execution passed with the expected two-cycle delta
and absolute target, the live oracle was extended to require JAL's architectural
`r31 = branch_pc + 8` result as well. A successful run reports a dedicated
`direct-call link PASSED` line; PC/cycle correctness alone is no longer enough.
The bounded run is extended from twelve to sixteen entries and reports a
`direct_calls` total, allowing more opportunities to select the second real JAL
frontier while staying below the ten-block capacity observed at six misses.
Because Lightrec normally consumes a generated chain's terminal PC before the
next selector boundary, a single guarded direct-call frontier continuation now
runs immediately when a validated linked exit lands on J/JAL with a safe delay
slot. It is separately counted in the summary and still returns to Lightrec.
Its first placement was accidentally scoped to the third-link cache-miss arm and
therefore reported `discovery=1/0` despite repeated JAL frontiers. It now runs
from the common validated post-execution continuation path.
After the fixed sixteen-entry gate completes, the diagnostic now enters a
general bounded phase for the already supported block subset. Each Lightrec
handoff may execute up to eight native blocks, stopping before the event deadline
or immediately on an unsupported boundary. The phase is capped at 64 executed
blocks, 256 native selections, or 256 handoff calls, after which it permanently
returns to Lightrec. Its phase-local summary separates unsupported, event-budget,
load-delay and compile fallbacks and reports native guest cycles and the longest
per-handoff chain as well as cache hits, misses and resets.
This is coverage measurement, not production core selection.
The app-local executable arena is 128 KiB with 257 direct-map entries. If it
fills during the general phase, a generation reset discards all cached code and
link metadata before recompilation; no patched pointer into overwritten code is
retained. Fixed canary cache exhaustion remains a hard diagnostic failure.
The first general P4 run executed three newly discovered blocks in sequence, but
then selected the fixed canary's linked indirect-return block with a different
runtime target. Generated dispatch correctly missed that link; its deterministic
oracle did not. The transition now starts with a fresh cache generation and
clears all fixed link/edge metadata, isolating broad execution from test-only
Ridge Racer paths and making the entire arena available to general discovery.
With that isolation, the first general handoff executed eight consecutive native
blocks and later ran a 3540-cycle generated loop. The phase completed all 64
native entries without an accounting failure or cache reset. One candidate at
`8002d2b0` exhausted the 1792-byte prefix region at 1788 emitted bytes; the region
is now 2048 bytes. Each diagnostic cache slot grew from 3072 to 3328 bytes so the
separately emitted branch adapter retains its previously proven 1024-byte region
and the final dispatcher retains 256 bytes. General compile failures roll back
their arena reservation, count
`compile_fallbacks`, and return that block to Lightrec while the bounded
dispatcher remains available for later PCs; fixed-gate compilation failures
remain fatal to the diagnostic.
The completed Ridge Racer phase measured 3863 native guest cycles across 64
generated entries, with a longest five-entry C-side chain, 37 phase-local cache
hits, 27 misses, 86 unsupported selections, and no event, load-delay, compile,
or cache-reset fallback. The first attempt to print the cycle total through the
ESP log's 64-bit formatter produced literal `lu` and shifted the following
labels; this bounded counter now uses the native 32-bit log format.
The first general-link run published seven disjoint one-hop links and executed
eleven linked blocks with exact PC/cycle validation and no failure. The general
phase now allows these observed edges to form an acyclic graph of up to eight
links and eight terminal paths. Both edge endpoints must first execute and
validate independently, source hashes must still match, and every source has at
most one linked successor. After adding an edge, all affected path oracles are
rebuilt recursively with exact guest-word and block counts; cycles, self edges,
stale entries and path explosion are rejected before generated code is patched.
Every dispatcher retains its event and exact-PC guards, and all unlinked exits
return to Lightrec.
The first P4 run exposed an incorrectly recorded event-branch patch position: it
patched the preceding cycle subtraction and left a zero-displacement branch,
locking the emulation task inside generated code. The patch position is now
captured immediately before emitting the branch.
The corrected P4 run executed the selected ten-instruction loop 357 times in one
generated-code entry and exited at the expected fall-through PC. The dispatcher
now also clears its PC-check scratch register before returning so normal exits
report status zero and expose the generated-link count accurately.

The earlier boundary candidate contained a nonempty sequence of nontrapping,
register-only ALU instructions followed by a conditional branch and safe ALU
delay slot. The current memory candidate still excludes indirect/unconditional
jumps and terminal loads. Unsupported boundaries are skipped without publishing
code and Lightrec remains in control. Invalid mappings and MMIO return an exact-PC
status-5 fallback before the memory operation, allowing Lightrec to execute it.

### Latest milestone: inline mapped-RAM access

The P4 diagnostic now compares all 448 restricted aligned memory cases with actual PCSX
load/store, arithmetic and load-delay handlers operating on a private
`psxRegisters` and isolated 256-byte RAM. Every address is checked before memory
helpers run; a private single-entry LUT cannot access live RAM or MMIO. Compare
GPRs, RAM, final PC, normalized fault status and instruction count. This does not
test real interrupt scheduling or architectural exception delivery.

On P4, generated loads and stores now call the same `psxMemRead*`/`psxMemWrite*`
API used by the emulator through that private `psxRegisters` and LUT. This also
exercises generated-code-to-C calls and signed-load extension. QEMU retains the
direct bounded-array model so the portable oracle remains independent.

An additional P4-only mapping suite installs private 256 KiB read/write lookup
tables and private scratchpad backing. Generated code indexes `memRLUT` or
`memWLUT` directly for physical RAM, cached KSEG0, and uncached KSEG1, then emits
the native byte, halfword, or word load/store against the resolved host address.
This common path therefore avoids both the mapping-classifier call and the PCSX
memory-helper call. The suite checks alias reads, writes, load delay, PC, and
cycles, then frees all temporary memory.

Live P4 blocks now take a still shorter main-RAM path before consulting those
tables. Exact range checks admit only `00000000`--`007fffff`,
`80000000`--`807fffff`, and `a0000000`--`a07fffff`; the resulting address wraps
onto `psxM` every 2 MiB as `lutMap()` does. Scratchpad, BIOS, MMIO, invalid
segments, and alignment faults retain the existing LUT/helper behavior. The
private mapping suite executes reads and writes through all three RAM aliases
with this shortcut enabled, while its scratchpad and invalid-address cases
guard the fallback boundary. Cumulative `ram_fastpaths` telemetry reports how
many such access sites were emitted into published live blocks. This removes a
random access to the 512 KiB PSRAM-backed read/write LUT from ordinary RAM
operations; its runtime value must still be established by hardware timing.

An invalid LUT entry branches to the conservative mapping classifier. Scratchpad
accesses proceed through the real PCSX memory helper; hardware registers and an
unbacked BIOS range exit at the exact guest instruction with normalized status 5
for future interpreter fallback. Tests verify that these unsafe addresses cannot
reach live hardware handlers and that no later instruction executes.

The ABI now names allocator slots 11/12 (`s2`/`s3`) as the full-address memory
address/value temporaries. The compiler verifies that its boundary adapter has
written back and cleared both slots before lowering every load/store, and the ABI
self-test verifies their distinct callee-saved mappings. This makes the current
spill-and-clear contract explicit; later lowering may replace it with temporary
allocation if retaining mappings across memory operations proves worthwhile.
An allocator-pressure regression fills dirty mappings through both temporary
slots, crosses a store and load-delay sequence, and checks every preserved GPR,
RAM, PC, cycle, and pending-load result after generated execution.

The bounded P4 shadow sample now emits this full-address inline sequence for the
loads and stores found in real game blocks. Its output remains unpublished and
unexecuted, but its compile/failure and code-size totals expose integration and
capacity problems before the path is permitted to affect guest execution. The
serial summary is labelled `shadow (inline map)` to distinguish this milestone.

Once per launch, the observer publishes and executes the longest initial safe ALU
run found in its 512-block sample. Candidate selection is independent of whether
the containing block later branches or accesses memory, and stops before memory,
control flow, or a trapping arithmetic instruction. Execution uses a cloned
`psxRegisters` and compares every GPR, PC, and cycle result with the reference
semantics. It cannot mutate live guest state. The shadow summaries expose the
current candidate as `alu_run`, and the final serial result identifies the exact
PC and operation count.

The observer also retains a real conditional branch with a safe ALU delay slot.
At the end of the sample it compiles and executes that pair twice against cloned
CPU state, forcing one taken and one untaken result. The comparison covers the
delay-slot update, link value where applicable, selected PC, cycles, and all GPRs.

A third clone probe retains a real game load followed by a safe ALU instruction.
It forces an aligned address into private RAM through a private full-size P4 LUT,
then compares generated execution with the actual PCSX interpreter handlers.
The comparison includes signed/unsigned load behavior, the architectural load
delay, all GPRs, private RAM, PC, cycles, and normalized exception status.

The matching store probe redirects a real SB/SH/SW sequence through a private
write LUT and compares the resulting private RAM and following ALU state with
the interpreter. Together these probes cover both inline lookup directions with
game-derived instructions.

The next P4 probe retains the longest short real prefix which contains both ALU
and memory operations, uses one address base that is not overwritten before an
access, and does not end with a pending load. It relocates that base into the
same private RAM and compares the whole generated prefix with the PCSX
interpreter. This exercises allocator state and dependencies across several real
instructions; it is still cloned-state validation, not live guest execution.

The observer also searches for a complete eligible block made from such a mixed
prefix followed by a conditional branch and safe delay slot. If found, it joins
the separately emitted prefix and branch code, publishes the complete generated
region, and compares one naturally selected cloned branch outcome with the
interpreter. Taken and untaken behavior remain covered separately by the real
branch-pair probe. An explicit `NOT
FOUND` result is logged when the bounded sample contains no safe candidate, so
absence of a PASS line cannot be mistaken for success.

A complete-block clone probe then selects the longest such prefix that ends in
a supported real conditional branch with a safe delay slot. It executes the
actual prefix-to-branch generated-code handoff twice, forcing taken and untaken
paths, and checks private RAM, all GPRs, final PC, cycles, and load-delay state.
Branch inputs must remain untouched by the prefix so both outcomes are controlled
without rewriting the captured game instructions.

This exposed and corrected a fixture bug: a successful consecutive load to the
same destination must cancel the older pending load. The handwritten reference
had shared the same mistake. Faulting loads still commit the older pending load.
A literal expected-value regression assertion also runs in QEMU.

QEMU debug/release suites and the gb300-p4 build pass. Hardware subsequently
passed the real-interpreter memory cross-check with no game regression. Generated
GPR accesses, generated PC exits, the cycle counter, and the restricted LW load
delay value/register marker now execute against a private real `psxRegisters`
object, guarded by compile-time offset assertions. Committed and cancelled loads
clear both real delay fields, which is asserted after every memory fixture run.
Two generated blocks also exercise an LW pending across a tail-dispatch boundary:
the following instruction captures its old source first, then dynamically commits
or cancels the load according to its destination. The producer uses the shared
compiler; the consumer remains a narrow boundary adapter, not general allocator
support for arbitrary entry load-delay state.

The first 512 blocks discovered by Lightrec are now sampled for shadow coverage.
The diagnostic compiles only each supported straight-line prefix into ordinary
scratch PSRAM; it never publishes or executes that output and frees its roughly
30 KiB workspace after the sample. Reports at 64 and 256 blocks include complete
blocks, successful/failed prefixes, guest/source instruction coverage, emitted
bytes, terminal pending loads, capped blocks, and the most frequent first blocker.
This temporarily adds compilation work during the sample, so its gameplay timing
must not be treated as a performance result.

The first Ridge Racer sample identified control flow—not multiply/divide—as the
dominant first blocker: JAL, JR, BNE and BEQ accounted for 184 of 256 blocks.
The sampler now joins a supported straight-line prefix to the tested branch and
delay-slot generator through canonical GPR state, and reports `branch` plus
complete-block coverage. A prefix ending in a pending LW is still excluded from
this join until the general allocator supports dynamic entry load-delay state.
Test-only fault/dispatcher fields remain after that object; architectural
exception/event exits, cross-block pending loads and production memory mapping
remain future work.

The working P4 Lightrec checkpoint is commit `fd856e3` on branch `psx`.
Lightrec/GNU Lightning remains the game CPU. Nothing in this milestone changes
guest instruction semantics, rendering, frameskip, memory mapping, or target
sdkconfig. S3 remains interpreter-only.

The new implementation lives in
`components/pcsx_rearmed/libpcsxcore/new_dynarec/rv32/`. It currently provides:

- Direct RV32IM instruction encoding, independent of GNU Lightning.
- One/two-instruction constants, signed/unsigned loads, stores and comparisons.
- Short branches/JAL with checked displacements and patching.
- Fixed-size full-address calls/jumps and long conditional branches. These work
  across RV32 address space, without assuming flash helpers are within JAL range
  of executable PSRAM. Multiword patches require quiescent code and cache sync.
- Sticky bounds/operand errors in release builds; multiword emission reserves
  space before writing. A failed emitter must never be published or executed.
- A shared instruction-execution test suite for QEMU and optional P4 startup.
- An explicit 22-slot host register mapping, entry/leave assembly, and a
  live-register-preserving C-helper bridge. The mapping is now exercised by the
  shared register allocator in a restricted harness; guest event handling and
  compiler-generated helper calls remain unconnected.
- Canonical 32-bit NZCV predicates for ADD/SUB/CMP and AArch64-style TST,
  preserving results across operand changes and helper calls. These are host
  compiler predicates, not emulated PSX flags, and are not yet wired into the
  shared compiler's condition helpers.
- The existing `alu_assemble`, `imm16_assemble`, `shiftimm_assemble` and
  `shift_assemble` functions now reside unchanged in `assemble_alu.h`, included
  by both `new_dynarec.c` and the RV32 differential-test harness. The new
  `assem_rv32_alu.h` implements their host-slot emission interface.
- The shared `disassemble_one`, register lookup/allocation helpers and four
  ALU allocation functions now also live in shared headers. Their function
  bodies are unchanged; both the upstream compiler and RV32 harness include
  them. Shared types/constants prevent the fixture's layouts drifting apart.

The full RV32 architecture is **not** selected in `new_dynarec.c` yet. Shared
arithmetic emission is connected and tested, but there is no RV32 `psxRec`
implementation. No FPS improvement is expected from
this diagnostic build. First emission uses 32-bit instructions only; compressed
instruction emission can be evaluated after correctness and integration.

## Tests

From the repository root, with the ESP-IDF compiler on PATH:

```powershell
python pcsx_rearmed/tests/rv32/run.py
```

The runner locates the installed Espressif QEMU (or accepts `--qemu PATH`). It
builds freestanding RV32 debug and optimized/NDEBUG binaries, executes generated
instructions, and checks the exit status plus a success marker. Each QEMU run
has a 30-second timeout. Build artifacts stay in `tests/rv32/build/` (ignored).

Checks include a GNU assembler encoding oracle, 3,456 ALU/conditional-branch
input combinations, immediate sign boundaries, multiplication high words,
RV32 division edge cases, narrow memory accesses, backward branches,
short/long target patching, JIT-to-C calls with 16-byte stack alignment, and
release-build overflow/invalid-operand rejection. Optimized C helpers use the
compressed ISA; emitted code does not. This tests host RV32 division semantics,
**not** the different PSX divide-by-zero result rules.

QEMU validates instruction behaviour, not the P4 MMU or cache hardware.
The freestanding test ELF deliberately uses a writable/executable RAM segment;
GNU ld's RWX-segment warning for this test harness is expected.

Local validation: both QEMU configurations pass, and the gb300-p4 firmware
builds with the diagnostic ON and OFF. The OFF build contains no `ndrc_rv32`
symbols. The user-tested log at 15:16:37 confirms the emitter suite passes on
real P4 executable PSRAM, followed by normal Lightrec game execution. The next
user-tested log at 18:44:18 confirms both emitter and ABI suites pass on P4;
the user reported no regressions. The 18:54:35 log confirms the predicate suite
also passes on P4. The 19:11:49 log confirms the shared-ALU tests and PCSX-handler
cross-check pass on P4, with no regressions reported. The expanded shared
decoder/allocator tests also passed on P4 at 07:53:50 on September 7, with no
regressions reported. The 14,024-byte workspace landed in PSRAM and the runtime
stack high-water mark remained 5,496 bytes. Dirty-writeback and constant tests
subsequently passed on P4 at 08:11:43 with no regressions reported. Terminal
branch/delay-slot tests passed on P4 at 08:22:40 with no regressions reported.
The bounded multi-block dispatcher passed on P4 at 08:30:52 with no regressions
reported. The subsequent bounded memory and load-delay suite passes debug and
optimized QEMU; its P4 execution remains pending. Existing target Kconfig
warnings remain untouched.
Full guest-core testing remains pending.

## First shared compiler integration

`rv32/compiler_test.c` uses the actual shared decoder, allocation helpers,
arithmetic/shift allocation routines and emission routines. Registers remain
mapped across instructions; only newly mapped inputs are reloaded, and pressure
forces the shared next-use/eviction logic to select victims. The fixture checks
that reuse and eviction actually happen, maps stay unique, and the cycle-count
slot remains reserved. Dirty results now remain in host registers until eviction,
block exit, or a conservative barrier before a potentially trapping instruction.
All displaced dirty mappings are stored before any newly mapped inputs are
loaded. The barrier preserves prior architectural state on overflow, without
requiring per-exception-stub dirty-state reconstruction yet.

The shared allocator's known results are materialized immediately into their
host slots, with its constant flags retained across nontrapping instructions.
The shared emitters can omit operations whose results are already known.
Potentially trapping instructions discard constant metadata and execute their
overflow checks normally. This does not implement upstream's delayed/final-value
constant loading or complete constant-propagation passes.

This is NOT the complete compiler pass pipeline. Non-ALU opcodes are rejected
before invoking the decoder; unavailable GTE/HLE metadata cannot be reached.
All nonzero GPRs are conservatively live, and bounded test-only overflow stubs
replace real exception delivery. The driver
explicitly pins ADD/SUB inputs when their destination is r0, since an overflow
must still trap. Liveness analysis, branch/delay-slot allocation and the full
production writeback pipeline are not yet tested. Emission failure rejects
the block; there is no interpreter fallback in generated code.

The allocator's lookahead can inspect one instruction past the block end. An
explicit zeroed sentinel gives this fixture the headroom the full compiler has;
the optimized pressure test caught a missing sentinel before firmware handoff.
About 14 KiB of cold compiler workspace is temporarily allocated with `MEM_SLOW`
on P4 and freed before Lightrec starts. No target stack configuration is changed.

Tests cover 24 arithmetic/immediate/shift instructions, all immediate shift
amounts, zero-register cases, source/destination aliases, signed immediate
boundaries, overflow before destination commit, a dependent 16-instruction
block and a mid-block overflow exit. New cases add a 64-instruction pressure
block, 128 mixed 64-instruction blocks, and overflow after pressure with both
normal and zero destinations. Dirty-state tests prove a 64-instruction accumulator
emits just one final guest-register store, check four chained constant results,
and verify positive/negative overflow preserves a destination aliasing its input.
Invalid block lengths and an exhausted code buffer are rejected without execution;
a canary checks that emission does not overrun the buffer. In total 825 compiled
cases are run with 16 initial register states each (13,200 executions).
All 32 GPRs, normalized
overflow status, fault/end instruction position and a test instruction counter
are compared with an independent restricted MIPS interpreter.

On P4 the same cases additionally cross-check PCSX's real arithmetic handlers
through a diagnostic-only whitelist in `psxinterpreter.c`. These handlers run on
a zero-initialized scratch `psxRegisters`; they never fetch memory or touch live
guest state. Overflow is intercepted before entering the emulator's global
exception path. Thus real CP0 exception delivery, branch/load delays, interrupts
and emulated cycle timing are explicitly NOT validated here. The test counter
is not the production new_dynarec cycle model. The interpreter adapter's measured
P4 stack frame is 896 bytes; no stack configuration was increased.

## Terminal branch and delay-slot fixture

The compiler suite additionally executes 1,500 branch/delay-slot cases (150
compiled pairs, ten input states each). It supports BEQ/BNE, BLEZ/BGTZ,
BLTZ/BGEZ and their link forms, J/JAL and JR/JALR. The shared decoder supplies
operands/link destinations; a restricted RV32 lowering captures the condition or
indirect target before writing the link and executing the shared ALU delay-slot
compiler. The selected guest PC is returned to scratch state, not dispatched.

Tests cover both condition outcomes, negative relative targets, the J/JAL
PC+4 high-nibble boundary, link values visible in the delay slot, JALR source/link
aliasing, link branches reading r31, and a delay slot changing the condition or
indirect-target register. Overflow returns normalized status 2 and branch EPC,
preserving the link write but not the faulting ALU destination. Both taken and
untaken branches execute their delay slot. Nested branches and loads in delay
slots are explicitly rejected by this fixture.

Branch expectations use an independent restricted reference model, not the live
PCSX branch interpreter (which accesses global state). This does NOT connect the
upstream branch allocation/emission passes, delayed loads, CP0 exception delivery,
indirect-target alignment/fetch exceptions, register-map joins, block linking,
event deadlines or a guest dispatcher. Unaligned selected targets are returned
as values without fetching them. Entry starts with a clean register map; the
multi-block fixture below now supplies earlier results through memory writeback.

## Bounded generated multi-block dispatcher

The fixture now compiles four blocks into separate regions of the same 16 KiB
scratch arena. A generated RV32 dispatcher looks up the returned guest PC and
tail-jumps to the next block without returning through C. Each block commits
dirty GPRs before exit; the next block rebuilds its register map from canonical
scratch state. This connects the terminal branch path to preceding ALU results
and executes both backedges and fallthroughs. It does not yet carry live host
register mappings across blocks or use production lookup/linking code.

There are 592 loop/budget combinations, checking every GPR, selected PC, remaining
block budget and instruction count. A four-block program accumulates loop results,
executes delay-slot updates, takes a fallthrough and then exits through an indirect
target missing from the table. A zero budget exits without executing a block.
Three further cases check an existing exception and newly generated delay-slot
overflow on both taken and untaken paths, including dirty results from the
preceding block. Exception state stops dispatch before another block runs.

The block budget is a test safety bound, not a PSX cycle deadline or interrupt
model. Lookup misses return without compiling or interpreting the target.
Production block caching, invalidation, linking, event handling and memory access
remain unconnected, and Lightrec remains the game CPU.

The later bounded P4 handoff does exercise a diagnostic cache and direct
generated links. The September 10 hardware log executed a four-block path with
an exact 21-cycle oracle match. Per-link `depth` messages describe the path when
that edge is first published; they do not cap a path extended by later acyclic
links.

## Bounded guest RAM and load delays

The compiler fixture now lowers aligned LB/LBU/LH/LHU/LW and SB/SH/SW against a 256-byte little-endian
scratch RAM embedded in its local test state. Address arithmetic wraps at 32 bits;
alignment and range are checked before any host load/store. Faults return normalized
status 3 for loads and 4 for stores, with the faulting instruction position. This
is intentionally not the PSX memory map: RAM mirrors, scratchpad, BIOS, MMIO,
cache isolation and device side effects remain unconnected.

The suite models one pending MIPS load. The instruction immediately following LW
captures its operands before the loaded value becomes visible. It checks a normal
delayed load, the following instruction overwriting the same destination,
consecutive loads to the same and different registers, a store consuming the old
value, LW to r0, and an ALU overflow while a load is pending. The older pending
load commits even when the following memory access or ALU instruction faults.
Aligned boundary addresses, wrapping/negative addresses, and misaligned or
out-of-range accesses are compared across all 32 GPRs and all 64 RAM words.

Branch-delay coverage also executes SB/SH/SW delay slots for both taken and
untaken branches. Stores are safe at the current generated-block boundary because
they leave no delayed result. Loads in delay slots remain rejected until target
entry and direct-link adapters consume an incoming MIPS load delay correctly.
Repeated load destinations remain excluded from a single live generated block.
Although the isolated differential suite checks their cancellation semantics,
a September 10 live block using repeated load destinations returned the correct
PC but advanced only 11 of its expected 16 cycles. The safety gate caught the
mismatch and returned control to Lightrec. The general phase now salvages the
safe portion by splitting before the second load and trimming an unresolved
terminal load; Lightrec resumes at that exact boundary. This keeps the failing
combination disabled while avoiding rejection of the entire prefix.
The broad general phase also admits supported conditional and indirect branch
blocks with an empty ALU/memory prefix. The deterministic 16-execution phase
retains its narrower selection policy. Branch-only lowering and the empty entry
adapter were already exercised separately; this combines them only after that
fixed hardware gate completes.

The general phase can also publish a bounded straight-line ALU/memory prefix
which stops immediately before an unsupported instruction. Its generated exit
stores that exact next PC and returns through the same event-checked dispatcher,
so Lightrec handles the unsupported operation without re-executing the prefix.
An unresolved load is trimmed from the end and repeated load destinations remain
excluded from each generated fragment. Straight blocks also participate in the
bounded acyclic generated-link graph. The link oracle derives one exit for a
straight block and one or two exits for a terminal primary branch, preserving
exact path cycle counts across mixed chains. Indirect branches remain
deliberately unlinked.

Live RAM addresses can be lowered inline, while an address that resolves to
scratchpad uses the PCSX helper. An unbacked or hardware address returns status
5 at the exact guest memory instruction. Guarded exits bypass the normal
event/link dispatcher so that it cannot overwrite the nonzero status. The live
wrapper retains all completed prefix effects, removes the guard's speculative
instruction-cycle increment, and lets Lightrec execute that memory instruction
once. These expected runtime fallbacks are counted separately from unsupported
selection and compilation failures.

After the 64-entry general gate completed with exact accounting, the sustained
diagnostic extends the same path to 2,048 native block entries and 8,192 bounded
selection attempts. It deliberately crosses executable-cache generation resets
and continues exercising mixed straight/branch links over a longer portion of
boot. Detailed per-block logging stops after entry 64; progress is reported every
256 entries, while errors remain unconditional. This remains a finite safety
stage rather than production core selection.

The first sustained run validated 388 generated executions and two complete
cache generations, but stopped after entry 256 with `valid=1` and `status=0`.
The original fixed-gate limit of 512 selector scans was still shared with the
long phase. That cap now applies only to the fixed 16-entry gate; sustained
selection is bounded by its own 8,192-attempt limit. Repeated direct-call and
linked-successor success dumps are also covered by the 64-entry detail cap.

The next P4 run completed all 2,048 generated entries, including 859 linked
blocks, 59,815 native guest cycles, a maximum native chain of 25 blocks, and 17
executable-cache generation resets. It then took an instruction-access fault
while emitting the former single, unusually long completion record. Completion
telemetry is now split into short records. This preserves the validated long
run while isolating the logging boundary from any latent post-run ABI fault. A
repeat run emitted every split record and continued normally, confirming the
generated execution and ABI were not the source of that fault.

Trapping `ADDI`, `ADD`, and `SUB` are now admitted within general native
prefixes. Their existing generated overflow stubs preserve prior effects and
return status 1 at the exact trapping PC. The live wrapper removes the
speculative instruction cycle and lets Lightrec deliver the architectural
overflow exception once. Delay-slot overflow remains excluded because replaying
the delay instruction alone would lose the branch exception context. Separate
admission and overflow-fallback counters make this extension measurable.

The finite sustained gate now also exercises an actual hybrid fallback loop.
While that gate is active, an unsupported boundary receives one Lightrec block,
then the existing outer CPU loop immediately offers the resulting PC back to
new_dynarec. This avoids surrendering the remainder of the event slice after
every BIOS HLE, COP/GTE, or otherwise unsupported block. The normal event-sized
Lightrec batch is restored as soon as the 2,048-entry gate completes or rejects,
so the experiment remains bounded. Completion telemetry reports the number of
these one-block Lightrec round trips.

The first hybrid run completed 2,048 native entries in 416 dispatcher calls
with 415 one-block Lightrec round trips. Unsupported selections fell from 761
to 32 because control returned to new_dynarec immediately after each fallback;
there were no compilation, accounting, or execution failures. Its maximum
native run was exactly the artificial eight-invocation call limit. The next
bounded stage therefore raises a native batch to 64 invocations and the total
gate to 16,384 entries/65,536 selections. Progress is emitted every 2,048
entries. Event checks remain active after every generated block, and completion
still restores normal event-sized Lightrec batching automatically.

Raising the batch and gate completed cleanly, with a maximum native chain of 64
and no compile or accounting failures. It also showed that block-count growth
alone mostly repeats a safe BIOS wait loop: 16,383 cache hits let the entire
16,384-entry gate finish before the first 60-frame report. Unsupported and
guarded instructions in the bounded phase now use PCSX-ReARMed's existing
`execI` dynarec fallback directly and resume native selection from its resulting
state. This removes Lightrec from individual opcode fallback while retaining it
as the temporary outer CPU/event owner. Interpreter calls and consumed cycles
are reported separately. An HLE recursion guard lets nested `ExecuteBlock`
calls use Lightrec normally, and a frontend stop reached by `execI` prevents an
extra Lightrec block from running afterward.

The first direct-interpreter run exposed a hybrid scheduling spin after ten
native entries and eleven successful interpreter fallbacks. Once a dispatcher
pass stopped advancing, the old phase-wide hybrid flag still forced 65,535
two-cycle Lightrec slices until the diagnostic call limit expired. The bridge
request is now a consumed, one-shot token armed only by a dispatcher pass that
actually changes canonical PC/cycles. A zero-progress pass therefore restores
normal event-sized Lightrec execution, allowing the event owner to cross the
boundary before new_dynarec is offered the resulting state again.

That change correctly removed all forced two-cycle bridge steps, but the next
run still reached 65,536 calls with only ten native entries. Its unchanged
selection count, zero event deferrals, and zero bridge steps identify callbacks
made while the frontend `stop` flag is held across a BIOS/HLE boundary. These
callbacks now suspend the diagnostic without consuming its finite call budget
or incrementing the no-native count. A separate stop-deferral counter records
them, and the same general phase resumes when the frontend becomes runnable.

The corrected run completed all 16,384 native entries in 269 productive calls:
30,506 linked blocks, 225,799 native guest cycles, 363 direct interpreter
fallbacks, 241 exact-PC guarded fallbacks, and no compile or execution failure.
It also recorded 1,048,575 suspended stop callbacks during the BIOS/HLE polling
window without exhausting the gate. With that broad execution path validated,
a productive native batch now synchronizes canonical registers to Lightrec and
returns directly to the CPU outer loop instead of executing a one-block
Lightrec bridge. Lightrec remains the temporary event owner only when native
dispatch cannot advance near an event boundary. The completion counter is now
reported as `native_returns` rather than `lightrec_steps`.

That native-owned gate again completed cleanly, but its final guest cycle was
only about 21.97 million; the first race in the extended trace began around 950
million cycles. All later play was therefore a handoff test running on Lightrec,
not native gameplay coverage. The general gate now uses a long finite ceiling
of 1,073,741,824 native entries and 4,294,967,295 selections, which is effectively
continuous for ordinary hardware testing while preserving a terminal bound.
Progress logging is reduced to every 1,048,576 native entries and split into
short records containing guest PC/cycle and cumulative fallback/cache totals.
Compile, publish, accounting, or execution failures still reject immediately.

The first continuous run did not deadlock: GPU/event counters and guest cycles
continued advancing, but only about 85 emulated frames completed in more than a
minute. At 4,194,304 native entries it had taken 590,174 status-5 exits and
618,656 interpreter calls, advancing just 26 million guest cycles. Those exits
were the earlier clone-safety rule rejecting invalid LUT entries before they
could reach PCSX's MMIO handlers. Real live blocks now use the existing
`psxMemRead*`/`psxMemWrite*` slow helper for those addresses, while synthetic
clone/selftest compilations retain the status-5 guard. Generated code publishes
the exact memory-instruction PC and live cycle before the C helper and reloads
the cycle afterward, so hardware timing side effects are visible to the native
dispatcher. This should sharply reduce runtime/interpreter fallback without
relaxing the isolated-test boundary.

Because DMA, MDEC and CD-ROM handlers may add cycles inside those helpers, a
linked path's decoded instruction count is now validated as a minimum rather
than an exact cycle total. Exit PC, link-register state and finite linked-path
membership remain mandatory. Hardware-added timing is counted separately in
the continuous `adjusted`/`extra_cycles` telemetry, and accounting failures now
print the actual PC before new_dynarec is disabled. Synthetic tests retain exact
cycle checks.

Linked dispatchers also check `next_interupt` between blocks. Their oracle now
records every acyclic intermediate successor with its cumulative instruction
and block counts, allowing a generated chain to return there only when the
event deadline is actually due. This keeps event delivery prompt without
mistaking a valid mid-chain boundary for an unmodelled terminal branch; such
returns are reported as `event_exits`.

The first continuous run beyond that boundary reached 2,097,152 native entries
without rejection, but needed 271 code-cache generations and advanced only
about 15.4 million guest cycles. The diagnostic cache was still the 128 KiB
selftest arena, which holds fewer than forty fixed-reserve live blocks. The P4
live arena is now 2 MiB with 2,047 PSRAM-backed metadata entries. Exact-PC cache
validation uses the direct-mapped slot rather than scanning the whole table;
this both avoids consuming scarce internal RAM and keeps larger-cache link
freshness checks constant-time.

That cache reduced resets at 2,097,152 entries from 271 to one and carried the
game beyond 17 million native entries without a runtime or accounting failure.
It also exposed direct-map collisions as the source of repeated stale-link
deferrals. Cache lookup/insertion now uses an eight-slot bounded probe, and a
rare genuinely stale chain is restored to unlinked dispatchers instead of
being logged and interpreted on every visit. Stale warnings are rate-limited,
the productive native batch grows from 64 to 512 selections, and detailed
per-block logging ends after 16 entries.

Productive cache hits now fetch and hash only the block extent recorded at
compile time, then reconstruct the small amount of dispatch metadata directly
from those words. Fetching and decoding the full candidate window remains on
cache misses.
The general dispatcher enters the block immediately after this validation, so
it no longer repeats the same source fetch and hash in the execution gate. The
bounded/fixed canary retains that second guard because it can remain armed
across Lightrec callbacks. Continuous cache telemetry reports `fast_hits` so
hardware logs can verify that the optimized path is actually dominant.

Live cache misses now compile twice: a measurement pass uses the former maximum
layout, then the published pass packs the generated prefix, optional branch
adapter, and 256-byte patchable dispatcher into adjacent 64-byte-aligned
regions. A spare cache line on each generated region covers address-dependent
materialization differences. Cache entries and link records carry the resulting
dispatcher offset, so link publication and stale-link detachment do not depend
on a fixed address. The 3,328-byte limit remains as a checked failure boundary,
but successful blocks return their unused reservation immediately to the arena.

The first compact-layout hardware run reduced typical live blocks from about
3,120 bytes to 384--2,112 bytes and cache resets from four to one over roughly
19 million native entries, without changing the approximately 3 FPS result.
This rules out code capacity and the former fixed gaps as the immediate speed
limit. The productive link table is therefore expanded from eight to 32 edges;
its table and transactional backup live in PSRAM, while exit/event oracles and
recursive path construction remain bounded to eight. Progress telemetry now
reports published links, linked blocks, and native returns to measure whether
fewer generated blocks are crossing the C dispatcher.

Hardware showed that widening this static link window was not sufficient:
only 278,495 of 17.8 million entries were reached through a link (about 1.6%),
and the run remained around 2--3 FPS. Productive blocks now use a dynamic
generated dispatcher instead. At every block boundary it first checks the
event deadline, synchronizes the live cycle, calls a conservative exact-PC
cache lookup which rehashes the recorded guest extent, and tail-jumps directly
to a valid cached target. Cache misses or modified source return to the normal
C compile/interpreter bridge. This also permits bounded-by-event traversal of
loops without publishing cyclic static-link oracles. Runtime validation counts
the exact dynamically selected guest words and accepts a return only at an
event boundary, cache miss, or existing exact-PC runtime fallback.

These live executions use the shared decoder but a restricted RV32 memory
lowering, not the upstream load/store allocator or assembler. Saved-register
mappings now survive memory operations. Aligned main-RAM accesses use the
direct `psxM` mirror path and other aligned mappings use direct LUT paths;
LWL/LWR/SWL/SWR initially use PCSX memory helpers while remaining inside the
native block/dispatcher. A standalone terminal integer load is exported through
the interpreter's real delayed-load slots; the common load -> branch -> delay
sequence is compiled as one unit. General native consumption of pending state at
an arbitrary successor entry, complete memory timing and real exception delivery
remain future work.

The ABI suite uses assembly sentinels for all twelve callee-saved registers and
checks gp/tp preservation. Generated code writes every allocatable register,
calls an ABI-compliant helper which overwrites every caller-saved integer
register, and exits via a tail jump. Six live-register masks and four initial
cycle values exercise selective saves, retained C return values, cycle wrap,
16-byte stack alignment and balanced helper frames. Emission-buffer exhaustion
must reject the entire helper bridge without writing a partial prologue.

Current private mapping: slots 0..7 are a0..a7, 8..10 are t0..t2,
11..20 are s2..s11, and slot 21 is the live cycle counter in s1. s0 holds the
local-state base, t3..t5 hold the private NZCV state, and t6 is
disposable emitter/address scratch. Entry consumes 64 stack bytes; helper saves
consume another 16..64 bytes. These bridges do not load PSX registers, synchronize
guest cycles around helpers, perform interrupt checks, or implement C argument
marshalling. Those remain compiler integration responsibilities.

`flags.h` holds N/Z as the arithmetic result in t3, C as a boolean in t4 and V
as a boolean in t5. Subtraction carry means no borrow. Fourteen predicates are
materialized without consuming this state. TST explicitly clears C/V as AArch64
does; ARM32 flag-preservation requirements still need auditing at each shared
compiler use. ADC/SBC, conditional flag updates and other flag producers are
not implemented. Initial lowering is correctness-first; once connected, flag
liveness and direct compare/branch lowering should remove unnecessary work.

The flags suite checks 8,544 generated function executions per configuration,
each returning all fourteen predicates. It includes signed overflow and carry
boundaries, deterministic pseudorandom inputs, discarded results, both
source/destination aliases, changed source operands, and a helper which clobbers
caller registers. Expected overflow/carry uses independent 64-bit arithmetic.

## Optional P4 diagnostic firmware

Dynamic dispatch profiling samples approximately one in 1,024 lookup/hash
calls using a pseudorandom selection to avoid locking onto a repeating guest
loop. `profile` records report native-entry wall time for the reporting window,
sampled lookup average in nanoseconds, and its estimated percentage of native
wall time. Native time includes lookup helpers, other helpers and preemption;
the remainder is not pure generated-instruction time. Microsecond timer
quantization, timer overhead and scheduling noise affect the estimate, so
compare several windows. Timing accumulators reset after each progress report.
Progress reports are deliberately spaced 33,554,432 native guest cycles apart.
The earlier 1,048,576-cycle interval produced several seven-line serial bursts
per second and measurably reduced frame rate, so results from that instrumented
run are not comparable with normal runs.

The progress record also reports cumulative top-level fallback opcodes and the
dominant `SPECIAL` and `COP2` subfunctions. This is the admission-roadmap input:
implement instruction families which actually break productive native chains,
then remeasure only after broad guest coverage is in place.

The first HI/LO hardware run reported a shared-compiler self-test failure even
though productive execution admitted the new operations. The ESP-only secondary
interpreter oracle deliberately exposes only the original pure 32-GPR ALU
whitelist; its interface cannot carry HI/LO. The shared test now invokes that
oracle only for its documented subset. HI/LO remains checked by the independent
reference model and explicit generated dependency/edge-case tests. That run also
placed two hot Ridge Racer prefixes at the old 3,072-byte measurement boundary;
the existing 5,120-byte reservation is unchanged, but its fixed measurement
layout initially assigned 3,328 bytes to the prefix and 1,536 bytes to the
branch before the 256-byte dispatcher. After the unaligned-memory family, an
18-op Ridge Racer prefix reached that 3,328-byte boundary exactly. Measurement
now reserves 6,144 bytes (3,840-byte prefix, 2,048-byte branch, 256-byte
dispatcher). Successful blocks still compact the reservation before cache
insertion, so this is miss-time headroom rather than steady-state cache bloat.

The full COP2/GTE coverage pass admits every valid GTE command, register/control
transfer, and `LWC2`/`SWC2`, using a generated-to-C semantic helper with exact
PC, cycle, busy/stall, and MFC/CFC load-delay handling. Its first extended Ridge
Racer run passed the shared selftest and remained native through 3D gameplay.
Compared with the preceding coverage build, the representative 3D interval
improved from roughly 122--225 ms/tick (4--8 FPS) to 90--97 ms/tick (10--12
FPS). `LWC2`, `SWC2`, and GTE commands disappeared from the leading fallback
set; the new dominant boundary was J after a terminal load, especially LWR.

The next build therefore fuses a terminal integer load with a following
supported branch and delay slot. Generated branch condition/indirect-target
capture occurs before the pending value is committed, while the delay slot sees
the committed value. A same-register JAL/JALR link cancels the load before
publishing the link. Debug/release and embedded regressions cover taken and
untaken BEQ, J, JR, and the link collision. Runtime telemetry reports
`load_branches`; hardware performance and compatibility confirmation are
pending.

The first load-branch hardware run confirmed 235 distinct fused blocks and
removed J and JR from the hot fallback set. Interpreter fallbacks fell from
about 1.91 million at 2.60 million native entries in the preceding run to 0.66
million at 1.39 million entries, while the comparable pre-3D interval improved
from roughly 92 to 77 ms/tick. The remaining leading non-HLE fallbacks were
supported loads. These were reached with pending state when a 16-instruction
prefix ended on another load, identifying the diagnostic's artificial block
limit rather than another missing opcode family.

The prefix limit is therefore doubled to 32 guest instructions, with a
34-word fetch/validation window for an optional branch and delay slot. The
measurement reservation grows to 12 KiB but successful blocks remain compact;
the 4 MiB executable arena is unchanged. Cross-page invalidation now clears the
33 possible preceding block starts required by the larger maximum. Telemetry
adds pending-load fallback and capped-prefix counts so the next run can show
whether 32 words is sufficient before considering a general dynamic
pending-entry adapter.

The first 32-instruction hardware run substantially reduced interpreter work
and improved the representative opening interval from about 77 to 63--72
ms/tick, but title-screen blocks at `8001920c`--`8001921c` repeatedly failed
with `RV_BAD_OPERAND`. Their 4.3--4.8 KiB output exposed an RV32 encoding limit:
memory and arithmetic-overflow guards used a conditional B-type branch to an
out-of-line stub, whose +/-4 KiB reach was no longer sufficient. Guards now use
an inverted local condition to skip a JAL, giving the stub transfer roughly
+/-1 MiB of reach. A regression deliberately places the stub beyond 4 KiB and
executes both the normal and exception paths. Debug/release host tests and the
P4 build pass. Hardware subsequently confirmed the long guards through the
title and 3D sections with no compile/runtime/accounting failure.

The confirming run passed every selftest and had no compile/runtime/accounting
failure through 3D. Before 3D it reached 805 million native cycles with about
128 thousand interpreter entries, versus about 435 thousand in the comparable
16-word run. The remaining load-delay cost was concentrated rather than
general: all eight terminal-load blocks were also blocks capped at 32 words,
and their hot execution drove `load_fallbacks` from 469 to 79,256 after 3D
started. The live prefix now uses the restricted compiler's already-tested
64-instruction capacity. Its miss-time reservation grows to 24 KiB, while the
published cache still reclaims unused space and remains 4 MiB. The next run
will determine whether any genuine pending-entry adapter remains worthwhile.

The 64-word run reduced capped blocks from 66 to 13 but did not reduce the hot
3D delay traffic (`load_fallbacks=78,473`, previously 79,256); five remaining
capped blocks still ended on loads. A larger arbitrary cap would only move
these cuts again. Capped straight blocks now retreat over their terminal run of
load instructions. Dispatch therefore starts the successor at the load and
compiles its consumer in the same block, preserving MIPS load-delay semantics
without exporting pending state. Genuine unsupported boundaries retain the
tested interpreter bridge. Telemetry reports these adjustments as
`split_loads`; `capped_loads` should remain zero for adjusted boundaries.

Hardware confirmed the boundary adjustment: five hot capped load endings were
retreated (`split_loads=5`), while `terminal_loads`, `capped_loads`, and
`load_fallbacks` all remained zero through 3D. At roughly 906 million native
cycles interpreter entries fell from about 207 thousand to 128 thousand, and
the representative 3D interval improved from about 91 ms/tick (roughly 11 FPS)
to 76--78 ms/tick (roughly 13--14 FPS). The dominant remaining fallback was
opcode `0x3b`, PCSX's synthetic HLE trap rather than a guest CPU instruction.
The dispatcher now handles valid HLE traps directly using the same terminal
host-call contract as Lightrec and the original new_dynarec. Nested HLE CPU
execution remains with Lightrec; direct call and cycle counts are reported as
`hle` telemetry.

The confirming direct-HLE run removed opcode `0x3b` from the fallback
histogram entirely. At roughly 906 million native guest cycles it recorded
122,567 direct HLE calls and only 5,897 interpreter entries; at 1.14 billion
cycles those totals were 123,405 and 6,735 respectively. Frame time remained
about 75--78 ms in the representative 3D section, showing that the HLE
handlers' own work, rather than their former dispatch boundary, dominates
that path. COP0 (`0x10`) was then the clear remaining fallback family.

COP0 coverage now includes delayed `MFC0`, all `MTC0` register writes, and
`RFE`. Status/Cause writes use the emulator's established cache-isolation,
coprocessor and interrupt machinery; Status/Cause and RFE end a native block
because they may immediately vector to an exception. Blocks are split before
such an operation when an earlier load is still pending. This avoids executing
stale generated instructions after a control transfer while retaining the
full valid COP0 family. Separate fallback telemetry reports the COP0 `rs`
sub-operation if malformed/reserved encodings still reach the interpreter.

Hardware confirmed the corrected COP0 selftest and the complete Ridge Racer
path through 3D without a compile, runtime or accounting failure. At roughly
906 million native guest cycles the interpreter total fell from 5,897 to 543;
COP0 fell from 5,354 entries to zero. Representative 3D frame time remained
about 74--79 ms, so instruction coverage is no longer the primary bottleneck.
The remaining fallbacks were 490 `BLEZ`, 34 `BNE`, 11 REGIMM and six syscalls.

The next optimization mirrors an important established Lightrec/new_dynarec
path: GTE commands whose FLAG output is provably overwritten before a CFC2
FLAG read now use the no-FLAG handlers. The analysis is deliberately local to
one straight-line native prefix; the final command at a block boundary always
computes full flags. ESP32-P4 `INTPL` also selects the existing SF/LM-specialized
handler. Runtime telemetry reports GTE call count, no-FLAG share and sampled
native cycles so this change can be evaluated in the same gameplay run.

Hardware confirmed the no-FLAG path with no correctness failures. In Ridge
Racer's 3D section roughly 58--59% of 248,000--295,000 GTE commands per
four-second reporting window used it. Sampled handler work accounted for about
103--129 million P4 cycles per window (roughly 7--9% of one core), while frame
time remained about 74--80 ms. GTE is therefore material but cannot explain the
remaining gap to Lightrec by itself.

True GTE commands now preserve allocator mappings held in ABI callee-saved
RV32 registers. Only caller-saved mappings are written back before the C GTE
handler; COP2 transfers and LWC2/SWC2 retain the conservative full barrier.
This avoids throwing away surrounding ALU state at each of the tens of
thousands of GTE calls per second. The P4 differential selftest carries a dirty
GPR result across two GTE calls and verifies both that result and complete CP2
state against the established handlers.

Straight runs of non-trapping integer ALU operations now defer their guest
cycle increments and materialize the total with one RV32 `ADDI` before the next
helper, possible exception, or block exit. Trapping arithmetic still updates
the counter before its overflow check, and memory/COP/HI-LO barriers retain
instruction-precise timing. The shared differential suite verifies dependent
GPR results, final cycle totals, load delays and overflow exits. Cumulative
`cycle_adds_saved` telemetry reports how many emitted additions were removed
from published live blocks.

Terminal branch blocks now retain allocator mappings held in RV32
callee-saved registers across the internal prefix-to-branch and
branch-to-delay-slot jumps. Caller-saved mappings are published before target
selection, while `s2`--`s11` mappings remain live; branch operands consume
those mappings directly, link writes update a mapped destination in place, and
the delay-slot compiler inherits the resulting map. The prior layout flushed
all dirty prefix results, reloaded branch operands from `psxRegs`, then began
the delay slot with another empty map. A differential test deliberately makes
the canonical GPR copies stale while evaluating both taken and untaken paths.
Runtime `branch_maps` telemetry counts mappings preserved in published blocks.
This is an intra-block step toward the larger remaining requirement: a stable
register contract across arbitrary dispatcher-linked blocks.

### Guest instruction coverage roadmap

The original new_dynarec decoder and instruction-type switch are the canonical
completeness checklist. Lightrec and the interpreter are the references for the
current PCSX-ReARMed helper ABI, exception behaviour, hardware timing and GTE
semantics. The restricted RV32 compiler currently covers:

- integer ALU/immediate and fixed/variable shift operations;
- the complete HI/LO family (`MULT/MULTU/DIV/DIVU` and
  `MFHI/MFLO/MTHI/MTLO`), including divide corner cases and busy-cycle stalls;
- aligned `LB/LBU/LH/LHU/LW` and `SB/SH/SW` with a direct mirrored-main-RAM
  path and LUT fast paths for other mappings;
- unaligned `LWL/LWR/SWL/SWR`, including all four byte alignments,
  same-register pending `LWL/LWR` merges, and unaligned stores in branch delay
  slots. Their first correctness lowering uses a PCSX memory helper so MMIO
  semantics match the interpreter; direct-LUT inlining remains an optimization;
- terminal integer loads at straight-block boundaries. The native producer
  exports a canonical slot-zero `dloadReg`/`dloadVal` with `dloadSel == 0`, and
  its dispatcher is deliberately left unlinked so `execI` consumes the next
  instruction and retires the delay. This removes load replay but is an
  intermediate bridge; a fully general native successor-entry adapter remains
  to be added. When the immediate successor is a supported branch, the load,
  branch and delay slot are fused, preserving both old-value branch semantics
  and new-value delay-slot semantics;
- `J/JAL/JR/JALR`, `BEQ/BNE/BLEZ/BGTZ`, and REGIMM branches when the delay
  instruction is an admitted ALU operation or store;
- all valid PSX GTE commands, `MFC2/CFC2/MTC2/CTC2`, and `LWC2/SWC2`, including
  GTE busy-cycle stalls and delayed CPU-register writes from `MFC2/CFC2`;
- PCSX HLE traps, with nested HLE CPU execution deliberately retained by the
  established core;
- valid COP0 `MFC0`, `MTC0`, and `RFE`, including MFC0 load delay and
  Status/Cause/RFE exception-sensitive block termination.

Remaining families, to be admitted according to the runtime histogram, are:

- general native consumption of load-delay state at arbitrary successor block
  entry (including faulting instructions and same-register `LWL/LWR` merges);
- syscall/break and reserved-instruction exceptions;
- currently unsupported instruction classes in branch delay slots.

Coverage is only the first performance milestone. Once interpreter boundaries
are rare, profile direct block linkage, cross-block register preservation and
generated memory paths; optimizing those earlier would measure a workload that
is still dominated by incomplete native chains.

After a normal P4 build has configured the application's build directory:

```powershell
cmake -S pcsx_rearmed -B pcsx_rearmed/build -DPCSX_NDRC_RV32_SELFTEST=ON
python rg_tool.py --target gb300-p4 build pcsx_rearmed
```

The options are app-local and default ON for ESP32-P4 and OFF for other chip
families; cached values persist until changed or the build directory is cleaned.
To remove the diagnostic, configure the same options OFF and rebuild. No shared
target configuration needs changing.

The diagnostic runs after Lightrec's existing executable-memory smoke test but
before Lightrec initializes its block allocator. It borrows the first 16 KiB of
the empty executable arena, without allocating another JIT buffer. It uses its
own P4 cache-publication callback, executes the same tests, clears the scratch
range, and then lets Lightrec initialize normally. A returning test failure is
logged; the unimplemented RV32 guest core is never selected. A hardware execution
fault can still panic or hang, so retain the known-working firmware for recovery.

Expected serial messages:

```text
new_dynarec RV32 emitter selftest starting (game core remains Lightrec)
new_dynarec RV32 emitter selftest PASSED (not a guest CPU test)
new_dynarec RV32 ABI selftest starting
new_dynarec RV32 ABI selftest PASSED (game core remains Lightrec)
new_dynarec RV32 flags selftest starting
new_dynarec RV32 flags selftest PASSED (game core remains Lightrec)
new_dynarec RV32 shared compiler selftest starting (real state; cross-block load delay; PCSX cross-check)
new_dynarec RV32 shared compiler selftest PASSED (game core remains Lightrec)
```

Capture startup through the result and confirm the game still starts. A PASS
validates this emitter on executable PSRAM, not PSX compatibility or speed.

## Integration work still required

1. **Host-register adapter integration and predicates.** The shared compiler assumes host
   slots 0 onward are C argument registers. RV32 hardware x0 is immutable zero,
   and x1..x4 are ra/sp/gp/tp, so connect the tested slot-to-hardware mapping. Preserve gp/tp,
   the ABI callee-saved registers and 16-byte stack alignment. Reserve a local
   state pointer, a callee-saved cycle counter, and emitter scratch registers.
   ARM flags do not exist on RV32: audit each compare/test/arithmetic-flags
   consumer, including carry/overflow and predicates surviving other emissions.
   Do not just alias ARM conditional helpers to signed RV32 branches.
2. **Linkage and event exits.** Connect the tested entry/leave and helper bridges;
   implement block lookup/linking and interrupt exits. Reconcile assembly-owned `psxRegs`,
   `rcnts` and linkage offsets with the current port's C definitions and stubs.
   Keep GTE/CP0 state and event deadlines correct across calls and exits.
3. **ESP32 memory integration.** Replace desktop mmap/mprotect paths with owned
   executable PSRAM allocation/publication. The upstream RV32-sized read/write
   tables alone need 8 MiB; audit this alongside translation cache, block metadata,
   PSX RAM/VRAM and current internal-memory headroom before choosing cache sizes.
   Never assume SP/GP always reference RAM: Destruction Derby uses scratchpad.
4. **Guest correctness.** Test MIPS delay slots, load delays, exceptions,
   overflow/division corner cases, unaligned loads/stores, RAM mirrors,
   scratchpad, hardware accesses, cache isolation and self-modifying code.
   Differential tests must precede performance comparisons.
5. **Explicit experimental core selection.** Add the RV32 compiler sources and
   select its `psxRec` only once the above paths exist. Keep a build-time Lightrec
   alternative. Do not silently claim a game boot through interpreter fallback
   as a dynarec success.
6. **Benchmark identical scenes.** Compare emulated tick time, rendered frames,
   emitted code size and cache behaviour for Ridge Racer and Destruction Derby.
   Extend compatibility testing beyond both games. A large FPS gain is a
   hypothesis, not guaranteed: GTE, GPU and PSRAM bandwidth costs remain.

The gbSP RV32 emitter/stubs are useful references for ISA encoding, ABI and P4
execution, but its fixed GBA register mapping and ARM guest flag handling do not
transfer directly to this MIPS compiler. Existing ARM/AArch64 new_dynarec backends
remain the references for its shared compiler contracts.
