.. _Proxies:

Calling Python Code from Java
*****************************
Proxies in JPype enable Python objects to implement Java interfaces directly,
so Java code can call into Python without knowing it. These proxies are
specifically designed to implement Java interfaces, acting as wrapper classes
that disguise the Python nature of the object in a Java type-safe manner.

Don't confuse this with :doc:`types_java`: this chapter is about a *Python*
object (defined with ``@JImplements``/``JProxy``) playing a *Java* role --
Python code written to satisfy a Java interface, called from Java code that
doesn't know it's talking to Python underneath. ``python.lang`` (covered in
:doc:`types_java`) is the opposite relationship: Java code directly
manipulating a live Python object through a typed Java-side handle
(``PyObject`` and its subtypes), with no pretense of being a Java type at
all. Both directions involve Java calling into Python, but the object
being passed around plays a different role in each.

In Java, proxies are foreign elements that pretend to implement a Java
interface. JPype uses this proxy API to allow Python code to implement any
Java interface. While proxies allow Python objects to fulfill the contract of a
Java interface, they are not equivalent to subclassing Java classes in Python.

Fortunately, many Java APIs are designed to minimize the need for subclassing.
For example, frameworks like AWT and SWING allow developers to create complete
user interfaces without requiring a single subclass. Subclassing is typically
reserved for more advanced features or specialized use cases.

For those cases where sub-classing is absolutely necessary (i.e. using Java's
SAXP classes), it is necessary to create an interface and a simple
subclass in Java that delegates the calls to that interface.  The interface
can then be implemented in Python using a proxy.

There are three APIs for supporting of Java proxies. The direct method is to
pass a Python function, method, bound method, or lambda to any Java method
that accepts a FunctionInterface or other SAM.  If more complex behaviors
need to be exchanged Python can implement a Java interface. Implementation of an
interface uses decorators which features strong error checking and easy
notation.  The older low-level interface allows any Python object or dictionary
to act as a proxy even if it does not provide the required methods for the
interface.

.. _calling_python_code_from_java_passing_python_callables_to_java_functional_interfaces:

Passing Python Callables to Java Functional Interfaces
=======================================================

JPype supports passing Python functions, methods, and bound methods directly to
Java methods or fields that expect a ``FunctionalInterface``. This lets Python
code be used directly as Java's functional programming constructs, such as
lambdas and method references, without requiring a proxy or explicit
implementation of the interface. This works with any Java method or field
that expects a functional interface, including Java Streams
(``java.util.stream``), Java Executors (``java.util.concurrent``), and
custom functional interfaces defined in Java code.

Suppose you have a Java method that expects a ``java.util.function.Function``:

.. code-block:: java

    import java.util.function.Function;

    public class Example {
        public static String applyFunction(Function<String, String> func, String input) {
            return func.apply(input);
        }
    }

A plain Python function, a lambda, or a bound method can all be passed
directly where that ``Function`` is expected:

.. code-block:: python

    import jpype
    import jpype.imports
    jpype.startJVM()

    from java.util.function import Function
    from Example import Example

    def to_uppercase(s):
        return s.upper()

    print(Example.applyFunction(to_uppercase, "hello"))          # HELLO
    print(Example.applyFunction(lambda s: s[::-1], "hello"))     # olleh

    class StringManipulator:
        def reverse(self, s):
            return s[::-1]

    manipulator = StringManipulator()
    print(Example.applyFunction(manipulator.reverse, "hello"))   # olleh

This is convenient, but it is not as fast as a real Java proxy for
high-frequency calls -- prefer a proxy (below) on a hot path. If an exception
occurs within the Python callable, it is wrapped in a ``RuntimeException`` on
the Java side. The return value must be implicitly convertible to the Java
type the interface declares.

.. _@JImplements:

Implements
==========

The newer style of proxy works by decorating any ordinary Python class to
designate it as a proxy.  This is most effective when you
control the Python class definition.  If you don't control the class definition
you either need to encapsulate the Python object in another object or
use the older style.

