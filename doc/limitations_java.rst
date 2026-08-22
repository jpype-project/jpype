Known Limitations (Java calling Python)
========================================

This section lists limitations specific to the reverse bridge — Java code
calling into Python objects through ``python.lang``/``python.io``/
``python.collections``/``python.datetime``/``python.decimal``/
``python.pathlib``/``python.exceptions``. Limitations that come from the JVM
or CPython themselves (unsupported platforms, PyPy, restarting or running
multiple JVMs, ``java.library.path``, fault handlers) apply equally in this
direction and are documented once, in :doc:`limitations_py`, rather than
repeated here.


.. _reverse_bridge_limitations_unimplemented_stdlib:

Unimplemented standard library coverage
----------------------------------------

Only a subset of the Python standard library has a typed Java front end
today. Implemented, with a ``WrapperService`` SPI provider:

- ``python.io`` — ``io``/``_io`` (``PyIOBase``/``PyBufferedReader``/
  ``PyTextIOWrapper``/etc.)
- ``python.collections`` — ``collections`` (``PyDeque``/``PyOrderedDict``/
  ``PyDefaultDict``/``PyCounter``/``PyChainMap``)
- ``python.datetime`` — ``datetime`` (``PyDate``/``PyDateTime``/
  ``PyTimeDelta``)
- ``python.decimal`` — ``decimal`` (``PyDecimal``)
- ``python.pathlib`` — ``pathlib`` (``PyPath``)

Anything else — including modules with a scoped-but-not-built plan
(``re.Pattern``/``re.Match``, ``queue.Queue``/``LifoQueue``/
``PriorityQueue``) — crosses into Java as a bare, untyped ``PyObject``.
This still works (every method is reachable through ``PyObject``'s own
attribute/call surface, or via ``$``-mangled direct dispatch on a custom
interface — see :doc:`customizers_java`), it just doesn't get a
purpose-built Java-shaped API the way ``java.util.Map`` gets from
``python.collections``.


.. _reverse_bridge_limitations_no_pickling_numpy:

No reverse-direction serialization or NumPy story
----------------------------------------------------

:doc:`pickling_py` (JPickler, Java objects serialized for Python pickling)
and :doc:`numpy_py` (bulk array transfer) both describe mechanisms for
Python code consuming Java data. Neither has a Java-calling-Python
counterpart today: there is no typed bulk-array or buffer-promotion path
for Java code to pull a NumPy array out of Python efficiently, and no
supported way for Java to participate in pickling a Python object. Treat
data crossing this direction as ``python.io`` streams or individual
``PyObject`` attribute/method calls until one of these is built.


.. _reverse_bridge_limitations_subinterpreters:

Subinterpreter isolation depends on how you launch it
--------------------------------------------------------

``org.jpype.Interpreter``/``SubInterpreter`` can start additional Python
subinterpreters alongside the main one (verified with two subinterpreters
plus the main interpreter running concurrently with no cross-talk or
hangs). ``_jpype`` is built with multi-phase init and declares itself
eligible for a genuinely isolated own-GIL configuration
(``Py_mod_multiple_interpreters = Py_MOD_PER_INTERPRETER_GIL_SUPPORTED``),
so full PEP 684 isolation — its own GIL and its own CPython object
allocator, not shared with the main interpreter — is available and is in
fact the *default* ``org.jpype.SubInterpreterBuilder`` configuration
(``ownGil()``). The one exception is the bare, no-argument
``SubInterpreter.start()`` entry point: it always launches with the older
**legacy** configuration — shared process GIL, shared allocator — kept for
backward compatibility. Don't assume a subinterpreter is legacy-style just
because it's a subinterpreter; check which launch path produced it. Use
``SubInterpreterBuilder.elevated()`` if shared-GIL behavior is what you
actually need (e.g. to import a single-phase-init extension that own-GIL
isolation's ``check_multi_interp_extensions`` would otherwise reject).

A related, deliberate guard applies regardless of GIL configuration: a
proxy or wrapped object obtained inside one (sub)interpreter cannot be
silently smuggled into another interpreter's scope. Attempting to do so
raises a detection error rather than corrupting state — this is
intentional, not a bug to work around.


.. _reverse_bridge_limitations_async_thread_pool:

Async calls are bounded by a fixed-size thread pool
--------------------------------------------------------

``PyCallable.callAsync``/``callAsyncWithTimeout`` run on a bounded native
thread pool, not one thread per call. Every call into Python from any of
those threads still has to acquire the single process GIL, so the pool
bounds *concurrency*, not *throughput* — see :doc:`threading_java` for the
full GIL model. Don't assume arbitrarily high fan-out from
async calls scales linearly; it's still gated by one interpreter's GIL
underneath.
