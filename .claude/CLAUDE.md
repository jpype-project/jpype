# JPype

Python-to-Java bridge. The native extension (`native/common`,
`native/python`, C++) and the Java support jar (`native/jpype_module`, built
with Ant via `native/build.xml`) are built by scikit-build-core/CMake; the
Python layer is in `jpype/`.

## Prerequisites

- A JDK (11+). `javac`/`java` on the `PATH` is enough; `JAVA_HOME` is
  optional.
- `ant` on the `PATH`. Editable builds require it: they build the support jar
  and the test harness from source.
- A C++ compiler and CMake.
- `uv`. `uv pip install` targets the active venv, or `.venv/` in the repo
  root if none is active.

## Build

```sh
uv pip install -e .[tests]
```

- The extra is called `tests`, not `test`.
- Rerun after any change to C++ (`native/common`, `native/python`) or Java
  code (`native/jpype_module`, `test/harness`). An editable build with a JDK
  runs Ant on every build, which recompiles changed Java sources, rewrites
  `org.jpype.jar` in the repo root (where the editable install loads it from)
  and updates `test/classes`. A rebuild with no changes takes a few seconds.
- Pure-Python changes in `jpype/` are picked up without a rebuild (the
  editable install redirects to the checkout).
- A non-editable build (`pip install .`, `uv build`) reuses an existing
  `org.jpype.jar` in the repo root as a cache and does not recompile Java. Run
  an editable build first, or delete that jar, if the Java code changed.
- Ant treats files modified within 1-2 s of the last build as up to date. This
  only matters for scripted edit/build loops.
- Pass extra CMake options with the `CMAKE_ARGS` environment variable.

## Test prerequisites

```sh
uv pip install -r test-requirements.txt   # pytest, xdist, numpy, pandas, ...
```

- On free-threaded Python (`3.14t`), `polars` has no wheel and its source
  build fails, which aborts the whole install. Install without it; the polars
  tests skip themselves:
  `grep -v '^polars' test-requirements.txt > /tmp/req.txt && uv pip install -r /tmp/req.txt`
- The tests need the compiled harness in `test/classes`. The editable build
  creates and updates it; `ant -f test/build.xml` does the same without a
  reinstall.
- `./resolve.sh` downloads extra jars (JDBC drivers for H2, HSQLDB, SQLite,
  Derby etc.) into `lib/` via Ivy. The `test_sql_*` tests need them; run it
  once per checkout.

## Running tests

```sh
cd test
python -m pytest jpypetest --checkjni --fast                # quick run (~20 s)
python -m pytest jpypetest --checkjni -n2 --dist loadfile   # full run, like CI
python -m pytest jpypetest/test_jstring.py --checkjni       # one file
```

With an editable install, running from the repo root
(`python -m pytest test/jpypetest ...`) works too. CI runs from `test/`
because it tests a non-editable install, and the checkout's `jpype/` would
otherwise shadow it.

Custom options (see `test/jpypetest/conftest.py`):

- `--fast`: skip slow tests. This includes the `subrun` tests (which start a
  JVM in a child process) and the `test_sql_*` tests, so use the full run
  before finishing.
- `--checkjni`: start the JVM with `-Xcheck:jni`. CI always uses it.
- `--convertStrings`, `--classpath <jar>`, `--jacoco`.

Notes:

- Only one JVM can be started per process, and it can't be restarted. Tests
  that need different JVM options use `subrun` (child processes).
- Don't re-enable pytest's faulthandler plugin (`-p no:faulthandler` is set in
  `pyproject.toml`): it breaks HotSpot's signal handling during shutdown.
- `pytest-randomly` shuffles the test order. To reproduce an order-dependent
  failure, rerun with the seed pytest prints in the header
  (`--randomly-seed=<n>`); `-p no:randomly` turns shuffling off.
- To debug a native crash (the JVM uses SIGSEGV internally, so don't stop on
  it):
  `gdb -batch -ex 'handle SIGSEGV nostop noprint pass' -ex run -ex bt --args python -m pytest jpypetest/test_x.py --checkjni`

## Type checking

CI runs mypy in an environment **without JPype installed** (Python 3.11):

```sh
uv venv /tmp/mypy-env && . /tmp/mypy-env/bin/activate
uv pip install mypy types-pyinstaller -r test-requirements.txt
mypy ./jpype/ ./test/jpypetest/
```

- Running mypy in the dev venv (with JPype installed) gives ~28 spurious
  `attr-defined` errors on `_jpype`.
- A local `_jpype-stubs/` directory in the repo root is also picked up and
  causes the same errors.
