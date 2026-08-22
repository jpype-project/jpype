Miscellaneous Tooling Topics
*****************************

This chapter collects the remaining Python-calling-Java "Miscellaneous
topics" from the original ``userguide.rst`` that don't have a
reverse-direction (Java-embedding-Python) story: Javadoc integration,
autopep8 interaction, performance notes, code completion, and garbage
collection. Of these, only garbage collection has a Java-side
counterpart — see :doc:`tooling_java` for ``PyObject`` proxy lifetime vs.
CPython refcounting and explicit resource release.

Javadoc
=======

JPype can display javadoc in ReStructured Text as part of the Python
documentation.  To access the javadoc, the javadoc package must be located on
the classpath.  This includes the JDK package documentation.

For example to get the documentation for ``java.lang.Class``, we start the JVM
with the JDK documentation zip file on the classpath.

.. code-block:: python

     import jpype
     jpype.startJVM(classpath='jdk-11.0.7_doc-all.zip')

We can then access the java docs for the String with ``help(java.lang.String)``
or for the methods with ``help(java.lang.String.trim)``.  To use the javadoc
supplied by a third party include the both the jar and javadoc in the
classpath.

.. code-block:: python

     import jpype
     jpype.startJVM(classpath=['gson-2.8.5.jar', 'gson-2.8.5-javadoc.jar'])

The parser will ignore any javadoc which cannot be extracted.  It has some
robustness against tags that are not properly closed or closed twice.  Javadoc
with custom page layouts will likely not be extracted.

If javadoc for a class cannot be located or extracted properly, default
documentation will be generated using Java reflection.

.. _miscellaneous_topics_autopep8:

Autopep8
========

When Autopep8 is applied a Python script, it reorganizes the imports to conform
to E402_. This has the unfortunate side effect of moving the Java imports above
the startJVM statement.  This can be avoided by either passing in ``--ignore
E402`` or setting the ignore in ``.pep8``.

.. _E402: https://www.flake8rules.com/rules/E402.html

Example:

.. code-block:: python

        import jpype
        import jpype.imports

        jpype.startJVM()

        from gov.llnl.math import DoubleArray


Result without ``--ignore E402``

.. code-block:: python

        from gov.llnl.math import DoubleArray  # Fails, no JVM running
        import jpype
        import jpype.imports

        jpype.startJVM()


.. _miscellaneous_topics_performance:

Performance
===========

JPype uses JNI, which is well known in the Java world as not being the most
efficient of interfaces. Further, JPype bridges two very different runtime
environments, performing conversion back and forth as needed. Both of these
can impose performance bottlenecks.

JNI is the standard native interface for most, if not all, JVMs, so there is
no getting around it. Down the road, it is possible that interfacing with CNI
(GCC's Java native interface) may be used. Right now, the best way to reduce
the JNI cost is to move time critical code over to Java.

Follow the regular Python philosophy : **Write it all in Python, then write
only those parts that need it in C.** Except this time, it's write the parts
that need it in Java.

Every time an object is passed back and forth, it will incur a conversion
cost. In cases where a given object (be it a string, an object, an array, etc
...) is passed often into Java, the object should be converted once and cached.
For most situations, this will address speed issues.

To improve speed issues, JPype has converted all of the base classes into
CPython.  This is a very significant speed up over the previous versions of
the module.  In addition, JPype provides a number of fast buffer transfer
methods. These routines are triggered automatically working with any buffer
aware class such as those in NumPy.

As a final note, while a JPype program will likely be slower than its pure
Java counterpart, it has a good chance of being faster than the pure Python
version of it. The JVM is a memory hog, but does a good job of optimizing
code execution speeds.


.. _miscellaneous_topics_code_completion:

Code completion
===============

Python supports a number of different code completion engines that are
integrated in different Python IDEs.  JPype has been tested with both the
IPython greedy completion engine and Jedi.  Greedy has the disadvantage
that it will execute code, potentially resulting in an undesirable
result in Java.

JPype is Jedi aware and attempts to provide whatever type information that
is available to Jedi to help with completion tasks.  Overloaded methods are
opaque to Jedi as the return type cannot be determined externally.  If all of
the overloads have the same return type, the JPype will add the return type
annotation permitting Jedi to autocomplete through a method return.

For example:

.. code-block:: python

        JString("hello").substring.__annotations__
        # Returns {'return': <java class 'java.lang.String'>}

Jedi can manually be tested using the following code.

.. code-block:: python

        js = JString("hello")
        src = 'js.s'
        script = jedi.Interpreter(src, [locals()])
        compl = [i.name for i in script.completions()]

This will produce a list containing all method and field that begin with
the letter "s".

JPype has not been tested with other autocompletion engines such as Kite.


.. _miscellaneous_topics_garbage_collection:

Garbage collection
===================

Garbage collection (GC) is supposed to make life easier for the programmer by
removing the need to manually handle memory.  For the most part it is a good
thing.  However, just like running a kitchen with two chefs is a bad idea,
running with two garbage collectors is also bad.  In JPype we have to contend
with the fact that both Java and Python provide garbage collection for their
memory and neither provided hooks for interacting with an external garbage
collector.

For example, Python is creating a bunch a handles to Java memory for a
period of time but they are in a structure with a reference loop internal to
Python.  The structures and handles are small so Python doesn't see an issue,
but each of those handles is holding 1M of memory in Java space.  As the heap
fills up Java begins garbage collecting, but the resources can't be freed
because Python hasn't cleaned up these structures.  The reverse occurs if a
proxy has any large NumPy arrays.  Java doesn't see a problem as it has plenty
of space to work in but Python is running its GC like mad trying to free up
space to work.

To deal with this issue, JPype links the two garbage collectors.  Python is
more aggressive in calling GC than Java and Java is much more costly than
Python in terms of clean up costs.  So JPype manages the balance.  JPype
installs a sentinel object in Java.  Whenever that sentinel is collected Java
is running out of space and Python is asked to clean up its space as well.  The
reverse case is more complicated as Python can't just call Java's expensive
routine any time it wants.  Instead JPype maintains a low-water and high-water
mark on Python owned memory.  Each time it nears a high-water mark during a
Python collection, Java GC gets called.  If the water level shrinks then
Java was holding up Python memory and the low-water mark is reset.
Depending on the amount of memory being exchanged the Java GC may trigger
as few as once every 50 Python GC cycles or as often as every other.
The sizing on this is dynamic so it should scale to the memory use of
a process.


