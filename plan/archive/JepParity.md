# Jep parity survey — corrected findings and follow-on plans

## Status (2026-07-11, later): all four follow-on items CLOSED

Item 2 (numpy interop) and item 3 (stdout/stderr redirect) are now also
closed - see the updated write-ups below. Items 1 and 4 were already
closed. Nothing remains open in this survey.

## Status (2026-07-11): survey done, corrected by user, follow-on work scoped into sub-plans below, nothing implemented yet

Background: jep 4.3.1 (`~/jcef/jep`) is a mature Java-calls-Python bridge,
same direction as jpype's reverse bridge. jep's own test suite only runs one
test in this environment, so confidence that jpype's reverse bridge actually
covers jep's use cases was low. An Explore-agent survey compared jep's public
API against `python.lang`/`python.io`/`python.exceptions`. The raw survey is
saved in memory (`jpype_jep_parity_survey`); this doc is the corrected,
user-reviewed version driving actual follow-on work.

## Corrections to the raw survey

1. **Thread attach is NOT a gap — it's a deliberate design win, don't build it.**
   The survey flagged jep's `Interpreter.attach(shareGlobals)` (manual
   per-thread attach/detach of a running interpreter) as something jpype
   lacks. User: jpype automatically handles ALL thread attachment; manual
   attach in jep was "a segfault magnet" — "every IDE managed to crash it."
   jpype's automatic model is the correct design. No action item here beyond
   not regressing it.

2. **numpy interop is likely NOT a real gap — verify, don't build numpy ties.**
   The survey claimed `python.lang.PyBuffer`/`PyMemoryView` don't support
   shape/strides/dtype-aware bridging the way jep's `NDArray`/`jep_numpy.c`
   do. This was wrong: `python/lang/PyMemoryView.java` already exposes
   `getFormat()`, `getShape()`, `getStrides()`, `getSubOffsets()`,
   `isReadOnly()`, `getSlice(...)` — the full Python buffer-protocol surface.
   User's point: the correct architecture is to expose a rich buffer *view*
   and let numpy build its own array over it (`np.frombuffer`/`np.asarray`
   on anything implementing the buffer protocol) — NOT to hand-roll
   numpy-specific conversion code in jpype itself. Building direct numpy
   ties is a known trap: "we end up tied to numpy at the hip and users
   complain."
   - **CLOSED, see `plan/NumpyBufferBench.md`**: confirmed empirically, not
     just by reading headers. 7 tests
     (`PyMemoryViewNumpyNGTest`) cover 1-D zero-copy round-trip, read-only
     `bytes` producing a non-writeable array, multi-dimensional C-contiguous
     reshape (shape/strides correctly reported through `PyMemoryView` cast
     to Java), non-contiguous strided slices, copy-vs-view semantics, a
     non-`uint8` dtype (`float64`), and `getSlice(...)`-derived sub-views -
     all pass with zero new production code. The environment blocker (no
     working numpy under either system Python) was fixed via
     `python3.12 -m pip install --user --ignore-installed numpy` (the
     broken apt-installed 1.21.5 under `/usr/lib/python3/dist-packages` was
     shadowing pip's own "already satisfied" check).

3. **stdout/stderr capture — CLOSED, see `plan/StreamRedirect.md`.**
   jep's `JepConfig.redirectStdout/redirectStdErr` now has a jpype
   equivalent: `Interpreter.setOutput`/`setError`/`setInput` (Java
   `OutputStream`/`Writer`/`InputStream`/`Reader` installed onto
   `sys.stdout`/`sys.stderr`/`sys.stdin`), plus `toPython()` on the Java
   stream side for the reverse direction. Implemented as `plan/IO.md`
   section F, committed `d9e5b24c`, tested both directions.

4. **Configurable sub-interpreter knobs — CLOSED, see `plan/SubInterpreterBuilder.md`.**
   jep's `SubInterpreterOptions` (obmalloc, allowFork/Exec/Threads,
   ownGIL, checkMultiInterpExtensions) now has a jpype equivalent:
   `org.jpype.SubInterpreterBuilder`, a `ProcessBuilder`-style mutable
   builder (`EnumSet<Option>` for the seven `PyInterpreterConfig` flags,
   plain setters for stdio) with `.start()`/`.asSupplier()` terminators.
   `native/common/jp_bridge.cpp`'s `startSubInterpreter` no longer
   hardcodes the config — the seven flags thread down from Java. Per
   `plan/MultiPhaseInit.md` (COMPLETE), `_jpype` is multi-phase-init
   eligible, so `SubInterpreterBuilder.ownGil()` reaches a genuinely
   isolated own-GIL/own-obmalloc subinterpreter, confirmed by
   `SubInterpreterBuilderNGTest.testOwnGilLaunchesAndImportsJpype`.

## Where jpype is already ahead (no action needed, confirmed by survey)

- Default-method proxy handling (`ProxyInstance.java:99`).
- Cross-language exception cause-chaining (`jp_exception.cpp`).
- GC-driven memory management (`jref`/`GlobalPool`, `plan/Globals.md`) vs
  jep's manual `WeakReference`+`ReferenceQueue` polling.
- `python.lang`'s ~55-interface type surface vs jep's thin
  `PyObject`/`PyCallable`/`PyBuiltins`.

## Not gaps (scope differences)

- jep's `ClassEnquirer`/`java_import_hook.py` (Python-side Java imports) —
  belongs to jep's classic/interactive direction, orthogonal to the reverse
  bridge.
- jep's REPL console (`console.py`) — low value for an embedding library's
  core API.
