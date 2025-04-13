import jpype
import jpype.imports
from jpype import *

jpype.startJVM(classpath=[
  "bridge/target/bridge-1.0-SNAPSHOT.jar",
  "test/classes"
])

fixture = JClass("jpype.common.Fixture")()
test = JClass("python.lang.Test")()

from java.util import Arrays

jpype._jbridge.initialize()

Bridge = JClass("org.jpype.bridge.Bridge")
Backend = JClass("org.jpype.bridge.Backend")
PyObject = JClass("python.lang.PyObject")
PyJavaObject = JClass("python.lang.PyJavaObject")
PyDict = JClass("python.lang.PyDict")
PyTuple = JClass("python.lang.PyTuple")
PyList = JClass("python.lang.PyList")
PyString = JClass("python.lang.PyString")
PyType = JClass("python.lang.PyType")

print(jpype._jcustomizer.getClassHints(PyTuple))
print(dir(jpype._jcustomizer.getClassHints(PyTuple)))

########################################################
# Check implicit conversions
print("===========================")
print("dict conversion", PyDict._canConvertToJava(dict()))
print("type conversion", PyType._canConvertToJava(type(dict())))
print("object conversion", PyObject._canConvertToJava(object()))
print("list conversion", PyList._canConvertToJava(list()))
print("str conversion", PyString._canConvertToJava(str()))
print("tuple conversion", PyTuple._canConvertToJava(tuple()))

########################################################
# Use implicit conversions
print("===========================")
be = Bridge.getBackend()
p = PyDict@dict()
p = PyType@type(dict())
p = PyString@"a"
p = PyTuple@tuple((1,2))

#print(test.examine(tuple((1,2))))
#print(test.examine(dict()))
#print(test.examine(str()))
#print(test.examine(list()))
#print(test.examine(type(object())))

#==========================================================
# Test the backend calls

print(type(be))
print(dir(be))
print(be.list(Arrays.asList("A","B","C")))
print(be.list("A","B","C"))
print(be.str(PyString@"hello"))

class MyTest():
    def __init__(self):
        pass
o = MyTest()
setattr(o, "field", "AA")
print(be.dir(o))
print(type(be.dir(o)))
print(be.getDict(o))
print(be.hasattr(o, "field"))
print(be.getattr(o, "field"))
print(be.setattr(o, "field", "BB"))
print(o.field=="BB")
print(be.delattr(o, "field"))
print(hasattr(o, "field"))

#==========================================================
# Test the object behavior

o = MyTest()
j = PyObject@o

print(j.asAttributes())

