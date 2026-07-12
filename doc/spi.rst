Service Provider Interface (SPI)
=================================

Overview
--------

The reverse bridge (``python.lang``, ``python.io``, and friends) lets Java
code call into Python objects through ordinary Java interfaces —
``PyObject``, ``PyBytes``, ``PyDict``, and so on. Those wrappers are all
built the same way: a Java interface plus a small ``dict[str, Callable]``
telling the bridge which Python callable backs each interface method.

The SPI is the extension point that lets code outside JPype's own
``python.*`` packages plug new wrappers into that same machinery, without
editing any of JPype's core classes. A provider — think a Java library that
wants to expose ``numpy.ndarray``, or any other Python type, as a Java
interface — implements ``org.jpype.WrapperService``, registers it via the
standard Java service-provider mechanism, and drops one small resource file
per Python class it wants to cover.

``python.io`` (``native/jpype_module/src/main/java/python/io/``) is not a
special case built into the bridge — it is an ordinary SPI provider,
``python.io.PyIOWrapperService``, and everything in this document can be
checked against its resource directory,
``native/jpype_module/src/main/resources/python/io/spi/``.

The interface
--------------

.. code-block:: java

    public interface WrapperService {
        String[] getModuleNames();
        String getVersion();
        Iterable<String> getResources();
    }

``getModuleNames()``
  The fully qualified Python module names this provider covers, e.g.
  ``{"io", "_io"}``. A single provider may need more than one name when a
  package has both a public facade module and an internal C-accelerator
  module backing it — see the ``io``/``_io`` split below.

``getVersion()``
  A version string for this provider's bindings — informational only today
  (e.g. the version of the Python package being targeted).

``getResources()``
  Classpath paths to every ``.pyspi`` resource this provider owns, one per
  Python class it registers plus one per mini-backend it needs (below). Most
  providers implement this as a one-line directory scan:

  .. code-block:: java

      @Override
      public Iterable<String> getResources() {
          return SpiLoader.listPyspiResources(MyWrapperService.class, "/my/package/spi");
      }

  ``SpiLoader.listPyspiResources`` works whether the provider's classes are
  on the classpath as an exploded directory (``mvn test``) or packaged
  inside a jar (the ``ant``-built ``org.jpype.jar``) — both are exercised by
  this project's own build.

Registering the provider
-------------------------

Discovery goes through ``java.util.ServiceLoader``, so a provider needs the
usual service-provider file:

.. code-block:: text

    META-INF/services/org.jpype.WrapperService

containing the provider's fully qualified class name, e.g.:

.. code-block:: text

    python.io.PyIOWrapperService

If the provider lives in a named JPMS module, ``ServiceLoader`` ignores
``META-INF/services`` for providers declared *inside that same module* — the
module descriptor's ``provides ... with`` clause is consulted instead:

.. code-block:: java

    module org.jpype {
        uses org.jpype.WrapperService;
        provides org.jpype.WrapperService with python.io.PyIOWrapperService;
    }

At interpreter startup, ``org.jpype.SpiLoader.load(installer)`` loads every
registered ``WrapperService``, reads each of its declared resources, and
replays them into the ``Installer`` — eagerly or lazily, per each resource's
own header (below). This happens once, from ``MainInterpreter.setInstaller``.

The ``.pyspi`` resource format
-------------------------------

Each ``.pyspi`` file is a small ``key: value`` header, a line containing
only ``---``, then a blob of Python source. When that source is ``exec``'d,
it must bind a top-level name ``METHODS`` to a ``dict[str, Callable]``
mapping Java-interface method names to the Python callables that implement
them.

There are two kinds of resource, selected by the header's ``kind:`` field.

Class registration
~~~~~~~~~~~~~~~~~~~

Declares that instances of a given Python class should be viewable as a
given Java interface:

.. code-block:: text

    kind: class
    module: _io
    class: BytesIO
    interface: python.io.PyBytesIO
    ---
    METHODS = {
        "getvalue": lambda x: x.getvalue(),
    }

``module``/``class`` identify the Python type (``_io.BytesIO``);
``interface`` is the fully qualified Java interface it should satisfy.
Every entry in ``METHODS`` takes the wrapped instance as its first
argument, exactly like an unbound method.

By default this registration is **eager** — replayed immediately at
interpreter startup, regardless of whether that Python class has been
imported yet. Add ``lazy: true`` to the header to defer it until an
instance of the class is actually seen crossing into Java:

.. code-block:: text

    kind: class
    module: _io
    class: StringIO
    interface: python.io.PyStringIO
    lazy: true
    ---
    METHODS = {
        "getvalue": lambda x: x.getvalue(),
    }

Lazy registration exists for two reasons. First, it avoids paying the
``exec`` cost for classes a given program never touches. Second, and more
importantly, some target classes may not exist as a real ``PyTypeObject*``
until their package is imported — a third-party type like a ``numpy``
subclass cannot be registered eagerly at all, since the class object
doesn't exist yet at interpreter boot.

Lazy resolution is batched **by module, not by class**: the first time any
instance of an unresolved type crosses into Java, every ``lazy: true``
resource for every ``__module__`` value across that type's ``__mro__`` gets
registered in one pass, not just the one class that triggered the miss.
This matters because of a real gotcha in the standard library's own
``io`` module: the public abstract base classes (``io.IOBase``,
``io.BufferedIOBase``, ...) report ``__module__ == "io"``, while every
concrete class (``_io.BytesIO``, ``_io.StringIO``, ...) reports
``__module__ == "_io"``, the C accelerator module actually backing the
``io`` facade. A provider that only declared ``getModuleNames()`` as
``{"io"}`` would never see its concrete-class resources triggered — hence
``PyIOWrapperService`` declaring both ``"io"`` and ``"_io"``. Any real
third-party package with a similar Python-facade/C-accelerator split (numpy
included) needs the same treatment.

Once resolved — eagerly at boot or lazily on first miss — there is no
further per-call cost: the class-to-interface mapping is a plain cache
lookup from then on, whether the code path is `eager` or `lazy`.

Mini-backend registration
~~~~~~~~~~~~~~~~~~~~~~~~~~

A provider frequently needs its own module-level entry points — mostly
factory functions, since a wrapped type's constructor has to live
somewhere. These cannot be added to JPype's own shared ``Backend``
interface — that is one fixed, compiled interface bound once to one
hardcoded dict, and no per-provider extension point exists on it (nor
should core ``python.lang`` gain per-provider knowledge). Instead, a
provider declares its own small backend-shaped interface and registers a
resource of ``kind: backend``:

.. code-block:: text

    kind: backend
    interface: python.io.IO
    ---
    import io

    def _new_bytes_io(initial=None):
        return io.BytesIO(bytes(initial)) if initial is not None else io.BytesIO()

    METHODS = {
        "bytesIO": _new_bytes_io,
    }

This is always eager (``lazy: true`` on a ``kind: backend`` resource is
rejected at parse time) — a mini-backend's own interface is a compiled Java
type known at boot, independent of whether any Python instance it can
construct has been seen yet.

On the Java side, the interface exposes a static accessor that looks itself
up wherever the provider's other objects are reached from — ``python.io.IO``
does this via ``IO.using(PyBuiltIn context)``, mirroring how any live
``PyObject`` reaches its backend through ``builtin()``:

.. code-block:: java

    public interface IO {
        static IO using(PyBuiltIn context) {
            return context.getBackend(IO.class);
        }
        PyBytesIO bytesIO();
        PyBytesIO bytesIO(PyBuffer initial);
    }

Worked example: ``python.io``
-------------------------------

``python.io.PyIOWrapperService`` covers the full shape described above in
one small provider:

- ``getModuleNames()`` returns ``{"io", "_io"}``, for the reason given above.
- ``getResources()`` is the one-line directory scan over
  ``python/io/spi/``, so adding a new class means dropping a new
  ``.pyspi`` file there — nothing in the service class itself changes.
- The abstract protocol interfaces (``PyIOBase``, ``PyBufferedIOBase``,
  ``PyTextIOBase``, ``PyRawIOBase``) are registered eagerly, since they
  exist as compiled Java types regardless of which concrete Python classes
  get touched.
- The concrete classes (``BytesIO``, ``FileIO``, ``BufferedReader``,
  ``TextIOWrapper``, ...) are registered lazily — the standard pattern for
  any real class-membership mapping, ``io`` included, since eager
  registration would only work by coincidence for a module this early in
  the standard library's own import order.
- ``python.io.IO`` is the mini-backend, providing ``bytesIO()``,
  ``stringIO()``, and ``fileIO(path, mode)`` as the construction entry
  points that cannot live on the shared ``Backend``.

A third-party provider follows the same three steps: write the Java
interfaces, write one ``.pyspi`` file per Python class (eager for anything
guaranteed to exist at boot, ``lazy: true`` for everything else), and write
one ``kind: backend`` file if the package needs its own factory functions.
