# Cross-library call-overhead benchmarks

Compares JPype's general per-call overhead against jpy and jep, to check
whether the JPClass::findJavaConversion caching work
(native/common/include/jp_conversioncache.h) has closed the gap. All
three run the same three operations: `Math.max(int,int)`, `new
Integer(int)`, `new String(...)` + `.toString()`.

`bench_classhints.py` is JPype-only (exercises the hint-list cache
specifically; jpy/jep have no equivalent mechanism to compare against).

`bench_deep_*.py` go past those trivial JDK-builtin calls into deeper
conversion-chain paths, using the shared `jpype.benchmark.DeepBench` test
class (test/harness/jpype/benchmark/DeepBench.java -- a plain compiled
class with no jpype dependency, so all three libraries can put
test/classes + test/harness directly on their classpath and call it):
overload resolution across 16 candidates (both monomorphic and
polymorphic call sites), int[] argument conversion from a fresh Python
list each call, plain-Object identity, and a proxy callback (Java calling
back into an established Python-side binding).

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
  there's no `bench_deep_jpy.py` proxy entry.

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
/tmp/jpype-bench-venv/bin/python project/benchmark/bench_jpype.py
/tmp/jpype-bench-venv/bin/python project/benchmark/bench_classhints.py
/tmp/jpype-bench-venv/bin/python project/benchmark/bench_deep_jpype.py
```

## jpy

Needs a built jpy wheel (see ~/devel/jpy/dist).

```
python3.12 -m venv /tmp/jpy-bench-venv
/tmp/jpy-bench-venv/bin/pip install /path/to/jpy/dist/jpy-*.whl
cd /path/to/jpy && /tmp/jpy-bench-venv/bin/python \
    /path/to/jpype/project/benchmark/bench_jpy.py

# bench_deep_jpy.py takes test/classes and test/harness as explicit args
# since DeepBench needs to be on the JVM classpath jpy starts:
cd /path/to/jpy && /tmp/jpy-bench-venv/bin/python \
    /path/to/jpype/project/benchmark/bench_deep_jpy.py \
    /path/to/jpype/test/classes /path/to/jpype/test/harness
```

(jpy's `jpyutil.init_jvm` locates the JVM relative to cwd/JAVA_HOME; run
from the jpy checkout if init_jvm can't find things.)

## jep

jep embeds CPython *inside* the JVM (opposite direction from
jpype/jpy), so it's launched as a Java process, not `python script.py`,
and its embedded stdout isn't connected to the launcher -- results are
written to a file (see bench_jep.py's out_path arg).

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

PYTHONPATH="$JEP_PKG_PARENT" java -classpath "$JEP_JAR" \
    -Djava.library.path="$JEP_LIB_DIR" jep.Run \
    project/benchmark/bench_jep.py /tmp/bench_jep_results.txt
cat /tmp/bench_jep_results.txt

# bench_deep_jep.py needs DeepBench on the classpath too:
CP="$JEP_JAR:$(pwd)/test/classes:$(pwd)/test/harness"
PYTHONPATH="$JEP_PKG_PARENT" java -classpath "$CP" \
    -Djava.library.path="$JEP_LIB_DIR" jep.Run \
    project/benchmark/bench_deep_jep.py /tmp/bench_deep_jep_results.txt
cat /tmp/bench_deep_jep_results.txt
```
