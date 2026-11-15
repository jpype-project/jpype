.. _jpype_types:

JPype Types
***********

Both Java and Python have a concept of a type.  Every variable refers to an
object which has a defined type.  A type defines the data that the variable is
currently holding and how that variable can be used.  In this chapter we will
learn how Java and Python types relate to one another, how to create import
types from Java, and how to use types to create Java objects.

This chapter covers the Python-calling-Java direction. If you are instead
embedding Python in a Java application, see :doc:`types_java` for the
matching Java-side type hierarchy (``python.lang``'s ``PyObject`` and its
concrete/protocol subtypes).


.. _jpype_types_stay_strong_in_a_weak_language:

Stay strong in a weak language
==============================

Before we get into the details of the types that JPype provides, we first need
to contrast some of the fundamental language differences between Java and
Python.  Python is inherently a weakly typed language.  Any variable can take
any type and the type of a particular variable can change over the
lifetime of a program.  Types themselves can be mutable as you can patch an
existing type to add new behaviors.  Python methods can in principle take any
type of object as an argument, however if the interface is limited it will produce
a TypeError to indicate a particular argument requires a specific type.  Python
objects and classes are open.  Each class and object is basically a dictionary
storing a set of key-value pairs.  Types implemented in native C are often more
closed and thus can't have their method dictionaries or data members altered
arbitrarily.  But subject to a few restrictions based implementation, it is
pretty much the wild west.

In contrast, Java is a strongly typed language.  Each variable can only take
a value of the specified class or a class that derives from the specified
class.  Each Java method takes only a specific number and type of arguments.
The type and number are all checked at compile time to ensure there is
little possibility of error.  As each method requires a specific number and type
of arguments, a method can be overloaded by having two different
implementations which take a different list of types sharing the same method
name. A primitive variable can never hold an object and it can only be converted
to or from other primitive types unless it is specifically cast to that type.
Java objects and classes are completely closed.  The methods and fields for a
particular class and object are defined entirely at compile time.  Though it is
possible to create classes with a dictionary allowing expansion, this is not
the Java norm and no standard mechanism exists.

Thus we need to introduce a few Java terms to the Python vocabulary.  These are
"conversion" and "cast".


.. _jpype_types_java_conversions:

Java conversions
----------------

A conversion is a permitted change from an object of one type to another.
Conversions have four different degrees.  These are: exact, derived, implicit,
and explicit.

Exact conversions are those in which the type of an object is identical.  In
Java each class has only one definition thus there is no need for an exact
conversion.  But when dealing with Python we have objects that are effectively
identical for which exact conversion rules apply.  For example, a Java string
and a Python string both bind equally well to a method which requires a string,
thus this is an exact conversion for the purposes of bind types.

The next level of conversion is derived.  A derived class is one that
descends from a required type.  It is better than implicit but worse than
exact.  If all of the types in a method match are exact or derived then it will
override a method in which one argument is implicit.

The next level of conversion is implicit.  An implicit conversion is one that
Java would perform automatically.  Java defines a number of other conversions
such as converting a primitive to a boxed type or from a boxed type back to a
primitive as implicit conversions.  Python conversions defined by the user are
also considered to be implicit.

Of course not every cast is safe to perform.  For example, converting an object
whose type is currently viewed as a base type to a derived type is not
performed automatically nor is converting from one boxed type to another.  For
those operations the conversion must be explicitly requested, hence these are
explicit conversions.   In Java, a cast is requested by placing the type name
in parentheses in front of the object to be cast.  Python does not directly
support Java casting syntax. To request an explicit conversion an object must
be "cast" using a cast operator @.   Overloaded methods with an explicit
argument will not be matched.  After applying an explicit cast, the match
quality can improve to exact or derived depending on the cast type.

Not every conversion is possible between Java types.  Types that cannot be
converted are considerer to be conversion type "none".

Details on how method overloads are resolved are given in `Method Resolution`_.
Details on the standard conversions provided by JPype are given in the section
`Type Matching`_.

.. _cast:

Java casting
------------

To access a casting operation we use the casting ``JObject`` wrapper.
For example, ``JObject(object, Type)`` would produce a copy with specified type.
The first argument is the object to convert and
the second is the type to cast to.  The second argument should always be a Java
type specified using a class wrapper, a Java class instance, or a string.
Casting will also add a hidden class argument to the resulting object such that
it is treated as the cast type for the duration of that variable lifespan.
Therefore, a variable created by casting is stuck as that type and cannot revert
back to its original for the purposes of method resolution.

The object construction and casting are sometimes a bit blurry.  For example,
when one casts a sequence to a Java list, we will end up constructing a new
Java list that contains the elements of the original Python sequence.  In
general JPype constructors only provide access the Java constructor methods
that are defined in the Java documentation.  Casting on the other hand is
entirely the domain of whatever JPype has defined including user defined casts.

As ``JObject`` syntax is long and does not look much like Java syntax, the
Python matmul operator is overloaded on JPype types such that one can use the
``@`` operator to cast to a specific Java type.   In Java, one would write
``(Type)object`` to cast the variable ``object`` to ``Type``.  In Python, this
would be written as ``Type@object``.   This can also be applied to array types
``JLong[:]@[1,2,3]``, collection types ``Iterable@[1,2,3]`` or Java functors
``DoubleUnaryOperator@(lambda x:x*2)``.  The result of the casting operator
will be a Java object with the desired type or raise a ``TypeError`` if the
cast or conversion is not possible.   For Python objects, the Java object will
generally be a copy as it is not possible to reflect changes in an array back
to Python.  If one needs to retrieve the resulting changes keep a copy of the
converted array before passing it.  For an existing Java object, casting
changes the resolution type for the object.  This can be very useful when
trying to call a specific method overload.   For example, if we have a Java
``a=String("hello")`` and there were an overload of the method ``foo`` between
``String`` and ``Object`` we would need to select the overload with
``foo(java.lang.Object@a)``.

.. _JObject:

