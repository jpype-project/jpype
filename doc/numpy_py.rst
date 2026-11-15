.. _working_with_numpy:

Working with NumPy
********************

This chapter covers the Python-calling-Java direction only. There is
presently no reverse-direction (Java-embedding-Python) NumPy story --
``python.io``/``python.collections``/``python.datetime`` don't expose a
NumPy-array front end, so no ``numpy_java.rst`` is planned. If that
changes, this note should be replaced with a cross-reference.

JPype supports bidirectional transfer of arrays between NumPy and Java,
letting numerical libraries on either side operate on the same underlying
data with minimal overhead.

.. _working_with_numpy_transferring_arrays_between_python_and_java:

Transferring Arrays Between Python and Java
===========================================

.. _working_with_numpy_transferring_arrays_to_numpy:

Transferring Arrays to NumPy
----------------------------

Java arrays can be transferred into NumPy arrays using Python's `memoryview`.
This enables efficient bulk data transfer for rectangular arrays.

.. code-block:: python

   import jpype
   import numpy as np

   jpype.startJVM()

   java_array = jpype.JDouble[:]([1.1, 2.2, 3.3])
   numpy_array = np.array(memoryview(java_array))

   print(numpy_array)  # [1.1 2.2 3.3]

.. _working_with_numpy_transferring_arrays_to_java:

Transferring Arrays to Java
---------------------------

NumPy arrays can be transferred to Java using the `JArray.of` function. This
maps the structure of a NumPy array to a Java multidimensional array.

.. code-block:: python

   import jpype
   import numpy as np

   jpype.startJVM()

   numpy_array = np.zeros((5, 10, 20))  # 5x10x20 array filled with zeros
   java_array = jpype.JArray.of(numpy_array)

   print(java_array[0][0][0])  # 0.0

Both directions require the array to be rectangular -- jagged arrays are not
supported -- and the element type to be a primitive Java type with a
compatible NumPy dtype (e.g. ``np.float64`` <-> ``double``). A jagged array
or an incompatible dtype raises ``TypeError``.

.. _working_with_numpy_buffer_backed_numpy_arrays:

Buffer Backed NumPy Arrays
==========================

Java direct buffers (``java.nio``) provide a mechanism for shared memory
between Java and Python, enabling high-speed data exchange by bypassing the
JNI layer. This is particularly useful for large datasets, such as
scientific computing or memory-mapped files. Direct buffers are not managed
by the garbage collector, so improper use may lead to memory leaks or
crashes.

The buffer can originate on either side. Both directions end with a NumPy
array that shares memory with the Java buffer, via ``np.asarray``:

.. code-block:: python

   import jpype
   import numpy as np

   jpype.startJVM()

   # Originate the buffer in Java:
   jb = java.nio.ByteBuffer.allocateDirect(80)  # 80 bytes
   db = jb.asDoubleBuffer()
   np_array = np.asarray(db)                    # NumPy array backed by the Java buffer

   # Or originate the buffer in Python:
   py_buffer = bytearray(80)
   jb = jpype.nio.convertToDirectBuffer(py_buffer)  # maps the bytearray into Java
   db = jb.asDoubleBuffer()
   np_array = np.asarray(db)                    # NumPy array backed by the Python buffer

``jpype.nio.convertToDirectBuffer`` honors the writability of the passed
object: it returns a read-only ``ByteBuffer`` if the source object (e.g. a
``bytes``) is itself read-only.

.. _working_with_numpy_important_considerations_for_buffer_backed_arrays:

Buffer Lifetime
----------------

Python and NumPy cannot detect when a Java-originated buffer becomes
invalid. Once the JVM shuts down, every buffer that originated in Java
becomes invalid, and any further access to it -- including through a NumPy
array still holding a reference -- can crash the process. Two ways to avoid
this:

- Originate the buffer in Python and pass it to Java, so Python retains
  control over the memory's lifetime.
- If the buffer must originate in Java, shut down with
  ``java.lang.Runtime.exit`` so both the JVM and the Python process end
  together, rather than leaving a dangling buffer behind in a still-running
  Python process.

This same buffer-backed mechanism isn't limited to NumPy -- it applies to
any application needing shared memory between processes or memory-mapped
files.

.. _working_with_numpy_numpy_primitives:

NumPy Primitives
=================

NumPy arrays can be mapped to Java boxed types or primitive arrays. The
following table summarizes the mapping:

=================  ============================
NumPy Type         Java Type (Boxed/Primitive)
=================  ============================
np.int8            java.lang.Byte / byte[]
np.int16           java.lang.Short / short[]
np.int32           java.lang.Integer / int[]
np.int64           java.lang.Long / long[]
np.float16         java.lang.Float / float[] (*)
np.float32         java.lang.Float / float[]
np.float64         java.lang.Double / double[]
=================  ============================

(*) ``np.float16`` has no Java equivalent, so it is automatically converted
to ``float32`` (``java.lang.Float`` / ``float[]``) during transfer to Java.
If precise ``float16`` handling matters, convert to ``float32``/``float64``
explicitly in Python before transferring.

.. code-block:: python

   import jpype
   import numpy as np

   jpype.startJVM()

   float16_array = np.array([1.1, 2.2, 3.3], dtype=np.float16)

   # As a boxed type (java.util.ArrayList of java.lang.Float):
   java_list = jpype.java.util.ArrayList()
   for value in float16_array:
       java_list.add(jpype.JFloat(value))  # converted to float32
   print(java_list)  # [1.1, 2.2, 3.3]

   # As a primitive array (float32[]):
   java_primitive_array = jpype.JArray(jpype.JFloat)(float16_array)
   print(java_primitive_array)  # [1.1, 2.2, 3.3]
