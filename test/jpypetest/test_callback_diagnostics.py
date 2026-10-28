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
import jpype
from jpype import java, JClass, JObject, JProxy
import common


class CallbackDiagnosticsTestCase(common.JPypeTestCase):
    def testWrappedPythonDiagnostics(self):
        import sys
        import sysconfig
        import threading
        import traceback

        gil_enabled = (sys._is_gil_enabled()
                       if sysconfig.get_config_var('Py_GIL_DISABLED') else None)
        Callable = JClass('java.util.concurrent.Callable')
        Executors = JClass('java.util.concurrent.Executors')
        ExecutionException = JClass('java.util.concurrent.ExecutionException')
        TimeUnit = JClass('java.util.concurrent.TimeUnit')
        barrier = threading.Barrier(2, timeout=5)
        errors = [ValueError('callback-diagnostic:' + str(i)) for i in range(4)]
        causes = [KeyError('original-cause:' + str(i)) for i in range(4)]

        def make_callback(index):
            def diagnostic_origin():
                if gil_enabled is not None:
                    self.assertEqual(sys._is_gil_enabled(), gil_enabled)
                barrier.wait()
                raise errors[index] from causes[index]
            return diagnostic_origin

        proxies = [JObject(JProxy(Callable, dict(call=make_callback(i))), Callable)
                   for i in range(4)]
        executor = Executors.newFixedThreadPool(2)
        try:
            futures = [executor.submit(proxy) for proxy in proxies]
            for index, future in enumerate(futures):
                original_traceback = None
                # Re-reading the same failed future must not mutate the
                # original traceback or add another link to its cause chain.
                for attempt in range(2):
                    with self.assertRaises(ExecutionException) as caught:
                        future.get(10, TimeUnit.SECONDS)
                    wrapped = caught.exception
                    self.assertIsInstance(wrapped.getCause(), java.lang.Throwable)
                    chain, current = [], wrapped
                    while current is not None:
                        self.assertFalse(any(current is previous for previous in chain))
                        chain.append(current)
                        current = current.__cause__
                    self.assertEqual(sum(error is errors[index] for error in chain), 1)
                    self.assertIs(errors[index].__cause__, causes[index])
                    self.assertIsNone(causes[index].__cause__)
                    if attempt == 0:
                        original_traceback = errors[index].__traceback__
                    self.assertIs(errors[index].__traceback__, original_traceback)
                    frames = traceback.extract_tb(original_traceback)
                    self.assertTrue(any(frame.name == 'diagnostic_origin'
                                        and frame.lineno > 0 for frame in frames))
                    formatted = ''.join(traceback.format_exception(wrapped))
                    self.assertIn('ValueError: callback-diagnostic:' + str(index), formatted)
                    self.assertIn('KeyError: ', formatted)
                    self.assertIn('original-cause:' + str(index), formatted)
                    self.assertIn('diagnostic_origin', formatted)
                    self.assertIn('ExecutionException', formatted)
                    for other in range(4):
                        if other != index:
                            self.assertNotIn('callback-diagnostic:' + str(other), formatted)
            if gil_enabled is not None:
                self.assertEqual(sys._is_gil_enabled(), gil_enabled)
        finally:
            executor.shutdownNow()
            self.assertTrue(executor.awaitTermination(5, TimeUnit.SECONDS))
