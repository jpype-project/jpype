# SubInterpreterBuilder with configurable PEP 684 knobs

## Status (2026-07-11): DONE, implemented and tested; default flipped to safest option (2026-07-11, later)

Since this builder API never shipped, there was no prior behavior to stay
backward-compatible with, so the default (a bare `new SubInterpreterBuilder()`)
was changed from the shared-GIL/shared-obmalloc combination to the safest
legal one: own GIL, own obmalloc, `check_multi_interp_extensions` enabled —
identical to `ownGil()`, which now exists purely so call sites can say so
explicitly. The old default preset, previously named `legacy()`, was
renamed `elevated()` since "legacy" implied a prior shipped behavior that
doesn't exist; it remains available for callers that need to opt out of
isolation (e.g. importing a single-phase-init extension). `SubInterpreter.
start()` (the plain, builder-less entry point) is unaffected — it keeps its
own historical fixed shared-GIL/shared-obmalloc values, unrelated to the
builder's default.

**Regression found and fixed while flipping the default:**
`testIllegalCombinationRejectedBeforeNativeCall` had hardcoded
`.without(USE_MAIN_OBMALLOC)` to build an illegal combination, relying on
the *old* default (`USE_MAIN_OBMALLOC` on) to make that removal the only
thing needed. Against the new default (`USE_MAIN_OBMALLOC` already off,
`CHECK_MULTI_INTERP_EXTENSIONS` already on), that no longer produces an
illegal combination — `validate()` correctly passed, `start()` launched a
real own-GIL subinterpreter, and the test discarded the return value
without closing it, which is a fatal, uncatchable
`PyInterpreterState_Delete: remaining subinterpreters` crash at JVM
shutdown (see the odd-ball scenario above — same failure mode, different
trigger). Reliably reproduced 5/5 runs before the fix, 0/5 after. Fixed by
having the test explicitly disable `CHECK_MULTI_INTERP_EXTENSIONS` instead
of `USE_MAIN_OBMALLOC`, which is illegal regardless of default. Worth
remembering: a test that builds "the illegal combination" by removing a
single flag from the current default is implicitly coupled to whatever
that default currently is — safer to build the illegal combination
explicitly than to rely on a relative mutation of
`new SubInterpreterBuilder()`.

Implemented as designed: `org.jpype.SubInterpreterBuilder`
(`native/jpype_module/src/main/java/org/jpype/SubInterpreterBuilder.java`),
`EnumSet<Option>` for the seven `PyInterpreterConfig` flags, plain setters
for stdio. `NativeLauncherControl.startSubInterpreter` and
`jp_bridge.cpp`'s implementation now take the seven flags as parameters
instead of a hardcoded `PyInterpreterConfig` literal.
`SubInterpreter.start()` (no-arg) is unchanged in behavior - it now calls a
new package-private `start(boolean...)` overload with the same fixed
legacy values it always used.

Cross-field validation confirmed directly against the vendored CPython
checkout at `~/jcef/cpython/Python/pylifecycle.c`'s
`init_interp_settings()`: the only hard rule enforced there is
`use_main_obmalloc=0` requires `check_multi_interp_extensions=1` (no
separate `gil`/`OWN_GIL` cross-constraint exists in that function) -
`SubInterpreterBuilder.validate()` checks exactly that one rule.

Tests: `native/jpype_module/src/test/java/python/lang/
SubInterpreterBuilderNGTest.java` (7 tests - `elevated()`-matches-plain-start
parity, a dedicated default-is-safest-not-elevated test, `ownGil()` smoke
test confirming a real own-GIL/own-obmalloc subinterpreter still works now
that `_jpype` is multi-phase-init-safe, illegal-combination rejection,
stdio wiring via the builder, `asSupplier()` launching independent
instances, try-with-resources closing a `SubInterpreter` automatically).
All pass under both python3.12 and python3.10 native builds (6 of the 7
skip under 3.10 via the same version-gate pattern as `SubInterpreterNGTest`;
the validation-rejection test does not skip, since it never reaches the
version check). Full existing `SubInterpreterNGTest`/`InterpreterStdioNGTest`
suites re-verified passing against the changed native signature.

