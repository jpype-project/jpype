# numpy buffer-protocol test bench (stride + copy ops)

## Status (2026-07-11, later): DONE, all cases pass, gap disproven

Environment fixed: `python3.12 -m pip install --user --ignore-installed numpy`
installed a working numpy 2.5.1 into user site-packages, which Python
resolves ahead of the broken apt-installed 1.21.5 under
`/usr/lib/python3/dist-packages` (the one that was raising
`ModuleNotFoundError: No module named 'numpy.core._multiarray_umath'`).
`--ignore-installed` was required because plain `pip install --user numpy`
saw the system package as "already satisfied" and skipped installing.

New test file: `native/jpype_module/src/test/java/python/lang/
PyMemoryViewNumpyNGTest.java`, isolated from `PyMemoryViewNGTest` per the
plan below, `@BeforeClass`-skips the whole class via `SkipException` if
`import numpy as np` fails. 7 tests, all pass (`mvn -q -o test
-Dtest=PyMemoryViewNumpyNGTest`), full suite re-verified clean across 2
runs:

- `test1DUint8ZeroCopyRoundTrip` - writable `bytearray` -> `memoryview` ->
  `np.frombuffer`, confirms values round-trip and mutation through numpy
  reaches the underlying bytearray (zero-copy).
- `testReadOnlyBytesProducesNonWritableArray` - `bytes` -> `memoryview` ->
  `np.frombuffer` produces `writeable=False`; write attempt raises
  `ValueError`. Also cross-checked `PyMemoryView.isReadOnly()` on the Java
  side for both a `PyBytes`-backed (readonly) and `PyByteArray`-backed
  (writable) view.
- `testMultiDimCContiguousReshape` - flat buffer reshaped to `(3, 4)` in
  Python; `PyMemoryView.getShape()`/`getStrides()` cast to Java report the
  real 2-D shape `(3, 4)` and C-contiguous strides `(4, 1)`, not a 1-D view.
  (This was the case flagged as most likely to fail - it didn't: Java-side
  `getShape()`/`getStrides()` genuinely reflect whatever the underlying
  Python memoryview reports, multi-dimensional or not.)
- `testNonContiguousStridesFromSlicedArray` - `arr[::2]` produces a
  non-contiguous buffer; confirmed real doubled stride `(8,)` (int32) both
  in Python and via Java's `getStrides()`, and `np.asarray(view)`
  reconstructs the correct logical values, not a naive contiguous
  reinterpretation.
- `testCopyVsViewSemantics` - `np.asarray(view)` is zero-copy (mutation
  propagates to the underlying bytearray); `np.array(view)` forces a copy
  (mutation does not propagate). Locks in the classic numpy copy/no-copy
  distinction as a regression guard.
- `testFloat64FormatRoundTrip` - non-`uint8` dtype case via
  `array.array('d', ...)`: `PyMemoryView.getFormat()` correctly reports
  `'d'` (not just byte-granularity), and `np.frombuffer(..., dtype=np.float64)`
  round-trips real float values.
- `testGetSliceRoundTrip` - `PyMemoryView.getSlice(5, 15)` taken on the Java
  side, then hand back into Python via `context.globals().putAny(...)`;
  `np.frombuffer` on the slice produces the correct offset sub-range, not
  the full buffer.

**Outcome:** the claim is confirmed, not just "likely" - jpype's existing
`PyMemoryView`/`PyBuffer` surface needed zero new production code for any
of these cases, including the multi-dimensional/non-contiguous ones that
were the most plausible failure candidates going in. This closes
`plan/JepParity.md` item 2 definitively.

## Status (2026-07-11): scoped, not started

Follow-on from `plan/JepParity.md` item 2. The claim to verify: jpype's
buffer-protocol surface (`python.lang.PyMemoryView`/`PyBuffer`,
`PyByteArray`/`PyBytes`) is already sufficient for numpy interop — no
numpy-specific glue code needed, because PEP 3118 buffers natively carry
format/shape/strides/suboffsets and numpy consumes that protocol directly
(`np.frombuffer`/`np.asarray`). This has not yet been proven with a real
numpy round-trip test, only by reading `PyMemoryView.java`'s header
surface. This plan is scoped as its own doc (rather than a one-line item
in `JepParity.md`) because the "all stride and copy ops" bar means several
distinct cases need coverage, not just a single smoke test.

