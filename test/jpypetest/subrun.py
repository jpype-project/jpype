import multiprocessing as mp
import inspect
import os
import sys
import traceback
import queue

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
        ctx = mp.get_context("spawn")
        self.inQueue = ctx.Queue()
        self.outQueue = ctx.Queue()
        self.process = ctx.Process(target=_execute, args=[
                                   self.inQueue, self.outQueue], daemon=True)
        self.process.start()
        self.timeout = 5

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

    def stop(self):
        self.inQueue.put(None)
        self.process.join()

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.stop()
        return False


class assertRaises(object):
    def __init__(self, exc):
        self.exc = exc

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        if issubclass(type, self.exc):
            return True
        frame = sys._getframe(1)
        lines = inspect.getsourcelines(frame)
        info = inspect.getframeinfo(frame)
        msg = []
        msg.append("Assert raises '%s' got '%s' at %s, line %d, in %s" % (
            self.exc.__name__, type.__name__, info.filename, info.lineno, info.function))
        msg.append(lines[0][frame.f_lineno-lines[1]-1])
        raise AssertionError("\n".join(msg))


def assertTrue(logic, *args):
    if bool(logic) is True:
        return
    frame = sys._getframe(1)
    lines = inspect.getsourcelines(frame)
    info = inspect.getframeinfo(frame)
    msg = []
    msg.append("Assert True failed at %s, line %d, in %s" %
               (info.filename, info.lineno, info.function))
    msg.append(lines[0][frame.f_lineno-lines[1]])
    raise AssertionError("\n".join(msg))


def assertFalse(logic, *args):
    if bool(logic) is False:
        return
    frame = sys._getframe(1)
    lines = inspect.getsourcelines(frame)
    info = inspect.getframeinfo(frame)
    msg = []
    msg.append("Assert False failed at %s, line %d, in %s" %
               (info.filename, info.lineno, info.function))
    msg.append(lines[0][frame.f_lineno-lines[1]])
    raise AssertionError("\n".join(msg))


def assertEqual(value1, value2, *args):
    if value1 == value2:
        return
    frame = sys._getframe(1)
    lines = inspect.getsourcelines(frame)
    info = inspect.getframeinfo(frame)
    msg = []
    msg.append("Assertion '%s'=='%s' failed at %s, line %d, in %s" %
               (value1, value2, info.filename, info.lineno, info.function))
    msg.append(lines[0][frame.f_lineno-lines[1]])
    raise AssertionError("\n".join(msg))
