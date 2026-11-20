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
import gc
import threading
import time
import unittest
import jpype
import _jpype
import common
import subrun


class GCReflectionTestCase(common.JPypeTestCase):
    """Python collections forced by Java collections."""

    def testCallbackRegisteredOnce(self):
        names = [getattr(c, "__name__", "") for c in gc.callbacks]
        self.assertEqual(names.count("_collect"), 1)

    def testStats(self):
        stats = _jpype.gcStats()
        self.assertEqual(set(stats), {"current", "java", "python", "max",
                                      "min", "triggered"})
        for value in stats.values():
            self.assertIsInstance(value, int)
            self.assertGreaterEqual(value, 0)
        self.assertGreaterEqual(stats["max"], stats["min"])
        before = stats["triggered"]
        gc.collect()
        self.assertGreaterEqual(_jpype.gcStats()["triggered"], before)

    def testForcedCollectionsAreThrottled(self):
        # Java garbage alone used to force a full Python collection for
        # nearly every Java collection, which left Python threads in the
        # collector most of the time when the Python heap was large (#1506).
        #
        # Only the collections that Java forces are throttled.  They run on
        # the reference queue thread, so time only collections on other
        # threads than this one.  Collections that this thread's own
        # allocations cause, and the System.gc() that the _collect callback
        # may run when one ends, are outside the throttle and their cost
        # depends on the machine; counting them made the test flaky (#1592).
        main = threading.get_ident()
        forced = []
        started = {}

        def callback(phase, info):
            tid = threading.get_ident()
            if tid == main:
                return
            if phase == "start":
                started[tid] = time.perf_counter()
            elif tid in started:
                forced.append(time.perf_counter() - started.pop(tid))

        heap = [[i, str(i), {i: [i]}] for i in range(200000)]
        gc.collect()
        Array = jpype.JClass("java.lang.reflect.Array")
        byte = jpype.JClass("java.lang.Byte").TYPE
        gc.callbacks.append(callback)
        try:
            # Run for at least 2 s, and on until Java has forced a couple
            # of collections: after a slow forced collection (Windows, a
            # busy agent, an earlier test) the throttle's cool down alone
            # can cover 2 s.  The bound below holds for any run length.
            t0 = time.perf_counter()
            while True:
                Array.newInstance(byte, 1 << 20)
                elapsed = time.perf_counter() - t0
                if elapsed >= 2.0 and len(forced) >= 2 or elapsed >= 20.0:
                    break
            wall = time.perf_counter() - t0
        finally:
            gc.callbacks.remove(callback)
        del heap
        if not forced:
            self.skipTest("Java forced no Python collection in %.1f s" % wall)
        # The throttle keeps forced collections to about one part in five of
        # the wall time; without it they take nearly all of it.
        self.assertLess(sum(forced), 0.5 * wall)


@subrun.TestCase
class GCMemoryTriggerTest(unittest.TestCase):
    """Growth of the process past the collection limit runs a Java
    collection.  This needs a fresh JVM so that the limit is the one set
    when the JVM started."""

    @classmethod
    def setUpClass(cls):
        jpype.startJVM(convertStrings=False)

    def testGrowthRunsJavaCollection(self):
        gc.collect()
        before = _jpype.gcStats()["triggered"]
        hold = []
        for step in range(12):
            # Sizes below the malloc mmap threshold, so that they are visible
            # to every platform's measure of memory use.
            hold.append([bytes([i % 250 + 1]) * 65536 for i in range(100)])
            gc.collect()
        self.assertGreater(_jpype.gcStats()["triggered"], before)
        del hold
