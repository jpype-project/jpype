# Results: conversion-path caching and fast paths, 2026-08-01

## What changed

- `JPClass::findJavaConversion` (native/common/include/jp_conversioncache.h,
  jp_class.cpp): a per-class, fixed 16-slot, atomically-accessed cache
  keyed on `Py_TYPE(object)`, consulted before re-running the conversion
  search (previously a linear scan, unbounded for classes with many
  registered `@JConversion` hints). `JPMatch::cacheable` (default true)
  lets individual `JPConversion::matches()` implementations opt out when
  their decision depends on the object's content, not just its type
  (duck-typed attributes, `Class` identity, sequence/buffer element scans,
  proxy interfaces, functional-interface arg counts). [`4a61fe38`]
- `JPConversionSequence` (jp_classhints.cpp) and `JPIntType::setArrayRange`
  (jp_inttype.cpp): a fast raw-type-check path for homogeneous lists of
  exact `int`s, falling back to the general (unchanged, fully correct)
  path from the first non-conforming element. [`82a552de`, `bc73d88f`]
- `JPClass::convertToPythonObject` / `JPBoxedType::convertToPythonObject`:
  skip the `findClassForObject` JNI upcall (a bytecode-level HashMap.get
  on the Java side) when `GetObjectClass`+`IsSameObject` against the
  already-known declared class confirms there's no covariant override in
  play. Applies to every non-primitive Java method return. [`5069df78`]
- `jp_proxy.cpp`'s `getArgs()`: the same `IsSameObject` fast path, applied
  to every argument of every Java-calls-Python proxy invocation (a
  separate code path from the return-value one above, with its own
  explicit `findClassForObject` call). [`0f21b4ad`]
- A real, independent bug fix in `JPMethodDispatch::findOverload`'s
  single-slot overload-dispatch cache: a failed cache-hit attempt left
  `bestMatch.m_Overload` non-null (since `JPMethod::matches()` sets it
  unconditionally), which fooled the fallback logic into reporting a
  corrupted match as successful instead of raising `TypeError`. Found via
  the array benchmark below, unrelated to the caching feature itself.
  [`d65781fb`]

None of this trades functionality for speed: each fast path checks the
exact same thing the general path would, just cheaper, and falls back to
the unmodified general path the instant the cheap check doesn't hold. See
project/benchmark/README.md to reproduce any of these numbers, and
project/benchmark/bench_*.py for the actual benchmark code.

## Results: master vs. this branch

ns/call, best-of-5, standard (non-instrumented) build. "master" =
`9daaf0af`, the actual released baseline. "final" = this branch (all five
changes above).

| category | operation | master | final | change |
|---|---|---:|---:|---:|
| int | `Math.max(int,int)` | 817 | 750 | -8% |
| int | `new Integer(int)` | 1107 | 840 | -24% |
| double | `Math.sqrt(double)` | 786 | 690 | -12% |
| double | `new Double(double)` | 1161 | 918 | -21% |
| string | `new String` + `.toString()` | 1107 | 1022 | -8% |
| object | `Object` identity (arg + return) | 1201 | 995 | -17% |
| arrays | `int[]` from fresh `list(100)` | 5700 | 3192 | -44% |
| method dispatch | overload x16, monomorphic | 712 | 686 | -4% |
| method dispatch | overload x16, polymorphic | 1227 | 925 | -25% |
| proxy | callback, established binding, `int` arg | 3034 | 2655 | -12% |
| proxy | callback, established binding, `Object` arg | 2972 | 2364 | -20% |

Plus, not in the table above since it has no single-call analog: the
hint-list conversion scan (`JPClassHints::getConversion`, see
`bench_classhints.py`) went from linear in the number of registered
hints -- up to 6.8us at 400 hints -- to a flat ~620ns regardless of count.

Worth noting: `model3` (the branch this work landed on) already differs
from `master` before any of the above -- an earlier, separate object-layout
rework ("fixed offsets, fungible boxed values") had already improved
boxed-type and object-identity paths by 10-17% on its own. The table above
is master-to-final, so it captures that too; if isolating just this
session's caching/fast-path work against model3's own pre-existing tip
(`5d35fa30`) is useful context, those numbers run 4-46% smaller per
category (e.g. arrays: -46%, proxy Object-arg: -19%, dispatch
polymorphic: -23%) since they exclude the layout rework's contribution.

## Why some paths didn't move

- **int/double/string, most cases**: these were already resolved via
  short, fixed-size matcher chains (a handful of `if` checks, not a
  scan), so there was nothing for a cache or fast path to skip. The
  modest movement they do show is from the object-layout rework, not
  this session's work.
- **Method dispatch, monomorphic**: already cheap via the pre-existing
  single-slot overload cache; the polymorphic case improved (25%) purely
  as a side effect of the `findJavaConversion` cache, since each of the
  16 candidate-overload checks in that scan calls `findJavaConversion`
  internally -- nothing in the dispatch loop itself was touched.
