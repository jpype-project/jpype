API Reference
=============

JVM Functions
~~~~~~~~~~~~~

These functions control and start the JVM.

.. autofunction:: jpype.startJVM
.. autofunction:: jpype.shutdownJVM
.. autofunction:: jpype.getDefaultJVMPath
.. autofunction:: jpype.getClassPath

Class importing
~~~~~~~~~~~~~~~

JPype supports several styles of importing.  The newer integrated style is 
provided by the imports_ module. The older ``JPackage`` method is available for 
accessing package trees with less error checking.  Direct loading of Java 
classes can be made with JClass_.

For convenience, the JPpype module predefines the following ``JPackage`` 
instances for ``java`` and ``javax``.

.. autoclass:: jpype.JPackage

Class Factories
~~~~~~~~~~~~~~~

.. _JClass:

.. autoclass:: jpype.JClass
.. autoclass:: jpype.JArray
.. autoclass:: jpype.JException

Java Types
~~~~~~~~~~

JPype has types for each of the Java primitives: ``JBoolean``, ``JByte``,
``JShort``, ``JInt``, ``JLong``, ``JFloat`` and ``JDouble``.  In addition
there is one class for working with Java objects, ``JObject``.  These serve
to be able to cast to a specified type and specify types with the ``JArray``
factory. There is a ``JString`` type provided for convenience when creating
or casting to strings.

.. autoclass:: jpype.JObject
.. autoclass:: jpype.JString

.. _synchronized:

Threading
~~~~~~~~~

.. autofunction:: jpype.synchronized
.. autofunction:: jpype.isThreadAttachedToJVM
.. autofunction:: jpype.attachThreadToJVM
.. autofunction:: jpype.detachThreadFromJVM


Decorators
~~~~~~~~~~~

JPype uses ordinary Python classes to implement functionality in Java. Adding 
these decorators to a Python class will mark them for use by JPype to interact 
with Java classes.

.. autodecorator:: jpype.JImplementationFor
.. autodecorator:: jpype.JImplements
.. autodecorator:: jpype.JOverride


Proxies
~~~~~~~

JPype can implement Java interfaces either using decorators or by manually 
creating a JProxy.  Java only support proxying interfaces, thus we cannot 
extend an existing Java class.

.. autoclass:: jpype.JProxy


Customized Classes
~~~~~~~~~~~~~~~~~~

JPype provides standard customizers for Java interfaces so that Java objects 
have syntax matching the corresponding Python objects. The customizers are 
automatically bound the class on creation without user intervention.  We are 
documentating the functions that each customizer adds here.

These internal classes can be used as example of how to implement your own 
customizers for Java classes.

.. autoclass:: jpype._jcollection._JIterable
.. autoclass:: jpype._jcollection._JCollection
.. autoclass:: jpype._jcollection._JList
.. autoclass:: jpype._jcollection._JMap
.. autoclass:: jpype._jcollection._JIterator
.. autoclass:: jpype._jcollection._JEnumeration
.. autoclass:: jpype._jio._JCloseable


Modules
~~~~~~~

Optional JPype behavior is stored in modules. These optional modules can be 
imported to add additional functionality.

.. _imports:

.. automodule:: jpype.imports
.. autofunction:: jpype.imports.registerDomain
.. autofunction:: jpype.imports.registerImportCustomizer
.. autoclass:: jpype.imports.JImportCustomizer

.. automodule:: jpype.pickle
.. autoclass:: jpype.pickle.JPickler
.. autoclass:: jpype.pickle.JUnpickler

.. automodule:: jpype.beans

.. automodule:: jpype.types

