Known Limitations (Python calling Java)
========================================

This section lists those limitations that are unlikely to change, as they come
from external sources. It covers the Python-to-Java direction; see
:doc:`limitations_java` for limitations specific to Java calling Python via
the reverse bridge.


.. _miscellaneous_topics_jpype_known_limitations_annotations:

Annotations
-----------

Some frameworks such as Spring use Java annotations to indicate specific
actions.  These may be either runtime annotations or compile time annotations.
Occasionally while using JPype someone would like to add a Java annotation to a
JProxy method so that a framework like Spring can pick up that annotation.

JPype uses the Java supplied ``Proxy`` to implement an interface.  That API
does not support addition of a runtime annotation to a method or class.  Thus,
all methods and classes when probed with reflection that are implemented in
Python will come back with no annotations.

Further, the majority of annotation magic within Java is actually performed at
compile time.  This is accomplished using an annotation processor.  When a
class or method is annotated, the compiler checks to see if there is an
annotation processor which then can produce new code or modify the class
annotations.  As this is a compile time process, even if annotations were added
by Python to a class they would still not be active as the corresponding
compilation phase would not have been executed.

This is a limitation of the implementation of annotations by the Java virtual
machine.  It is technically possible though the use of specialized code
generation with the ASM library or other code generation to add a runtime
annotation.  Or through exploits of the Java virtual machine annotation
implementation one can add annotation to existing Java classes.  But these
annotations are unlikely to be useful. As such JPype will not be able to
support class or method annotations.



.. _miscellaneous_topics_restarting_the_jvm:

Restarting the JVM
-------------------

JPype caches many resources to the JVM. Those resource are still allocated
after the JVM is shutdown as there are still Python objects that point to those
resources.  If the JVM is restarted, those stale Python objects will be in a
broken state and the new JVM instance will obtain the references to these
resulting in a memory leak. Thus it is not possible to start the JVM after it
has been shut down with the current implementation.


.. _miscellaneous_topics_running_multiple_jvm:

Running multiple JVM
--------------------

JPype uses the Python global import module dictionary, a global Python to Java
class map, and global JNI TypeManager map.  These resources are all tied to the
JVM that is started or attached. Thus operating more than one JVM does not
appear to be possible under the current implementation.  Further, as of Java
1.2 there is no support for creating more than one JVM in the same process.

Difficulties that would need to be overcome to remove this limitation include:

- Finding a JVM that supports multiple JVMs running in the same process.
  This can be achieved on some architectures by loading the same shared
  library multiple times with different names.
- Alternatively as all available JVM implementations support on one JVM
  instance per process, a communication layer would have to proxy JNI
  class from JPype to another process. But this has the distinct problem that
  remote JVMs cannot register native methods nor share memory without
  considerable effort.
- Which JVM would a static class method call. The class types
  would need to be JVM specific (ie. ``JClass('org.MyObject', jvm=JVM1)``)
- How would a wrapper from two different JVM coexist in the
  ``jpype._jclass`` module with the same name if different class
  is required for each JVM.
- How would the user specify which JVM a class resource is created in
  when importing a module.
- How would objects in one JVM be passed to another.
- How can boxed and String types hold which JVM they will box to on type
  conversion.

Thus it appears prohibitive to support multiple JVMs in the JPype
class model.


.. _miscellaneous_topics_jpype_known_limitations_java_library_path:

Modifying java.library.path at runtime
---------------------------------------

The JVM property ``java.library.path`` specifies directories to search for native
libraries when using ``System.loadLibrary()``. This path is read by the JVM only
during startup and cannot be modified at runtime after the JVM has been started.

This is a limitation of the JVM itself, not JPype. While various workarounds
exist that use reflection to modify private JVM fields, these are fragile hacks
that rely on internal implementation details and are not supported.

The recommended approach for loading native libraries dynamically is to use
``System.load()`` with absolute paths instead of ``System.loadLibrary()``:

.. code-block:: python

    import jpype
    import jpype.imports
    from java.lang import System

    # Load a native library with an absolute path
    System.load('/absolute/path/to/library.so')

This approach works at any time after JVM startup and does not depend on
``java.library.path``. If you need to set ``java.library.path`` for third-party
libraries, it must be configured when calling ``startJVM()``:

.. code-block:: python

    import jpype

    jpype.startJVM('-Djava.library.path=/path/to/native/libs')

Note that this limitation makes it difficult to use multiple Python packages
that each wrap different Java libraries requiring different native library
paths in the same process, as only one ``java.library.path`` can be set at
JVM startup.


.. _miscellaneous_topics_jpype_known_limitations_errors_reported_by_python_fault_handler:

Errors reported by Python fault handler
---------------------------------------

The JVM takes over the standard fault handlers resulting in unusual behavior if
Python handlers are installed.  As part of normal operations the JVM will
trigger a segmentation fault when starting and when interrupting threads.
Python's fault handler can intercept these operations and interpret these as
real faults.  The Python fault handler will then report extraneous fault
messages or prevent normal JVM operations.  When operating with JPype, the
Python fault handler module should be disabled.

