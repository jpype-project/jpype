.. _controlling_the_jvm:

Controlling the JVM
*******************

In this chapter, we will discuss how to control the JVM from within Python.
For the most part, the JVM is invisible to Python.  The only user controls
needed are to start up and shutdown the JVM.

This chapter covers the Python-calling-Java direction. If you are instead
embedding Python in a Java application, see :doc:`jvm_java` for the
matching Java-side interpreter lifecycle (there is no JVM to start in that
direction -- Java is already running -- but there is an embedded CPython
interpreter with its own start/stop rules).

.. _startJVM:

Starting the JVM
================

JPype requires the Java Virtual Machine (JVM) to be started before interacting
with Java. This section explains how to start the JVM, configure its options,
and troubleshoot common issues.

.. _controlling_the_jvm_key_requirements:

Key Requirements
----------------
Before starting the JVM, ensure the following prerequisites are met:

1. **Java Installation**: A Java Runtime Environment (JRE) or Java Development
   Kit (JDK) must be installed. JPype supports Java versions 11 and later.

2. **Architecture Match**: The architecture of the Python interpreter (e.g.,
   64-bit or 32-bit) must match the architecture of the installed JVM.

3. **Classpath Configuration**: Specify the paths to Java classes or JAR files
   required by your application.

4. **Environment Variable**: Ensure the `JAVA_HOME` environment variable is set
   to the directory containing the Java installation.


How to Start the JVM
--------------------
To start the JVM, use the ``jpype.startJVM()`` function. This function
initializes the JVM with the specified options. Raw JVM flags (``-ea``,
``-Xmx512m``, ...) are given as positional arguments; JPype's own behavior
is controlled with keyword arguments, the most commonly used being:

- **``classpath``**: A list of paths to JAR files or directories containing
  Java classes.
- **``convertStrings``**: A boolean flag controlling whether Java strings are
  automatically converted to Python strings.
- **``ignoreUnrecognized``**: A flag that suppresses errors for unrecognized
  JVM options.


Example: Starting the JVM
~~~~~~~~~~~~~~~~~~~~~~~~~
Here is a typical example of starting the JVM:

.. code-block:: python

    import jpype

    # Start the JVM with classpath and a raw JVM flag
    jpype.startJVM(
        "-ea",  # Enable assertions
        classpath=['lib/*', 'classes']
    )



Classpath Configuration
-----------------------
JPype supports two methods for specifying the classpath:

1. **``classpath`` Argument**: Pass a list of paths directly to the
   ``startJVM()`` function. Wildcards (``*``) are supported for JAR files in a
   directory.

.. code-block:: python

    jpype.startJVM(classpath=['lib/*', 'classes'])

2. **``addClassPath()`` Function**: Use ``jpype.addClassPath()`` to add paths
   dynamically before starting the JVM.

.. code-block:: python

    jpype.addClassPath('lib/*')
    jpype.addClassPath('classes')
    jpype.startJVM()

To debug classpath issues, print the effective classpath after starting the
JVM:

.. code-block:: python

    print(java.lang.System.getProperty('java.class.path'))

JVM Option Configuration
-------------------------
Just as ``addClassPath()`` lets independent code contribute classpath
entries before the JVM starts, ``jpype.addJVMOption()`` lets independent
code contribute JVM flags (memory settings, GC settings, ``-D``
properties, ...) without needing to own the ``startJVM()`` call site:

.. code-block:: python

    jpype.addJVMOption('-Xmx2g')
    jpype.addJVMOption('-Djava.awt.headless=true')
    jpype.startJVM()  # picks up accumulated options automatically

Accumulated options are applied first, so an explicit argument to
``startJVM()`` for the same property will take precedence. Calling
``addJVMOption()`` after the JVM has started raises ``OSError``.

.. _controlling_the_jvm_handling_jar_files_compiled_for_newer_java_versions:

Handling JAR Files Compiled for Newer Java Versions
---------------------------------------------------
If a JAR file is compiled for a newer version of Java than the JVM being used,
JPype will fail to load the classes from the JAR file, and the JVM will throw
an ``UnsupportedClassVersionError``. This occurs because the JVM cannot
interpret class files compiled for a newer version.


.. _controlling_the_jvm_behavior:

Behavior
~~~~~~~~
When attempting to load a JAR file compiled for a newer version of Java, the
JVM will throw an error similar to the following::

    java.lang.UnsupportedClassVersionError: <class_name> has been compiled by a
    more recent version of the Java Runtime (class file version X), this
    version of the Java Runtime only recognizes class file versions up to Y.

