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
"""Free-threaded bridge contracts in bounded, independently owned JVMs."""
from pathlib import Path
import os
import subprocess
import sys
import sysconfig
import textwrap
import unittest


_START = '''
import concurrent.futures
import faulthandler
import gc
import queue
import sys
import sysconfig
import threading
import weakref
assert sysconfig.get_config_var("Py_GIL_DISABLED") == 1
assert not sys._is_gil_enabled()
import jpype
import _jpype
assert not sys._is_gil_enabled(), "JPype import enabled the GIL"
faulthandler.enable()
faulthandler.dump_traceback_later(45)
jpype.startJVM("-Xcheck:jni", classpath=[sys.argv[1]])
assert not sys._is_gil_enabled()
print("FT-ENV", sys.version, _jpype.__file__, jpype.getJVMVersion(), flush=True)
'''


@unittest.skipUnless(sysconfig.get_config_var("Py_GIL_DISABLED"),
                     "requires a free-threaded CPython build")
class FreeThreadedTestCase(unittest.TestCase):
    def _run(self, scenario):
        script = (_START + '\ntry:\n'
                  + textwrap.indent(textwrap.dedent(scenario), '    ')
                  + '\nfinally:\n    jpype.shutdownJVM()\n'
                  + 'assert not jpype.isJVMStarted()\n'
                  + 'assert not sys._is_gil_enabled()\n'
                  + 'faulthandler.cancel_dump_traceback_later()\n'
                  + 'print("FT-OK", flush=True)\n')
        environment = os.environ.copy()
        # Verify the extension's normal import behavior, without forcing GIL=0.
        environment.pop('PYTHON_GIL', None)
        classes = Path(__file__).resolve().parents[1] / 'classes'
        result = subprocess.run([sys.executable, '-c', script, str(classes)],
                                env=environment, capture_output=True,
                                text=True, timeout=60)
        print(result.stdout, end='')
        print(result.stderr, end='', file=sys.stderr)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('FT-OK', result.stdout)

    def testJavaCallsOverlap(self):
        self._run('''
            Fixture = jpype.JClass("jpype.concurrent.FreeThreadedTest")
            Latch = jpype.JClass("java.util.concurrent.CountDownLatch")
            TimeUnit = jpype.JClass("java.util.concurrent.TimeUnit")
            # Isolate overlap from the separately tested cold metadata cache.
            Fixture.hold(Latch(0), Latch(0))
            entered, release = Latch(2), Latch(1)
            def worker():
                assert not sys._is_gil_enabled()
                Fixture.hold(entered, release)
                assert not sys._is_gil_enabled()
            with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
                futures = [pool.submit(worker) for _ in range(2)]
                try:
                    assert entered.await_(5, TimeUnit.SECONDS), "Java calls did not overlap"
                    assert not any(f.done() for f in futures)
                finally:
                    release.countDown()
                for future in futures:
                    future.result(timeout=10)
        ''')

    def testBlockedJavaCallAllowsPythonGC(self):
        self._run('''
            Fixture = jpype.JClass("jpype.concurrent.FreeThreadedTest")
            Latch = jpype.JClass("java.util.concurrent.CountDownLatch")
            TimeUnit = jpype.JClass("java.util.concurrent.TimeUnit")
            Fixture.hold(Latch(0), Latch(0))
            entered, release = Latch(1), Latch(1)
            with concurrent.futures.ThreadPoolExecutor(max_workers=1) as pool:
                future = pool.submit(Fixture.hold, entered, release)
                try:
                    assert entered.await_(5, TimeUnit.SECONDS)
                    gc.collect()
                    assert not future.done(), "Java wait ended before GC completed"
                finally:
                    release.countDown()
                future.result(timeout=10)
        ''')

    def testParallelJavaCallbacksReenterJava(self):
        self._run('''
            Executors = jpype.JClass("java.util.concurrent.Executors")
            Callable = jpype.JClass("java.util.concurrent.Callable")
            TimeUnit = jpype.JClass("java.util.concurrent.TimeUnit")
            Math = jpype.JClass("java.lang.Math")
            Integer = jpype.JClass("java.lang.Integer")
            Math.addExact(20, 22)
            Integer(0)
            barrier = threading.Barrier(2, timeout=5)
            identities = queue.SimpleQueue()
            def callback():
                assert not sys._is_gil_enabled()
                identities.put(threading.get_ident())
                barrier.wait()
                result = Math.addExact(20, 22)
                assert not sys._is_gil_enabled()
                return Integer(result)
            # Materialize distinct proxies before Java executor threads use them.
            proxies = [jpype.JObject(jpype.JProxy(Callable, dict(call=callback)), Callable)
                       for _ in range(2)]
            executor = Executors.newFixedThreadPool(2)
            try:
                futures = [executor.submit(proxy) for proxy in proxies]
                assert [int(f.get(10, TimeUnit.SECONDS)) for f in futures] == [42, 42]
                assert len({identities.get_nowait(), identities.get_nowait()}) == 2
            finally:
                executor.shutdownNow()
                assert executor.awaitTermination(5, TimeUnit.SECONDS)
        ''')

    def testCallbackFailureThenReuseJavaThread(self):
        self._run('''
            Executors = jpype.JClass("java.util.concurrent.Executors")
            Callable = jpype.JClass("java.util.concurrent.Callable")
            TimeUnit = jpype.JClass("java.util.concurrent.TimeUnit")
            ExecutionException = jpype.JClass("java.util.concurrent.ExecutionException")
            Integer = jpype.JClass("java.lang.Integer")
            identities = []
            raised = threading.Event()
            def callback():
                assert not sys._is_gil_enabled()
                identities.append(threading.get_ident())
                if len(identities) == 2:
                    raised.set()
                    raise ValueError("intentional callback failure")
                return Integer(42)
            proxy = jpype.JObject(jpype.JProxy(Callable, dict(call=callback)), Callable)
            executor = Executors.newSingleThreadExecutor()
            try:
                assert int(executor.submit(proxy).get(5, TimeUnit.SECONDS)) == 42
                try:
                    executor.submit(proxy).get(5, TimeUnit.SECONDS)
                except ExecutionException as ex:
                    assert len(identities) == 2
                    assert raised.is_set() and ex.getCause() is not None
                else:
                    raise AssertionError("callback exception was swallowed")
                assert int(executor.submit(proxy).get(5, TimeUnit.SECONDS)) == 42
                assert len(identities) == 3 and len(set(identities)) == 1
            finally:
                executor.shutdownNow()
                assert executor.awaitTermination(5, TimeUnit.SECONDS)
        ''')

    def testConcurrentJavaBufferExports(self):
        self._run('''
            ByteBuffer = jpype.JClass("java.nio.ByteBuffer")
            WeakReference = jpype.JClass("java.lang.ref.WeakReference")
            expected = bytes(range(64))
            buffer = ByteBuffer.allocateDirect(64)
            buffer.put(jpype.JArray(jpype.JByte)(list(expected)))
            barrier = threading.Barrier(4, timeout=5)
            def worker():
                assert not sys._is_gil_enabled()
                barrier.wait()
                for _ in range(8):
                    view = memoryview(buffer).cast('B')
                    try:
                        assert view.tobytes() == expected
                    finally:
                        view.release()
                return 8
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                assert sum(pool.map(lambda _: worker(), range(4))) == 32
            view = memoryview(buffer).cast('B')
            owner = WeakReference(buffer)
            del buffer
            gc.collect()
            jpype.JClass("java.lang.System").gc()
            try:
                assert owner.get() is not None
                assert view.tobytes() == expected
            finally:
                view.release()
            # Owner retention while exported is checked above; eventual
            # collection after release is not deterministic and is not asserted.
        ''')

    def testWrappedCallbackDiagnosticRecovery(self):
        self._run('''
            import traceback
            Fixture = jpype.JClass("jpype.concurrent.FreeThreadedTest")
            Callable = jpype.JClass("java.util.concurrent.Callable")
            Executors = jpype.JClass("java.util.concurrent.Executors")
            TimeUnit = jpype.JClass("java.util.concurrent.TimeUnit")
            ExecutionException = jpype.JClass("java.util.concurrent.ExecutionException")
            barrier = threading.Barrier(2, timeout=5)
            def make_callback(index):
                def diagnostic_origin():
                    assert not sys._is_gil_enabled()
                    barrier.wait()
                    raise ValueError("diagnostic:" + str(index))
                return diagnostic_origin
            proxies = [jpype.JObject(jpype.JProxy(Callable, dict(call=make_callback(i))), Callable)
                       for i in range(4)]
            executor = Executors.newFixedThreadPool(2)
            try:
                futures = [executor.submit(proxy) for proxy in proxies]
                for index, future in enumerate(futures):
                    try:
                        future.get(10, TimeUnit.SECONDS)
                    except ExecutionException as wrapped:
                        try:
                            # A test-only Java throw exercises the existing
                            # restoration path without decoding native fields.
                            Fixture.rethrow(wrapped.getCause())
                        except ValueError as original:
                            assert type(original) is ValueError
                            assert str(original) == "diagnostic:" + str(index)
                            frames = traceback.extract_tb(original.__traceback__)
                            assert any(frame.name == "diagnostic_origin"
                                       and frame.filename == "<string>" and frame.lineno > 0
                                       for frame in frames)
                            assert not sys._is_gil_enabled()
                        else:
                            raise AssertionError("Python exception was not restored")
                    else:
                        raise AssertionError("callback failure was not propagated")
            finally:
                executor.shutdownNow()
                assert executor.awaitTermination(5, TimeUnit.SECONDS)
        ''')

    def testPythonBufferOwnerWhileJavaReads(self):
        self._run('''
            class Storage(bytearray):
                pass
            source = Storage(range(64))
            owner = weakref.ref(source)
            buffer = jpype.nio.convertToDirectBuffer(source)
            assert buffer.get(0) == 0
            try:
                source.extend(b'x')
            except BufferError:
                pass
            else:
                raise AssertionError("exported storage was resized")
            del source
            gc.collect()
            assert owner() is not None
            barrier = threading.Barrier(4, timeout=5)
            def worker():
                assert not sys._is_gil_enabled()
                barrier.wait()
                assert [int(buffer.get(i)) for i in range(64)] == list(range(64))
                return True
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                assert list(pool.map(lambda _: worker(), range(4))) == [True] * 4
            # No raw memory access is attempted after JVM shutdown.
        ''')
