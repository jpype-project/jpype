.. _collections:

Collections
***********

JPype uses customizers to augment Java collection classes to operate like Python
collections. Enhanced objects include ``java.util.List``, ``java.util.Set``,
``java.util.Map``, and ``java.util.Iterator``. These classes generally comply
with the Python API except in cases where there is a significant name conflict.
This section details the integration of Java collections with Python constructs.

This chapter covers the Python-calling-Java direction. If you are instead
embedding Python in a Java application, see :doc:`collections_java` for the
matching Java-side ``python.collections`` front end.

.. _collections_specialized_collection_wrappers:

Specialized Collection Wrappers
===============================

JPype customizes Java collection classes to behave like Python collections,
making them intuitive for Python developers. This includes support for iteration,
indexing, and key-value access. Below are the key behaviors of specific Java
collection types.

.. _collections_iterable:

Iterable
--------

Java classes that implement ``java.util.Iterable`` are customized to support
Python's iteration constructs, so they work directly in Python `for` loops
and list comprehensions. For example, a Java ``ArrayList`` can be iterated
directly:

.. code-block:: python

    from java.util import ArrayList

    jlist = ArrayList()
    jlist.add("apple")
    jlist.add("orange")
    jlist.add("banana")

    for item in jlist:
        print(item)

This integration ensures that Java collections behave like Python sequences,
providing a natural experience for Python developers.

.. _collections_iterators:

Iterators
---------

Java classes that implement ``java.util.Iterator`` act as Python iterators.
This means they can be used in Python `for` loops and list comprehensions
without requiring additional conversion. For example:

.. code-block:: python

    from java.util import Vector

    jvector = Vector()
    jvector.add("apple")
    jvector.add("orange")

    iterator = jvector.iterator()
    for item in iterator:
        print(item)

.. _collections_collection:

Collection
----------

Java classes that inherit from ``java.util.Collection`` work directly with
Python's collection constructs. They support operations such as length
retrieval, iteration, and implicit conversion of Python sequences into Java
collections. For example:

.. code-block:: python

    from java.util import ArrayList

    pylist = ["apple", "orange", "banana"]
    jlist = ArrayList(pylist)  # Convert Python list to Java collection

    print(len(jlist))  # Output: 3
    for item in jlist:
        print(item)

Methods that accept Java collections can automatically convert Python sequences
if all elements are compatible with Java types. Otherwise, a ``TypeError`` is
raised.

.. _java.util.List:

Lists
-----

Java `List` classes, such as ``ArrayList`` and ``LinkedList``, can be used in
Python `for` loops and list comprehensions. They also support indexing and
deletion, making them behave like Python lists. For example:

.. code-block:: python

    from java.util import ArrayList

    jlist = ArrayList()
    jlist.add("apple")
    jlist.add("orange")
    jlist.add("banana")

    print(jlist[0])  # Output: apple
    del jlist[1]     # Remove "orange"
    print(jlist)     # Output: [apple, banana]

Java lists can also be converted to Python lists and vice versa using the copy
constructor. For example:

.. code-block:: python

    pylist = ["apple", "orange", "banana"]
    jlist = ArrayList(pylist)  # Convert Python list to Java list
    pylist2 = list(jlist)      # Convert Java list back to Python list

Note that individual elements remain Java objects when converted to Python.
Converting to Java will attempt to convert each argument
individually to Java.  If there is no conversion it will produce a
``TypeError``.  The conversion can be forced by casting to the appropriate
Java type with a list comprehension or by defining a new conversion
customizer.

Lists also have iterable, length, item deletion, and indexing.  Note that
indexing of ``java.util.LinkedList`` is supported but can have a large
performance penalty for large lists.  Use of iteration is much for efficient.


.. _java.util.Map:

Maps
----

Java classes that implement ``java.util.Map`` behave like Python dictionaries.
They support key-value access, iteration, and deletion. 
Here is a summary of their capabilities:

=========================== ================================
Action                       Python
=========================== ================================
Place a value in the map     ``jmap[key]=value``
Delete an entry              ``del jmap[key]``
Get the length               ``len(jmap)``
Lookup the value             ``v=jmap[key]``
Get the entries              ``jmap.items()``
Fetch the keys               ``jmap.key()``
Check for a key              ``key in jmap``
=========================== ================================

Example using Java HashMap with Pythonic interface:

.. code-block:: python

    from java.util import HashMap

    jmap = HashMap()
    jmap.put("key1", "value1")
    jmap.put("key2", "value2")

    print(jmap["key1"])  # Output: value1
    del jmap["key2"]     # Remove "key2"
    print(len(jmap))     # Output: 1

Maps also support iteration over keys and values:

.. code-block:: python

    for key, value in jmap.items():
        print(f"{key}: {value}")

Methods that accept Java maps can implicitly convert Python dictionaries if
all keys and values are compatible with Java types. Otherwise, a ``TypeError``
is raised.

.. _collections_map_entries:

Map Entries
-----------

Java map entries unpack into key-value pairs, allowing easy iteration in Python
loops. For example:

.. code-block:: python

    for key, value in jmap.items():
        print(f"{key}: {value}")

.. _collections_sets:

Sets
----

Java classes that implement ``java.util.Set`` behave like Python sets. They
support operations such as item deletion, iteration, and length retrieval. For
example:

.. code-block:: python

    from java.util import HashSet

    jset = HashSet()
    jset.add("apple")
    jset.add("orange")

    print(len(jset))  # Output: 2
    jset.remove("orange")
    print(jset)       # Output: [apple]

.. _collections_enumeration:

Enumeration
-----------

Java classes that implement ``java.util.Enumeration`` act as Python iterators.
This allows them to be used in Python `for` loops and list comprehensions. For
example:

.. code-block:: python

    from java.util import Vector

    jvector = Vector()
    jvector.add("apple")
    jvector.add("orange")

    enumeration = jvector.elements()
    for item in enumeration:
        print(item)


.. _collections_integrating_pythonic_constructs_with_java_collections:

Combining Pythonic Constructs with Java Streams
================================================

Because Java collections satisfy Python's iteration protocol, ordinary list
comprehensions and generator expressions work directly on them:

.. code-block:: python

    from java.util import ArrayList

    jlist = ArrayList()
    jlist.add("apple")
    jlist.add("orange")
    jlist.add("banana")

    filtered = [item.upper() for item in jlist if item.startswith("a")]
    print(filtered)  # Output: ['APPLE']

Java's own `Stream` API (`filter`, `map`, `reduce`, `.parallelStream()`, ...)
is available the same way, called directly on the collection:

.. code-block:: python

   from java.util.stream import Collectors

   filtered = jlist.stream().filter(lambda s: s.startswith("a")).map(
       lambda s: s.upper()).collect(Collectors.toList())
   print(filtered)  # Output: [APPLE]

The two mix freely -- a Java stream can feed a Python comprehension for
further processing, or vice versa:

.. code-block:: python

   filtered_stream = jlist.stream().filter(lambda s: s.startswith("a")).collect(
       Collectors.toList())
   final_result = [item.lower() for item in filtered_stream]
   print(final_result)  # Output: ['apple']

As a rule of thumb: reach for a Python comprehension for simple
filtering/mapping and quick prototyping, and for a Java stream when the
operation is more naturally expressed with Java's built-in collectors
(grouping, reducing, `.parallelStream()`) or needs to interoperate with a
Java API that already expects a `Stream`. Whichever you pick, avoid
frequent small back-and-forth calls across the Python/Java boundary in a
loop -- collect data in bulk, then process it on one side.