For example:

- Java 11 corresponds to class file version 55.
- Java 17 corresponds to class file version 61.

If the JAR file contains class files compiled with a newer version than the
JVM supports, the JVM cannot interpret them.

.. _controlling_the_jvm_starting_the_jvm_handling_jar_files_compiled_for_newer_java_versions_steps_to_resolve:

Steps to Resolve
~~~~~~~~~~~~~~~~
1. **Upgrade the JVM**:
   Ensure the JVM version matches or exceeds the version used to compile the
   JAR file. Use the following command to check the JVM version::

       java -version

2. **Recompile the JAR**:
   If you have access to the source code, recompile the JAR with an older
   version of Java using the ``--release`` flag. For example::

       javac --release 11 -d output_directory source_files

   This ensures compatibility with Java 11.

3. **Check the Class File Version**:
   Use the ``javap`` command to verify the class file version of the JAR::

       javap -verbose <class_name>

   Look for the ``major version`` field in the output.


.. _controlling_the_jvm_best_practices:

Best Practices
~~~~~~~~~~~~~~
- Always ensure the JVM version matches the requirements of the JAR files
  being loaded.
- If possible, use JAR files compiled for long-term support (LTS) versions of
  Java, such as Java 11 or Java 17, to maximize compatibility.

.. _controlling_the_jvm_automatic_jvm_path_detection:

Automatic JVM Path Detection
----------------------------
JPype automatically detects the path to the JVM shared library using the
``JAVA_HOME`` environment variable. If ``JAVA_HOME`` is not set, JPype searches
common directories based on the platform. You can retrieve the detected path
using:

.. code-block:: python

    print(jpype.getDefaultJVMPath())

If the automatic detection fails, specify the JVM path manually as the first
argument to ``startJVM()``:

.. code-block:: python

    jpype.startJVM('/path/to/libjvm.so', classpath=['lib/*'])


.. _controlling_the_jvm_handling_nonascii_characters_in_the_jvm_path:

Handling Non-ASCII Characters in the JVM Path
----------------------------------------------
JPype has been revised to handle JVM paths containing non-ASCII characters. Due
to restrictions in Java, JPype must make a copy of the JVM shared library when
the path includes non-ASCII characters. This ensures compatibility with the
Java Virtual Machine.

**Windows-Specific Behavior**:
On Windows, the copied JVM shared library cannot be deleted after use due to
file locking restrictions imposed by the operating system. As a result, the
temporary file will remain on disk after the JVM is shut down.

**Implications**:

- The copied JVM shared library will occupy disk space until manually removed.

- This behavior is specific to Windows and does not affect Linux or macOS.

**Best Practices**:

- Avoid using non-ASCII characters in the JVM path when running JPype on
  Windows to prevent unnecessary file duplication.

- If non-ASCII characters are unavoidable, ensure sufficient disk space is
  available for temporary files.

**Troubleshooting**:
To locate the copied JVM shared library, check the directory where the JVM path
is specified. The copied file will have the same name as the original shared
library but may include additional identifiers.

**Example**:
If the original JVM path is:
C:\Program Files\Java\jdk-11.0.7\bin\server\jvm.dll

And it contains non-ASCII characters, JPype will create a copy in a temporary directory.

This behavior is necessary to ensure compatibility with Java's handling of
non-ASCII paths.


.. _controlling_the_jvm_additional_flags_for_startjvm:

Additional Keyword Arguments for `startJVM()`
----------------------------------------------
Beyond ``classpath``, ``convertStrings``, and ``ignoreUnrecognized`` above,
``startJVM()`` accepts:

1. **``jvmpath``**: Path to the JVM shared library, if it can't be found
   automatically (see :ref:`controlling_the_jvm_automatic_jvm_path_detection`
   below). It may also be given as the first positional argument.
   Example: ``jpype.startJVM(jvmpath="/path/to/libjvm.so")``

2. **``modulepath``**: A list of paths added to the JVM's module path, for
   Java modular applications.
   Example: ``jpype.startJVM(modulepath=["modules/*"])``

3. **``add_modules``**, **``add_opens``**, **``add_exports``**,
   **``add_reads``**: Correspond to the JVM's ``--add-modules``,
   ``--add-opens``, ``--add-exports``, and ``--add-reads`` flags for the
   Java Platform Module System.

4. **``interrupt``**: Whether JPype installs a ``^C`` signal handler that
   stops the process; when ``False``, ``^C`` is instead delivered to Python
   as a normal ``KeyboardInterrupt``. Defaults to ``False`` when Python is
   running as an interactive shell.

