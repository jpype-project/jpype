# Cross-library call-overhead benchmarks

Compares JPype's general per-call overhead against jpy and jep, to check
whether the JPClass::findJavaConversion caching work
(native/common/include/jp_conversioncache.h) has closed the gap.

Laid out as one directory per library -- `jpype/`, `jpy/`, `jep/` -- with
matching filenames across all three, one file per benchmark category. To
compare a category across libraries, just look at the same filename in
each directory (e.g. `jpype/array_flat.py` vs `jpy/array_flat.py` vs
`jep/array_flat.py`); a file missing from one directory is a documented
gap (see below), not an oversight. Categories, and where each one lives:

| file | category | jpype | jpy | jep |
|---|---|---|---|---|
| `int.py` | `Math.max(int,int)`, `new Integer(int)` | yes | yes | yes |
| `double.py` | `Math.sqrt(double)`, `new Double(double)` | yes | yes | yes |
| `strings.py` | `new String(...)` + `.toString()` | yes | yes | yes |
| `object.py` | plain `Object` identity (arg + return) | yes | yes | yes |
| `dispatch.py` | overload resolution x16, mono + polymorphic | yes | yes | yes |
| `proxy.py` | established callback binding, int + Object arg | yes | -- | yes |
| `array_flat.py` | 1D list->array/buffer->array, 100/1k/10k/100k elements | yes | yes | yes |
| `array_multidim.py` | 2D-5D list->array/buffer->array, fixed element count | yes | yes | yes |
| `classhints.py` | `@JConversion` hint-list cache scan | yes | -- | -- |

`int.py`/`double.py`/`strings.py` are trivial single-overload JDK-builtin
calls -- a baseline with no interesting conversion machinery behind it.
`object.py`/`dispatch.py`/`proxy.py`/`array_*.py` all use the shared
`jpype.benchmark.DeepBench` test class
(test/harness/jpype/benchmark/DeepBench.java -- a plain compiled class
with no jpype dependency, so all three libraries can put test/classes +
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

`classhints.py` is JPype-only (exercises the hint-list cache
specifically; jpy/jep have no `@JConversion`-style extensible hint
mechanism to compare against). Requires the test harness classes
(`jpype.classhints.Custom`/`ClassHintsTest`, built via
`BUILD_TEST_HARNESS=ON`) on the classpath.

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
```
