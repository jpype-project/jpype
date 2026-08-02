# Cross-library call-overhead benchmarks

Compares JPype's general per-call overhead against jpy and jep, to check
whether the JPClass::findJavaConversion caching work
(native/common/include/jp_conversioncache.h) has closed the gap. All
three run the same three operations: `Math.max(int,int)`, `new
Integer(int)`, `new String(...)` + `.toString()`.

`bench_classhints.py` is JPype-only (exercises the hint-list cache
specifically; jpy/jep have no equivalent mechanism to compare against).

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
```

## jpy

Needs a built jpy wheel (see ~/devel/jpy/dist).

```
python3.12 -m venv /tmp/jpy-bench-venv
/tmp/jpy-bench-venv/bin/pip install /path/to/jpy/dist/jpy-*.whl
cd /path/to/jpy && /tmp/jpy-bench-venv/bin/python \
    /path/to/jpype/project/benchmark/bench_jpy.py
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
```
