import jpype
import jpype.imports
import sys

jpype.startJVM()

Probe = jpype.JClass("python.lang.Probe")

def show(label, obj):
    try:
        print(f"{label}: {Probe.probe(obj)}")
    except Exception as ex:
        print(f"{label}: FAILED, {type(ex).__name__}: {ex}")


# Core
show("object", object())
show("None", None)

# Strings / binary
show("str", "hello")
show("bytes", bytes([1, 2]))
show("bytearray", bytearray([1, 2]))
show("memoryview(bytes)", memoryview(bytes([1, 2])))

# Containers
show("tuple", (1, 2))
show("dict", {"a": 1})

# Numbers
show("int", 1)
show("float", 1.0)
show("complex", complex(1, 2))
show("bool", True)
show("ellipse", ...)

# Callable / attributes / type-ish
show("callable lambda", lambda x: x)
show("callable function", show)
show("type(float)", type(float))
show("type(object)", type(object))

# Iterator family
show("iter(list)", iter([1, 2]))
show("zip", zip([1, 2], [3, 4]))
show("enumerate", enumerate([1, 2]))
show("slice", slice(0, 5))

# Java object
show("java string", jpype.JString("hello"))
show("throwable", ValueError())