5. **``minimum_version``**: Raises if the detected JVM is older than the
   given version string.

Anything else -- memory limits (``-Xmx512m``), GC tuning
(``-XX:+UseG1GC``), assertions (``-ea``), system properties (``-Dkey=val``)
-- is a raw JVM flag, passed positionally rather than as a keyword:

.. code-block:: python

    jpype.startJVM("-Xmx512m", "-XX:+UseG1GC", classpath=["lib/*", "classes"])

.. _string_conversions:

String Conversions
------------------
The ``convertStrings`` argument controls whether Java strings are automatically
converted to Python strings. By default, this behavior is disabled
(``convertStrings=False``) to preserve Java string methods and avoid
unnecessary conversions.


If enabled (``convertStrings=True``), Java strings are returned as Python
strings, but this can impact performance and chaining of Java string methods.
This option is considered a legacy option as it will result in unnecessary
calls to ``str()`` every time a String is passed from Java.

Best practice: Set ``convertStrings=False`` unless your application explicitly
requires automatic conversion.


.. _controlling_the_jvm_checking_jvm_state:

Checking JVM State
------------------
Use the following functions to check the status of the JVM:

- **``jpype.isJVMStarted()``**: Returns ``True`` if the JVM is running.
- **``jpype.getJVMVersion()``**: Retrieves the version of the running JVM.

Example:

.. code-block:: python

    if not jpype.isJVMStarted():
        print("JVM is not running!")
    else:
        print("JVM version:", jpype.getJVMVersion())

.. _controlling_the_jvm_common_issues_and_troubleshooting:

Common Issues and Troubleshooting
---------------------------------
1. **Classpath Errors**: Ensure that all required JAR files and directories are
   included in the classpath. Use ``java.lang.System.getProperty('java.class.path')``
   to verify the effective classpath.

2. **Architecture Mismatch**: Ensure the Python interpreter and JVM have
   matching architectures (e.g., both 64-bit or both 32-bit). Running a 64-bit
   Python interpreter with a 32-bit JVM will cause startup failures.

3. **Environment Variable Issues**: Verify that the ``JAVA_HOME`` environment
   variable is set correctly. If necessary, set it manually:
   - **Windows**: ``set JAVA_HOME=C:\Program Files\Java\jdk-<version>``
   - **Linux/Mac**: ``export JAVA_HOME=/usr/lib/jvm/java-<version>``

4. **Unrecognized JVM Options**: If you encounter errors for unrecognized JVM
   options, use the ``ignoreUnrecognized=True`` flag to suppress them.

5. **Memory Allocation Errors**: Ensure sufficient memory is allocated to the
   JVM using the ``-Xmx`` option.

6. **Debugging Startup Failures**: Enable stack traces for additional
   diagnostics:

.. code-block:: python

    import _jpype
    _jpype.enableStacktraces(True)

7. **Class Loader Conflicts with Third-Party Frameworks**: In environments
   with multiple class loaders (e.g., OSGi, Java EE application servers,
   Eclipse Modeling Framework), you may encounter "No matching overloads found"
   errors even when types appear to match. This occurs because Java considers
   classes loaded by different class loaders to be incompletely different types,
   even if they have identical names and bytecode.

   **Symptoms**: Error messages like "No matching overloads found for
   ClassName.method(Type)" where the type shown in the error matches the
   method signature exactly.

   **Limitation**: JPype cannot automatically resolve class loader conflicts.
   When a third-party framework uses its own class loader, classes loaded
   through that framework are incompatible with classes loaded through JPype's
   class loader.

   **Solution**: Explicitly load classes using the same class loader as your
   framework. When working with frameworks that use custom class loaders (e.g.,
   OSGi, EMF), you must load the Java classes through that framework's class
   loader rather than JPype's default:

   .. code-block:: python

       # Get the framework's class loader
       framework_loader = my_framework_object.getClass().getClassLoader()

       # Load the enum class using the framework's class loader
       MyEnumClass = jpype.JClass('com.example.MyEnum', loader=framework_loader)

       # Now enum values will be compatible with framework methods
       enum_value = MyEnumClass.valueOf("VALUE")
       framework_object.setEnum(enum_value)  # This will work!

   Alternatively, use Java's ``Class.forName()`` with the specific class loader:

   .. code-block:: python

       from java.lang import Class

       # Get the framework's class loader
       framework_loader = my_framework_object.getClass().getClassLoader()

       # Load the class using forName with the specific loader
       MyEnumClass = Class.forName('com.example.MyEnum', True, framework_loader)

       # Access enum constants through reflection
       enum_value = MyEnumClass.getField("VALUE").get(None)

   **Note**: You must use the framework's class loader consistently for all
   related classes. Mixing classes from different loaders will result in type
   mismatch errors. If you cannot access the framework's class loader, consider
   configuring your framework to use a shared parent class loader.


