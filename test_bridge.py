import jpype
import jpype.imports
from jpype import *

jpype.startJVM(classpath=[
  "bridge/target/bridge-1.0-SNAPSHOT.jar",
  "test/classes"
])

fixture = JClass("jpype.common.Fixture")()

from java.util import Arrays

jpype._jbridge.initialize()

Bridge = JClass("org.jpype.bridge.Bridge")
Backend = JClass("org.jpype.bridge.Backend")

# Concrete
PyObject = JClass("python.lang.PyObject")
PyJavaObject = JClass("python.lang.PyJavaObject")
PyDict = JClass("python.lang.PyDict")
PyTuple = JClass("python.lang.PyTuple")
PyList = JClass("python.lang.PyList")
PyString = JClass("python.lang.PyString")
PyType = JClass("python.lang.PyType")

# Protocols
PyAttributes = JClass("python.protocol.PyAttributes")
PyMapping = JClass("python.protocol.PyMapping")
PyNumber = JClass("python.protocol.PyNumber")
PySequence = JClass("python.protocol.PySequence")

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

#==========================================================
# Test the backend calls

built = JClass("org.jpype.bridge.BuiltIn")

print(built.list(Arrays.asList("A","B","C")))
print(built.list("A","B","C"))
print(built.str(PyString@"hello"))

class MyTest():
    def __init__(self):
        pass
o = MyTest()
setattr(o, "field", "AA")
print(built.dir(o))
print(type(built.dir(o)))
#print(built.getDict(o))
print(built.hasattr(o, "field"))
print(built.getattr(o, "field"))
print(built.setattr(o, "field", "BB"))
print(o.field=="BB")
print(built.delattr(o, "field"))
print(hasattr(o, "field"))

print(built.isinstance(int(), PyObject[:]((int, float))))
#==========================================================
# Test the object behavior

o = MyTest()
setattr(o, "A", 1)
setattr(o, "B", 2)
j = PyObject@o
a = PyAttributes@(j.asAttributes())
print(dir(a))
print(a.get("A")) # 1
print(a.get("B")) # 2
a.set("B",3)
print(a.has("B")) # True
print(">>")
print(a.get("B")) # 3
print(a.has("C")) # False
print(a.dir())  # List with A, B
print(a.dict()) # Dict with A, B
a.del_("B")  
print(a.has("B"))  #False
#print(a.del_("C"))  # AttributeError

