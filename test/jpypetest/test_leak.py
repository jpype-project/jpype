# -*- coding: utf-8 -*-
import jpype
import gc
import sys
import os
from os import path
import subrun
import unittest

try:
    import resource
except ImportError:
    resource = None
    pass


def haveResource():
    return bool(resource)


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
        for j in range(10):
            for i in range(size):
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
        for i in range(len(grow0)):
            print('  Pass%d: %f %f  - %d %d %d' %
                  (i, grow0[i], grow1[i], rss_memory[i], jvm_total_mem[i], jvm_free_mem[i]))
        print()
        return True


def runLeakChecker(func, counts):
    lc = LeakChecker()
    return lc.memTest(func, 5000)


@subrun.TestCase(individual=True)
class LeakTestCase(unittest.TestCase):

    def setUp(self):
        root = path.dirname(path.abspath(path.dirname(__file__)))
        jpype.addClassPath(path.join(root, 'classes'))
        jvm_path = jpype.getDefaultJVMPath()
        classpath_arg = "-Djava.class.path=%s"
        classpath_arg %= jpype.getClassPath()
        jpype.startJVM(jvm_path, "-ea",
                       # "-Xcheck:jni",
                       "-Xmx256M", "-Xms16M", classpath_arg)

    @unittest.skipUnless(haveResource(), "resource not available")
    def testStringLeak(self):
        def stringFunc():
            jpype.java.lang.String('aaaaaaaaaaaaaaaaa')
        runLeakChecker(stringFunc, 5000)

    @unittest.skipUnless(haveResource(), "resource not available")
    def testClassLeak(self):
        def classFunc():
            cls = jpype.JClass('java.lang.String')
        runLeakChecker(classFunc, 5000)

    @unittest.skipUnless(haveResource(), "resource not available")
    def testCtorLeak(self):
        cls = jpype.JClass("java.lang.String")

        def func():
            cls("test")
        runLeakChecker(func, 5000)

    @unittest.skipUnless(haveResource(), "resource not available")
    def testInvokeLeak(self):
        jstr = jpype.JString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaa")

        def func():
            jstr.getBytes()
        runLeakChecker(func, 5000)