## Environment note (found while scoping, not yet resolved)

numpy is not currently importable in either system Python on this
machine: `python3.10 -c "import numpy"` finds no numpy at all;
`python3.12 -c "import numpy"` has a broken install
(`ModuleNotFoundError: No module named 'numpy.core._multiarray_umath'` —
version mismatch between an apt-installed numpy 1.21.5 under
`/usr/lib/python3/dist-packages` and the running 3.12 interpreter). Either
interpreter will need a working numpy (likely a `pip install --user numpy`
into the actual interpreter jpype's tests run against, see
`jpype_build_env_gotchas` memory for the per-version build quirks) before
this test bench can actually run — this is a prerequisite, not part of the
test design itself.

## Existing test harness to build on

`native/jpype_module/src/test/java/python/lang/PyMemoryViewNGTest.java`
(extends `PyTestHarness`) already exercises `context.memoryview(...)`,
`getShape()`, `getSlice()`, `getBuffer()`, `isReadOnly()`, `release()` on
one-dimensional `PyByteArray`-backed views — but nothing multi-dimensional
and nothing that hands the buffer to numpy. This is the file to extend, or
a sibling `PyMemoryViewNumpyNGTest.java` if keeping numpy-dependent tests
isolated (so the base suite doesn't gain a numpy dependency) is preferred
— lean toward a separate file/test-group so numpy stays an optional/
skippable dependency for the rest of the suite, matching how other
optional-dependency tests in this codebase are usually isolated (check
`plan/DOCS.md`/existing NGTest conventions for a skip-if-absent pattern
before inventing a new one).

## Cases to cover ("all stride and copy ops")

- **1-D contiguous, read-only and read-write**: `PyBytes`/`PyByteArray` ->
  `PyMemoryView` -> `np.frombuffer(view, dtype=uint8)`; confirm values
  round-trip both directions (numpy write-back visible on the Java/Python
  side for the read-write `PyByteArray` case, and confirm the read-only
  `PyBytes` case correctly produces a non-writeable numpy array, not a
  silent copy that would mask a real gap).
- **Multi-dimensional, C-contiguous** (e.g. reshape a flat buffer to a 2-D
  or 3-D shape on the Python side before handing it across, or build a
  genuinely multi-dim buffer if `PyMemoryView.getShape()`/`getStrides()`
  support constructing one from the Java side — check whether Java-side
  buffer creation currently only produces 1-D views and multi-dim is
  Python-side-only today).
- **Non-trivial (non-contiguous) strides**: slice a numpy array with a
  step (`arr[::2]`) or transpose a 2-D array before taking a memoryview of
  it, confirm `PyMemoryView.getStrides()` reports the real non-contiguous
  strides and that `np.asarray(view)` reconstructs the correct logical
  values, not a naive contiguous reinterpretation.
- **Copy vs view semantics**: confirm which operations are true zero-copy
  views (mutating the numpy array should be visible back on the Java/
  Python `bytearray`) vs which force a copy (e.g. `np.array(view)` vs
  `np.asarray(view)` — the classic numpy copy/no-copy distinction), and
  assert the expected one for each, so a future regression that
  accidentally starts copying (silently losing zero-copy performance)
  gets caught.
- **dtype variety**: at least one non-`uint8` case (`float64`/`int32`) to
  confirm `getFormat()`'s PEP 3118 format-string output is actually
  dtype-correct, not just byte-granularity.
- **Round-trip through `getSlice(start, end)`**: confirm a
  `PyMemoryView.getSlice(...)`-derived sub-view also produces a correct
  numpy array with the right offset/strides, not just the full view.

## Related

`plan/ArrayRegionCopy.md` — a bulk Java-array -> caller-provided-buffer
copy operation (the "provide a destination, JVM copies into it" JNI
shape), scoped separately since it's a distinct feature from proving the
existing zero-copy `PyMemoryView` export works, but its test coverage
extends this bench (copying into a preallocated/non-contiguous numpy
array).

## Outcome

If every case above passes, this closes out `JepParity.md` item 2
definitively (delete the "likely" hedge, mark it a confirmed non-gap). If
any case fails — most plausible candidate is Java-side construction of a
genuinely multi-dimensional/strided buffer, since `PyMemoryViewNGTest`'s
existing coverage is all 1-D — that specific failure becomes a real, newly
scoped gap with a concrete repro, not a guess.
