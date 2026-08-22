#!/bin/bash
# Canonical local coverage entrypoint. This is the one script that should
# ever be updated/extended for coverage work -- do not reconstruct this
# logic ad hoc from plan/*.md notes (those are transient, gitignored, and
# not guaranteed current). See CLAUDE.md.
#
# Combines coverage from THREE languages/toolchains:
#   1. Python (jpype/) -- coverage.py, via the pytest suite below.
#   2. Java (native/jpype_module/src/main/java) -- JaCoCo, from TWO
#      separate test suites that exercise opposite directions of the
#      bridge and are NOT run by the same JVM:
#        a. test/jpypetest (pytest, Python calling into Java) -- always run.
#        b. native/jpype_module's Maven/TestNG suite (Java hosting Python,
#           "reverse embedding", 938 tests as of 2026-08-17) -- attempted;
#           failure here is reported but not fatal to this script, since
#           it's a separate/newer harness than (a) and shouldn't block
#           getting its numbers. If it fails, don't add `-DforkCount=0` or
#           other single-test-class isolation flags to debug it in place
#           -- that changes the suite's execution model (runs the
#           embedded-Python bootstrap inside the already-running Maven JVM
#           instead of a clean fork) and produces misleading crashes
#           unrelated to the real suite (see
#           plan/archive/ReverseEmbeddingBootstrapSegfault.md).
#      The two suites' JaCoCo output can't be `jacoco:merge`'d directly:
#      ant (suite a's org.jpype.jar) and Maven (suite b's target/classes)
#      don't produce CRC-identical classfiles for the same source, so
#      JaCoCo can't correlate one suite's exec data against the other's
#      classes. Instead plan/tools/merge_jacoco_reports.py merges at the
#      method level (a method counts as covered if EITHER suite covered
#      it) -- see that script's docstring for the full rationale.
#   3. C++ (native/common/, native/python/ -- the _jpype.so/_jpyne.so
#      source) -- gcov/gcovr, instrumented via CMake's ENABLE_COVERAGE
#      option. Both test suites above run against the SAME instrumented
#      build (the Maven suite runs against ABI-tagged copies of the exact
#      .so files the pytest suite's venv install also uses), so their
#      .gcda counters accumulate together automatically -- no merge step
#      needed here, unlike the Java side.
#
# Usage: ./coverage.sh [venv_dir]
#   venv_dir defaults to /tmp/jpype-coverage-venv (disposable, per
#   CLAUDE.md -- never run this against a real/persistent environment).

set -e

VENV="${1:-/tmp/jpype-coverage-venv}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$REPO_ROOT"

echo "=== 0. Clean stale build outputs ==="
# Both ant (native/build/classes, the jar suite 1 instruments) and Maven
# (native/jpype_module/target/classes, suite 2) do incremental compiles
# that do NOT delete .class files for source files that were since removed
# -- a coverage report built from either without cleaning first can report
# a deleted class as still 0%-covered dead weight, which is exactly
# backwards for a script whose whole job is telling you what's real.
#
# org.jpype.jar at repo root also needs removing, separately: the
# top-level CMakeLists.txt deliberately CACHES it (`if(EXISTS SRC_JAR) ...
# use from cache` -- lets a plain C++-only build skip needing a JDK at
# all) and will happily keep reusing a stale one indefinitely rather than
# ever re-invoking ant, even after `native/build` itself is removed.
rm -rf native/build native/jpype_module/target build org.jpype.jar

echo "=== 1. Disposable venv + editable build ($VENV), with C++ gcov instrumentation ==="
python3.12 -m venv "$VENV"
"$VENV/bin/pip" install --upgrade pip -q
"$VENV/bin/pip" install -q scikit-build-core pybind11 pytest pytest-randomly pytest-cov numpy build gcovr
"$VENV/bin/pip" install --no-build-isolation -e . \
  --config-settings=cmake.define.BUILD_TEST_HARNESS=ON \
  --config-settings=cmake.define.ENABLE_COVERAGE=ON

PYTAG=$("$VENV/bin/python" -c "import sys; print('cp%d%d' % sys.version_info[:2])")
CPP_BUILD_DIR="$(echo "$REPO_ROOT"/build/$PYTAG-*)"

