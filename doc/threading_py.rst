.. _concurrent_processing:

Concurrent Processing
*********************

This chapter covers the topic of threading, synchronization, and multiprocess.
Much of this material depends on the use of :ref:`Proxies <Proxies>` covered
in the prior chapter.

This chapter covers the Python-calling-Java direction. If you are instead
embedding Python in a Java application, see :doc:`threading_java` for the
matching Java-side GIL-per-call model and async call support.

.. _concurrent_processing_threading:

Threading
=========

JPype supports all types of threading subject to the restrictions placed by
Python.  Java is inherently threaded and support a vast number of threading
styles such as execution pools, futures, and ordinary thread.  Python is
somewhat more limited.  At its heart Python is inherently single threaded
and requires a master lock known as the GIL (Global Interpreter Lock) to
be held every time a Python call is made.  Python threads are thus more
cooperative that Java threads.

To deal with this behavior, JPype releases the GIL every time it leaves from
Python into Java to any user defined method.  Shorter defined calls such as
to get a string name from from a class may not release the GIL.  Every time
the GIL is released it is another opportunity for Python to switch to a different
cooperative thread.

.. _concurrent_processing_threading_python_threads:

Python Threads
--------------

For the most part, Python threads based on OS level threads (i.e. POSIX
threads) will work without problem. The only challenge is how Java sees threads.
In order to operate on a Java method, the calling thread must be attached to
Java.  Failure to attach a thread will result in a segmentation fault.  It used
to be a requirement that users manually attach their thread to call a Java
function, but as the user has no control over the spawning of threads by other
applications such as an IDE, this inevitably lead to unexpected segmentation
faults.  Rather that crashing randomly, JPype automatically attaches any
thread that invokes a Java method.  These threads are attached automatically as
daemon threads so that will not prevent the JVM from shutting down properly
upon request.  If a thread must be attached as a non-daemon, use the method
``java.lang.Thread.attach()`` from within the thread context.  Once this is
done the JVM will not shut down until that thread is completed.

There is a function called ``java.lang.Thread.isAttached()`` which will check
if a thread is attached.  As threads automatically attach to Java, the only
way that a thread would not be attached is if it has never called a Java method.

The downside of automatic attachment is that each attachment allocates a
small amount of resources in the JVM.  For applications that spawn frequent
dynamically allocated threads, these threads will need to be detached prior
to completing the thread with ``java.lang.Thread.detach()``.  When
implementing dynamic threading, one can detach the thread
whenever Java is no longer needed.  The thread will automatically reattach if
Java is needed again.  There is a performance penalty each time a thread is
attached and detached.

.. _concurrent_processing_threading_java_threads:

Java Threads
------------

To use Java threads, create a Java proxy implementing
``java.lang.Runnable``.  The Runnable can then be passed any Java threading
mechanism to be executed.  Each time that Java threads transfer control
back to Python, the GIL is reacquired.

.. _concurrent_processing_threading_other_threads:

Other Threads
-------------

Some Python libraries offer other kinds of thread, (i.e. microthreads). How
they interact with Java depends on their nature. As stated earlier, any OS-
level threads will work without problem. Emulated threads, like microthreads,
will appear as a single thread to Java, so special care will have to be taken
for synchronization.


.. _concurrent_processing_customizing_javalangthread:

Customizing java.lang.Thread
============================

.. _concurrent_processing_overview:

Thread Attachment Overview
---------------------------

JPype automatically attaches Python threads to the JVM when they interact with
Java resources. Threads are attached as daemon threads to ensure that they do
not block JVM shutdown. While this behavior simplifies integration, it can lead
to resource leaks in thread-heavy applications if threads are not properly
detached when they terminate.

To address this, JPype customizes ``java.lang.Thread`` with additional methods
for managing thread attachment and detachment. These methods allow developers
to explicitly detach threads, freeing resources in the JVM and preventing leaks.

