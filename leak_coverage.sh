#!/bin/sh
# Leak-sweep coverage gap finder.
#
# Answers a narrower question than coverage.sh's normal correctness
# coverage: not "what native/ code is exercised at all", but "what
# native/ code is exercised by the full test suite that NO CURRENT
# test/jpypetest/leak_targets.txt entry ever touches at all". A high
# gap-line count for a file means real, reachable code with zero
# leak-check regression coverage -- a candidate for a new leak_targets.txt
# entry.
#
# This is a *reachability* signal, not a leak measurement: the
# leak-target pass below runs each target test exactly once (plain
# pytest), not leaksweep.py's own repeated-batch budget loop -- that
# keeps this fast enough to run routinely, at the cost of only proving
# "this code path is reachable from a leak target", not "no leak
# targets currently catch a leak in it". Use leaksweep.py itself (or
# `make -f project/dev.mk leak-sweep`) for the real budgeted sweep once
# a candidate line's target is added.
#
# Usage: ./leak_coverage.sh [venv_dir]
#   venv_dir defaults to /tmp/jpype-leak-coverage-venv (disposable, per
#   CLAUDE.md -- never run this against a real/persistent environment).
set -e

VENV="${1:-/tmp/jpype-leak-coverage-venv}"
REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_ROOT"

echo "=== 0. Disposable venv ($VENV) ==="
python3.12 -m venv "$VENV"
PIP="$VENV/bin/pip"
PYTHON="$VENV/bin/python"
"$PIP" install --upgrade pip -q
"$PIP" install -q scikit-build-core pybind11 pytest pytest-randomly numpy gcovr

echo "=== 1. Clean stale build state ==="
make -f project/dev.mk clean >/dev/null 2>&1 || true
rm -f org.jpype.jar

echo "=== 2. Build org.jpype.jar ==="
make -f project/dev.mk jar

echo "=== 3. Build _jpype with coverage instrumentation ==="
"$PIP" install --no-build-isolation -e . \
    --config-settings=cmake.define.BUILD_TEST_HARNESS=ON \
    --config-settings=cmake.define.ENABLE_COVERAGE=ON

echo "=== 4. Full-suite baseline pass ==="
# Coverage data is written by the instrumented binary regardless of
# pass/fail, and this script's whole purpose is the coverage report, not
# a pass/fail gate -- don't let `set -e` abort the whole run over a
# single flaky test (e.g. test_leak.py's fixed-batch heuristics can
# false-positive under this build's -O0/--coverage timing, confirmed
# non-reproducible in isolation more than once). A real, widespread
# build breakage still shows up clearly in the printed pytest summary
# even though the script continues past it.
(cd test && "$PYTHON" -m pytest -q jpypetest) || true
gcovr -r . --filter 'native/' --json build_leak_full.json -s

echo "=== 5. Reset coverage counters ==="
find . -name '*.gcda' -delete

echo "=== 6. Leak-sweep-target-only pass (each target run once, not budgeted) ==="
"$PYTHON" project/tools/leak_target_ids.py test/jpypetest/leak_targets.txt \
    > /tmp/jpype_leak_node_ids.txt
(cd test && "$PYTHON" -m pytest -q $(cat /tmp/jpype_leak_node_ids.txt | tr '\n' ' ')) || true
gcovr -r . --filter 'native/' --json build_leak_targets.json -s

echo "=== 7. Diff: memory-relevant lines full-suite-covered but leak-target-uncovered ==="
# This is the number that actually matters -- raw line coverage across all
# of native/ mixes in mechanical dispatch/wrapper code (jp_convert.cpp's
# dtype switch,
# jp_javaframe.cpp's per-primitive-type JNI passthroughs) that can't leak by
# its nature, understating how well the leak-sweep target set actually covers
# the code that manages references/resources. --memory-only restricts to
# lines near an actual acquire/release (see memory_relevant_lines.py).
"$PYTHON" project/tools/leak_coverage_diff.py build_leak_full.json build_leak_targets.json \
    --memory-only --repo-root .

echo
echo "=== 7b. For comparison, raw (unfiltered) line coverage ==="
"$PYTHON" project/tools/leak_coverage_diff.py build_leak_full.json build_leak_targets.json --top 10

echo
echo "Full JSON reports left at build_leak_full.json / build_leak_targets.json"
echo "for closer inspection (e.g. per-line detail, or --memory-only) with"
echo "project/tools/leak_coverage_diff.py."
