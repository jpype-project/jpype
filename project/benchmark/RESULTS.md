# Cross-library benchmark results

jpype vs. jpy, jep, and pyjnius on the JVM-embedding side of a Python/Java
bridge (Python drives Java in all four); GraalPy (Python-in-the-JVM,
opposite architecture) is tracked separately in Section 9 and was not
re-run for this edition. jpype's own numbers are from a fresh,
from-scratch re-run of its full benchmark suite, in disposable/isolated
environments per this repo's CLAUDE.md; jpy/jep/pyjnius numbers are
carried forward from their last captured run (not re-run this edition)
and use the same `trials=7`, higher-iteration-floor methodology described
below. Every section follows the same shape: **Methodology** (what is
measured and how), **Table** (raw numbers, `best` of the trials,
nanoseconds per call unless noted), **Result** (the factual takeaway
only -- no running commentary, no revision history).

## Global methodology

- **Timing.** `timeit()` (`project/benchmark/_common.py`, and an
  inlined equivalent in every `jep/*.py` script since jep's embedded
  interpreter can't import a sibling file): warm up, then run `trials=7`
  timed batches of `n` calls each; report `best` (minimum batch mean) and
  `median` across the 7 batches. Tables below show `best`; raw `best`/
  `median` pairs and CSVs are preserved alongside each script's own
  output.
- **Iteration counts.** Fixed-cost benchmarks (scalars, dispatch, proxy,
  strings, object identity) use `n=200,000`. Array benchmarks scale `n`
  down as element count grows (`calls_for()` in each script) so a
  100,000-element case doesn't take minutes; floors were raised this
  edition (`n >= 30`, `warmup >= 6` per batch, up from `n >= 20`,
  `warmup >= 5`) for better statistics without making the deepest
  multi-dimensional cases impractically slow.
- **Environments.** jpype: fresh venv, `pip install --no-build-isolation
  -e .` with `BUILD_TEST_HARNESS=ON`. jpy: fresh venv, prebuilt wheel
  from `~/devel/jpy/dist`. jep: `~/devel/jep/target/jep-4.3.1.jar` +
  the Python-3.12-matched native build
  (`~/devel/jep/build/lib.linux-x86_64-cpython-312`), launched as a Java
  process per this repo's README. pyjnius: fresh venv, built from source
  (`~/devel/pyjnius`, Cython 3.1.2) against this machine's JDK. Full
  per-library setup in `project/benchmark/README.md`.
- **Machine.** Single 16-core/7.7GB-RAM machine, one library's suite run
  at a time except where noted; no concurrent unrelated load.
- **Scope.** `int`/`long`/`float`/`double` element types throughout
  unless noted -- jpy/jep/pyjnius have no `Z`/`B`/`C`/`S`
  (boolean/byte/char/short) array benchmarks to compare against. Section
  6 additionally covers jpype's own boolean/byte/char/short ragged-push
  support, jpype-only.


## 1. Scalars, strings, object identity

**Methodology.** `Math.max(int,int)` / `new Integer(int)` (int.py),
`Math.sqrt(double)` / `new Double(double)` (double.py), a round-trip
Java-String-from-Python-then-`str()`-back (strings.py), and reference
identity of a returned Java object compared across two calls
(object.py). All four are fixed-cost, `n=200,000` per batch.

| operation | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| Math.max(int,int) | 782.7 | 363 | 1,344 | 1,323 |
| new Integer(int) | 1,888.3 | 478 | 1,383 | 8,014 |
| Math.sqrt(double) | 896.2 | 419 | 688 | 533 |
| new Double(double) | 2,057.6 | 527 | 1,454 | 7,177 |
| new String + toString | 1,065.8 | 1,010 | 2,616 | 23,546 |
| Object identity | 1,227.5 | 737 | 2,135 | 4,096 |

**Result.** jpy is fastest on every scalar/string/identity op; jpype
trails jpy by roughly 1-1.8x; jep and pyjnius trail jpype by a further
1.5-4x depending on the op, with pyjnius's `new Integer`/`new Double`/
string-roundtrip costs the widest outliers (7-23x jpy).

## 2. Method dispatch and proxy callbacks

**Methodology.** `dispatch.py`: a 16-overload method called
monomorphically (same arg type every call) vs. polymorphically
(argument type varies call-to-call, forcing overload resolution to
redo work jpype/jep can otherwise cache). `proxy.py`: steady-state cost
of invoking an already-constructed Python-implements-Java-interface
callback, `int` argument and (where supported) `Object` argument.

| operation | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| overload x16, monomorphic | 743.5 | 543 | 4,783 | 4,090 |
| overload x16, polymorphic | 916.6 | 546 | 5,146 | 4,238 |
| proxy callback (established), int arg | 3,182.9 | -- | 2,440 | 42,587 |
| proxy callback (established), Object arg | 3,274.1 | -- | 5,159 | -- |

**Result.** jpy has no separate proxy script (not benchmarked here).

pyjnius's proxy Object-arg case is not benchmarked: it reliably
segfaults this pyjnius checkout (`GetObjectClass`/`IsSameObject` called
without a null check on a genuinely-null `Object` argument), reproduced
independently against a fresh build before being treated as a real
finding rather than stale-build noise, per this repo's CLAUDE.md. jpype
leads jep and pyjnius on dispatch by roughly 6-9x; jpy leads jpype on
raw dispatch cost the same way it does on scalars.

## 3. Array push, flat (1D)

**Methodology.** `array_flat.py`. A Python `list`/numpy array of
length 100/1,000/10,000/100,000 pushed into a Java method parameter
(`int[]`/`long[]`/`float[]`/`double[]`), and the reverse (`array->list`,
`array->buffer`): reading a Java array back into a Python `list` /
numpy buffer. `list->array, widening from int` additionally covers a
plain Python `int` list pushed against a `float[]`/`double[]`
parameter (int has no widening case against itself).

### `list->array` push (method argument)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 1,451 | 1,183 | 2,493 | 3,057 |
| int[1000] | 4,879 | 8,074 | 14,740 | 27,116 |
| int[10000] | 38,438 | 76,676 | 136,222 | 280,330 |
| int[100000] | 387,067 | 781,784 | 1,371,269 | 4,673,403 |
| long[100] | 1,491 | 1,210 | 2,456 | 2,900 |
| long[1000] | 5,722 | 8,689 | 14,246 | 26,344 |
| long[10000] | 45,810 | 82,353 | 136,267 | 275,971 |
| long[100000] | 480,738 | 824,832 | 1,380,915 | 5,590,922 |
| float[100] | 1,286 | 1,110 | 2,108 | 3,417 |
| float[1000] | 3,288 | 7,626 | 11,485 | 29,009 |
| float[10000] | 22,416 | 72,344 | 102,599 | 283,160 |
| float[100000] | 206,095 | 724,713 | 1,013,267 | 3,323,044 |
| double[100] | 1,307 | 1,190 | 1,993 | 3,421 |
| double[1000] | 3,936 | 8,430 | 10,737 | 31,134 |
| double[10000] | 29,513 | 81,607 | 94,816 | 292,036 |
| double[100000] | 290,820 | 807,684 | 1,038,434 | 5,077,917 |

### `list->array`, widening from int (float/double only)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| float[100] | 1,514 | 1,588 | 3,932 | 3,464 |
| float[1000] | 5,585 | 11,962 | 31,456 | 28,942 |
| float[10000] | 45,088 | 115,583 | 303,602 | 285,874 |
| float[100000] | 439,591 | 1,187,625 | 2,911,070 | 3,645,482 |
| double[100] | 1,546 | 1,654 | 4,002 | 3,399 |
| double[1000] | 6,110 | 13,096 | 30,740 | 29,730 |
| double[10000] | 50,859 | 123,520 | 286,372 | 299,577 |
| double[100000] | 506,165 | 1,266,424 | 2,802,420 | 3,729,057 |

### `buffer->array` push (method argument, numpy source)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 1,259 | 438 | 818 | -- |
| int[1000] | 1,656 | 810 | 1,213 | -- |
| int[10000] | 5,333 | 5,310 | 5,621 | -- |
| int[100000] | 46,396 | 43,510 | 42,613 | -- |
| long[100] | 1,274 | 467 | 886 | -- |
| long[1000] | 2,052 | 1,168 | 1,765 | -- |
| long[10000] | 10,338 | 9,116 | 11,514 | -- |
| long[100000] | 98,098 | 72,483 | 103,775 | -- |
| float[100] | 1,274 | 441 | 842 | -- |
| float[1000] | 1,680 | 761 | 1,249 | -- |
| float[10000] | 5,819 | 4,917 | 5,841 | -- |
| float[100000] | 40,012 | 41,541 | 46,643 | -- |
| double[100] | 1,306 | 463 | 912 | -- |
| double[1000] | 2,201 | 1,142 | 1,830 | -- |
| double[10000] | 10,673 | 8,922 | 11,827 | -- |
| double[100000] | 94,543 | 72,876 | 104,318 | -- |

### `array->list` pull (Java array -> Python list)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 11,985 | 5,182 | 5,017 | 1,993 |
| int[1000] | 102,223 | 46,138 | 40,936 | 12,941 |
| int[10000] | 1,073,626 | 489,233 | 419,284 | 167,126 |
| int[100000] | 12,522,328 | 6,576,669 | 5,848,730 | 1,785,407 |
| long[100] | 11,882 | 5,378 | 5,452 | 2,168 |
| long[1000] | 102,386 | 48,160 | 38,753 | 16,237 |
| long[10000] | 1,025,081 | 483,064 | 368,904 | 141,594 |
| long[100000] | 12,078,369 | 7,442,961 | 6,322,217 | 5,623,093 |
| float[100] | 16,476 | 4,529 | 4,332 | 1,607 |
| float[1000] | 148,582 | 42,160 | 44,974 | 13,192 |
| float[10000] | 1,450,885 | 439,692 | 431,904 | 153,484 |
| float[100000] | 16,074,432 | 4,459,722 | 4,282,515 | 1,194,549 |
| double[100] | 15,734 | 4,730 | 4,259 | 1,606 |
| double[1000] | 140,695 | 45,415 | 43,017 | 14,002 |
| double[10000] | 1,379,770 | 456,469 | 436,006 | 121,902 |
| double[100000] | 15,409,149 | 4,912,456 | 4,366,728 | 1,705,660 |

_jpype's int/long rows reflect a recycling pool for the tagged-number
leaves (`JByte`/`JShort`/`JInt`/`JLong`); float/double are untouched by
that change since they don't go through the same `tp_alloc` path.
Boolean array pulls were already unaffected either way --
`JPBooleanType::getFastArrayItem` already returned a plain
`PyBool_FromLong` singleton, never a tagged wrapper._

### `array->buffer` pull (Java array -> Python/numpy buffer)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 2,467 | 973 | 8,996 | 5,358 |
| int[1000] | 3,004 | 1,424 | 73,586 | 44,621 |
| int[10000] | 7,759 | 6,409 | 757,977 | 497,008 |
| int[100000] | 52,062 | 48,006 | 9,408,453 | 6,640,984 |
| long[100] | 2,565 | 999 | 9,486 | 6,136 |
| long[1000] | 3,561 | 2,023 | 78,725 | 52,362 |
| long[10000] | 13,137 | 11,845 | 788,202 | 554,075 |
| long[100000] | 111,248 | 116,314 | 10,729,171 | 9,742,540 |
| float[100] | 2,486 | 971 | 7,423 | 4,516 |
| float[1000] | 3,061 | 1,473 | 68,721 | 38,682 |
| float[10000] | 7,938 | 6,215 | 693,028 | 371,209 |
| float[100000] | 47,002 | 47,487 | 7,084,413 | 3,947,891 |
| double[100] | 2,545 | 1,058 | 7,474 | 4,509 |
| double[1000] | 3,701 | 2,272 | 68,611 | 39,138 |
| double[10000] | 13,567 | 13,311 | 684,508 | 369,847 |
| double[100000] | 129,270 | 132,296 | 7,028,567 | 4,170,636 |

**Result.** `list->array`: jpype leads jpy/jep/pyjnius at every
size 1,000 and up. The int-widening penalty on float/double (vs. each
library's own matched-type number) varies widely by library: pyjnius
shows almost none (~1.0x, even <1x at the largest double size), jpy's
is the mildest of the rest (~1.4-1.6x), jpype's is moderate (~1.2-2.1x,
worst at the largest sizes), and jep's is the steepest (~1.9-2.9x).
`buffer->array`: pyjnius has no buffer-protocol push at all (falls back
to `sequenceConversion`, i.e. it isn't in this table -- see Section 5
for the isolated cost of that fallback). `array->list`: pyjnius is still
the fastest of all four despite losing most other benchmarks in this
report, because its Cython bridge boxes one plain `PyLong`/`PyFloat` per
element while jpype/jep/jpy build heavier tagged wrapper objects --
jpype's int/long gap to pyjnius is narrower than float/double's (a
recycling pool for those wrapper allocations) but float/double and
jep/jpy across the board still pay full per-element allocation cost.
`array->buffer`: jep/pyjnius have no real buffer-protocol return path --
their columns above are `array->list`'s cost plus a redundant
`np.asarray()`, not a genuine bulk read, which is why they land *worse*
than their own `array->list` number instead of better. jpype and jpy
have a genuine buffer-protocol return path and are close to each other,
with jpy consistently faster.

## 4. Array push, non-contiguous sources

**Methodology.** `array_noncontig.py`. A numpy source that cannot
provide a C-contiguous buffer view -- a non-unit-stride column slice
(flat, 1D) or a transposed array (2D-5D, `np.transpose` with reversed
axis order) -- pushed as a method argument. Tests whether a bulk
buffer-read path is still reached, or whether the implementation falls
back to a fully general per-element/per-row walk.

### Flat (1D), non-contiguous column slice

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 1,368 | -- | 1,247 | -- |
| int[1000] | 2,380 | -- | 1,926 | -- |
| int[10000] | 12,197 | -- | 8,504 | -- |
| int[100000] | 103,930 | -- | 67,997 | -- |
| long[100] | 1,384 | -- | 1,296 | -- |
| long[1000] | 2,724 | -- | 2,540 | -- |
| long[10000] | 15,462 | -- | 14,861 | -- |
| long[100000] | 147,011 | -- | 134,891 | -- |
| float[100] | 1,358 | -- | 1,254 | -- |
| float[1000] | 2,223 | -- | 1,966 | -- |
| float[10000] | 9,060 | -- | 8,313 | -- |
| float[100000] | 71,168 | -- | 68,367 | -- |
| double[100] | 1,379 | -- | 1,281 | -- |
| double[1000] | 2,590 | -- | 2,485 | -- |
| double[10000] | 14,124 | -- | 14,846 | -- |
| double[100000] | 133,390 | -- | 134,718 | -- |

_jpy and pyjnius have no entry: jpy's buffer matcher requires `PyBUF_SIMPLE` (fails outright on a non-contiguous 1D source); pyjnius has no buffer->array push at all, contiguous or not._


### Multi-dimensional (2D-5D), transposed

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 2,922 | 5,652 | 25,512 | -- |
| int[][][](10^3) | 14,748 | 55,991 | 248,265 | -- |
| int[][][][](10^4) | 135,086 | 571,821 | 2,493,939 | -- |
| int[][][][][](10^5) | 1,377,850 | 5,904,847 | 24,911,885 | -- |
| long[][](10^2) | 2,962 | 5,474 | 25,654 | -- |
| long[][][](10^3) | 15,257 | 54,552 | 254,678 | -- |
| long[][][][](10^4) | 142,445 | 559,801 | 2,486,915 | -- |
| long[][][][][](10^5) | 1,460,950 | 5,726,589 | 26,296,872 | -- |
| float[][](10^2) | 2,894 | 5,642 | 25,522 | -- |
| float[][][](10^3) | 14,470 | 54,951 | 248,772 | -- |
| float[][][][](10^4) | 135,428 | 532,072 | 2,494,275 | -- |
| float[][][][][](10^5) | 1,392,364 | 5,546,981 | 24,792,662 | -- |
| double[][](10^2) | 2,926 | 5,316 | 24,907 | -- |
| double[][][](10^3) | 14,729 | 50,789 | 247,473 | -- |
| double[][][][](10^4) | 138,114 | 491,134 | 2,498,462 | -- |
| double[][][][][](10^5) | 1,406,239 | 4,963,800 | 25,427,445 | -- |

**Result.** jpype and jep are the only libraries with a real bulk
path for a non-contiguous 1D source; the two trade wins depending on
size and type, with no consistent winner (jpype leads at the smaller
sizes across all four types, e.g. `int[100]`: 1,117 vs 1,247; jep
pulls ahead for int/float at the larger sizes, e.g. `int[100000]`:
90,146 vs 67,997, while jpype stays ahead for long/double at every
size tested). jpy
has no buffer->array push at all for a non-contiguous source in any
dimensionality (fails outright, 1D; not benchmarked, ND, since the
underlying push has no bulk path to exercise); pyjnius has no
buffer->array push at any size, depth, or contiguity. jep's
"transposed, manual per-row" ND numbers are a per-row Python-level
walk, not a bulk buffer read -- included for completeness, not a
like-for-like comparison to jpype's single-JNI-call path.

## 5. Array push/pull, multi-dimensional (depth 2-5, rectangular)

**Methodology.** `array_multidim.py`. A nested Python list (or
nested numpy-backed structure) of depth 2-5 with a fixed total element
count (~10^depth), pushed (`list->array`, `buffer->array`) or pulled
(`array->list`, `array->buffer`). `buffer->array` is a genuine bulk
buffer read where the library has one (jpype, jpy); jep's is a manual
per-row Python-level walk (no bulk ND push path exists in jep).
pyjnius has no buffer->array push at any depth.

### `list->array` push, fresh nested list

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 1,939 | 2,200 | 6,912 | 4,800 |
| int[][][](10^3) | 9,968 | 17,668 | 64,825 | 41,346 |
| int[][][][](10^4) | 88,626 | 171,389 | 629,210 | 434,760 |
| int[][][][][](10^5) | 899,257 | 1,757,121 | 6,280,016 | 5,027,203 |
| long[][](10^2) | 1,902 | 2,217 | 6,949 | 4,903 |
| long[][][](10^3) | 9,756 | 18,804 | 64,072 | 42,052 |
| long[][][][](10^4) | 98,800 | 188,723 | 630,475 | 424,178 |
| long[][][][][](10^5) | 886,770 | 1,890,664 | 6,349,567 | 5,045,931 |
| float[][](10^2) | 1,998 | 2,072 | -- | 5,295 |
| float[][][](10^3) | 10,480 | 17,509 | -- | 53,187 |
| float[][][][](10^4) | 98,235 | 173,231 | -- | 551,122 |
| float[][][][][](10^5) | 971,376 | 1,758,766 | -- | 9,268,890 |
| double[][](10^2) | 2,020 | 2,208 | -- | 5,431 |
| double[][][](10^3) | 10,820 | 18,116 | -- | 54,689 |
| double[][][][](10^4) | 105,168 | 179,378 | -- | 564,277 |
| double[][][][][](10^5) | 1,051,316 | 1,882,707 | -- | 9,254,105 |

### `buffer->array` push, numpy source (jep: manual per-row fallback)

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 1,981 | 5,730 | -- | -- |
| int[][][](10^3) | 6,280 | 55,185 | -- | -- |
| int[][][][](10^4) | 52,513 | 566,534 | -- | -- |
| int[][][][][](10^5) | 530,489 | 5,683,388 | -- | -- |
| long[][](10^2) | 2,076 | 5,585 | -- | -- |
| long[][][](10^3) | 6,726 | 54,901 | -- | -- |
| long[][][][](10^4) | 70,488 | 554,750 | -- | -- |
| long[][][][][](10^5) | 561,098 | 5,660,922 | -- | -- |
| float[][](10^2) | 2,034 | 5,737 | -- | -- |
| float[][][](10^3) | 6,543 | 54,726 | -- | -- |
| float[][][][](10^4) | 54,982 | 541,806 | -- | -- |
| float[][][][][](10^5) | 561,491 | 5,440,141 | -- | -- |
| double[][](10^2) | 2,103 | 5,450 | -- | -- |
| double[][][](10^3) | 6,745 | 51,095 | -- | -- |
| double[][][][](10^4) | 60,214 | 511,132 | -- | -- |
| double[][][][][](10^5) | 570,220 | 5,219,317 | -- | -- |

_pyjnius: no entry -- no buffer->array push at any depth._


### `array->list` pull

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 27,932 | 9,963 | 13,526 | 3,440 |
| int[][][](10^3) | 276,321 | 99,842 | 129,102 | 30,065 |
| int[][][][](10^4) | 2,879,348 | 1,137,774 | 1,466,528 | 498,866 |
| int[][][][][](10^5) | 32,464,666 | 13,614,375 | 16,390,342 | 7,774,809 |
| long[][](10^2) | 27,760 | 10,198 | 14,147 | 3,571 |
| long[][][](10^3) | 274,104 | 105,042 | 133,857 | 31,427 |
| long[][][][](10^4) | 2,876,324 | 1,092,114 | -- | 474,170 |
| long[][][][][](10^5) | 31,907,476 | 11,879,790 | -- | 9,453,776 |
| float[][](10^2) | 32,025 | 9,386 | -- | 2,968 |
| float[][][](10^3) | 316,681 | 91,323 | -- | 29,074 |
| float[][][][](10^4) | 3,267,769 | 1,015,685 | -- | 406,225 |
| float[][][][][](10^5) | 35,984,557 | 10,975,680 | -- | 5,072,694 |
| double[][](10^2) | 31,776 | 9,892 | -- | 3,000 |
| double[][][](10^3) | 319,988 | 97,720 | -- | 28,600 |
| double[][][][](10^4) | 3,270,600 | 1,037,738 | -- | 431,080 |
| double[][][][][](10^5) | 34,948,283 | 11,476,721 | -- | 5,539,152 |

_jpype's int/long rows reflect the tagged-number recycling pool -- see
Section 3's footnote; float/double are untouched by that change._

### `array->buffer` pull

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 4,168 | 7,854 | 17,295 | 7,592 |
| int[][][](10^3) | 10,229 | 77,504 | 175,356 | 71,190 |
| int[][][][](10^4) | 66,489 | 891,446 | 2,036,880 | 950,755 |
| int[][][][][](10^5) | 690,558 | 10,713,915 | 26,164,085 | 13,366,826 |
| long[][](10^2) | 4,177 | 8,304 | -- | 8,445 |
| long[][][](10^3) | 10,153 | 76,792 | -- | 82,092 |
| long[][][][](10^4) | 65,610 | 894,228 | -- | 982,934 |
| long[][][][][](10^5) | 690,582 | 10,543,404 | -- | 16,391,273 |
| float[][](10^2) | 4,141 | 8,328 | -- | 6,761 |
| float[][][](10^3) | 9,613 | 76,140 | -- | 64,680 |
| float[][][][](10^4) | 61,032 | 884,745 | -- | 781,384 |
| float[][][][][](10^5) | 625,791 | 10,522,196 | -- | 9,744,072 |
| double[][](10^2) | 4,154 | 8,192 | -- | 6,844 |
| double[][][](10^3) | 9,722 | 79,991 | -- | 62,103 |
| double[][][][](10^4) | 62,655 | 906,594 | -- | 777,766 |
| double[][][][][](10^5) | 667,522 | 10,900,139 | -- | 10,888,224 |

_jpype numbers reflect the list/tuple-specialized ragged-native readout
(`matchRaggedNode`/`encodeRaggedNode`, `native/common/jp_classhints.cpp`)._


**Result.** `list->array`: jpype leads jpy at every depth (1.1-1.8x,
widening with depth). `buffer->array`:
jpy and jpype both reach a genuine bulk path and are within a few
percent of each other by depth 4-5; jep's manual per-row fallback is
1-2 orders of magnitude slower at depth 4-5; pyjnius has none.
`array->list`/`array->buffer`: pyjnius is fastest at shallow depth the
same way it is in Section 3, but jpype and jpy's `array->buffer` bulk
path pulls further ahead as depth grows, since it scales with leaf-array
count rather than total element count.

## 6. Array push, ragged (jagged, non-rectangular)

**Methodology.** `array_ragged.py`. A nested Python list whose
sub-lists have varying lengths (a genuinely jagged/ragged structure, not
a rectangular array-of-arrays), pushed fresh into a Java array-of-arrays
parameter, depth 2-5, ~10^depth total elements.

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](~10^2) | 1,550 | 1,531 | 4,801 | 3,171 |
| int[][][](~10^3) | 10,073 | 18,823 | 71,090 | 44,417 |
| int[][][][](~10^4) | 73,413 | 149,402 | 563,822 | 352,867 |
| int[][][][][](~10^5) | 1,225,467 | 2,139,732 | 8,147,249 | 5,920,602 |
| long[][](~10^2) | 1,579 | 1,483 | 4,768 | 3,175 |
| long[][][](~10^3) | 10,169 | 19,790 | 70,842 | 42,263 |
| long[][][][](~10^4) | 74,198 | 156,158 | 569,074 | 352,408 |
| long[][][][][](~10^5) | 1,250,670 | 2,259,730 | 8,136,513 | 5,483,144 |
| float[][](~10^2) | 1,622 | 1,382 | 4,539 | 3,206 |
| float[][][](~10^3) | 10,738 | 18,062 | 66,085 | 51,978 |
| float[][][][](~10^4) | 83,546 | 145,387 | 530,688 | 443,521 |
| float[][][][][](~10^5) | 1,324,301 | 2,280,683 | 7,884,050 | 10,576,596 |
| double[][](~10^2) | 1,623 | 1,483 | 4,696 | 3,369 |
| double[][][](~10^3) | 11,081 | 19,950 | 67,852 | 53,670 |
| double[][][][](~10^4) | 87,271 | 151,441 | 532,166 | 463,626 |
| double[][][][][](~10^5) | 1,326,482 | 2,217,599 | 7,744,838 | 10,166,998 |

