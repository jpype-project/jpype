# Cross-library call-overhead benchmarks

Compares JPype's general per-call overhead against jpy, jep, pyjnius, and
GraalPy, to establish a baseline against which future conversion-path
optimization work can be measured, and (for GraalPy specifically) how far
JPype is behind a bridge with a genuine tiered JIT (Truffle/Graal) instead
of an interpreter.

Laid out as one directory per library -- `jpype/`, `jpy/`, `jep/`,
`pyjnius/`, `graalpy/` -- with matching filenames across all five, one
file per benchmark category. To compare a category across libraries, just
look at the same filename in each directory (e.g. `jpype/array_flat.py`
vs `jpy/array_flat.py` vs `jep/array_flat.py` vs `pyjnius/array_flat.py`
vs `graalpy/array_flat.py`); a file missing from one directory, or a row
missing from one file, is a documented gap (see below), not an oversight.
Categories, and where each one lives:

| file | category | jpype | jpy | jep | pyjnius | graalpy |
|---|---|---|---|---|---|---|
| `int.py` | `Math.max(int,int)`, `new Integer(int)` | yes | yes | yes | yes | yes |
| `double.py` | `Math.sqrt(double)`, `new Double(double)` | yes | yes | yes | yes | yes |
| `strings.py` | `new String(...)` + `.toString()` | yes | yes | yes | yes | yes |
| `object.py` | plain `Object` identity (arg + return) | yes | yes | yes | yes | yes |
| `dispatch.py` | overload resolution x16, mono + polymorphic | yes | yes | yes | yes | yes |
| `proxy.py` | established callback binding, int + Object arg | yes | -- | yes | int arg only\*\*\* | yes\*\*\*\*\*\*\* |
| `array_flat.py` | 1D list->array/buffer->array, 100/1k/10k/100k elements, int/long/float/double | yes | yes | yes | list->array only\*\*\*\* | list->array + manual buffer->array\*\*\*\*\*\*\*\* |
| `array_multidim.py` | 2D-5D list->array/buffer->array, fixed element count, int/long/float/double | yes | yes | yes | list->array only\*\*\*\* | list->array + manual buffer->array\*\*\*\*\*\*\*\* |
| `array_ragged.py` | ragged (irregular sibling-length) nested-list push, 2D-5D, int/long/float/double | yes | yes | yes | yes | yes |
| `array_noncontig.py` | non-contiguous numpy buffer push (column slice / transposed), flat + 2D-5D, int/long/float/double | yes | yes | yes | stub only\*\*\*\*\* | manual buffer->array\*\*\*\*\*\*\*\* |
| `array_shape.py` | 2D/3D shape sweep at fixed total element count, push only, int/long/float/double | yes | yes | yes | list->array only\*\*\*\*\*\* | list->array + manual buffer->array\*\*\*\*\*\*\*\* |
| `classhints.py` | `@JConversion` hint-list cache scan | yes | -- | -- | -- | -- |
| `array_of.py` | `JArray.of()` constructing an array directly from a buffer, vs. the naive sequence constructor | yes | -- | -- | -- | -- |

`int.py`/`double.py`/`strings.py` are trivial single-overload JDK-builtin
calls -- a baseline with no interesting conversion machinery behind it.
`object.py`/`dispatch.py`/`proxy.py`/`array_*.py` all use the shared
`jpype.benchmark.DeepBench` test class
(test/harness/jpype/benchmark/DeepBench.java -- a plain compiled class
with no jpype dependency, so all four libraries can put test/classes +
test/harness directly on their classpath and call it) to exercise deeper
conversion-chain paths: overload resolution across 16 candidates,
Object-argument/return identity, a proxy callback (Java calling back into
an established Python-side binding), and array argument/return
conversion.

`array_flat.py`/`array_multidim.py` sweep two axes: flat (1D) transfer at
100/1k/10k/100k elements, and multi-dimensional transfer at 2D-5D holding
total element count fixed (10\*\*dims elements, so e.g. the 3D case and
array_flat.py's 10k case both move the same 10,000 ints -- isolating
nesting-depth overhead from raw element count).

