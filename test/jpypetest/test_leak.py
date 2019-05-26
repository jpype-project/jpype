# -*- coding: utf-8 -*-
import jpype
import gc
import sys
import os
from os import path
from subprocess import call

try:
    import unittest2 as unittest
except ImportError:
    import unittest

try:
    import resource
except ImportError:
    resource = None
    pass

try:
    xrange
except NameError:
    xrange = range


def haveResource():
    if resource:
        return True
    return False


def hasRefCount():
    try:
        sys.getrefcount(sys)
        return True
    except:
        return False


class LeakChecker():

    def __init__(self):
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
        for j in xrange(10):
            for i in xrange(size):
                func()
            (rss_memory1, jvm_total_mem1, jvm_free_mem1) = self.freeResources()

            rss_memory.append(rss_memory1)
            jvm_total_mem.append(jvm_total_mem1)
            jvm_free_mem.append(jvm_free_mem1)

            growth0 = (rss_memory1-rss_memory0)/(float(size))
            growth1 = (jvm_total_mem1-jvm_total_mem0)/(float(size))
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
        for i in xrange(len(grow0)):
            print('  Pass%d: %f %f  - %d %d %d' %
                  (i, grow0[i], grow1[i], rss_memory[i], jvm_total_mem[i], jvm_free_mem[i]))
        print()
        return True

# Test functions


def stringFunc():
    jpype.java.lang.String('aaaaaaaaaaaaaaaaa')


def classFunc():
    cls = jpype.JClass('java.lang.String')


def ctorFunc(cls):
    cls("hello")


def invokeFunc(obj):
    obj.getBytes()

# Test case

# These test require a clean JVM, thus we will use subprocess to start a fresh
# copy each time


def subJVM(impl, methodName):

    # if called from a test framework, fork it
    if __name__ != "__main__":
        # In some cases during testing the forked copy finds a previous version of JPype
        os.environ['PYTHONPATH'] = os.getcwd()
        f = call([sys.executable, os.path.realpath(__file__), methodName])
        return f == 0

    # Otherwise run the requested test
    impl()

    return True


class LeakTestCase(unittest.TestCase):

    def runTest(self):
        pass

    def setUp(self):
        if not haveResource():
            return

        if __name__ == "__main__":
            self.cls = jpype.JClass('java.lang.String')
            self.string = self.cls("there")
            self.lc = LeakChecker()

    @unittest.skipUnless(haveResource(), "resource not available")
    def testStringLeak(self):
        def f():
            self.assertFalse(self.lc.memTest(stringFunc, 5000))
        self.assertTrue(subJVM(f, 'testStringLeak'))

    @unittest.skipUnless(haveResource(), "resource not available")
    def testClassLeak(self):
        def f():
            self.assertFalse(self.lc.memTest(classFunc, 5000))
        self.assertTrue(subJVM(f, 'testClassLeak'))

    @unittest.skipUnless(haveResource(), "resource not available")
    def testCtorLeak(self):
        def f():
            self.assertFalse(self.lc.memTest(lambda: ctorFunc(self.cls), 5000))
        self.assertTrue(subJVM(f, 'testCtorLeak'))

    @unittest.skipUnless(haveResource(), "resource not available")
    def testInvokeLeak(self):
        def f():
            self.assertFalse(self.lc.memTest(
                lambda: invokeFunc(self.string), 5000))
        self.assertTrue(subJVM(f, 'testInvokeLeak'))

    @unittest.skipUnless(hasRefCount(), "no refcount")
    def testRefCountCall(self):
        def f():
            obj = jpype.JString("help me")
            initialObj = sys.getrefcount(obj)
            initialValue = sys.getrefcount(obj.__javavalue__)
            for i in range(0, 100):
                obj.charAt(0)
            self.assertTrue(sys.getrefcount(obj)-initialObj < 5)
            self.assertTrue(sys.getrefcount(
                obj.__javavalue__)-initialValue < 5)

            initialObj = sys.getrefcount(obj)
            initialValue = sys.getrefcount(obj.__javavalue__)
            for i in range(0, 100):
                obj.compareTo(obj)
            self.assertTrue(sys.getrefcount(obj)-initialObj < 5)
            self.assertTrue(sys.getrefcount(
                obj.__javavalue__)-initialValue < 5)
        self.assertTrue(subJVM(f, 'testRefCountCall'))


if __name__ == "__main__":
    # Launch jpype with a clean JVM
    root = path.dirname(path.abspath(path.dirname(__file__)))
    jpype.addClassPath(path.join(root, 'classes'))
    jvm_path = jpype.getDefaultJVMPath()
    classpath_arg = "-Djava.class.path=%s"
    classpath_arg %= jpype.getClassPath()
    jpype.startJVM(jvm_path, "-ea",
                   # "-Xcheck:jni",
                   "-Xmx256M", "-Xms16M", classpath_arg)

    # Execute the requested test case
    ltc = LeakTestCase()
    ltc.setUp()
    getattr(ltc, sys.argv[1])()

    # Return 0 on success
    exit(0)
