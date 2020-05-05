# This is the audit script for checking the GC linker.
#
# It beats to memory to see if everything holds up,  but not suitable for
# the test suite.  It is run in conjuction with the printf statement in
# jp_gc.cpp to see how aggressive we are being about running the gc.
# The goal is to keep the memory as low as possible while still maintaining
# good speed.  It has to be checked with 3 different block sizes to
# verify function (<1k, 10k, >1Mb) as different behaviors occur at different
# usage points.

from os import path
import _jpype
import jpype
from jpype.types import *
import numpy as np
import gc
import time

# print(gc.callbacks)
#
# def callHook(*args):
#    jpype.java.lang.System.gc()
#
# gc.callbacks.append(callHook)

trials = 100000
tally = np.zeros((trials,), dtype=np.int8)


class DestructionTracker:
    del_calls = 0
    init_calls = 0

    def __init__(self, i, obj):
        self.index = i
        self.obj = obj
        DestructionTracker.init_calls += 1
        tally[i] = 1
        super().__init__()

    def __del__(self):
        tally[self.index] += 1
        DestructionTracker.del_calls += 1

    def callback(self, message):
        pass


if __name__ == '__main__':
    jpype.startJVM(classpath=['test/classes', 'project/jpype_java/dist/*'],
                   convertStrings=True)

    print()
    kB = (1024 / 8)
    MB = (1024**2 / 8)

    fixture = JClass("jpype.common.Fixture")()
    for i in range(trials):
        x = np.arange(int(10 * kB), dtype=np.int64)

        interface = jpype.JProxy("java.io.Serializable",
                                 dict={'callback': DestructionTracker(i, x).callback})
        interface_container = fixture.callObject(interface)

        if (i % 1000) == 0:
            stats = _jpype.gcStats()
            print("created=", DestructionTracker.init_calls,
                  "  destroyed=", DestructionTracker.del_calls,
                  "  delta=", DestructionTracker.init_calls - DestructionTracker.del_calls,
                  "  current=", stats['current'],
                  "  min=", stats['min'],
                  "  max=", stats['max'],
                  "  triggered=", stats['triggered'],
                  )
            time.sleep(1)
#        print(_jpype.gcStats())
        del interface, interface_container
#        if DestructionTracker.del_calls != 0:
#            print(f'{i} We have deleted something: {DestructionTracker.del_calls}')
#        else:
#            print(f'{i} Still no deletion on it {i}\n', end="")
    print()
print(np.sum(tally == 2))
jpype.shutdownJVM()
print(np.sum(tally == 2))