.. _concurrent_processing_customized_methods:

Customized Thread Methods
--------------------------

The following methods are added to ``java.lang.Thread`` by JPype:

.. method:: java.lang.Thread.attach()
   :no-index:

   Attaches the current thread to the JVM as a user thread.

   User threads prevent the JVM from shutting down until they are terminated or
   detached. This method can be used to convert a daemon thread to a user
   thread.

   **Raises**:
     - `RuntimeError`: If the JVM is not running.

.. method:: java.lang.Thread.attachAsDaemon()
   :no-index:

   Attaches the current thread to the JVM as a daemon thread.

   Daemon threads act as background tasks and do not prevent the JVM from
   shutting down. JPype automatically attaches threads as daemon threads when
   they interact with Java resources. Use this method to explicitly attach a
   thread as a daemon.

   **Raises**:
     - `RuntimeError`: If the JVM is not running.

.. method:: java.lang.Thread.detach()
   :no-index:

   Detaches the current thread from the JVM.

   This method frees the associated resources in the JVM for the current thread.
   It is particularly important for thread-heavy applications to prevent leaks.
   Detaching a thread does not interfere with its ability to reattach later.

   **Notes**:
     - This method cannot fail and is safe to call even if the JVM is not
       running.
     - There is no harm in calling this method multiple times or detaching
       threads early.

.. _concurrent_processing_examples_with_java_thread:

Examples with Java Thread
-------------------------

Here are examples of how to use the customized methods for `java.lang.Thread`:

.. code-block:: python

   import jpype
   import jpype.imports

   jpype.startJVM()

   # Attach the thread as a user thread
   java.lang.Thread.attach()
   print("Thread attached as a user thread.")

   # Perform Java operations here...

   # Detach the thread after completing Java operations
   java.lang.Thread.detach()
   print("Thread detached from the JVM.")

   # Attach the thread as a daemon thread
   java.lang.Thread.attachAsDaemon()
   print("Thread attached as a daemon thread.")

.. _concurrent_processing_best_practices_for_java_thread:

Best Practices for Java Thread
------------------------------

- **Detach Threads When They End**: For thread-heavy applications, ensure that
  Python threads detach themselves from the JVM before they terminate. This
  prevents resource leaks and ensures efficient memory usage.

- **Avoid Excessive Attachments**: While JPype automatically attaches threads,
  excessive thread creation without proper detachment can lead to resource
  exhaustion in the JVM.

- **Detach Early**: Detaching threads early, after completing all Java
  operations, is safe and does not interfere with reattachment later. This is
  especially important for applications that spawn many short-lived threads.

- **Monitor Resource Usage**: Regularly monitor JVM memory usage in
  thread-heavy applications to identify potential leaks caused by lingering
  thread attachments.

.. _concurrent_processing_summary_of_java_thread:

Summary of Java Thread
----------------------

JPype customizes `java.lang.Thread` to provide additional methods for managing
thread attachment and detachment to/from the JVM. While JPype automatically
attaches threads as daemon threads, it is crucial to detach threads explicitly
in thread-heavy applications to prevent resource leaks. By following best
practices, developers can ensure efficient memory usage and smooth integration
between Python and Java.


.. _customizing_javaio_streams:

Customizing java.io Streams
============================

.. _customizing_javaio_overview:

Stream Customization Overview
-------------------------------

Just as JPype customizes ``java.lang.Thread`` with attachment methods (see
`Customizing java.lang.Thread`_ above), it customizes the ``java.io`` stream
hierarchy with a ``toPython()`` method. This is added directly to
``java.io.Writer``, ``java.io.Reader``, ``java.io.OutputStream``, and
``java.io.InputStream``, so it is automatically available on every concrete
subclass as well — ``java.io.PrintStream``, ``java.io.BufferedReader``,
``java.io.FileOutputStream``, and so on.