Each sweep splits into four categories, not one "arrays" bucket -- a
plain Python list and a buffer-protocol object (numpy) are genuinely
different native code paths, not just different inputs to the same one,
and conflating them under one label was actively misleading (a fast
buffer-based row and a slow list-based row averaged together looks like
neither number, and hides which one a real caller is actually going to
hit):
  - `list->array` -- push, from a plain (nested) Python list
    (`DeepBench.sum*IntArray(list)`).
  - `buffer->array` -- push, from a buffer-protocol object
    (`DeepBench.sum*IntArray(numpy_array)`).
  - `array->list` -- pull, a fresh Java array
    (`DeepBench.make*IntArray`) fully materialized into plain
    (recursive) Python lists.
  - `array->buffer` -- pull, the same fresh Java array read back via
    `np.asarray()`.

jpy and jep each have real, source-confirmed limitations that make some
of these categories collapse onto the same code path, or (for jep's
multi-dimensional `buffer->array` push) not exist automatically at all --
see the comments at the top of each script for specifics and, for jep,
the manually-assembled workaround measured there instead.

`array_ragged.py`, `array_noncontig.py`, and `array_shape.py` extend the
flat/multidim sweeps along three more axes -- irregular (non-rectangular)
nested-list shapes, non-contiguous numpy sources, and lopsided (row-heavy
vs. column-heavy) 2D/3D shapes at a fixed total element count,
respectively. `jpy/array_ragged.py` and `jpy/array_noncontig.py` port
directly (jpy accepts buffer-protocol push arguments the same way jpype
does), but see their docstrings for jpy-specific notes: jpy's per-element
recursion has no ragged-vs-rectangular distinction to begin with (unlike
jpype's ragged-native fast path), and jpy's buffer-argument matching
(`JType_ConvertPyArgToJObjectArg`, jpy_jtype.c) requests `PyBUF_SIMPLE`
with no strides support and only engages for a flat (not nested) target
type -- source-level evidence that the 1D non-contiguous row there may
error rather than degrade gracefully, not yet confirmed by an actual run.
`jpy/array_shape.py` ports directly with no caveats -- it exercises the
same list/buffer push paths already covered by array_flat.py/
array_multidim.py.

`jep/array_ragged.py` also ports directly, for the same reason as jpy's:
jep's per-element push recursion (`pyfastsequence_as_jobject`) doesn't
validate sibling-length uniformity before walking a nested list, so
there's no ragged-vs-rectangular fast-path distinction on the jep side
to even ask a question about -- see its docstring for how that's phrased
(measured, not assumed). `jep/array_noncontig.py`'s flat (1D) row also
ports directly and is a genuine test of jep's real numpy fast path's
contiguity handling; its ND (transposed) row hits the same "jep's numpy
fast path only targets flat 1D" limitation as `array_multidim.py`'s
`buffer->array` push (see below), so it's routed through that same
manual per-row assembly workaround instead, fed a transposed
(non-contiguous) source so the per-row leaf calls themselves are
non-contiguous -- see its docstring for the full reasoning.
`jep/array_shape.py`'s `list->array` category ports directly; its
`buffer->array` category hits the identical multi-dim limitation and
also reuses the manual per-row assembly workaround, generalized to
arbitrary (not just square) shapes.

`pyjnius/array_ragged.py` ports directly too, with no caveat: ragged
shape is a push-side, plain-nested-list concept (a numpy array can't
even represent a ragged shape), and pyjnius's list->array push takes a
plain nested Python list the same way any other library does, ragged or
not -- there's no rectangular-only restriction to work around.
`pyjnius/array_noncontig.py` and `pyjnius/array_shape.py` both run into
pyjnius's `buffer->array` push limitation already described below for
array_flat.py/array_multidim.py -- see \*\*\*\*\* and \*\*\*\*\*\*.

\*\*\*\* **pyjnius has no `buffer->array` push at all, at any size or
depth** -- confirmed empirically, not assumed: passing a numpy array
where a Java array argument is expected raises `JavaException('Expecting
a python list/tuple, got array(...)')` unconditionally, stricter than
jep (which at least has a real fast path for a flat 1D target) and
stricter than jpy/jpype (both accept a buffer-protocol object). So
`pyjnius/array_flat.py`/`array_multidim.py` only have three rows, not
four: `list->array` push, and `array->list`/`array->buffer` pull. Pull
is also structurally different from the other three libraries: a
returned Java array comes back from pyjnius *already* a fully
materialized, recursively-nested plain Python list -- there's no wrapper
array object to convert afterward, so `array->list` there is just the
raw return value, and `array->buffer` is that same value plus an extra
`np.asarray()` step (never faster, since there's no bulk Java-array-to-
numpy path to win with either).

\*\*\*\*\* **`pyjnius/array_noncontig.py` is a stub with no benchmarks
in it, on purpose.** array_noncontig.py's whole point (for jpype/jpy) is
whether a non-contiguous buffer source hits a bulk buffer-read path or
falls back to a slower per-element walk -- a question that presupposes a
`buffer->array` push path exists to fall back *from*. pyjnius doesn't
have one at all (see \*\*\*\* above), so a non-contiguous numpy source is
rejected by the exact same unconditional `JavaException` a contiguous
one is -- there's no degradation to measure, just a no-path. The file
still exists (matching the one-file-per-category layout every other
library follows) and prints an explanation when run directly, rather
than being silently absent.

\*\*\*\*\*\* **`pyjnius/array_shape.py` has only the `list->array` push
category, not `list->array` + `buffer->array`.** Same root cause as
\*\*\*\* above: pyjnius has no `buffer->array` push, so there's no
shape-dependence question to ask about a push path that doesn't exist.
The `list->array` category itself (the same `nested_list_shaped`
helper, same `SHAPES_2D`/`SHAPES_3D` lists, same fixed-total-varying-
shape point) ports directly with no other caveat.

\*\*\*\*\*\*\*\* **GraalPy has no `buffer->array` push at all, at any
size, depth, or element type -- worse than pyjnius's version of the same
gap.** Passing a numpy array where a Java array argument is expected
raises `TypeError('invalid instantiation of foreign object')`
unconditionally (confirmed empirically), even for a flat 1D target --
stricter than jep (which at least has a real fast path for flat 1D) and
on par with pyjnius's blanket rejection, except pyjnius at least fails
with one consistent, clear exception type rather than a generic
`TypeError` from the polyglot layer's own argument-conversion machinery.
Unlike pyjnius's version of this gap, it is **not** treated as a
documented no-path stub here: converting a numpy array to/from a Java
array is basic orchestration for a Python/Java bridge, not an edge case,
so `graalpy/_arrayutil.py`'s `build_manual()` emulates the missing push
by hand -- allocate a genuine Java array via `java.type('<prim>[]...')`
and fill it element by element (recursing one level per dimension) from
the numpy source -- and every `graalpy/array_*.py` file measures it as a
real "buffer->array (manual)" category, not a gap to skip. It's a plain,
uniform per-element Python-level loop, exactly the kind of hot loop a
real JIT is supposed to be good at optimizing, so it's fair game to
measure on its own terms even though it takes the long way around.

Because it's so slow, `build_manual()`'s numbers are collected at n=3
samples/trial at every size that matters (`_arrayutil.py`'s
`calls_for_manual()`, a ~1,000x smaller budget than every other category
in this suite uses) -- real numbers from real code, but at far lower
statistical confidence than anywhere else in this comparison; see
`RESULTS.md`'s Section 10 intro for the full statement of what that
means for reading these rows.

**Measured result: `build_manual()` is roughly 250-300x slower per
element than GraalPy's own automatic `list->array` push** (~13,000
ns/element vs. ~45 ns/element, see `RESULTS.md`) -- confirming that
GraalPy's JIT helps with hot scalar/dispatch/proxy call overhead
(Sections 2-3 there) but does nothing to compensate for a genuinely
missing bulk-transfer primitive; no amount of tier-4 compilation turns an
element-at-a-time polyglot marshalling loop into a bulk memcpy. At
extreme row-heavy shapes (`graalpy/array_shape.py`'s `100000x3`/`3x100000`
2D shapes), `build_manual()`'s per-object overhead can exhaust a capped
GraalPy heap outright -- a genuine `MemoryError`, reproduced reliably at
`-Xmx3g` on this machine, caught per-row (recorded as `N/A` in the CSV,
not silently dropped) so one exhausted shape doesn't take the rest of the
sweep down with it. See `graalpy/array_shape.py`'s docstring and
`RESULTS.md`'s GraalPy section for the full writeup of what this implies
for using GraalPy in scientific-Python-orchestration workloads (which are
dominated by exactly this kind of bulk array transfer, not scalar call
overhead) versus microscript/glue-code use cases (where GraalPy's fast
scalar/proxy path and JIT are a genuine advantage).