Implementing a proxy is simple.  First construct an ordinary Python class with
method names that match the Java interface to be implemented.  Then add
the ``@JImplements`` decorator to the class definition.  The first argument
to the decorator is the interface to implement.  Then mark each
method corresponding to a Java method in the interface with ``@JOverride``.
When the proxy class is declared, the methods will be checked against the Java
interface.  Any missing method will result in JPype raising an exception.

High-level proxies have one other important behavior.  When a proxy created
using the high-level API returns from Java it unpacks back to the original
Python object complete with all of its attributes.  This occurs whether the
proxy is the ``self`` argument for a method or
proxy is returned from a Java container such as a list.  This is accomplished
because the actually proxy is a temporary Java object with no substance,
thus rather than returning a useless object, JPype unpacks the proxy
to its original Python object.

.. _calling_python_code_from_java_proxy_method_overloading:

Proxy Method Overloading
------------------------

Overloaded methods will issue to a single method with the matching name.  If
they take different numbers of arguments then it is best to implement a method
dispatch:

.. code-block:: python

    @JImplements(JavaInterface)
    class MyImpl:
        @JOverride
        def callOverloaded(self, *args):
            # always use the wild card args when implementing a dispatch
            if len(args) == 2:
                return self.callMethod1(*args)
            if len(args) == 1 and isinstance(args[0], JString):
                return self.callMethod2(*args)
            raise RuntimeError("Incorrect arguments")

        def callMethod1(self, a1, a2):
            # ...
            pass

        def callMethod2(self, jstr):
            # ...
            pass

.. _calling_python_code_from_java_multiple_interfaces:

Multiple interfaces
-------------------

Proxies can implement multiple interfaces as long as none of those interfaces
have conflicting methods.  To implement more than one interface, use a
list as the argument to the JImplements decorator.  Each interface must be
implemented completely.

.. _calling_python_code_from_java_deferred_realization:

Deferred realization
--------------------

Sometimes it is useful to implement proxies before the JVM is started.  To
achieve this, specify the interface using a string and add the keyword argument
``deferred`` with a value of ``True`` to the decorator.

.. code-block:: python

    @JImplements("org.foo.JavaInterface", deferred=True)
    class MyImpl:
        # ...
        pass


Deferred proxies are not checked at declaration time, but instead at the time
for the first usage.  Because of this, when uses an deferred proxy the code
must be able to handle initialization errors wherever the proxy is created.

Other than the raising of exceptions on creation, there is no penalty to
deferring a proxy class. The implementation is checked once on the first
usage and cached for the remaining life of the class.

.. _calling_python_code_from_java_proxy_factory:

Proxy Factory
=============

When a foreign object from another module for which you do not control the class
implementation needs to be passed into Java, the low level API is appropriate.
In this API you manually create a JProxy object.  The proxy object must either
be a Python object instance or a Python dictionary.  Low-level proxies use the
JProxy API.

.. _calling_python_code_from_java_jproxy:

JProxy
------

The ``JProxy`` allows Python code to "implement" any number of Java interfaces,
so as to receive callbacks through them.  The JProxy factory has the signature::

   JProxy(int, [dict=obj | inst=obj] [, deferred=False])

The first argument is the interface to be implemented.  This may be either
a string with the name of the interface, a Java class, or a Java class instance.
If multiple interfaces are to be implemented the first argument is
replaced by a Python sequence.  The next argument is a keyword argument
specifying the object to receive methods.  This can either be a dictionary
``dict`` which names the methods as keys or an object instance ``inst`` which
will receive method calls.  If more than one option is selected, a ``TypeError``
is raised.  When Java calls the proxy the method is looked up in either
the dictionary or the instance and the resulting method is called.  Any
exceptions generated in the proxy will be wrapped as a ``RuntimeException``
in Java.  If that exception reaches back to Python it is unpacked to return
the original Python exception.

Assume a Java interface like:

.. code-block:: java

  public interface ITestInterface2
  {
          int testMethod();
          String testMethod2();
  }

You can create a proxy *implementing* this interface in two ways.
First, with an object:

.. code-block:: python

  class C:
      def testMethod(self):
          return 42

      def testMethod2(self):
          return "Bar"

  c = C()  # create an instance
  proxy = JProxy("ITestInterface2", inst=c)  # Convert it into a proxy

or you can use a dictionary.

.. code-block:: python

    def _testMethod():
        return 32

    def _testMethod2():
        return "Fooo!"

    d = { 'testMethod': _testMethod, 'testMethod2': _testMethod2, }
    proxy = JProxy("ITestInterface2", dict=d)

.. _calling_python_code_from_java_wrapping_a_python_instance_with_new_behaviors_for_java:

Wrapping a Python instance with new behaviors for Java
======================================================

``JProxy`` also accepts ``dict`` and ``inst`` together. This lets a Python
object satisfy a Java interface using a mix of its own methods and methods
supplied ad hoc through a dictionary -- useful when the object's existing
method names don't match what the interface expects, or when only some of
the interface's methods are already implemented on the object. If a method
name is present in both, the dictionary entry wins and receives ``inst`` as
its first (``self``) argument; if a name is present in neither, JPype falls
back to the interface's own default method if it has one, and raises
``NotImplementedError`` otherwise.

.. code-block:: java

    public interface MyInterface {
        String method1();
        default String method2() {
            return "hello";
        }
    }

.. code-block:: python

    from jpype import JProxy

    class MyClass:
        def __init__(self, name):
            self.name = name

    my_dict = {
        "method1": lambda self: f"Hello, {self.name} from method1"
    }
    my_instance = MyClass("Alice")

    proxy = JProxy("MyInterface", dict=my_dict, inst=my_instance)

    print(proxy.method1())  # Hello, Alice from method1
    print(proxy.method2())  # hello -- falls back to the interface's default method

This also means the dictionary can be used to override a method the
instance already provides, without modifying the instance's class.

.. _calling_python_code_from_java_reference_loops:

Reference Loops
================

It is strongly recommended that object used in proxies must never hold a
reference to a Java container.  If a Java container is asked to hold a Python
object and the Python object holds a reference to the container, then a
reference loop is formed.  Both the Python and Java garbage collectors are
aware of reference loops within themselves and have appropriate handling for
them.  But the memory space of the other machine is opaque and neither Java nor
Python is aware of the reference loop.  Therefore, unless you manually break
the loop by either clearing the container, or removing the Java reference from
Python these objects can never be collected.  Once you lose the handle they
will both become immortal.

Ordinarily the proxy by itself would form a reference loop.  The Python
object points to a Java invocation handler and the invocation handler points
back to Python object to prevent the Python object from going away as long
as Java is holding onto the proxy.  This is resolved internally by making
the Python weak reference the Java portion.  If Java ever garbage collects
the Java half, it is recreated again when the proxy is next used.

This does have some consequences for the use of proxies.  Proxies must never
be used as synchronization objects.  Whenever
they are garbage collected, they lose their identity.  In addition, their
hashCode and system id both are reissued whenever they are refreshed.
Therefore, using a proxy as a Java map key can be problematic.  So long as
it remains in the Java map, it will maintain the same identify.  But once
it is removed, it is free to switch identities every time it is garbage
collected.

.. _calling_python_code_from_java_proxying_python_objects:

Proxying Python objects
========================

Sometimes it is necessary to push a Python object into Java memory space as an
opaque object.  This can be achieved using be implementing a proxy for
an interface which has no methods.  For example, ``java.io.Serializable`` has
no arguments and little functionality beyond declaring that an object can be
serialized. As low-level proxies to not automatically convert back to Python
upon returning to Java, the special keyword argument ``convert`` should be set
to True.

For example, let's place a generic Python object such as NumPy array into Java.

.. code-block:: python

    import numpy as np
    u = np.array([[1,2],[3,4]])
    ls = java.util.ArrayList()
    ls.add(jpype.JProxy(java.io.Serializable, inst=u, convert=True))
    u2 = ls.get(0)
    print(u is u2)  # True!

We get the expected result of ``True``.  The Python has passed through Java
unharmed.
