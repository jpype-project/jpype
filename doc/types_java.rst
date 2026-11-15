
Python Types (for Java)
=========================

This is the Java-side counterpart to :doc:`types_py`: the ``python.lang``
type hierarchy that represents Python values on the Java side of the
reverse bridge. Every interface named below is real, shipping code in the
``python.lang`` package.

.. contents::
   :local:
   :depth: 1


The PyObject hierarchy
------------------------

Every Python value crossing into Java implements ``python.lang.PyObject``,
the root interface. Like the rest of ``python.lang``, ``PyObject`` and its
subtypes are **interfaces, not concrete classes** -- you never construct one
with ``new``. Instances come from:

- a ``PyBuiltIn`` factory method (``context.str(...)``, ``context.dict()``,
  ``context.list(...)``, ``context.tuple(...)``, ...), or
- ``eval()``/``exec()`` returning a ``PyObject`` you then narrow with a cast,
  or
- a value coming back from a Python callable you invoked (see
  :doc:`quickguide_java`).

Don't confuse this with :doc:`proxies_py`: that chapter covers a *Python*
object (``@JImplements``/``JProxy``) playing a *Java* role, called from
Java code that doesn't know it's talking to Python underneath.
``python.lang`` is the opposite relationship -- Java code directly
manipulating a live Python object through a typed Java-side handle, with
no pretense of being a Java type at all.

Where a Java collection interface already describes the right shape,
``python.lang`` types implement it directly rather than inventing a
parallel API: a ``PyDict`` is a ``java.util.Map``, a ``PyList`` is a
``java.util.List``, a ``PyString`` is a ``CharSequence``. This means you can
usually hand a Python container straight to ordinary Java code that expects
the matching ``java.util``/``java.lang`` interface. Where Python supports an
operation Java has no interface for (e.g. ``__contains__`` beyond
``Collection.contains``), it's exposed as its own named method instead of
being forced into an ill-fitting Java shape.

Not every object supports every protocol -- Python objects pick and choose
which protocols (iteration, mapping, buffer, ...) they implement, and so
does the Java wrapper. Calling a method for a protocol the underlying
Python object doesn't actually support raises an exception rather than
silently doing nothing.


Protocol interfaces
----------------------

These describe *capabilities* a ``PyObject`` may or may not have, mirroring
Python's own protocol-based design (``__iter__``, ``__call__``,
``__getitem__``, the buffer protocol, ...):

======================= =================================================
Interface               Capability
======================= =================================================
``PyIterable<T>``       extends ``Iterable<T>`` -- usable in a Java
                        for-each loop
``PyIterator<T>``       extends ``PyIter<T>`` -- a live Python iterator
``PySequence<T>``       extends ``PyCollection<T>``, ``List<T>`` -- ordered,
                        indexable
``PyMapping<K,V>``      extends ``PyCollection<K>``, ``Map<K,V>``
``PyCallable``          invocable with ``call(Object...)``; see
                        :doc:`quickguide_java`
``PyBuffer``            supports Python's buffer protocol
                        (``memoryview``-backed transfer)
``PyNumber``            arithmetic protocol, implemented by ``PyInt``,
                        ``PyFloat``, ``PyComplex``
``PyIndex``             convertible to a Java ``int``/``long`` index
``PySized``             has a Python ``len()``
``PyCombinable``        supports ``|``-style dict/set combination
                        operators
======================= =================================================

A concrete type typically implements several of these at once -- for
example ``PyList`` is a ``PySequence<PyObject>``, which already pulls in
``PyCollection``, ``List``, ``Iterable``, and ``PySized`` by extension.


Concrete types
-----------------

The types below are the ones with a dedicated Java interface today.

``PyString``
  Extends ``CharSequence``, so it can be passed anywhere Java expects
  character data.

  .. code-block:: java

      PyString s = context.str("hello world");
      boolean has = s.containsSubstring("world");   // true
      int len = s.length();                          // CharSequence method

``PyInt`` / ``PyFloat`` / ``PyComplex``
  Numeric types, all ``PyNumber``. Construct with the ``PyBuiltIn``
  ``$int``/``$float`` factories (prefixed with ``$`` because ``int`` and
  ``float`` are Java reserved words).

  .. code-block:: java

      PyInt i = context.$int(42);
      PyFloat f = context.$float(3.14);

``PyDict``
  A ``PyMapping<PyObject, PyObject>``, so it's a ``java.util.Map``.

  .. code-block:: java

      PyDict dict = context.dict();
      dict.putAny("a", 1);
      int size = dict.size();                        // java.util.Map method

``PyList`` / ``PyTuple``
  Both ``PySequence<PyObject>``, i.e. ``java.util.List``. ``PyTuple`` is
  still immutable underneath -- mutating methods inherited from ``List``
  will throw.

  .. code-block:: java

      PyList list = context.list(java.util.Arrays.asList("a", "b"));
      PyTuple args = context.tuple(2, 3);

``PySet`` / ``PyFrozenSet``
  ``PySet`` is a mutable ``java.util.Set``; ``PyFrozenSet`` is immutable.

``PyBytes`` / ``PyByteArray``
  Both a ``PySequence<PyInt>`` and a ``PyBuffer``. ``PyBytes`` is
  immutable, ``PyByteArray`` is mutable.

  .. code-block:: java

      PyBytes b = context.bytes("abc");               // encodes as UTF-8

``PySlice`` / ``PyRange``
  ``PySlice`` mirrors Python's ``slice`` object; ``PyRange`` is a
  ``PyIter<PyInt>`` mirroring ``range()``.

  .. code-block:: java

      PySlice slice = context.slice(1, 5, 2);

``PyMemoryView``
  A ``PySequence<PyInt>`` over a buffer-supporting object, mirroring
  Python's ``memoryview()``.

  .. code-block:: java

      PyMemoryView view = context.memoryview(bytesObj);


Exceptions
------------

Python exceptions are not part of ``python.lang`` -- they live under
``python.exceptions`` (``PyValueError``, ``PyZeroDivisionError``, ...),
mirroring Python's own exception hierarchy so they can be caught by type
from Java. ``PyExc`` (in ``python.lang``) is the underlying handle for
inspecting the live exception object. See :doc:`quickguide_java`'s
"Exceptions" section.


Standard library types
-------------------------

Types for specific standard-library modules (``collections``, ``io``,
``datetime``, ``decimal``, ``pathlib``) live in their own ``python.*``
packages, not ``python.lang``, and are reached through a
``<Module>.using(context)`` factory rather than ``PyBuiltIn``. See
:doc:`limitations_java` for the full list of what's covered and what still
crosses as a bare, untyped ``PyObject``.


Where to next
---------------

- :doc:`quickguide_java` -- calling into Python and getting these types
  back.
- :doc:`limitations_java` -- what's not typed yet, and the GIL/
  subinterpreter model these types operate under.