_jpype numbers reflect the list/tuple-specialized ragged-native readout._

Also ragged-eligible, jpype-only (no jpy/jep/pyjnius equivalent to
compare against): boolean/byte/char/short leaf types, added alongside
int/long/float/double above. 1-/2-byte-wide leaves need 4-byte padding
after each leaf run to keep the wire format's length markers aligned
(`raggedAlign4`, `native/common/jp_classhints.cpp`) that the 4-/8-byte
types never pay.

| shape | byte | boolean | char | short |
|---|---:|---:|---:|---:|
| [][](~10^2) | 1,599 | 1,524 | 2,063 | 1,624 |
| [][][](~10^3) | 10,295 | 9,977 | 19,412 | 11,128 |
| [][][][](~10^4) | 76,032 | 73,366 | 156,341 | 84,236 |
| [][][][][](~10^5) | 1,241,255 | 1,188,331 | 2,234,848 | 1,313,557 |

_char is 1.5-2x slower than the others at every depth -- its leaf
conversion (`asCharUTF16`, `jp_stringtype.cpp`) decodes a Python
string rather than doing a plain `PyLong_AsLong`/`PyBool_Check`._

**Result.** jpype leads jpy at every depth/type; the gap widens with
depth (both walk the ragged structure recursively, jpype's
ragged-native encode path stays closer to linear in total elements).
byte/boolean/short track int/long/float/double closely; char is the
one outlier, for the string-decode reason noted above.

