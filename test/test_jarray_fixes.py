#!/usr/bin/env python
"""
Checks for memory leak in the JVM when pushing array data from Python to Java.
"""

import pytest
import jpype

#!!! These settings are finely tuned !!!
#!!! DO NOT fiddle with them unless you know what you are doing !!!

#   Size of array to be copied.
ARRAY_SIZE = 4000000

#   Number of iterations to run.
#   - raise this value to increase efficacy of the memory leak test.
ITERATIONS = 10

#   Maximum size of JVM heap.
#   - sets a cap to allow memory leak detection.
MAX_JVM_HEAP_SIZE_BYTES = 128647168

def setup_module(module):
    #   Module-level setup.
    if not jpype.isJVMStarted():
        jvm_args = [
            '-Xmx%dM' % (MAX_JVM_HEAP_SIZE_BYTES // 1000 ** 2),
        ]
        jpype.startJVM(jpype.getDefaultJVMPath(), *jvm_args)
    module.JavaDoubleArray = jpype.JArray(jpype.JDouble, 1)

def test_memory_leak_fix():
    """
    This test raises java.lang.VirtualMachineErrorPyRaisable
    (java.lang.OutOfMemoryError: Java heap space) if the memory leak
    is present.

    """
    #   Check memory settings.
    rt = jpype.java.lang.Runtime.getRuntime()
    assert rt.maxMemory() == MAX_JVM_HEAP_SIZE_BYTES

    #   Perform leak test.
    for i in xrange(ITERATIONS):
        print 'iteration:', i
        py_list1 = [float(f) for f in xrange(ARRAY_SIZE)]
        j_array1 = JavaDoubleArray(py_list1)
        py_list2 = j_array1[:]
        assert py_list1 == py_list2

def test_jarray_basic_slicing_fix():
    jl1 = JavaDoubleArray([1., 2., 3.])
    assert list(jl1) == [1., 2., 3.]
    assert list(jl1[0:-1]) == [1., 2.]
    assert list(jl1[0:1]) == [1.]

def test_jarray_slice_copy_fix():
    jl1 = JavaDoubleArray([1., 2., 3.])
    pl1 = jl1[:]
    assert list(jl1) == pl1

def test_jarray_slice_assignment_fix():
    jl2 = JavaDoubleArray([1., 2., 3.])
    jl2[:] = [4., 5., 6.]
    assert list(jl2) == [4., 5., 6.]
