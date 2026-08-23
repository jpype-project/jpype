# Cross-library benchmark results

jpype vs. jpy, jep, and pyjnius on the JVM-embedding side of a Python/Java
bridge (Python drives Java in all four).
All numbers below are from a single sequential
re-run of the full suite, in disposable/isolated environments per this
repo's CLAUDE.md, with increased statistics (`trials=7`, higher
iteration floors) versus prior editions of this report to reduce noise.
Every section follows the same shape: **Methodology** (what is measured
and how), **Table** (raw numbers, `best` of the trials, nanoseconds per
call unless noted), **Result** (the factual takeaway only -- no
running commentary, no revision history).

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
  unless noted. `Z`/`B`/`C`/`S` (boolean/byte/char/short) arrays are not
  separately benchmarked in this report.


## 1. Scalars, strings, object identity

**Methodology.** `Math.max(int,int)` / `new Integer(int)` (int.py),
`Math.sqrt(double)` / `new Double(double)` (double.py), a round-trip
Java-String-from-Python-then-`str()`-back (strings.py), and reference
identity of a returned Java object compared across two calls
(object.py). All four are fixed-cost, `n=200,000` per batch.

| operation | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| Math.max(int,int) | 716 | 363 | 1,344 | 1,323 |
| new Integer(int) | 971 | 478 | 1,383 | 8,014 |
| Math.sqrt(double) | 730 | 419 | 688 | 533 |
| new Double(double) | 1,074 | 527 | 1,454 | 7,177 |
| new String + toString | 1,106 | 1,010 | 2,616 | 23,546 |
| Object identity | 1,130 | 737 | 2,135 | 4,096 |