.. _controlling_the_jvm_best_practices_for_jvm_starting:

Best Practices for JVM starting
-------------------------------
- **Start Early**: Start the JVM at the beginning of your program to avoid
  issues with imports and initialization.
- **Specify Classpath Explicitly**: Use the ``classpath`` argument to ensure
  all required JAR files and directories are loaded.
- **Disable String Conversion**: Set ``convertStrings=False`` for better
  control and performance.
- **Avoid Restarting the JVM**: JPype does not support restarting the JVM after
  it has been shut down. Design your application to start the JVM once and keep
  it running for the program's lifetime.
- **Monitor Resource Usage**: If your application uses large Java objects,
  monitor memory usage to avoid out-of-memory errors.

.. _shutdownJVM:

Shutting Down the JVM
======================

At the end of your program, you may want to shut down the JVM to terminate the
Java environment explicitly. While this is possible, it is generally not
recommended unless absolutely necessary. JPype automatically shuts down the JVM
when the Python process terminates, ensuring a clean exit without manual
intervention.

.. _controlling_the_jvm_risks_of_shutting_down_the_jvm:

Risks of Shutting Down the JVM
------------------------------

Shutting down the JVM manually can lead to serious risks and instability,
especially if there are lingering Java references or shared resources. Once the
JVM is shut down, all Java objects become invalid, and any attempt to access
them will result in errors. This includes:

- **Lingering Java References**: Any Java objects held by Python will become
  invalid after the JVM is shut down. Accessing these objects will raise
  exceptions and could result in undefined behavior.

- **Shared Resources**: Shared resources such as buffers (e.g., memory mapped
  from Java to NumPy) will become unstable. Accessing these buffers after the
  JVM is shut down may cause crashes or memory corruption.

- **Proxies and Threads**: If Java threads or proxies are active when the JVM is
  shut down, they will be terminated abruptly, potentially leaving the system in
  an inconsistent state.

- **Non-Daemon Threads**: All threads must be attached as daemon threads before
  shutting down the JVM. Non-daemon threads will block the shutdown process,
  causing it to hang indefinitely. Python threads that interact with Java are
  automatically attached as daemon threads by JPype, but any custom threads
  created in Java must also be marked as daemon.

For most applications, it is safer to allow the JVM to shut down automatically
when the Python process exits. This ensures that all resources are cleaned up
properly and avoids the risks associated with manual shutdown.

.. _controlling_the_jvm_how_jpype_shuts_down_the_jvm:

How JPype Shuts Down the JVM
----------------------------

JPype performs the following steps during JVM shutdown to ensure proper cleanup:

1. **Request JVM Shutdown**: JPype requests the JVM to shut down gracefully.
2. **Wait for Non-Daemon Threads**: The JVM waits for all non-daemon threads to
   terminate. If you have active Java threads, ensure they are properly
   terminated or marked as daemon before shutting down the JVM.
3. **Execute Shutdown Hooks**: The JVM executes any registered shutdown hooks.
   These hooks can be used to clean up resources before the JVM terminates.
4. **Release JPype Reference Queue**: JPype shuts down its internal reference
   queue, which is responsible for dereferencing Python resources tied to Java
   objects.
5. **Release JPype Type Manager**: JPype releases its type manager, which
   handles mappings between Python and Java types.
6. **Unload JVM Shared Library**: The JVM shared library is unloaded, freeing
   memory used by the JVM.
7. **Finalize Python Resources**: JPype cleans up any remaining Python handles
   tied to Java objects, ensuring that no invalid references remain.

Once the JVM is shut down, all Java objects are considered dead and cannot be
reactivated. Any attempt to access their data field will raise an exception.

.. _controlling_the_jvm_managing_threads_during_jvm_shutdown:

Managing Threads During JVM Shutdown
------------------------------------

The JVM requires all threads to be attached as daemon threads during shutdown.
Daemon threads are background threads that do not prevent the JVM from
terminating. Non-daemon threads, on the other hand, will block the shutdown
process, causing it to hang indefinitely until those threads terminate.

JPype automatically attaches Python threads that interact with Java as daemon
threads. However, if you create custom threads in Java, you must explicitly mark
them as daemon threads to ensure they do not block the JVM shutdown.

