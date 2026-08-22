"""Sanity check for the graalpy benchmark harness: java.type() classpath
access, numpy (native extension), and DeepBench on test/classes+
test/harness all working together in one GraalPy context.

Usage: run via the Bench launcher with DeepBench on the classpath, e.g.
    java --enable-native-access=ALL-UNNAMED -cp \
        target/classes:target/lib/*:../../../test/classes:../../../test/harness \
        org.jpype.bench.graalpy.Bench smoke_test.py
"""
import java
import numpy as np

System = java.type('java.lang.System')
print('java says:', System.getProperty('java.vm.name'))

arr = np.arange(10, dtype=np.int32)
print('numpy', np.__version__, 'array:', arr, 'sum:', int(arr.sum()))

DeepBench = java.type('jpype.benchmark.DeepBench')
print('DeepBench.sumIntArray([1,2,3]) =', DeepBench.sumIntArray([1, 2, 3]))