- A separate attempt at a raw-type-check fast path for scalar
  `findJavaConversion` itself (bypassing even a cache *hit*, mirroring
  the array fast path) was built, fully verified, and benchmarked on a
  branch -- and showed no measurable gain. The array fast path pays off
  because it fires ~100 times per call (once per list element); a scalar
  argument only gets it once per call, which isn't enough to clear the
  noise floor of everything else a method call does (JPMatch
  construction, dispatch bookkeeping, the JNI call itself). Reverted.

## Multi-dimensional array push conversion (follow-on)

JPype already had a fast bulk path for building a Java array from a flat
Python buffer (`newMultiArray`/`convertMultiArray`, used by the explicit
`_jpype.arrayFromBuffer()` upload call), but it wasn't reachable from
ordinary argument conversion -- passing a multi-dimensional numpy array
as an `int[][]`/`double[][][]`/etc. argument went through
`JPConversionSequence`'s general path instead, which walks the outer
dimension via per-row Python-level indexing (materializing a fresh numpy
sub-array view object per row) and does a separate buffer conversion per
row.

`JPConversionMultiArrayBuffer` (jp_classhints.cpp) now recognizes a
buffer-protocol argument whose `ndim` matches the target array's nesting
depth and routes it directly through the existing bulk machinery
instead. `JPArrayClass` precomputes its nesting depth and primitive leaf
type once at construction, so this new check costs nothing on the (far
more common) 1D-array and non-buffer paths -- confirmed both by
benchmark and by instrumented call-counting during development, which
showed zero invocations of the new conversion for either a 1D list or a
nested list-of-lists argument.

| category | operation | before | after | change |
|---|---|---:|---:|---:|
| arrays | `int[][]` from numpy(10x10), fresh | 10453 | 3014 | -71% |
| arrays | `int[][]` from nested list(10x10), fresh | 15300 | 16122 | +5%\* |

\* Nested lists-of-lists never touch the new path at all -- no buffer
protocol, so `PyObject_CheckBuffer` fails immediately, and instrumented
call-counting confirmed zero invocations of the new conversion here.
This small shift is compiler code-layout noise from the recompiled
translation units, not added runtime work in the hot path. It's real
(reproducible across repeated runs) but not something to chase further.

Nested-list `int[][]` itself remains exactly as slow as before this
work, and there's no cheap fix in sight for it: it's an
architecture mismatch, not a missing fast path. The buffer trick here
works because a `Py_buffer` hands over shape, strides, and a contiguous
memory region up front, letting the whole array be walked with a
handful of JNI calls. A Python list-of-lists offers none of that --
each row is an arbitrary sequence that has to be indexed, checked, and
converted element-by-element through the general (necessarily slower,
but also more permissive -- mixed types, ragged rows, arbitrary
iterables) `JPConversionSequence` path, the same as any other list
argument. Making that materially faster would mean a fundamentally
different (and narrower, less correct) traversal, not a bolt-on cache or
buffer check -- the same tradeoff jpy already makes for its own array
matching (see below).

## Supplementary: jpy / jep comparison

For context only -- neither is a drop-in replacement for jpype's
feature set, and where they're faster it's mostly not because jpype's
implementation is worse. Two different things are going on, and they
shouldn't be conflated:

- **Arrays**: jpy's array-argument matching genuinely skips work jpype
  does -- it doesn't inspect elements before committing to a conversion,
  where jpype validates every element up front to support correct
  Java-style overload disambiguation. This isn't inferred from the
  timing gap; it was confirmed by reading jpy's own C source
  (`jpy_jtype.c`/`jpy_jmethod.c`) earlier in this work. That's a real
  correctness/speed tradeoff jpy makes and jpype doesn't.
