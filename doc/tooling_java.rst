
Object Lifetime and Cleanup (for Java)
==========================================

This is the Java-side counterpart to :doc:`tooling_py`'s garbage
collection section: what keeps a Python object alive while Java code holds
a reference to it, and how to release resources explicitly rather than
waiting on the garbage collector. The relevant classes are
``org.jpype.ref.NativeReferenceQueue``, ``python.lang.PyMemoryView``, and
``python.io``'s ``close()``/``closed()`` methods.

.. contents::
   :local:
   :depth: 1


PyObject proxy lifetime vs. CPython refcounting
------------------------------------------------------

Every ``PyObject`` on the Java side wraps a live CPython object reference
(``PyObject*``) underneath. JPype links the two languages' memory
management the same way in this direction as it does in the forward
direction described in :doc:`tooling_py`: a background reference-queue
thread (``NativeReferenceQueue``, a phantom-reference-based cleaner) tracks
each Java-side wrapper, and when the JVM garbage collector reclaims a
wrapper with no remaining Java references, the queue releases the
underlying Python reference in turn. This means a Python object referenced
only from Java stays alive exactly as long as its Java wrapper does -- you
don't need to do anything special to keep it alive, and you don't leak the
Python side by simply letting the Java wrapper go out of scope.

The corollary: **don't assume prompt cleanup**. Like any JVM object,
a ``PyObject`` wrapper is reclaimed whenever the garbage collector next
gets to it, not the instant it becomes unreachable, so the corresponding
Python-side release is similarly delayed. For anything that holds a scarce
resource underneath (an open file, a memory buffer), don't rely on GC
timing -- release it explicitly instead, as described below.


Explicit release: PyMemoryView
-----------------------------------

``PyMemoryView.release()`` (mirroring Python's own
``memoryview.release()``) releases the underlying buffer immediately,
independent of when the Java wrapper itself gets collected.

.. code-block:: java

    PyMemoryView view = context.memoryview(bytesObj);
    // ... use the view ...
    view.release();   // buffer released now, not whenever GC gets to it

After ``release()``, the view is unusable -- the same contract Python's
``memoryview`` itself has.


Explicit release: python.io streams
-----------------------------------------

``python.io`` types expose ``close()``/``closed()`` directly, mirroring
Python's own file-like object protocol. As with any stream, explicit
``close()`` (or try-with-resources, where the type supports
``AutoCloseable``) is the correct way to release the underlying resource
promptly rather than waiting on collection:

.. code-block:: java

    PyStringIO buf = IO.using(context).stringIO();
    buf.write("hello");
    assertFalse(buf.closed());

    buf.close();
    assertTrue(buf.closed());


Where to next
---------------

- :doc:`tooling_py` -- the Python-calling-Java direction's garbage
  collection linking, described from the other side.
- :doc:`quickguide_java` -- the standard-library front ends
  (``python.io``, ``python.collections``, ...) these lifetime rules apply
  to.
