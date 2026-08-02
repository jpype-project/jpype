# Results: JPClass::findJavaConversion caching, 2026-08-01

Context for this: `JPClassHints::getConversion`'s linear scan over
registered `@JConversion` hints, and later the whole `findJavaConversion`
chain generally, was suspected to be a significant per-call cost. This
records what was actually measured, before/after, and against jpy/jep.

See project/benchmark/README.md for how to reproduce any of this.

## What changed

- `JPClass::findJavaConversion` (native/common/include/jp_conversioncache.h,
  jp_class.cpp): a per-class, fixed 16-slot, atomically-accessed cache
  keyed on `Py_TYPE(object)`, consulted before re-running the conversion
  search. `JPMatch::cacheable` (default true) lets individual
  `JPConversion::matches()` implementations opt out when their decision
  depends on the object's content, not just its type (duck-typed
  attributes, `Class` identity, sequence/buffer element scans, proxy
  interfaces, functional-interface arg counts).
- `JPConversionSequence` (jp_classhints.cpp) and `JPIntType::setArrayRange`
  (jp_inttype.cpp): a fast raw-type-check path for homogeneous lists of
  exact `int`s, falling back to the general path from the first
  non-conforming element -- not a caching change, but found and built
  while digging into why `int[]` conversion stayed slow after the cache
  landed.
- A real, independent bug fix in `JPMethodDispatch::findOverload`'s
  single-slot overload-dispatch cache (stale `bestMatch.m_Overload` on a
  failed cache hit), found via the array benchmark, unrelated to the
  caching feature itself.

Commits: `4a61fe38`, `82a552de`, `bc73d88f` (the cache + array fast
paths), `d65781fb` (the dispatch bug fix).

## Results

ns/call, best-of-5. "jpype-original" = `5d35fa30` (last commit before this
work). "jpype-cache" = current tree.

| category | operation | jpype-original | jpype-cache | jpy | jep |
|---|---|---:|---:|---:|---:|
| int | `Math.max(int,int)` | 750 | 734 | 376 | 1324 |
| int | `new Integer(int)` | 918 | 845 | 475 | 1301 |
| double | `Math.sqrt(double)` | 691 | 689 | 406 | 667 |
| double | `new Double(double)` | 1014 | 930 | 513 | 1371 |
| string | `new String` + `.toString()` | 1052 | 1020 | 940 | 2492 |
| object | `Object` identity (arg + return) | 1098 | 1084 | 729 | 1964 |
| arrays | `int[]` from fresh `list(100)` | 5833 | 3181 | 1258 | 1979 |
| proxy | callback into established Python binding | 2826 | 2677 | N/A* | 2257 |

\* jpy's `PyObject.createProxy()` didn't produce a usable object from
Python in this checkout -- see README.md. Looks like a jpy-side issue,
not a benchmark gap.

## Verdict

Meaningful, but narrowly scoped to what the cache actually targets:

- **Hint-list scan** (the original motivating case, see
  `bench_classhints.py`, not in the table above): flat ~620ns regardless
  of registered-hint count, down from up to 6.8us at N=400. Real,
  unbounded win for interfaces with many registered conversions; ~1.2-1.6x
  for a typical few-hint interface.
- **Arrays**: 5833 -> 3181ns (45%), the one category where the effect
  shows up against jpy/jep too, not just against jpype's own past --
  still ~2.5x behind jpy, roughly half the original ~4.6x gap.
- **int/double/string/object/proxy**: flat to ~10% at best, because none
  of these route through the cached machinery -- they were already
  resolved via short, fixed-size matcher chains, so there was nothing for
  a cache to skip. The ~2x gap against jpy on these is a different
  problem (primitive marshaling overhead in the two-pass match/convert
  design generally, not something a conversion cache addresses) and was
  never in scope for this work.
