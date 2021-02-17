API Reference
=============

JVM Functions
~~~~~~~~~~~~~

These functions control and start the JVM.

.. autofunction:: jpype.startJVM
.. autofunction:: jpype.shutdownJVM
.. autofunction:: jpype.getDefaultJVMPath
.. autofunction:: jpype.getClassPath
.. autodecorator:: jpype.onJVMStart

Class importing
~~~~~~~~~~~~~~~

JPype supports several styles of importing.  The newer integrated style is 
provided by the imports_ module. The older ``JPackage`` method is available for 
accessing package trees with less error checking.  Direct loading of Java 
classes can be made with JClass_.

For convenience, the JPype module predefines the following ``JPackage`` 
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
``JShort``, ``JInt``, ``JLong``, ``JFloat`` and ``JDouble``.  There is one
class for working with Java objects, ``JObject``.  This serves to cast to a
specific object type.  

.. autoclass:: jpype.JObject

.. _threading:

Threading
~~~~~~~~~

.. autofunction:: jpype.synchronized
.. autoclass:: java.lang.Thread
    :members:


Decorators
~~~~~~~~~~~

JPype uses ordinary Python classes to implement functionality in Java. Adding 
these decorators to a Python class will mark them for use by JPype to interact 
with Java classes.

.. autodecorator:: jpype.JConversion
.. autodecorator:: jpype.JImplementationFor
.. autodecorator:: jpype.JImplements
.. autodecorator:: jpype.JOverride


Proxies
~~~~~~~

JPype can implement Java interfaces either by using decorators or by manually 
creating a JProxy.  Java only support proxying interfaces, thus we cannot 
extend an existing Java class.

.. autoclass:: jpype.JProxy


Customized Classes
~~~~~~~~~~~~~~~~~~

JPype provides standard customizers for Java interfaces so that Java objects 
have syntax matching the corresponding Python objects. The customizers are 
automatically bound to the class on creation without user intervention.  We are 
documentating the functions that each customizer adds here.  Information about
Java methods can be found in the Javadoc.

These internal classes can be used as example of how to implement your own 
customizers for Java classes.

.. autoclass:: java.util.Iterable
    :members:
    :special-members:
.. autoclass:: java.util.Collection
    :members:
    :special-members:
.. autoclass:: java.util.List
    :members:
    :special-members:
.. autoclass:: java.util.Map
    :members:
    :special-members:
.. autoclass:: java.util.Set
.. autoclass:: java.util.Iterator
.. autoclass:: java.util.Enumeration
.. autoclass:: java.lang.AutoCloseable

Modules
~~~~~~~

Optional JPype behavior is stored in modules. These optional modules can be 
imported to add additional functionality.

.. _imports:

.. automodule:: jpype.imports

.. automodule:: jpype.pickle
.. autoclass:: jpype.pickle.JPickler
.. autoclass:: jpype.pickle.JUnpickler

.. automodule:: jpype.beans

.. automodule:: jpype.types