To mark a Java thread as a daemon, use the following pattern:

.. code-block:: python

    import java.lang.Thread

    # Create a Java thread
    thread = java.lang.Thread()

    # Mark the thread as daemon
    thread.setDaemon(True)

    # Start the thread
    thread.start()

If you need to check whether a thread is a daemon, use the `isDaemon()` method:

.. code-block:: python

    print(f"Thread is daemon: {thread.isDaemon()}")

Ensure that all non-daemon threads are properly terminated or marked as daemon
before shutting down the JVM. Failure to do so may cause the shutdown process to
hang indefinitely.

.. _controlling_the_jvm_how_to_shut_down_the_jvm:

How to Shut Down the JVM
------------------------

If you must shut down the JVM manually, you can use the `jpype.shutdownJVM()`
function. This should only be called from the main Python thread. Calling it
from any other thread will raise an exception.

.. code-block:: python

    import jpype

    # Shut down the JVM
    jpype.shutdownJVM()


Numerous examples found on the internet explicitly state that `shutdownJVM` is a
good practice.  These examples are legacy from early development.  At the time
shutdownJVM brutally closed the JVM and bypassed all of the JVM's shutdown routines,
thus causing the program to skip over errors in the JPype module resulting
from mishandled race conditions.   While it is still acceptable to shutdown the
JVM and may be desirable to do so if a module needs a particular order to shutdown
cleanly, the use of an explicit shutdown is discouraged.


.. _controlling_the_jvm_debugging_jvm_shutdown:

Debugging JVM Shutdown
----------------------

If the JVM shutdown process hangs or fails, it is often due to lingering threads
or resources that were not properly terminated. Use the following techniques to
debug shutdown issues:

1. **Check Active Threads**: Before shutting down the JVM, check for active
   non-daemon threads that may be preventing the shutdown. You can use the
   following Java code to list all active threads:

   .. code-block:: python

       import java.lang.Thread

       # Get all active threads
       threads = java.lang.Thread.getAllStackTraces().keySet()
       for thread in threads:
           print(f"Thread: {thread.getName()}, Daemon: {thread.isDaemon()}")

   Ensure that all non-daemon threads are terminated or marked as daemon before
   calling `jpype.shutdownJVM()`.

2. **Inspect Shutdown Hooks**: If you have attached shutdown hooks, verify that
   they complete quickly and do not hang. Long-running shutdown hooks can delay
   or block JVM termination.

3. **Monitor Resource Usage**: If shared resources such as buffers are in use,
   ensure that they are properly released before shutting down the JVM. For
   example, copy buffer contents to a Python object to preserve data.

4. **Enable Debugging Logs**: JPype can provide additional diagnostics during
   the shutdown process. Use the following command to enable debugging logs:

   .. code-block:: python

       import _jpype
       _jpype.enableStacktraces(True)

   This will print detailed stack traces for exceptions that occur during the
   shutdown process.

5. **Handle Hanging Threads**: If the JVM shutdown hangs due to threads that
   cannot terminate, you can forcefully terminate the Python process using
   `os._exit()` or `java.lang.Runtime.exit()`. **However, note that calling
   `exit` will bypass normal `atexit` routines in both Python and Java.** This
   means that any cleanup tasks, such as writing logs (e.g., Jacoco coverage
   reports) or flushing buffers, will not be executed. Use this approach only
   as a last resort when all other debugging techniques fail.

.. _controlling_the_jvm_best_practices_for_jvm_shutdown:

Best Practices for JVM Shutdown
-------------------------------

- **Avoid Manual Shutdown**: Whenever possible, allow the JVM to shut down
  automatically when the Python process exits. This avoids the risks of lingering
  references and shared resource instability.

- **Terminate Threads Properly**: Ensure all non-daemon Java threads are
  terminated or marked as daemon before shutting down the JVM. Failure to do so
  may cause the shutdown process to hang indefinitely.

- **Handle Buffers Carefully**: If you are using shared buffers (e.g., Java
  direct buffers with NumPy), avoid accessing them after the JVM is shut down.
  If you need to preserve data, copy the buffer contents to a Python object
  before shutting down the JVM.

- **Use Shutdown Hooks**: Attach shutdown hooks only when necessary to clean up
  resources. Ensure that the hooks complete quickly to avoid delaying JVM
  termination.

- **Avoid Forceful Termination**: Avoid using `os._exit()` or `java.lang.Runtime.exit()`
  unless absolutely necessary. These methods prevent normal cleanup routines from
  executing, which can result in missing logs, incomplete resource cleanup, or
  other unintended consequences.