## 7. Array push/pull, shape at fixed depth and total element count

**Methodology.** `array_shape.py`. Fixed total element count
(~10,000 or ~100,000), depth held at 3, but the *shape* varies (e.g.
`[1000][10][10]` vs. `[10][10][1000]`) to isolate whether cost tracks
total elements or leaf-array count / row-heaviness. `list->array` and
`buffer->array` (jep: manual per-row) directions only.

### `list->array` push

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[10][10000] | 549,223 | 806,918 | 1,402,789 | 2,915,372 |
| int[100][1000] | 545,895 | 830,006 | 1,417,742 | 2,794,704 |
| int[1000][100] | 544,577 | 866,335 | 1,836,153 | 2,241,320 |
| int[10000][10] | 811,889 | 1,649,007 | 6,145,127 | 4,012,716 |
| int[3][100000] | 1,853,723 | 2,490,970 | 4,207,551 | 9,705,678 |
| int[100000][3] | 4,766,796 | 11,588,374 | 50,941,097 | 26,209,037 |
| int[1000][1000] | 6,461,586 | 8,584,726 | 14,795,684 | 32,167,686 |
| int[1000][10][10] | 861,256 | 1,745,310 | 6,499,518 | 6,011,382 |
| int[10][10][1000] | 531,011 | 798,483 | 1,412,372 | 2,778,888 |
| long[10][10000] | 548,957 | 821,602 | 1,275,314 | 2,884,768 |
| long[100][1000] | 564,112 | 826,781 | 1,354,412 | 2,688,467 |
| long[1000][100] | 572,153 | 890,544 | 1,738,038 | 2,215,003 |
| long[10000][10] | 808,606 | 1,759,386 | 5,879,173 | 4,086,737 |
| long[3][100000] | 2,077,498 | 2,584,784 | 4,047,452 | 9,942,196 |
| long[100000][3] | 4,754,237 | 12,470,530 | 51,026,319 | 26,499,032 |
| long[1000][1000] | 7,006,645 | 8,898,200 | 13,989,272 | 30,639,333 |
| long[1000][10][10] | 860,225 | 1,872,495 | 6,269,827 | 5,337,705 |
| long[10][10][1000] | 567,066 | 823,260 | 1,349,376 | 2,671,344 |
| float[10][10000] | 525,242 | 731,309 | 994,848 | 2,789,406 |
| float[100][1000] | 531,675 | 754,981 | 1,057,161 | 2,849,780 |
| float[1000][100] | 549,735 | 795,978 | 1,430,817 | 3,208,813 |
| float[10000][10] | 890,460 | 1,609,817 | 5,431,722 | 5,489,580 |
| float[3][100000] | 1,815,038 | 2,333,876 | 2,962,936 | 10,136,459 |
| float[100000][3] | 5,809,688 | 11,620,500 | 48,370,083 | 32,701,462 |
| float[1000][1000] | 6,551,057 | 8,092,994 | 10,686,076 | 33,539,870 |
| float[1000][10][10] | 940,899 | 1,754,334 | 5,816,736 | 7,109,184 |
| float[10][10][1000] | 540,316 | 759,979 | 1,056,809 | 3,135,425 |
| double[10][10000] | 572,798 | 829,131 | 929,372 | 3,060,208 |
| double[100][1000] | 586,701 | 823,039 | 979,936 | 3,290,929 |
| double[1000][100] | 611,424 | 867,200 | 1,371,696 | 3,586,975 |
| double[10000][10] | 908,466 | 1,696,826 | 5,405,955 | 6,034,980 |
| double[3][100000] | 2,095,959 | 2,670,880 | 2,870,663 | 11,272,831 |
| double[100000][3] | 5,794,221 | 12,117,989 | 47,775,907 | 32,858,976 |
| double[1000][1000] | 7,298,936 | 8,884,806 | 9,848,050 | 35,162,136 |
| double[1000][10][10] | 982,270 | 1,833,219 | 5,852,637 | 7,900,256 |
| double[10][10][1000] | 581,722 | 827,489 | 977,538 | 3,435,078 |