- **Everything else (scalars, dispatch, proxy)**: the same source dive
  found jpy's matchers there are *not* cutting correctness corners --
  they're just leaner architecturally (fewer abstraction layers, less
  general-purpose machinery to walk through). jpype pays a real cost
  for breadth of behavior a narrower binding doesn't have to support
  (implicit numeric widening, functional-interface duck typing,
  hint-based custom conversions, the whole `JPConversion` chain this
  session's caching work targets). So "jpy is faster here" is a fair
  data point, just not evidence that jpype's implementation of the same
  narrow behavior is inefficient.

jpy in particular wasn't judged a strong reference point for this
comparison overall.

ns/call, best-of-5. jep is on Python 3.10 (this checkout's only working
native build), not 3.12 like jpype/jpy, and embeds Python inside the JVM
(the reverse of jpype/jpy's architecture), so its numbers carry extra
uncertainty.

| category | operation | jpype (final) | jpy | jep |
|---|---|---:|---:|---:|
| int | `Math.max(int,int)` | 750 | 353 | 1299 |
| int | `new Integer(int)` | 840 | 479 | 1257 |
| double | `Math.sqrt(double)` | 690 | 393 | 645 |
| double | `new Double(double)` | 918 | 506 | 1378 |
| string | `new String` + `.toString()` | 1022 | 927 | 2512 |
| object | `Object` identity (arg + return) | 995 | 730 | 1971 |
| arrays | `int[]` from fresh `list(100)` | 3192 | 1262 | 2029 |
| arrays | `int[][]` from nested list(10x10), fresh | 16122 | 2105 | 5875 |
| arrays | `int[][]` from numpy(10x10), fresh | 3014 | 5316 | N/A† |
| method dispatch | overload x16, monomorphic | 686 | 370 | 4454 |
| method dispatch | overload x16, polymorphic | 925 | 456 | 4558 |
| proxy | callback, `int` arg | 2655 | N/A* | 2240 |
| proxy | callback, `Object` arg | 2364 | N/A* | 4629 |

\* jpy's `PyObject.createProxy()` didn't produce a usable object from
Python in this checkout -- see README.md. Looks like a jpy-side issue,
not a benchmark gap.

† jep raises `TypeError: Error matching ndarray.dtype to Java primitive
type` for any multi-dimensional numpy array -- a real jep limitation
(no multi-dimensional numpy support at all), not a benchmark setup
issue; confirmed by mirroring jep's own conversion code path.

The 2D numpy row is the interesting one from the multi-dimensional-array
follow-on above: jpype (3014) is now *faster* than jpy (5316) here,
flipping what was a ~2x jpype deficit into a ~1.76x jpype lead. jpy
apparently has no equivalent bulk path for multi-dimensional buffers
either -- its own numpy-2D handling is slower than its plain
nested-list conversion (5316 vs 2105), so it's paying buffer-inspection
overhead without a matching payoff.

### jpype vs. jep specifically

jpype beats jep in 8 of the 12 comparable rows above (`int[][]` from
numpy has no jep entry -- see above). Where jep wins, it's genuinely
because it skips work jpype does -- confirmed by reading jep's own
source, not inferred from the timing gap:

- **Arrays** (`int[]` from list: 2029 vs 3192; `int[][]` from nested
  list: 5875 vs 16122): jep's overload resolution
  (`pyjmultimethod_call`, `src/main/c/Objects/pyjmultimethod.c:118-155`
  in the jep checkout) first filters candidates by parameter count
  alone -- no type inspection -- and only runs its per-argument
  compatibility check (`PyJMethod_CheckArguments`) when *two or more*
  candidates share that count. `DeepBench.sumIntArray`/`sum2DIntArray`
  each have exactly one overload, so that check never runs at all for
  them: jep goes straight from an arity match to `pyjmethod_call`
  (`pyjmethod.c:284`), which converts each argument directly against
  the one already-known parameter type, optimistically, raising only if
  an element actually fails to convert. That's one pass over the data.
  jpype's `JPMethod::matches()` always computes a full match quality
  first (a complete scan of every array element via
  `JPConversionSequence`, needed to correctly rank against sibling
  overloads even when there happens to be only one), and only then does
  `convert()` do a second full pass to actually build the array --
  two traversals where jep does one, for exactly the case (no
  competing overload) these benchmarks exercise.
- **`Math.sqrt`** (645 vs 690, 7%): too small a gap to attribute to
  this or anything else specific -- likely just noise.
- **Proxy, `int` arg** (2240 vs 2655, 18%): not this mechanism -- both
  sides have exactly one interface method here, so there's no
  overload ambiguity to skip resolving on jpype's side either. More
  likely explained by jep's reversed embedding architecture: it hosts
  Python *inside* the JVM, so a Java-calls-Python callback is jep's
  native direction, while jpype has to reach back out through JNI into
  the interpreter hosting the JVM.

This isn't true across the board, though: on the 16-overload dispatch
benchmark, where every candidate shares the same arity, jep's fast
filter can't help and it falls back to calling `PyJMethod_CheckArguments`
repeatedly -- and there jpype is dramatically faster (686-925 vs
4454-4558). The one-pass-vs-two-pass array gap above is a real,
specific tradeoff for the no-ambiguity case, not evidence that jep's
overload resolution is generally cheaper.

## Verification

Every change above went through the full local suite (standard and
`ENABLE_COVERAGE=ON`/fault-injection builds, both fixed and
`pytest-randomly` orderings) in a disposable venv per this repo's
CLAUDE.md, plus targeted regression tests for the specific correctness
edge cases each fast path introduced (null arguments, covariant/subclass
returns, mixed-type lists, boxed-type round trips, repeated-call cache
invalidation). See the individual commits for details.

The multi-dimensional array follow-on adds its own regression tests in
`test_array.py`: 2D/3D numpy round trips through the new path, a
transposed (non-contiguous) numpy array correctly falling back to the
general path, a dtype the buffer machinery can't convert correctly
raising `TypeError` instead of being silently accepted, and a ragged
nested list (which was never affected) still working.
