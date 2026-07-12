import jpype
import jpype.imports
import _jpype
import weakref
import itertools

try:
    import numpy as np
    HAVE_NUMPY = True
except ImportError:
    HAVE_NUMPY = False

jpype.startJVM()
from java.lang.reflect import Modifier

def is_static_method(m):
    return Modifier.isStatic(m.getModifiers())

def testProbe(obj):
    interf, methods = _jpype.probe(obj)
    print(f"=== {obj} ===")
    print("interfaces:")
    for i in interf:
        print("  ", i)
    print("methods:")
    for m in methods:
        print("  ", m)

    for i in interf:
        print("--", i)
        for m in i.class_.getDeclaredMethods():
            if m.isDefault():
                continue
            if m.getDeclaringClass() != i.class_:
                continue
            if is_static_method(m):
                continue
            print("  ", m.getName(), str(m.getName()) in methods)
    print()

types_to_check = [
    object,
    type,

    bool,
    int,
    float,
    complex,

    str,
    bytes,
    bytearray,
    memoryview,

    list,
    tuple,
    dict,
    set,
    frozenset,
    range,
    slice,

    enumerate,
    zip,
    type(iter([])),

    BaseException,
    Exception,

    weakref.WeakKeyDictionary,
    weakref.WeakValueDictionary,
]

if HAVE_NUMPY:
    types_to_check.extend([
        np.ndarray,
        np.int32,
        np.float32,
    ])

for t in types_to_check:
    try:
        testProbe(t)
    except Exception as ex:
        print(f"=== {t} FAILED ===")
        print(type(ex).__name__, ex)
        print()
