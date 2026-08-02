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

## Supplementary: jpy / jep comparison

For context only -- neither is a drop-in replacement for jpype's
feature set (in particular, jpy's array-argument overload matching
doesn't inspect elements before committing to a conversion, which is
part of why it's faster there; jpype's design validates every element up
front to support correct Java-style overload disambiguation). jpy in
particular wasn't judged a strong reference point for this comparison.

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
| method dispatch | overload x16, monomorphic | 686 | 370 | 4454 |
| method dispatch | overload x16, polymorphic | 925 | 456 | 4558 |
| proxy | callback, `int` arg | 2655 | N/A* | 2240 |
| proxy | callback, `Object` arg | 2364 | N/A* | 4629 |

\* jpy's `PyObject.createProxy()` didn't produce a usable object from
Python in this checkout -- see README.md. Looks like a jpy-side issue,
not a benchmark gap.

## Verification

Every change above went through the full local suite (standard and
`ENABLE_COVERAGE=ON`/fault-injection builds, both fixed and
`pytest-randomly` orderings) in a disposable venv per this repo's
CLAUDE.md, plus targeted regression tests for the specific correctness
edge cases each fast path introduced (null arguments, covariant/subclass
returns, mixed-type lists, boxed-type round trips, repeated-call cache
invalidation). See the individual commits for details.