Casting is performed through the Python class ``JObject``.  JObject is called
with two arguments which are the object to be cast and the type to cast too.
The cast first consults the conversion table to decide if the cast it permitted
and produces a ``TypeError`` if the conversion is not possible.

``JObject`` also serves as a abstract base class for testing if an object
instance belongs to Java.  All objects that belong to Java will return
true when tested with ``isinstance``.  Like Python's sequence, JObject is an
abstract base class.  No classes actual derive from ``JObject``.

.. _null:

Of particular interest is the concept of Java ``null``.  In Java, null is a
typeless entity which can be placed wherever an object is taken to
indicate that the object is not available.  The equivalent concept in Python is
``None``.  Thus all methods that accept any object type that permit a null will
accept None as an argument with implicit conversion.  However, sometimes it is
necessary to pass an explicit type to the method resolution.  To achieve this
in JPype use ``Type@None`` which will create a null pointer with the
desired type.  To test if something is null we have to compare the handle to
None.  This unfortunately trips up some code quality checkers.  The idiom in
Python is ``obj is None``, but as this only matches things that Python
considers identical, we must instead use ``obj==None``.

Casting ``None`` is used to specify types when calling between overloads
with variadic arguments such as ``foo(Object a)`` and ``foo(Object... many)``.
If we want to call ``foo(None)`` it is ambiguous whether we intend to call the
first with a null object or the second with a null array.  We can resolve the
ambiguity with ``foo(java.lang.Object@None)`` or
``foo(java.lang.Object[:]@None)``

Type enforcement appears in three different places within JPype.  These are
whenever a Java method is called, whenever a Java field is set, and whenever
Python returns a value back to Java.


Primitive Types
===============

Unlike Python, Java makes a distinction between objects and primitive data types.
Primitives represent the minimum data that can be manipulated by a computer. These
stand in contrast to objects, which have the ability to contain any combination of
data types and objects within themselves, and can be inherited from.

Java primitives come in three categories:

- **Logical**: `boolean` (true/false values).
- **Textual**: `char` (single Unicode character).
- **Numerical**: Fixed-point or floating-point numbers of varying sizes.

JPype maps Java primitives to Python classes. To avoid naming conflicts with
Python, JPype prefixes each primitive type with `J` (e.g., `JBoolean`, `JInt`).

.. _jpype_types_primitive_types_jboolean:

JBoolean
--------

Represents a logical value (`True` or `False`). In JPype, `True` and `False` are
exact matches for `JBoolean`. Methods returning a `JBoolean` will always return a
Python `bool`.

.. code-block:: python

   # Example usage
   java_boolean = JBoolean(True)
   print(java_boolean)  # Output: True

.. _jpype_types_primitive_types_jchar:

JChar
-----

Represents a single character. Java `char` types are 16-bit Unicode characters,
but some Unicode characters require more than 16 bits. JPype maps `JChar` to
Python strings of length 1. While `JChar` supports numerical operations, modifying
characters numerically can corrupt their encoding.

.. code-block:: python

   # Example usage
   java_char = JChar('A')
   print(java_char)  # Output: 'A'

.. _jpype_types_primitive_types_jbyte,_jshort,_jint,_jlong:

JByte, JShort, JInt, JLong
--------------------------

These types represent signed integers of varying sizes:

- **JByte**: 8 bits
- **JShort**: 16 bits
- **JInt**: 32 bits
- **JLong**: 64 bits

JPype maps these types to Python's `int`. Methods returning integer primitives will
return Python `int` values. Methods accepting integer primitives will accept Python
integers or any object that can be converted into the appropriate range.

.. code-block:: python

   # Example usage
   java_int = JInt(42)
   print(java_int)  # Output: 42


.. _jpype_types_jfloat_jdouble:

JFloat, JDouble
---------------

These types represent floating-point numbers:

- **JFloat**: 32-bit precision
- **JDouble**: 64-bit precision

JPype maps these types to Python's `float`. Numbers exceeding the range of `JFloat`
or `JDouble` will result in positive or negative infinity. Range checks are
performed when converting Python types, and an `OverflowError` will be raised if
the value is out of bounds.

.. code-block:: python

   # Example usage
   java_double = JDouble(3.14)
   print(java_double)  # Output: 3.14


.. _jpype_types_objects__classes:

Objects & Classes
=================

In contrast to primitive data type, objects can hold any combination of
primitives or objects.  Thus they represent structured data.  Objects can also
hold methods which operate on that data.  Objects can inherit from one another.

However unlike Python, Java objects must have a fixed structure which defines
its type.  These are referred to the object's class.  Here is a point of
confusion.  Java has two different class concepts: the class definition and the
class instance.  When you import a class or refer to a method using the class
name you are accessing the class definition.  When you call ``getClass`` on an
object it returns a class instance.  The class instance is a object whose
structure can be used to access the data and methods that define the class
through reflection.  The class instance cannot directly access the fields or
method within a class but instead provides its own interface for querying the
class.  For the purposes of this document a "class" will refer to the class
definition which corresponds to the Python concept of a class. Wherever the
Java reflection class is being referred to we will use the term "class
instance".  The term "type" is synonymous with a "class" in Java, though often
the term "type" is only used when inclusively discussing the type of primitives
and objects, while the term "class" generally refers to just the types
associated with objects.

All objects in Java inherit from the same base class ``java.lang.Object``, but
Java does not support multiple inheritance.  Thus each class can only inherit
from a single parent.  Multiple inheritance, mix-ins, and diamond pattern are
not possible in Java.  Instead Java uses the concept of an interface.  Any Java
class can inherit as many interfaces as it wants, but these interfaces may not
contain any data elements.  As they do not contain data elements there can
be no ambiguity as to what data a particular lookup.

.. _JInterface:

The meta class ``JInterface`` is used to check if a class type is an interface
using ``isinstance``.  Classes that are pure interfaces cannot be instantiated,
thus, there is no such thing as an abstract instance.  Every value used from
Python still needs real object behavior (``equals``, ``hashCode``,
``toString``, ...) even when its static type is a pure interface with no
object of its own.  To provide this in Python, every interface inherits
``java.lang.Object`` methods even though it does not have
``java.lang.Object`` as a parent.  This ensures that anonymous classes and
lambdas have full object behavior.

