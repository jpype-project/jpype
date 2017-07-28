# -*- coding: utf-8 -*-
import jpype
import gc
import sys
from . import common

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

            growth0=(rss_memory1-rss_memory0)/(float(size))
            growth1=(jvm_total_mem1-jvm_total_mem0)/(float(size))
            rss_memory0 = rss_memory1
            jvm_total_mem0 = jvm_total_mem1
            jvm_free_mem0 = jvm_total_mem1

            grow0.append(growth0)
            grow1.append(growth1)

            if ( growth0<0 ) or (growth1<0):
                continue

            if ( growth0<4 ) and ( growth1<4 ):
                success+=1

            if success>3:
                return False

        print()
        for i in xrange(len(grow0)):
            print('  Pass%d: %f %f  - %d %d %d'%(i, grow0[i], grow1[i], rss_memory[i], jvm_total_mem[i], jvm_free_mem[i]))
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
class LeakTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not haveResource():
            return
        self.cls = jpype.JClass('java.lang.String')
        self.string = self.cls("there")
        self.lc = LeakChecker()

    @unittest.skipUnless(haveResource(), "resource not available")
    def testStringLeak(self):
        self.assertFalse(self.lc.memTest(stringFunc, 5000))
 
    @unittest.skipUnless(haveResource(), "resource not available")
    def testClassLeak(self):
        self.assertFalse(self.lc.memTest(classFunc, 5000))
 
    @unittest.skipUnless(haveResource(), "resource not available")
    def testCtorLeak(self):
        self.assertFalse(self.lc.memTest(lambda : ctorFunc(self.cls), 5000))
 
    @unittest.skipUnless(haveResource(), "resource not available")
    def testInvokeLeak(self):
        self.assertFalse(self.lc.memTest(lambda : invokeFunc(self.string), 5000))

