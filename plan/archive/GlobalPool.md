# GlobalPool allocator: benchmark-driven fix, not a design change

## Status (2026-07-11, even later): DONE - pool-identity stamp added, closing the wraparound gap

The generation-counter removal below was correct for what it removed
(per-slot reuse-within-a-pool detection - genuinely unneeded, since C++
lifetime discipline already rules it out). But the user's original intent
for "generation bits" was a different, still-real concern I'd flagged as
an unaddressed open follow-up: the 16-bit prefix wraps after 65536 pool
constructions in one process, so a stale handle's prefix can collide with
a *different*, later pool that reused that same prefix number - a routing
error, not a lifetime one, and a real (if rare) possibility given a
long-running process opening/closing many subinterpreters over time.

Fix: each pool now also stamps a hash of its owning interpreter's native
context pointer - computed once at construction, not per-call - into
every handle it mints (`STAMP_SHIFT`/`STAMP_MASK`, reusing the 16 bits the
old per-slot generation vacated). `tryRelease` checks the resolved pool's
stamp against the handle's before touching anything; on mismatch it logs
via `java.util.logging.Logger` and safely no-ops instead of trusting a
same-prefix pool that didn't actually mint the handle. `get()` checks the
same stamp for symmetry. The hash uses a MurmurHash3-style 64-bit
finalizer (`mixStamp`), not a raw `Long.hashCode`, so the stamp's low bits
don't correlate with a context pointer's own low bits (relevant for the
test below, where "context pointers" are sequential small longs).

`GlobalPool(long contextAddress)` is now the real constructor;
`NativeContext.java` passes its own native `contextAddress` in (moved
`globalPool`'s construction from a field initializer into the constructor
body, right after `contextAddress` is assigned, since it needs that
value). The no-arg `GlobalPool()` remains for tests only, seeding the hash
from `ThreadLocalRandom` instead of a real pointer.

Cost: unchanged from before - this reuses bits that were sitting unused
after the generation removal, so there's no new per-slot memory, and the
check is one extra `int` comparison on an already-decoded field, not an
extra lock or per-call hash.

New test: `testStampMismatchAfterPrefixWraparoundIsSafelyDropped` -
constructs up to 70,000 pools (real wraparound, not simulated) until a
prefix collision actually occurs, takes a handle from the original owner
of that prefix, and confirms `tryRelease` on it does not touch the new
pool that now holds that prefix (block count unchanged, the new pool's
own live handle still resolves). The warning log line was observed firing
during the run, confirming the mismatch path actually executed. Full
suite: 555/555, 0 failures/errors.

## Status (2026-07-11, later): generation counter also dropped, correctly