.. _jpype_types_classes:

Classes
-------

In JPype, Java classes are instances of the Python ``type`` and function like
any ordinary Python class.  However unlike Python types, Java classes are
closed and cannot be extended.  To enforce extension restrictions, all Java
classes are created from a special private meta class called
``_jpype._JClass``.  This gatekeeper ensures that the attributes of classes
cannot be changed accidentally nor extended.  The type tree of Java is fixed
and closed.

All Java classes have the following functionality.

Class constructor
  The class constructor is accessed by using the Python call syntax ``()``.
  This special method invokes a dispatch whenever the class is called
  as a function.  If a matching constructor is found a new Java instance
  is created and a Python handle to that instance is returned.  In the case
  of primitive types, the constructor creates a Java value with the exact
  type requested.

Get attribute
  The Python ``.`` operator gets an attribute from a class with a specified
  name.  If no method or field exists a ``AttributeError`` will be raised.
  For public static methods, the getattr will produce a Python descriptor which
  can be called to invoke the static method.  For public static fields, a Python
  descriptor will be produced that allows the field to be get or set depending
  on whether the field is final or not.  Public instance methods and instance
  fields will produce a function that can be applied to a Java object to
  execute that method or access the field.  Function accessors are
  non-virtual and thus they can provide access to behaviors that have been
  hidden by a derived class.

Set attribute
  In general, JPype only allows the setting of public non-final fields.  If you
  attempt to set any attribute on an object that does not correspond to a
  settable field it will produce an ``AttributeError``.  There is one exception
  to this rule.  Sometimes it is necessary to attach additional private
  metadata to classes and objects.  Attributes that begin with an underbar are
  considered to be Python private attributes.  These are handled by the
  default Python attribute mechanism, letting them be attached directly to the
  Python handle.  This data is invisible to Java and it is retained only on
  the Python instance.  If an object with Python metadata is passed to Java
  and Java returns the object, the new Python handle will not contain any of
  the attached data, since that data was lost when the object was passed to
  Java.

``class_`` Attribute
  For Java classes there is a special attribute called ``class``.  This
  is a keyword in Python so `name mangling`_ applies.  This is a class instance
  of type ``java.lang.Class``.  It can be used to access fields and methods.

Inner classes
  For methods and fields, public inner classes appear as attributes of
  the class.  These are regular types that can be used to construct objects,
  create array, or cast.

String
  The Java method ``toString`` is mapped into the Python function ``str(obj)``.

Equality
  The Java method ``equals()`` has been mapped to Python ``==`` with special
  augmentations for null pointers.  Java ``==`` is not exposed directly
  as it would lead to numerous errors.  In principle, Java ``==`` should map
  to the Python concept of ``is`` but it is not currently possible to overload
  Python in such a way to achieve the desired effect.

Hash
  The Java method ``hashCode`` is mapped to Python ``hash(obj)`` function.
  There are special augmentations for strings and nulls.  Strings will return
  the same hash code as returned by Python so that Java strings and Python
  strings produce the same dictionary lookups.  Null pointers produce the
  same hash value as None.

  Java defines ``hashCode`` on many objects including mutable ones.  Often
  the ``hashCode`` for a mutable object changes when the object is changed.
  Only use immutable Java object (String, Instant, Boxed types) as
  dictionary keys or risk undefined behavior.

Java objects are instances of Java classes and have all of the methods defined
in the Java class including static members.  However, the get attribute method
converts public instance members and fields into descriptors which act on
the object.

Now that we have defined the basics of Java objects and classes, we will
define a few special classes that operate a bit differently.

.. _jpype_types_array_classes:

Array Classes
-------------

In Java, all arrays are objects, but they cannot define any methods beyond a
limited set of Java array operations. These operations have been mapped into
Python to their closest Python equivalent.

`JArray` is an abstract base class for all Java array classes. Thus, you can
test if something is an array class using ``issubclass``, and check if a Java
object is an array using ``isinstance``.

Creating Array Types
~~~~~~~~~~~~~~~~~~~~

In principle, you can create an array class using ``JClass``, but the signature
required would need to use the proper name as required for the Java method
``java.lang.Class.forName``. Instead, JPype provides two specialized methods to
create array types: arrays may be produced through the factory ``JArray`` or
through the index operator ``[]`` on any `JClass` instance.

.. _JArray:

The signature for `JArray` is ``JArray(type, [dims=1])``. The `type` argument
accepts any Java type, including primitives, and constructs a new array class.
This class can be used to create new instances, cast, or serve as the input to
the array factory. The resulting object has a constructor method that takes
either:

- A number, which specifies the desired size of the array.
- A sequence, which provides the elements of the array. If the members of the
  initializer sequence are not Java objects, each will be converted. If any
  element cannot be converted, a ``TypeError`` will be raised.

As a shortcut, the ``[]`` operator can be used to specify an array type or
create a new instance of an array with a specified length. You can also create
multidimensional arrays or arrays with unspecified dimensions after a specific
point. This applies to both primitive and object types. Because of the number
of options, we will walk through each use case.

To create a one-dimensional array type, append ``[:]`` to any Java class or
primitive type. For example:

- ``JInt[:]`` creates a Java array type for integers.
- ``java.lang.Object[:]`` creates a Java array type for objects.
- ``java.util.List[:]`` creates a Java array type for lists.

Once the array type is created, you can use it to construct arrays, cast Python
sequences to Java arrays, or define multidimensional arrays.

.. code-block:: python

   # Example: Creating array types
   int_array_type = JInt[:]
   object_array_type = java.lang.Object[:]

   # Creating arrays
   int_array = int_array_type([1, 2, 3])
   object_array = object_array_type([None, "Hello", 42])

   print(int_array)  # Output: [1, 2, 3]
   print(object_array)  # Output: [null, Hello, 42]

