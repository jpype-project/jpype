.. _customization:

Customization
*************

JPype supports customization to enhance the integration between Java and Python.
This allows users to modify Java classes and type conversions to better suit
their needs, making Java APIs more Pythonic or letting them interoperate
directly with Python data structures.

This chapter covers the Python-calling-Java direction. If you are instead
embedding Python in a Java application, see :doc:`customizers_java` for the
matching Java-side extension points (SPI providers and the ``$``-mangled
direct-dispatch mechanism).

There are two primary types of customizations available:

1. **Class Customizers**: Add Python methods and properties to Java classes to
   make them behave like native Python classes.
2. **Type Conversion Customizers**: Define implicit conversions between Python
   types and Java types.

.. _customization_class_customizers:

Class Customizers
=================

Customizers are applied to JPype wrapper classes to enhance their Pythonic
interface. By adding Python methods and properties to Java classes, customizers
make Java objects behave like native Python objects. These customizations are
applied to wrappers, whether they encapsulate a proxy or a Java reference.

Java wrappers can be customized to better match the expected behavior in Python.
Customizers are defined using decorators. Applying the annotations
``@JImplementationFor`` and ``@JOverride`` to a regular Python class will
transfer methods and properties to a Java class.

``@JImplementationFor`` requires the class name as a string, a Java class
wrapper, or a Java class instance. Only a string can be used prior to starting
the JVM. ``@JOverride``, when applied to a Python method, will hide the Java
implementation, allowing the Python method to replace the Java implementation.
When a Java method is overridden, it is renamed with a preceding underscore to
appear as a private method. Optional arguments to ``@JOverride`` can be used to
control the renaming and force the method override to apply to all classes that
derive from a base class ("sticky").

Generally speaking, a customizer should be defined before the first instance of
a given class is created so that the class wrapper and all instances will have
the customization.

.. _customization_example_customizing_javautilmap:

Example: Customizing ``java.util.Map``
--------------------------------------

The following example demonstrates how to customize the ``java.util.Map`` class
to behave like a Python dictionary:

.. code-block:: python

   @_jcustomizer.JImplementationFor('java.util.Map')
   class _JMap:
       def __jclass_init__(self):
           Mapping.register(self)

       def __len__(self):
           return self.size()

       def __iter__(self):
           return self.keySet().iterator()

       def __delitem__(self, i):
           return self.remove(i)

The name of the class does not matter for the purposes of the customizer,
though it should be a private class so that it does not get used accidentally.
The customizer code will steal from the prototype class rather than acting as a
base class, ensuring that the methods will appear on the most derived Python
class and are not hidden by the Java implementations.

The customizer copies methods, callable objects, ``__new__``, class member
strings, and properties.

.. _customization_type_conversion_customizers:

Type Conversion Customizers
============================

JPype allows users to define custom conversion methods that are called whenever
a specified Python type is passed to a particular Java type. To specify a
conversion method, add ``@JConversion`` to an ordinary Python function with the
name of the Java class to be converted to and one keyword of ``exact`` or
``instanceof``. The keyword controls how strictly the conversion will be
applied:

- ``exact``: Restricted to Python objects whose type exactly matches the
  specified type.
- ``instanceof``: Accepts anything that matches ``isinstance`` to the specified
  type or protocol.

In some cases, the existing protocol definition will be overly broad. Adding
the keyword argument ``excludes`` with a type or tuple of types can be used to
prevent the conversion from being applied. Exclusions always apply first.

User-supplied conversions are tested after all internal conversions have been
exhausted and are always considered to be an implicit conversion.

.. _customization_example_converting_python_sequences_to_java_collections:

Example: Converting Python Sequences to Java Collections
----------------------------------------------------------

The following example demonstrates how to convert Python sequences into Java
collections:

.. code-block:: python

   @JConversion("java.util.Collection", instanceof=Sequence,
                             excludes=str)
   def _JSequenceConvert(jcls, obj):
       return _jclass.JClass('java.util.Arrays').asList(obj)

JPype supplies customizers for certain Python classes by default. These include:

========================= ==============================
Python class              Implicit Java Class
========================= ==============================
pathlib.Path              java.io.File
pathlib.Path              java.nio.file.Path
datetime.datetime         java.time.Instant
collections.abc.Sequence  java.util.Collection
collections.abc.Mapping   java.util.Map
========================= ==============================

.. _customization_jpype_beans_module:

JPype Beans Module
====================

The ``jpype.beans`` module is an optional feature that converts Java
Bean-style getter/setter methods into Python properties: a class with
``getName()``/``setName(x)`` gains a Python ``name`` property backed by
those two methods. It's particularly useful for interactive programming or
when working with Java classes that heavily follow the Bean pattern.

It is not enabled by default, because it makes it ambiguous from the
outside whether a given attribute is a real Java field or a property
synthesized by JPype, and it applies globally and irreversibly to every
Java class already loaded and every class loaded afterward -- there is no
scoped or per-class opt-in. Enable it by importing it once:

.. code-block:: python

  import jpype
  import jpype.beans
  jpype.startJVM()

  Person = jpype.JClass("Person")   # has getName()/setName(String)
  person = Person()
  person.name = "Alice"  # calls setName("Alice")
  print(person.name)     # calls getName(), "Alice"

If a Java class already has a Python member with the same name as a
would-be property, the property is not added, avoiding a silent collision.
Given the global, irreversible nature of the module, prefer it for
interactive/exploratory use or small scripts, and reach for an explicit
:ref:`class customizer <customization_class_customizers>` instead in a
library that other code will import, so the behavior stays local and
predictable.

.. _customization_resolving_method_name_conflicts_with_customizers:

Resolving Method Name Conflicts with Customizers
==================================================

When a Java class has a public field and a public method sharing the same
name, JPype exposes the method and the field becomes inaccessible by that
name. A customizer can walk the class's declared fields and expose the
shadowed ones under a different name -- here, appending an underscore and
wrapping the field in a Python ``property``:

.. code-block:: python

    def asProperty(field):
        def get(E):
            return field.get(E)
        def set(E, V):
            field.set(E, V)
        return property(get, set)

    @jpype.JImplementationFor("java.lang.Object")  # use your own base class
    class MyCustomizer(object):

        # applied to every class deriving from the base class above
        def __jclass_init__(cls):
            for field in cls.class_.getDeclaredFields():
                name = str(field.getName())
                tp = type(cls.__dict__.get(name, None))

                if tp is type(None):
                    continue  # not a name JPype already bound

                if tp is jpype.JMethod:
                    # a method already owns this name; expose the field as `name_`
                    cls._customize("%s_" % name, asProperty(field))

Given a Java class ``A`` with both a field ``mean`` and a method ``mean``,
this customizer exposes the field as ``a.mean_`` (gettable and settable)
alongside the method still reachable as ``a.mean()``.

``__jclass_init__`` runs once per class the customizer applies to (here,
every class deriving from ``java.lang.Object`` -- in practice, narrow the
base class in ``@JImplementationFor`` to limit the scope), which is what
makes this pattern practical for name collisions that recur across a large
Java library rather than a one-off.