``toPython()`` wraps the Java stream in a Python object that is a real
``io.TextIOBase`` subclass, satisfying Python's own text-file-object
contract (``write``/``read``/``readline``/``flush``/``close``, context
manager support, line iteration, ...). This makes it possible to use a
Java-owned stream anywhere Python expects a file-like object — most notably
by assigning it to ``sys.stdout``, ``sys.stderr``, or ``sys.stdin``.

.. _customizing_javaio_methods:

Customized Stream Methods
---------------------------

.. method:: java.io.Writer.toPython(encoding=None, errors="strict")
   :no-index:

   Wraps this ``Writer`` as a Python ``io.TextIOBase`` write-only stream.

.. method:: java.io.Reader.toPython(encoding=None, errors="strict")
   :no-index:

   Wraps this ``Reader`` as a Python ``io.TextIOBase`` read-only stream.

.. method:: java.io.OutputStream.toPython(encoding="utf-8", errors="strict")
   :no-index:

   Wraps this ``OutputStream`` in a ``java.io.OutputStreamWriter`` for the
   given charset and returns its ``toPython()``.

.. method:: java.io.InputStream.toPython(encoding="utf-8", errors="strict")
   :no-index:

   Wraps this ``InputStream`` in a ``java.io.InputStreamReader`` for the
   given charset and returns its ``toPython()``.

The same ``toPython()`` convention is also used to give a genuine,
independent Python value for a handful of immutable Java value types —
unlike the stream wrappers above, these return a real value copy with no
residual Java reference:

.. method:: java.time.Instant.toPython()
   :no-index:

   Returns a timezone-aware ``datetime.datetime`` in UTC.

.. method:: java.sql.Date.toPython()
   :no-index:

   Returns a ``datetime.date``.

.. method:: java.sql.Time.toPython()
   :no-index:

   Returns a ``datetime.time``.

.. method:: java.sql.Timestamp.toPython()
   :no-index:

   Returns a ``datetime.datetime``.

.. method:: java.math.BigDecimal.toPython()
   :no-index:

   Returns a ``decimal.Decimal``.

.. method:: java.nio.file.Path.toPython()
   :no-index:

   Returns a ``pathlib.Path``.

.. method:: java.io.File.toPython()
   :no-index:

   Returns a ``pathlib.Path``.

.. _customizing_javaio_examples:

Examples
--------

Using ``toPython()`` directly from Python needs no ``jpype`` internals at
all — the method is right there on the Java object:

.. code-block:: python

   import sys
   import jpype
   import jpype.imports

   jpype.startJVM()
   from java.io import StringWriter

   sw = StringWriter()
   sys.stdout = sw.toPython()
   print("this goes into the Java StringWriter")
   sys.stdout.flush()
   sys.stdout = sys.__stdout__  # restore

   print(str(sw.toString()))

Java code that embeds an interpreter (``org.jpype.MainInterpreter`` or
``org.jpype.SubInterpreter``) can trigger the same redirect explicitly, one
directional stream at a time, via ``setOutput``/``setError``/``setInput`` on
the shared ``Interpreter`` interface — each is a thin wrapper that installs
the stream's ``toPython()`` result onto ``sys.stdout``/``sys.stderr``/
``sys.stdin`` for that interpreter:

.. code-block:: java

   Interpreter interpreter = MainInterpreter.getInstance();
   OutputStream captured = new ByteArrayOutputStream();
   interpreter.setOutput(captured);
   // ... run Python code; everything it prints lands in `captured` ...
   interpreter.resetOutput();  // back to the real sys.__stdout__

Because each ``SubInterpreter`` owns its own ``sys`` module, redirecting one
subinterpreter's stdio has no effect on the main interpreter or any other
subinterpreter.

``org.jpype.SubInterpreterBuilder`` combines PEP 684 interpreter
configuration with this same stdio wiring in one place, modeled on
``java.lang.ProcessBuilder``:

