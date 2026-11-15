.. _serialization_with_jpickler:

Serialization with JPickler
****************************

This chapter covers the Python-calling-Java direction only. There is
presently no reverse-direction (Java-embedding-Python) pickling story --
``python.io``/``python.collections``/``python.datetime`` don't expose a
JPickler equivalent, so no ``pickling_java.rst`` is planned. If that
changes, this note should be replaced with a cross-reference.

JPype provides the **JPickler** utility for serializing (pickling) Java
objects into Python-compatible byte streams -- useful for saving Java
objects to disk, transferring them between systems, or capturing their
state for offline debugging. Python's own ``pickle`` module has no support
for Java objects; JPickler bridges that gap.

.. _serialization_with_jpickler_how_jpickler_works:

How JPickler Works
===================

JPickler uses Java's ``Serializable`` interface to serialize Java objects
into a byte stream that can be stored or transferred. Its companion,
**JUnpickler**, deserializes that byte stream back into Java objects. Only
Java objects that implement ``java.io.Serializable`` can be pickled; a
non-serializable object raises an error. Fields marked ``transient`` on the
Java side are excluded, same as with plain Java serialization.

.. _serialization_with_jpickler_example_1_basic_serialization:

Example 1: Basic Serialization
-------------------------------

.. code-block:: python

    import jpype
    import jpype.imports
    from jpype.pickle import JPickler, JUnpickler

    jpype.startJVM()

    java_list = jpype.java.util.ArrayList()
    java_list.add("Hello")
    java_list.add("World")

    with open("serialized_java_list.pkl", "wb") as f:
        JPickler(f).dump(java_list)

    with open("serialized_java_list.pkl", "rb") as f:
        deserialized_list = JUnpickler(f).load()

    print(deserialized_list)  # [Hello, World]

.. _serialization_with_jpickler_example_2_serializing_complex_java_objects:

Example 2: Serializing a Custom Java Class
--------------------------------------------

JPickler can handle any Java object that implements ``java.io.Serializable``,
including your own classes:

.. code-block:: java

    // Save this as MySerializableClass.java and compile it
    import java.io.Serializable;

    public class MySerializableClass implements Serializable {
        private String name;
        private int value;

        public MySerializableClass(String name, int value) {
            this.name = name;
            this.value = value;
        }

        @Override
        public String toString() {
            return "MySerializableClass{name='" + name + "', value=" + value + "}";
        }
    }

.. code-block:: python

    import jpype
    import jpype.imports
    from jpype.pickle import JPickler, JUnpickler

    jpype.startJVM(classpath=["."])

    MySerializableClass = jpype.JClass("MySerializableClass")
    java_object = MySerializableClass("TestObject", 42)

    with open("serialized_java_object.pkl", "wb") as f:
        JPickler(f).dump(java_object)

    with open("serialized_java_object.pkl", "rb") as f:
        deserialized_object = JUnpickler(f).load()

    print(deserialized_object)  # MySerializableClass{name='TestObject', value=42}

.. _serialization_with_jpickler_limitations_of_jpickler:

Limitations
============

- Only classes implementing ``java.io.Serializable`` can be pickled.
- A byte stream produced by one JVM version is not guaranteed to be
  readable by ``JUnpickler`` running against a different JVM version --
  this is a limitation of Java serialization itself, not of JPickler.
- As with any proxy or Java-held Python object, avoid reference loops
  between the pickled object graph and Python-side containers; see
  :ref:`calling_python_code_from_java_reference_loops` for why.

.. _serialization_with_jpickler_pickling_python_objects_with_java_attributes:

Pickling Python Objects with Java Attributes
=============================================

JPickler serializes Java objects themselves, but it does not help with the
more common case of a plain Python object that merely *holds* one or more
Java objects as attributes. Python's standard `pickle` module cannot
serialize those attributes directly, since Java-backed objects are not
picklable.

The fix is the same technique Python uses for any other unpicklable
attribute (open file handles, sockets, and so on): exclude the Java-backed
attributes in `__getstate__`, and rebuild them in `__setstate__` after the
rest of the state has been restored.

.. code-block:: python

    import jpype

    class MyModel:
        # Names of attributes that hold Java objects and must be excluded
        # from pickling.
        _java_attrs = ("java_obj",)

        def __init__(self):
            self.java_obj = jpype.JClass("some.pkg.SomeClass")()

        def __getstate__(self):
            state = self.__dict__.copy()
            for name in self._java_attrs:
                del state[name]
            return state

        def __setstate__(self, state):
            self.__dict__.update(state)
            if not jpype.isJVMStarted():
                jpype.startJVM()
            # Recreate the excluded Java-backed attributes.
            self.java_obj = jpype.JClass("some.pkg.SomeClass")()

Listing the excluded attribute names in one place (`_java_attrs` above)
keeps `__getstate__` from having to special-case each field, and gives
`__setstate__` a single list to regenerate. Exactly how a Java-backed
attribute gets rebuilt is application-specific: it may mean re-running a
constructor, as above, or re-deriving the value from other, already-restored
state on the object.

See `issue #1019 <https://github.com/jpype-project/jpype/issues/1019>`_ for
the original report and workaround this pattern is generalized from.