echo "=== 2. pytest suite (Python -> Java), with --jacoco ==="
mkdir -p build/coverage
# Not fatal to this script (matches the Maven suite below): test_fault.py
# deliberately requires JP_INSTRUMENTATION (this build's ENABLE_COVERAGE
# turns it on) but its fault-injection markers have drifted from actual
# native call sites -- known, pre-existing, unrelated to coverage itself.
# See plan/FaultInjectionMarkerDrift.md.
"$VENV/bin/python" -m pytest -q test/jpypetest \
  --cov=jpype --cov-report=xml:build/coverage/coverage_py.xml --cov-report=term \
  --classpath="native/jpype_module/target/classes:test/classes" \
  --jacoco --checkjni \
  || echo "    pytest reported failures (see above) -- continuing to build the rest of the report."

echo "=== 3. Java report for suite 1 (against the real org.jpype.jar) ==="
rm -rf build/coverage/jar_extract
mkdir -p build/coverage/jar_extract
( cd build/coverage/jar_extract && jar xf "$REPO_ROOT/org.jpype.jar" )
java -jar lib/org.jacoco.cli-0.8.5-nodeps.jar report build/coverage/jacoco.exec \
  --classfiles build/coverage/jar_extract \
  --xml build/coverage/coverage_java_pytest.xml --html build/coverage/java_pytest \
  --sourcefiles native/jpype_module/src/main/java

echo "=== 4. Maven/TestNG suite (Java -> Python, reverse embedding) ==="
MAVEN_OK=0
(
  cd native/jpype_module
  # Do NOT stage ABI-tagged _jpype/_jpyne copies at the repo root here (an
  # earlier version of this script did, matching project/dev.mk's dev-tree
  # convention). This build uses scikit-build-core's editable.mode=redirect
  # (see pyproject.toml), which already makes the venv's installed
  # _jpype/_jpyne/jpype properly importable -- a second copy staged ahead of
  # it on PYTHONPATH shadows the exact file org.jpype.Launcher's
  # System.load() already dlopen'd from site-packages, so the embedded
  # interpreter's own `import _jpype` loads a SECOND, independent copy of
  # the same native code instead of reusing it. That breaks the
  # per-interpreter metaclass identity check in PyJPClass_isWrapperMeta
  # (tp_dealloc compared against a PyJPClass_dealloc resolved from whichever
  # copy happens to be running), surfacing as a 100%-reproducible
  # `SystemError: Missing Java slot on `_jpype._JClass`` crash on the very
  # first class construction of every run. See
  # plan/MissingJavaSlotBootstrapBug.md for the full trace (dladdr-verified:
  # the two addresses resolve to two different .so files on disk).
  mvn -o test -Djpype.nocache=true
) && MAVEN_OK=1 || echo "    Maven suite failed. Skipping its coverage -- see output above."

if [ "$MAVEN_OK" = "1" ]; then
  echo "=== 5. Java report for suite 2, merged into suite 1's ==="
  mvn -o -f native/jpype_module/pom.xml jacoco:report
  python3 plan/tools/merge_jacoco_reports.py \
    build/coverage/coverage_java_pytest.xml \
    native/jpype_module/target/site/jacoco/jacoco.xml \
    --out build/coverage/coverage_java_merged.tsv
  echo "    Merged Java coverage: build/coverage/coverage_java_merged.tsv"
else
  echo "=== 5. Skipped (suite 2 did not run) -- Java coverage below is suite 1 (pytest) ONLY ==="
  echo "    and understates real coverage for anything the reverse-embedding suite exercises"
  echo "    (org.jpype.script.*, FunctionalAdapters, GlobalPool, ReferenceSet, MainInterpreter,"
  echo "    Script, SubInterpreter*, Runner, Launcher -- see native/jpype_module/src/test/java)."
fi

echo "=== 6. C++ coverage (native/common/, native/python/) ==="
# .gcda counters from BOTH suites above have already accumulated against
# this same instrumented build by this point (see the header comment) --
# nothing suite-specific to run here, just report.
mkdir -p build/coverage/cpp
"$VENV/bin/gcovr" -r . --object-directory "$CPP_BUILD_DIR" \
  --filter 'native/common/' --filter 'native/python/' \
  --html-details -o build/coverage/cpp/jpype.html \
  --xml build/coverage/coverage_cpp.xml \
  --print-summary \
  --exclude-unreachable-branches --exclude-throw-branches \
  --gcov-ignore-parse-errors=negative_hits.warn_once_per_file

echo "=== Done ==="
echo "Python coverage:      build/coverage/coverage_py.xml"
echo "Java coverage (pytest-only): build/coverage/coverage_java_pytest.xml / build/coverage/java_pytest/"
if [ "$MAVEN_OK" = "1" ]; then
  echo "Java coverage (merged, both suites): build/coverage/coverage_java_merged.tsv"
fi
echo "C++ coverage:          build/coverage/coverage_cpp.xml / build/coverage/cpp/jpype.html"