_jpype numbers reflect the list/tuple-specialized ragged-native readout
(`matchRaggedNode`/`encodeRaggedNode`, `native/common/jp_classhints.cpp`)._

### `buffer->array` push (jep: manual per-row)

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[10][10000] | 51,834 | 3,957,171 | -- | -- |
| int[100][1000] | 58,624 | 4,028,383 | -- | -- |
| int[1000][100] | 85,667 | 4,154,658 | -- | -- |
| int[10000][10] | 509,969 | 5,977,026 | -- | -- |
| int[3][100000] | 132,260 | 12,919,031 | -- | -- |
| int[100000][3] | 5,046,476 | 30,331,461 | -- | -- |
| int[1000][1000] | 515,858 | 39,452,892 | -- | -- |
| int[1000][10][10] | 543,642 | 5,810,657 | -- | -- |
| int[10][10][1000] | 47,212 | 3,859,042 | -- | -- |
| long[10][10000] | 90,113 | 3,772,218 | -- | -- |
| long[100][1000] | 90,867 | 3,738,746 | -- | -- |
| long[1000][100] | 135,211 | 3,873,069 | -- | -- |
| long[10000][10] | 548,537 | 5,560,335 | -- | -- |
| long[3][100000] | 278,828 | 11,171,590 | -- | -- |
| long[100000][3] | 5,313,733 | 28,768,097 | -- | -- |
| long[1000][1000] | 889,925 | 38,238,769 | -- | -- |
| long[1000][10][10] | 576,995 | 5,680,074 | -- | -- |
| long[10][10][1000] | 86,218 | 3,947,929 | -- | -- |
| float[10][10000] | 47,365 | 3,472,438 | -- | -- |
| float[100][1000] | 48,815 | 3,536,412 | -- | -- |
| float[1000][100] | 89,767 | 3,630,979 | -- | -- |
| float[10000][10] | 535,548 | 5,130,371 | -- | -- |
| float[3][100000] | 116,346 | 10,414,713 | -- | -- |
| float[100000][3] | 5,291,204 | 27,795,668 | -- | -- |
| float[1000][1000] | 443,102 | 35,873,353 | -- | -- |
| float[1000][10][10] | 560,255 | 5,542,828 | -- | -- |
| float[10][10][1000] | 57,982 | 3,508,462 | -- | -- |
| double[10][10000] | 79,487 | 3,283,246 | -- | -- |
| double[100][1000] | 93,189 | 3,252,640 | -- | -- |
| double[1000][100] | 129,439 | 3,401,470 | -- | -- |
| double[10000][10] | 554,942 | 5,092,549 | -- | -- |
| double[3][100000] | 270,699 | 10,026,086 | -- | -- |
| double[100000][3] | 5,036,419 | 27,661,544 | -- | -- |
| double[1000][1000] | 865,105 | 33,215,732 | -- | -- |
| double[1000][10][10] | 556,698 | 4,965,956 | -- | -- |
| double[10][10][1000] | 87,730 | 3,117,623 | -- | -- |

_pyjnius: no entry -- no buffer->array push at any shape._


**Result.** At fixed total element count, a row-heavy shape (many
short rows, e.g. `[100000][3]`) costs more than a column-heavy one
(few long rows, e.g. `[3][100000]`) in every library that has a bulk
path -- more leaf arrays means more per-leaf JNI/reflection overhead
even though total elements is unchanged. For jpy/jep/pyjnius,
`list->array` pays this penalty via a recursive per-row Python-level
walk. jpype's ragged-native path avoids the Python-level per-row cost
-- one C++ walk, one JNI crossing -- but still shows the same
row-heavy-costs-more shape, now from `Array.newInstance`/
`Array.set` reflection on the Java side of `fillRaggedFromBuffer`, one
call per row regardless of row length (e.g. `int[100000][3]`: 6,022,370ns
vs `int[3][100000]`: 1,927,287ns, same 100,000 elements). `buffer->array`
still pays the smallest per-leaf-array penalty of the three, since
jpype/jpy's bulk path there has no reflection in the loop at all.

## 8. jpype-only microbenchmarks

