# jpype vs. jpy vs. jep vs. pyjnius: feature comparison

Scoped to the question "if we ported jpype's test suite to jpy/jep/pyjnius,
how much of it would even have something to run against" (see the
testbench-porting discussion this doc grew out of). Everything below is
grounded in reading each library's own source, not inferred from behavior
or docs: jpy's C source (`~/devel/jpy/src/main/c`) and 21-file Python test
suite, jep's C source (`~/devel/jep/src/main/c`) and 34-file/247-test Python
test suite, and pyjnius's Cython source (`~/devel/pyjnius/jnius/*.pxi`,
`reflect.py`) and 37-file/160-test Python test suite. See
`project/benchmark/RESULTS.md` for the jpype/jpy/jep/pyjnius speed
comparison.

jpy is architecturally a thin C extension with essentially no Python-side
wrapper layer. That's the source of most rows in its table below: jpype
spends effort on Python-idiomatic ergonomics and correctness breadth that
jpy's design never took on. jep sits in between: it embeds Python inside
the JVM (the reverse of jpype/jpy's architecture) and, unlike jpy, does
give Java collections real Python protocol support (`pyjlist.c`,
`pyjmap.c`, `pyjcollection.c`, `pyjiterable.c`) and both functional-interface
duck typing and general multi-method proxy support (`pyjtype.c`'s
`functionalInterface` detection, `jep/Proxy.java` + `java_access/Proxy.c`)
— closer to jpype in scope than jpy is, though still narrower. pyjnius is
closer still in *scope* (it has real collection-protocol, `Comparable`,
functional-interface, and general-proxy support too, all confirmed working
this session -- see below), but its multi-dimensional/buffer array support
is the narrowest of the three, and its general-proxy implementation has a
real, reproduced crash bug jpy/jep don't have.

## Feature matrix: all four libraries

A single-glance summary of the pairwise sections below, which have the
full evidence/citations for every row here. "Yes"/"No" means fully
working and verified (empirically this session, where practical) or
confirmed absent from source; anything more nuanced gets a footnote.

### Language / object-model integration

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Collection protocols (`List`/`Map`/`Iterator` as native Python `list`/`dict`/iterator) | Yes | No | Yes (`pyjlist.c`/`pyjmap.c`/etc.) | Yes (`protocol_map`) |
| `Comparable`/`Iterable` duck-typing (`<`, `for x in`, `hash()`) | Yes | No | Yes | Yes (`protocol_map`) |
| `AutoCloseable` → Python context manager (`with obj:`) | Yes (`jpype/_jio.py`) | No | Yes (`pyjautocloseable.c`) | Yes (`protocol_map`) |
| Functional-interface duck typing (bare `lambda`/callable as a Java SAM arg, no proxy class needed) | Yes | No (explicit proxy object only) | Yes (`pyjtype.c`'s `functionalInterface`) | Yes (`jnius_conversion.pxi`) |
| General proxy (Python object implementing an arbitrary Java interface) | Yes, multithread-safe | Broken in this checkout\* | Yes | Yes, but with a reproduced crash bug\*\* |

\* jpy's `PyObject.createProxy()` exists but didn't produce a usable
object from Python in this checkout (see jpy section below).

\*\* pyjnius's proxy mechanism itself works for the common case, but a
Python-implemented interface method receiving a **null** `Object`
argument reliably segfaults the JVM (`jni_GetObjectClass` on a null
jobject), and even the non-null case silently returns `None` instead of
the real object -- see the dedicated bug section below. This is a defect
found in this checkout, not a design gap like jpy's row above.

### Conversion / arrays

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Class hints / custom conversions (`@JConversion`) | Yes (54 tests) | No | No | No |
| Buffer-protocol (numpy) array push, flat 1D | Yes, bulk path, value-correct across ~14 source formats (see dtype matrix below) | Yes, via a separate argument-matching fast path, not `jpy.array()` itself -- **but with no dtype check: silently bit-reinterprets same-width dtype mismatches, see dtype matrix below** | Yes, genuine bulk path, but a closed 8-dtype allowlist (no `float16`) | **No, rejected unconditionally** (`"Expecting a python list/tuple"`) |
| Buffer-protocol (numpy) array push, multi-dimensional (`int[][]`+) | Yes, bulk path (this session's follow-on) | No (`TypeError`, falls back to slower per-element path) | No (`TypeError: Error matching ndarray.dtype...`) | **No** (same blanket rejection as 1D) |
| Per-class `findJavaConversion` caching w/ invalidation | Yes (this session) | N/A -- already unconditionally cheap per call | N/A -- short-circuits on arity before any per-arg work | Not applicable the same way; no equivalent caching opportunity found |

### numpy scalar argument dispatch (`Math.max(int,int)`, i.e. an overloaded method)

| numpy scalar type | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| `numpy.int32` | OK | FAILS ("ambiguous Java method call") | OK | FAILS ("no matching method") |
| `numpy.int64` | OK | FAILS (same) | OK | FAILS (same) |
| `numpy.float32` | OK | FAILS (same) | FAILS ("cannot be interpreted as an integer") | FAILS (same) |
| `numpy.float64` | OK | OK (genuine Python `float` subclass) | OK | OK (same reason) |

All four verified empirically this session (not assumed from source
alone). jpype is the only one that's fully correct across every numpy
scalar type. jep is notably *better* than jpy/pyjnius here for integers
(`int32`/`int64` both resolve correctly against `Math.max`'s overloads --
`jep_numpy.c` has an explicit numpy-scalar path jpy/pyjnius lack) but has
its *own*, different gap at `float32` specifically. jpy and pyjnius share
essentially the same failure shape (only a genuine Python `int`/`float`
passes their fast dispatch, no numpy-aware fallback for `int`/`long`
params) but fail with different, differently-worded errors.

### Buffer dtype conversion: the full matrix, and a serious jpy correctness bug

Requested explicitly this session ("check all the conversion types, not
just float8 -- jpype has a very large number"). Tested every combination
empirically, source-grounded where behavior needed explaining, not
assumed from a single example.

**jpype's real fast-path matrix is large and value-correct.** Its buffer
dispatcher (`getConverter()`, `jp_convert.cpp`) recognizes 14 source
buffer format codes -- `?`/`c`/`b` (bool/int8), `B` (uint8), `h`/`H`
(int16/uint16), `i`/`l`/`I`/`L` (int32/uint32), `q`/`Q` (int64/uint64),
`f`/`d` (float32/float64), `n`/`N` (native ssize_t/size_t) -- **and
`e`, IEEE 754 half-precision (`float16`), via a dedicated hand-written
bit-level decoder** (`Half<Convert<float>::toX>`, manually unpacks sign/
exponent/fraction including subnormals, then reuses the same
`Convert<float>::toX` machinery every other type shares) -- not a
fallback, a genuine bulk fast path, contrary to what the previous
revision of this doc assumed without checking. Tested all 12 realistic
numpy dtypes (`bool`/`int8`/`uint8`/`int16`/`uint16`/`int32`/`uint32`/
`int64`/`uint64`/`float16`/`float32`/`float64`) against all 8 Java
primitive array types directly (`JArray(JType)(arr)`): **94 of 96
combinations succeed with genuinely converted values** (confirmed
`float32([1.0,2.0,3.0]) -> int[]` gives `[1, 2, 3]`, not bit garbage).
The only 2 failures are `boolean`/`float*` → `char`, both sensible
rejections (Java has no implicit boolean-to-char or float-to-char
narrowing either). `float8_e4m3` (via `ml_dtypes`, since neither Java
nor numpy has a native 8-bit float) is the one case with no recognized
format code at all -- it still succeeds for `float`/`double` targets, but
via the general per-element fallback (`ml_dtypes` scalars support
Python's `__float__` protocol), not the buffer fast path, since `float8`
isn't in the 14-format list above.

**jpy's buffer path has no dtype/format check at all -- confirmed to
silently corrupt data for same-width dtype mismatches.** Its argument
buffer handler (`jpy_jtype.c` ~line 2026) does a raw `memcpy(arrayItems,
pyBuffer->buf, itemCount*itemSize)` after checking only that the byte
length matches -- it never inspects `pyBuffer->format`. Verified: passing
`numpy.float32([1.0, 2.0, 3.0])` where `DeepBench.sumIntArray(int[])`
expects an `int[]` returns `3217031168`, not an error and not `6` --
that's the raw IEEE-754 bit patterns of `1.0f`/`2.0f`/`3.0f`
reinterpreted as `int32` and summed (`1065353216 + 1073741824 +
1077936128 = 3217031168`, confirmed by hand). This is a **silent
correctness bug**, not a mere gap: it happens whenever the source dtype
and the target's byte width match but the dtype itself doesn't
(`float32`↔`int32`, `float64`↔`int64`) -- a plausible real mistake (wrong
dtype passed to a Java-typed API) produces a plausible-looking wrong
number with no error at all. Different-width mismatches are safely
caught (`uint8`/`float16` → `int[]` both correctly raise "no matching
Java method overloads found", since the byte-length check alone catches
those) -- it's specifically the same-width, wrong-dtype case that's
dangerous. Arguably worse than pyjnius's crash bug below: a crash is at
least loud.

**jep's numpy fast path is a safe, closed allowlist** -- confirmed
`float32` → `int[]` and `float16` → `int[]` both cleanly fail
(`"Error matching ndarray.dtype to Java primitive type"`), consistent
with `convert_pyndarray_jprimitivearray`'s exact-match check against
exactly 8 `NPY_*` constants (no `NPY_FLOAT16` in the list at all, so
`float16` is a hard, permanent gap for jep, not just untested). No
reinterpretation risk, since dtype identity is checked before any data
is touched.

**pyjnius**: rejects all numpy input unconditionally regardless of
dtype (established earlier) -- trivially safe, trivially incapable.

### Other

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Pickling / `copyreg` support | Yes (9 tests) | No | No | No |
| Caller-sensitive JDK method handling | Yes (20 tests) | No | No | No |
| Javadoc-derived docstrings / Jedi / typing-stub generation | Yes (~37 tests) | No | No (bare `dir()` only) | No (bare `dir()` only, confirmed `__doc__ is None`) |
| Rich JVM-finder / startup-options API | Yes | Different, narrower (`jpy.create_jvm`) | N/A -- embeds Python *inside* the JVM, reverse architecture | Auto-starts on first `autoclass()` call from a preset classpath (`jnius_config`) -- simplest of the four, but least configurable |
| Late class loading (add a jar/path to the classpath *after* the JVM is already running) | Yes -- `addClassPath()` live-injects into `org.jpype.JPypeContext`'s custom classloader via JNI, a mechanism built specifically for this | No -- `jvm_classpath` is only settable as an argument to `init_jvm()`, no post-startup API found | No -- architecture mismatch, not just a missing feature: jep doesn't start its own JVM, it's embedded inside one already launched via `java -classpath ...`; `JepConfig.setClassLoader()` supplies a pre-built `ClassLoader` to a new sub-interpreter at construction time, not a live "add this jar now" call | No, and more deliberately than jpy/jep: `add_classpath()` calls `check_vm_running()` first and raises `ValueError` if the JVM has already started -- functionally identical to `set_classpath()`, both pre-startup only |

### Test suite size

| | jpype | jpy | jep | pyjnius |
|---|---:|---:|---:|---:|
| test files | 90 | 21 | 34 | 37 |
| tests | 1,884 | 151 | 247 | 160 |

Collection-protocol/Comparable/functional-interface/general-proxy support
means jep and pyjnius both have *something* to port a meaningfully larger
fraction of jpype's suite against than jpy does. `test_classhints.py`/
`test_hints.py`/`test_customizer.py`, `test_pickle.py`/`test_serial.py`,
and the introspection-ergonomics files are gaps for all three of
jpy/jep/pyjnius. Multi-dimensional/buffer array tests are a gap for jpy
(partial) and pyjnius (total) but not jep (partial, same as jpy).

## jpype vs. jpy

### Features jpype has that jpy does not

| Feature | jpype | jpy | Evidence |
|---|---|---|---|
| Python collection protocols on `java.util.List`/`Map`/etc. | `list`/`dict`-like `__getitem__`, iteration, `len()` | None — Java collections are opaque wrapper objects | `jpy_jtype.c:2690-91` sets `tp_as_sequence`/`tp_as_mapping` to `NULL` unconditionally |
| `Comparable`/`Iterable`/`Hashable` duck-typing | Java objects implementing these participate in Python's `<`, `for x in`, `hash()`, etc. | None found | no `Comparable`/`Iterable` handling in `jpy_jtype.c` |
| Class hints / custom conversions (`@JConversion`, `JConversionCustomizer`) | Full registration system (`test_classhints.py`, `test_hints.py`, `test_customizer.py`, 54 tests) | No equivalent subsystem | grep of `jpy/src/main/c` for hints/customizer machinery: nothing |
| Functional-interface duck typing (pass a Python `lambda`/callable directly as a Java SAM interface arg) | Supported (`test_functional.py`, `test_lambdas.py`) | Only via explicit proxy objects, not implicit lambda conversion | no functional-interface matcher in `jpy_jtype.c`/`jpy_jmethod.c` |
| Proxy (`@JImplements`, Python object implementing a Java interface) | Supported, multithread-safe (`test_proxy.py`, `test_proxy_multithreaded.py`) | Present (`PyObject.createProxy()`) but did not produce a usable object in this checkout — see `project/benchmark/RESULTS.md` footnote | reproduced this session |
| Multi-dimensional numpy array push (`int[][]` etc. from an ndarray) | Bulk buffer path, see `caching-multidim-push` branch | Not supported — raises `TypeError: Error matching ndarray.dtype to Java primitive type` for any ndim > 1 | reproduced this session via `bench_deep_jep.py`-style harness (jpy hits its own equivalent failure) |
| Pickling / `copyreg` support for Java objects | `test_pickle.py`, `test_serial.py` (9 tests) | No equivalent | not present in jpy source or test suite |
| Introspection ergonomics: docstrings from Javadoc, `repr()`, Jedi/IDE completion, module/typing-stub generation | `test_docstring.py`, `test_jedi.py`, `test_repr.py`, `test_module.py`, `test_module2.py` (~37 tests) | None | no analogous test files or source in jpy |
| Caller-sensitive JDK method handling | `test_caller_sensitive.py` (20 tests) | Not handled as a distinct case | no reference in jpy source |
| JVM lifecycle ergonomics (`jvmfinder`, startup options, `test_startup.py`/`test_opts.py`, 40 tests) | Rich, discoverable JVM-finding + startup-option API | Different, narrower launch API (`jpy.create_jvm`) — not a subset, a different shape | jpy has no equivalent finder/options surface |
| Per-class conversion caching with generation-based invalidation | Yes (this session's `caching` branch) | N/A — jpy's matching is already unconditionally cheap per call (see below), so it never needed a cache | `jpy_jtype.c`/`jpy_jmethod.c` read this session |

### Where jpy is faster *because* it does less (not a jpype gap — a jpy tradeoff)

These aren't "jpype missing a feature" rows — they're places jpy is
faster because it skips work jpype does on purpose. Recorded here for
completeness since they came up in the same source dive.

| Behavior | jpype | jpy |
|---|---|---|
| Array-argument matching | Validates every element up front (needed for correct Java-style overload disambiguation) | Does not inspect elements before committing to a conversion — confirmed by reading `jpy_jtype.c`/`jpy_jmethod.c` |
| Scalar/dispatch/proxy matching | More abstraction layers, broader general-purpose machinery (implicit numeric widening, the full `JPConversion` chain) | Leaner architecturally, but *not* found to be cutting correctness corners here — see `RESULTS.md` |

### Not compared here (excluded, no jpy equivalent to port to)

Fault-injection (`test_fault.py`, 88 tests) and coverage-instrumentation
tests (`test_coverage.py`, `test_javacoverage.py`, 50 tests) test jpype's
own internal error paths, not a portable behavior — excluded per the
original ask, not because jpy lacks the feature.

### jpy suite-size context

jpype: 1,884 tests across 90 files (~21k lines), `test/jpypetest/`.
jpy: 151 tests across 21 Python test files, `~/devel/jpy/src/test/python/`.

Of jpype's suite, roughly 250+ tests exercise features with literally no
jpy counterpart (the first table above) — those can't be "ported," only
noted as gaps. The remainder (conversion, arrays, strings, exceptions,
fields/properties, overloads/varargs, reflect, jclass/jpackage/imports,
numeric/boxing, buffers, inherit, hash, synchronized) is the realistic
portable subset if this comparison is ever turned into an actual ported
test run.

## jpype vs. jep

### Features jpype has that jep does not

| Feature | jpype | jep | Evidence |
|---|---|---|---|
| Class hints / custom conversions (`@JConversion`, `JConversionCustomizer`) | Full registration system (54 tests) | No equivalent subsystem | no hints/customizer machinery found in `jep/src/main/c` |
| Multi-dimensional numpy array *push* (Python ndarray → `int[][]` etc. as a method argument) | Bulk buffer path, see `caching-multidim-push` branch | Not supported — raises `TypeError: Error matching ndarray.dtype to Java primitive type` for any ndim > 1 | reproduced this session; `jep_numpy.c`'s only `PyArray_NDIM` use (line 399) is on the opposite direction (Java array → Python ndarray return value), confirming the push path genuinely has no multi-dim handling, not just an untested one |
| Pickling / `copyreg` support for Java objects | `test_pickle.py`, `test_serial.py` (9 tests) | No equivalent | not present in jep source or test suite |
| Javadoc-derived docstrings, Jedi/IDE completion, module/typing-stub generation | `test_docstring.py`, `test_jedi.py`, `test_module.py`, `test_module2.py` | Only bare `dir()` listing method names (`test_dir.py`) — no docstrings, no stub generation | `pyjobject`/`pyjtype` expose method names via `dir()` but no docstring text sourced from Javadoc |
| Caller-sensitive JDK method handling | `test_caller_sensitive.py` (20 tests) | Not handled as a distinct case | no reference found in jep source |
| Per-class conversion caching with generation-based invalidation | Yes (this session's `caching` branch) | Not applicable the same way — jep's overload resolution already short-circuits on arity before doing any per-argument type work (see below) | `pyjmultimethod.c:118-155`, `pyjmethod.c:284`, read this session |

### Features jep has that jpy lacks (closer to jpype here)

Noted because it changes the porting-effort picture from the jpy table
above — these jpype-suite categories that were "no counterpart, don't
bother" for jpy actually have something to port to for jep.

| Feature | jep | Evidence |
|---|---|---|
| Python collection protocols on `java.util.List`/`Map`/`Collection`/`Iterable` | Real `__getitem__`/`__setitem__`/slicing/iteration, backed by the actual Java collection | `pyjlist.c`, `pyjmap.c`, `pyjcollection.c`, `pyjiterable.c`, `pyjiterator.c` |
| `Comparable`/hashing duck-typing | `tp_richcompare` delegates to `Comparable.compareTo`, `tp_hash` to `hashCode` | `pyjobject.c:155-181`, `pyjobject.c:310-312` |
| Functional-interface duck typing (pass a Python callable directly as a Java SAM interface arg) | Detected via `isInterface` + single-abstract-method check | `pyjtype.c:259-335` (`functionalInterface`) |
| General multi-method proxy (Python object implementing an arbitrary Java interface) | Full `InvocationHandler`-style proxy, own test file | `jep/Proxy.java`, `java_access/Proxy.c`, `test_jproxy.py` |
| `synchronized` block support | `pyjmonitor.c`, `test_synchronized.py` | jpy has no equivalent test or source |

### Where jep is faster *because* it does less (not a jpype gap — a jep tradeoff)

Verified this session by reading `pyjmultimethod.c`/`pyjmethod.c`, not
inferred from timing. jep's overload resolution filters candidates by
**parameter count only** (O(1), no type inspection); the expensive
per-argument type-compatibility check (`PyJMethod_CheckArguments`) only
runs when two or more candidates share the same arity. For a method with a
single overload (the common case, and the case for every array/scalar
benchmark in `RESULTS.md`), jep converts arguments directly against the one
known parameter type in a single pass. jpype always does two passes — a
full `matches()` scoring scan, then a separate `convert()` — even with no
overload ambiguity, because its architecture doesn't special-case "only one
candidate." This is a genuine skip, not a leaner-but-equivalent
implementation: it's also *why* jep falls badly behind on the 16-overload
dispatch benchmark in `RESULTS.md` (all candidates share arity there, so
the shortcut can't fire and the expensive check runs repeatedly).

### Not compared here (excluded, no jep equivalent to port to)

Same exclusion as jpy: fault-injection (`test_fault.py`) and
coverage-instrumentation tests (`test_coverage.py`, `test_javacoverage.py`)
test jpype's own internals, not portable behavior.

### jep suite-size context

jep: 34 Python test files, 247 tests, `~/devel/jep/src/test/python/`.
Larger than jpy's suite, still under a fifth of jpype's 1,884. The
collection-protocol, Comparable, functional-interface, and proxy support
above mean a meaningfully larger fraction of jpype's suite has *something*
to port against for jep than for jpy — but `test_classhints.py`/
`test_hints.py`/`test_customizer.py`, `test_pickle.py`/`test_serial.py`,
and the introspection-ergonomics files remain gaps for jep too.

## jpype vs. pyjnius

### Features jpype has that pyjnius does not

| Feature | jpype | pyjnius | Evidence |
|---|---|---|---|
| Class hints / custom conversions (`@JConversion`, `JConversionCustomizer`) | Full registration system (54 tests) | No equivalent subsystem | grep of `jnius/*.pxi`/`reflect.py` for hints/customizer/register-conversion machinery: nothing |
| Multi-dimensional / buffer-protocol array push (`int[]`/`int[][]` etc. from an ndarray) | Bulk buffer path (`caching-multidim-push` branch), any dimension | Not supported at **any** dimension, including flat 1D — narrower than both jpy and jep, which at least accept a 1D buffer object | reproduced this session: `DeepBench.sumIntArray(numpy.arange(...))` raises `JavaException('Expecting a python list/tuple, got array(...)')` unconditionally, for any ndim |
| Pickling / `copyreg` support for Java objects | `test_pickle.py`, `test_serial.py` (9 tests) | No equivalent | no `__reduce__`/pickle-registration logic in pyjnius's own source (the one `pickle` hit in the built `jnius.c` is Cython's own generated module boilerplate, not a feature) |
| Introspection ergonomics: docstrings from Javadoc, Jedi/IDE completion, module/typing-stub generation | `test_docstring.py`, `test_jedi.py`, `test_repr.py`, `test_module.py`, `test_module2.py` (~37 tests) | Only bare `dir()` listing (method names visible, `__doc__` is `None` for every bound method, confirmed this session) — same situation as jep | no docstring-generation code found in `jnius/*.pxi`/`reflect.py` |
| Caller-sensitive JDK method handling | `test_caller_sensitive.py` (20 tests) | Not handled as a distinct case | no reference found in pyjnius source |
| Per-class conversion caching with generation-based invalidation | Yes (this session's `caching` branch) | Not applicable the same way — no equivalent per-call matching-cost problem was found to cache in the first place (see numpy-scalar-dispatch note below, which is a coverage gap, not a caching opportunity) | `jnius_conversion.pxi` read this session |

### Features pyjnius has that jpy lacks (closer to jpype here, comparable to jep)

Confirmed working empirically this session, not just found in source:

| Feature | pyjnius | Evidence |
|---|---|---|
| Python collection protocols on `java.util.List`/`Map`/`Collection`/`Iterator`/`Map.Entry` | Real `__getitem__`/`__setitem__`/`__len__`/`__contains__`/`__iter__`, backed by the actual Java collection | `reflect.py`'s `protocol_map`, applied automatically inside `autoclass()` to every class whose hierarchy includes one of these interfaces; verified an `ArrayList`/`HashMap` support `len()`, indexing, iteration, and `in` directly |
| `Comparable`/`Iterable` duck-typing | `__lt__`/`__gt__`/`__eq__`/etc. delegate to `compareTo`/`equals`; `__iter__` delegates to `iterator()` | same `protocol_map`; verified `Integer(3) < Integer(5)` works directly |
| `AutoCloseable`/`Closeable` → Python context manager protocol | `__enter__`/`__exit__` delegate to `close()` | same `protocol_map` (jpype has this too, `jpype/_jio.py` -- not a jpype gap, just noting pyjnius has it as well, unlike the jpy/jep tables above which didn't need to call it out) |
| Functional-interface duck typing (pass a Python `lambda`/callable directly as a Java SAM interface arg) | Supported | `jnius_conversion.pxi`'s functional-interface detection (~line 415-451); verified `DeepBench.invokeCallback(lambda x: x + 1, 5)` works directly, no proxy class needed |
| General multi-method proxy (Python object implementing an arbitrary Java interface) | `PythonJavaClass` subclass + `@java_method('<jni-signature>')`, own test file (`test_proxy.py`) | present and works for the common case (verified `int`-arg callback) -- **but see the bug below**, unlike jep's proxy support, which has no equivalent defect found |

### A real, reproduced defect in pyjnius's proxy implementation (not a feature gap — a bug)

Unlike jpy's proxy gap (a construction-time failure, "no matching Java
method overloads found" — see the jpy table above) and jep's proxy support
(no defect found), pyjnius's general proxy mechanism has a genuine crash
bug: a Python-implemented Java interface method receiving a genuinely
**null** `Object` argument (`DeepBench.invokeObjectCallbackWithNull` —
`jpype.benchmark.DeepBench`'s `ObjectCallback` methods exist specifically
to cover this case, per `jp_proxy.cpp`'s own history) reliably segfaults
the JVM with a native `SIGSEGV` in `jni_GetObjectClass`. Reproduced
independently three times against a fresh build in a disposable venv
(ruled out as stale-build-state first, per this repo's CLAUDE.md, before
treating it as real). pyjnius's proxy-argument-marshalling code calls
`GetObjectClass`/`IsSameObject` on the argument without a null check first
— undefined behavior over JNI, not a usage error on this session's part.

Even the non-crashing case (a real, non-null `Object` argument) doesn't
work correctly: `invokeObjectCallback` silently returns `None` instead of
the object the Python callback handed back — a separate, non-fatal
correctness bug in the return-value path. See
`project/benchmark/RESULTS.md`'s pyjnius section and
`project/benchmark/pyjnius/proxy.py` for the full detail; that script
deliberately never calls the null-argument variant.

### Where pyjnius is faster/slower *because* of its own tradeoffs

| Behavior | jpype | pyjnius |
|---|---|---|
| numpy scalar dispatch for `int`/`long` parameters | Resolves all numpy scalar types (`int32`/`int64`/`float32`/`float64`) correctly and unambiguously | Same underlying gap as jpy (no fallback for non-exact-Python-type numeric args on `int`/`long` params), but a cleaner failure mode: `Math.max(np.int32(3), 5)` raises `"No static methods called max in java/lang/Math matching your arguments... available: [...]"` — a plain "no match," not jpy's "ambiguous Java method call" |
| Scalar/boxed/string/proxy call overhead generally | See `RESULTS.md` | Slowest of the four libraries on every boxed/string/proxy row measured this session, sometimes by 5-10x (`new Integer`: 7403ns vs. 840-1257ns elsewhere; `new String`+`.toString()`: 22349ns vs. 927-2512ns; proxy: 39412ns vs. 2240-2655ns) — flagged, not attributed to a specific mechanism at the source level the way the jpy/jep rows above were |

### Not compared here (excluded, no pyjnius equivalent to port to)

Same exclusion as jpy/jep: fault-injection (`test_fault.py`) and
coverage-instrumentation tests (`test_coverage.py`, `test_javacoverage.py`)
test jpype's own internals, not portable behavior.

### pyjnius suite-size context

pyjnius: 37 Python test files, 160 tests, `~/devel/pyjnius/tests/`. Between
jpy's 21 files and jep's 34-file/247-test suite in file count, but fewer
total tests than jep's. The collection-protocol/Comparable/functional-
interface/general-proxy support above means pyjnius has *something* to
port against for a comparably large slice of jpype's suite as jep does —
but `test_classhints.py`/`test_hints.py`/`test_customizer.py`,
`test_pickle.py`/`test_serial.py`, the introspection-ergonomics files, and
(uniquely among the three) *any* multi-dimensional/buffer array test
remain gaps for pyjnius.

## Future: jpype's reverse-embedding direction (`origin/reverse`)

**A wrong assumption got made in this doc's first pass, and it's worth
recording plainly, because it's the kind of mistake an AI assistant
reasoning about this codebase keeps being tempted to make again:** the
initial write-up claimed jpype's architecture is permanently
Python-hosts-the-JVM, and that jep's "embeds Python inside the JVM"
niche — a pure Java application (`java -cp your_app.jar
com.your.Main`) bringing up an embedded Python interpreter itself,
Java as the host process, not Python — was therefore something no
amount of jpype development could ever close. That claim was made
without checking the `reverse` branch first, and it's false.

`org.jpype.MainInterpreter` (`~/devel/jpype` on `origin/reverse`,
`native/jpype_module/src/main/java/org/jpype/MainInterpreter.java`) is
exactly that entry point — a Java-side singleton, "main entry point for
interacting with the Python interpreter" per its own javadoc, that
locates/probes/launches an embedded CPython interpreter from Java code,
no Python process involved in starting anything. Demonstrated concretely
by `native/jpype_module/src/test/java/runner/HelloWorldMain.java`, a
pure `public static void main(String[] args)` with zero Python
involvement in bootstrapping:

```java
public static void main(String[] args) {
    MainInterpreter.getInstance().start(new String[0]);
    Script context = new Script(MainInterpreter.getInstance());
    context.exec("msg = 'Hello World from Python'");
    PyObject msg = context.eval("msg");
    ...
}
```

It goes further than parity with jep: `org.jpype.script.JPypeScriptEngine`
implements `javax.script.AbstractScriptEngine`/`Invocable` — the standard
JSR-223 scripting API every JVM already has a pluggable-scripting-language
story for (`ScriptEngineManager`). That's arguably a *more* idiomatic
"Java embeds a scripting language" integration than jep's own bespoke API,
since it's the interface Java itself defines for this exact use case.

**Status, to avoid overclaiming the other direction**: `origin/reverse`
is 190 commits ahead of `review` (the main branch) and not yet merged —
substantial, apparently mature work (subinterpreters, cross-interpreter
GC, an `InterpreterPipe`, `toPython()` conversions for
`Instant`/`Path`/`File`/`BigDecimal`/dates, coverage raised to 90-100% on
many modules per its own plan docs), but "future," not current `review`
behavior, and not re-verified with the same empirical rigor (actually
running it, checking edge cases) applied to jpy/jep/pyjnius throughout
the rest of this document. Once merged, though, the conclusion changes
concretely: jpype covers *both* embedding directions natively — its
existing, dominant Python-hosts-Java strength (everything above), plus
Java-hosts-Python via `MainInterpreter`/`JPypeScriptEngine` — where jep
currently has the *only* other native story for the second direction, and
jpy/pyjnius have neither.

**The general lesson, stated for whoever (human or AI) reads this doc
next**: "library X's architecture makes Y permanently impossible" is a
claim that must be checked against that library's own branches/plans
before being written down, not inferred from "that's not what the
`review` branch does today." This doc got that wrong once already.