.. code-block:: java

   try (SubInterpreter sub = SubInterpreterBuilder.ownGil()
           .setOutput(captured)
           .start())          // genuinely isolated own-GIL subinterpreter
   {
       // ...
   }   // sub.close() runs automatically, even if the block throws

A bare ``new SubInterpreterBuilder()`` defaults to the safest legal
combination - own GIL, own obmalloc, ``check_multi_interp_extensions``
enabled - the same as ``ownGil()``, which exists purely so call sites can
say so explicitly. Use ``elevated()`` to opt into the less-restrictive
shared-GIL/shared-obmalloc combination (the fixed values the plain,
builder-less ``SubInterpreter.start()`` still uses) when the default
isolation is too restrictive for what the subinterpreter needs to import
or share. Individual ``PyInterpreterConfig`` flags (``ALLOW_FORK``,
``ALLOW_EXEC``, ``ALLOW_THREADS``, ``ALLOW_DAEMON_THREADS``,
``CHECK_MULTI_INTERP_EXTENSIONS``, ``USE_MAIN_OBMALLOC``, ``OWN_GIL``) can be
toggled directly via ``with(Option...)``/``without(Option...)``; an illegal
combination (currently: disabling ``USE_MAIN_OBMALLOC`` without enabling
``CHECK_MULTI_INTERP_EXTENSIONS``) raises ``IllegalStateException`` from
``start()`` before any native call is made.

.. warning::

   Always close a ``SubInterpreter`` - via try-with-resources in Java
   (shown above; ``SubInterpreter`` implements ``AutoCloseable``) or a
   ``with`` block in Python (``SubInterpreter`` picks up ``__enter__``/
   ``__exit__`` for free through JPype's blanket ``AutoCloseable``
   customizer) - rather than a bare call to ``.close()`` at the end of a
   method. If an exception skips the close (for example, the
   cross-interpreter proxy guard described below raising mid-block) and
   the subinterpreter is left open, the *process* dies at shutdown with a
   fatal, uncatchable CPython error (``PyInterpreterState_Delete:
   remaining subinterpreters``) rather than a normal Python exception or
   a leak. ``with``/try-with-resources close on every exit path,
   including exceptions, and are the only way to guarantee this can't
   happen.

   This matters in particular because evaluating an expression inside a
   subinterpreter and returning its result across the interpreter
   boundary is exactly the kind of call that can raise mid-block: a
   result object that still belongs to the subinterpreter's own
   allocator is rejected with a ``RuntimeError``/``IllegalStateException``
   ("smuggled proxy") rather than risking memory corruption, so code that
   evaluates subinterpreter expressions should expect that failure mode
   and still guarantee cleanup around it.

.. _customizing_javalangreflect_method:

Customizing java.lang.reflect.Method
=====================================

``java.lang.reflect.Method`` also has a ``toPython()`` customizer, but it
serves a different purpose than the value-copy and stream conversions above:
it turns one already-resolved overload into a plain Python callable, bound to
exactly that signature.

.. method:: java.lang.reflect.Method.toPython()
   :no-index:

   Returns a Python callable bound to this specific ``Method`` object,
   skipping JPype's normal per-call overload search. Because a
   ``java.lang.reflect.Method`` has no attribute to bind ``self`` to,
   instance methods take the instance as an explicit first argument:
   ``m.toPython()(instance, *args)``. Static methods are called directly,
   ``m.toPython()(*args)``. Calling the result with an argument count or
   types that don't match this one signature raises ``TypeError`` rather
   than falling back to search other overloads of the same name.

This is useful when a ``Method`` was obtained via reflection (for example
``Class.getDeclaredMethod(name, *parameterTypes)``) and the exact overload
is already known, so repeating JPype's normal overload resolution on every
call would be redundant:

.. code-block:: python

   Math = jpype.JClass("java.lang.Math")
   m = Math.class_.getDeclaredMethod("max", jpype.JInt, jpype.JInt)
   f = m.toPython()
   f(3, 5)  # 5, calls exactly this overload