**Odd-ball scenario also tested:** `test/jpypetest/test_inception.py` -
forward-bridge Python (Python hosts the JVM) using `SubInterpreterBuilder`
via Java to launch and pilot a *second* CPython subinterpreter from
inside Python itself. `exec_()` (Python-side rename of `Script.exec`,
which collides with the builtin) works fine; `eval()` results crossing
back into the outer/host interpreter correctly hit the Smuggler guard
(`plan/Smuggler.md`) instead of corrupting memory, surfacing as a plain
Python `RuntimeError` (not `jpype.JException` - it fires during `_jpype`'s
own conversion path, not Java exception marshaling). Also confirmed: an
unclosed `SubInterpreter` left dangling when an exception skips `.close()`
is a **fatal**, uncatchable CPython error at process shutdown
(`PyInterpreterState_Delete: remaining subinterpreters`), which is why
`doc/userguide.rst` now explicitly recommends try-with-resources/`with`
over a bare `.close()` call - both work for free since `SubInterpreter`
implements `AutoCloseable` (Java try-with-resources is a language
guarantee; Python's `with` comes from JPype's blanket `AutoCloseable`
customizer in `jpype/_jio.py`).

Follow-on from `plan/JepParity.md` item 4. Motivated by comparing against
jep's `SubInterpreterOptions` (obmalloc, allowFork/Exec/Threads, ownGIL,
checkMultiInterpExtensions) — jpype currently has no way to configure any of
this per-instance.

## Current state

`native/common/jp_bridge.cpp`'s `Java_org_jpype_internal_NativeLauncherControl_startSubInterpreter`
hardcodes a single fixed `PyInterpreterConfig`:

```cpp
PyInterpreterConfig config = {
    .use_main_obmalloc = 1,
    .allow_fork = 0,
    .allow_exec = 0,
    .allow_threads = 1,
    .allow_daemon_threads = 0,
    .check_multi_interp_extensions = 0,
    .gil = 0   // shared GIL, legacy-style, not real PEP 684 isolation
};
```

Per `plan/MultiPhaseInit.md` (COMPLETE 2026-07-11), `_jpype` is now built as
a proper multi-phase-init extension and is eligible for
`check_multi_interp_extensions=1` + `gil=PyInterpreterConfig_OWN_GIL` — a
genuinely isolated own-GIL subinterpreter is now reachable, it's just never
requested because the config is hardcoded to the legacy-compatible values.

`org.jpype.SubInterpreter` (Java side) has no constructor/builder
parameters at all — `start()` takes nothing.

## Goal

A single class, `org.jpype.SubInterpreterBuilder`, that is *itself* the
builder — deliberately modeled on `java.lang.ProcessBuilder` rather than the
classic immutable-`Config`-plus-separate-`Builder` split: a mutable,
reusable object you configure with setters, then `.start()` repeatedly to
launch independently-configured `SubInterpreter` instances, e.g.:

```java
SubInterpreterBuilder builder = new SubInterpreterBuilder()
    .with(SubInterpreterBuilder.Option.OWN_GIL,
          SubInterpreterBuilder.Option.CHECK_MULTI_INTERP_EXTENSIONS)
    .without(SubInterpreterBuilder.Option.ALLOW_FORK,
             SubInterpreterBuilder.Option.ALLOW_EXEC)
    .setOutput(capturedOut);

SubInterpreter sub = builder.start();   // launches and starts it
SubInterpreter sub2 = builder.start();  // same config, independent instance
```

Like `ProcessBuilder.start()`, `.start()` returns an already-launched
instance (not a lazy `Supplier`) and the builder itself remains reusable
afterward for further launches with the same configuration.

A second terminator, `.asSupplier()`, adapts the same builder to
`Supplier<SubInterpreter>` for callers that need to hand a lazy source to
something else (e.g. a worker pool pulling a fresh subinterpreter per
task) instead of launching immediately:

```java
Supplier<SubInterpreter> supplier = builder.asSupplier();  // this::start
...
SubInterpreter sub = supplier.get();  // launches on demand
```

It's a thin adapter (`return this::start;`) — no new state or validation
path, just a deferred `.start()` call for interop with `Supplier`-shaped
APIs.

## Design sketch

