.. _miscellaneous_topics_using_jpype_for_debugging_java_code:

Using JPype for debugging Java code
*************************************

This chapter covers the Python-calling-Java direction only -- debugging a
JVM being driven from Python, and getting diagnostics out of JPype itself.
There is no reverse-direction (Java-embedding-Python) counterpart planned;
debugging a Python interpreter embedded in Java is a different problem
(standard Python debugger attachment, not covered here).

One common use of JPype is to function as a Read-Eval-Print Loop for Java. When
operating Java though Python as a method of developing or debugging Java there
are a few tricks that can be used to simplify the job.  Beyond being able to
probe and plot the Java data structures interactively, these methods include:

1) Attaching a debugger to the Java JVM being run under JPype.
2) Attaching debugging information to a Java exception.
3) Serializing the state of a Java process to be evaluated at a later point.

We will briefly discuss each of these methods.


.. _miscellaneous_topics_using_jpype_for_debugging_java_code_attaching_a_debugger:

Attaching a Debugger
--------------------

Interacting with Java through a shell is great, but sometimes it is necessary
to drop down to a debugger. To make this happen we need to start the JVM
with options to support remote debugging.

We start the JVM with an agent which will provide a remote debugging port which
can be used to attach your favorite Java debugging tool.  As the agent is
altering the Java code to create additional debugging hooks, this process can
introduce additional errors or alter the flow of the code.  Usually this is
used by starting the JVM with the agent, placing a pause marker in the Python
code so that developer can attach the Java debugger, executing the Python code
until it hits the pause, attaching the debugger, setting break point in Java,
and then asking Python to proceed.

So lets flesh out the details of how to accomplish this...

.. code-block:: python

    jpype.startJVM("-Xint", "-Xdebug", "-Xnoagent",
      "-Xrunjdwp:transport=dt_socket,server=y,address=12999,suspend=n")

Next, add a marker in the form of a pause statement at the location where
the debugger should be attached.

.. code-block:: python

    input("pause to attach debugger")
    myobj.callProblematicMethod()

When Python reaches that point during execution, switch to a Java IDE such as
NetBeans and select Debug : Attach Debugger.  This brings up a window (see
example below).  After attaching (and setting desired break points) go back to
Python and hit enter to continue.  NetBeans should come to the foreground when
a breakpoint is hit.

.. image:: attach_debugger.png


Attach data to an Exception
---------------------------

Sometimes getting to the level of a debugger is challenging especially if the
code is large and error occurs rarely. In this case, it is often beneficial to
attach data to an exception. To achieve this, we need to write a small utility
class. Java exceptions are not strictly speaking expandable, but they can be
chained. Thus, if we create a dummy exception holding a ``java.util.Map`` and
attach it as the cause of the exception, it will be passed back down the
call stack until it reaches Python. We can then use ``getCause()`` to retrieve
the map containing the relevant data.


.. _miscellaneous_topics_capturing_the_state:

Capturing the state
-------------------

If the program is not running in an interactive shell or the program run time
is long, we may not want to deal with the problem during execution. In this
case, we can serialize the state of the relevant classes and variables. To use
this option, we must make sure all of the classes in Java that we are using
are ``Serializable``.  Then add a condition that detects the faulty algorithm state.
When the fault occurs, create a ``java.util.HashMap`` and populate it with
the values to be examined from within Python.  Use serialization to write
the entire structure to a file.  Execute the program and collect all of the
state files.

Once the state files have been collected, start Python with an interactive
shell and launch JPype with a classpath for the jars.  Finally,
deserialize the state files to access the Java structures that have
been recorded.


.. _miscellaneous_topics_getting_additional_diagnostics:

Getting Additional Diagnostics
==============================

For the most part, JPype operates as intended, but that does not mean there are
no bugs or edge cases. Given the complexity of interactions between Python and
Java, untested scenarios may occasionally arise. JPype provides several
diagnostic tools to assist in debugging these issues. These tools require
accessing private JPype symbols, which may change in future releases. As such,
they should not be used in production code.

.. _checking-type-cast:

Checking the Type of a Cast
---------------------------

Sometimes it is difficult to understand why a particular method overload is
selected by the method dispatch. To check the match type for a conversion, use
the private method ``Class._canConvertToJava``. This will return a string
indicating the type of conversion performed: ``none``, ``explicit``,
``implicit``, or ``exact``.

To test the result of the conversion process, call ``Class._convertToJava``.
Unlike an explicit cast, this method attempts to perform the conversion without
bypassing the logic involved in casting. It replicates the exact process used
when a method is called or a field is set.

.. _cpp-exceptions:

