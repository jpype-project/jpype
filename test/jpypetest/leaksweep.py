#!/usr/bin/env python
# -*- coding: utf-8 -*-
# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
"""Config-driven, time-budgeted leak-detection sweep.

Generalizes test_leak.py's proven fixed-batch LeakChecker/memTest to a
curated, time-budgeted run across multiple tests (see
plan/LeakCheckHarness.md for the full design rationale). Each target listed
in a config file (default: leak_targets.txt, next to this script) gets run
through leakharness.memTestBudget() for its own configured wall-clock
budget, in its own subrun-isolated, fresh small-heap JVM subprocess -- the
same isolation test_leak.py already uses, just driven from outside pytest
so entries can run for minutes/hours instead of a fast-suite-sized fixed
batch count.

This is deliberately NOT collected by pytest (it is not a test_*.py file
importable as a normal test module) -- it is an opt-in sweep, run directly:

    python leaksweep.py [config_file] [--jobs N]

Entries run concurrently across a process pool (default: one job per 2
cores), since each entry already isolates itself into its own subprocess.

Two config-line shapes are supported, both `:`-separated:

  module.py:ClassName.testMethod:budget_seconds
      Legacy shape (the original 5 curated entries). Targets a
      subrun.TestCase(individual=True)-decorated test method that calls
      assertNotLeaky(closure) itself -- the loop/isolation is the target's
      own subrun-provided JVM restart, driven for budget_seconds via the
      JPYPE_LEAK_BUDGET_SECONDS env var test_leak.py's assertNotLeaky reads.

  GENERIC:module.py:Class.method:budget_seconds[:size]
  GENERIC:module.py:function:budget_seconds[:size]
      Generic shape: wraps an *existing, unmodified* function or test
      method -- one never written with leak-checking in mind, no
      assertNotLeaky call inside it -- via leakharness.runTargetBudget().
      This driver supplies the loop, JVM isolation (its own fresh
      small-heap JVM per entry, started directly rather than relying on
      the target class's own setUp), and measurement itself. The optional
      trailing `size` overrides the default 1000-calls-per-batch (needed
      for a target that is already heavy per call, e.g. one that does its
      own internal allocation loop -- a much smaller size keeps a batch's
      wall-clock cost reasonable).
"""
import argparse
import concurrent.futures
import importlib.util
import multiprocessing
import os
import queue as queue_module
import sys
import time
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_CONFIG = os.path.join(HERE, 'leak_targets.txt')
DEFAULT_TIMEOUT_MARGIN = 30.0  # seconds of IPC slack on top of a target's own budget


def parse_config(config_path):
    """Parses config lines -- see module docstring for the two shapes.

    Returns a list of entries, each either:
      ('legacy', module_file, clsname, methodname, budget_seconds)
      ('generic', module_file, qualname, budget_seconds)
    """
    entries = []
    with open(config_path) as f:
        for lineno, raw_line in enumerate(f, 1):
            line = raw_line.strip()
            if not line or line.startswith('#'):
                continue

            generic = False
            if line.upper().startswith('GENERIC:'):
                generic = True
                line = line[len('GENERIC:'):]

            parts = line.split(':')
            if generic and len(parts) == 4:
                module_file, qualname, budget, size = parts
                entries.append(('generic', module_file, qualname, float(budget), int(size)))
                continue
            if len(parts) != 3:
                raise ValueError(
                    "%s:%d: malformed entry %r, expected "
                    "module.py:ClassName.testMethod:budget_seconds "
                    "(optionally GENERIC:-prefixed, optionally :size-suffixed)" %
                    (config_path, lineno, raw_line.strip()))
            module_file, qualname, budget = parts

            if generic:
                entries.append(('generic', module_file, qualname, float(budget), 1000))
                continue

            qualparts = qualname.split('.')
            if len(qualparts) != 2:
                raise ValueError(
                    "%s:%d: malformed entry %r, expected "
                    "module.py:ClassName.testMethod:budget_seconds" %
                    (config_path, lineno, raw_line.strip()))
            clsname, methodname = qualparts
            entries.append(('legacy', module_file, clsname, methodname, float(budget)))
    return entries


def _import_module(module_file):
    module_name = os.path.splitext(os.path.basename(module_file))[0]
    module_path = os.path.join(HERE, module_file)
    spec = importlib.util.spec_from_file_location(
        module_name, module_path, submodule_search_locations=[HERE])
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


