JImport
=======
Module for dynamically loading Java Classes using the import system.

This is a replacement for the jpype.JPackage("com").fuzzy.Main type syntax.
It features better safety as the objects produced are checked for class
existence.

Setup
-----

To enable Java imports, you must first start the JVM and then import the
``jpype.imports`` module:

 .. code-block:: python

  import jpype
  import jpype.imports  # Enable Java imports

  # Start the JVM
  jpype.startJVM()

  # Now you can import Java classes
  import java.lang
  from java.util import ArrayList

Once ``jpype.imports`` is imported, Python's import system will automatically
load Java classes and packages as if they were Python modules.

Import Styles
-------------

This module supports three different styles of importing java classes.

1) Import of the package path
-----------------------------

**import <java_package_path>**

Importing a series of package creates a path to all classes contained
in that package.  The root package is added to the global scope.
Imported packages are added to the directory of the base module.

 .. code-block:: python

  import java

  mystr = java.lang.String('hello')
  mylist = java.util.LinkedList()
  path = java.nio.files.Paths.get() 

2) Import of the package path as a module
-----------------------------------------

**import <java_package> as <var>**

A package can be imported as a local variable. This provides access to
all Java classes in that package including contained packages. 

Example:
 .. code-block:: python

  import java.nio as nio
  bb = nio.ByteBuffer()
  path = nio.file.Path()

3) Import a class from an object
--------------------------------

**from <java_package> import <class>[,<class>\*] [as <var>]**

An individual class can be imported from a java package. This supports
inner classes as well.

Example:

 .. code-block:: python

  # Import one class
  from java.lang import String
  mystr = String('hello')

  # Import multiple classes
  from java.lang import Number,Integer,Double
  # Import java inner class java.lang.ProcessBuilder.Redirect
  from java.lang.ProcessBuilder import Redirect

This method can also be used to import a static variable or method
from a class.  Wildcards import all packages and public classes into
the global scope.

Import caveats
--------------

Keyword naming
~~~~~~~~~~~~~~

Occasionally a java class may contain a python keyword.
Python keywords as automatically remapped using trailing underscore.

Example::

  from org.raise_ import Object  => imports "org.raise.Object"

Lazy imports (Python 3.15+)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Java packages and classes can be imported with the ``lazy`` import
statements added in Python 3.15 (:pep:`810`).  The lookup is deferred until
the name is first used, so a lazy import may even be written before the JVM
is started:

 .. code-block:: python

  import jpype
  import jpype.imports

  lazy from java.lang import String
  lazy import java.util as ju

  jpype.startJVM()

  s = String('hello')        # resolved here
  mylist = ju.ArrayList()

A class that does not exist is reported as an ``ImportError`` at first use
rather than at the import statement.

Importing a Java class initializes it, which runs its static initializer.
A lazy import defers that to the first use of the name, and an error in the
static initializer is likewise raised at first use.  A class imported only
for the side effect of its static initializer (for example registering a
JDBC driver) should be imported eagerly, or initialized explicitly with
``jpype.JClass``.

When global lazy imports are enabled (``python -X lazy_imports=all`` or
``PYTHON_LAZY_IMPORTS=all``) every plain module-level import is deferred,
including ``import jpype.imports``.  That import is only needed for its side
effect of installing the Java import hook, so the name is never used and the
hook would never be installed.  ``jpype.startJVM()`` detects a pending
``jpype.imports`` and loads it, so the usual order of importing
``jpype.imports`` before starting the JVM works unchanged.  If
``jpype.imports`` is imported only after the JVM is started, force it to load
by referencing it once:

 .. code-block:: python

  import jpype
  jpype.startJVM()

  import jpype.imports
  jpype.imports              # loads the Java import hook

  from java.util import ArrayList

Limitations
~~~~~~~~~~~

* Non-static members can be imported but can not be called without an
  instance. JPype does not provide an easy way to determine which
  functions objects can be called without an object.