Multidimensional Arrays
~~~~~~~~~~~~~~~~~~~~~~~

JPype supports the creation of multi-dimensional arrays by appending additional
dimensions using ``[:]``. For example:

- ``JInt[:,:]`` creates a two-dimensional array type for integers.
- ``java.lang.Object[:,:]`` creates a two-dimensional array type for objects.
- ``JDouble[:,:,:]`` creates a three-dimensional array type for double-precision
  floating-point numbers.

When creating multi-dimensional arrays, you can initialize them using nested
Python lists. JPype automatically converts nested lists into the appropriate
Java array structure.

.. code-block:: python

   # Example: Creating multidimensional arrays
   int_2d_array_type = JInt[:, :]
   int_2d_array = int_2d_array_type([[1, 2], [3, 4]])

   print(int_2d_array[0][1])  # Output: 2

   # Creating a 3D array
   double_3d_array_type = JDouble[:, :, :]
   double_3d_array = double_3d_array_type([[[1.1, 2.2], [3.3, 4.4]], [[5.5, 6.6], [7.7, 8.8]]])

   print(double_3d_array[1][0][1])  # Output: 6.6

Jagged Arrays
~~~~~~~~~~~~~

Java supports jagged arrays, which are arrays of arrays with varying lengths.
To create jagged arrays in JPype, replace the final dimension with `[:]`. For
example:

- `JInt[5, :]` creates a jagged array of integers with 5 rows.
- `java.lang.Object[3, :]` creates a jagged array of objects with 3 rows.

Jagged arrays can be initialized using nested Python lists with varying lengths.

.. code-block:: python

   # Example: Creating jagged arrays
   jagged_int_array_type = JInt[3, :]
   jagged_int_array = jagged_int_array_type([[1, 2], [3, 4, 5], [6]])

   print(jagged_int_array[1][2])  # Output: 5

Use of Java Arrays
~~~~~~~~~~~~~~~~~~

Java arrays provide several Python methods:

- **Get Item**:
  Arrays are collections of elements. Array elements can be accessed using the
  Python ``[]`` operator. For multidimensional arrays, JPype uses Java-style
  access with a series of index operations, such as ``jarray[4][2]``.

- **Get Slice**:
  Arrays can be accessed using slices, like Python lists. The slice operator is
  ``[start:stop:step]``. Note that array slices are views of the original array,
  so any alteration to the slice will affect the original array. Use the `clone`
  method to create a copy of the slice if needed.

- **Set Item**:
  Array items can be set using the Python ``[]=`` operator.

- **Set Slice**:
  Multiple array items can be set using a slice assigned with a sequence. The
  sequence must have the same length as the slice. If the items being transferred
  are a buffer, a faster buffer transfer assignment will be used.

- **Buffer Transfer**:
  Buffer transfers from Java arrays work for primitive types. Use Python's
  ``memoryview(jarray)`` function to create a buffer for transferring data.
  Memory views of Java arrays are not writable.

- **Bulk Transfer**:
  For primitive arrays, ``pullTo(dest)`` and ``pushFrom(src)`` bulk-copy
  data in place between the array and an existing, caller-supplied buffer
  (such as a preallocated numpy array), taking the fastest available
  transfer path automatically. This works for arrays of any dimension;
  ``dest``/``src`` need only match the array's total element count, not
  its shape. ``pushFrom`` additionally accepts a source with a different
  dtype, converting as it copies. Note that a buffer previously obtained
  from ``memoryview(jarray)`` and still held open will *not* reflect a
  later ``pushFrom`` on that same array -- a multi-dimensional Java
  array's memory isn't contiguous, so ``memoryview()`` exports a one-time
  snapshot for the life of that export. Release the memoryview (or take a
  fresh one) after mutating the array to see the new data.

- **To List**:
  Use the ``toList()`` method to bulk-convert a Java array into a genuine
  Python list. Multi-dimensional primitive arrays are converted into
  genuinely nested lists.

- **Iteration (For Each)**:
  Java arrays can be used in Python `for` loops and loop comprehensions.

- **Clone**:
  Java arrays can be duplicated using the `clone()` method.

- **Length**:
  Arrays in Java have a defined, immutable length. Use Python's ``len(array)``
  function to get the array length.

Character specialization
~~~~~~~~~~~~~~~~~~~~~~~~

- The Java class `JChar[]` has additional customizations to work better with
  string types.
- Java arrays do not support additional mathematical operations at this time.
- Creating a Java array is required for pass-by-reference syntax when using Java
  methods that modify array contents.

.. code-block:: python

   orig = [1, 2, 3]
   obj = jpype.JInt[:](orig)
   javaInstance.modifies(obj)   # a Java method that doubles each element in place
   orig[:] = obj                # copies the changed values back into the Python list


.. _jpype_types_buffer_classes:

Buffer classes
--------------

In addition to array types, JPype also supports Java ``nio`` buffer types.
Buffers in Java come in two flavors.  Array backed buffers have no special
access.  Direct buffers can be converted to Python buffers with both
read and write capabilities.

Each primitive type in Java has its own buffer type named based on the
primitive type.  ``java.nio.ByteBuffer`` has the greatest control, allowing
any type to be read and written to it.  Buffers in Java function like
memory mapped files and have a concept of a read and write pointer which
is used to traverse the array.  They also have direct index access to their
specified primitive type.

Java buffers provide an additional Python method:

Buffer transfer
  Buffer transfers from a Java buffer work for a direct buffer.  Array backed
  buffers will raise a ``BufferError``.  Use the Python ``memoryview(buffer)``
  function to create a buffer that can be used to transfer any portion of a Java
  buffer out.  Memory views of Java buffers are readable and writable.

Buffers do not currently support element-wise access.


.. _jpype_types_boxed_classes:

Boxed Classes
-------------

Often one wants to be able to place a Java primitive into a method of
fields that only takes an object.  The process of creating an object from a
primitive is referred to as creating a "boxed" object.  The resulting object is
an immutable object which stores just that one primitive.