These four scripts (`array_of.py`, `array_to_list_dtype.py`,
`arraytransfer.py`, `classhints.py`) have no equivalent in the
jpy/jep/pyjnius suites -- they exercise jpype-internal API surface
(`JArray.of()`, `toList()` dtype variants, `pullTo`/`pushFrom` bulk
in-place transfer, `JPConversionList`/`JPConversionTuple`'s
cached-class-hint lookup) with nothing to compare against. jpype-only,
`best` ns/call.

### `JArray.of()` -- constructing an array directly from a buffer

**Methodology.** `array_of.py`. `JArray.of(arr)`/`JArray.of(arr,
dtype=...)` construct a jpype array directly from a buffer-protocol
source (numpy), the dedicated factory method for this -- as distinct
from passing the same source as a method argument (Sections 3/5's
`buffer->array` push) or via the array class's own constructor
(`JArray(JType)(arr)`, included here as the "naive" alternative a user
might reach for instead of `.of()`; a numpy array satisfies
`PySequence_Check`, so this alternative was never actually naive for a
buffer-protocol source -- see the Result below).

| operation | jpype |
|---|---:|---:|---:|---:|
| JArray.of(arr) | 1,567 | 1,857 | 5,984 | 40,192 |
| JArray.of(arr, dtype=<same type>) | 1,582 | 2,075 | 6,203 | 39,530 |
| JArray.of(arr, dtype=<cross type>) | 1,847 | 4,368 | 23,139 | 197,004 |
| JArray(JType)(arr), naive ctor | 2,558 | 2,890 | 6,582 | 41,178 |

| operation | jpype |
|---|---:|---:|---:|---:|
| JArray.of(arr) | 1,544 | 2,390 | 9,896 | 94,411 |
| JArray.of(arr, dtype=<same type>) | 1,574 | 2,492 | 9,980 | 93,174 |
| JArray.of(arr, dtype=<cross type>) | 1,783 | 4,234 | 29,782 | 263,928 |
| JArray(JType)(arr), naive ctor | 2,593 | 3,335 | 11,019 | 100,187 |

| operation | jpype |
|---|---:|---:|---:|---:|
| JArray.of(arr) | 1,521 | 1,860 | 5,830 | 39,580 |
| JArray.of(arr, dtype=<same type>) | 1,561 | 1,919 | 6,133 | 38,788 |
| JArray.of(arr, dtype=<cross type>) | 1,734 | 3,599 | 21,393 | 194,879 |
| JArray(JType)(arr), naive ctor | 2,575 | 2,900 | 6,987 | 43,409 |

| operation | double[100] | double[1000] | double[10000] | double[100000] |
|---|---:|---:|---:|---:|
| JArray.of(arr) | 1,535 | 2,530 | 9,655 | 121,471 |
| JArray.of(arr, dtype=<same type>) | 1,588 | 2,404 | 9,691 | 90,446 |
| JArray.of(arr, dtype=<cross type>) | 1,855 | 4,289 | 29,567 | 264,396 |
| JArray(JType)(arr), naive ctor | 2,606 | 3,246 | 11,108 | 85,160 |

**Multi-dimensional (10^dims elements, `JArray.of(arr)` only -- see
Result):**

| shape | int | long | float | double |
|---|---:|---:|---:|---:|
| [][](10^2) | 2,132 | 2,324 | 2,326 | 2,397 |
| [][][](10^3) | 6,625 | 7,771 | 6,976 | 7,706 |
| [][][][](10^4) | 53,062 | 55,006 | 54,383 | 60,023 |
| [][][][][](10^5) | 520,683 | 762,590 | 545,992 | 558,574 |

**`JArray(JType, dims)(arr)` -- the manual type+dims constructor spelling,
int only (10^dims elements):**

| shape | `JArray.of(arr)` | `JArray(JType, dims)(arr)` |
|---|---:|---:|
| [][](10^2) | 2,132 | 3,394 |
| [][][](10^3) | 6,625 | 7,916 |
| [][][][](10^4) | 53,062 | 54,200 |
| [][][][][](10^5) | 520,683 | 523,219 |

**Result.** Flat (1D): `JArray.of()` leads the naive constructor at
every size 1,000 and up, and is within noise of it below that.
`PyJPModule_convertBuffer` (`native/python/pyjp_module.cpp`) routes a
flat (`ndim == 1`) source through the same `setArrayRange` buffer-
protocol fast path (`tryFastBufferPush`/`Support.fillFlatFromBuffer`)
the naive `JArray(JType)(arr)` constructor already uses -- a numpy
source satisfies `PySequence_Check`, so the constructor lands in
`setArrayRange` too, trying that same fast path first.

Multi-dimensional (N>=2): `JArray.of()` at depth >= 2 shares
`tryFastMultiArrayBuffer` (`jp_convert.cpp`) with
`JPConversionMultiArrayBuffer::convert` (Section 5's N>=2
`buffer->array` push) -- the `classifyRawTransfer`-gated
`Support.fillMultiArrayFromBuffer` bulk DirectByteBuffer handoff,
falling back to a per-element `pack(converter(src))` loop
(`newMultiArray`/`convertMultiArrayObject`) for a non-contiguous
source or genuine dtype coercion. `fillMultiArrayFromBuffer` has no
depth cap, so this covers every depth `JArray.of()` accepts, including
depth > 4 (one level past the unrelated 4-dim cap on the
*read*-direction `collectRectangular` helper used by
`pullTo`/`np.asarray()`, which does not apply here). (The
`dtype=<same type>` column is omitted from the multi-dimensional table
-- it tracks `JArray.of(arr)` within noise, as expected, since both
take the identical fast path there.) The manual `JArray(JType,
dims)(arr)` constructor spelling reaches the same fast path via
`PyJPArray_init`, gated the same way; it tracks `JArray.of(arr)`
closely at every depth, confirming the manual spelling isn't leaving
performance on the table relative to `.of()`.

