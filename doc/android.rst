JPype for Android
=================

We ported JPype to the Android system.  There are a number of important
differences between JPype on Android and on a JVM.  The Andoid platform uses
the Dalvik virtual machine(DVM) which supports only a portion of the Java
specification.  As a result some portions of JPype have been removed or
function differently.

Supported Functions
-------------------

The following features operate the same on both the JVM and DVM version.

- Java classes using ``jpype.JClass``
- Access to Java methods and fields
- Java arrays using ``jpype.JArray`` including slicing and direct transfers
- Java primitive types, string type, and boxed types
- Java.nio byte buffer including memory mapped data
- Python collections API for Java collection types
- Class customizers and type conversions
- Support of scientific codes such as numpy
- Implementation of Java interfaces using JProxy


Functional differences
----------------------

When using JPype on Android, the virtual machine is started prior to the start
of Python.  Thus, there is no need to lauch the machine.  Instead to use jpype
only the statement ``import jpype`` is required.  In addtion, the following
functions have been removed...

- ``jpype.startJVM`` is removed as the virtual machine cannot be started more
  than once.

- ``jpype.shutdownJVM`` is removed as DVM cannot to stopped during operation.

- ``jpype.addClassPath`` is removed as DVM does not support class path based
  jar loading.  Dex files can be loaded dynamically using the Android API.

- ``jpype.getDefaultJVMPath`` is removed as there is no JVM on Android. 

- ``jpype.beans`` has been removed as adding addition properties for semantic
  sugar increase the memory profile unnecessarily.

- attach and detaching of threads is not allowed and those entry points have been
  removed.


Removed JPype Services
----------------------

Not all JPype services are provided on Android as some functions depend on the
internals of the JVM.  These include...

- The dynamic class loader does not operate on JPype as DVM does not support
  jar files.

- jpype.imports and jpype.JPackage use the jar file system to identify packages
  which is not available on DVM.  Therefore, all classes must be loaded use
  ``jpype.JClass``.

- jpype support for Javadoc was removed as DVM does not support Javadoc jars in
  the classpath.

- jpype does not install interrupt handlers for ^C on Android.


Unsupported Java libraries
--------------------------

Some Java libraries are not supported on Android.

- Bytecode manipulation libraries to not function on DVM. This means generation
  of dynamic classes, manipulation of loaded classes, and Java agent code will
  not function.

- Some standard Java libraries are not implemented on Android such as AWT.
  Jar libraries that access these unimplemented libraries will not function.

- Private JVM internal classes such as ``sun.*`` are not implemented and cannot be used.


Behavior differences
--------------------

Some changes in JPype behavior occur simply because the DVM operates
differently.  These changes include

- java.lang.Class.forName will not load classes from classes.dex.  Attempting
  to access classes with forName will get ClassNotFoundException.  Applying
  forName to base classes that use the system classloader function as expected.