Java boxed types in JPype are wrapped with classes that inherit from Python
``int`` and ``float`` types as both are immutable in Python.  This means that
a boxed type regardless of whether produced as a return or created explicitly
are treated as Python types. They will obey all the conversion rules
corresponding to a Python type as implicit matches.

In addition, they produce an exact match with their corresponding Java
type. The type conversion for this is somewhat looser than Java.  While Java
provides automatic unboxing of an Integer to a double primitive, JPype can
implicitly convert Integer to a Double boxed.

To box a primitive into a specific type such as to place it into a
``java.util.List`` use ``JObject`` on the desired boxed type or call
the constructor for the desired boxed type directly.  For example:

.. code-block:: python

     lst = java.util.ArrayList()
     lst.add(JObject(JInt(1)))      # Create a Java integer and box it
     lst.add(java.lang.Integer(1))  # Explicitly create the desired boxed object

JPype boxed classes have some additional functionality.  As they inherit from
a mathematical type in Python they can be used in mathematical operations.
But unlike Python numerical types they can take an addition state corresponding
to being equal to a null pointer.  The Python methods are not aware of this
new state and will treat the boxed type as a zero if the value is a null.

To test for null, cast the boxed type to a Python type explicitly and the
result will be checked.  Casting null pointer will raise a ``TypeError``.

.. code-block:: python

     b = JObject(None, java.lang.Integer)
     a = b+0      # This succeeds and a gets the value of zero
     a = int(b)+0 # This fails and raises a TypeError

Boxed objects have the following additional functionality over a normal object.

Convert to index
  Integer boxed types can be used as Python indices for arrays and other
  indexing tasks. This method checks that the value of the boxed
  type is not null.

Convert to int
  Integer and floating point boxed types can be cast into a Python integer
  using the ``int()`` method.  The resulting object is always of type ``int``.
  Casting a null pointer will raise a ``TypeError``.

Convert to float
  Integer and floating point boxed types can be cast into a Python float
  using the ``float()`` method.  The resulting object is always of type
  ``float``.  Casting a null pointer will raise a ``TypeError``.

Comparison
  Integer and floating point types implement the Python rich comparison API.
  Comparisons for null pointers only succeed for ``==`` and ``!=`` operations.
  Non-null boxed types act like ordinary numbers for the purposes of
  comparison.


.. _jpype_types_number_class:

Number Class
------------

The Java class ``java.lang.Number`` is a special type in Java. All numerical
Java primitives and Python number types can convert implicitly into a
Java Number.

========================== ========================
Input                      Result
========================== ========================
None                       java.lang.Number(null)
Python int, float          java.lang.Number
Java byte,   NumPy int8    java.lang.Byte
Java short,  NumPy int16   java.lang.Short
Java int,    NumPy int32   java.lang.Integer
Java long,   NumPy int64   java.lang.Long
Java float,  NumPy float32 java.lang.Float
Java double, NumPy float64 java.lang.Double
========================== ========================

Additional user defined conversion are also applied.  The primitive types
boolean and char and their corresponding boxed types are not considered to
numbers in Java.

.. _java.lang.Object:

Object Class
------------

Although all classes inherit from Object, the object class itself has special
properties that are not inherited.  All Java primitives will implicitly convert
to their box type when placed in an Object.  In addition, a number of Python
types implicitly convert to a Java object.  To convert to a different object
type, explicitly cast the Python object prior to placing in a Java object.

Here a table of the conversions:

================ =======================
Input            Result
================ =======================
None             java.lang.Object(null)
Python str       java.lang.String
Python bool      java.lang.Boolean
Python int       java.lang.Number
Python float     java.lang.Number
================ =======================

In addition it inherits the conversions from ``java.lang.Number``.
Additional user defined conversion are also applied.

.. _java.lang.String:

String Class
------------

The String class in Java is a special representation often pointing either to
a dynamically created string or to a constant pool item defined in the class.
All Java strings are immutable just like Python strings and thus these are
considered to be equivalent classes.

Because Java strings are in fact just pointers to blob of bytes they are
actually slightly less than a full object in some JVM implementation.  This is
a violation of the Object Oriented (OO) principle, never take something away
by inheritance.  Unfortunately, Java is a frequent violator of that rule, so
this is just one of those exceptions you have to trip over.  Therefore, certain
operations such as using a string as a threading control with ``notify`` or ``wait``
may lead to unexpected results.  If you are thinking about using a Java string
in synchronized statement then remember it is not a real object.

Java strings have a number of additional functions beyond a normal
object.

Length
  Java strings have a length measured in the number of characters required
  to represent the string.  Extended Unicode characters
  count for double for the purpose of counting characters.  The string length
  can be determined using the Python ``len(str)`` function.

Indexing
  Java strings can be used as a sequence of characters in Python and thus
  each character can be accessed as using the Python indexing operator ``[]``.

Hash
  Java strings use a special hash function which matches the Python hash code.
  This ensures that they will always match the same dictionary keys as
  the corresponding string in Python.  The Python hash can be determined using
  the Python ``hash(str)`` function.  Null pointers are not currently handled.
  To get the actual Java hash, use ``s.hashCode()``

Contains
  Java strings implement the concept of ``in`` when using the Java method
  ``contains``.  The Java implementation is sufficiently similar that it will
  work fairly well on strings.
  For example, ``"I" in java.lang.String("team")`` would be equal to False.

  Testing other types using the ``in`` operator
  will likely raise a ``TypeError`` if Java is unable to convert the other item
  into something that can be compared with a string.

Concatenation
  Java strings can be appended to create a new string which contains the
  concatenation of the two strings.  This is mapped to the Python operator
  ``+``.

Comparison
  Java strings are compared using the Java method ``compareTo``.  This
  method does not currently handle null and will raise an exception.

For each
  Java strings are treated as sequences of characters and can be used with a
  for-loop construct and with list comprehension.  To iterate through all of the
  characters, use the Python construct ``for c in str:``.

Unfortunately, Java strings do not yet implement the complete list of
requirements to act as Python sequences for the purposes of
``collections.abc.Sequence``.

