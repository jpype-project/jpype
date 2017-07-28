# -*- coding: utf-8 -*-
import jpype
import gc
import sys
import resource
from . import common

try:
    xrange
except NameError:
    xrange = range

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
        (rss_memory0, jvm_total_mem0, jvm_free_mem0) = self.freeResources()
        for i in xrange(size):
            func()
        (rss_memory1, jvm_total_mem1, jvm_free_mem1) = self.freeResources()

        for i in xrange(size):
            func()
        (rss_memory2, jvm_total_mem2, jvm_free_mem2) = self.freeResources()

        for i in xrange(size):
            func()
        (rss_memory3, jvm_total_mem3, jvm_free_mem3) = self.freeResources()


        growth0=((rss_memory3-rss_memory2)/(float(size)))
        growth1=((jvm_total_mem3-jvm_total_mem3)/(float(size)))
        leak=( growth0>4 or growth1>4)
        if leak:
            print('Pre:   %d %d %d'%(rss_memory0, jvm_total_mem0, jvm_free_mem0))
            print('Pass1: %d %d %d'%(rss_memory1, jvm_total_mem1, jvm_free_mem1))
            print('Pass2: %d %d %d'%(rss_memory2, jvm_total_mem2, jvm_free_mem2))
            print('Pass3: %d %d %d'%(rss_memory3, jvm_total_mem3, jvm_free_mem3))
            print("Growth: ",growth0,growth1)
        return leak

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
        self.cls = jpype.JClass('java.lang.String')
        self.string = self.cls("there")
        self.lc = LeakChecker()

    def testStringLeak(self):
        self.assertFalse(self.lc.memTest(stringFunc, 10000))
 
    def testClassLeak(self):
        self.assertFalse(self.lc.memTest(classFunc, 10000))
 
    def testCtorLeak(self):
        self.assertFalse(self.lc.memTest(lambda : ctorFunc(self.cls), 10000))
 
    def testInvokeLeak(self):
        self.assertFalse(self.lc.memTest(lambda : invokeFunc(self.string), 10000))

