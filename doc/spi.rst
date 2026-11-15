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

.. contents::
   :local:
   :depth: 1

The interface
--------------

.. code-block:: java

    public interface WrapperService {
        String[] getModuleNames();
        String getVersion();
        Iterable<String> getResources();
        default void initialize(Backend backend) {}
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
  on the classpath as an exploded directory or packaged inside a jar, and
  returns them in sorted filename order — both are exercised by this
  project's own build.

``initialize(Backend)``
  A default no-op hook. **Nothing calls it today** — ``SpiLoader`` never
  invokes it, and no other code path does either. Treat it as reserved, not
  as a live extension point; don't design a provider around it.

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

This distinction is not academic: ``org.jpype``'s own ``module-info.java``
declares all five of its built-in providers (``python.io``,
``python.collections``, ``python.datetime``, ``python.pathlib``,
``python.decimal``) via ``provides ... with``, and only ``python.io`` also
ships a ``META-INF/services`` file (unused at runtime, since the
module-path route already finds it). A **third-party** provider is not
part of the ``org.jpype`` module, so it cannot rely on that ``provides``
clause — it must ship its own ``META-INF/services/org.jpype.WrapperService``
so classpath/module-path discovery of a foreign module still finds it.

At interpreter startup, ``org.jpype.SpiLoader.load(installer)`` loads every
registered ``WrapperService`` (in whatever order ``ServiceLoader`` yields
them), reads each one's declared resources in the order ``getResources()``
returns them, and replays them into the ``Installer`` — eagerly or lazily,
per each resource's own header (below). This happens once, from
``MainInterpreter.setInstaller``, after the shared ``Backend`` has already
been bound (see `Backends`_ below — some SPI resources depend on that
ordering).

The ``.pyspi`` resource format
-------------------------------

Each ``.pyspi`` file is a small ``key: value`` header, a line containing
exactly ``---``, then a blob of Python source, exactly as read by
``SpiResource.parse``:

- The header/body separator is the literal substring ``"\n---\n"`` — not
  just "a line with three dashes somewhere." A malformed file raises
  ``"Missing '---' header/body separator"`` at load time, not later.
- Header lines are trimmed; blank lines and lines starting with ``#`` are
  skipped as comments.
- Recognized header keys: ``kind`` (``class`` or ``backend``, defaults to
  ``class`` if omitted), ``module``, ``class``, ``interface`` (always
  required), ``lazy`` (``true``/anything else = false). There is no
  ``version`` or ``package`` header field. Unrecognized keys are accepted
  silently and simply never read — a typo in a header key fails silently,
  not loudly, so double-check spelling rather than relying on a load-time
  error to catch it.
- The body, after the separator, is ``exec``'d as a full Python module
  body — ordinary imports and helper ``def``s are allowed before the
  required top-level ``METHODS = {...}`` binding. Several built-in
  providers do exactly this (``pathlib``'s ``_compare_to``/``_join``
  helpers, ``datetime``'s ``_utc_offset_seconds``, mini-backends importing
  their own modules).

**The ``METHODS`` key convention is not simply the Java method name.** It
depends on whether the target Java interface extends
``python.lang.PyObject``:

- **Class registrations** (``kind: class``) target interfaces that extend
  ``PyObject`` — every one of them does, since that's what makes them
  ``PyObject``-family wrappers. Those interfaces go through JPype's
  ``$``-mangled dispatch (:doc:`customizers_java`), so every key in
  ``METHODS`` must be **dot-prefixed** to match: ``interface.getvalue()``
  is reached through the key ``".getvalue"``, not ``"getvalue"``. Every
  real ``kind: class`` resource in this repo uses dotted keys — see
  ``python/io/spi/_io.BytesIO.pyspi`` below.
- **Mini-backend registrations** (``kind: backend``) target interfaces that
  do *not* extend ``PyObject`` (they're plain factory-style interfaces),
  so they are never mangled — keys are the **plain** method name
  (``"bytesIO"``, not ``".bytesIO"``).

Getting this wrong doesn't fail at load time — the resource parses and
registers fine, and the class-to-interface mapping succeeds. It fails
later, as a clean ``NoSuchMethodError``/miss the first time Java actually
calls the method, since the mangled wire name JPype looks up (``.foo``)
was never present as a key. If a registered class's methods appear to not
be found at call time, checking the dot prefix is the first thing to
check.

Class registration
~~~~~~~~~~~~~~~~~~~

Declares that instances of a given Python class should be viewable as a
given Java interface. Every entry in ``METHODS`` is called as an unbound
method — the wrapped instance is always the first positional argument:

.. code-block:: text

    kind: class
    module: _io
    class: BytesIO
    interface: python.io.PyBytesIO
    ---
    METHODS = {
        ".getvalue": lambda x: x.getvalue(),
    }

``module``/``class`` identify the Python type (``_io.BytesIO``);
``interface`` is the fully qualified Java interface it should satisfy.

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
        ".getvalue": lambda x: x.getvalue(),
    }

Lazy registration exists for two reasons. First, it avoids paying the
``exec`` cost for classes a given program never touches. Second, and more
importantly, some target classes may not exist as a real ``PyTypeObject*``
until their package is imported — a third-party type like a ``numpy``
subclass cannot be registered eagerly at all, since the class object
doesn't exist yet at interpreter boot.

Mechanically, an eager resource is registered by calling
``importlib.import_module(pyModule)``, ``getattr(mod, pyClass)``, then
recording the result in the same two module-level dicts JPype's own
hand-written core types use: ``_jpype._concrete[pyType] = interface`` and
``_jpype._methods[interface] = methods``. A lazy resource skips all of
that at load time and just stashes ``(interface, methodsSource)`` under
``_jpype._lazy_pending[module][class]``.

The lazy hook fires from the type-probe cache miss path
(``_LazyCache.__getitem__``, the dict backing ``_jpype._cache``): the first
time an *unrecognized* type is seen crossing into Java, it computes the set
of ``__module__`` values across **that type's entire MRO**, intersects it
against the pending-module keys, and — for every match — eagerly resolves
**every** class still pending for that module in one pass, not just the
one class that triggered the miss. Resolution is therefore batched by
module, not by class. This matters because of a real gotcha in the
standard library's own ``io`` module: the public abstract base classes
(``io.IOBase``, ``io.BufferedIOBase``, ...) report ``__module__ == "io"``,
while every concrete class (``_io.BytesIO``, ``_io.StringIO``, ...) reports
``__module__ == "_io"``, the C accelerator module actually backing the
``io`` facade. A provider that only declared ``getModuleNames()`` as
``{"io"}`` would never see its concrete-class resources triggered — hence
``PyIOWrapperService`` declaring both ``"io"`` and ``"_io"``. Any real
third-party package with a similar Python-facade/C-accelerator split
(numpy included) needs the same treatment.

Once resolved — eagerly at boot or lazily on first miss — there is no
further per-call cost: the class-to-interface mapping is a plain cache
lookup from then on, whether the resolution was eager or lazy.

Mini-backend registration
~~~~~~~~~~~~~~~~~~~~~~~~~~

A provider frequently needs its own module-level entry points — mostly
factory functions, since a wrapped type's constructor has to live
somewhere. These cannot be added to JPype's own shared ``Backend``
interface — that is one fixed, compiled interface bound once, JVM-wide,
and no per-provider extension point exists on it (nor should core
``python.lang`` gain per-provider knowledge). Instead, a provider declares
its own small backend-shaped interface and registers a resource of
``kind: backend``:

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

Backends
--------

Two, unrelated registries share the name "backend" and are easy to
confuse:

- **The shared ``Backend``** (``org.jpype.Backend``) is one large, fixed
  interface (``call``, ``getattr``, ``newDict``, ``type``, ...) bound
  exactly once, JVM-wide, as a static field on ``MainInterpreter`` before
  any SPI loading happens. Every ``PyObject`` reaches it through
  ``builtin()``. Providers never extend or touch it directly.
- **Mini-backends** are per-provider, per-interpreter: each ``kind:
  backend`` resource registers its interface into a
  ``ConcurrentHashMap<Class<?>, Object>`` on the interpreter's
  ``NativeContext``, looked up later via ``context.getBackend(MyIface
  .class)`` (throws ``IllegalStateException`` if that provider's resource
  was never discovered/registered). ``PyBuiltIn.getBackend`` is just a
  forwarding call to this map.

A mini-backend interface like ``IO`` does *not* extend ``PyObject`` — it's
a plain construction-and-lookup facade — so it is never ``$``-mangled, and
its ``METHODS`` keys stay unmangled (see the format note above).

Annotations that interact with SPI
------------------------------------

Three unrelated annotations live near this machinery; only one has
anything to do with SPI registration:

- ``@JImplements`` (Python-side, ``jpype._jproxy``) builds an ordinary
  proxy from a hand-written Python class with ``@JOverride``-annotated
  methods. It is a completely separate route to the same ``$``-mangled
  dispatch mechanism SPI-registered classes use — see
  :doc:`customizers_java` — and has no ``.pyspi``/``WrapperService``
  involvement at all.
- ``@Bypass`` (``org.jpype.annotation.Bypass``, Java-side) marks an
  interface method whose call should **never** cross into Python: the
  proxy invokes the interface's own default-method body directly via a
  ``MethodHandle`` and skips the downcall entirely. ``PyMapping``'s
  default ``get(Object)`` (plain ``x[key]``) is bypassed this way, while
  ``PyDict``'s override that needs real dict semantics is not. This is
  orthogonal to SPI registration — it fires (or doesn't) purely based on
  the interface's own method declaration, regardless of whether the
  concrete class behind it was registered eagerly, lazily, or not at all.
- ``@Builtin`` (``org.jpype.annotation.Builtin``) is an internal marker
  used to let an interface method return the ``PyBuiltIn`` backing a proxy
  (how ``builtin()`` itself is implemented). Not something a provider
  author needs to use.

Bypassing SPI entirely
------------------------

``.pyspi``/``WrapperService`` is the sanctioned route for *persistent*
registration — something that should apply to every instance of a Python
class, discovered automatically at startup. For one-off or exploratory
work against a Python object that isn't (fully) covered by a built-in
front end, two lighter routes exist, both documented in full in
:doc:`customizers_java`:

- **``$``-mangled direct dispatch**: name an interface method ``$foo`` to
  reach the wrapped object's real ``foo`` attribute directly, bypassing
  JPype's own dispatch map (and therefore any ``METHODS`` registration)
  entirely. Construct via ``jpype.JProxy(iface, dict=...)`` plus an
  ``@``-cast, or via manual registration (below) — both routes work.
- **Manual registration**: everything a ``.pyspi`` class registration does
  at load time reduces to two dict writes —
  ``_jpype._concrete[pyType] = javaInterface`` and
  ``_jpype._methods[javaInterface] = methodsDict``. These are private,
  unwrapped implementation details (no public ``jpype.registerWrapper(...)``
  exists), but seeing them spelled out is often the fastest way to
  understand what a ``.pyspi`` file actually does, or to prototype a
  registration before committing it to a resource file.

Argument-passing conventions, and when overloads are allowed
----------------------------------------------------------------

**Reverse direction — a ``METHODS`` entry being called from Java.** Every
entry is invoked unbound-method style, wrapped instance first:
``lambda x, *args: ...``. Arguments arrive as ordinary Python
objects/primitives — standard JPype boxing rules apply on the way in, no
extra unwrapping is needed inside the callable.

**Forward direction — Python calling a proxy method** (``$``-mangled or
plain, whether SPI-registered or hand-built via ``@JImplements``):
varargs methods (``Object... args``) get their trailing array unpacked
before the call reaches Python; a trailing ``PyKwArgs`` argument
(``python.lang.PyKwArgs``, built via ``PyKwArgs.of().kw(name, value)``) is
recognized specially and splits into real Python ``**kwargs`` on the
far side — so ``obj.$bob(1L, 2L, PyKwArgs.of().kw("key", "K"))`` reaches
Python as ``bob(1, 2, key="K")``, not as three positional arguments.

**Overload resolution — the one real limitation to design around.**
``$``-mangling (and the ``.``-mangling used by ``.pyspi`` ``METHODS``
keys) operates on **method name only**, with no visibility into parameter
types. Two Java overloads sharing a name — e.g. ``PyDict``'s
``remove(Object)`` and ``remove(Object, Object)`` — mangle to the exact
same wire name and therefore land on the **same single entry** in
``METHODS``. There is no overload-aware dispatch at the wire layer; if
both overloads need to reach Python, the shared Python callable has to
disambiguate itself, typically on ``len(args)`` (this is exactly what
``PyDict``'s real ``remove`` binding does — dispatches on arity because
both Java overloads share one wire name).

Practical guidance:

- **Prefer a single, unambiguous Java signature per SPI-exposed method**
  when writing a new interface. This is where strong typing pays off —
  a Java interface with one signature per method name maps cleanly onto
  one Python callable per ``METHODS`` key, with no dispatch logic needed
  inside the callable at all.
- **Allow overloads only when the shared callable can cheaply
  disambiguate** — arity is enough for ``remove(Object)`` vs.
  ``remove(Object, Object)``; type-based disambiguation on
  ``isinstance(args[i], ...)`` also works but is more fragile as more
  overloads accumulate.
- **If overloads need genuinely independent behavior that can't cheaply
  share one callable, don't use Java overloading at all** — give the
  interface methods distinct names instead. The SPI plumbing has no way
  to route same-named overloads to different Python callables; the
  interface design has to route around that, not the ``.pyspi`` file.

Practical gotchas for provider authors
-----------------------------------------

Found while building the ``python.collections``, ``python.datetime``, and
``python.pathlib`` providers — none of these are wiring mistakes, they're
wrong assumptions about how Python semantics carry across the bridge, and
the same class of bug is easy to reintroduce in a new provider.

**A Java ``default`` method's hardcoded argument does not reach the Python
side.** Given a Python method with a default argument
(``OrderedDict.move_to_end(key, last=True)``), the natural-looking Java
translation is:

.. code-block:: java

    void moveToEnd(PyObject key, boolean last);
    default void moveToEnd(PyObject key) { moveToEnd(key, true); }

This does not work: dispatch routes by ``METHODS`` key (the mangled method
name), not by Java signature, so the one-arg call reaches the ``.pyspi``
lambda directly with only ``(x, key)`` — the default-method body never
runs. Put the default on the **Python** side instead, exactly like
``io.IOBase.pyspi``'s ``_io_seek(x, offset, whence=0)`` does:

.. code-block:: python

    def _move_to_end(x, key, last=True):
        x.move_to_end(key, bool(last))

The Java ``default`` overload can stay as API surface / a Javadoc anchor,
but must not be relied on to supply the default — the ``.pyspi`` callable
has to handle the shorter argument list itself.

**A stdlib type's "special missing-key/out-of-range" behavior may only be
implemented on one protocol method, not the one you'd inherit for free.**
``collections.Counter``'s "returns 0 for a missing key" behavior is
implemented via ``__missing__``, which fires through ``__getitem__`` only
— not through ``.get()``. It is tempting to assume a `dict` subclass's
special behavior shows up through every inherited ``Map``-surface method;
confirm which protocol method actually implements it before writing
Javadoc that claims otherwise:

.. code-block:: text

    python3 -c "import collections; c = collections.Counter(); print(c.get('x'), c['x'])"
    # None 0   <- these differ; one inherited method cannot cover both

**Return type matters even though dispatch doesn't check it.** A
``$``-mangled or SPI-registered method declared to return plain
``java.lang.Object`` gets a hard ``PyTypeError`` at call time, not a
silent pass-through — the ``Object``-typed conversion path never includes
the Python-proxy conversion the way a ``PyObject``-rooted return type
does. Always declare the tightest real ``PyObject``-hierarchy return type
the method actually produces (``PyTuple``, ``PyString``, ...); plain
``PyObject`` itself is safe but loose — it always matches, so it proves
nothing about the returned shape (a numpy wrapper returning a scalar would
get silently typed as bare ``PyObject`` instead of the specific numeric
type if the interface author wasn't careful).

Worked example: ``python.io``
-------------------------------

``python.io.PyIOWrapperService`` covers the full shape described above in
one small provider:

- ``getModuleNames()`` returns ``{"io", "_io"}``, for the ``io``/``_io``
  split reason given above.
- ``getResources()`` is a one-line directory scan over ``python/io/spi/``,
  so adding a new class means dropping a new ``.pyspi`` file there —
  nothing in the service class itself changes.
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
interfaces, write one ``.pyspi`` file per Python class (dotted
``METHODS`` keys, eager for anything guaranteed to exist at boot,
``lazy: true`` for everything else), and write one ``kind: backend`` file
(plain ``METHODS`` keys) if the package needs its own factory functions.
