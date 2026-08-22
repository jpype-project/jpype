
Python QuickStart Guide (for Java)
===================================

This is a 5-minute tour of the reverse bridge: a Java application that embeds
Python and calls into it, rather than a Python program calling Java (that
direction is :doc:`quickguide_py`). Every snippet below runs as shown.

.. contents::
   :local:
   :depth: 1


Starting the interpreter
-------------------------

``org.jpype.MainInterpreter`` is the singleton entry point. Starting it boots
an embedded CPython interpreter inside the JVM process; :doc:`limitations_java`
covers what you can't do afterwards (restart it, run a second one).

.. code-block:: java

    import org.jpype.MainInterpreter;
    import org.jpype.Interpreter;
    import python.lang.PyBuiltIn;

    MainInterpreter interpreter = MainInterpreter.getInstance();
    if (!interpreter.isStarted())
        interpreter.start(new String[0]);

    PyBuiltIn builtin = interpreter.getBuiltIn();

    // ... use the interpreter ...

    interpreter.close();

``PyBuiltIn`` is the Java-side handle for Python's builtins (``str()``,
``int()``, ``dict()``, ``eval()``, ...). ``org.jpype.Script`` wraps a
``PyBuiltIn`` together with its own private globals/locals dict, which is
usually more convenient than sharing the interpreter's top-level namespace:

.. code-block:: java

    import org.jpype.Script;

    Script context = new Script(interpreter);


Evaluating and executing code
-------------------------------

``eval()`` returns a value; ``exec()`` runs statements and returns nothing.
Both take the script's own globals/locals, so names defined by ``exec()``
are visible to later ``eval()`` calls on the same ``Script``.

.. code-block:: java

    context.exec(
            "class Point:\n" +
            "    def __init__(self, x, y):\n" +
            "        self.x, self.y = x, y\n" +
            "p = Point(1, 2)\n");

    python.lang.PyObject p = context.eval("p");
    python.lang.PyObject x = context.getattr(p, "x");


Basic types
-----------

Every ``python.lang`` type (``PyString``, ``PyInt``, ``PyFloat``, ``PyDict``,
``PyList``, ``PyTuple``, ``PySet``, ...) implements the matching Java
collection/comparison interfaces where it makes sense, so a ``PyDict`` behaves
like a ``java.util.Map`` and a ``PyList`` like a ``java.util.List``.

.. code-block:: java

    import python.lang.PyString;
    import python.lang.PyDict;

    PyString s = context.str("hello world");
    boolean has = s.containsSubstring("world");   // true

    PyDict dict = context.dict();
    dict.putAny("a", 1);
    dict.putAny("b", 2);
    int size = dict.size();                        // 2, java.util.Map-style


Calling into Python
--------------------

Any ``PyCallable`` (a Python function, bound method, lambda, or callable
object) can be invoked directly with plain Java arguments via
``call(Object... args)``, or built up incrementally with ``call()``'s
fluent ``CallBuilder`` when you need keyword arguments.

.. code-block:: java

    import python.lang.PyCallable;
    import python.lang.PyObject;

    PyCallable add = (PyCallable) context.eval("lambda x, y: x + y");
    PyObject result = add.call(2, 3);               // 5

Async calls run on a bounded native thread pool -- see
:doc:`limitations_java` for the GIL caveat that applies to them.


Standard library front ends
-----------------------------

A handful of standard-library modules have a typed Java front end reachable
through a ``<Module>.using(context)`` factory, so you don't have to write
raw ``eval()``/``exec()`` strings for common container and I/O work. See
:doc:`limitations_java` for the full list and what's still untyped.

.. code-block:: java

    import python.collections.PyCollections;
    import python.collections.PyDeque;
    import python.io.IO;
    import python.io.PyStringIO;
    import python.pathlib.Pathlib;
    import python.pathlib.PyPath;

    PyDeque deque = PyCollections.using(context).deque();
    deque.append(context.$int(1));

    PyStringIO buf = IO.using(context).stringIO();
    buf.write("hello");
    String contents = buf.getvalue().toString();

    PyPath path = Pathlib.using(context).path("/a/b", "c.txt");
    String name = path.name();                      // "c.txt"


Exceptions
----------

Python exceptions raised through ``eval()``/``exec()`` or a ``PyCallable``
call surface as Java exceptions under ``python.exceptions``, mirroring
Python's own hierarchy (``PyValueError``, ``PyZeroDivisionError``, ...) so
you can catch them by type.

.. code-block:: java

    import python.exceptions.PyValueError;

    try
    {
        context.eval("int('not a number')");
    } catch (PyValueError ex)
    {
        // ex.get() returns the underlying PyExc for further inspection
    }


Where to next
--------------

- :doc:`limitations_java` -- what the reverse bridge doesn't cover yet, and
  the subinterpreter/GIL/async model.
- :doc:`spi` -- how the ``<Module>.using(context)`` front ends are built and
  registered, if you want to add one for a module that isn't covered yet.
