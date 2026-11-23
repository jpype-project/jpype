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
import multiprocessing as mp
import inspect
import os
import pickle
import sys
import time
import traceback
import queue
import unittest
import common
from contextlib import redirect_stdout
import io

_modules = {}  # type: ignore[var-annotated]


def _import(filename):
    import importlib.util
    module_name = os.path.basename(filename)[:-3]
    dirname = os.path.dirname(filename)
    if filename in _modules:
        return _modules[filename]
    spec = importlib.util.spec_from_file_location(
        module_name, filename, submodule_search_locations=[dirname])
    origin = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = origin
    spec.loader.exec_module(origin)
    _modules[filename] = origin
    return origin


class _RemoteTraceback(Exception):
    """Carries the traceback text of an exception raised in the child."""

    def __init__(self, tb):
        super().__init__(tb)
        self.tb = tb

    def __str__(self):
        return self.tb


def _portable(ret, out, ex, tb):
    """Return a result tuple that survives the trip through the queue.

    ``Queue.put`` pickles in a feeder thread; when that fails the item is
    dropped and the parent only sees a timeout.  Java exceptions, for
    example, do not pickle.  Check here and replace what does not survive
    with a plain error that names the original.
    """
    try:
        pickle.loads(pickle.dumps(ex))
    except Exception:
        ex = RuntimeError(f"{type(ex).__qualname__}: {ex}")
    try:
        pickle.loads(pickle.dumps(ret))
    except Exception as ex2:
        if ex is None:
            ex = RuntimeError("unpicklable return value of type "
                              f"{type(ret).__qualname__}: {ex2}")
        ret = None
    return [ret, out, ex, tb]


def _execute(inQueue, outQueue):
    while True:
        datum = inQueue.get()
        if datum is None:
            break
        ex = None
        ret = None
        tb = None
        (func_name, func_file, args, kwargs) = datum
        f = io.StringIO()
        try:
            with redirect_stdout(f):
                module = _import(func_file)
                func = getattr(module, func_name)
                ret = func(*args, **kwargs)
        except Exception as ex1:
            tb = traceback.format_exc()
            sys.stderr.write(tb)
            ex = ex1
        outQueue.put(_portable(ret, f.getvalue(), ex, tb))


class Client(object):
    def __init__(self):
        self.start()

    def start(self):
        ctx = mp.get_context("spawn")
        self.inQueue = ctx.Queue()
        self.outQueue = ctx.Queue()
        self.process = ctx.Process(target=_execute, args=(self.inQueue, self.outQueue), daemon=True)
        self.process.start()
        # leaksweep.py's time-budgeted sweep drives a test method for far
        # longer than the normal fast-suite default below -- it sets this
        # env var before the test runs (individual=True's restart() calls
        # start() again per test) rather than plumbing a parameter through
        # every layer between the sweep driver and this IPC client.
        self.timeout = float(os.environ.get('JPYPE_SUBRUN_TIMEOUT', '20'))

    def execute(self, function, *args, **kwargs):
        self.inQueue.put([function.__name__, os.path.abspath(
            inspect.getfile(function)), args, kwargs])
        what = f"function {function} with args: {args} and kwargs: {kwargs}"
        deadline = time.monotonic() + self.timeout
        while True:
            try:
                (ret, out, ex, tb) = self.outQueue.get(True, 0.25)
                break
            except queue.Empty:
                pass
            if not self.process.is_alive():
                # The result may have been written just before the exit.
                try:
                    (ret, out, ex, tb) = self.outQueue.get(True, 0.5)
                    break
                except queue.Empty:
                    raise AssertionError(
                        f"subprocess died (exit code {self.process.exitcode}) "
                        f"while running {what}") from None
            if time.monotonic() > deadline:
                # Kill it so that later calls fail fast instead of each
                # waiting out the timeout, and stop() does not hang.
                self.process.kill()
                self.process.join()
                raise AssertionError(
                    f"subprocess did not answer within {self.timeout}s "
                    f"while running {what}")
        if out:
            sys.stdout.write(out)
        if ex is not None:
            if tb is not None:
                raise ex from _RemoteTraceback(tb)
            raise ex
        return ret

    def restart(self):
        self.stop()
        self.start()

    def stop(self):
        if self.process.is_alive():
            self.inQueue.put(None)
        self.process.join()

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.stop()
        return False


def TestCase(cls=None, **kwargs):
    """ Decorator that makes tests run in a subprocess """
    if cls:
        return _prepare(cls)

    def modify(cls):
        return _prepare(cls, **kwargs)
    return modify


def _hook(filename, clsname, funcname, *args):
    module = _import(filename)
    cls = getattr(module, clsname)
    inst = '_instance_%s' % cls.__name__
    if not inst in module.__dict__:
        setattr(module, inst, cls())
    inst = getattr(module, inst)
    getattr(inst, funcname)(*args)


def _prepare(orig, individual=False):
    clsname = orig.__name__
    filename = os.path.abspath(inspect.getfile(orig))

    class ProxyClass(orig):
        def __init__(self, *args):
            orig.__init__(self, *args)

        @classmethod
        def tearDownClass(cls):
            ProxyClass._client.execute(
                _hook, filename, clsname, '_tearDownClass')
            ProxyClass._client.stop()

        @classmethod
        def setUpClass(cls):
            ProxyClass._client = Client()
            ProxyClass._client.execute(_hook, filename, clsname, '_setUpClass')

        def setUp(self):
            if common.fast:
                raise unittest.SkipTest("fast")
            if individual:
                ProxyClass._client.restart()
            ProxyClass._client.execute(_hook, filename, clsname, '_setUp')
            if hasattr(self, "setUpLocals"):
                ProxyClass._client.execute(
                    _hook, filename, clsname, '_set', self.setUpLocals())

        def _set(self, dic):
            for k, v in dic.items():
                setattr(self, k, v)

        def tearDown(self):
            ProxyClass._client.execute(_hook, filename, clsname, '_tearDown')

    class ProxyMethod(object):
        def __init__(self, name):
            self.name = name
            self.__name__ = name
            self.__qualname__ = "%s.%s" % (clsname, name)

        def __call__(self):
            ProxyClass._client.execute(_hook, filename, clsname, self.name)

    for k, v in orig.__dict__.items():
        if k.startswith("test"):
            test = ProxyMethod("_" + k)
            test.__name__ = k
            type.__setattr__(ProxyClass, k, test)
            type.__setattr__(ProxyClass, "_" + k, v)

    type.__setattr__(ProxyClass, "_setUp", orig.setUp)
    type.__setattr__(ProxyClass, "_setUpClass", orig.setUpClass)
    type.__setattr__(ProxyClass, "_tearDown", orig.tearDown)
    type.__setattr__(ProxyClass, "_tearDownClass", orig.tearDownClass)

    return ProxyClass