`classhints.py` is JPype-only (exercises the hint-list cache
specifically; jpy/jep have no `@JConversion`-style extensible hint
mechanism to compare against). Requires the test harness classes
(`jpype.classhints.Custom`/`ClassHintsTest`, built via
`BUILD_TEST_HARNESS=ON`) on the classpath.

`array_of.py` is also JPype-only -- `JArray.of()` (construct an array
directly from a buffer-protocol source) has no jpy/jep/pyjnius
equivalent to compare against; measures it against `JArray(dtype)(arr)`,
the naive sequence-constructor route.

**proxy**: each library's mechanism for exposing a Python object as a
Java interface differs and isn't drop-in comparable:
- jpype: `@JImplements` on a class, constructed once -- steady-state,
  matches an established callback binding (e.g. a Comparator used
  repeatedly), not per-call proxy creation (a bare Python function would
  measure that instead, since JPFunctional re-wraps one fresh each call).
- jep: `jep.jproxy(pyobj, [interfaces])`, also constructed once.
- jpy: `PyObject.createProxy(Class)`, called from Java in jpy's own
  tests (`ReachabilityFenceTestFixture.stressProxy`). Calling it from
  Python (`jpy.convert(obj, PyObject_type).createProxy(cls)`) produced an
  object that jpy's own dispatcher then refuses to match against any
  Java method ("no matching Java method overloads found"), including
  `DeepBench.invokeCallbackLoop` (which loops entirely in Java, so it
  isn't about calling the proxy's methods from Python either) -- this
  looks like a real gap/bug in this jpy checkout, not a usage error, so
  there's no `jpy/proxy.py`.