.. _JString:

The somewhat outdated JString factory is a Python class that pretends to be a
Java string type.  It has the marginal advantage that it can be imported before
the JVM is actually started.  Once the JVM is started, its class representation
is pointed to ``java.lang.String`` and can be used to construct a new string
object or to test if an object is actually a Java string using ``isinstance``.
It does not implement any of the other string methods and just serves as
convenience class.  The more capable ``java.lang.String`` can be imported
in place of JString, but only after the JVM is started.

String objects may optionally convert to Python strings when returned
from Java methods, though this option is a performance issue and can lead to
other difficulties.  This setting is selected when the JVM is started.
See :ref:`String Conversions <string_conversions>` for details (in
:doc:`jvm_py`).

Java strings will cache the Python conversion so we only pay the conversion
cost once per string.


.. _jpype_types_exception_classes:

Exception Classes
-----------------

Both Python and Java treat exception classes differently from other objects.
Only these types may be caught as part of a try block.  Therefore, the
exceptions have a special wrapper.  Most of the mechanics of exceptions happen
under the surface.  The one difference between Python and Java is the behavior
when the argument is queried.  Java arguments can either be the string value, the exception
itself, or the internal construction key depending on how the exception came
into existence.  Therefore, the arguments to a Java exception should never be
used as their values are not guaranteed.

Java exception can report their stacktrace to Python in two different ways.  If
printed through the Python stack trace routine, Java exceptions are split
between the Python code that raised and a phantom Java ``cause`` which contains the
Java exception in Python order.  If the debugging information for the Java
source is enabled, Python may even print the Java source code lines
where the error occurred.  If you prefer Java style stack traces then print the
result from the ``stacktrace()`` method.  Unhandled exception that terminate
the program will print the Python style stack trace information.

.. _JException:

The base class ``JException`` is a special type located in ``jpype.types`` that
can be imported prior to the start of the JVM.  This serves as the equivalent
of ``java.lang.Throwable`` and contains no additional methods.  It is currently
being phased out in favor of catching the Java type directly.

Using ``jpype.JException`` with a class name as a string was supported in
previous JPype versions but is currently deprecated.  For further information
on dealing with exception, see the `Exception Handling`_ section.  To create a
Java exception use JClass or any of the other importing methods.


.. _jpype_types_anonymous_classes:

Anonymous Classes
-----------------

Sometimes Java will produce an anonymous class which does to have any actual
class representation.  These classes are generated when a method implements
a class directly as part of its body and they serve as a closure with access
to some of the variables that were used to create it.

For the purpose of JPype these classes are treated as their parents.  But this
is somewhat problematic when the parent is simply an interface and not an actual
object type.


.. _jpype_types_lambdas:

Lambdas
-------

The companion of anonymous classes are lambda classes.  These are generated
dynamically and their parent is always an interface.  Lambdas are always
Single Abstract Method (SAM) type interfaces.  They can implement additional
methods in the form of default methods but those are generally not accessible
within JPype.

.. _jpype_types_inner_classes:

Inner Classes
-------------

For the most part, inner classes can be used like normal classes, with the
following differences:

- Inner classes in Java natively use $ to separate the outer class from the
  inner class. For example, inner class Foo defined inside class Bar is called
  Bar.Foo in Java, but its real native name is Bar$Foo.
- Inner classes appear as member of the containing class. Thus to access them
  import the outer class and call them as members.
- Non-static inner classes cannot be instantiated from Python code.  Instances
  received from Java code can be used without problem.

.. _jpype_types_buffer_transfers:

Buffer Transfers
----------------
Java arrays provide efficient buffer transfers for primitive types using Python's
`memoryview`. This allows direct use with libraries like NumPy for numerical
operations, without a copy. For strategies to optimize data exchange, 
see :ref:`Optimize Data Transfers <optimize_data_transfers>`.

.. code-block:: python

   # Example: Buffer transfer
   import numpy as np

   int_array = JInt[:](5)
   int_array[:] = [1, 2, 3, 4, 5]  # Transfer data to Java array

   buffer = memoryview(int_array)
   np_array = np.array(buffer)     # Convert to NumPy array

   print(np_array)  # Output: [1, 2, 3, 4, 5]


Importing Java classes
======================

As Java classes are remote from Python and can neither be created nor extended within
Python, they must be imported.  JPype provides three different methods for
creating classes.

The highest level API is the use of the import system.
To import a Java class, one must first import the optional module
``jpype.imports`` which has the effect of binding the Java package system
to the Python module lookup.  Once this is completed package or class can
be imported using the standard Python import system.  The import system
offers a very rich error reporting system.  All failed imports produce
an ``ImportError`` with diagnostics as to what went wrong.  Errors include
unable to find the class, unable to find a required dependency, and incorrect
Java version.

One important caveat when dealing with importing Java modules.  Python always
imports local directories as modules before calling the Java importer.  So any
directory named ``java``, ``com``, or ``org`` will hide corresponding Java
package.  We recommend against naming directories as ``java`` or top level
domain.

.. _JPackage:

The older method of importing a class is with the ``JPackage`` factory.
This factory automatically loads classes as attributes as requested.
If a class cannot be found it will produce an ``AttributeError``.  The
symbols ``java`` and ``javax`` in the ``jpype`` module are both ``JPackage``
instances.  Only public classes appear on ``JPackage`` but protected and even
private classes can be accessed by name.  Though most private classes
don't have any methods or fields that can be accessed.

.. _jpype_types_jclass_factory:

The last mechanism for looking up a class is through the use of the ``JClass``
factory.  This is a low level API allowing the loading of any class available
using the forName mechanism in Java.  The JClass method can take up to three
arguments corresponding to arguments of the forName method and can be used
with alternative class loaders.  The majority of the JPype test bench uses
JClass so that the tests are only evaluating the desired functionality and not
the import system.  But this does not imply that JClass is the preferred
mechanic for importing classes.  The first argument can be a string or
a Java class instance.  There are two keyword arguments ``loader`` and
``initialize``.  The loader can point to an alternative ClassLoader which
is handy when loading custom classes through mechanisms such as over the
web.  A False ``initialize`` argument loads a class without
loading dependencies nor populating static fields.  This option is likely
not useful for ordinary users.  It was provided when calling forName was problematic
due to :ref:`caller-sensitive <caller sensitive>` issues.