C++ Exceptions in JPype
------------------------

Internally, JPype can generate C++ exceptions, which are converted into Python
exceptions for the user. To trace an error back to its C++ source, you can
enable stack traces for C++ exceptions. Use the following command:

.. code-block:: python

   import _jpype
   _jpype.enableStacktraces(True)

Once enabled, all C++ exceptions that fall through a C++ exception handling
block will produce an augmented stack trace. If the JPype source code is
available, the stack trace can even include the corresponding lines of code
where the exceptions occurred. This can help identify the source of errors that
originate in C++ code but propagate to Python as exceptions.

.. _tracing:

Tracing
-------

Tracing mode logs every JNI call, along with object addresses and exceptions,
to the console. To enable tracing, JPype must be recompiled with the CMake
``ENABLE_TRACING`` option.

This can be done via:

.. code-block:: shell

    pip install -v -e . -C cmake.args="-DENABLE_TRACING=ON"


Tracing is useful for identifying failures that originate in one JNI call but
manifest later. However, this mode produces verbose logs and is recommended
only for advanced debugging.


.. _instrumentation:

Instrumentation
---------------

JPype supports an instrumentation mode for testing error-handling paths. This
mode allows you to simulate faults at designated points in JPype's execution
flow. To enable instrumentation, recompile JPype with the CMake
``ENABLE_COVERAGE`` option (``pip install -e . --config-setting
cmake.args="-DENABLE_COVERAGE=ON"``).

Once instrumentation is enabled, use the private module command
``_jpype.fault`` to trigger an error. The argument to the fault command must be
the name of a function or a predefined fault point. When the fault point is
encountered, a ``SystemError`` is raised. Instrumentation is primarily useful
for verifying the robustness of JPype's exception handling mechanisms.


.. _using-debugger:

Using a Debugger
----------------

If JPype crashes, it may be necessary to use a debugger to obtain a backtrace.
However, debugging JPype can be challenging due to the JVM's handling of
segmentation faults. Java installs its own signal handlers that take over the
process on a segfault, which usually corrupts the stack by the time a core
file is examined, and can also cause a debugger attached from the start to
crash immediately rather than let you catch the fault.

To debug JPype using tools such as ``gdb`, you must configure the debugger to
ignore segmentation faults intentionally triggered by the JVM. For example, use
the following command to start ``gdb`` and ignore the first fault:

.. code-block:: shell

   gdb --args python script.py
   (gdb) handle SIGSEGV nostop noprint pass

This configuration allows the debugger to bypass JVM-related faults while
capturing legitimate errors. Additionally, disable Python's fault handler to
avoid interference with segmentation fault reporting. There is no known way
to stop the JVM from altering the stack frames in a non-interactively
produced core dump, so an interactive gdb session (as above) is the
reliable option.

Older JVMs can add their own wrinkle: Open-JDK 8 has had gdb compatibility
problems, and starting JPype under gdb may produce ``gdb.error: No type
named nmethod.`` Upgrading to Open-JDK 9 or later resolves it.

If the crash looks like it's landing in code that makes no sense for what
you called, suspect Python's memory allocator: CPython recycles freed
objects from allocation pools rather than always allocating fresh memory,
so a use-after-free can silently vector into an unrelated object of a
different type instead of crashing where the bug actually is. Forcing a
different allocation policy via the ``PYTHONMALLOC`` environment variable
can avoid the recycling and make the fault easier to catch, though it may
also shift execution enough that the bug doesn't reproduce the same way.

If you hit a segfault that immediately shows a clean, meaningful backtrace
at a null-pointer write rather than a garbled crash, that's likely JPype's
own deliberate-crash mechanism, not memory corruption: JPype intentionally
triggers a null-pointer write in a handful of catastrophic startup/resource
failure paths where it cannot deliver an exception to either Python or
Java, specifically so gdb gets a usable stack trace instead of a swallowed
signal. If you see this, it indicates a genuine unrecoverable failure --
please open a GitHub issue with the backtrace.


.. _caller sensitive:

Caller-Sensitive Methods
-------------------------

Java's security model tracks the caller of certain methods to determine the
level of access. These methods, known as "caller-sensitive methods," require
special handling in JPype. Examples of caller-sensitive methods include
``Class.forName``, ``java.lang.ClassLoader`` methods, and certain methods in
``java.sql.DriverManager``.

To handle caller-sensitive methods, JPype routes calls through an internal
Java package, ``org.jpype``, which executes the method within the JVM. This
ensures proper security context and avoids access errors. Although this
mechanism introduces slight overhead, it is necessary for compatibility with
Java's security model.