- pyjnius: `PythonJavaClass` subclass + `@java_method('<jni-signature>')`,
  also constructed once.
- GraalPy: nothing at all -- a plain Python object (or even a bare
  function, for a single-method interface) with a matching method name is
  auto-adapted to any Java functional interface wherever one is expected,
  confirmed empirically for both the `int`-arg and `Object`-arg cases,
  including the null-`Object`-argument case that crashes pyjnius (see
  \*\*\* below) -- GraalPy's Java-hosts-Python direction handles it
  cleanly with no proxy-construction API of any kind. See
  \*\*\*\*\*\*\* below.

\*\*\*\*\*\*\* **GraalPy has no explicit proxy-construction step, unlike
every other library here** -- see `graalpy/proxy.py`'s docstring. This
makes GraalPy's proxy category the one place in this whole suite where
GraalPy is architecturally *simpler*, not just faster or slower, than
jpype/jep/pyjnius: those three all need an explicit
class-implements-interface declaration (`@JImplements`, `jep.jproxy()`,
`PythonJavaClass` subclassing) constructed once ahead of the steady-state
calls being measured; GraalPy needs nothing extra at all, the plain
callback object itself is already a valid argument.

\*\*\* **pyjnius's proxy Object-arg case is not benchmarked because it
crashes the JVM.** `DeepBench.invokeObjectCallbackWithNull` (a
Python-implemented Java interface method receiving a genuinely null
`Object` argument -- exactly the case jp_proxy.cpp's own regression
coverage exists for) reliably segfaults this pyjnius checkout with a
native `SIGSEGV` in `jni_GetObjectClass`, reproduced independently three
times against a fresh build in a disposable venv (ruled out as a
stale-build artifact first, per this repo's CLAUDE.md, before treating it
as a real finding). This pyjnius checkout's proxy-argument-marshalling
code calls `GetObjectClass`/`IsSameObject` on the argument without
checking for null first -- undefined behavior over JNI. Even the
non-crashing case (a real, non-null `Object` argument) doesn't work
correctly either: `invokeObjectCallback` silently returns `None` instead
of the object the Python callback handed back, a separate (non-fatal)
correctness bug in pyjnius's proxy return-value handling. Neither is
worth a benchmark number, and the crash means `pyjnius/proxy.py` must
never call the null-argument variant -- only the `int`-arg case
(`invokeCallback`) is measured there.

Per this repo's CLAUDE.md, never install any of these into a real/shared
Python environment -- always a disposable venv, one per library since
they each embed their own JVM/native glue.

## JPype

```
python3.12 -m venv /tmp/jpype-bench-venv
/tmp/jpype-bench-venv/bin/pip install --upgrade pip
/tmp/jpype-bench-venv/bin/pip install scikit-build-core pybind11 pytest numpy
/tmp/jpype-bench-venv/bin/pip install --no-build-isolation -e . \
    --config-settings=cmake.define.BUILD_TEST_HARNESS=ON

# run any/all of them -- each is self-contained (starts its own JVM):
for f in project/benchmark/jpype/*.py; do
    /tmp/jpype-bench-venv/bin/python "$f"
done
```

