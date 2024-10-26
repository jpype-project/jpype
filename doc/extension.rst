This information is a draft for the user guide.

Java Extensions
===============

JPype can extend Java classes from within Python.  Java Extensions are defining
using Python class definitions but they have a number of restrictions.  Here is
a summary of the key differences.

- The JVM must be running to define a Java extension type.

- Java extensions use decorators to mark up Python elements for use by Java.
  Annotations include ``@JPublic``, ``@JProtected``, ``@JPrivate``,
  ``@JOverride`` and ``@JThrows``.  The concepts of ``final`` and ``static``
  are not currently supported.

- ``@JPublic``, ``@JProtected``, or ``@JPrivate`` are considered accessors
  decorators.  They are used without the ``@`` when declaring fields.

- Java extensions must have all Java base types.

- Java base types must not be final or sealed.

- Java bases can have at most one concrete or abstract base type, but they can
  have multiple interfaces.

- If no concrete type is included then JObject will be added as the concrete
  base.

- All Java extensions are private classes without a package.  Thus, they cannot
  be annotated with ``@JPublic``, ``@JProtected``, or ``@JPrivate``.

- Java extensions will have a backing Java classes and therefore all exposed Java
  methods and fields must be strongly typed.

- All methods and fields must be annotated with an accessor decorator.

- Methods can be annotated with ``@JOverride``.  Methods with ``@JOverride``
  must have a signature that matches a method in the parent.

- Java methods must take ``self``, ``cls`` or ``this`` as the first argument.
  This is a Java handle with privilaged access to private methods and fields.
  The ``cls`` argument for static methods will be the class which defined the
  static method. It will not work as a Python ``classmethod`` normally would.

- Extension classes have the same access restrictions to package, protected
  and private visibile methods and fields as they would have if the class was
  written in Java.

- Java methods must have a return annotation if they have a return type.
  The return type can be ommitted or None if the method returns ``void``.
  The return type must be a Java class with the exception of None.

- Java methods must have all parameters specified with the exception of the
  ``self``, ``cls`` or ``this``.

- Java methods may not accept keyword arguments.

- Java methods may not have default values.

- The Python decorator ``@classmethod`` may be used instead of ``@JStatic``. However,
  as previously mentioned, it will not behave in the same fashion as a Python classmethod.

- The Python decorator ``@staticmethod`` are not currently supported.

- Java methods can be overloaded so long as the signatures are not conflicting.

- Variadic arguements are not currently supported.

- Java classes are closed so all fields must be defined in advance before being
  used.  Use ``JPublic``, ``JProtected``, ``JPrivate`` and ``JStatic`` to declare
  those slots in advance.

- Java fields are specified using ``JPublic``, ``JProtected``, ``JPrivate``, ``JFinal``
  and ``JStatic`` as type annotations using ``typing.Annotated``.

- Field names must not be Java keywords.

- The constructor is specified with ``__init__``.  Overloading of constructors
  is allowed.

- The first call to the Java class must be to the base class initializer.
  Accessing any other field or method will produce a ``ValueError``.

- The singleton pattern is not supported. Supporting this would require
  instantiating the Python class before it is defined.

- Nested classes, enums, records and throwables are not supported.

- Extension classes may be subclassed. However, at least one Java visible constructor
  must always be defined.

- Extension classes and Java methods cannot be abstract.

- Extension classes may have Python methods and members. A non Java visible ``__init__``
  may be defined. Note however, that this ``__init__`` will always be called **after**
  the Java constructors and with no arguments. Attempting to use ``super().__init__()``
  will have no effect because the Python implementations are called from within the
  Java constructor in the JVM.


Mechanism
---------

When a Java class is used as a base type, control is transfered a class builder
by the meta class JClassMeta.  The meta class probes all elements in the Python
prototype for decorated elements.  Those decorated elements are turned into a
class description which is passed to Java.  Java ASM is then used to construct
the requested classes which is loaded dynamically.  The resulting class is then
passed back to Python to finish creating a specialized Python class wrapper.

The majority of the errors are should be caught when building the class
description.  Errors detected at this stage will produce the using
``TypeError`` exceptions.  However, in some cases errors in the class
description may result in errors in the class generation or loading phase.
The exceptions from these stages currently produce Java exceptions.