- `SubInterpreterBuilder.Option` — an `enum` of the seven boolean-ish
  `PyInterpreterConfig` knobs (`USE_MAIN_OBMALLOC`, `ALLOW_FORK`,
  `ALLOW_EXEC`, `ALLOW_THREADS`, `ALLOW_DAEMON_THREADS`,
  `CHECK_MULTI_INTERP_EXTENSIONS`, `OWN_GIL`), so the on/off state lives in
  a single `EnumSet<Option>` field rather than seven separate booleans.
  `.with(Option...)`/`.without(Option...)` mutate that set and return
  `this` for chaining. Default set matches today's hardcoded legacy
  behavior (`USE_MAIN_OBMALLOC`, `ALLOW_THREADS` on; everything else off) —
  `new SubInterpreterBuilder()` with no calls behaves exactly like
  `SubInterpreter.start()` does today.
- Non-flag knobs get ordinary setters directly on the builder rather than
  enum entries — e.g. `setOutput`/`setError`/`setInput` (reusing the
  `Writer`/`OutputStream`/`Reader`/`InputStream` overloads already on
  `Interpreter` from `plan/StreamRedirect.md`): the builder just stores the
  stream and calls `sub.setOutput(...)` etc. on the freshly-started instance
  before returning it from `.start()`. No new stdio-wiring logic needed —
  it's the same `installStdio` machinery, just invoked by the builder
  instead of by the caller after the fact.
- Validate the known-illegal combination inside `.start()` before touching
  native code, rather than letting it fail obscurely in C: per the existing
  comment in `jp_bridge.cpp` (`startSubInterpreter`), CPython's
  `init_interp_settings` (`Python/pylifecycle.c`) requires
  `USE_MAIN_OBMALLOC` on whenever `CHECK_MULTI_INTERP_EXTENSIONS` is off —
  read that function again when implementing to confirm the exact legal
  combinations (including whatever `OWN_GIL` requires), don't just invert
  the comment from memory. Throw `IllegalStateException` with a clear
  message on violation.
  Provide two named presets as static factory methods returning
  pre-configured instances: `SubInterpreterBuilder.ownGil()` (own GIL, own
  obmalloc, `check_multi_interp_extensions` enabled — the safest legal
  combination, and also the default state of a bare
  `new SubInterpreterBuilder()`, since this API never shipped and there is
  no prior behavior to default to for compatibility) and
  `SubInterpreterBuilder.elevated()` (the shared-GIL/shared-obmalloc
  combination `SubInterpreter.start()` has always used, for callers that
  need to opt out of isolation to import single-phase-init extensions or
  share the main obmalloc/GIL).
- `startSubInterpreter`'s native call gains an overload (or parameter)
  threading the resolved `PyInterpreterConfig` values down instead of the
  hardcoded literal in `jp_bridge.cpp`.
- Backward compatibility: `SubInterpreter.start()` (no-arg, on an instance
  constructed directly) continues to use the legacy hardcoded config — no
  behavior change for existing callers/tests. `SubInterpreterBuilder` is an
  additive way to get a configured instance, not a replacement for the
  existing constructor/`start()` path.

## Testing

Done - see "Status" above for what `SubInterpreterBuilderNGTest` actually
covers.

**Deferred, not done this pass:** rerunning
`SubInterpreterNGTest.testSmuggledProxyAcrossInterpretersThrows` (see
`plan/Smuggler.md`) under an `ownGil()`-configured pair of subinterpreters
instead of the default legacy shared-GIL pair it uses today - genuine
own-GIL isolation is the scenario the Smuggler guard was actually written
to protect against, so proving it under real isolation (not just the
legacy config that happens to already be safe-by-sharing) closes a real
gap, but it's a follow-on to this plan rather than part of it - it touches
`SubInterpreterNGTest`, not `SubInterpreterBuilder` itself.

## Open questions

- Should `checkMultiInterpExtensions=1` be rejected outright until every
  other extension module `_jpype`'s subinterpreter might try to `import`
  (numpy, etc.) is also multi-phase-init-safe, or is that the caller's
  problem to discover via CPython's own `ImportError`? Resolved as
  designed: caller's problem - `SubInterpreterBuilder` doesn't pre-validate
  the transitive import graph, just passes the config through and lets
  CPython's own checks fire (confirmed via `testOwnGilLaunchesAndImportsJpype`,
  which only imports `_jpype` itself and succeeds cleanly).
