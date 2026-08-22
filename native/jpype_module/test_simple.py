#!/usr/bin/env python3
import jpype
jpype.startJVM(classpath=['native/jpype_module/target/classes'])

from jpype import JClass
PyDict = JClass("python.lang.PyDict")
PyObject = JClass("python.lang.PyObject")

# Create dict and add string value
d = PyDict()
d.putAny("key1", "value1")

# Try to get - this should return PyObject
print("Calling d.get('key1')...")
try:
    result = d.get("key1")
    print(f"SUCCESS: got {result}, type={type(result)}")
except Exception as e:
    print(f"FAIL: {e}")

print("\nCalling d.getOrDefault('key1', None)...")
try:
    result = d.getOrDefault("key1", None)
    print(f"SUCCESS: got {result}, type={type(result)}")
except Exception as e:
    print(f"FAIL: {e}")

jpype.shutdownJVM()
