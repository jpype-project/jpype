
Threading (for Java)
========================

This is the Java-side counterpart to :doc:`threading_py`: what it means to
call into Python from multiple Java threads, and the async call support
built on top of that. Read :doc:`jvm_java`'s "GIL model, briefly" section
first if you haven't -- this chapter is that section in full. Every claim
here is grounded in ``python.lang.PyCallable``.

.. contents::
   :local:
   :depth: 1


The GIL-per-call model
--------------------------

CPython's Global Interpreter Lock (GIL) allows only one thread to execute
Python bytecode at a time, no matter how many OS threads exist. Every call
from Java into Python -- ``eval()``, ``exec()``, a ``PyCallable``
invocation, even an incidental ``toString()`` on a wrapped object --
acquires the GIL for the duration of that call and releases it when control
returns to Java. This happens automatically; there is no explicit
acquire/release API for ordinary synchronous calls.

The practical consequence: calling into Python from **any** Java thread is
safe (you don't need to route calls through one dedicated thread), but
calling from **many** threads concurrently doesn't buy you parallel Python
execution -- the GIL serializes it underneath, same as it would for
`threading` in pure Python. What you do get is that a slow or blocked
Python call on one thread doesn't have to freeze the rest of your Java
application; other Java threads keep running until *they* need the GIL
too.

Subinterpreters add a wrinkle, but not the one you'd expect. ``_jpype`` is
built with multi-phase init (it declares ``Py_mod_multiple_interpreters =
Py_MOD_PER_INTERPRETER_GIL_SUPPORTED``), so it is eligible for genuine,
own-GIL subinterpreters. ``org.jpype.SubInterpreterBuilder`` launches
exactly that: its default configuration (``ownGil()``) gives each
subinterpreter its own GIL and its own obmalloc arena -- full PEP 684
isolation, not shared with the main interpreter or any other
subinterpreter. The bare, no-argument ``org.jpype.SubInterpreter.start()``
is the one exception: it always launches with the older **legacy**
configuration -- shared process GIL, shared obmalloc -- kept for
backward compatibility, not because ``_jpype`` requires it. Use the
builder (and ``SubInterpreterBuilder.elevated()`` if you specifically need
the shared-GIL combination, e.g. to import a single-phase-init extension
that own-GIL isolation would otherwise reject) rather than assuming
subinterpreters are always legacy-style -- see :doc:`limitations_java` for
the full caveat, including the cross-interpreter proxy-smuggling guard
that still applies even under own-GIL isolation.


Async calls
--------------

``PyCallable.callAsync(PyTuple, PyDict)`` and
``callAsyncWithTimeout(PyTuple, PyDict, long)`` run the call on a bounded,
fixed-size (32) daemon thread pool and return a ``java.util.concurrent.Future``,
rather than blocking the calling thread.

.. code-block:: java

    import java.util.concurrent.Future;
    import java.util.concurrent.TimeUnit;

    PyCallable fn = (PyCallable) context.eval("lambda x, y: x + y");
    Future<PyObject> future = fn.callAsync(context.tuple(2, 3), context.dict());
    PyObject result = future.get(10, TimeUnit.SECONDS);      // "5"

    // with a timeout on the Python-side call itself:
    Future<PyObject> withTimeout =
            fn.callAsyncWithTimeout(context.tuple(2, 3), context.dict(), 5000);

Because every one of those pool threads still has to acquire the same
process GIL to actually run the call, the pool bounds *concurrency* (how
many calls can be in flight, contending for the GIL, at once), not
*throughput* -- don't expect near-linear speedup from firing off a large
batch of ``callAsync`` calls. What it does give you is non-blocking
dispatch: the calling thread gets a ``Future`` back immediately and can go
do other work while the pool serializes the actual Python execution.

The pool's threads are daemons, so they never keep the JVM alive on their
own; ``MainInterpreter.close()`` shuts the pool down and waits for
in-flight calls to finish before finalizing the interpreter.


What's safe from a background thread
-----------------------------------------

- Calling any ``PyObject``/``PyCallable`` method from any Java thread:
  safe, each call acquires/releases the GIL on its own.
- Sharing one ``PyObject`` (a function, a container, ...) across multiple
  threads making concurrent calls into it: safe at the JPype layer, even
  with many concurrent calls against the same callable. Whether it's safe
  at the *Python* level depends on what that Python code does (mutating
  shared state without a Python-side lock is exactly as unsafe as it would
  be in pure Python).
- A call that never returns control to Java -- blocked native code inside
  a custom SPI provider, an infinite loop -- holds the GIL for that
  duration and can starve every other thread waiting to call into Python.
  There is no forced preemption; a hung Python call is a hung GIL for
  everyone.


Where to next
---------------

- :doc:`jvm_java` -- interpreter startup/shutdown, and the "GIL model,
  briefly" summary this chapter expands on.
- :doc:`limitations_java` -- the subinterpreter isolation caveat and the
  async thread pool's fixed size, in full.