This is particularly a problem for running under pytest as the first action it
performs is to take over the error handlers. This can be disabled by adding
this block as a fixture at the start of the test suite.

.. code-block:: python

    try:
        import faulthandler
        faulthandler.enable()
        faulthandler.disable()
    except:
        pass

This code enables fault handling and then returns the default handlers which
will point back to those set by Java.

Python faulthandlers can also interfere with the JIT compiler.  The Java
Just-In-Time (JIT) compiler determines when a portion of code needs to be
compiled into machine code to improve performance.  When it does, it triggers
either a SIGSEGV or SIGBUS depending on the machine architecture which breaks
out any threads which are currently executing the existing code.  Because the
JIT compiler self triggers, this will cause a failure to appear in a call which
worked earlier in the execution without an issue.

If the Python fault handler interrupts this process, it will produce a "Fatal
Python Error:" followed by either SIGSEGV or SIGBUS.  This error message then
fails to hit the needed Java handler resulting in a crash.  This message will
disappear if the JIT compiler is disabled with the option "-Xint".  Running
without the JIT compiler creates a severe performance penalty so disabling the
fault handler should be the preferred solution.

Some modules such as Abseil Python Common Libraries (absl) automatically and
unconditionally install faulthandlers as part of their normal operation.  To
prevent an issue simply insert a call to disable the faulthandler after the
module has enabled it, using

.. code-block:: python

    import faulthandler
    faulthandler.disable()

For example, absl installs faulthandlers in ``app.run``, thus the first call to
main routine would need to disable faulthandlers to avoid potential crashes.



.. _miscellaneous_topics_unsupported_java_versions:

Unsupported Java Versions
-------------------------

JPype now requires the use of the module API, which was introduced in **Java
9**. As a result, the earliest version of Java supported by JPype is **Java
11**, which is part of the Long-Term Support (LTS) release.

If you need to use **Java 8**, you must use JPype version **1.5.2 or earlier**,
as newer versions of JPype no longer support Java 8.



.. _miscellaneous_topics_unsupported_python_versions:

Unsupported Python versions
---------------------------


.. _miscellaneous_topics_python_38_and_earlier:

Python 3.8 and earlier
~~~~~~~~~~~~~~~~~~~~~~

The oldest version of Python that we currently support is Python 3.8. Before
Python 3.8, recent changes in memory models make continued support
impractical, and before Python 3.5 there were a number of structural
difficulties in the object model and the buffering API. In principle, those
older-version features could be excised from JPype to extend support to an
earlier Python 3 series version, but that is unlikely to happen without a
significant effort.


.. _miscellaneous_topics_python_2:

Python 2
~~~~~~~~

CPython 2 support was removed starting in 2020.  Please do not report to us
that Python 2 is not supported.  Python 2 was a major drag on this project for
years.  Its object model is grossly outdated and thus providing for it greatly
impeded progress.  When the life support was finally pulled on that beast,
I like many others breathed a great sigh of relief and gladly cut out the
Python 2 code.  Since that time JPype operating speed has improved anywhere
from 300% to 10000% as we can now implement everything back in CPython rather
than band-aiding it with interpreted Python code.


.. _miscellaneous_topics_pypy:

PyPy
~~~~

The GC routine in PyPy 3 does not play well with Java. It runs when it thinks
that Python is running out of resources. Thus a code that allocates a lot
of Java memory and deletes the Python objects will still be holding the
Java memory until Python is garbage collected. This means that out of
memory failures can be issued during heavy operation.  We have addressed linking
the garbage collectors between CPython and Java, but PyPy would require a
modified strategy.

Further, when we moved to a completely Python 3 object model we unfortunately
broke some of the features that are different between CPython and PyPy.  The
errors make absolutely no sense to me.  So unless a PyPy developer generously
volunteering time for this project, this one is unlikely to happen.


.. _miscellaneous_topics_jython_python:

Jython Python
~~~~~~~~~~~~~

If for some reason you wandered here to figure out how to use Java from
Jython using JPype, you are clearly in the wrong place. On the other hand,
if you happen to be a Jython developer who is looking for inspiration on how
to support a more JPype like API that perhaps we can assist you.  Jython aware
Python modules often mistake JPype for Jython at least up until the point
that differences in the API triggers an error.



.. _miscellaneous_topics_unsupported_java_virtual_machines:

Unsupported Java virtual machines
---------------------------------

The open JVM implementations *Cacao* and *JamVM* are known not to work with
JPype.


.. _miscellaneous_topics_unsupported_platforms:

Unsupported Platforms
---------------------

Some platforms are problematic for JPype due to interactions between the
Python libraries and the JVM implementation.


.. _miscellaneous_topics_cygwin:

Cygwin
~~~~~~

Cygwin was usable with previous versions of JPype, but there were numerous
issues for which there was no good solution.

Cygwin does not appear to pass environment variables to the JVM properly
resulting in unusual behavior with certain windows calls. The path
separator for Cygwin does not match that of the Java DLL, thus specification
of class paths must account for this.  Threading between the Cygwin libraries
and the JVM was often unstable.
