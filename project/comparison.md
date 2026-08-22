# jpype vs. jpy vs. jep vs. pyjnius: feature comparison

Scope: if jpype's test suite were ported to jpy/jep/pyjnius, how much of it
would have something to run against (see the testbench-porting discussion
this doc grew out of). Organized by axis — object model, conversion,
exceptions, threading, object lifetime, interpreter lifecycle, embedding,
extensibility, ergonomics — rather than by library pair, so a reader can
go straight to the axis they care about and see all four libraries
side by side. Grounded in each library's own source: jpy's C source
(`~/devel/jpy/src/main/c`, 21-file/151-test suite), jep's C source
(`~/devel/jep/src/main/c`, 34-file/247-test suite), and pyjnius's Cython
source (`~/devel/pyjnius/jnius/*.pxi`, `reflect.py`, 37-file/160-test
suite). See `project/benchmark/RESULTS.md` for the jpype/jpy/jep/pyjnius
speed comparison.

jpy is architecturally a thin C extension with essentially no Python-side
wrapper layer — most of jpy's gaps below follow from that: jpype spends
effort on Python-idiomatic ergonomics and correctness breadth that jpy's
design doesn't take on. jep embeds Python inside the JVM (the reverse of
jpype/jpy's architecture) and, unlike jpy, gives Java collections real
Python protocol support plus functional-interface duck typing and
general multi-method proxy support — closer to jpype in scope than jpy,
though still narrower. pyjnius is closer still in scope (real
collection-protocol, `Comparable`, functional-interface, and
general-proxy support, all verified working), but its multi-dimensional/
buffer array support is the narrowest of the three, and its general-proxy
implementation has a reproducible crash under a specific input that
jpy/jep don't share.

