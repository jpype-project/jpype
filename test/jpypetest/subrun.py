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
import sys
import traceback
import queue
import unittest
import common

_modules = {}


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


def _execute(inQueue, outQueue):
    while (True):
        datum = inQueue.get()
        if datum == None:
            break
        ex = None
        ret = None
        (func_name, func_file, args, kwargs) = datum
        try:
            module = _import(func_file)
            func = getattr(module, func_name)
            ret = func(*args, **kwargs)
        except Exception as ex1:
            traceback.print_exc()
            ex = ex1
        # This may fail if we get a Java exception so timeout is used
        outQueue.put([ret, ex])


class Client(object):
    def __init__(self):
        self.start()

    def start(self):
        ctx = mp.get_context("spawn")
        self.inQueue = ctx.Queue()
        self.outQueue = ctx.Queue()
        self.process = ctx.Process(target=_execute, args=[
                                   self.inQueue, self.outQueue], daemon=True)
        self.process.start()
        self.timeout = 20

    def execute(self, function, *args, **kwargs):
        self.inQueue.put([function.__name__, os.path.abspath(
            inspect.getfile(function)), args, kwargs])
        try:
            (ret, ex) = self.outQueue.get(True, self.timeout)
        except queue.Empty:
            raise AssertionError("function failed")
        if ex != None:
            raise ex
        return ret

    def restart(self):
        self.stop()
        self.start()

    def stop(self):
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