def run_legacy_entry(module_file, clsname, methodname, budget_seconds,
                      timeout_margin=DEFAULT_TIMEOUT_MARGIN):
    """Runs one legacy-shape config entry to completion. Executes in a
    worker process of this module's own pool (see main()) -- sets the env
    vars test_leak.py (JPYPE_LEAK_BUDGET_SECONDS) and subrun.py
    (JPYPE_SUBRUN_TIMEOUT) read, then drives the named test method the same
    way pytest would, relying on the target class's own
    subrun.TestCase(individual=True) decoration for JVM isolation.
    """
    if HERE not in sys.path:
        sys.path.insert(0, HERE)
    os.environ['JPYPE_LEAK_BUDGET_SECONDS'] = str(budget_seconds)
    os.environ['JPYPE_SUBRUN_TIMEOUT'] = str(budget_seconds + timeout_margin)

    module = _import_module(module_file)
    cls = getattr(module, clsname)

    suite = unittest.TestSuite()
    suite.addTest(cls(methodname))
    result = unittest.TestResult()

    label = "%s:%s.%s" % (module_file, clsname, methodname)
    start = time.monotonic()
    suite.run(result)
    elapsed = time.monotonic() - start

    if result.wasSuccessful():
        return (label, True, elapsed, None, None)
    # assertNotLeaky's own AssertionError lands in .failures (a genuine
    # measured leak); anything else (import error, JVM startup failure,
    # ...) lands in .errors -- worth telling apart in the report.
    if result.failures:
        return (label, False, elapsed, result.failures[0][1], 'leak')
    return (label, False, elapsed, result.errors[0][1], 'error')


def _generic_worker(module_file, qualname, budget_seconds, size, result_queue):
    """Runs in its own freshly spawned, dedicated process (see
    run_generic_entry) -- unlike the legacy shape, a generic target has no
    subrun.TestCase of its own to provide JVM isolation, so this process
    starts its own small-heap JVM directly (leakharness.startSmallHeapJVM,
    same -Xmx256M/-Xms16M convention as test_leak.py/conftest.py) before
    resolving and running the target. One process per entry, never reused,
    so there is no risk of a second startJVM() in the same process.
    """
    try:
        import leakharness
        leakharness.startSmallHeapJVM(HERE)
        label, leaky = leakharness.runTargetBudget(
            module_file, qualname, budget_seconds, size=size, search_dir=HERE)
        result_queue.put((label, leaky, None, 'leak' if leaky else None))
    except BaseException as e:  # noqa: BLE001 -- must reach the parent regardless
        import traceback
        result_queue.put((
            "%s:%s" % (module_file, qualname), True, traceback.format_exc(), 'error'))


def run_generic_entry(module_file, qualname, budget_seconds, size=1000,
                       timeout_margin=DEFAULT_TIMEOUT_MARGIN):
    """Runs one GENERIC-shape config entry in its own dedicated subprocess
    (spawned directly by this function, not the legacy shape's
    subrun.Client) so it gets the same "fresh JVM per entry" isolation
    without requiring the target to be a subrun-decorated class.
    """
    label = "%s:%s" % (module_file, qualname)
    ctx = multiprocessing.get_context("spawn")
    result_q = ctx.Queue()
    proc = ctx.Process(
        target=_generic_worker, args=(module_file, qualname, budget_seconds, size, result_q),
        daemon=True)
    start = time.monotonic()
    proc.start()
    try:
        label, is_leaky, message, kind = result_q.get(True, budget_seconds + timeout_margin)
    except queue_module.Empty:
        proc.terminate()
        elapsed = time.monotonic() - start
        return (label, False, elapsed,
                "timed out after %.1fs waiting for a result "
                "(budget %.1fs + %.1fs margin)" %
                (elapsed, budget_seconds, timeout_margin), 'error')
    finally:
        proc.join(timeout=5)
        if proc.is_alive():
            proc.terminate()
    elapsed = time.monotonic() - start
    return (label, not is_leaky, elapsed, message, kind)


def run_entry(entry):
    if entry[0] == 'legacy':
        _, module_file, clsname, methodname, budget_seconds = entry
        return run_legacy_entry(module_file, clsname, methodname, budget_seconds)
    _, module_file, qualname, budget_seconds, size = entry
    return run_generic_entry(module_file, qualname, budget_seconds, size=size)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        'config', nargs='?', default=DEFAULT_CONFIG,
        help="path to a leak-target config file (default: %s)" % DEFAULT_CONFIG)
    parser.add_argument(
        '--jobs', type=int, default=max(1, (os.cpu_count() or 2) // 2),
        help="number of leak-checked entries to run concurrently "
             "(default: one per 2 cores)")
    args = parser.parse_args(argv)

    entries = parse_config(args.config)
    if not entries:
        print("No entries found in %s" % args.config)
        return 1

    failed = []
    sweep_start = time.monotonic()
    with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(run_entry, entry): entry for entry in entries}
        # +submit offsets let a sweep watcher (or, e.g., a later automated
        # check) see actual concurrency from the log directly: entries
        # whose [+submit, +submit+elapsed] windows overlap really did run
        # at the same time, not queued serially behind each other.
        for future, entry in futures.items():
            print("[submit +%.1fs] %s" % (time.monotonic() - sweep_start, entry))
        for future in concurrent.futures.as_completed(futures):
            label, ok, elapsed, message, kind = future.result()
            status = "PASS" if ok else ("ERROR" if kind == 'error' else "LEAK")
            print("[%s +%.1fs] %s (%.1fs)" %
                  (status, time.monotonic() - sweep_start, label, elapsed))
            if not ok:
                failed.append((label, message, kind))

    if failed:
        print()
        print("Failed entries:")
        for label, message, kind in failed:
            print("  [%s] %s" % (kind.upper() if kind else "?", label))
            if message:
                print("    %s" % message.strip().splitlines()[-1])
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
