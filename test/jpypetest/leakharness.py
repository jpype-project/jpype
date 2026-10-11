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
"""Shared measurement core for jpype's leak-detection tests.

Originally lived only in test_leak.py; pulled out here so both the
hand-picked fixed-batch tests in test_leak.py and the config-driven,
time-budgeted sweep in leaksweep.py can share the exact same, calibrated
measurement/tolerance logic instead of two copies drifting apart.
"""
import gc
import importlib.util
import os
import sys
import time
import unittest

try:
    import resource
except ImportError:
    resource = None  # type: ignore[assignment]


def haveResource():
    return resource is not None


def startSmallHeapJVM(search_dir):
    """Starts a JVM with the same small-heap args test_leak.py's own
    subrun-isolated setUp() uses, for a generic target that has no such
    setUp of its own (e.g. a bare module-level function, or a test method
    on a class that instead expects pytest's session-scoped `jvm_session`
    fixture, which won't run outside pytest). No-op if a JVM is already
    running in this process.
    """
    import jpype
    if jpype.isJVMStarted():
        return
    root = os.path.dirname(os.path.abspath(search_dir))
    jpype.addClassPath(os.path.join(root, 'classes'))
    # Mirrors conftest.py's jvm_session fixture: JDBC driver jars (sqlite,
    # h2, hsqldb) and other test-only dependencies live here, not under
    # classes/. Without these, any GENERIC target that opens a JDBC
    # connection fails with "No suitable driver found" even though the
    # equivalent test passes fine under pytest.
    jpype.addClassPath(os.path.join(root, '..', 'lib', '*'))
    jpype.addClassPath(os.path.join(search_dir, '..', 'jar', '*'))
    jvm_path = jpype.getDefaultJVMPath()
    classpath_arg = "-Djava.class.path=%s" % jpype.getClassPath()
    jpype.startJVM(jvm_path, "-ea", "-Xmx256M", "-Xms16M", classpath_arg)


def _importModuleFromPath(module_file, search_dir):
    module_name = os.path.splitext(os.path.basename(module_file))[0]
    module_path = os.path.join(search_dir, module_file)
    spec = importlib.util.spec_from_file_location(
        module_name, module_path, submodule_search_locations=[search_dir])
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    if search_dir not in sys.path:
        sys.path.insert(0, search_dir)
    spec.loader.exec_module(module)
    return module


def resolveTarget(module_file, qualname, search_dir):
    """Resolves an *existing, unmodified* target -- a bare module-level
    function (qualname has no '.'), or a 'Class.method' pair -- into a
    zero-arg repeatable callable plus an optional teardown callable.

    For a 'Class.method' pair where Class is a unittest.TestCase subclass,
    the instance's own setUp() is called once up front (not per-call) and
    tearDown() once at the end, mirroring how a real test run would use it,
    but treating the bound test method itself as the repeated unit of work.
    """
    module = _importModuleFromPath(module_file, search_dir)
    parts = qualname.split('.')
    if len(parts) == 1:
        return getattr(module, parts[0]), None
    if len(parts) != 2:
        raise ValueError(
            "malformed target %r, expected 'function' or 'Class.method'" %
            (qualname,))
    clsname, methodname = parts
    obj = getattr(module, clsname)
    if isinstance(obj, type) and issubclass(obj, unittest.TestCase):
        instance = obj(methodname)
        instance.setUp()
        return getattr(instance, methodname), instance.tearDown
    return getattr(obj, methodname), None


def sanityCheckRepeatable(func, label, slowdown_factor=10.0, min_seconds=0.01):
    """Calls func twice, back to back, before real measurement starts.
    Fails fast with a clear error for a target that can't tolerate being
    called in a loop -- raises on repetition (consumed state, a fixture
    that isn't meant to run twice) or grows suspiciously slower on the
    second call (accumulating cost rather than a steady-state operation)
    -- instead of producing a confusing leak signal from a target that was
    never a fair candidate for this harness.
    """
    t0 = time.monotonic()
    try:
        func()
    except Exception as e:
        raise RuntimeError(
            "target %r failed on its first call: %s" % (label, e)) from e
    d0 = time.monotonic() - t0

    t1 = time.monotonic()
    try:
        func()
    except Exception as e:
        raise RuntimeError(
            "target %r is not repeatable (raised on second call): %s" %
            (label, e)) from e
    d1 = time.monotonic() - t1

    if d0 > min_seconds and d1 > d0 * slowdown_factor:
        raise RuntimeError(
            "target %r looks non-repeatable: second call took %.3fs vs "
            "%.3fs for the first (>%gx slower) -- likely accumulating "
            "state rather than a steady-state operation" %
            (label, d1, d0, slowdown_factor))