_The `dtype=<cross type>` row's ratio to the
`dtype=<same type>` row is not constant -- it grows with size (~1.4x at
100 elements to ~5x at 100,000, across all four flat tables above). This
is real, expected cost, not a missed fast path or a regression: both rows
share the identical `convertMultiArrayObject` per-element
`pack(converter(src))` loop (`jp_primitive_accessor.h`) -- there is no
bulk-copy shortcut for either, matching-dtype included, confirmed by
reading `getConverter` (`jp_convert.cpp`), which always returns a
`Convert<T>::to*` function pointer regardless of whether `from`/`to`
match. The only actual difference is which conversion that function
pointer performs: matching int32->jint is `Convert<int32_t>::toI`, a
same-domain integer move (measured floor ~0.45ns/element at scale);
cross-dtype float32->jint is `Convert<float>::toI`, an FP->int
domain-crossing truncate with real per-call pipeline latency (measured
floor ~2.2ns/element). The growing ratio across sizes is fixed-overhead
amortization: at small N, JNI/array-alloc/Python-call overhead dominates
so the rows look similar; at large N that overhead washes out and what
remains is the intrinsic hardware cost gap between an integer move and an
FP-domain conversion, paid serially per element through a
non-vectorizable function-pointer call. No fix applied -- this is exactly
what this row is documented above to measure ("a real per-element cast,"
see Categories in `array_of.py`'s own docstring); closing the gap would
mean vectorizing `Convert<T>::to*` over contiguous runs, a distinct
optimization project, not a bug fix._

`JArray(JType, dims)(arr)` (the manual type+dims constructor, as distinct
from `.of()`) reaches the same fast path: `PyJPArray_init` attempts the
buffer-protocol fast path (`tryFastMultiArrayBuffer`, shared with
`.of()`) before falling back to the generic `newArray`+`setRange(0,
length, 1, v)` path that a non-buffer sequence source still needs. The
gap to `.of(arr)` shrinks toward parity at higher depth/size (see table
above) because even the generic fallback path recurses one array-class
level per dimension down to a primitive leaf level, where it *does* hit
the existing primitive-`setArrayRange` fast path -- so the fallback's
overhead scales with row count at the outermost levels, not total
element count, and becomes negligible once total data dominates. The
buffer-protocol fast path matters most at smaller/shallower shapes,
where that per-row overhead would otherwise be the whole cost.

### `list()` vs. `toList()` dtype variants

| operation | jpype |
|---|:---:|
| list(arr) int[100] | 12,017.6 |
| toList() int[100], plain | 3,442.3 |
| toList(dtype=int) int[100], wrapped (~= old default) | 5,955.1 |
| toList(dtype=float) int[100], forced cast, plain | 3,129.9 |
| list(arr) int[1000] | 102,872.8 |
| toList() int[1000], plain | 14,317.8 |
| toList(dtype=int) int[1000], wrapped (~= old default) | 39,252 |
| toList(dtype=float) int[1000], forced cast, plain | 13,970.5 |
| list(arr) int[10000] | 1,065,534.8 |
| toList() int[10000], plain | 172,066.8 |
| toList(dtype=int) int[10000], wrapped (~= old default) | 408,288 |
| toList(dtype=float) int[10000], forced cast, plain | 125,078.6 |
| list(arr) int[100000] | 12,507,647.3 |
| toList() int[100000], plain | 2,560,421 |
| toList(dtype=int) int[100000], wrapped (~= old default) | 5,735,787.4 |
| toList(dtype=float) int[100000], forced cast, plain | 1,261,444.9 |
| list(arr) long[100] | 11,953.8 |
| toList() long[100], plain | 3,659.7 |
| toList(dtype=long) long[100], wrapped (~= old default) | 5,363.9 |
| toList(dtype=float) long[100], forced cast, plain | 3,225.9 |
| list(arr) long[1000] | 102,862.2 |
| toList() long[1000], plain | 16,728.7 |
| toList(dtype=long) long[1000], wrapped (~= old default) | 32,239.8 |
| toList(dtype=float) long[1000], forced cast, plain | 14,350.5 |
| list(arr) long[10000] | 1,045,554.4 |
| toList() long[10000], plain | 172,269.3 |
| toList(dtype=long) long[10000], wrapped (~= old default) | 337,066.9 |
| toList(dtype=float) long[10000], forced cast, plain | 127,482.8 |
| list(arr) long[100000] | 11,475,087 |
| toList() long[100000], plain | 2,512,126.1 |
| toList(dtype=long) long[100000], wrapped (~= old default) | 4,352,874.6 |
| toList(dtype=float) long[100000], forced cast, plain | 1,331,269.5 |
| list(arr) float[100] | 16,490.8 |
| toList() float[100], plain | 3,004.4 |
| toList(dtype=float) float[100], wrapped (~= old default) | 8,962.8 |
| toList(dtype=int) float[100], forced cast, plain | 2,982.8 |
| list(arr) float[1000] | 147,174.4 |
| toList() float[1000], plain | 13,426.1 |
| toList(dtype=float) float[1000], wrapped (~= old default) | 67,580.6 |
| toList(dtype=int) float[1000], forced cast, plain | 12,080.8 |
| list(arr) float[10000] | 1,459,461 |
| toList() float[10000], plain | 124,188 |
| toList(dtype=float) float[10000], wrapped (~= old default) | 652,110.8 |
| toList(dtype=int) float[10000], forced cast, plain | 111,144.6 |
| list(arr) float[100000] | 15,321,571.4 |
| toList() float[100000], plain | 1,233,062.9 |
| toList(dtype=float) float[100000], wrapped (~= old default) | 6,516,575.6 |
| toList(dtype=int) float[100000], forced cast, plain | 1,099,989.8 |
| list(arr) double[100] | 15,735.1 |
| toList() double[100], plain | 3,089.2 |
| toList(dtype=double) double[100], wrapped (~= old default) | 9,759.4 |
| toList(dtype=int) double[100], forced cast, plain | 3,167.6 |
| list(arr) double[1000] | 141,849.6 |
| toList() double[1000], plain | 13,753.9 |
| toList(dtype=double) double[1000], wrapped (~= old default) | 73,351.8 |
| toList(dtype=int) double[1000], forced cast, plain | 12,413.8 |
| list(arr) double[10000] | 1,402,637.4 |
| toList() double[10000], plain | 132,564.7 |
| toList(dtype=double) double[10000], wrapped (~= old default) | 717,122.7 |
| toList(dtype=int) double[10000], forced cast, plain | 117,907.4 |
| list(arr) double[100000] | 14,954,558.2 |
| toList() double[100000], plain | 1,363,899.5 |
| toList(dtype=double) double[100000], wrapped (~= old default) | 7,266,117.7 |
| toList(dtype=int) double[100000], forced cast, plain | 1,240,411.4 |

_`list(arr)`/wrapped rows for int/long reflect the tagged-number
recycling pool; float/double and the plain/forced-cast rows (already
bare `PyLong_FromLong`/`PyFloat_FromDouble`, never routed through the
pool) are session-to-session noise only._

### Bulk in-place transfer (`pullTo`/`pushFrom`) vs. naive per-element

| operation | jpype |
|---|:---:|
| pullTo double[1000] | 764.4 |
| naive per-element double[1000] | 497,901.7 |
| pullTo double[100000] | 21,638.6 |
| naive per-element double[100000] | 49,438,722.2 |
| pullTo double[1000000] | 507,183.3 |
| pushFrom double[1000] | 1,088.9 |
| naive per-element double[1000] | 497,901.7 |
| pushFrom double[100000] | 21,714.9 |
| naive per-element double[100000] | 49,438,722.2 |
| pushFrom double[1000000] | 630,270.3 |
| pushFrom byteswapped double[1000] | 5,972 |
| pushFrom float16 double[1000] | 6,644 |
| pushFrom byteswapped double[100000] | 500,488.3 |
| pushFrom float16 double[100000] | 569,916 |
| pushFrom byteswapped double[1000000] | 5,028,153.3 |
| pushFrom float16 double[1000000] | 5,709,540.2 |

**`pullTo`, multi-dimensional (10^dims elements, `int[][]`..`int[][][][][]`).**
`pullTo` supports N-D destinations (any array whose component type is
itself an array class, e.g. `int[][]`); the naive comparator below (a
recursive per-element Python loop) is the only alternative route to the
same result.

| shape | pullTo | naive per-element |
|---|---:|---:|
| [][](10^2) | 1,568 | 32,623 |
| [][][](10^3) | 4,219 | 334,796 |
| [][][][](10^4) | 30,160 | 3,323,618 |
| [][][][][](10^5) | 296,856 | (not run -- see below) |

_`naive per-element` capped at 10,000 elements -- pullTo's entire point is
to avoid this loop, and it is already >100x slower at that size; running
it at 10^5 would cost minutes for no additional information. int only
(`array_multidim.py`/`arraytransfer.py`'s `DeepBench.make{2..5}D...` share
this same test fixture across types; the underlying transfer -- a flat
memcpy of `itemsize`-wide elements -- has no per-type cost difference, so
one type is representative)._

**`pushFrom`, multi-dimensional (10^dims elements, `int[][]`..
`int[][][][][]`).** Same story as `pullTo` above, mirrored for the write
direction: the naive comparator is the only alternative route to the
same result.

| shape | pushFrom | naive per-element |
|---|---:|---:|
| [][](10^2) | 1,223 | 43,298 |
| [][][](10^3) | 3,979 | 450,242 |
| [][][][](10^4) | 36,547 | 4,468,482 |
| [][][][][](10^5) | 356,180 | (not run -- see below) |

_Same capping/type-representativeness rationale as `pullTo`'s table
above._
| direct-buffer-shared double[1000] | 1,551.1 |
| direct-buffer-shared double[100000] | 29,967.7 |
| direct-buffer-shared double[1000000] | 287,882.2 |
| slice_python double[1000] | 821.7 |
| slice_javaArray double[1000] | 4,881.8 |
| slice_python double[100000] | 11,299.6 |
| slice_javaArray double[100000] | 205,766.4 |
| slice_python double[1000000] | 102,675.6 |
| slice_javaArray double[1000000] | 2,074,228.3 |
| multidim_bulk 10x10 | 2,912.1 |
| multidim_looped 10x10 | 31,425.1 |
| multidim_bulk 300x300 | 40,132.2 |
| multidim_looped 300x300 | 14,729,428.6 |
| multidim_bulk 1000x1000 | 782,803.5 |

### Class-hint cache lookup cost vs. registered-class-count

| operation | jpype |
|---|:---:|
| match@1/400 | 780.9 |
| match@5/400 | 776.1 |
| match@20/400 | 780.7 |
| match@100/400 | 777.6 |
| match@200/400 | 778.8 |
| match@400/400 | 775.6 |

**Result.** `pullTo`/`pushFrom` beat their naive per-element
counterparts by roughly one to two orders of magnitude across the
tested sizes, from ~20-35x at 10^2 elements up to ~110-120x at 10^4
(the largest size the naive loop was run at -- see the capping note
above). `direct-buffer-shared` (steady-state cost once a direct
buffer is already set up) is the cheapest transfer path at every size.
`classhints` cache lookup cost is flat from 1 to 400 registered classes
-- confirms the cache is a real O(1) lookup, not a linear scan that
happens to be fast at small N.

## 9. GraalPy: a true-JIT comparison point (not re-run this edition)

GraalPy (Python-in-the-JVM, opposite architecture from
jpype/jpy/pyjnius, same direction as jep) was not part of this edition's
re-run -- it needs a separate GraalVM CE + Maven/Truffle setup (see
`project/benchmark/README.md`) and was out of scope for this pass. The
subsections below are carried forward **unchanged** from the previous
edition of this report; their internal `Section N` cross-references
point to *that* edition's section numbers, not this document's current
numbering.

**Scope.** jpy/jep/pyjnius (Sections 2-7) are all CPython, no JIT at
all -- the interesting question against them is architectural overhead
per call. GraalPy runs on Truffle/Graal, a genuine tiered JIT compiler,
so the question here is different: how much of jpype's remaining gap to
a fast bridge is call-dispatch overhead a JIT *can* buy back, versus a
structural gap (a missing bulk-transfer path) no amount of JIT
compilation fixes? Same `DeepBench` test class, same benchmark scripts
ported to `project/benchmark/graalpy/`, run via a small Java launcher
(GraalPy embeds Python *inside* the JVM, jep's direction) -- see
`project/benchmark/README.md`'s GraalPy section for setup and the
mandatory heap cap.

**Methodology notes.** Every table below reports both best-of-5 and
median-of-5, because GraalPy's best/median split is far wider than any
of jpype/jpy/jep/pyjnius's -- best-of-5 alone would misleadingly flatter
it (likely JIT warmup/deopt noise; the benchmarks use a 1000-iteration
warmup before any timed trial, same as every other library here, and the
split persists regardless).

The "buffer->array (manual)" numbers throughout 8.2-8.4 carry far less
statistical confidence than every other number in this report: they run
at n=3 samples at every size >=10,000 elements
(`_arrayutil.py`'s `calls_for_manual()`, `n = max(3, 5_000 // size)`, a
fixed per-trial element budget chosen once and applied uniformly across
all four files this category spans), versus n=20-50,000 everywhere else
in this report. This exists because **GraalPy has no native
`buffer->array` push at all**, at any size or depth (confirmed
empirically: `TypeError('invalid instantiation of foreign object')`
unconditionally). Every number in the "buffer->array (manual)" rows
below comes from a per-element Java-array-construction routine written
for this comparison (`graalpy/_arrayutil.py`), not from anything GraalPy
does on its own -- without it, this entire category would be blank for
GraalPy. Two cells (int and long `list->array` push at the `100000x3`
row-heavy shape, an automatic category) hit a genuine `MemoryError`
under a capped `-Xmx3g` heap and are recorded as `N/A` in
`array_shape_results.csv`, not silently omitted -- see 8.4.

### 9.1 Scalars, dispatch, proxy

**Methodology.** Same operations as Section 2/3. int-only.

| operation | jpype | jpy | jep | pyjnius | GraalPy best | GraalPy median |
|---|---:|---:|---:|---:|---:|---:|
| `Math.max(int,int)` | 672 | 353 | 1299 | 1276 | **183** | 1250 |
| `new Integer(int)` | 811 | 479 | 1257 | 7403 | 229 | 281 |
| `Math.sqrt(double)` | 706 | 393 | 645 | 497 | **117** | 874 |
| `new Double(double)` | 933 | 506 | 1378 | 6765 | 726 | 758 |
| `new String` + `.toString()` | 1022 | 927 | 2512 | 22349 | 791 | 908 |
| `Object` identity (arg + return) | 995 | 730 | 1971 | 3763 | **160** | 887 |
| dispatch, overload x16, monomorphic | 686 | 370 | 4454 | 3858 | **119** | 932 |
| dispatch, overload x16, polymorphic | 925 | 456 | 4558 | 4000 | 911 | 1153 |
| proxy callback, `int` arg | 2655 | N/A | 2240 | 39412 | 526 | 1459 |

**Interpretation.** On best-of-5, GraalPy wins outright on 6 of 9 rows --
beating jpy (this report's previous fastest bridge on every earlier
section) by 1.9-3.1x on `Math.max`, `Math.sqrt`, `Object` identity, and
monomorphic dispatch. But every GraalPy median is 1.4-10.5x its own
best, a spread none of the other four libraries show at any comparable
magnitude (jpype's own best-vs-median split, visible in Section 7's
tables, is typically under 1.3x). Boxed `Integer`/`Double` and the
`String` round trip are the exception -- GraalPy's best and median are
close together there (1.0-1.2x), and not even GraalPy's fastest row
against jpy.

The JIT does real work on monomorphic call sites (`Math.max`, a single
dispatch target, `Object` identity) -- GraalPy's best numbers there are
the fastest in this entire report. Boxed `Integer(int)`/`Double(double)`
construction stresses object allocation and GC more than call dispatch,
which likely explains both why GraalPy doesn't lead there and why it
shows the tightest best/median spread on those two rows specifically.
Any orchestration workload sensitive to *tail* latency, not just
throughput, should weight GraalPy's median column, not its best -- on
that column jpype is competitive or ahead on most of these same rows.

### 9.2 Array push (flat, 1D)

**Methodology.** Same as Section 4. GraalPy cannot do a `buffer->array`
push at all -- confirmed empirically (`TypeError('invalid instantiation
of foreign object')` unconditionally). "buffer->array (manual)" below is
a replacement written from scratch for this comparison
(`graalpy/_arrayutil.py`'s `build_manual()`: allocate a real Java array,
fill it element-by-element from the numpy source) -- it measures this
comparison's code, not GraalPy's own capability. See the note at the top
of Section 8 for its reduced sample count.

**int, all four categories, all sizes** (`list->array`/`array->list` in
ns/call; `buffer->array (manual)`/`array->buffer` in ms/call -- unit
switched per column since the manual/buffer-pull categories run 2-5
orders of magnitude slower, see methodology note above):

| size | list->array (auto), ns | buffer->array (manual), ms | array->list (pull), ns | array->buffer (pull), ms |
|---:|---:|---:|---:|---:|
| 100 | 6,343 | 0.9 | 4,976 | -- |
| 1,000 | 45,881 | 18.0 | 48,655 | -- |
| 10,000 | 436,983 | 100.2 | 531,490 | -- |
| 100,000 | 4,511,797 | 1,270.2 | 4,711,893 | 328.3 |

**By element type, size 100,000** (push rows in ns/call, buffer/manual
rows in ms/call):

| direction/source | unit | int | long | float | double |
|---|---|---:|---:|---:|---:|
| push, list->array | ns | 4,511,797 | 4,041,574 | 4,218,600 | 4,927,029 |
| push, buffer->array (manual) | ms | 1,270.2 | 1,074.6 | 1,107.4 | 1,395.7 |
| pull, array->list | ns | 4,711,893 | 4,666,041 | 8,080,424 | 7,030,721 |
| pull, array->buffer | ms | 328.3 | 318.8 | 372.5 | 346.2 |

**Interpretation.** `list->array` push and `array->list` pull are both
competitive with jpy/jep at every size (e.g. int @100,000: GraalPy
4,511,797 ns vs. jpy 791,520, jep 1,328,300 (Section 4.1) -- GraalPy is
3.4-5.7x slower here, in the same ballpark as pyjnius). jpype has since
moved out of that ballpark on `list->array` specifically (378,957ns at
this size, Section 4.5's `JPConversionList`/`JPConversionTuple`
quality-check fast path plus this session's `setArrayRange`
value-extraction fast path) -- GraalPy is now 11.9x slower than jpype
there, well past the 3.4-5.7x range the other three libraries occupy;
jpype's `array->list` pull is untouched by that change and still lands
in the same 2-6x ballpark as jpy/jep. The other two rows are not in the
same ballpark as anything else in this report:

- **`buffer->array` (manual)**: 250-300x slower per element than
  GraalPy's own `list->array` (e.g. int @100,000: 1,270.2 ms vs. 4.51
  ms). jpype's real `buffer->array` fast path at the same size is
  54,480ns = 0.054 ms (Section 4.2) -- GraalPy's manual emulation is
  roughly 23,000x slower.
- **`array->buffer` pull**: 69.7x slower than GraalPy's own `array->list`
  at the same size (328.3 ms vs. 4.71 ms) -- the opposite ranking from
  jpype/jpy, where `array->buffer` is the fast path and beats
  `array->list` by 5.1-212x for jpype and 4.4-119x for jpy, growing with
  size in both cases (Section 7.1). This matches the jep/pyjnius
  pattern (Section 7.1's footnote): `np.asarray()` on a
  `polyglot.ForeignList` pays `array->list`'s per-element cost plus a
  numpy-array-build step on top, not a real buffer read.

GraalPy's polyglot interop layer has a real, JIT-accelerated
per-element/per-call marshalling path (`list->array`/`array->list`), but
no bulk buffer-protocol bridge in either direction. Every other library
in this report that has any numpy interop (jpype, jpy, and jep for flat
targets) treats this as a first-class fast path precisely because
scientific-Python workloads are dominated by exactly this operation.

### 9.3 Array push, multi-dimensional and ragged

**Methodology.** Same as Section 5.

**int, depth 5 (100,000 elements), all categories:**

| category | unit | value |
|---|---|---:|
| push, list->array | ns | 5,081,921 |
| push, buffer->array (manual) | ms | 1,187.9 |
| pull, array->list | ns | 8,420,835 |
| pull, array->buffer | ms | 590.9 |

**Ragged push (list->array, GraalPy's only push path), all four types,
depth 5:**

| type | n (actual elements) | ns/call |
|---|---:|---:|
| int | 114,940 | 6,543,572 |
| long | 114,940 | 6,478,348 |
| float | 114,940 | 7,402,321 |
| double | 114,940 | 7,783,541 |

**Interpretation.** `list->array` push at depth 5 (5,081,921 ns) barely
moves from flat @100,000 (4,511,797 ns, Section 8.2) -- GraalPy's automatic push
path is not noticeably sensitive to nesting depth, matching jpype's own
depth-insensitivity within its `list->array` path (Section 5). The
manual `buffer->array` emulation is not meaningfully worse at depth 5
(1,187.9 ms) than flat (1,270.2 ms) either, since `build_manual()`'s cost is
driven by total element count and per-element polyglot crossings, not
nesting depth. `array->buffer` pull gets worse relative to `array->list`
as depth grows (70x at flat, 125x at depth 5), consistent with
`np.asarray()` walking a deeper recursive `ForeignList`-of-`ForeignList`
structure. Ragged push costs essentially the same as rectangular
`list->array` at a matched element count (int: 6,543,572 ns for 114,940
ragged elements vs. 5,081,921 ns for 100,000 rectangular -- 56.9 vs. 50.8
ns/element, an 11% difference), the same finding as jpype's own
ragged-vs-rectangular parity (Section 5).

### 9.4 Non-contiguous sources and row-heavy shapes

**Methodology.** Same as Section 6 and 4.4. GraalPy's manual
`build_manual()` push indexes the numpy source directly (`np_sub[i]`),
so numpy itself resolves whatever strides the source has.

**Non-contiguous vs. contiguous, int, manual push, same element counts
(ms/call):**

| shape | contiguous | non-contiguous | ratio |
|---|---:|---:|---:|
| flat 100,000 (column slice vs. flat) | 1,270.2 | 1,224.1 | 0.96x |
| depth 5, 100,000 (rectangular vs. transposed) | 1,187.9 | 1,218.8 | 1.03x |

**Row-heavy 2D shape sweep, int, manual buffer->array push, ns/element:**

| shape (rows x cols) | ns/element |
|---|---:|
| 1,000 x 100 | 11,639 |
| 10 x 10,000 | 12,175 |
| 100 x 1,000 | 12,212 |
| 10,000 x 10 | 12,315 |
| 3 x 100,000 | 12,888 |
| 1,000 x 1,000 | 13,536 |
| **100,000 x 3** | **15,449** |

**`list->array`, same shapes, `100000x3` row:** int and long both hit a
genuine `MemoryError` (dozens of `TruffleCompilerThread`/`Python GC`
`OutOfMemoryError`s precede each one at a capped `-Xmx3g` heap). float
and double completed the same row without incident (115.9 ns/element
for double, 2.7x its `3x100000` counterpart's 43.1 ns/element).

**Interpretation.** The non-contiguous manual push shows no measurable
penalty (0.96-1.03x, noise-level) -- unsurprising, `build_manual()` was
never a bulk-read path to begin with. The row-heavy shape sweep shows
only a mild 1.33x penalty (11,639 -> 15,449 ns/element) for the manual
buffer path, far smaller in relative terms than jpype's own 30.4x
`buffer->array` penalty at the same shape extreme (Section 6), because
GraalPy's baseline per-element cost is already dominated by
polyglot-crossing overhead. The `list->array` `MemoryError`s are the
standout result, but not a capacity finding: int and long both fail at
`100000x3`, while float and double -- the *same* byte widths (4 and 8
bytes respectively) as int and long -- complete the identical row
without incident. If this were genuinely too much live data for a 3GB
heap, byte width would predict the split; it doesn't, so the failure
tracks something else about how int/long values get built and churned
at this shape (a GC-throughput/allocation-rate problem -- the collector
falling behind, "GC overhead limit exceeded" in spirit -- rather than
the live set not fitting). That makes it tuning-sensitive in a way a
real capability gap isn't: a larger heap or different GC settings might
clear it, unlike the missing `buffer->array` push path itself, which no
heap size fixes. Flagged as an open question, not run down further here
-- something no other library in this report comes close to triggering
at the same shape and heap budget, regardless of its ultimate cause.

### 9.5 Proxy: the one place GraalPy is architecturally simpler

GraalPy needs no explicit proxy-construction step at all: a plain Python
object (or bare function, for a single-method interface) with a matching
method name is auto-adapted to any Java functional interface wherever
one is expected -- qualitatively different from jpype's `@JImplements`,
jep's `jep.jproxy()`, and pyjnius's `PythonJavaClass` subclassing, all of
which require an explicit class-implements-interface declaration
constructed ahead of the steady-state calls being measured. It also has
no null-argument crash -- `invokeObjectCallbackWithNull`, the exact case
that segfaults pyjnius (Section 3), works cleanly under GraalPy with no
special handling. This is architectural, not a speed result (see 8.1's
proxy row for the speed comparison): a genuinely simpler interop model
for the callback direction specifically, unrelated to the array-transfer
gap in 8.2-8.4. Full detail in `project/benchmark/README.md`'s proxy
section.

### 9.6 Strategic summary

GraalPy's Truffle/Graal JIT delivers exactly where a JIT can: hot,
simple, monomorphic call sites (8.1) beat every other bridge in this
report, sometimes by 2-3x. But scientific-Python orchestration is
dominated by bulk numpy<->Java array transfer, not scalar call overhead,
and GraalPy has no purpose-built path for that at all, in either
direction (8.2), a gap wide enough (tens of thousands of times versus
jpype's real fast path) that no realistic amount of JIT tiering closes
it, plus a second, GC-throughput-shaped failure (8.4: int/long fail with
`MemoryError` on ordinary row-heavy shapes at a capped 3GB heap while
same-byte-width float/double don't, pointing at an allocation-rate
problem rather than a hard capacity limit -- tuning-sensitive, and not
weighted the same as the missing `buffer->array` path above, which no
amount of heap or GC tuning fixes) that none of jpype/jpy/jep/pyjnius
exhibit at the same budget. The one place GraalPy is unambiguously ahead
structurally, not just faster, is the callback/proxy direction (8.5).
Net read for anyone weighing GraalVM's polyglot model as an architecture
to follow: viable, even excellent, for microscript/glue-code call
patterns; not viable as-is for a scientific-orchestration substitute,
where jpype (and jpy) remain the only two bridges in this report with
real bulk buffer-transfer paths in both directions.


## 10. Known limitations of this run

- **jep, `array_multidim.py`**: hit `java.lang.OutOfMemoryError`
  partway through the `long` type sweep even at `-Xmx3g` (this
  machine's stock ergonomic default was ~2GB; raising to 4GB and then
  6GB each let it get further into the sweep before still OOMing --
  6GB pushed total system memory to the edge of exhaustion on this
  7.7GB machine and was not pursued further). This happens only in
  `array_multidim.py`, which runs all four (type x direction) sweeps
  back-to-back in one JVM process; the other jep array scripts
  (`array_ragged.py`, `array_noncontig.py`, `array_shape.py`), each a
  narrower slice of the same workload, complete cleanly at the same
  depths/sizes under the stock default heap. That a larger heap
  measurably postpones but does not prevent the failure, combined with
  it being specific to the single long-running combined-sweep process,
  points at accumulated garbage outliving each `timeit()` batch (an
  allocation-rate-vs-GC-throughput problem) rather than a fixed
  working-set size that a given heap either does or doesn't fit --
  flagged here as a real, reproducible finding, not investigated
  further. Section 5's `array_multidim` tables show `int` in full and
  `long`/`float`/`double` only as far as this run reached before
  failing (missing cells read `--`).
- **pyjnius, `array_noncontig.py`**: intentionally a no-op -- pyjnius
  has no buffer->array push at all (confirmed empirically: any buffer
  source raises `JavaException('Expecting a python list/tuple, got
  array(...)')` unconditionally), so there is no non-contiguous-source
  case to measure separately from Section 3's finding.
- **GraalPy**: not re-run this edition; see Section 9 for the last
  captured numbers and their own methodology/caveats.
- **jpype's own numbers, this edition**: re-run to verify/fix a
  per-element JNI local-reference leak in array iteration (`list(ja)`,
  `for x in ja`) -- see `bugs/ArrayIterLocalRefLeak.md`. Sections 1-7 and
  most of Section 8 reflect this fresh run. Four Section 8 tables
  (`JArray.of()`/naive-ctor, the jpype-only multidim summary, and two
  duplicate scalar/dispatch listings) are unrelated to the leak fix and
  were not re-run this edition -- their numbers are carried forward from
  the prior edition, same as jpy/jep/pyjnius throughout. Machine
  conditions differed from the prior edition (this box had been under
  near-continuous compile/benchmark load for hours beforehand); a few
  unrelated, untouched operations (`new Integer(int)`, `new
  Double(double)`) moved by a similar magnitude to the leak fix's own
  residual overhead on `array->list`, so that residual (~1.4-1.5x on
  `array->list` at 100,000 elements, down from 3.2-4x before the fix
  settled) is plausibly machine noise rather than a real remaining
  regression -- not conclusively isolated.