.. _name_mangling:

Name mangling
=============

When providing Java package, classes, methods, and fields to Python,
there are occasionally naming conflicts.  For example, if one has a method
called ``with`` then it would conflict with the Python keyword ``with``.
Wherever this occurs, JPype renames the offending symbol with a trailing
under bar.  Java symbols with a leading or trailing under bars are consider to
be privates and may not appear in the JPype wrapper entirely with the exception
of package names.

The following Python words will trigger name mangling of a Java name:

=========== =========== ============= =========== ==========
``False``   ``None``    ``True``      ``and``     ``as``
``async``   ``await``   ``def``       ``del``     ``elif``
``except``  ``exec``    ``from``      ``global``  ``in``
``is``      ``lambda``  ``nonlocal``  ``not``     ``or``
``pass``    ``print``   ``raise``     ``with``    ``yield``
=========== =========== ============= =========== ==========


.. _methods:
.. _jpype_types_method_resolution:


Method Resolution
=================

Because Java supports method overloading and Python does not, JPype wraps Java
methods as a "method dispatch". The dispatch is a collection of all of the
methods from the class and all of its parents which share the same name. The
job of the dispatch is to choose the method to call. Enforcement of the strong
typing of Java must be performed at runtime within Python. Each time a method
is invoked, JPype must match against the list of all possible methods that the
class implements and choose the best possible overload. For this reason, the
methods that appear in a JPype class will not be the actual Java methods, but
rather a "dispatch" whose job is deciding which method should be called based
on the type of the provided arguments. If no method is found that matches the
provided arguments, the method dispatch will produce a ``TypeError``. This is
the exact same outcome that Python uses when enforcing type safety within a
function. If a type doesn't match, a ``TypeError`` will be produced.

Dispatch Example
----------------

When JPype is unable to decide which overload of a method to call, the user
must resolve the ambiguity. This is where casting comes in. Take for example
the ``java.io.PrintStream`` class. This class has a variant of the print and
println methods! So for the following code:

.. code-block:: python

   java.lang.System.out.println(1)

JPype will automatically choose the ``println(long)`` method, because the
Python ``int`` matches exactly with the Java ``long``, while all the other
numerical types are only "implicit" matches. However, if that is not the
version you wanted to call, you must cast it. In this case, we will use a
primitive type to construct the correct type. Changing the line thus:

.. code-block:: python

   java.lang.System.out.println(JByte(1))  # <--- wrap the 1 in a JByte

This tells JPype to choose the byte version. When dealing with Java types,
JPype follows the standard Java matching rules. Types can implicitly grow to
larger types but will not shrink without an explicit cast.

Caching Optimization for Method Resolution
------------------------------------------

JPype optimizes method resolution by caching the results of previous matches.
If the same method is called repeatedly with the same argument types (e.g.,
inside a loop or list comprehension), JPype reuses the cached resolution,
avoiding the overhead of re-evaluating all overloads. This greatly improves
performance for repetitive calls.

For example, consider the following code:

.. code-block:: python

   fruits = ["apple", "orange", "banana"]
   jlist = java.util.ArrayList()
   [jlist.add(fruit) for fruit in fruits]  # Cached resolution for each iteration

In this case, JPype caches the resolution for ``add(str)`` to ``add(String)``
method after the first call, and subsequent calls reuse the cached result. This
optimization is particularly beneficial in loops and list comprehensions.  A
call to ``add(int)`` would trigger a new resolution.  The next call to ``add(str)``
will once again trigger a resolution request.

**Note**: For an in-depth discussion on how this caching mechanism improves
loop performance, particularly in list comprehensions, see the
:ref:`Performance <miscellaneous_topics_performance>` section.

Interactions of Custom Converters and Caching
---------------------------------------------

It is unwise to define very broad conversions as it can interact poorly with 
caching. Suppose that one defined a convertion from all Python strings to the
Java class for date under some condition. Or perhaps an even broader conversion
was defined such as all Python classes that inherit from object.

If such overly broad conversions are applied to a function
for which both date and string were acceptable it were prefer the date
conversion when method resolution starts.  As the type for the cache was string
it would attempt the out of order resolution of date first.  If the 
condition yield a fail it will fall back to normal method resolution, but
an overly broad conversion specialization may end up being dispatched to the 
previously defined conversion.

Under normal operation of JPype the type conversions are narrowly defined such
that the cache will always yield the proper resolution.  But user defined
conversions may cause unexpected results.  In such a case, a cast operation to
the Java type would be required to resolve the ambiguity.



.. _jpype_types_type_matching:

Type Matching
=============

This section provides tables documenting the JPype conversion rules.
JPype defines different levels of "match" between Python objects and Java
types. These levels are:

- **none**, There is no way to convert.
- **explicit (E)**, JPype can convert the desired type, but only
  explicitly via casting.  Explicit conversions are only execute automatically
  in the case of a return from a proxy.
- **implicit (I)**, JPype will convert as needed.
- **exact (X)**, Like implicit, but when deciding with method overload
  to use, one where all the parameters match "exact" will take precedence
  over "implicit" matches.

See the previous section on `Java Conversions`_ for details.

There are special conversion rules for ``java.lang.Object`` and ``java.lang.Number``.
(`Object Class`_ and `Number Class`_)

============== ========== ========= =========== ========= ========== ========== =========== ========= ========== =========== ========= ================== =================
Python\\Java    byte      short       int       long       float     double     boolean     char      String      Array       Object    java.lang.Object   java.lang.Class
============== ========== ========= =========== ========= ========== ========== =========== ========= ========== =========== ========= ================== =================
    int         I [1]_     I [1]_       X          I        I [3]_     I [3]_     X [7]_                                                       I [10]_
   long         I [1]_     I [1]_     I [1]_       X        I [3]_     I [3]_                                                                  I [10]_
   float                                                    I [1]_       X                                                                     I [10]_
 sequence
