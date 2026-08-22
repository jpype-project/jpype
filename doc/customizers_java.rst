
Extending the Bridge (for Java)
===================================

This is the Java-side counterpart to :doc:`customizers_py`: the two ways to
extend how the reverse bridge treats a Python object. Most readers want the
``$``-mangled direct-dispatch mechanism below; the SPI mechanism that
powers every built-in front end (``python.collections``, ``python.io``,
...) already has its own full writeup at :doc:`spi` and is only summarized
here.

.. contents::
   :local:
   :depth: 1


SPI providers: registering a new typed front end
-----------------------------------------------------

A ``WrapperService`` SPI provider is how ``python.collections.PyDeque``,
``python.io.PyStringIO``, ``python.datetime.PyDate``, and every other
built-in typed front end is wired up: a Python class gets registered (eager
or lazy) as viewable through a given Java interface, with its methods
resolved through a ``.pyspi`` resource file discovered via
``java.util.ServiceLoader``. This is genuinely a separate topic from the
dispatch mechanism below, with its own worked example (``python.io``) and
its own gotchas (module-granularity lazy resolution, the eager/lazy
tradeoff) -- see :doc:`spi` for the complete picture; writing a new
provider means reading that page, not this one.


The ``$``-mangled direct-dispatch mechanism
------------------------------------------------

Every proxy method call crosses from Java to Python by name. For an
ordinary user interface (``Runnable``, ``Comparator``, any interface with
no ``python.lang.PyObject`` in its hierarchy) that name crosses unchanged
and is looked up directly on the target -- this is unmangled, exact-name
dispatch, and every existing JPype user already relies on it.

Any interface that extends ``python.lang.PyObject`` -- directly or
transitively, which is every interface in ``python.lang``,
``python.collections``, ``python.io``, and friends -- is different: those
proxies duck-type arbitrary third-party Python objects (a user's own
``io.RawIOBase`` subclass, say), and a plain method name like ``read`` or
``write`` is both a plausible *protocol* method JPype wants to route
through its own dispatch map **and** a plausible real attribute name on the
wrapped object. To keep those two namespaces from colliding, every method
name on a ``PyObject``-rooted interface is mangled before dispatch:

.. code-block:: java

    private static String mangle(String name)
    {
      if (name.startsWith("$"))
        return name.substring(1);   // user wrapper method -> real direct dispatch
      return "." + name;            // protocol method -> map-only, collision-free
    }

In practice this means: **name a method ``$foo`` to reach the wrapped
Python object's real ``foo`` attribute directly**, bypassing JPype's own
dispatch map entirely. Any other name is assumed to be one of JPype's own
mapped protocol methods (``read``, ``size``, ``get``, ...) and is looked up
through the ``.``-mangled map instead -- never confused with a real
attribute of the same plain name. ``equals``/``hashCode``/``toString`` are
carved out of mangling entirely, since silently changing their dispatch
would break Java identity semantics.

Use ``$foo`` when you're writing your own interface against an object
that isn't (fully) covered by a built-in front end -- the same escape
hatch ``PyBuiltIn#eval`` provides on the value-conversion side, but for
method dispatch.


Worked example
------------------

The example below uses a small interface (``PyAliceBobCharlieDerik``) with
every method ``$``-prefixed, against a plain Python class with no
``@JImplements``/``.pyspi`` wiring at all:

.. code-block:: java

    // Python side (via context.exec):
    //   class _DFAliceBobCharlieDerik:
    //       def alice(self, arg):
    //           return ('alice', arg)
    //       def bob(self, a, b, key=None):
    //           return ('bob', a, b, key)
    //       wilma = 99
    //       # no 'ghost' attribute at all

    PyAliceBobCharlieDerik obj = ...;   // wrapped via jpype.JProxy(...)+@-cast,
                                          // or via the same _jpype._concrete
                                          // registration every real SPI
                                          // provider uses -- both routes work

The ``@``-cast is not optional decoration -- it's required whenever the
value comes back through ``Script.eval()``'s generic ``PyObject`` return
type. Conversion tries a fixed-order list of candidate paths, and the
plain structural/duck-typing path (``pythonConversion``) is tried *before*
the proxy-aware path (``proxyConversion``) ever runs; without an explicit
``targetClass @ pyProxy`` cast on the Python side, the structural path can
win first and the result never becomes the target interface at all. This
applies to any construction route that goes through a generically-typed
``eval()``/``exec()`` result, not just this worked example.

    PyTuple result = obj.$alice(42L);
    result.toString();                   // "('alice', 42)"

    result = obj.$bob(1L, 2L, PyKwArgs.of().kw("key", "K"));
    result.toString();                   // "('bob', 1, 2, 'K')"


Failure modes are distinct and worth knowing before you rely on this:

- ``obj.$ghost()`` against an attribute that doesn't exist at all: a clean
  ``NoSuchMethodError``, the same miss you'd get from a totally
  unimplemented interface method.
- ``obj.$wilma()`` against a real but non-callable attribute (``wilma =
  99``): a ``python.exceptions.PyTypeError`` carrying Python's own
  ``'int' object is not callable`` message -- not a crash.
- A real, callable attribute whose body raises: the actual Python
  exception type crosses the bridge (e.g. ``PyValueError``), not a
  generic wrapped exception.
- A real, callable attribute that returns a value incompatible with the
  interface method's declared return type: a ``PyTypeError`` with
  ``"Return value is not compatible with required type."`` -- a type
  mismatch here is loud, never silently coerced.


Where to next
---------------

- :doc:`spi` -- the full SPI provider mechanism (registration, ``.pyspi``
  resources, eager vs. lazy resolution) that this page only summarizes.
- :doc:`types_java` -- the ``python.lang`` type hierarchy that most
  ``PyObject``-rooted interfaces build on.