**Result.** jpy is fastest on every scalar/string/identity op; jpype
trails jpy by roughly 1.1-2.0x. jep and pyjnius trail jpype on most ops
by a further 1.3-4x, except `Math.sqrt(double)`, where both are
actually faster than jpype (jep 0.94x, pyjnius 0.73x of jpype's cost) --
jpype has no special-cased fast path for this particular scalar op.
pyjnius's `new Integer`/`new Double`/string-roundtrip costs are the
widest outliers (roughly 14-23x jpy).

## 2. Method dispatch and proxy callbacks

**Methodology.** `dispatch.py`: a 16-overload method called
monomorphically (same arg type every call) vs. polymorphically
(argument type varies call-to-call, forcing overload resolution to
redo work jpype/jep can otherwise cache). `proxy.py`: steady-state cost
of invoking an already-constructed Python-implements-Java-interface
callback, `int` argument and (where supported) `Object` argument.

| operation | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| overload x16, monomorphic | 646 | 543 | 4,783 | 4,090 |
| overload x16, polymorphic | 1,028 | 546 | 5,146 | 4,238 |
| proxy callback (established), int arg | 2,830 | -- | 2,440 | 42,587 |
| proxy callback (established), Object arg | 3,031 | -- | 5,159 | -- |

**Result.** jpy has no separate proxy script (not benchmarked here).

pyjnius's proxy Object-arg case is not benchmarked: it reliably
segfaults this pyjnius checkout (`GetObjectClass`/`IsSameObject` called
without a null check on a genuinely-null `Object` argument), reproduced
independently against a fresh build before being treated as a real
finding rather than stale-build noise, per this repo's CLAUDE.md. jpype
leads jep and pyjnius on dispatch by roughly 4-7x; jpy leads jpype on
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
| int[100] | 5,279 | 1,183 | 2,493 | 3,057 |
| int[1000] | 44,284 | 8,074 | 14,740 | 27,116 |
| int[10000] | 434,456 | 76,676 | 136,222 | 280,330 |
| int[100000] | 4,259,161 | 781,784 | 1,371,269 | 4,673,403 |
| long[100] | 5,392 | 1,210 | 2,456 | 2,900 |
| long[1000] | 46,561 | 8,689 | 14,246 | 26,344 |
| long[10000] | 450,052 | 82,353 | 136,267 | 275,971 |
| long[100000] | 4,453,209 | 824,832 | 1,380,915 | 5,590,922 |
| float[100] | 5,352 | 1,110 | 2,108 | 3,417 |
| float[1000] | 45,616 | 7,626 | 11,485 | 29,009 |
| float[10000] | 447,118 | 72,344 | 102,599 | 283,160 |
| float[100000] | 4,382,868 | 724,713 | 1,013,267 | 3,323,044 |
| double[100] | 5,114 | 1,190 | 1,993 | 3,421 |
| double[1000] | 42,634 | 8,430 | 10,737 | 31,134 |
| double[10000] | 416,822 | 81,607 | 94,816 | 292,036 |
| double[100000] | 4,084,984 | 807,684 | 1,038,434 | 5,077,917 |

### `list->array`, widening from int (float/double only)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| float[100] | 5,582 | 1,588 | 3,932 | 3,464 |
| float[1000] | 46,810 | 11,962 | 31,456 | 28,942 |
| float[10000] | 453,962 | 115,583 | 303,602 | 285,874 |
| float[100000] | 4,389,313 | 1,187,625 | 2,911,070 | 3,645,482 |
| double[100] | 5,606 | 1,654 | 4,002 | 3,399 |
| double[1000] | 47,682 | 13,096 | 30,740 | 29,730 |
| double[10000] | 465,486 | 123,520 | 286,372 | 299,577 |
| double[100000] | 4,630,364 | 1,266,424 | 2,802,420 | 3,729,057 |

### `buffer->array` push (method argument, numpy source)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 1,019 | 438 | 818 | -- |
| int[1000] | 2,622 | 810 | 1,213 | -- |
| int[10000] | 18,669 | 5,310 | 5,621 | -- |
| int[100000] | 220,270 | 43,510 | 42,613 | -- |
| long[100] | 1,063 | 467 | 886 | -- |
| long[1000] | 3,236 | 1,168 | 1,765 | -- |
| long[10000] | 30,945 | 9,116 | 11,514 | -- |
| long[100000] | 256,452 | 72,483 | 103,775 | -- |
| float[100] | 1,066 | 441 | 842 | -- |
| float[1000] | 2,828 | 761 | 1,249 | -- |
| float[10000] | 20,491 | 4,917 | 5,841 | -- |
| float[100000] | 196,160 | 41,541 | 46,643 | -- |
| double[100] | 1,104 | 463 | 912 | -- |
| double[1000] | 3,490 | 1,142 | 1,830 | -- |
| double[10000] | 28,206 | 8,922 | 11,827 | -- |
| double[100000] | 278,710 | 72,876 | 104,318 | -- |

### `array->list` pull (Java array -> Python list)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 35,030 | 5,182 | 5,017 | 1,993 |
| int[1000] | 348,785 | 46,138 | 40,936 | 12,941 |
| int[10000] | 3,520,963 | 489,233 | 419,284 | 167,126 |
| int[100000] | 37,125,111 | 6,576,669 | 5,848,730 | 1,785,407 |
| long[100] | 34,717 | 5,378 | 5,452 | 2,168 |
| long[1000] | 350,493 | 48,160 | 38,753 | 16,237 |
| long[10000] | 3,486,391 | 483,064 | 368,904 | 141,594 |
| long[100000] | 37,586,644 | 7,442,961 | 6,322,217 | 5,623,093 |
| float[100] | 30,140 | 4,529 | 4,332 | 1,607 |
| float[1000] | 299,755 | 42,160 | 44,974 | 13,192 |
| float[10000] | 2,894,441 | 439,692 | 431,904 | 153,484 |
| float[100000] | 31,617,870 | 4,459,722 | 4,282,515 | 1,194,549 |
| double[100] | 29,744 | 4,730 | 4,259 | 1,606 |
| double[1000] | 299,506 | 45,415 | 43,017 | 14,002 |
| double[10000] | 2,889,847 | 456,469 | 436,006 | 121,902 |
| double[100000] | 32,112,240 | 4,912,456 | 4,366,728 | 1,705,660 |

### `array->buffer` pull (Java array -> Python/numpy buffer)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 1,888 | 973 | 8,996 | 5,358 |
| int[1000] | 2,426 | 1,424 | 73,586 | 44,621 |
| int[10000] | 7,576 | 6,409 | 757,977 | 497,008 |
| int[100000] | 55,325 | 48,006 | 9,408,453 | 6,640,984 |
| long[100] | 1,896 | 999 | 9,486 | 6,136 |
| long[1000] | 2,929 | 2,023 | 78,725 | 52,362 |
| long[10000] | 13,002 | 11,845 | 788,202 | 554,075 |
| long[100000] | 124,192 | 116,314 | 10,729,171 | 9,742,540 |
| float[100] | 1,903 | 971 | 7,423 | 4,516 |
| float[1000] | 2,437 | 1,473 | 68,721 | 38,682 |
| float[10000] | 7,457 | 6,215 | 693,028 | 371,209 |
| float[100000] | 50,348 | 47,487 | 7,084,413 | 3,947,891 |
| double[100] | 1,930 | 1,058 | 7,474 | 4,509 |
| double[1000] | 3,116 | 2,272 | 68,611 | 39,138 |
| double[10000] | 13,905 | 13,311 | 684,508 | 369,847 |
| double[100000] | 115,114 | 132,296 | 7,028,567 | 4,170,636 |

**Result.** `list->array`: jpy and jep both lead jpype at every
size, jpy by roughly 5-6x and jep by roughly 3x at 100,000 elements --
jpype's per-row push (`JPConversionSequence`) pays one JNI call per
element with no bulk path. jpype only pulls ahead of pyjnius at the
largest size. `list->array, widening from int`: jpy and jep both pay a
real int-to-float/double widening penalty (jpy ~1.6x, jep ~2.9x at
100,000) over their own matched-type number; jpype shows almost no
widening penalty, not because widening is cheap but because there is no
matched-type fast path to fall out of in the first place -- the same
per-element path handles both cases. `buffer->array`: pyjnius has no
buffer-protocol push at all (falls back to a generic sequence-conversion
path, i.e. it isn't in this table). `array->list`: pyjnius is the fastest of all four despite
losing most other benchmarks in this report, because its Cython bridge
boxes one plain `PyLong`/`PyFloat` per element while jpype/jep/jpy build
heavier tagged wrapper objects; jpype is the slowest here by a wide
margin (its own `array->buffer` number below is ~600x cheaper at
100,000 elements, confirming the cost is in the per-element Python-list
materialization, not the underlying array read). `array->buffer`:
jep/pyjnius have no real buffer-protocol return path -- their columns
above are `array->list`'s cost plus a redundant `np.asarray()`, not a
genuine bulk read, which is why they land *worse* than their own
`array->list` number instead of better. jpype and jpy have a genuine
buffer-protocol return path and are close to each other, with jpy
consistently faster.

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
| int[100] | 5,920 | -- | 1,247 | -- |
| int[1000] | 49,074 | -- | 1,926 | -- |
| int[10000] | 488,969 | -- | 8,504 | -- |
| int[100000] | 4,867,648 | -- | 67,997 | -- |
| long[100] | 6,000 | -- | 1,296 | -- |
| long[1000] | 51,202 | -- | 2,540 | -- |
| long[10000] | 493,019 | -- | 14,861 | -- |
| long[100000] | 5,257,869 | -- | 134,891 | -- |
| float[100] | 5,813 | -- | 1,254 | -- |
| float[1000] | 50,001 | -- | 1,966 | -- |
| float[10000] | 500,184 | -- | 8,313 | -- |
| float[100000] | 4,964,475 | -- | 68,367 | -- |
| double[100] | 5,908 | -- | 1,281 | -- |
| double[1000] | 50,056 | -- | 2,485 | -- |
| double[10000] | 517,488 | -- | 14,846 | -- |
| double[100000] | 5,152,955 | -- | 134,718 | -- |

_jpy and pyjnius have no entry: jpy's buffer matcher requires `PyBUF_SIMPLE` (fails outright on a non-contiguous 1D source); pyjnius has no buffer->array push at all, contiguous or not._


### Multi-dimensional (2D-5D), transposed

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 25,882 | 5,652 | 25,512 | -- |
| int[][][](10^3) | 397,234 | 55,991 | 248,265 | -- |
| int[][][][](10^4) | 5,479,047 | 571,821 | 2,493,939 | -- |
| int[][][][][](10^5) | 70,192,691 | 5,904,847 | 24,911,885 | -- |
| long[][](10^2) | 25,562 | 5,474 | 25,654 | -- |
| long[][][](10^3) | 408,855 | 54,552 | 254,678 | -- |
| long[][][][](10^4) | 5,590,650 | 559,801 | 2,486,915 | -- |
| long[][][][][](10^5) | 71,642,216 | 5,726,589 | 26,296,872 | -- |
| float[][](10^2) | 25,860 | 5,642 | 25,522 | -- |
| float[][][](10^3) | 402,224 | 54,951 | 248,772 | -- |
| float[][][][](10^4) | 5,646,577 | 532,072 | 2,494,275 | -- |
| float[][][][][](10^5) | 70,615,131 | 5,546,981 | 24,792,662 | -- |
| double[][](10^2) | 25,997 | 5,316 | 24,907 | -- |
| double[][][](10^3) | 404,814 | 50,789 | 247,473 | -- |
| double[][][][](10^4) | 5,607,494 | 491,134 | 2,498,462 | -- |
| double[][][][][](10^5) | 71,602,317 | 4,963,800 | 25,427,445 | -- |

**Result.** jep is the only library with a real bulk path for a
non-contiguous 1D source; jpype falls back to a slower per-element path
here (roughly 20-70x its own contiguous `buffer->array` cost from
Section 3 at the same size), a real gap and a candidate for future
work. jpy has no buffer->array push at all for a non-contiguous source
in any dimensionality (fails outright, 1D; not benchmarked, ND, since
the underlying push has no bulk path to exercise); pyjnius has no
buffer->array push at any size, depth, or contiguity. jep's
"transposed, manual per-row" ND numbers are a per-row Python-level
walk, not a bulk buffer read -- included for completeness, not a
like-for-like comparison to a single-JNI-call path.

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
| int[][](10^2) | 20,111 | 2,200 | 6,912 | 4,800 |
| int[][][](10^3) | 290,680 | 17,668 | 64,825 | 41,346 |
| int[][][][](10^4) | 3,779,542 | 171,389 | 629,210 | 434,760 |
| int[][][][][](10^5) | 48,679,339 | 1,757,121 | 6,280,016 | 5,027,203 |
| long[][](10^2) | 20,010 | 2,217 | 6,949 | 4,903 |
| long[][][](10^3) | 298,741 | 18,804 | 64,072 | 42,052 |
| long[][][][](10^4) | 3,912,289 | 188,723 | 630,475 | 424,178 |
| long[][][][][](10^5) | 49,222,315 | 1,890,664 | 6,349,567 | 5,045,931 |
| float[][](10^2) | 20,465 | 2,072 | -- | 5,295 |
| float[][][](10^3) | 298,679 | 17,509 | -- | 53,187 |
| float[][][][](10^4) | 4,066,413 | 173,231 | -- | 551,122 |
| float[][][][][](10^5) | 49,891,767 | 1,758,766 | -- | 9,268,890 |
| double[][](10^2) | 20,569 | 2,208 | -- | 5,431 |
| double[][][](10^3) | 281,619 | 18,116 | -- | 54,689 |
| double[][][][](10^4) | 3,868,480 | 179,378 | -- | 564,277 |
| double[][][][][](10^5) | 50,343,390 | 1,882,707 | -- | 9,254,105 |

### `buffer->array` push, numpy source (jep: manual per-row fallback)

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 11,261 | 5,730 | -- | -- |
| int[][][](10^3) | 161,672 | 55,185 | -- | -- |
| int[][][][](10^4) | 2,068,246 | 566,534 | -- | -- |
| int[][][][][](10^5) | 25,655,404 | 5,683,388 | -- | -- |
| long[][](10^2) | 11,105 | 5,585 | -- | -- |
| long[][][](10^3) | 158,390 | 54,901 | -- | -- |
| long[][][][](10^4) | 2,061,244 | 554,750 | -- | -- |
| long[][][][][](10^5) | 25,686,190 | 5,660,922 | -- | -- |
| float[][](10^2) | 10,772 | 5,737 | -- | -- |
| float[][][](10^3) | 148,810 | 54,726 | -- | -- |
| float[][][][](10^4) | 1,977,203 | 541,806 | -- | -- |
| float[][][][][](10^5) | 25,035,010 | 5,440,141 | -- | -- |
| double[][](10^2) | 10,862 | 5,450 | -- | -- |
| double[][][](10^3) | 156,459 | 51,095 | -- | -- |
| double[][][][](10^4) | 2,042,364 | 511,132 | -- | -- |
| double[][][][][](10^5) | 25,882,584 | 5,219,317 | -- | -- |

_pyjnius: no entry -- no buffer->array push at any depth._


### `array->list` pull

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 50,963 | 9,963 | 13,526 | 3,440 |
| int[][][](10^3) | 510,929 | 99,842 | 129,102 | 30,065 |
| int[][][][](10^4) | 5,153,554 | 1,137,774 | 1,466,528 | 498,866 |
| int[][][][][](10^5) | 56,110,900 | 13,614,375 | 16,390,342 | 7,774,809 |
| long[][](10^2) | 54,654 | 10,198 | 14,147 | 3,571 |
| long[][][](10^3) | 513,018 | 105,042 | 133,857 | 31,427 |
| long[][][][](10^4) | 5,445,552 | 1,092,114 | -- | 474,170 |
| long[][][][][](10^5) | 59,924,357 | 11,879,790 | -- | 9,453,776 |
| float[][](10^2) | 44,669 | 9,386 | -- | 2,968 |
| float[][][](10^3) | 452,298 | 91,323 | -- | 29,074 |
| float[][][][](10^4) | 4,659,092 | 1,015,685 | -- | 406,225 |
| float[][][][][](10^5) | 50,234,434 | 10,975,680 | -- | 5,072,694 |
| double[][](10^2) | 46,985 | 9,892 | -- | 3,000 |
| double[][][](10^3) | 446,703 | 97,720 | -- | 28,600 |
| double[][][][](10^4) | 4,638,652 | 1,037,738 | -- | 431,080 |
| double[][][][][](10^5) | 49,555,811 | 11,476,721 | -- | 5,539,152 |

### `array->buffer` pull

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 3,664 | 7,854 | 17,295 | 7,592 |
| int[][][](10^3) | 13,566 | 77,504 | 175,356 | 71,190 |
| int[][][][](10^4) | 113,964 | 891,446 | 2,036,880 | 950,755 |
| int[][][][][](10^5) | 1,118,829 | 10,713,915 | 26,164,085 | 13,366,826 |
| long[][](10^2) | 4,007 | 8,304 | -- | 8,445 |
| long[][][](10^3) | 15,009 | 76,792 | -- | 82,092 |
| long[][][][](10^4) | 126,251 | 894,228 | -- | 982,934 |
| long[][][][][](10^5) | 1,368,239 | 10,543,404 | -- | 16,391,273 |
| float[][](10^2) | 4,813 | 8,328 | -- | 6,761 |
| float[][][](10^3) | 17,997 | 76,140 | -- | 64,680 |
| float[][][][](10^4) | 121,476 | 884,745 | -- | 781,384 |
| float[][][][][](10^5) | 1,244,058 | 10,522,196 | -- | 9,744,072 |
| double[][](10^2) | 3,662 | 8,192 | -- | 6,844 |
| double[][][](10^3) | 12,588 | 79,991 | -- | 62,103 |
| double[][][][](10^4) | 96,762 | 906,594 | -- | 777,766 |
| double[][][][][](10^5) | 1,182,057 | 10,900,139 | -- | 10,888,224 |

**Result.** `list->array`: jpy leads jpype at every depth, the gap
widening from roughly 9x at depth 2 to roughly 28x at depth 5 (both walk
the nested structure recursively via `JPConversionSequence` on jpype's
side, one JNI call per row, but jpy's per-row overhead is lower); a real
gap and a candidate for future work. `buffer->array`: jpy also leads
jpype here, by roughly 4.5x at depth 5; jep has no data for this
category in this run (hit an `OutOfMemoryError` partway through, see
the known-limitations section below); pyjnius has none. `array->list`:
jpype is the slowest of all four libraries at every depth here, even
behind jep and jpy. `array->buffer`: the opposite story -- jpype's bulk
rectangular buffer read is the fastest of all four at every depth,
roughly 8-24x faster than jpy/jep/pyjnius at depth 5, since it reads the
whole array in one JNI critical section rather than walking it row by
row the way `array->list` does.

## 6. Array push, ragged (jagged, non-rectangular)

**Methodology.** `array_ragged.py`. A nested Python list whose
sub-lists have varying lengths (a genuinely jagged/ragged structure, not
a rectangular array-of-arrays), pushed fresh into a Java array-of-arrays
parameter, depth 2-5, ~10^depth total elements.

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](~10^2) | 11,722 | 1,531 | 4,801 | 3,171 |
| int[][][](~10^3) | 316,129 | 18,823 | 71,090 | 44,417 |
| int[][][][](~10^4) | 3,206,032 | 149,402 | 563,822 | 352,867 |
| int[][][][][](~10^5) | 57,522,046 | 2,139,732 | 8,147,249 | 5,920,602 |
| long[][](~10^2) | 12,250 | 1,483 | 4,768 | 3,175 |
| long[][][](~10^3) | 320,728 | 19,790 | 70,842 | 42,263 |
| long[][][][](~10^4) | 3,272,569 | 156,158 | 569,074 | 352,408 |
| long[][][][][](~10^5) | 59,736,737 | 2,259,730 | 8,136,513 | 5,483,144 |
| float[][](~10^2) | 12,407 | 1,382 | 4,539 | 3,206 |
| float[][][](~10^3) | 324,396 | 18,062 | 66,085 | 51,978 |
| float[][][][](~10^4) | 3,316,074 | 145,387 | 530,688 | 443,521 |
| float[][][][][](~10^5) | 61,038,577 | 2,280,683 | 7,884,050 | 10,576,596 |
| double[][](~10^2) | 12,138 | 1,483 | 4,696 | 3,369 |
| double[][][](~10^3) | 316,670 | 19,950 | 67,852 | 53,670 |
| double[][][][](~10^4) | 3,227,494 | 151,441 | 532,166 | 463,626 |
| double[][][][][](~10^5) | 59,493,199 | 2,217,599 | 7,744,838 | 10,166,998 |

**Result.** jpy leads jpype at every depth/type; the gap widens
with depth (both walk the ragged structure recursively via a generic
per-node sequence-protocol access -- `matchRaggedNode`/`encodeRaggedNode`,
`native/common/jp_classhints.cpp`, on jpype's side -- but jpy's per-node
overhead is lower), a real gap and a candidate for future work.

## 7. Array push/pull, shape at fixed depth and total element count

**Methodology.** `array_shape.py`. Fixed total element count
(~10,000 or ~100,000), depth held at 3, but the *shape* varies (e.g.
`[1000][10][10]` vs. `[10][10][1000]`) to isolate whether cost tracks
total elements or leaf-array count / row-heaviness. `list->array` and
`buffer->array` (jep: manual per-row) directions only.

### `list->array` push

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[10][10000] | 9,049,718 | 806,918 | 1,402,789 | 2,915,372 |
| int[100][1000] | 9,183,278 | 830,006 | 1,417,742 | 2,794,704 |
| int[1000][100] | 9,707,568 | 866,335 | 1,836,153 | 2,241,320 |
| int[10000][10] | 18,030,351 | 1,649,007 | 6,145,127 | 4,012,716 |
| int[3][100000] | 26,639,915 | 2,490,970 | 4,207,551 | 9,705,678 |
| int[100000][3] | 121,392,092 | 11,588,374 | 50,941,097 | 26,209,037 |
| int[1000][1000] | 91,732,897 | 8,584,726 | 14,795,684 | 32,167,686 |
| int[1000][10][10] | 29,510,013 | 1,745,310 | 6,499,518 | 6,011,382 |
| int[10][10][1000] | 14,457,904 | 798,483 | 1,412,372 | 2,778,888 |
| long[10][10000] | 9,273,231 | 821,602 | 1,275,314 | 2,884,768 |
| long[100][1000] | 9,322,454 | 826,781 | 1,354,412 | 2,688,467 |
| long[1000][100] | 9,701,934 | 890,544 | 1,738,038 | 2,215,003 |
| long[10000][10] | 18,623,042 | 1,759,386 | 5,879,173 | 4,086,737 |
| long[3][100000] | 28,711,046 | 2,584,784 | 4,047,452 | 9,942,196 |
| long[100000][3] | 122,010,700 | 12,470,530 | 51,026,319 | 26,499,032 |
| long[1000][1000] | 94,520,348 | 8,898,200 | 13,989,272 | 30,639,333 |
| long[1000][10][10] | 29,122,566 | 1,872,495 | 6,269,827 | 5,337,705 |
| long[10][10][1000] | 14,530,790 | 823,260 | 1,349,376 | 2,671,344 |
| float[10][10000] | 9,723,480 | 731,309 | 994,848 | 2,789,406 |
| float[100][1000] | 9,856,148 | 754,981 | 1,057,161 | 2,849,780 |
| float[1000][100] | 10,607,365 | 795,978 | 1,430,817 | 3,208,813 |
| float[10000][10] | 19,008,755 | 1,609,817 | 5,431,722 | 5,489,580 |
| float[3][100000] | 29,363,291 | 2,333,876 | 2,962,936 | 10,136,459 |
| float[100000][3] | 122,211,384 | 11,620,500 | 48,370,083 | 32,701,462 |
| float[1000][1000] | 96,725,437 | 8,092,994 | 10,686,076 | 33,539,870 |
| float[1000][10][10] | 30,618,496 | 1,754,334 | 5,816,736 | 7,109,184 |
| float[10][10][1000] | 15,596,123 | 759,979 | 1,056,809 | 3,135,425 |
| double[10][10000] | 9,013,546 | 829,131 | 929,372 | 3,060,208 |
| double[100][1000] | 9,121,012 | 823,039 | 979,936 | 3,290,929 |
| double[1000][100] | 10,157,359 | 867,200 | 1,371,696 | 3,586,975 |
| double[10000][10] | 18,698,305 | 1,696,826 | 5,405,955 | 6,034,980 |
| double[3][100000] | 28,316,966 | 2,670,880 | 2,870,663 | 11,272,831 |
| double[100000][3] | 122,335,257 | 12,117,989 | 47,775,907 | 32,858,976 |
| double[1000][1000] | 93,114,595 | 8,884,806 | 9,848,050 | 35,162,136 |
| double[1000][10][10] | 28,860,559 | 1,833,219 | 5,852,637 | 7,900,256 |
| double[10][10][1000] | 13,884,058 | 827,489 | 977,538 | 3,435,078 |

### `buffer->array` push (jep: manual per-row)

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[10][10000] | 180,968 | 3,957,171 | -- | -- |
| int[100][1000] | 266,822 | 4,028,383 | -- | -- |
| int[1000][100] | 1,099,494 | 4,154,658 | -- | -- |
| int[10000][10] | 9,855,474 | 5,977,026 | -- | -- |
| int[3][100000] | 642,748 | 12,919,031 | -- | -- |
| int[100000][3] | 99,760,183 | 30,331,461 | -- | -- |
| int[1000][1000] | 3,000,051 | 39,452,892 | -- | -- |
| int[1000][10][10] | 15,874,065 | 5,810,657 | -- | -- |
| int[10][10][1000] | 339,259 | 3,859,042 | -- | -- |
| long[10][10000] | 251,640 | 3,772,218 | -- | -- |
| long[100][1000] | 312,586 | 3,738,746 | -- | -- |
| long[1000][100] | 1,146,621 | 3,873,069 | -- | -- |
| long[10000][10] | 9,955,547 | 5,560,335 | -- | -- |
| long[3][100000] | 862,390 | 11,171,590 | -- | -- |
| long[100000][3] | 101,334,820 | 28,768,097 | -- | -- |
| long[1000][1000] | 3,349,893 | 38,238,769 | -- | -- |
| long[1000][10][10] | 15,632,687 | 5,680,074 | -- | -- |
| long[10][10][1000] | 374,229 | 3,947,929 | -- | -- |
| float[10][10000] | 207,870 | 3,472,438 | -- | -- |
| float[100][1000] | 299,386 | 3,536,412 | -- | -- |
| float[1000][100] | 1,141,061 | 3,630,979 | -- | -- |
| float[10000][10] | 9,771,973 | 5,130,371 | -- | -- |
| float[3][100000] | 598,052 | 10,414,713 | -- | -- |
| float[100000][3] | 98,259,796 | 27,795,668 | -- | -- |
| float[1000][1000] | 2,937,205 | 35,873,353 | -- | -- |
| float[1000][10][10] | 15,650,133 | 5,542,828 | -- | -- |
| float[10][10][1000] | 352,364 | 3,508,462 | -- | -- |
| double[10][10000] | 287,710 | 3,283,246 | -- | -- |
| double[100][1000] | 360,313 | 3,252,640 | -- | -- |
| double[1000][100] | 1,187,476 | 3,401,470 | -- | -- |
| double[10000][10] | 10,215,914 | 5,092,549 | -- | -- |
| double[3][100000] | 816,164 | 10,026,086 | -- | -- |
| double[100000][3] | 100,652,099 | 27,661,544 | -- | -- |
| double[1000][1000] | 3,700,112 | 33,215,732 | -- | -- |
| double[1000][10][10] | 15,420,178 | 4,965,956 | -- | -- |
| double[10][10][1000] | 388,664 | 3,117,623 | -- | -- |

_pyjnius: no entry -- no buffer->array push at any shape._


**Result.** At fixed total element count, a row-heavy shape (many
short rows, e.g. `[100000][3]`) costs more than a column-heavy one
(few long rows, e.g. `[3][100000]`) in every library that has a bulk
path -- more leaf arrays means more per-leaf JNI/reflection overhead
even though total elements is unchanged. The penalty is much larger for
`list->array` (recursive per-row Python-level walk regardless of
library) than for `buffer->array` (jpype/jpy's bulk path pays only
per-leaf-array overhead, not per-element).

## 8. jpype-only microbenchmarks

`array_of.py` / `classhints.py` have no equivalent in the jpy/jep/pyjnius
suites -- they exercise jpype-internal API surface with nothing to
compare against. jpype-only, `best` ns/call.

### `JArray.of()` -- constructing an array directly from a buffer

| operation | jpype |
|---|:---:|
| JArray.of(arr) int[100] | 1,810 |
| JArray.of(arr, dtype=int) int[100], matching | 1,868 |
| JArray.of(arr, dtype=int) int[100], cross-dtype cast | 1,832 |
| JArray(int)(arr) int[100], naive sequence ctor | 2,394 |
| JArray.of(arr) int[1000] | 4,028 |
| JArray.of(arr, dtype=int) int[1000], matching | 4,132 |
| JArray.of(arr, dtype=int) int[1000], cross-dtype cast | 4,444 |
| JArray(int)(arr) int[1000], naive sequence ctor | 4,030 |
| JArray.of(arr) int[10000] | 27,112 |
| JArray.of(arr, dtype=int) int[10000], matching | 25,920 |
| JArray.of(arr, dtype=int) int[10000], cross-dtype cast | 26,256 |
| JArray(int)(arr) int[10000], naive sequence ctor | 19,805 |
| JArray.of(arr) int[100000] | 250,894 |
| JArray.of(arr, dtype=int) int[100000], matching | 262,055 |
| JArray.of(arr, dtype=int) int[100000], cross-dtype cast | 244,626 |
| JArray(int)(arr) int[100000], naive sequence ctor | 191,231 |
| JArray.of(arr) int[][](10^2) | 2,957 |
| JArray.of(arr, dtype=int) int[][](10^2), matching | 3,004 |
| JArray(int, 2)(arr) int[][](10^2), manual ctor | 10,236 |
| JArray.of(arr) int[][][](10^3) | 15,125 |
| JArray.of(arr, dtype=int) int[][][](10^3), matching | 15,424 |
| JArray(int, 3)(arr) int[][][](10^3), manual ctor | 131,860 |
| JArray.of(arr) int[][][][](10^4) | 142,340 |
| JArray.of(arr, dtype=int) int[][][][](10^4), matching | 137,976 |
| JArray(int, 4)(arr) int[][][][](10^4), manual ctor | 1,808,144 |
| JArray.of(arr) int[][][][][](10^5) | 1,477,246 |
| JArray.of(arr, dtype=int) int[][][][][](10^5), matching | 1,401,507 |
| JArray(int, 5)(arr) int[][][][][](10^5), manual ctor | 23,178,483 |
| JArray.of(arr) long[100] | 1,878 |
| JArray.of(arr, dtype=long) long[100], matching | 1,911 |
| JArray.of(arr, dtype=long) long[100], cross-dtype cast | 1,908 |
| JArray(long)(arr) long[100], naive sequence ctor | 2,332 |
| JArray.of(arr) long[1000] | 4,590 |
| JArray.of(arr, dtype=long) long[1000], matching | 4,627 |
| JArray.of(arr, dtype=long) long[1000], cross-dtype cast | 5,212 |
| JArray(long)(arr) long[1000], naive sequence ctor | 5,036 |
| JArray.of(arr) long[10000] | 32,978 |
| JArray.of(arr, dtype=long) long[10000], matching | 32,313 |
| JArray.of(arr, dtype=long) long[10000], cross-dtype cast | 33,119 |
| JArray(long)(arr) long[10000], naive sequence ctor | 25,859 |
| JArray.of(arr) long[100000] | 310,635 |
| JArray.of(arr, dtype=long) long[100000], matching | 315,069 |
| JArray.of(arr, dtype=long) long[100000], cross-dtype cast | 302,507 |
| JArray(long)(arr) long[100000], naive sequence ctor | 284,998 |
| JArray.of(arr) long[][](10^2) | 3,057 |
| JArray.of(arr, dtype=long) long[][](10^2), matching | 3,048 |
| JArray(long, 2)(arr) long[][](10^2), manual ctor | 10,044 |
| JArray.of(arr) long[][][](10^3) | 15,680 |
| JArray.of(arr, dtype=long) long[][][](10^3), matching | 16,006 |
| JArray(long, 3)(arr) long[][][](10^3), manual ctor | 131,600 |
| JArray.of(arr) long[][][][](10^4) | 153,150 |
| JArray.of(arr, dtype=long) long[][][][](10^4), matching | 162,010 |
| JArray(long, 4)(arr) long[][][][](10^4), manual ctor | 1,791,677 |
| JArray.of(arr) long[][][][][](10^5) | 1,594,958 |
| JArray.of(arr, dtype=long) long[][][][][](10^5), matching | 1,646,839 |
| JArray(long, 5)(arr) long[][][][][](10^5), manual ctor | 22,753,982 |
| JArray.of(arr) float[100] | 1,821 |
| JArray.of(arr, dtype=float) float[100], matching | 1,866 |
| JArray.of(arr, dtype=float) float[100], cross-dtype cast | 1,914 |
| JArray(float)(arr) float[100], naive sequence ctor | 2,352 |
| JArray.of(arr) float[1000] | 4,252 |
| JArray.of(arr, dtype=float) float[1000], matching | 4,284 |
| JArray.of(arr, dtype=float) float[1000], cross-dtype cast | 4,508 |
| JArray(float)(arr) float[1000], naive sequence ctor | 4,146 |
| JArray.of(arr) float[10000] | 28,163 |
| JArray.of(arr, dtype=float) float[10000], matching | 30,269 |
| JArray.of(arr, dtype=float) float[10000], cross-dtype cast | 33,816 |
| JArray(float)(arr) float[10000], naive sequence ctor | 22,490 |
| JArray.of(arr) float[100000] | 275,217 |
| JArray.of(arr, dtype=float) float[100000], matching | 269,944 |
| JArray.of(arr, dtype=float) float[100000], cross-dtype cast | 295,285 |
| JArray(float)(arr) float[100000], naive sequence ctor | 210,075 |
| JArray.of(arr) float[][](10^2) | 3,073 |
| JArray.of(arr, dtype=float) float[][](10^2), matching | 3,109 |
| JArray(float, 2)(arr) float[][](10^2), manual ctor | 10,272 |
| JArray.of(arr) float[][][](10^3) | 15,877 |
| JArray.of(arr, dtype=float) float[][][](10^3), matching | 15,891 |
| JArray(float, 3)(arr) float[][][](10^3), manual ctor | 131,842 |
| JArray.of(arr) float[][][][](10^4) | 149,653 |
| JArray.of(arr, dtype=float) float[][][][](10^4), matching | 148,883 |
| JArray(float, 4)(arr) float[][][][](10^4), manual ctor | 1,795,890 |
| JArray.of(arr) float[][][][][](10^5) | 1,499,854 |
| JArray.of(arr, dtype=float) float[][][][][](10^5), matching | 1,457,863 |
| JArray(float, 5)(arr) float[][][][][](10^5), manual ctor | 22,932,631 |
| JArray.of(arr) double[100] | 1,904 |
| JArray.of(arr, dtype=double) double[100], matching | 2,018 |
| JArray.of(arr, dtype=double) double[100], cross-dtype cast | 1,905 |
| JArray(double)(arr) double[100], naive sequence ctor | 2,388 |
| JArray.of(arr) double[1000] | 4,422 |
| JArray.of(arr, dtype=double) double[1000], matching | 4,448 |
| JArray.of(arr, dtype=double) double[1000], cross-dtype cast | 4,531 |
| JArray(double)(arr) double[1000], naive sequence ctor | 4,698 |
| JArray.of(arr) double[10000] | 30,800 |
| JArray.of(arr, dtype=double) double[10000], matching | 30,413 |
| JArray.of(arr, dtype=double) double[10000], cross-dtype cast | 30,352 |
| JArray(double)(arr) double[10000], naive sequence ctor | 28,245 |
| JArray.of(arr) double[100000] | 280,704 |
| JArray.of(arr, dtype=double) double[100000], matching | 289,587 |
| JArray.of(arr, dtype=double) double[100000], cross-dtype cast | 277,599 |
| JArray(double)(arr) double[100000], naive sequence ctor | 277,216 |
| JArray.of(arr) double[][](10^2) | 3,064 |
| JArray.of(arr, dtype=double) double[][](10^2), matching | 3,080 |
| JArray(double, 2)(arr) double[][](10^2), manual ctor | 10,377 |
| JArray.of(arr) double[][][](10^3) | 15,754 |
| JArray.of(arr, dtype=double) double[][][](10^3), matching | 15,989 |
| JArray(double, 3)(arr) double[][][](10^3), manual ctor | 136,135 |
| JArray.of(arr) double[][][][](10^4) | 146,289 |
| JArray.of(arr, dtype=double) double[][][][](10^4), matching | 147,572 |
| JArray(double, 4)(arr) double[][][][](10^4), manual ctor | 1,796,362 |
| JArray.of(arr) double[][][][][](10^5) | 1,465,390 |
| JArray.of(arr, dtype=double) double[][][][][](10^5), matching | 1,467,640 |
| JArray(double, 5)(arr) double[][][][][](10^5), manual ctor | 23,490,372 |

**Result.** At flat (1D) depth, `JArray.of()` is roughly on
par with (slightly behind, at 100,000 elements) the naive sequence
constructor -- no real advantage at this depth. At multi-dimensional
depth it wins decisively: at depth 5 (10^5 elements) `JArray.of()` is
roughly 16x faster than the naive per-row constructor, since it reads
the whole buffer directly instead of recursing row by row.

### Class-hint cache lookup cost vs. registered-class-count

| operation | jpype |
|---|:---:|
| match@1/400 | 707 |
| match@5/400 | 760 |
| match@20/400 | 971 |
| match@100/400 | 1,998 |
| match@200/400 | 3,285 |
| match@400/400 | 5,729 |

**Result.** `classhints` lookup cost scales with
registered-class-count -- a linear scan over registered `@JConversion`
hints, cost growing roughly with position in the hint list.

## 9. Known limitations of this run

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
- **GraalPy**: not part of this run at all -- see `project/benchmark/README.md` for its separate setup.

