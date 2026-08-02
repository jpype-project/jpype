# Results: JPClass::findJavaConversion caching + follow-on fast paths, 2026-08-01

Context for this: `JPClassHints::getConversion`'s linear scan over
registered `@JConversion` hints, and later the whole `findJavaConversion`
chain generally, was suspected to be a significant per-call cost. This
records what was actually measured, before/after, and against jpy/jep,
across every path touched this session.

See project/benchmark/README.md for how to reproduce any of this.

## What changed

- `JPClass::findJavaConversion` (native/common/include/jp_conversioncache.h,
  jp_class.cpp): a per-class, fixed 16-slot, atomically-accessed cache
  keyed on `Py_TYPE(object)`, consulted before re-running the conversion
  search. `JPMatch::cacheable` (default true) lets individual
  `JPConversion::matches()` implementations opt out when their decision
  depends on the object's content, not just its type (duck-typed
  attributes, `Class` identity, sequence/buffer element scans, proxy
  interfaces, functional-interface arg counts). [`4a61fe38`]
- `JPConversionSequence` (jp_classhints.cpp) and `JPIntType::setArrayRange`
  (jp_inttype.cpp): a fast raw-type-check path for homogeneous lists of
  exact `int`s, falling back to the general path from the first
  non-conforming element. [`82a552de`, `bc73d88f`]
- `JPClass::convertToPythonObject` / `JPBoxedType::convertToPythonObject`:
  skip the `findClassForObject` JNI upcall (a bytecode-level HashMap.get)
  when `GetObjectClass`+`IsSameObject` against the already-known declared
  class confirms there's no covariant override in play. Applies to every
  non-primitive Java method return. [`5069df78`]
- `jp_proxy.cpp`'s `getArgs()`: the same `IsSameObject` fast path, applied
  to every argument of every Java-calls-Python proxy invocation (a
  separate code path from the return-value one above, with its own
  explicit `findClassForObject` call). [`0f21b4ad`]
- A real, independent bug fix in `JPMethodDispatch::findOverload`'s
  single-slot overload-dispatch cache (stale `bestMatch.m_Overload` left
  over from a failed cache-hit attempt, causing a corrupted match to be
  reported as successful instead of raising `TypeError`) -- found via the
  array benchmark, unrelated to the caching feature itself. [`d65781fb`]

## Results

ns/call, best-of-5. "jpype-original" = `5d35fa30` (last commit before this
work). "jpype-final" = current tree (all five fixes above).

| category | operation | jpype-original | jpype-final | jpy | jep |
|---|---|---:|---:|---:|---:|
| int | `Math.max(int,int)` | 751 | 750 | 353 | 1299 |
| int | `new Integer(int)` | 940 | 840 | 479 | 1257 |
| double | `Math.sqrt(double)` | 696 | 690 | 393 | 645 |
| double | `new Double(double)` | 1041 | 918 | 506 | 1378 |
| string | `new String` + `.toString()` | 1030 | 1022 | 927 | 2512 |
| object | `Object` identity (arg + return) | 1103 | 995 | 730 | 1971 |
| arrays | `int[]` from fresh `list(100)` | 5878 | 3192 | 1262 | 2029 |
| method dispatch | overload x16, monomorphic | 663 | 686 | 370 | 4454 |
| method dispatch | overload x16, polymorphic | 1201 | 925 | 456 | 4558 |
| proxy | callback, established binding, `int` arg | 2833 | 2655 | N/A* | 2240 |
| proxy | callback, established binding, `Object` arg | 2908 | 2364 | N/A* | 4629 |

\* jpy's `PyObject.createProxy()` didn't produce a usable object from
Python in this checkout -- see README.md. Looks like a jpy-side issue,
not a benchmark gap.

## Verdict

Every number in the "jpype-final" column reflects a change that either
skips real, provably-unnecessary work (the two `IsSameObject` fast paths,
the array fast path) or fixes a genuine correctness bug found along the
way -- none of it trades functionality for speed. What it doesn't do is
close the gap with jpy's baseline: jpy's own dispatch code
(`jpy_jtype.c`) is faster mainly because it does *less* validation, not
because it's better-optimized doing the *same* validation -- e.g. its
array-argument scoring returns a flat constant for any non-`String`
array type without inspecting a single element, unlike jpype's (and
jpy's own `String[]`) full per-element check. Matching that speed exactly
would mean accepting that same gap (see the `Math.max` conversation this
session), which wasn't judged worth it.

By area:
- **Hint-list scan** (the original motivating case, see
  `bench_classhints.py`, not in the table above): flat ~620ns regardless
  of registered-hint count, down from up to 6.8us at N=400.
- **Arrays**: 5878 -> 3192ns (46%), still ~2.5x behind jpy (was ~4.7x).
- **Object identity (return path)**: 1103 -> 995ns (10%) from the
  `IsSameObject` fast path -- confirms the upcall was real but not
  dominant; most of the remaining ~650-670ns "return-wrap" cost is
  object-construction machinery (`tp_alloc`, `PyJPClass_create`,
  `PyJPValue_assignJavaSlot`), not class resolution.
- **Proxy, Object-typed argument**: 2908 -> 2364ns (19%) from the same
  fast path applied to `getArgs()` -- bigger relative effect than the
  return-path case since it fires once per argument, not once per call.
- **Proxy, int argument** and **method dispatch (monomorphic)**: no
  meaningful change, as expected -- primitive arguments never touch
  `findClassForObject`, and a single-overload/already-cached dispatch was
  already cheap.
- **Method dispatch (polymorphic overload)**: 1201 -> 925ns (23%), a side
  effect of the `findJavaConversion` cache -- each of the 16 candidate
  checks in `JPMethodDispatch::findOverload`'s scan calls
  `findJavaConversion` internally, so caching helped here even though
  nothing in the dispatch loop itself was touched.
- **int/double/string**: flat to ~10% at best, unaffected by design --
  these were already resolved via short, fixed-size matcher chains with
  nothing for a cache or fast path to skip.
