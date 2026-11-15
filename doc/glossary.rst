Freezing, Trivia, and Glossary
********************************

This chapter is direction-neutral -- it applies equally whether Python is
calling Java or Java is embedding Python, so there is no ``_py``/``_java``
pairing for it.


JPype supports freezing and deployment with
`PyInstaller <https://pyinstaller.readthedocs.io/>`_.  The hook is included
with JPype installations and no extra configuration should be needed.


.. _miscellaneous_topics_glossary:

Useless Trivia
==============

Thrameos, the primary developer, maintains the correct pronunciation of
JPype is Jay-Pie-Pee like the word recipe.  His position is that application
of a silent e is inappropriate for this made up name.

1) Jay-Pie is more emblematic of the project goal which is connection of
Java and Python.

2) Rules in English regarding a silent e following a p are poor and depend
mostly on the origin of the word.  Is it like type from the Greek word
typus or French récipé?  As the y in not being modified as it would otherwise
be pronounced as Pie like NumPy and SciPy, that means the only logical
conclusion is that it was intended to be voiced.

3) Jay-pipe or gh-pipe implies that this project is built on pipes (also known
as series of tubes such as the internet).  JPype is based on JNI (Jay-eN-Ey).
If you are looking for a pipe based Java connection use Py4J.  If you are looking
for solution based on JNI use JPypé.





Glossary
========

.. _miscellaneous_topics_glossary_b:

B
-

**Boxed Types**
Immutable Java objects that wrap primitive types (e.g., `java.lang.Integer`
for `int`). Used when primitives need to be treated as objects.

.. _miscellaneous_topics_glossary_c:

C
-

**Caller Sensitive Methods**
Java methods that determine the caller's access level based on the call stack.
JPype uses special mechanisms to handle these methods safely.

**Classpath**
A parameter specifying the location of Java classes or JAR files required by
the JVM. It is essential for loading Java libraries.

.. _miscellaneous_topics_glossary_d:

D
-

**Deferred Proxy**
A proxy that is created before the JVM is started by specifying the interface
as a string and using the `deferred=True` argument. The implementation is
checked only when the proxy is first used.

.. _miscellaneous_topics_glossary_e:

E
-

**Exact Conversion**
A type conversion in JPype where the Python type matches the Java type
exactly. Example: Python `int` to Java `int`.

.. _miscellaneous_topics_glossary_f:

F
-

**Functional Interface**
A Java interface with a single abstract method (SAM). JPype allows Python
callables (e.g., functions, lambdas) to be passed directly to Java methods
expecting a functional interface.

.. _miscellaneous_topics_glossary_g:

G
-

**Garbage Collection (GC)**
The process of automatically reclaiming memory occupied by unused objects.
JPype links Python's and Java's garbage collectors to avoid memory issues.

.. _miscellaneous_topics_glossary_i:

I
-

**Implicit Conversion**
A type conversion in JPype where Python types are automatically converted to
compatible Java types. Example: Python `int` to Java `long`.

**Interface**
A Java construct that defines a set of methods without implementations. JPype
allows Python classes to implement Java interfaces using proxies.


.. _miscellaneous_topics_glossary_m:

M
-

**Mapping**
A Python concept for key-value pairs. JPype customizes Java `Map` classes to
behave like Python dictionaries.

**Multidimensional Arrays**
Java arrays with multiple dimensions. JPype supports creating and working with
these arrays using nested lists.

.. _miscellaneous_topics_glossary_n:

N
-

**NumPy Integration**
JPype's ability to efficiently transfer data between Java arrays and NumPy
arrays using memory buffers.

.. _miscellaneous_topics_glossary_o:

O
-

**Object Instance**
A Java object created from a class. Example: `obj = MyClass()` creates an
instance of `MyClass`.

**Overloaded Methods**
Java methods with the same name but different parameter types or counts. JPype
selects the appropriate overload based on the Python arguments provided.

.. _miscellaneous_topics_glossary_p:

P
-

**Primitive Types**
Basic data types in Java (e.g., `int`, `float`, `boolean`). JPype maps these
types to Python equivalents (e.g., `JInt`, `JFloat`, `JBoolean`).

**Proxy**
A proxy is an intermediary that allows Python objects to implement Java
interfaces. In the context of JPype, Proxies focus solely on providing the
required Java interface functionality.

.. _miscellaneous_topics_glossary_s:

S
-

**SAM (Single Abstract Method)**
A Java interface with a single abstract method. Used in functional programming
and supported by JPype for Python callables.

**Synchronized**
A Java keyword for thread-safe operations. JPype provides the
`jpype.synchronized()` method for similar functionality in Python.

.. _miscellaneous_topics_glossary_t:

T
-

**Type Factory**
A JPype mechanism for creating Java types in Python. Examples include `JClass`
for classes and `JArray` for arrays.

.. _miscellaneous_topics_glossary_u:

U
-

**UnsupportedClassVersionError**
A JVM error indicating that a JAR file was compiled for a newer Java version
than the JVM being used.

.. _miscellaneous_topics_glossary_w:

W
-

**Wrapper**
A wrapper is an object used to represent an object from one programming language
in another programming language, providing a native look and feel. In JPype,
wrappers are used to present Java objects to Python, making them behave like
native Python objects while retaining their underlying Java functionality.
Wrappers encapsulate Java references, such as class instances, arrays, or boxed
types, and may also include proxies when implementing Java interfaces.

The reverse direction -- Java manipulating Python objects -- is also
supported, via the ``python.lang`` wrapper hierarchy (:doc:`types_java`)
rather than this same ``Wrapper`` mechanism.
