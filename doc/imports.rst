JImport
=======
Module for dynamically loading Java Classes using the import system.

This is a replacement for the jpype.JPackage("com").fuzzy.Main type syntax.
It features better safety as the objects produced are checked for class
existence. To use Java imports, import the domains package prior to
importing a Java class.

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

Limitations
~~~~~~~~~~~

* Non-static members can be imported but can not be called without an
  instance. JPype does not provide an easy way to determine which
  functions objects can be called without an object.