## jpy

Needs a built jpy wheel (see ~/devel/jpy/dist).

```
python3.12 -m venv /tmp/jpy-bench-venv
/tmp/jpy-bench-venv/bin/pip install /path/to/jpy/dist/jpy-*.whl

# int.py/double.py/strings.py take no args:
cd /path/to/jpy && /tmp/jpy-bench-venv/bin/python \
    /path/to/jpype/project/benchmark/jpy/int.py

# the rest need DeepBench on the JVM classpath jpy starts, so they take
# test/classes and test/harness as explicit args:
cd /path/to/jpy && /tmp/jpy-bench-venv/bin/python \
    /path/to/jpype/project/benchmark/jpy/dispatch.py \
    /path/to/jpype/test/classes /path/to/jpype/test/harness
```

(jpy's `jpyutil.init_jvm` locates the JVM relative to cwd/JAVA_HOME; run
from the jpy checkout if init_jvm can't find things.)

## jep

jep embeds CPython *inside* the JVM (opposite direction from
jpype/jpy), so it's launched as a Java process, not `python script.py`,
and its embedded stdout isn't connected to the launcher -- results are
written to a file (each script's `out_path` arg).

Needs jep's jar (~/devel/jep/target/jep-4.3.1.jar) and a native
`libjep.so` build matching the Python version PYTHONPATH points at (this
project's ~/devel/jep checkout only had libjep.so built for Python 3.10
at the time this was written, not 3.12 -- check
~/devel/jep/build/lib.linux-x86_64-<pyver>/jep/ for what's actually
available before assuming a version).

```
JEP_JAR=~/devel/jep/target/jep-4.3.1.jar
JEP_LIB_DIR=~/devel/jep/build/lib.linux-x86_64-3.10/jep
JEP_PKG_PARENT=~/devel/jep/build/lib.linux-x86_64-3.10

# int.py/double.py/strings.py need no extra classpath:
PYTHONPATH="$JEP_PKG_PARENT" java -classpath "$JEP_JAR" \
    -Djava.library.path="$JEP_LIB_DIR" jep.Run \
    project/benchmark/jep/int.py /tmp/bench_jep_int_results.txt
cat /tmp/bench_jep_int_results.txt

# the rest need DeepBench on the classpath too:
CP="$JEP_JAR:$(pwd)/test/classes:$(pwd)/test/harness"
PYTHONPATH="$JEP_PKG_PARENT" java -classpath "$CP" \
    -Djava.library.path="$JEP_LIB_DIR" jep.Run \
    project/benchmark/jep/dispatch.py /tmp/bench_jep_dispatch_results.txt
cat /tmp/bench_jep_dispatch_results.txt

# array_flat.py/array_multidim.py/array_ragged.py/array_noncontig.py/
# array_shape.py additionally take an optional second arg, a CSV output
# path (defaults to <category>_results.csv next to the first arg):
PYTHONPATH="$JEP_PKG_PARENT" java -classpath "$CP" \
    -Djava.library.path="$JEP_LIB_DIR" jep.Run \
    project/benchmark/jep/array_shape.py \
    /tmp/bench_jep_array_shape_results.txt \
    /tmp/bench_jep_array_shape_results.csv
```

## pyjnius

No prebuilt wheel in ~/devel/pyjnius -- it's a Cython extension
(`jnius/jnius.pyx`), built from source. Needs `JAVA_HOME` pointing at a
JDK (not just a JRE -- `setup.py` asserts this) and Cython pinned per
`pyproject.toml` (`Cython~=3.1.2` at the time this was written).

```
python3.12 -m venv /tmp/pyjnius-bench-venv
/tmp/pyjnius-bench-venv/bin/pip install --upgrade pip
/tmp/pyjnius-bench-venv/bin/pip install "Cython~=3.1.2" setuptools wheel numpy
cd ~/devel/pyjnius && /tmp/pyjnius-bench-venv/bin/pip install -e .

# int.py/double.py/strings.py take no args -- unlike jpy/jep, pyjnius
# auto-starts its embedded JVM on first autoclass() call and needs no
# explicit init or shutdown call:
/tmp/pyjnius-bench-venv/bin/python \
    /path/to/jpype/project/benchmark/pyjnius/int.py

# the rest need DeepBench on the classpath, set via jnius_config *before*
# the first `from jnius import ...` -- these scripts take classes_dir/
# harness_dir as optional positional args (defaulting to test/classes,
# test/harness relative to cwd):
cd /path/to/jpype && /tmp/pyjnius-bench-venv/bin/python \
    project/benchmark/pyjnius/dispatch.py test/classes test/harness
```

## GraalPy

GraalPy embeds Python *inside* the JVM (same direction as jep, opposite
of jpype/jpy/pyjnius): a small Java launcher (`graalpy/src/main/java/org/
jpype/bench/graalpy/Bench.java`) opens a GraalPy `Context` with
`allowAllAccess(true)` and evals the given `.py` script inside it. Unlike
jep, GraalPy's embedded stdout *is* connected to the launching process's
stdout, so these scripts print directly and import `../_common.py` the
normal way (`__file__` is defined, unlike jep's embedded interpreter).

Needs a GraalVM CE JDK with a version-matched (not just any) GraalPy
Maven/Truffle artifact set -- see `graalpy/pom.xml`'s `graalpy.version`
property comment for exactly why the versions are pinned where they are
(short version: 25.0.2 on both sides is the newest version with both a
prebuilt `numpy` wheel and a matching standalone GraalVM CE JDK release;
anything newer forces either a from-source numpy build via meson -- which
OOM-killed this 7.7GB machine during setup, see this repo's CLAUDE.md --
or a libgraal native-ABI mismatch between the JDK's bundled compiler and
the Truffle jars, both fatal to the actual point of this comparison).
GraalVM CE isn't in apt for this distro; it was installed under
`~/.local`:

```
GRAALVM_DIR=~/.local/graalvm-community-openjdk-25.0.2+10.1
# download/verify against the checksummed asset at
# https://github.com/graalvm/graalvm-ce-builds/releases/tag/jdk-25.0.2
# (jdk-25.0.2 specifically -- see the pom.xml comment above)

cd project/benchmark/graalpy
JAVA_HOME="$GRAALVM_DIR" mvn package
# pulls numpy==2.2.4 (pinned to the newest prebuilt GraalPy wheel, see
# pom.xml) into a GraalPy-managed venv under
# target/classes/org.graalvm.python.vfs/venv via graalpy-maven-plugin,
# and copies runtime dependency jars to target/lib/ for the classpath
# below.
```

**Always cap the JVM heap explicitly when running any of these** -- an
uncapped run doesn't fail cleanly on this machine, see this repo's
CLAUDE.md and the OOM incident during this harness's own setup (a nested
Maven -> pip -> meson build chain, each its own ~1GB+ Truffle runtime,
exhausted all 7.7GB of RAM and triggered the kernel OOM killer, which
killed unrelated processes system-wide, not just the offending build).
`graalpy/run_all.sh` runs the full suite sequentially (never
concurrently) with `-Xmx3g -Dpolyglot.engine.CompilerThreads=2` for
exactly this reason -- **always invoke `$GRAALVM_DIR/bin/java` directly
by absolute path, not whatever `java`/`JAVA_HOME` this shell's profile
happens to default to** (this repo's normal `JAVA_HOME` is a plain
Temurin JDK with no bundled libgraal; running under it silently falls
back to an interpreter-only "fallback runtime", defeating the entire
point of this comparison, with no error, just a `[engine] WARNING: ...
JVMCI is not enabled` line easy to miss in the log):

```
# int.py/double.py/strings.py/object.py/dispatch.py/proxy.py/array_*.py
# all run through the same Bench launcher -- DeepBench needs
# test/classes + test/harness on the classpath (harmless to include even
# for the three that don't call it):
cd project/benchmark/graalpy
"$GRAALVM_DIR/bin/java" -Xmx3g -Dpolyglot.engine.CompilerThreads=2 \
    --enable-native-access=ALL-UNNAMED \
    -cp "target/classes:target/lib/*:../../../test/classes:../../../test/harness" \
    org.jpype.bench.graalpy.Bench int.py

# or run everything sequentially, logging each script to
# graalpy/run_logs/<script>.log:
./run_all.sh
```

`--enable-native-access=ALL-UNNAMED` suppresses a native-access warning
from numpy's compiled C-extension module running under GraalPy's C-API
emulation layer; harmless to omit, just noisy.