After the allocator swap below, user pushback caught a second, unrelated
mistake: the per-slot generation counter (present before this session,
kept unchanged through the allocator rewrite) was solving a problem that
doesn't exist. It was meant to guard against a released handle resolving
to whatever now occupies its old (reused) slot. But a handle is only ever
resolved (`get`) through its live owning C++ wrapper (`JPClass`/
`JPMethod`/`JPField`/...), and that same wrapper's own destructor is the
only thing that ever calls `tryRelease` on it, exactly once (confirmed via
`jp_field.cpp`, `jp_method.cpp`, `jp_class.cpp`, `jp_array.cpp`,
`jp_buffer.cpp`, `jp_monitor.cpp`, `jp_proxy.cpp`, plus `jp_value.h`'s
`JPValue::getJavaObject`/`fromGlobal`). C++ object-lifetime rules alone
mean nothing can call `get()` on a handle after its owner's destructor
already ran - that would require using an already-destroyed object. So a
released handle is never resolved again by anything, whether or not its
slot has since been reused - the same guarantee level JNI's own global
ref slots already have (JNI reuses ref slots constantly with no
generation check). The counter was manufacturing a safety net for a
scenario the call discipline already rules out by construction - unneeded
complexity and per-slot memory cost (an `int` per slot, doubling the
`Object[64]` block's footprint) for zero actual protection.

Removed entirely: `generations` array, `GENERATION_SHIFT`/
`GENERATION_MASK` constants, `decodeGeneration`, and the generation checks
in `get()`/`remove()`; `encode()` dropped its `generation` parameter.
Prefix + registry routing (the actually-correct per-interpreter design)
is untouched. `GlobalPoolNGTest`'s `testTryReleaseThenReuseGivesDistinctHandle`
became `testReleasedSlotIsReusedForNextAdd`, asserting the slot **is**
reused (same handle value) rather than asserting the old handle goes
stale - the previous test was asserting the now-removed behavior. Full
suite re-verified: 553/553, 0 failures/errors.

## Status (2026-07-11): DONE, benchmarked, implemented, tested

`org/jpype/ref/GlobalPool.java` carried two FIXMEs since it was written:
multiple responsibilities (registry + per-pool storage) and a "large shift
from j2ni/J2GlobalPool design." Investigated why, at the user's request.

## What's actually correct vs. what was wrong

The user's read, confirmed correct: **per-interpreter pool instances plus a
central prefix-keyed router are the right design.** J2NI has exactly one
process because it's a single JVM-connection bridge; jpype's reverse bridge
supports multiple concurrently-live Python subinterpreters (PEP 684,
`plan/SubInterpreterBuilder.md`), each with its own lifetime, so each needs
its own pool that dies with its `NativeContext` (`GlobalPool.close()`), and
native destructors calling `tryRelease(long)` with no other context in hand
need a way to route to the right one (or safely no-op if it's already torn
down). None of that is wrong and none of it changed here.

**What was wrong: the internal allocator.** The original implementation
backed each pool with a fully-`synchronized` `ArrayList<Object>` +
`ArrayList<Integer>` (generations) + `ArrayDeque<Integer>` (free list) -
every `add`/`get`/`remove` took the pool's monitor. J2NI's `J2GlobalPool`
(built on `J2ReferencePool64`) uses 64-element blocks with a `long`
occupancy bitmask per block and `VarHandle` acquire/release for reads, so
`get()` takes no lock at all - only structural mutation
(`add`/`remove`/block growth) synchronizes.

## Benchmark methodology

Built two standalone (non-Maven, no jpype deps) prototypes in scratch and
compared them head-to-head on a 16-core machine, plain `java` process, no
JMH: `GlobalPoolSync` (an exact copy of the shipped code, renamed) vs.
`GlobalPoolBlock` (J2NI's block/bitmask/VarHandle allocator, same external
contract: prefix, generation, static registry, `tryRelease`). Three
scenarios, 3 runs each after a warmup pass:

1. **Single-threaded add/get/release churn** - baseline, no contention.
2. **Concurrent `get()` only**, N reader threads against a fixed
   pre-populated pool - the realistic hot path, since `get()` is called
   from JNI on effectively every native-to-Java handle resolution.
3. **Mixed**: N reader threads + 1 writer thread continuously
   `add`/`tryRelease` churning - simulates object creation/GC pressure
   happening concurrently with normal handle resolution.

## Results (16 cores)

| Scenario | sync | block | speedup |
|---|---|---|---|
| single-thread churn | ~21M ops/s | ~25M ops/s | 1.17x |
| `get()`, 1 thread | 39M | 46M | 1.17x |
| `get()`, 2 threads | 49M | 88M | 1.8x |
| `get()`, 4 threads | 13M (**drops**) | 175M | 13x |
| `get()`, 16 threads | 16M | 575M | **36x** |
| mixed, 4 readers + 1 writer | 11M | 102M | 9x |
| mixed, 16 readers + 1 writer | 13M | ~330M | **~26x** |

The sync version's `get()` throughput doesn't plateau under contention, it
**collapses** (39M at 1 thread -> 13M at 4 threads) because every reader
serializes on the same monitor as every writer even though nothing
logically conflicts. The block/bitmask design scales close to linearly
with core count.

## Implementation

Ported the block/bitmask allocator directly into
`native/jpype_module/src/main/java/org/jpype/ref/GlobalPool.java`, keeping
the external contract identical (constructor, `add(Object)`, `get(long)`,
`close()`, static `tryRelease(long)`) - confirmed via `grep` that the only
JNI-bound entry point is the static `tryRelease` (`jp_context.cpp`'s
`bindGlobalPoolRelease`, signature `(J)V`); `add`/`get`/`close` are called
only from Java (`NativeContext.java`), so no native-side signature could
have been affected either way.

New test: `src/test/java/org/jpype/ref/GlobalPoolNGTest.java`, 10 tests -
null-handle special case, cross-pool handle rejection, stale-generation
rejection after `tryRelease`/reuse, post-`close()` no-op and `get()`
returning null, multi-block allocation/round-trip (exceeds `BLOCK_SIZE`),
freed-slot reuse (doesn't leak a new block), and a concurrent stress test
(N reader threads hammering `get()` while one writer thread continuously
`add`/`tryRelease`s, asserting no exception/corruption is ever observed -
the regression guard for this exact rewrite).

Full suite: 553 tests, 0 failures, 0 errors (was 543 before the 10 new
tests), verified after the rewrite.

## FIXMEs resolved

- "Large shift from j2ni/J2GlobalPool design" - the *routing* layer
  (prefix + registry) is a deliberate, correct divergence forced by
  per-interpreter lifetime; only the *storage* layer needed to match
  J2NI's approach, and now does.
- "Multiple responsibilities: registry and individual pool" - left as-is.
  Splitting the static registry out into its own class was considered but
  rejected as premature: the registry is a single `ConcurrentHashMap`
  keyed by prefix with two call sites (`tryRelease`, constructor/`close`)
  - there's no second responsibility pulling on it that would justify the
  extra indirection.

## Open follow-up, not benchmarked

The static prefix `REGISTRY` is still a `ConcurrentHashMap`, touched on
pool construction, `close()`, and every `tryRelease()` call (native
destructors - `jp_field.cpp`, `jp_method.cpp`, `jp_class.cpp`,
`jp_array.cpp`, `jp_buffer.cpp`, `jp_monitor.cpp`, `jp_proxy.cpp`,
`jp_bridge.cpp`). Assumed to be off the hot path relative to `get()`
(object teardown is far less frequent than handle resolution), but that's
an assumption, not a benchmarked conclusion - if `tryRelease` frequency
ever becomes suspect, benchmark it the same way before touching it,
rather than assume `ConcurrentHashMap` is fine.
