API Reference
=============

JVM Functions
~~~~~~~~~~~~~

These functions control and start the JVM.

.. autofunction:: jpype.startJVM
.. autofunction:: jpype.shutdownJVM
.. autofunction:: jpype.getDefaultJVMPath
.. autofunction:: jpype.getClassPath

Class Factories
~~~~~~~~~~~~~~~

.. autoclass:: jpype.JClass
.. autoclass:: jpype.JArray
.. autoclass:: jpype.JException

Types
~~~~~

JPype has types for each of the Java primitives: ``JBoolean``, ``JByte``,
``JShort``, ``JInt``, ``JLong``, ``JFloat`` and ``JDouble``.  In addition
there is one class which can be used to cast objects.

.. autoclass:: jpype.JObject

Threading
~~~~~~~~~

.. autofunction:: jpype.synchronized
.. autofunction:: jpype.isThreadAttachedToJVM
.. autofunction:: jpype.attachThreadToJVM
.. autofunction:: jpype.detachThreadFromJVM


Decorators
~~~~~~~~~~~

JPype uses ordinary Python classes to implement functionality in Java. Adding these decorators to a Python
class will mark them for use by JPype to interact with Java classes.

.. autodecorator:: jpype.JImplementationFor
.. autodecorator:: jpype.JImplements
.. autodecorator:: jpype.JOverride


Proxies
~~~~~~~

JPype can implement Java interfaces either using decorators or by manually creating a JProxy.  Java only 
support proxying interfaces, thus we cannot extend an existing Java class.

.. autoclass:: jpype.JProxy


Customized Classes
~~~~~~~~~~~~~~~~~~

JPype provides standard customizers for Java interfaces so that Java objects have syntax matching the 
corresponding Python objects. The customizers are automatically bound the class on creation without user 
intervention.  We are documentating the functions that each customizer adds here.

These internal classes can be used as example of how to implement your own customizers for Java classes.

.. autoclass:: jpype._jcollection._JIterable
.. autoclass:: jpype._jcollection._JCollection
.. autoclass:: jpype._jcollection._JList
.. autoclass:: jpype._jcollection._JMap
.. autoclass:: jpype._jcollection._JIterator
.. autoclass:: jpype._jcollection._JEnumeration
.. autoclass:: jpype._jio._JCloseable


Modules
~~~~~~~

Optional JPype behavior is stored in modules. These optional modules can be imported to add additional 
functionality.

.. automodule:: jpype.imports
.. autofunction:: jpype.imports.registerDomain
.. autofunction:: jpype.imports.registerImportCustomizer
.. autoclass:: jpype.imports.JImportCustomizer

.. automodule:: jpype.types