Each axis section below follows the same shape: a short intro, a feature
table, prose analysis for anything that doesn't fit a table cell, and —
where a direct API mapping exists across libraries — a syntax-equivalence
table. Not every axis has enough of a direct API mapping to warrant a
syntax table (internal mechanisms like GIL discipline or GC handling
aren't something a user calls directly); those axes end with the prose
analysis instead.

## Axis 0: Starting the JVM/interpreter and managing the classpath

The first thing any of these libraries does. Grounded here specifically
in what each library's own startup API looks like and how late a jar or
directory can be added to the classpath — not a deep dive into general
class-access syntax (`JClass(...)`, `autoclass(...)`, etc.), which this
document's source reading didn't go deep enough on for every library to
put in a verified table; pyjnius's `autoclass()` is the one class-access
call cited elsewhere in this document with enough grounding to include
here.

| Concern | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| JVM/interpreter startup API | Rich, discoverable JVM-finding + startup-option API (`startJVM()`) | Different, narrower launch API (`jpy.create_jvm`) — a different shape, not a subset | N/A — embeds Python inside the JVM, reverse architecture: a Python process doesn't start a JVM here, a JVM starts Python | Auto-starts on first `autoclass()` call from a preset classpath (`jnius_config`) — simplest of the four, least configurable |
| Add a jar/path to the classpath after startup | Yes — `addClassPath()` live-injects into `org.jpype.JPypeContext`'s custom classloader via JNI | No — `jvm_classpath` is only settable as an argument to `init_jvm()`, no post-startup API found | No — architectural: jep doesn't start its own JVM, it's embedded inside one already launched via `java -classpath ...`; `JepConfig.setClassLoader()` supplies a pre-built `ClassLoader` to a new sub-interpreter at construction time, not a live "add this jar now" call | No, and more deliberately than jpy/jep: `add_classpath()` calls `check_vm_running()` first and raises `ValueError` if the JVM has already started — functionally identical to `set_classpath()`, both pre-startup only |

Class access syntax: pyjnius's `autoclass('java.package.ClassName')`
(`reflect.py`) is the one call this document verified directly (see the
object-model axis below, where `protocol_map` is applied "inside
`autoclass()`"). jpype, jpy, and jep each have their own class-access
entry point (`JClass`/`JPackage` imports for jpype, a `jpy`-module
lookup for jpy, an interpreter-level import call for jep) that this
comparison didn't verify in the same depth — worth a follow-up pass
before treating exact call signatures as citable here.

## Axis 1: Object-model integration

Whether a Java object behaves like the Python thing it resembles —
`List`/`Map` as `list`/`dict`, `Comparable`/`Iterable` in Python's own
operators, `AutoCloseable` under `with`, a Java functional interface
satisfied by a bare Python callable — and whether a Python object can
stand in for an arbitrary Java interface via a proxy.

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Collection protocols (`List`/`Map`/`Iterator` as native Python `list`/`dict`/iterator) | Yes | No — `jpy_jtype.c:2690-91` sets `tp_as_sequence`/`tp_as_mapping` to `NULL` unconditionally | Yes (`pyjlist.c`/`pyjmap.c`/`pyjcollection.c`/`pyjiterable.c`/`pyjiterator.c`) | Yes (`reflect.py`'s `protocol_map`, applied automatically inside `autoclass()`; verified an `ArrayList`/`HashMap` support `len()`, indexing, iteration, and `in` directly) |
| `Comparable`/`Iterable`/`Hashable` duck-typing (`<`, `for x in`, `hash()`) | Yes | No — no `Comparable`/`Iterable` handling found in `jpy_jtype.c` | Yes (`tp_richcompare` delegates to `compareTo`, `tp_hash` to `hashCode`, `pyjobject.c:155-181,310-312`) | Yes (same `protocol_map`; verified `Integer(3) < Integer(5)` works directly) |
| `AutoCloseable` → Python context manager (`with obj:`) | Yes (`jpype/_jio.py`) | No | Yes (`pyjautocloseable.c`) | Yes (same `protocol_map`, `__enter__`/`__exit__` delegate to `close()`) |
| Functional-interface duck typing (bare `lambda`/callable as a Java SAM arg, no proxy class needed) | Yes | No — only explicit proxy objects, no implicit lambda conversion | Yes (`pyjtype.c:259-335`'s `functionalInterface` detection) | Yes (`jnius_conversion.pxi`'s functional-interface detection, ~line 415-451; verified `DeepBench.invokeCallback(lambda x: x + 1, 5)` works directly) |
| General proxy (Python object implementing an arbitrary Java interface) | Yes, multithread-safe (`test_proxy.py`, `test_proxy_multithreaded.py`) | `PyObject.createProxy()` exists but did not produce a usable object in this checkout | Yes, no defect found (`jep/Proxy.java`, `java_access/Proxy.c`, `test_jproxy.py`) | Works for the common case (verified `int`-arg callback), with a reproduced crash on one input shape — see below |
| `synchronized` block support | Yes | No equivalent test or source | Yes (`pyjmonitor.c`, `test_synchronized.py`) | Not covered in this pass |

### A reproduced defect in pyjnius's proxy implementation

Unlike jpy's proxy gap (a construction-time failure — `PyObject.createProxy()`
never produced a usable object in this checkout) and jep's proxy support
(no defect found), pyjnius's general proxy mechanism crashes on one
specific input: a Python-implemented Java interface method receiving a
genuinely **null** `Object` argument (`DeepBench.invokeObjectCallbackWithNull`
— `jpype.benchmark.DeepBench`'s `ObjectCallback` methods exist
specifically to cover this case, per `jp_proxy.cpp`'s own history)
segfaults the JVM with a native `SIGSEGV` in `jni_GetObjectClass`.
Reproduced independently three times against a fresh build in a
disposable venv (checked for stale-build-state first, per this repo's
CLAUDE.md, before treating it as real). pyjnius's proxy-argument-marshalling
code calls `GetObjectClass`/`IsSameObject` on the argument without a
null check.

The non-crashing case (a real, non-null `Object` argument) also doesn't
round-trip correctly: `invokeObjectCallback` returns `None` instead of
the object the Python callback handed back. See
`project/benchmark/RESULTS.md`'s pyjnius section and
`project/benchmark/pyjnius/proxy.py`; that script deliberately never
calls the null-argument variant.

### Where jep does less work per call

Verified by reading `pyjmultimethod.c`/`pyjmethod.c`, not inferred from
timing. jep's overload resolution filters candidates by parameter count
only (O(1), no type inspection); the per-argument type-compatibility
check (`PyJMethod_CheckArguments`) only runs when two or more candidates
share the same arity. For a method with a single overload (the common
case, and the case for every array/scalar benchmark in `RESULTS.md`),
jep converts arguments directly against the one known parameter type in
a single pass. jpype always runs a full `matches()` scoring scan then a
separate `convert()`, even with no overload ambiguity, since its
architecture doesn't special-case "only one candidate" — this is also
why jep falls behind on the 16-overload dispatch benchmark in
`RESULTS.md` (all candidates share arity there, so jep's shortcut can't
fire and the more expensive check runs repeatedly).

### Syntax: constructing a proxy

jpype has three ways to make a Python object answer to a Java interface
(a fourth, fully automatic path exists too — see the embedding/
extensibility axis below, since it's tied to interfaces that today live
on the not-yet-merged `origin/reverse` branch):

| Call shape | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Bare callable as a functional-interface argument | Any `lambda`/callable, no wrapping | Not supported — must construct a proxy object explicitly | Any callable, detected via `isInterface` + single-abstract-method check | Any `lambda`/callable, no wrapping |
| Explicit proxy, modern/typed form | `@JImplements(...)` decorator on a Python class (`jpype/_jproxy.py`) | `PyObject.createProxy()` — did not produce a usable object in this checkout | `jep.jproxy()` | `PythonJavaClass` subclass + `@java_method('<jni-signature>')` |
| Explicit proxy, manual/dict form | `JProxy(interfaces, dict=... \| inst=...)` — `@JImplements`'s predecessor, still supported (`_jproxy.py:186-234`) | (same call as above — jpy has one proxy form, not two) | (same call as above — jep has one proxy form, not two) | (same call as above — pyjnius has one proxy form, not two) |

## Axis 2: Conversion & array transfer

How Python values become Java-typed arguments, with the heaviest focus
on numpy array push/pull, since that's where the four libraries differ
most and where the sharpest correctness bug in this whole comparison
(jpy's silent dtype bit-reinterpretation) lives.

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Class hints / custom conversions (`@JConversion`) | Yes (54 tests) | No | No | No |
| Buffer-protocol (numpy) array push, flat 1D | Yes, bulk path, value-correct across ~14 source formats (see dtype matrix below); the dtype coercion runs in Java (`Support.fillFlatFromBuffer`) via a single JNI call | Yes, via a separate argument-matching fast path, not `jpy.array()` itself — no dtype check: same-width dtype mismatches are bit-reinterpreted rather than rejected, see dtype matrix below | Yes, genuine bulk path, closed 8-dtype allowlist (no `float16`) | No, rejected unconditionally (`"Expecting a python list/tuple"`) |
| Buffer-protocol (numpy) array push, multi-dimensional (`int[][]`+) | Yes, bulk path | No (`TypeError`, falls back to a per-element path) | No (`TypeError: Error matching ndarray.dtype...`) | No (same rejection as 1D) |
| Per-class `findJavaConversion` caching w/ invalidation | Yes | N/A — matching is already cheap per call | N/A — short-circuits on arity before any per-arg work | No equivalent per-call cost to cache |

### numpy scalar argument dispatch (`Math.max(int,int)`, i.e. an overloaded method)

| numpy scalar type | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| `numpy.int32` | OK | Fails ("ambiguous Java method call") | OK | Fails ("no matching method") |
| `numpy.int64` | OK | Fails (same) | OK | Fails (same) |
| `numpy.float32` | OK | Fails (same) | Fails ("cannot be interpreted as an integer") | Fails (same) |
| `numpy.float64` | OK | OK (genuine Python `float` subclass) | OK | OK (same reason) |

Verified empirically for all four. jpype resolves every numpy scalar type
against an overloaded method correctly. jep resolves `int32`/`int64`
correctly (`jep_numpy.c` has an explicit numpy-scalar path) but not
`float32`. jpy and pyjnius share the same failure shape (only a genuine
Python `int`/`float` passes their fast dispatch, no numpy-aware fallback
for `int`/`long` params), with differently-worded errors.

### Buffer dtype conversion: the full matrix

Tested every combination empirically, with source read where the
behavior needed explaining.

**jpype's fast-path matrix covers 14 source buffer format codes** —
`?`/`c`/`b` (bool/int8), `B` (uint8), `h`/`H` (int16/uint16), `i`/`l`/`I`/`L`
(int32/uint32), `q`/`Q` (int64/uint64), `f`/`d` (float32/float64), `n`/`N`
(native ssize_t/size_t), and `e` (IEEE 754 half-precision `float16`, via a
dedicated bit-level decoder, `Half<Convert<float>::toX>`, reusing the same
`Convert<float>::toX` machinery every other type shares). Tested all 12
realistic numpy dtypes (`bool`/`int8`/`uint8`/`int16`/`uint16`/`int32`/
`uint32`/`int64`/`uint64`/`float16`/`float32`/`float64`) against all 8 Java
primitive array types (`JArray(JType)(arr)`): 94 of 96 combinations
succeed with genuinely converted values (`float32([1.0,2.0,3.0]) ->
int[]` gives `[1, 2, 3]`, not bit garbage). The 2 failures are
`boolean`/`float*` → `char`, both rejections consistent with Java having
no implicit boolean-to-char or float-to-char narrowing. `float8_e4m3`
(via `ml_dtypes`) has no recognized format code — it still succeeds for
`float`/`double` targets via the general per-element fallback
(`ml_dtypes` scalars support `__float__`), not the buffer fast path.

**jpy's buffer path does not inspect the source dtype.** Its argument
buffer handler (`jpy_jtype.c` ~line 2026) does `memcpy(arrayItems,
pyBuffer->buf, itemCount*itemSize)` after checking only that the byte
length matches — it never reads `pyBuffer->format`. Concretely: passing
`numpy.float32([1.0, 2.0, 3.0])` where `DeepBench.sumIntArray(int[])`
expects `int[]` returns `3217031168` — no error, and not `6` — the raw
IEEE-754 bit patterns of `1.0f`/`2.0f`/`3.0f` reinterpreted as `int32` and
summed (`1065353216 + 1073741824 + 1077936128 = 3217031168`). This
happens specifically when the source dtype and the target's byte width
match but the dtype itself doesn't (`float32`↔`int32`, `float64`↔`int64`);
different-width mismatches are caught by the length check
(`uint8`/`float16` → `int[]` both raise "no matching Java method
overloads found"). Net effect: a wrong-dtype argument at matching byte
width produces a plausible-looking wrong number with no exception raised
— worth flagging distinctly from a rejection, since a rejection is at
least visible at the call site whereas this case is not.

**jep's numpy fast path checks dtype identity against a closed
allowlist** — `float32` → `int[]` and `float16` → `int[]` both cleanly
fail (`"Error matching ndarray.dtype to Java primitive type"`), matching
`convert_pyndarray_jprimitivearray`'s exact-match check against 8
`NPY_*` constants (no `NPY_FLOAT16`, so `float16` is a hard, permanent
gap for jep rather than an untested case). No same-width reinterpretation
risk, since dtype identity is checked before data is touched.

**pyjnius** rejects all numpy input unconditionally regardless of dtype
(established above).

### Non-contiguous buffer sources

A numpy column slice or a transposed array is a valid buffer-protocol
object that can't offer a C-contiguous view.
`project/benchmark/{jpype,jpy,jep,pyjnius}/array_noncontig.py` measures
what happens when one is pushed as a Java array argument, at flat (1D)
and multi-dimensional depths, across all four primitive types.

**jpy fails on the 1D case, for every type and size** (confirmed by
running it): `DeepBench.sumIntArray(non_contiguous_column)` raises
`RuntimeError: no matching Java method overloads found`. Root cause
(`jpy_jtype.c`, `JType_ConvertPyArgToJObjectArg`): jpy's flat-array
buffer-argument path requests the source buffer with `flags =
PyBUF_SIMPLE` — no `PyBUF_ND`/`PyBUF_STRIDES` — so numpy's own
`bf_getbuffer` refuses the request against a non-contiguous array. The
error text that surfaces reads as an overload-resolution problem with
the Java method signature, not a contiguity problem with the input array
— jpy's overload matcher treats the failed buffer request the same as
"argument doesn't match any candidate," so nothing in the message points
at transposing or slicing the array as the fix. jpy's multi-dimensional
buffer argument matching is unaffected — an `int[][]`-or-deeper target
never enters this buffer branch (confirmed by source and by running it;
every transposed multi-dimensional case pushed correctly, at the same
cost as the contiguous case).

jpype handles a non-contiguous source correctly at every depth; the flat
(1D) case reaches the same single-JNI-call bulk path as a contiguous
source — `int[100000]` non-contiguous column slice: 80,715ns, close to
jep's non-contiguous number below (see `project/benchmark/RESULTS.md`
Section 4/8).

**jep handles the flat (1D) case correctly, through its numpy fast
path** (confirmed by running `project/benchmark/jep/array_noncontig.py`
against this branch's harness): a non-contiguous column slice pushes
successfully at every size and type, close to jep's own contiguous
`buffer->array` numbers (`int[100000]`: 75,448ns non-contiguous vs.
~51,150ns contiguous, `array_flat.py`) — jep's numpy fast path doesn't
require contiguity the way jpy's flat buffer-argument path does. jep has
no automatic multi-dimensional numpy push at all (established above), so
the ND/transposed case is instead measured via the same manual per-row
assembly used for jep's ordinary multi-dimensional `buffer->array` push;
this completes successfully at every depth (e.g. `double[][][][][]`
(10^5): 21.5M ns), consistent with jep's contiguous manual-assembly
numbers at the same depth, since each per-row leaf call is itself a
small, independently-contiguous 1D slice.

pyjnius has no `buffer->array` push at any size or depth, contiguous or
not; its `array_noncontig.py` is a stub rather than a benchmark.

### Fast bulk-transfer path coverage

The sections above establish each path individually; laid out as one
matrix, this counts how many of these eight push/pull paths are genuine
bulk operations versus a per-element fallback that still succeeds.
"Genuine" here means what the sections above verified empirically — a
call that succeeds via a per-element/per-row walk is marked as such
rather than counted as a bulk-path pass.

| Path | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| `list->array` push, flat (1D) | Y | Y | ⚠ | Y |
| `list->array` push, multi-dim (rectangular) | Y | Y | ⚠ | Y |
| `buffer->array` push, flat (1D), contiguous | Y | Y\* | Y | N |
| `buffer->array` push, flat (1D), non-contiguous | Y | N | Y | N |
| `buffer->array` push, multi-dim, contiguous | Y | N | N | N |
| `buffer->array` push, multi-dim, non-contiguous (transposed) | Y | N | ⚠ | N |
| `array->buffer` pull, flat (1D) | Y | Y | N | N |
| `array->buffer` pull, multi-dim | Y | Y | N | N |
| **Genuine bulk paths (of 8)** | **8** | **5** | **2** | **2** |

Legend: **Y** = a real, verified bulk path. **N** = no bulk path —
rejected outright, or falls back to a fully general per-element walk
with no dedicated fast-path code (jep's/pyjnius's `array->buffer` rows
return real data but by paying `array->list`'s per-element cost plus a
redundant wrapping step, landing worse than the library's own list-pull
number). **⚠** = a path exists and executes but isn't bulk the way the
others are: jep's `list->array` rows are incomplete at multi-dim depth
(its own `float`/`double` sweeps didn't finish — see the OOM finding
below, so the Y for jep's flat `list->array` row is provisional on
`int`/`long` only), and jep's transposed-ND `buffer->array` push is a
manual, user-written per-row Python walk rather than an automatic
argument-conversion path. \* jpy's flat contiguous `buffer->array` push
is real and bulk, but see the dtype-check gap above — read with that
caveat.

Framed this way: jpype has all eight paths, jpy has five of eight (one
of those five silently wrong on dtype mismatch), and jep and pyjnius
each have two of eight with no caveats. This table is scoped
specifically to bulk numeric array transfer; other conversion/array
gaps (class hints, dtype checking, boxing) are covered elsewhere in this
axis.

### Boxed numeric type selection for a generic `Object`/`Number` argument

A separate conversion question from the array/buffer paths above: when a
plain Python `int` needs to become some boxed Java `Number` type (no
declared target narrower than `Object`/`Number`), what runtime class does
it get?

| | jpype | jpy | jep |
|---|---|---|---|
| Boxing rule | Fixed: always `java.lang.Long`, independent of magnitude (`JPConversionBoxLong::convert`, `native/common/jp_classhints.cpp:1662-1697`); numpy scalar types vary by dtype identity (`int32`→`Integer`, `int16`→`Short`) | Magnitude-dependent: picks `Byte`/`Short`/`Integer`/`Long` by testing whether the value survives a narrowing cast (`JType_CreateJavaNumberFromPythonInt`, `~/devel/jpy/src/main/c/jpy_jtype.c:542-563`) | Declared-type-driven: `pylong_as_jobject` dispatches on the expected Java type via `IsAssignableFrom` checks (Long → Integer → Byte → Short → BigInteger fallback on overflow), not the Python value's magnitude (`src/main/c/Jep/convert_p2j.c:317-368`) |

jpy's rule means the same Python `int` literal boxes to a different Java
runtime class depending purely on its magnitude at a given call:
`foo(5)` boxes as `Byte`, `foo(5000)` as `Short`, `foo(5_000_000)` as
`Integer`, `foo(5_000_000_000)` as `Long`. Since Java-side overload
resolution and `instanceof` both dispatch on the boxed object's runtime
class, `someMethod(x)`'s effective behavior can change based on how
large `x` happens to be, for arguments that are all equally plain Python
`int`s — worth flagging because the boundary is a magnitude threshold
rather than anything visible at the call site. jep's rule is type-stable
like jpype's, just driven by the declared target type rather than a
fixed default.

## Axis 3: Exceptions crossing the language boundary

Two distinct directions get conflated easily, so they're kept separate
here: a **Java exception surfacing in Python** (the common case — Python
code calls a Java method that throws) and, where a library has a
Java-hosts-Python direction at all, a **Python exception surfacing in
Java** (embedded Python code raises, the host JVM code needs to see it).
This document's source reading covered the first direction for jpy,
jpype, and pyjnius, and the second for jep specifically — not every cell
is filled from equally deep verification.

| Direction | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Java exception → Python | Per-class mapping — `except java.lang.NullPointerException` distinguishable from `except java.lang.IllegalArgumentException` | Collapses to a single `RuntimeError` for every Java exception type | Maps a fixed set of common Java exception classes to the matching built-in Python exception type (`IndexError`, `ValueError`, etc.); anything else, including `NullPointerException`, falls back to `RuntimeError` — see below | Collapses to a single `JavaException` for every Java exception type, but with more structured payload than jpy's — see below |
| Python exception → Java | N/A in the scope checked here | N/A — jpy's embeddable direction (`PyLib.startPython()`) wasn't checked for this | Collapses to a single `JepException`, with partial cause-chaining for one specific case (see below) | N/A — no Java-hosts-Python direction |

**Java exception → Python: jpy collapses to one type, no per-type
distinction.** `JPy_HandleJavaException`
(`~/devel/jpy/src/main/c/jpy_module.c:1244-1420`) is jpy's only
Java-to-Python exception path and always ends in
`PyErr_Format(PyExc_RuntimeError, ...)` (lines 1398/1411), regardless of
whether the underlying exception was a `NullPointerException`, an
`IllegalArgumentException`, or an application-defined checked exception.
`getCause()` is walked (line 1394) to splice `"caused by "` text into
that one message (lines 1259-1284) — there is no `__cause__`, no
`__context__`, and no distinct Python exception object per cause. The
detailed message text (the stack-trace walk) only runs when
`JPy_VerboseExceptions` is set (line 1255); otherwise the message is
`error.toString()`. Net effect: `except SomeSpecificException` isn't
available through jpy — every failure is a `RuntimeError` to the caller.
jpype's `JException` (`jpype/_jexception.py`, `@JImplementationFor
("java.lang.Throwable", base=True)`) maps each Java exception class onto
its own Python exception type, mirroring the Java `Throwable` hierarchy,
so `except java.lang.NullPointerException` and `except
java.lang.IllegalArgumentException` are distinguishable, and
`getCause()`/`getMessage()`/`printStackTrace()` remain available as
methods on the exception object.

**Java exception → Python: pyjnius also collapses to one type, but
carries more structured data than jpy's flat string.**
`check_exception` (`jnius_utils.pxi:41-79`) is pyjnius's only
Java-to-Python exception path: it calls `ExceptionOccurred`/`ExceptionClear`,
walks `getMessage()`/`getCause()`/`getStackTrace()` to build a Python
list of trace frames, and raises a single `JavaException` class
(`jnius_export_class.pxi:4`) regardless of the real Java exception type
— `except java.lang.NullPointerException` isn't available, the same
shape as jpy's collapse. Unlike jpy's flat string-concatenated message,
`JavaException` carries the original class name, message, and stack
trace as separate constructor arguments/attributes
(`.classname`/`.innermessage`/`.stacktrace`), always populated rather
than gated behind a verbosity flag — so `except JavaException as e: if
e.classname == 'java.lang.NullPointerException':` is a workable, if
manual, substitute for jpype's/jpy's `except SomeSpecificException` that
neither jpy nor pyjnius offer directly.

**Java exception → Python: jep maps a fixed set of common exceptions to
matching Python built-ins, unlike jpy's and pyjnius's flat collapse.**
`process_java_exception` (`src/main/c/Jep/jep_exceptions.c:413-465`) is
jep's Java-to-Python exception path; it calls
`pyerrtype_from_throwable` (lines 473-527), which checks the Java
exception's runtime type against a fixed list via `IsInstanceOf` and
maps it to the corresponding Python built-in: `ClassNotFoundException` →
`ImportError`, `IndexOutOfBoundsException` → `IndexError`, `IOException`
→ `IOError`, `ClassCastException` → `TypeError`,
`IllegalArgumentException` → `ValueError`, `ArithmeticException` →
`ArithmeticError`, `OutOfMemoryError` → `MemoryError`, `AssertionError`
→ `AssertionError`. Its own comment states the intent directly: "to
enable more precise try: except: blocks in Python for Java exceptions."
Anything not on that list — including `NullPointerException`, the
example used throughout this axis — falls through to the same
`RuntimeError` default jpy and pyjnius use for everything. The raised
Python exception also isn't a plain string message: `jpyExc =
jobject_As_PyObject(env, exception)` wraps the live Java `Throwable`
object itself and passes it via `PyErr_SetObject(pyExceptionType,
jpyExc)`, so the caught Python exception carries the actual wrapped Java
object — `getMessage()`/`getCause()` remain callable on it, closer to
jpype's model than to jpy's/pyjnius's flattened text, just gated behind
a much narrower set of distinguishable Python exception *types* than
jpype's per-class mapping offers.

**Python exception → Java: jep collapses to one type too, with one
partial exception.** `process_py_exception`
(`src/main/c/Jep/jep_exceptions.c:42-175`) is jep's only
Python-to-Java exception path; every Python exception becomes a
`JepException(String, long)` built from `"ExcType: message"` string
concatenation (lines 148-163), not a distinct Java exception class per
Python exception type — `catch SomeSpecificPythonException` isn't
available on the Java side. One case is handled more precisely: if the
Python exception is itself wrapping a Java exception that crossed into
Python and back (a `PyJObject`-backed exception), jep preserves that as
a real `Throwable` cause via a second constructor,
`JepException(String, Throwable)` (lines 162-167) — cause-chaining for
that specific round-trip case, not for exceptions that originate
natively in Python.

### Syntax: catching a specific failure

| | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Catch a specific Java exception in Python | `except java.lang.NullPointerException:` | `except RuntimeError:` (only option — no per-type distinction) | `except IndexError:`/`except ValueError:`/etc. for the fixed set of mapped exceptions (see above); `except RuntimeError:` for everything else, including `NullPointerException` | `except JavaException as e:`, then inspect `e.classname` — no per-type distinction at the `except` clause itself, but structured data is available inside the handler |
| Catch a specific Python exception in Java | N/A in the scope checked here | N/A | `catch (JepException e)` (only option — no per-type distinction, except the one wrapped-Java-exception case above) | N/A |

## Axis 4: Threading & GIL discipline

Two separate concerns: how a thread that crosses into Java (or Python)
gets registered and released, and how the GIL is acquired/released at
each native entry point so concurrent calls from arbitrary threads don't
corrupt interpreter state. Internal mechanism more than user-facing API
— there's a small syntax table at the end for the one place a library
exposes attach/detach as a call a user can make directly.

| Concern | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Thread attachment for calls crossing into Java | Daemon (`AttachCurrentThreadAsDaemon`), with explicit `attach()`/`attachAsDaemon()`/`detach()` API | Non-daemon (`AttachCurrentThread`), no detach call anywhere in source | Daemon (`AttachCurrentThreadAsDaemon`), with a comment explaining why: no hooks exist to detach later | Non-daemon (plain `AttachCurrentThread`), but with an explicit `jnius.detach()` call exposed — the one place pyjnius offers something jpy doesn't |
| GIL acquisition at native entry points | `PyGILState_Ensure`/`Release`, `PyGILState_Check()` for subinterpreter reliability | `PyGILState_Ensure`/`Release`, ~35 call sites, rejects a call made mid-shutdown | `PyEval_AcquireThread`/`ReleaseThread` against a per-thread cached `PyThreadState` — different API family, same underlying discipline | Cython's `with gil` clause on the proxy-callback entry point — compiler-generated `PyGILState_Ensure`/`Release`, not hand-written, but the same underlying primitive |
| Shutdown-race guard on a callback still in flight | `isRunning()`/`is_shutting_down` check (`jp_proxy.cpp:161`), on top of `DestroyJavaVM`'s own JNI-mandated block on non-daemon threads | `Py_IsFinalizing()` check, one direction only, self-acknowledged racy (TOCTOU) | None found in the proxy-invocation path, either direction | None found in the proxy-callback path — matches jep's gap, not jpy's/jpype's guard |
| Adversarial concurrency test (many threads, shared mutable interpreter state, relying on automatic locking) | `GilConcurrencyParityNGTest` — passes | `MultiThreadedEvalTestFixture` — passes | jep's own tests pass, but exercise a narrower scenario — see below | Written for this document, no pre-existing test found in pyjnius's own suite — passes, see below |

**jpy: threads that call from Python into Java are attached as
non-daemon, and never detached.** `JPy_GetJNIEnv`
(`jpy_module.c:267-298`) calls plain `AttachCurrentThread` (line 281),
not `AttachCurrentThreadAsDaemon`, on `JNI_EDETACHED`. There is no
`DetachCurrentThread` call anywhere in jpy's C or Java source (confirmed
by grep across `src/main/c/*.c`). Two consequences follow: every Python
thread that calls a Java method keeps its JVM-side thread registration
for the thread's whole life, with no jpy API to release it; and because
the attach is non-daemon, `DestroyJavaVM` (JNI-mandated to block until
all non-daemon threads exit) will wait on any such thread that's still
alive but idle — a Python thread that made one Java call and is now
sitting in `time.sleep()` can hold up JVM shutdown, for a reason not
visible from the call site that triggered the attach. jpype also
auto-attaches, but as a daemon, with the tradeoff named in its own API:
`JPContext::getEnv()` (`native/common/jp_context.cpp:870-894`) attaches
via `AttachCurrentThreadAsDaemon` "so that the newly attached thread does
not deadlock the shutdown" (comment, line 885-886), and
`java.lang.Thread.isAttached()`/`.attach()`/`.attachAsDaemon()`/
`.detach()` (`jpype/_jthread.py:22-84`) let a long-running thread detach
explicitly.

**jpy: `DestroyJavaVM` has no guard against a daemon thread mid-callback
into Python.** `JPy_destroy_jvm` (`jpy_module.c:510-521`) calls
`DestroyJavaVM()` with no `isRunning()`/shutting-down check anywhere in
jpy's proxy-invocation path, in either direction. jpy's only
shutdown-race guard is `Py_IsFinalizing()` (`org_jpy_PyLib.c:57-86`),
checked at the top of every native entry point for the opposite
direction (a Java thread calling into Python while Python is
finalizing) — and its own comment states it "doesn't completely prevent
the race condition (TOCTOU), but... mitigates the risk significantly."
jpype's model (`[[jvm_shutdown_daemon_thread_safety]]`) relies on
`DestroyJavaVM`'s own JNI-mandated block on non-daemon threads for the
general case (`jp_context.cpp:97-101`: "VM_Exit parks all remaining
daemon threads at the final safepoint; nothing executes Java code after
DestroyJavaVM returns"), plus one targeted check
(`jp_proxy.cpp:161`, `context->isRunning() || ...is_shutting_down`) for
the one gap that guarantee doesn't cover — a daemon-thread proxy
callback still parked when shutdown completes underneath it.

**jpy: GIL acquisition itself matches jpype's discipline.** Separate
from the attachment/shutdown gaps above: `PyGILState_Ensure`/`Release`
around every JNI entry point, ~35 call sites, rejecting a Python call
made mid-interpreter-shutdown rather than racing it
(`~/devel/jpy/src/main/c/jni/org_jpy_PyLib.c:65-86` and e.g. lines 284,
407, 501, 881). jpype uses the same primitive with the same discipline:
`PyGILState_Ensure`/`Release` around Python calls from Java threads,
including the reentrant case (`PyGILState_LOCKED` → release is a
documented no-op) and `PyGILState_Check()`, chosen because it stays
reliable across subinterpreters (`native/common/jp_bridge.cpp:280-390,601-624`,
`native/python/jp_pythontypes.cpp:407-476`). The non-daemon
thread-attachment gap above is a separate point in the same subsystem —
this GIL-discipline match holds independently of it. Put together with
the conversion axis's boxing/dtype findings, the shape across this
document is consistent: jpy's C-side value-conversion logic (boxing,
buffer dtype checks) has the real gaps; its Java-side threading and
lifetime engineering (GIL discipline, the reachability fence in the
next axis) does not.

**jep: the proxy-invocation path has no liveness check at shutdown.**
`jep.python.InvocationHandler.invoke()`
(`src/main/java/jep/python/InvocationHandler.java:132-141`) calls
straight into native code with no liveness check, and the native side,
`Java_jep_python_InvocationHandler_invoke`
(`src/main/c/Jep/python/invocationhandler.c`), has no
`Py_IsFinalizing()`/interpreter-liveness check anywhere in the file. A
Java thread mid-callback into a Python-implemented proxy
(`jep.jproxy()`) when the interpreter is closing has no equivalent of
jpype's `jp_proxy.cpp:161` `isRunning()`/`is_shutting_down` guard, and
not even jpy's TOCTOU-checked `Py_IsFinalizing()` on the other
direction — this is the one place neither jpy's nor jpype's guard has a
counterpart in jep.

**jep: thread attachment matches jpype's daemon approach.**
`AttachCurrentThreadAsDaemon`, with a comment reasoning through why:
"there are no hooks to detach the thread later[, so] daemon is the only
way to let the process exit normally" (`src/main/c/Jep/pyembed.c:817-834`)
— jep's authors designed around the hazard jpy's plain-`AttachCurrentThread`
-without-detach creates, rather than hitting it and patching later. This
is one of three places jep's design independently lands on the same
choice as jpype rather than jpy's: type-stable boxing in the conversion
axis, and real (not faked) sub-interpreter shutdown in the
interpreter-lifecycle axis are the other two.

**jep: GIL discipline uses a different mechanism, no shutdown-race guard
found.** jep acquires/releases via `PyEval_AcquireThread(jepThread->tstate)`/
`PyEval_ReleaseThread` against a per-`JepThread`-cached `PyThreadState`
(12+ call sites in `pyembed.c`), not the `PyGILState_*` TLS API jpy and
jpype both use — a choice consistent with jep predating PEP 684's
sub-interpreter story. No shutdown-race guard comparable to jpy's
`Py_IsFinalizing()` check was found in this path.

**jep: its own adversarial concurrency tests pass, for a narrower claim
than jpype/jpy's.** Built jep's existing native lib
(`build/lib.linux-x86_64-cpython-312`) and ran its two adversarial
multithreading tests directly: `jep.test.synchronization.TestCrossLangSync`
(16 Python-sub-interpreter threads + 16 Java threads on one shared
lock/`AtomicInteger` via `obj.synchronized()`) and
`jep.test.TestSharedModulesThreads` (16 threads concurrently creating
`SubInterpreter`s and importing the same shared module). Both exited 0.
Neither is the same claim as `GilConcurrencyParityNGTest`
(`native/jpype_module/src/test/java/org/jpype/GilConcurrencyParityNGTest.java`)
or jpy's `MultiThreadedEvalTestFixture` test: N uncoordinated threads
mutating one shared interpreter's globals with no explicit lock, relying
on the automatic per-call GIL guard for correctness. jep has no
construct for that scenario — `SharedInterpreter`'s javadoc states each
instance "still maintains distinct global variables" even though
modules are shared, and mixing `Interpreter` instances on the same
thread at the same time is unsupported (`SharedInterpreter.java:34-44`);
`MainInterpreter` is bootstrap machinery for GIL-deadlock avoidance, not
something application code runs against directly. jep's concurrency
safety here comes from architecturally not sharing mutable interpreter
state across threads, rather than from a guard proven safe under shared
mutable state the way jpype's is
(`[[jvm_shutdown_daemon_thread_safety]]`-adjacent).

**pyjnius: thread attachment is non-daemon like jpy's, but with an
explicit release call jpy lacks.** `get_jnienv()`
(`jnius_env.pxi:9-22`), the function nearly every native call site in
pyjnius goes through, calls plain `AttachCurrentThread` (line 21), not
`AttachCurrentThreadAsDaemon` — the same non-daemon hazard as jpy's,
above. The function's own comment names a related, narrower leak too:
`# XXX if threads are created from C (not java), we'll leak here.`
Unlike jpy, though, pyjnius does expose a release call: `detach()`
(`jnius_env.pxi:25-26`, calling `DetachCurrentThread` directly) is in
the module's public `__all__` (`jnius.pyx:91`) as `jnius.detach()` — a
user can call it manually, which jpy offers no equivalent of, though
nothing calls it automatically the way jpype's daemon-by-default
attachment sidesteps needing to.

**pyjnius: the proxy-callback entry point acquires the GIL via Cython's
`with gil`, with no shutdown-race guard.** `invoke0`/`py_invoke0`
(`jnius_proxy.pxi:79-157`), the JNI-registered native method a Java
thread calls into when invoking a Python-implemented proxy
(`PythonJavaClass`), are declared `with gil` in Cython — the compiler
generates the `PyGILState_Ensure`/`Release` pair automatically around
the function body, the same primitive jpy and jpype use by hand, just
not hand-written here. No `Py_IsFinalizing()`-style check or equivalent
of jpype's `is_shutting_down` guard was found anywhere in this path,
either function — the same gap as jep's proxy-invocation path above, not
jpy's (racy but present) or jpype's (targeted, non-racy) protection.

**pyjnius: no pre-existing adversarial concurrency test was found in its
own suite, so one was written for this document, matching jpype's/jpy's
scenario rather than jep's narrower one.** 16 Java threads, each running
a `PythonJavaClass` `Runnable` proxy that increments one shared,
unlocked Python `dict` value 200 times, running concurrently with 16
Python threads independently constructing `java.lang.Integer` objects —
no lock anywhere around the shared counter, relying entirely on the
`with gil` discipline confirmed above. Ran cleanly three times in a
disposable venv (`python3.12`, matching the pre-built extension's ABI):
final counter value exactly matched the expected total (3200) every
time, no lost updates, no crash. One dead end worth naming so it isn't
mistaken for a finding: an earlier draft passed the raw
`PythonJavaClass` proxy object directly to `Thread(runnable)` instead of
`Thread(cast('java/lang/Runnable', runnable.j_self))` — pyjnius's
overload matching didn't reject this, it silently matched `Thread`'s
zero-argument constructor instead, so `run()` was never invoked at all
(counter stayed at 0) and, in one run, that malformed setup crashed the
JVM with a `SIGSEGV` during the JVM's own secondary error reporting.
That crash didn't reproduce with the corrected test across three runs,
and the corrected test is the one that actually exercises the proxy
callback path this row is about — recorded here as a caution about
`autoclass`/proxy argument typing, not as a concurrency defect.

### Syntax: explicit attach/detach

| | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Explicit thread attach/detach call | `java.lang.Thread.attach()` / `.attachAsDaemon()` / `.detach()` (`jpype/_jthread.py:22-84`) | None — attachment is automatic and permanent, no release call | None — attachment is automatic (daemon), no release call needed since it's daemon | `jnius.detach()` (`jnius_env.pxi:25-26`) — attachment itself is automatic and non-daemon (see above), but a release call exists, unlike jpy |

## Axis 5: Native object lifetime / GC

How each library prevents a native handle from being freed or reused
while a JNI call is still using it — and, on the Java side, how a
wrapped Python object's lifetime is managed. Internal mechanism, not a
user-facing call (a user's one lifetime-relevant action, closing a
resource with `with obj:`, is covered under `AutoCloseable` in the
object-model axis) — no syntax table here.

| Library | Mechanism | Model |
|---|---|---|
| jpype | `org.jpype.ref.NativeReference`, a `PhantomReference` that copies the native handle onto itself at construction (`hostReference` field) | A `PhantomReference` is only enqueued once the referent is already proven unreachable, so there's no live-object race to fence against in the first place — no explicit fence needed by construction |
| jpy | `Reference.reachabilityFence(this)` (`PyObject.java:33-40`) | Reads the native pointer off the live wrapper object, then relies on an explicit, manually-placed fence to stop the JIT from deciding the wrapper is dead early and letting GC collect it mid-call |
| jep | Manual only — no `PhantomReference`/`Cleaner`/`reachabilityFence` anywhere in `jep.python.PyObject` | Cleanup exclusively via explicit `close()`; a `PyObject` becomes invalid once its owning interpreter closes (per its own javadoc) |
| pyjnius | `LocalRef`, a Cython extension type whose `__dealloc__` calls `DeleteGlobalRef` (`jnius_localref.pxi:1-18`) | Despite the name, holds a Java *global* ref (`NewGlobalRef` in `create()`) released synchronously when CPython's own refcounting drops the Python wrapper to zero — no phantom reference, no explicit fence |

jpy's and jpype's mechanisms reach the same outcome — no use-after-free
of the native handle — two different ways: jpy adds an explicit guard
against a hazard its design creates (reading the pointer off a live,
GC-reachable object); jpype's design doesn't create that hazard shape to
begin with (the phantom reference only exists once GC has already
proven the object unreachable). jep's manual-`close()` model sidesteps
jpy's live-pointer race entirely — there's no GC-triggered decref racing
a JNI call, because there's no GC-triggered decref at all — but trades
it for a pure manual-lifetime contract: forgetting to call `close()`
leaks the native object until its owning sub-interpreter tears down. A
third design point, distinct from both jpy's fence-guarded live-pointer
model and jpype's phantom-reference model, rather than a strictly better
or worse one — leak-on-`close()`-omission versus a fencing discipline
that has to be applied correctly at every call site are different
failure modes, not directly ranked here.

pyjnius's model is a fourth point again, and structurally simpler than
any of the three above for a specific reason: CPython's reference
counting is synchronous and deterministic, not a concurrent collector
the way the JVM's is, so a `LocalRef` held by an active Python call
frame can't be dealloc'd out from under that call the way jpy's fence
guards against — there's no equivalent of the JIT deciding a wrapper is
provably dead early. That's a narrower version of jpype's own reasoning
(no race by construction) reached for a different underlying reason
(deterministic refcounting vs. phantom-reference timing), not the same
mechanism wearing a different name. It doesn't, by itself, say anything
about whether the *Java*-side global ref could be freed while some other
Java thread is still using it — that direction wasn't checked here.

## Axis 6: Interpreter lifecycle

Restart semantics, real subinterpreter isolation vs. a cheaper
shared-globals construct, and whether a Java-side handle to a Python
object is checked against the interpreter that actually produced it.

| Concern | jpype | jpy | jep |
|---|---|---|---|
| "Restart" the interpreter | No restart primitive; independently disposable subinterpreters instead (`Py_NewInterpreterFromConfig`/`Py_EndInterpreter`, `org.jpype.SubInterpreter`) | `stopPython()`/`startPython()` exists, but a real second stop after restart "currently causes a fatal error" per jpy's own javadoc; the test suite's own Maven config sets a flag that turns "stop" into a no-op rather than exercise this | No restart primitive found; real subinterpreter isolation (`Py_EndInterpreter`) same as jpype |
| Real (own-GIL) subinterpreter isolation | Yes — `SubInterpreter` | No — no `Py_NewInterpreter`/`Py_EndInterpreter` reference anywhere in jpy's C or Java source; jpy has no subinterpreter concept at all | Yes — `jep.SubInterpreter`, confirmed via `Py_EndInterpreter` at close |
| Cheap same-interpreter, separate-globals construct | Yes — `Script` (one interpreter, N independent globals dicts, any of them anonymous) | Partial — `PyModule` wraps a real, `sys.modules`-registered Python module (`PyModule.importModule(name)`/`PyModule.getMain()`), not an arbitrary anonymous globals dict; usable as a separate scope only if backed by an actual importable module | Yes — `SharedInterpreter`, though named as a sibling of `SubInterpreter` in the same `Interpreter` API family rather than visibly separate |
| Check that a Java-side object handle is used by the interpreter that created it | Yes — `proxy->m_Context != context` raises `RuntimeError` rather than touching the pointer (`JPClass::convertToPythonObject`, `native/common/jp_class.cpp:379-403`) | N/A — with no subinterpreter isolation at all, there's no separate arena boundary this kind of check would guard | No — documented as a caller contract in javadoc, not enforced in code (see below) |

**jpy: restarting the interpreter is disabled by default in jpy's own
test config.** `PyLib.stopPython()`'s own javadoc
(`~/devel/jpy/src/main/java/org/jpy/PyLib.java:243-259`) states that
stopping the interpreter again after a restart "currently causes a fatal
error in the Java Runtime Environment," linking jpy's own
[issue #70](https://github.com/bcdev/jpy/issues/70). `STOP_IS_NO_OP`
(`PyLib.java:57`, `Boolean.getBoolean("jpy.stopIsNoOp") ||
ON_WINDOWS`) makes `stopPython()` skip `Py_Finalize` when set, so "stop"
becomes a no-op that leaves the interpreter alive — no module teardown,
no `sys.modules` clear. jpy's own `setup.py:226-232` (`test_maven`) sets
`-Djpy.stopIsNoOp=true` for every Maven test run, because, per its own
comment, multiple start/stop cycles in the same JVM crash CPython. The
one test that exercises a real stop/start/stop cycle,
`LifeCycleTest.testCanStartAndStopWithoutException`, self-skips under
that flag (`Assume.assumeFalse(..., "jpy.stopIsNoOp")`) — it doesn't run
under the config the suite's own Maven run uses. jpype doesn't offer
interpreter restart either, but takes a different approach to the same
underlying constraint: it exposes PEP 684 subinterpreters
(`Py_NewInterpreterFromConfig`/`Py_EndInterpreter`,
`native/common/jp_bridge.cpp:454-560`) as independently disposable
instances (`org.jpype.SubInterpreter`) rather than a single root
interpreter meant to be torn down and revived.

**jpy: no subinterpreter concept at all; its nearest thing to a separate
scope is a real Python module, not an anonymous globals dict.**
Confirmed by grep: no `Py_NewInterpreter`/`Py_EndInterpreter` reference
anywhere in jpy's C or Java source, so there is no own-GIL isolation to
compare against jpype's/jep's `SubInterpreter` at all — the ownership
check jpype has for that scenario (`proxy->m_Context != context`) has
nothing to guard in jpy, not because jpy solved the problem but because
the problem's precondition doesn't exist there. jpy's `PyModule`
(`~/devel/jpy/src/main/java/org/jpy/PyModule.java`) is the closest thing
to jpype's `Script`/jep's `SharedInterpreter` — `PyModule.getMain()`
returns a Java handle to the interpreter's real `__main__` module, and
`PyModule.importModule(name)` returns a handle to any other real,
`sys.modules`-registered module — but every one of these is a genuine
named Python module, not an arbitrary anonymous globals dict created on
demand the way `Script`/`SharedInterpreter` are. Getting a second
"separate scope" in jpy means importing a second real module (or
constructing one via `CreateModule.java`), not calling a constructor
with no name.

**jep: sub-interpreter shutdown is real, matching jpype's shape.**
`pyembed_thread_close` calls a genuine `Py_EndInterpreter(jepThread->tstate)`
when closing a non-main interpreter thread — no `stopIsNoOp`-style flag
anywhere in the source (`src/main/c/Jep/pyembed.c:794-805`). jep's
multi-`Jep`-instance isolation claim holds up architecturally, matching
the honest shape of jpype's `SubInterpreter.close()`, not a no-op flag
papering over a crash the way jpy's does.

**jep: no check that a `PyObject` is being touched by the interpreter
that created it.** jpype's guard for this
(`JPClass::convertToPythonObject`, `native/common/jp_class.cpp:379-403`)
exists because own-GIL subinterpreters have separate allocators/arenas —
handing one interpreter's `PyObject*` to another's Python code is memory
corruption, so `proxy->m_Context != context` is checked explicitly and
raises a `RuntimeError` rather than touching the pointer. jep documents
the same constraint without enforcing it: `jep.python.PyObject`'s
javadoc states "This class is not thread safe and PyObjects can only be
used on the Thread where they were created. When an Interpreter instance
is closed all PyObjects from that instance will be invalid"
(`src/main/java/jep/python/PyObject.java:36-38`). Tracing the call path:
`PyObject.tstate()` → `MemoryManager.getThreadState()` →
`getThreadLocalJep()` (`src/main/java/jep/python/MemoryManager.java:124-134`)
looks up whatever `Jep` instance is bound to the calling thread via a
plain `ThreadLocal<Jep>`, throwing only if none is bound
("`Invalid thread access.`") — it doesn't check that this specific
`PyObject`'s originating interpreter matches. That `tstate` is then
passed into native code together with the object's raw pointer
(`Java_jep_python_PyObject_getAttr` and five sibling functions,
`src/main/c/Jep/python/jep_object.c:36-278`) with no ownership check.
The general shape of the hazard: a Java-side collection that outlives
the interpreter call which produced the stored `PyObject`/proxy is a
potential site for this, not only a cross-thread one.

Two reproduction attempts on the same thread: (1) opening a second
`SharedInterpreter` while a first is still open is blocked outright —
`JepException: "Unsafe reuse of thread main for another Python
Interpreter. Please close() the previous Interpreter to ensure
stability"` — a real guard, though incidental rather than an ownership
check. (2) Closing the first interpreter, then opening a second on the
freed thread and touching the first's stashed object returned a
correct-looking result rather than crashing; the reason is that
`SharedInterpreter` instances aren't separate CPython-level
subinterpreters at all (see below). A genuine own-GIL
`SubInterpreter`-vs-`SubInterpreter` reproduction — the case that would
actually exercise this gap, since each `new SubInterpreter()` normally
gets its own `MemoryManager` — wasn't attempted. So: the structural gap
is confirmed by source (no `proxy->m_Context != context`-style check
anywhere in jep), but the two concrete repro attempts tried here didn't
produce a live crash, for reasons specific to which jep class each one
exercised — an open question rather than a demonstrated crash.

**jep: `SharedInterpreter` and jpype's `Script` are the same underlying
construct under different names.** Checked directly: opening a
`SharedInterpreter` does `globals = PyDict_New();
PyDict_SetItemString(globals, "__builtins__", ...)`
(`src/main/c/Jep/pyembed.c:766-768`) — a fresh Python dict, not
`Py_NewInterpreter`/`Py_NewInterpreterFromConfig` anywhere in that path.
Every `SharedInterpreter` instance runs in the same underlying CPython
interpreter, one GIL, one `sys.modules`, with only its own `globals`
dict — the same pattern as jpype's `Script` (`org.jpype.Script`, "a
scope of variables in the Python interpreter... housed in Java space"),
distinct from `jep.SubInterpreter` or jpype's own `SubInterpreter` (both
`Py_NewInterpreter`-backed isolation). jep gives this shared-globals
pattern its own class in a naming family that otherwise reads as
"isolated interpreter" (`SubInterpreter`/`SharedInterpreter` share the
`Interpreter` API), where jpype keeps the always-cheap, never-isolated
version (`Script`) visibly separate from the opt-in real-isolation one
(`SubInterpreter`).

### Syntax: real isolation vs. shared-globals

| | jpype | jpy | jep |
|---|---|---|---|
| Real, own-GIL subinterpreter | `SubInterpreter()` | Not offered — no subinterpreter concept exists | `jep.SubInterpreter()` |
| Shared-interpreter, separate-globals only | `Script()` — anonymous, no name required | `PyModule.importModule("some.real.module")` / `PyModule.getMain()` — must be a real, named module | `jep.SharedInterpreter()` |
| Stop/restart the (single) root interpreter | Not offered — use `SubInterpreter` instead | `PyLib.stopPython()` / `startPython()` — see the restart caveat above | Not offered — `MainInterpreter.close()` (`MainInterpreter.java:230-235`) just interrupts a background thread, doesn't finalize the interpreter; `setInitParams()` explicitly throws if called after the first `Interpreter` is created, implying the root config is fixed for the process |

## Axis 7: Embedding & discovery (Java hosts Python)

The reverse direction from everything above: a pure Java application
bringing up an embedded Python interpreter itself, with Java as the host
process. jep's architecture *is* this, natively. jpy has a secondary
entry point for it that turns out to be more built-out than "just
start/stop" — it has its own JSR-223 implementation, checked below.
pyjnius has neither. jpype's version of this exists on `origin/reverse`
— substantial, but **not yet merged into `review` (the main branch)**,
so everything in this axis about jpype is a statement about that
branch, not about jpype's current shipped behavior; treat it
accordingly.

| Concern | jpype (`origin/reverse`) | jpy | jep | pyjnius |
|---|---|---|---|---|
| Is Java-hosts-Python the library's native/primary direction? | No — bolted onto jpype's existing Python-hosts-Java architecture, on an unmerged branch | No — secondary capability alongside its usual Python-hosts-Java mode | Yes — this is jep's native architecture | N/A — no Java-hosts-Python direction found |
| Standard JVM scripting API (JSR-223) | Yes — `org.jpype.script.JPypeScriptEngine` | Yes — `org.jpy.jsr223.ScriptEngineImpl`, same `AbstractScriptEngine`/`Invocable` interfaces jpype's implements | No — no `javax.script` reference anywhere in `~/devel/jep/src/main/java/jep/` | N/A |
| Context/session object (multiple independent scopes against one interpreter) | Yes — `org.jpype.Script`, anonymous globals dict per instance | Partial — `PyModule`, but scoped to a real, named Python module rather than an arbitrary anonymous scope (see the interpreter-lifecycle axis) | Yes, but not named as a distinct concept from real isolation — see the interpreter-lifecycle axis | N/A |
| Typed object library mirroring Python's builtin types | Yes — `python.lang`, 54 files | No — three wrapper classes total (`PyModule`/`PyDictWrapper`/`PyListWrapper`), no broader typed hierarchy | No — one generic `jep.python.PyObject` catch-all plus a closed, hardcoded conversion chain (see the extensibility axis for the proxy-selection version of this gap) | N/A |

**jpype (`origin/reverse`): a Java-side entry point with no Python
process involved in starting anything.** `org.jpype.MainInterpreter`
(`native/jpype_module/src/main/java/org/jpype/MainInterpreter.java`) is
a Java-side singleton that locates/probes/launches an embedded CPython
interpreter from Java code — the same shape as jep's niche. Demonstrated
by `native/jpype_module/src/test/java/runner/HelloWorldMain.java`, a
pure `public static void main(String[] args)` with no Python involvement
in bootstrapping:

```java
public static void main(String[] args) {
    MainInterpreter.getInstance().start(new String[0]);
    Script context = new Script(MainInterpreter.getInstance());
    context.exec("msg = 'Hello World from Python'");
    PyObject msg = context.eval("msg");
    ...
}
```

`origin/reverse` exposes three separate embedding layers:

1. **JSR-223** (`org.jpype.script.JPypeScriptEngine`) — implements
   `javax.script.AbstractScriptEngine`/`Invocable`, the standard scripting
   API every JVM already has a pluggable-scripting-language story for
   (`ScriptEngineManager`) — not jpype-specific.
2. **Context** (`org.jpype.Script`, built on `MainInterpreter`) — its own
   javadoc: "a scope of variables in the Python interpreter... we can
   consider these to be modules housed in Java space." Each `Script`
   instance owns its own globals/locals `PyDict`, exposes `eval()`/
   `exec()`/`importModule()`, and multiple `Script` instances can coexist
   against one shared interpreter — the same pattern other embedding
   systems call a Context/session object (e.g. GraalVM's `Context`).
3. **`python.lang`** — a 54-file typed Java class library mirroring
   Python's builtin type system: `PyObject`, `PyDict`, `PyList`,
   `PySet`/`PyFrozenSet`, `PyInt`/`PyFloat`/`PyComplex`,
   `PyString`/`PyBytes`/`PyByteArray`, `PyGenerator`/`PyCoroutine`/
   `PyAwaitable`, `PyCallable`, iterators for all of it. Its
   `package-info.java` states the design intent: implement Java
   collection interfaces where they don't conflict with Python
   semantics, tight return types, loose parameter types, fall back to
   `eval()` only when a wrapper can't express something. (How Python
   objects get mapped onto these `python.lang` types automatically is a
   proxy-selection mechanism covered in the extensibility axis below,
   since it's the same machinery as jpype's user-extensible SPI.)

jep has no JSR-223 `ScriptEngine` implementation (checked
`~/devel/jep/src/main/java/jep/` — no `javax.script` reference), and its
public embedding surface is essentially one class (`Jep`, with
`eval`/`exec`/`getValue`/`set`) plus its `pyj*` wrapper types, not three
separated layers.

jpy also has a Java-hosts-Python entry point, independent of jep's:
`org.jpy.PyLib.startPython()`/`stopPython()`/`isPythonRunning()` lets a
pure Java application embed and control a Python interpreter directly,
demonstrated by a JUnit test with no Python bootstrap
(`src/test/java/org/jpy/EmbeddableTestJunit.java` ->
`EmbeddableTest`'s `PyLibControl` inner class, `~/devel/jpy`).

**jpy's entry point is more than bare start/stop — it has a real JSR-223
implementation too.** `org.jpy.jsr223.ScriptEngineImpl`
(`~/devel/jpy/src/main/java/org/jpy/jsr223/ScriptEngineImpl.java`)
extends `AbstractScriptEngine` and implements `Invocable`, the same two
interfaces jpype's `JPypeScriptEngine` implements on `origin/reverse` —
`eval()` runs a script against the engine's bindings via
`PyObject.executeCode()`, `invokeFunction()`/`invokeMethod()` call named
Python functions/methods (`PyModule.getMain().call(name, args)` for the
top-level case), and `getInterface()` returns a Java proxy backed by
compiled Python functions via `PyModule.getMain().createProxy(clasz)` /
`PyObject.createProxy(clasz)`. That last path leans on the same
`PyObject.createProxy()` machinery this document's object-model axis
found didn't produce a usable object in this checkout — so jpy's
`getInterface()` is architecturally present but inherits that same
proxy defect, worth flagging rather than counting as a clean win. jpy
has no `python.lang`-style typed builtin-type library to go with its
JSR-223 support — `PyModule`/`PyDictWrapper`/`PyListWrapper` remain the
full extent of its typed wrappers, the same three classes noted
elsewhere in this document.

So: jep's architecture is natively Java-hosts-Python (its primary
direction); jpy has it as a secondary capability alongside its usual
Python-hosts-Java mode, and that capability is more built-out than a
first look suggests — real JSR-223, not just raw start/stop; pyjnius has
neither (checked its Java sources specifically — only test fixtures and
the `PythonJavaClass` proxy-callback machinery, no embeddable launcher).
If `origin/reverse` merges, jpype would cover both embedding directions
— the ground jep and jpy each independently cover on the
Java-hosts-Python side, plus jpype's existing Python-hosts-Java surface,
and with a broader `python.lang` typed-object story than jpy's JSR-223
alone provides — while pyjnius remains the only one of the four with
just one direction.

### Launching embedded Python from Java: three discovery models

Before any of the API surface above can run, something has to find the
right `python`/`libpython` on disk and load it into the JVM process.
pyjnius doesn't have this problem (no Java-hosts-Python direction).
jpype, jpy, and jep each answer it differently.

**jpy: static, ahead-of-time, no runtime discovery.** `PyLibConfig`'s
static initializer
(`~/devel/jpy/src/main/java/org/jpy/PyLibConfig.java:53-71`) only reads a
`jpyconfig.properties` file — from the classpath, `-Djpy.config=<path>`,
or the current working directory — and copies its keys into `System`
properties. That file is written once, ahead of time, by a separate
Python-side step (`jpyutil.write_config()`, run manually during setup).
`PyLib.loadLib()` reads `jpy.jpyLib`/`jpy.pythonLib` out of that config
via `getProperty(key, mustHave=true)` (`PyLib.java:521-546`) and calls
`System.load()` directly — no search, no fallback. If the config is
missing or stale (Python reinstalled, venv moved, wheel rebuilt), the
failure is immediate: `RuntimeException("missing configuration property
'jpy.jpyLib'")` (`PyLibConfig.java:120-122`). The discovery problem is
pushed onto the user/build system, once, before Java runs.

**jep: runtime discovery, by re-deriving Python's own search path in
Java.** `MainInterpreter.initialize()` first tries the conventional
`System.loadLibrary("jep")` (whatever `-Djava.library.path` already
points at); only on `UnsatisfiedLinkError` does it fall back to
`LibraryLocator.findJepLibrary()`
(`~/devel/jep/src/main/java/jep/MainInterpreter.java:124-135`). That
locator walks `PYTHONPATH` (`searchPythonPath`), then reimplements
CPython's own `site.py`-`getsitepackages()` layout against
`PYTHONHOME`/`VIRTUAL_ENV` (`searchSitePackages` — `lib`/`lib64`/`Lib`,
`site-packages`, `site-python`, versioned `pythonX.Y/site-packages`),
then user-site locations per PEP 370 across all three OS conventions
(`searchUserSitePackages` — `~/.local/lib/pythonX.Y/site-packages`,
Windows `%APPDATA%/Python`, macOS `~/Library/Python/X.Y`) —
`LibraryLocator.java:100-227`. A narrow self-healing step: if
`libjep`'s `System.load()` fails because a specific `libpython*.so`
isn't found, it regex-parses the missing library name out of the
`UnsatisfiedLinkError` message and searches `PYTHONHOME` for it
(`findPythonLibrary`, `LibraryLocator.java:253-292`). All of this is a
Java-side mirror of Python's layout logic rather than a query against a
live Python process — the class's own doc comment: "this is just a
mirror of what Python is doing, if there are changes to Python it may
require changes here" (`LibraryLocator.java:39-46`). No caching (the
full walk re-runs on every failed-`loadLibrary` startup), and no
install/self-heal step if nothing is found — it returns `false` and the
original `UnsatisfiedLinkError` propagates.

**jpype: runs a Python subprocess to ask the interpreter directly,
caches the answer, and can self-heal via `pip`.**
`Launcher.resolveLibraries()`
(`native/jpype_module/src/main/java/org/jpype/Launcher.java:417-456`)
resolves which `python` executable to target (system property →
`PYTHONHOME` env var → first `python3` on `PATH`,
`getExecutable`/`checkPath`, lines 190-253), then launches that
executable and runs a bundled probe script inside it
(`loadProbeResource`/`executeProbe`, lines 167-314): the probe reports
its own `sys.executable`, library paths, and JPype install location as
`Properties`, sourced from the live interpreter rather than
reconstructed from directory-layout conventions. That result is cached
on disk (`~/.jpype/jpype.properties` or the Windows `AppData`
equivalent) keyed by a hash of the executable path
(`saveCache`/`loadFromCache`, lines 255-264, 336-415), with a staleness
check on load that verifies the cached library paths still exist before
trusting the cache — specifically because a `pip install --upgrade` can
move the native module to a new wheel-cache path while leaving the
interpreter itself untouched (comment, `Launcher.java:397-402`). If the
probe fails outright and `jpype.install=true`, `runPipInstall()` (lines
462-504) looks for a local matching wheel first, falls back to a network
`pip install JPype1>=<version> --only-binary` otherwise, then re-probes.

**Summary**: jpy's discovery is entirely ahead-of-time (a file generated
once, that must be regenerated if the environment moves); jep's is
runtime discovery via simulating Python's own path logic in Java, which
tracks CPython's layout only as well as the simulation stays in sync
with it; jpype queries the live interpreter, caches the answer with a
validity check, and can repair a missing install rather than fail.
jep's approach is real discovery, not absent — the difference is that
jpype's is the only one of the three that both queries Python directly
and validates/self-heals its cache.

**Status, again:** everything above about jpype in this axis describes
`origin/reverse`, 190 commits ahead of `review` and not yet merged —
substantial and apparently mature, but not current `review` behavior,
and not re-verified with the same empirical rigor applied to
jpy/jep/pyjnius elsewhere in this document.

## Axis 8: Extensibility — customizers and the SPI

Whether a third party can teach a library to bridge a *new* type — on
either side of the boundary — without patching that library's own
source. This axis leans on `origin/reverse` for its reverse-direction
half (model 4, `python.lang`, `WrapperService`), so the same **not yet
merged into `review`** caveat from the embedding axis applies to those
rows; the forward-direction half (`@JImplementationFor`/`@JConversion`)
is current, shipped jpype behavior.

| Concern | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Customize how an existing Java class looks to Python, by class name, no source changes to the target | Yes — `@JImplementationFor`/`@JConversion` (`jpype/_jcustomizer.py`), string-keyed, works retroactively on an already-loaded class | No equivalent found — no `Customizer`/`ServiceLoader`/registration machinery anywhere in `~/devel/jpy/src/main/java/org/jpy` or `src/main/c` | No equivalent found — no `ServiceLoader`/`registerType`/`registerConversion` machinery anywhere in `~/devel/jep/src/main/java/jep` or `src/main/c/Jep` | No equivalent found — `jclass_register` (`jnius_export_class.pxi:118`) is a memoization cache keyed by `(classname, params)` for `autoclass()`'s own generated wrapper classes, not a third-party customization point |
| Automatic structural proxy: Java declares an interface, any Python object that structurally satisfies it gets proxied with no explicit call (`origin/reverse`) | Yes — `JPConversionPython`/`PyJP_probe` | No — three hand-written wrapper classes, constructed explicitly, no probing | No — fixed C-level type chain to a closed set of concrete Java types (plus one functional-interface special case, see below) | N/A — no Java-hosts-Python direction |
| User-extensible SPI: expose a new Python class as a typed Java interface, by dropping a file, no library source changes (`origin/reverse`) | Yes — `WrapperService`/`.pyspi`, `java.util.ServiceLoader`-discovered | No equivalent | No equivalent — a new target interface means patching and recompiling jep's C source | No equivalent |

**The automatic structural proxy (model 4) is models 2/3's own machinery,
invoked by probing instead of an explicit call.** `JPConversionPython`
(`native/common/jp_classhints.cpp:1908-1992`), a conversion rule
`JPPybaseType::findJavaConversionImpl` (`jp_pybasetype.cpp:34-48`) tries
for `java.lang.Object` and, by inheritance, every interface type that
falls through to it. Any time a Python value needs to become a
Java-typed value — argument, return, field, not just an explicit proxy
site — `matches()` calls `PyJP_probe(st, Py_TYPE(object))`
(`native/python/pyjp_probe.cpp`), which reads the Python type's own
C-level protocol slots (`tp_call`, `tp_as_buffer`, `tp_as_sequence`,
`tp_as_mapping`, `tp_as_number`, `__enter__`/`__index__`) plus
`collections.abc` subclass checks to derive which `python.lang`
interfaces that type structurally satisfies. If a probed interface
matches the target, `convert()` (`jp_classhints.cpp:1972-1991`)
constructs a `JProxy` on the spot — `_jpype._JProxy`, the same class
backing the explicit-proxy forms in the object-model axis — wrapping the
value with the method table the probe resolved.

jpy's typed wrappers (`PyModule`, `PyDictWrapper`, `PyListWrapper` —
three classes total) must be constructed explicitly by the caller around
a generic `PyObject`, with no probe-driven automatic selection; its own
general-proxy support (`PyObject.createProxy()`) didn't produce a usable
object in this checkout regardless. jep's Python-to-Java dispatcher,
`PyObject_As_jobject`
(`~/devel/jep/src/main/c/Jep/convert_p2j.c:1050-1116`), has one
automatic, declared-type-driven case: `PyCallable_Check(pyobject) &&
isFunctionalInterfaceType(env, expectedType)` triggers
`PyCallable_as_functional_interface` (lines 1091-1097), converting any
Python callable into any SAM-shaped target interface automatically, on
both argument and return paths. Everything else in that function is a
fixed, hardcoded C-level `if`/`else` chain (`PyLong_Check`/`PyDict_Check`/
`PyUnicode_Check`/buffer/numpy) mapping to a closed, compiled-in set of
concrete Java types, with `jep.python.PyObject` as the catch-all —
adding a new target interface to jep's version means patching and
recompiling its C source, the same gap as the `WrapperService`/`.pyspi`
comparison below, applying here to proxy-selection specifically.

jpype is the only one of the four with a structural, extensible,
no-call-site-changes automatic path; the other three require the Python
side to either be plain-callable or opt into a proxy at construction
time. jep independently arrived at the same idea, scoped to
callables-as-functional-interfaces rather than generalized to arbitrary
multi-method interfaces via protocol introspection. jpy and pyjnius have
no reverse direction to compare this against (jpy's exists but wasn't
usable in this checkout; pyjnius's doesn't exist).

**A user-extensible SPI, beyond `python.lang`'s builtin coverage.**
`org.jpype.WrapperService` (discovered via `java.util.ServiceLoader`,
JPMS-compatible via `provides ... with` in `module-info.java`) lets any
Java library expose any Python class as a typed Java interface by
registering a provider and dropping a declarative resource file per
class (`.pyspi`: a `key: value` header naming the Python
module/class/target Java interface, a `---` separator, then a Python
source blob binding a `METHODS = {...}` dict) — no editing of jpype's
own source. Example, `collections.deque.pyspi`:

```
kind: class
module: collections
class: deque
interface: python.collections.PyDeque
---
METHODS = {
    ".addFirst": lambda x, v: x.appendleft(v),
    ".removeFirst": lambda x: x.popleft(),
    ".size": len,
    ...
}
```

— mapping Python's `deque` onto a Java interface using
`java.util.Deque`'s own method names (`addFirst`/`removeFirst`), so Java
code gets a collection backed transparently by the real Python object.
jpype ships five built-in providers this way (27 `.pyspi` files,
475/475 tests passing): `python.io` (the `io`/`_io` hierarchy —
`BytesIO`/`StringIO`/`FileIO`/`BufferedReader`/`Writer`/`TextIOWrapper`/
etc.), `python.collections` (`ChainMap`/`Counter`/`OrderedDict`/
`defaultdict`/`deque`), `python.datetime` (`date`/`datetime`/
`timedelta`), `python.decimal` (`Decimal`), `python.pathlib`
(`PosixPath`/`WindowsPath`).

None of jpy/jep/pyjnius have an equivalent. jep's collection support
(`pyjlist.c`/`pyjmap.c`/etc.) and pyjnius's `protocol_map`
(`reflect.py`) are both real and both verified working (see the
object-model axis), but both are fixed in each library's own source —
adding support for a new Python stdlib or third-party class (say,
exposing `numpy.ndarray` as a typed Java interface) means patching jep's
or pyjnius's C/Cython source and rebuilding the extension. jpy has no
collection-protocol support to compare against, let alone an extension
mechanism for one.

**Neither side of the bridge needs awareness of the other.**
`JClassHints.registerClassImplementation(classname, proto)`
(`jpype/_jcustomizer.py:222-231`, behind `@JImplementationFor`) keys
purely on a string class name — no marker interface, no annotation, no
jpype dependency on the target's classpath, and no requirement that the
class exists yet at registration time (`_applyCustomizerPost` handles
customizing a class that's already loaded). `WrapperService`/`.pyspi`
has the same property from the other side: a Python module is declared
as satisfying a Java interface by name, with the module itself needing
no jpype awareness. Consequence: a closed-source, never-published Java
library or Python module can be customized to feel native, with the
customization living in a third location the end user writes, while the
library or module being customized stays unaware anything is bridging
into it. jep and jpy have no equivalent gate — the only way to get
comparable ergonomics for a private class there is patching and
recompiling their own C source, not an option for someone else's
internal library.

`jpype/_jcustomizer.py`'s `JImplementationFor(javaClassName)`/
`JConversion(cls, ...)` (a string-named target Java class, a decorator
registering a prototype whose methods get copied onto or converted to
that class's wrapper, applied retroactively even to an already-loaded
class) is the forward-direction version of the same pattern
`WrapperService`/`.pyspi` implements in reverse: string-named target
Python module/class, a declarative method binding to a named Java
interface, discovered and replayed at startup instead of hardcoded.
Every Python-side customizer referenced elsewhere in this doc
(`_JCharArray` on `byte[]`/`char[]`, the `toPython()` conventions on
`java.io` streams) is built on the forward version of this mechanism.

### Syntax: registering a customization

| | jpype forward (Java class → Python) | jpype reverse (Python class → Java, `origin/reverse`) |
|---|---|---|
| Decorator/registration call | `@JImplementationFor("java.lang.String")` / `@JConversion(...)` | `.pyspi` declarative file, discovered via `ServiceLoader` |
| Keying | String class name | String module/class name + target Java interface name |
| Works on an already-loaded/already-imported class? | Yes — `_applyCustomizerPost` | Yes — replayed at startup, no import-order requirement found |

jpy/jep/pyjnius have no equivalent mechanism on either side — this
syntax table has no columns for them because there is nothing to fill
in.

## Axis 9: Introspection & ergonomics

Things that don't change whether a program runs, but change how much a
Python-side developer can rely on IDE tooling, `pickle`, and Java's own
caller-sensitivity rules working transparently through the bridge.

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Pickling / `copyreg` support | Yes (9 tests) | No | No | No |
| Caller-sensitive JDK method handling | Yes (20 tests) | No | No | No |
| Javadoc-derived docstrings / Jedi / typing-stub generation | Yes (~37 tests) | No | No (bare `dir()` only) | No (bare `dir()` only, `__doc__ is None`) |

Caller-sensitive JDK methods (`Class.forName`, `ClassLoader.getResource`,
etc.) resolve based on the caller's declaring class, which normally
means the JVM's own call stack — a detail invisible from a bridged
language unless the bridge accounts for it explicitly. jpype handles
this as a distinct case (`test_caller_sensitive.py`); no reference was
found in jpy or jep's source, and pyjnius wasn't checked for it either.

Docstrings, `repr()`, Jedi/IDE completion, and module/typing-stub
generation are all downstream of the same question: does the bridge
carry Javadoc text and type information across, or does a bound method
show up to Python tooling as an opaque callable with no metadata? jep
and pyjnius both expose method names via `dir()` (so autocomplete on
method *names* works) but neither carries docstring text — pyjnius
confirms `__doc__ is None` for every bound method; jep's `test_dir.py`
covers the same bare-listing behavior. jpy has no analogous test files
or source for any of this.

### Syntax: listing what's available on an object

| | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| `dir(obj)` lists Java methods | Yes, with docstrings sourced from Javadoc | No `__dir__` override or populated `tp_methods` found (`jpy_jtype.c:2706` sets `tp_methods` to `NULL`); attribute access instead runs through a custom `tp_getattro` (`JType_getattro`), so default `dir()` wasn't checked to actually list anything beyond Python's own type slots | Yes, method names only, no docstrings | Yes, method names only, `__doc__` is `None` |
| Pickle a Java-backed object | Yes (`test_pickle.py`, `test_serial.py`) | No | No | No |

## Testing methodology: how each library tests itself

A different question from every axis above: not what each library's
bridge can do, but how rigorously its own test suite checks that the
bridge does it correctly. Four sub-questions: does correctness get
checked from both language sides independently, or only from Python?
Is there any coverage instrumentation, or does test count stand in for
it? Is memory/reference leaking checked at all, and if so how? Is
error-path handling itself under test (deliberately breaking things), or
only the happy path?

| Concern | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Python-side test suite | Yes — 1,884 tests, `test/jpypetest/` | Yes — 151 tests, `src/test/python/` | Yes — 247 tests, `src/test/python/` | Yes — 160 tests, `tests/` |
| Independent Java-side test suite (its own assertions, its own pass/fail, not just a target Python calls into) | Yes — 104 TestNG classes, `native/jpype_module/src/test/java/`, exercising the reverse-embedding direction as its own suite | Yes — 10 JUnit-annotated classes (`@Test`), `src/test/java/org/jpy/` (`LifeCycleTest`, `PyProxyTest`, `MultiThreadedEvalTestFixture`, etc.) | No independent framework — 37 files under `src/test/java/jep/test/`, but none use JUnit/TestNG/an assertion library; they're plain classes with `public static void main()`, run standalone or driven from a Python test that does the actual assertion (e.g. `TestCrossLangSync` ↔ `test_synchronized.py`) | No — Java files under `tests/java-src/` (e.g. `ClassArgument.java`) are plain fixture classes with no `main()` and no assertions, existing only to be called into from Python tests |
| Coverage instrumentation | Yes — `coverage.sh`, merges Python (`coverage.py`), Java (JaCoCo, merged at the method level across *both* Java-side and Python-side suites since they don't produce CRC-identical classfiles), and C++ (gcov) into one report; last measured at 95.1% with every sub-100% method individually categorized (real gap / dead code, since removed / deliberately deferred) rather than left as a raw number | No coverage config, plugin, or CI step found (`pom.xml`, `setup.cfg`) | No coverage config, plugin, or CI step found (`pom.xml`) | No coverage config found (`setup.cfg`, `pyproject.toml`) |
| Memory/reference leak checking | Yes — `test/jpypetest/leakharness.py`/`leaksweep.py`/`test_leak.py`/`test_leak2.py`: starts a small-heap JVM, measures actual process memory growth over a config-driven, time-budgeted sweep of repeated operations against a target list (`leak_targets.txt`); always runs, no special build required | None found | Yes, but narrower and conditionally skipped — `test_python_memory.py` asserts `sys.gettotalrefcount()` grows by exactly the expected delta per operation, which is more precise per-call than jpype's growth-over-many-iterations approach, but only runs on a Python built `--with-pydebug` (`@unittest.skipIf`) — not exercised on an ordinary interpreter build | None found |
| Fault-injection (deliberately broken inputs/state exercising error paths, not just correctness on valid input) | Yes — `test_fault.py`, 88 tests | None found | None found | None found |

None of jpy/jep/pyjnius run coverage-instrumented CI at all, as far as
this document's source reading found — test *count* is the only signal
available for those three, which is exactly why the porting-coverage
section below leans on file/test counts rather than coverage percentages
for the cross-library comparison. jep's Java-side files being fixtures
rather than an independent test suite is worth sitting with for a
moment: several results earlier in this document (`TestCrossLangSync`,
`TestSharedModulesThreads`) were run directly as standalone Java
programs via their own `main()` methods, not via a test framework's
runner — a real, reproducible result, but with no independent Java-side
pass/fail signal or reporting the way jpy's JUnit suite or jpype's
TestNG suite provide. pyjnius is the one library here with no
independent Java-side correctness check of any kind, and no leak
checking on either side — its 160 Python-side tests are the entirety of
its own verification story.

## Test-suite / porting coverage

Closing summary, not an axis comparison: if jpype's test suite were
actually ported to run against jpy/jep/pyjnius, how much of it would
have something to run against.

| | jpype | jpy | jep | pyjnius |
|---|---:|---:|---:|---:|
| test files | 90 | 21 | 34 | 37 |
| tests | 1,884 | 151 | 247 | 160 |

jpype: 1,884 tests across 90 files (~21k lines), `test/jpypetest/`. jpy:
151 tests across 21 Python test files, `~/devel/jpy/src/test/python/`.
jep: 247 tests across 34 files, `~/devel/jep/src/test/python/`. pyjnius:
160 tests across 37 files, `~/devel/pyjnius/tests/` — between jpy's file
count and jep's, but fewer total tests than jep's.

Collection-protocol/`Comparable`/functional-interface/general-proxy
support (axis 1) means jep and pyjnius both have something to port a
meaningfully larger fraction of jpype's suite against than jpy does.
`test_classhints.py`/`test_hints.py`/`test_customizer.py` (axis 8),
`test_pickle.py`/`test_serial.py`, and the introspection-ergonomics
files (axis 9) are gaps for all three of jpy/jep/pyjnius.
Multi-dimensional/buffer array tests (axis 2) are a gap for jpy (partial)
and pyjnius (total) but not jep (partial, same as jpy).

Roughly 250+ of jpype's tests exercise features with no jpy counterpart
at all — those can only be noted as gaps, not ported. The remainder
(conversion, arrays, strings, exceptions, fields/properties,
overloads/varargs, reflect, jclass/jpackage/imports, numeric/boxing,
buffers, inherit, hash, synchronized) is the realistic portable subset
against jpy if this comparison were ever turned into an actual ported
test run. Against jep and pyjnius, that portable subset is somewhat
larger, per the collection-protocol/proxy support noted above.

**Not compared here, for all three of jpy/jep/pyjnius:** fault-injection
tests (`test_fault.py`, 88 tests) and coverage-instrumentation tests
(`test_coverage.py`, `test_javacoverage.py`, 50 tests) exercise jpype's
own internal error paths, not a portable behavior — excluded from every
axis above, not because any of jpy/jep/pyjnius specifically lack the
feature, but because there's no "feature" there to lack.

## Further out (speculative): J2NI, and what "JPype2" could mean

Speculative — a separate project (`~/javafx2/j2ni`, its own repo, own
`pom.xml`), not a jpype branch, not present in jpype's own git history
(checked every local and remote branch). Its own README states,
verbatim: `**DRAFT**` — "most piece are drafted but getting all the
definitions consistent and tested will take a while." None of this is
jpype's committed roadmap; it's included because it changes what "jpype
ahead of jpy/jep/pyjnius" would mean if it lands.

**What it is**: a proposed replacement for JNI itself, built on Java
22+'s Foreign Function & Memory API (Project Panama) — a rework of the
layer every one of jpy/jep/pyjnius, and jpype's own current codebase,
sits on top of. Concretely:

- **Metadata-driven dispatch** (signature hashing, pre-cooked method
  handles) instead of JNI's string-based method/field lookup.
- **Query-View-Pull** bulk data transfer — native code queries a "view"
  (size, element type, writability) then pulls/pushes in bursts —
  instead of JNI's pin-a-raw-pointer model.
- **Process-agnostic identifiers** (`int64_t` handles, not raw
  object-header pointers), so the same protocol works whether native and
  JVM code share a process, share memory, or are separate, possibly
  remote processes. `j2ni-remote` is a bit-packed wire protocol for this
  (`FIXME.md` shows header-layout tuning —
  `(size:11)(routing:16)(ack:1)(op:12)(checksum:24)`, with routing,
  opcodes, and checksums) — an object broker over a wire, not an
  in-process trampoline.
- `FIXME.md` mentions J2NI's own native-to-Java export surface "grows
  via the SPI extension pattern" — the same extensibility approach as
  `WrapperService`/`.pyspi` above, in a different project by the same
  author.

Everything else in this "Future" section — the reverse bridge, three
embedding layers, the SPI, subinterpreters — is jpype getting more
capable within the JNI-based architecture jpy/jep/pyjnius are also built
on. J2NI is a different kind of change: if jpype's native layer is ever
rebuilt on it, the comparison stops being about feature count and
becomes about memory-safety and cross-process/remote capability that
isn't a row on a feature-matrix table, since none of jpy/jep/pyjnius (or
jpype's own `review` branch, today) have anything comparable. Whether it
ships as part of jpype is unknown from here; this section is a marker of
direction, not a claim about outcome.