def runTargetBudget(module_file, qualname, budget_seconds, size=1000,
                     search_dir=None):
    """Generic entry point: wraps an *existing* target (bare function or
    Class.method) named by module_file/qualname into a repeatable,
    time-budgeted leak check, without requiring the target to have been
    written with leak-checking in mind (contrast test_leak.py's
    hand-written stringFunc/classFunc-style closures).

    Returns (label, leaky, batches) for a driver to turn into a report
    line. Raises RuntimeError (via sanityCheckRepeatable) if the target
    isn't fit for this harness at all.
    """
    if search_dir is None:
        search_dir = os.path.dirname(os.path.abspath(module_file))
    label = "%s:%s" % (module_file, qualname)
    func, teardown = resolveTarget(module_file, qualname, search_dir)
    try:
        sanityCheckRepeatable(func, label)
        lc = LeakChecker()
        leaky = lc.memTestBudget(func, size, budget_seconds)
    finally:
        if teardown is not None:
            teardown()
    return (label, leaky)


class LeakChecker:

    def __init__(self):
        import jpype
        self.runtime = jpype.java.lang.Runtime.getRuntime()

    def memory_usage_resource(self):
        # The Python docs aren't clear on what the units are exactly, but
        # the Mac OS X man page for `getrusage(2)` describes the units as bytes.
        # The Linux man page isn't clear, but it seems to be equivalent to
        # the information from `/proc/self/status`, which is in kilobytes.
        if sys.platform == 'darwin':
            rusage_mp = 1
        else:
            rusage_mp = 1024
        return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * rusage_mp

    def freeResources(self):
        self.runtime.gc()  # Garbage collect Java
        gc.collect()  # Garbage collect Python?
        rss_memory = self.memory_usage_resource()
        jvm_total_mem = self.runtime.totalMemory()
        jvm_free_mem = self.runtime.freeMemory()
        num_gc_objects = len(gc.get_objects())
        return (rss_memory, jvm_total_mem, jvm_free_mem)

    def memTest(self, func, size):
        # Returns true if there may be a leak

        # Note, some growth is possible due to loading of objects and classes,
        # Thus we will run it a few times to check the growth rate.

        rss_memory = list()
        jvm_total_mem = list()
        jvm_free_mem = list()
        grow0 = list()
        grow1 = list()

        (rss_memory0, jvm_total_mem0, jvm_free_mem0) = self.freeResources()
        success = 0
        for j in range(10):
            for i in range(size):
                func()
            (rss_memory1, jvm_total_mem1, jvm_free_mem1) = self.freeResources()

            rss_memory.append(rss_memory1)
            jvm_total_mem.append(jvm_total_mem1)
            jvm_free_mem.append(jvm_free_mem1)

            growth0 = (rss_memory1 - rss_memory0) / (float(size))
            growth1 = (jvm_total_mem1 - jvm_total_mem0) / (float(size))
            rss_memory0 = rss_memory1
            jvm_total_mem0 = jvm_total_mem1
            jvm_free_mem0 = jvm_total_mem1

            grow0.append(growth0)
            grow1.append(growth1)

            if (growth0 < 0) or (growth1 < 0):
                continue

            if (growth0 < 4) and (growth1 < 4):
                success += 1

            if success > 3:
                return False

        print()
        for i in range(len(grow0)):
            print('  Pass%d: %f %f  - %d %d %d' %
                  (i, grow0[i], grow1[i], rss_memory[i], jvm_total_mem[i], jvm_free_mem[i]))
        print()
        return True

    def memTestBudget(self, func, size, budget_seconds):
        # Time-budgeted sibling of memTest(): same per-batch growth-rate
        # computation and the same "4 clean batches" minority-suffices
        # tolerance rule (see plan/LeakCheckHarness.md for why that ratio
        # is calibrated, not arbitrary -- a real leak grows in nearly
        # every batch, GC-timing noise only occasionally), but loops
        # batches until budget_seconds of wall-clock has elapsed instead
        # of a fixed 10-batch cap. A larger budget means more batches (more
        # sensitivity), not a longer/bigger single batch.
        rss_memory = list()
        jvm_total_mem = list()
        jvm_free_mem = list()
        grow0 = list()
        grow1 = list()

        (rss_memory0, jvm_total_mem0, jvm_free_mem0) = self.freeResources()
        success = 0
        start = time.monotonic()
        while time.monotonic() - start < budget_seconds:
            for i in range(size):
                func()
            (rss_memory1, jvm_total_mem1, jvm_free_mem1) = self.freeResources()

            rss_memory.append(rss_memory1)
            jvm_total_mem.append(jvm_total_mem1)
            jvm_free_mem.append(jvm_free_mem1)

            growth0 = (rss_memory1 - rss_memory0) / (float(size))
            growth1 = (jvm_total_mem1 - jvm_total_mem0) / (float(size))
            rss_memory0 = rss_memory1
            jvm_total_mem0 = jvm_total_mem1
            jvm_free_mem0 = jvm_total_mem1

            grow0.append(growth0)
            grow1.append(growth1)

            if (growth0 < 0) or (growth1 < 0):
                continue

            if (growth0 < 4) and (growth1 < 4):
                success += 1

            if success > 3:
                return False

        print()
        for i in range(len(grow0)):
            print('  Pass%d: %f %f  - %d %d %d' %
                  (i, grow0[i], grow1[i], rss_memory[i], jvm_total_mem[i], jvm_free_mem[i]))
        print()
        return True
