JImport
=======
Module for dynamically loading Java Classes using the import system.

This is a replacement for the jpype.JPackage("com").fuzzy.Main type syntax.
It features better safety as the objects produced are checked for class
existence. To use java imports, import the domains package prior to
importing a java class.

This module supports three different styles of importing java classes.

1) Import of the package path
-----------------------------

**import <java_package_path>**

Importing a series of package creates a path to all classes contained
in that package. It does not provide access the the contained packages.
The root package is added to the global scope. Imported packages are
added to the directory of the base module.

 .. code-block:: python

  import java.lang      # Adds java as a module
  import java.util

  mystr = java.lang.String('hello')
  mylist = java.util.LinkedList()
  path = java.nio.files.Paths.get() # ERROR java.nio.files not imported

2) Import of the package path as a module
-----------------------------------------

**import <java_package> as <var>**

A package can be imported as a local variable. This provides access to
all java classes in that package. Contained packages are not available.

Example:
 .. code-block:: python

  import java.nio as nio
  bb = nio.ByteBuffer()
  path = nio.file.Path()   # ERROR subpackages file must be imported

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
from a class.

Import caveats
--------------

Wild card Imports
~~~~~~~~~~~~~~~~~

Wild card imports for classes will import all static method and
fields into the global namespace. They will also import any
inner classes that have been previously be accessed.

Wild card importation of package symbols are not currently supported
and have unpredictable effects. Because of the nature of class loaders
it is not possible to determine what classes are currently loaded. Some
classes are loaded by the boot strap loader and thus are not available
for discovery.

As currently implemented [from <java_package> import \*] will import
all classes and static variables which have already been imported by
another import call. As a result which classes will be imported
is based on the code pat and thus very unreliable.

It is possible to determine the classes available using Guava for
java extension jars or for jars specifically loaded in the class path.
But this is sufficiently unreliable that we recommend not using wildcards
for any purpose.

Keyword naming
~~~~~~~~~~~~~~

Occasionally a java class may contain a python keyword.
Python keywords as automatically remapped using trailing underscore.

Example::

  from org.raise_ import Object  => imports "org.raise.Object"

Controlling Java package imports
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By default domains imports four top level domains (TLD) into the python
import system (com, gov, java, org). Additional domains can be added
by calling registerDomain. Domains can be an alias for a java package
path.

Example:

 .. code-block:: python

  domains.registerDomain('jname')
  from jname.framework import FrameObject
  domains.registerDomain('jlang', alias='java.lang')
  from jlang import String


Limitations
~~~~~~~~~~~
* Wildcard imports are unreliable and should be avoided. Limitations
  in the Java specification are such that there is no way to get
  class information at runtime. Python does not have a good hook
  to prevent the use of wildcard loading.

* Non-static members can be imported but can not be called without an
  instance. Jpype does not provide an easy way to determine which
  functions objects can be called without an object.

