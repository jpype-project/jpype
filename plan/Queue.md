# python.queue: SPI provider for Python's queue module

## Status (2026-07-11): scoped, not started, opportunistic (lower priority than Datetime/Decimal/Pathlib)

## The problem

No Java front-end exists for `queue.Queue`/`LifoQueue`/`PriorityQueue`.
Narrower audience than `datetime`/`decimal`/`pathlib`, but specifically
worth considering given how much subinterpreter/GIL/async groundwork
already exists in this repo ([[jpype_gil_reacquire_bug]],
[[jpype_subinterpreter_difficulty]], [[jpype_multiphase_init_status]],
`PyCallable`'s `callAsync`/`callAsyncWithTimeout`) — a typed front-end for
cross-thread handoff between a Java thread and a Python
worker/subinterpreter plays directly to that strength, unlike a purely
opportunistic addition.

See `plan/archive/Collections.md` for the general "why SPI, not core" rationale
and `plan/archive/SPI.md` / [[jpype_spi_installer_status]] for the
mechanism.

## Scope

- `queue.Queue` → `PyQueue` — `put`/`get` (blocking and non-blocking
  variants, `put_nowait`/`get_nowait`), `qsize`/`empty`/`full`, `task_done`/
  `join`.
- `queue.LifoQueue`, `queue.PriorityQueue` — same protocol as `Queue`
  (Python implements them as subclasses with a different internal
  ordering); likely `PyLifoQueue extends PyQueue` /
  `PyPriorityQueue extends PyQueue` with no new methods, verify against a
  real interpreter rather than assuming no divergence.

## Design questions to resolve before coding

- Blocking `get()`/`put()` calls hold the Python-side GIL/thread while
  waiting. Given this repo's own hard-won subinterpreter/GIL correctness
  work, this is the one type in this whole SPI batch where getting the
  threading story right matters more than the method surface — do not
  treat this as mechanical the way `Collections`/`Datetime` mostly are.
  Confirm blocking calls interact safely with the existing
  callAsync/subinterpreter machinery before shipping, ideally with a
  dedicated concurrency stress test modeled on
  [[jpype_globalpool_allocator_status]]'s or
  [[jpype_inception_finding]]'s test approach.
- Whether `get(timeout=...)`'s `queue.Empty` exception needs its own
  `python.exceptions` mapping (it's a real, catchable Python exception a
  caller needs to distinguish from other failures) — check whether it
  already exists in the `python.exceptions` package before assuming it
  needs adding.

## Naming-mangling interaction

Same caveat as the other plans in this batch: verify the current `$`/`.`
mangling convention against a live `.pyspi` file before writing new ones.

## Steps (mirror `plan/archive/Collections.md`)

1. Resolve the threading-safety design question above first — this is a
   prerequisite, not a detail to fill in later.
2. Design `PyQueue`/`PyLifoQueue`/`PyPriorityQueue`'s method surfaces.
3. `.pyspi` resources under
   `native/jpype_module/src/main/resources/python/queue/spi/`.
4. `PyQueueWrapperService` implementing `WrapperService`, registered in
   `module-info.java`.
5. End-user Javadoc, `python/queue/package-info.java`.
6. Tests: single-thread correctness plus a real concurrent
   producer/consumer stress test across the Java/Python boundary — this
   type's whole point is concurrent use, so don't ship it verified only
   single-threaded.

## Verification

- Full suite green on python3.10 and python3.12.
- The concurrency stress test above must pass repeatably (run it multiple
  times, not once) given how much of this repo's history is subinterpreter
  race conditions that only reproduced intermittently.
</content>