dictionary
  string                                                                                     I [2]_       X                                    I
  unicode                                                                                    I [2]_       X                                    I
   JByte          X                                                                                                                            I [8]_
  JShort                     X                                                                                                                 I [8]_
   JInt                                 X                                                                                                      I [8]_
   JLong                                           X                                                                                           I [8]_
  JFloat                                                      X                                                                                I [8]_
  JDouble                                                                X                                                                     I [8]_
 JBoolean                                                                           X                                                          I [8]_
   JChar                                                                                       X                                               I [8]_
  JString                                                                                                 X                                    I
  JArray                                                                                                          I/X [4]_                     I
  JObject                                                                                                         I/X [5]_    I/X [6]_         I/X [6]_
  JClass                                                                                                                                       I                  X
 "Boxed"[9]_      I          I          I          I          I          I          I                                                          I
============== ========== ========= =========== ========= ========== ========== =========== ========= ========== =========== ========= ================== =================

.. [1] Conversion will occur if the Python value fits in the Java
       native type.

.. [2] Conversion occurs if the Python string or unicode is of
       length 1.

.. [3] Java defines conversions from integer types to floating point
       types as implicit conversion. Java's conversion rules are based
       on the range and can be lossy.
       See (http://stackoverflow.com/questions/11908429/java-allows-implicit-conversion-of-int-to-float-why)

.. [4] Number of dimensions must match and the types must be
       compatible.

.. [5] Only if the specified type is a compatible array class.

.. [6] The object class is an exact match, otherwise
       implicit.

.. [7] Only the values `True` and `False` are implicitly converted to
       booleans.

.. [8] Primitives are boxed as per Java rules.

.. [9] Java boxed types are mapped to Python primitives, but will
       produce an implicit conversion even if the Python type is an exact
       match. This is to allow for resolution between methods
       that take both a Java primitive and a Java boxed type.

.. [10] Boxed to ``java.lang.Number``


Exception Handling
==================

Error handling is an important part of any non-trivial program. All Java
exceptions occurring within Java code raise a `jpype.JException`, which derives
from Python's `Exception`. These can be caught either using a specific Java
exception or generically as a `jpype.JException` or `java.lang.Throwable`. You
can then use the `stacktrace()`, `str()`, and `args` to access extended
information.

.. _jpype_types_catching_a_specific_java_exception:

Catching a Specific Java Exception
----------------------------------

The following example demonstrates catching a specific Java exception:

.. code-block:: python

    try:
        # Code that throws a java.lang.RuntimeException
    except java.lang.RuntimeException as ex:
        print("Caught the runtime exception:", str(ex))
        print(ex.stacktrace())

.. _jpype_types_catching_multiple_java_exceptions:

Catching Multiple Java Exceptions
---------------------------------

Multiple Java exceptions can be caught together or separately:

.. code-block:: python

    try:
        # Code that may throw various exceptions
    except (java.lang.ClassCastException, java.lang.NullPointerException) as ex:
        print("Caught multiple exceptions:", str(ex))
        print(ex.stacktrace())
    except java.lang.RuntimeException as ex:
        print("Caught runtime exception:", str(ex))
        print(ex.stacktrace())
    except jpype.JException as ex:
        print("Caught base exception:", str(ex))
        print(ex.stacktrace())
    except Exception as ex:
        print("Caught Python exception:", str(ex))

.. _jpype_types_raising_exceptions_from_python_to_java:

Raising Exceptions from Python to Java
--------------------------------------

Exceptions can be raised in proxies to throw an exception back to Java.
Exceptions within the JPype core are issued with the most appropriate Python
exception type, such as `TypeError`, `ValueError`, `AttributeError`, or
`OSError`.

.. _jpype_types_raising_exceptions_in_proxies:

Raising Exceptions in Proxies
-----------------------------

JPype allows Python proxies to raise exceptions that are propagated back to
Java. This is particularly useful when implementing Java interfaces in Python
and handling invalid inputs or unexpected conditions.

When an exception is raised in Python, it is wrapped in a `RuntimeException` in
Java. If the exception propagates back to Python, it is unpacked to return the
original Python exception.

.. _jpype_types_example:

Example
~~~~~~~

The following example demonstrates raising a Python exception from a proxy:

.. code-block:: python

    import jpype
    import jpype.imports

    jpype.startJVM()

    from java.util.function import Function

    @jpype.JImplements(Function)
    class MyFunction:
        @jpype.JOverride
        def apply(self, value):
            if value is None:
                raise ValueError("Invalid input: None is not allowed")
            return value.upper()

    try:
        func = MyFunction()
        result = func.apply(None)  # This will raise a ValueError
    except ValueError as ex:
        print("Caught Python exception:", str(ex))


.. _jpype_types_exception_aliasing:

Exception Aliasing
------------------

Certain exceptions in Java have a direct correspondence with existing Python
exceptions. Rather than forcing JPype to translate these exceptions or
requiring the user to handle Java exception types throughout the code, these
exceptions are "derived" from their Python counterparts. This allows the user
to catch them using standard Python exception types.

+---------------------------------------+------------------+
| Java Exception                        | Python Exception |
+---------------------------------------+------------------+
| `java.lang.IndexOutOfBoundsException` | `IndexError`     |
| `java.lang.NullPointerException`      | `ValueError`     |
+---------------------------------------+------------------+


.. _jpype_types_aliasing_example:

Aliasing Example
~~~~~~~~~~~~~~~~

The following example demonstrates catching an aliased exception:

.. code-block:: python

    try:
        # Code that throws a java.lang.IndexOutOfBoundsException
    except IndexError as ex:
        print("Caught IndexError:", str(ex))

By deriving these exceptions from Python, the user is free to catch the
exception either as a Java exception or as the more general Python exception.
Remember that Python exceptions are evaluated in order from most specific to
least.