.. _synchronized:

Synchronization
===============

Java synchronization support can be split into two categories. The first is the
``synchronized`` keyword, both as prefix on a method and as a block inside a
method. The second are the three methods available on the Object class
(``notify, notifyAll, wait``).

To support the ``synchronized`` functionality, JPype defines a method called
``synchronized(obj)`` to be used with the Python ``with`` statement, where
obj has to be a Java object. The return value is a monitor object that will
keep the synchronization on as long as the object is kept alive.  For example,

.. code-block:: python

    from jpype import synchronized

    mySharedList = java.util.ArrayList()

    # Give the list to another thread that will be adding items
    otherThread.setList(mySharedList)

    # Lock the list so that we can access it without interference
    with synchronized(mySharedList):
        if not mySharedList.isEmpty():
            ...  # process elements
    # Resource is unlocked once we leave the block

The Python ``with`` statement is used to control the scope.  Do not
hold onto the monitor without a ``with`` statement.  Monitors held outside of a
``with`` statement will not be released until they are broken when the monitor
is garbage collected.

The other synchronization methods are available as-is on any Java object.
However, as general rule one should not use synchronization methods on Java
String as internal string representations may not be complete objects.

For synchronization that does not have to be shared with Java code, use
Python's support directly rather than Java's synchronization to avoid
unnecessary overhead.


.. _concurrent_processing_threading_examples:

Threading examples
==================

Java provides a very rich set of threading tools.  This can be used in Python
code to extend many of the benefits of Java into Python.  However, as Python
has a global lock, the performance of Java threads while using Python is not
as good as native Java code.

.. _concurrent_processing_limiting_execution_time:

Limiting execution time
-----------------------

We can combine proxies and threads to produce achieve a number of interesting
results.  For example:

.. code-block:: python

    def limit(method, timeout):
        """ Convert a Java method to asynchronous call with a specified timeout. """
        def f(*args):
            @jpype.JImplements(java.util.concurrent.Callable)
            class g:
                @jpype.JOverride
                def call(self):
                    return method(*args)
            future = java.util.concurrent.FutureTask(g())
            java.lang.Thread(future).start()
            try:
                timeunit = java.util.concurrent.TimeUnit.MILLISECONDS
                return future.get(int(timeout*1000), timeunit)
            except java.util.concurrent.TimeoutException as ex:
                future.cancel(True)
            raise RuntimeError("canceled", ex)
        return f

    print(limit(java.lang.Thread.sleep, timeout=1)(200))
    print(limit(java.lang.Thread.sleep, timeout=1)(20000))

Here we have limited the execution time of a Java call.


.. _concurrent_processing_multiprocessing:

Multiprocessing
===============

Because only one JVM can be started per process, JPype cannot be used with
processes created with ``fork``.  Forks copy all memory including the JVM.  The
copied JVM usually will not function properly thus JPype cannot support
multiprocessing using fork.

To use multiprocessing with JPype, processes must be created with "spawn".  As
the multiprocessing context is usually selected at the start and the default
for Unix is fork, this requires the creating the appropriate spawn context.  To
launch multiprocessing properly the following recipe can be used.

.. code-block:: python

   import multiprocessing as mp

   ctx = mp.get_context("spawn")
   process = ctx.Process(...)
   queue = ctx.Queue()
   # ...

When using multiprocessing, Java objects cannot be sent through the default
Python ``Queue`` methods as calls pickle without any Java support.  This can be
overcome by wrapping Python ``Queue`` to first encode to a byte stream using
the JPickle package.  By wrapping a ``Queue`` with the Java pickler any
serializable Java object can be transferred between processes.

In addition, a standard Queue will not produce an error if is unable to pickle
a Java object.  This can cause deadlocks when using multiprocessing IPC, thus
wrapping any Queue is required.
