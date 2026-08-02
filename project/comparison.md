# jpype vs. jpy vs. jep: feature comparison

Scoped to the question "if we ported jpype's test suite to jpy/jep, how much
of it would even have something to run against" (see the testbench-porting
discussion this doc grew out of). Everything below is grounded in reading
each library's own source, not inferred from behavior or docs: jpy's C
source (`~/devel/jpy/src/main/c`) and 21-file Python test suite, and jep's C
source (`~/devel/jep/src/main/c`) and 34-file/247-test Python test suite.
See `project/benchmark/RESULTS.md` for the jpype/jpy/jep speed comparison.

jpy is architecturally a thin C extension with essentially no Python-side
wrapper layer. That's the source of most rows in its table below: jpype
spends effort on Python-idiomatic ergonomics and correctness breadth that
jpy's design never took on. jep sits in between: it embeds Python inside
the JVM (the reverse of jpype/jpy's architecture) and, unlike jpy, does
give Java collections real Python protocol support (`pyjlist.c`,
`pyjmap.c`, `pyjcollection.c`, `pyjiterable.c`) and both functional-interface
duck typing and general multi-method proxy support (`pyjtype.c`'s
`functionalInterface` detection, `jep/Proxy.java` + `java_access/Proxy.c`)
— closer to jpype in scope than jpy is, though still narrower.

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
jpy: 21 Python test files total, `~/devel/jpy/src/test/python/`.

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
