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
import time
import jpype
import _jpype
import common


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
        # collector most of the time when the Python heap was large.
        spent = [0.0]
        started = [0.0]

        def callback(phase, info):
            if phase == "start":
                started[0] = time.perf_counter()
            else:
                spent[0] += time.perf_counter() - started[0]

        heap = [[i, str(i), {i: [i]}] for i in range(200000)]
        gc.collect()
        Array = jpype.JClass("java.lang.reflect.Array")
        byte = jpype.JClass("java.lang.Byte").TYPE
        gc.callbacks.append(callback)
        try:
            t0 = time.perf_counter()
            while time.perf_counter() - t0 < 2.0:
                Array.newInstance(byte, 1 << 20)
            wall = time.perf_counter() - t0
        finally:
            gc.callbacks.remove(callback)
        del heap
        self.assertLess(spent[0], 0.5 * wall)
