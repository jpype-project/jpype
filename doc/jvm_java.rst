
Controlling the Interpreter (for Java)
==========================================

This is the Java-side counterpart to :doc:`jvm_py`: there is no JVM to
start from this direction (Java is already running), but there is an
embedded CPython interpreter with its own lifecycle, and that lifecycle has
sharper edges than starting/stopping a JVM does. Every claim below is
grounded in ``org.jpype.MainInterpreter``.

.. contents::
   :local:
   :depth: 1


Starting and stopping
------------------------

``MainInterpreter`` is a singleton (``getInstance()``). ``start(String[])``
boots the embedded CPython interpreter inside the JVM process;
``isStarted()`` reports whether that has already happened:

.. code-block:: java

    import org.jpype.MainInterpreter;
    import org.jpype.Script;

    MainInterpreter interpreter = MainInterpreter.getInstance();
    if (!interpreter.isStarted())
        interpreter.start(new String[0]);

    Script context = new Script(interpreter);

    // ... use the interpreter ...

    interpreter.close();

``close()`` is **irrevocable** -- once called, the interpreter is
permanently terminated for the life of the process; there is no restart,
mirroring the JVM-restart limitation described in :doc:`jvm_py`. A typical
long-lived application calls ``start()`` once near the top of ``main()``
and relies on JVM shutdown to tear things down:

.. code-block:: java

    if (MainInterpreter.getInstance().isStarted())
        MainInterpreter.getInstance().close();


Launch configuration
------------------------

``start(String[])`` doesn't need any arguments to work -- by default it
finds a ``python``/``python.exe`` on ``PATH``, probes it once, and caches
the result. But every part of that is overridable via ``python.config.*``
Java system properties, set before ``start()`` (or before ``prepare()``,
see below):

.. list-table::
   :header-rows: 1

   * - Property
     - Controls
   * - ``python.config.program_name``
     - ``PyConfig.program_name``; defaults to the resolved executable.
   * - ``python.config.home``
     - ``PyConfig.home``.
   * - ``python.config.path``
     - Extra ``sys.path`` entries (``File.pathSeparator``-joined), combined
       with ``python.module.path`` (see below).
   * - ``python.config.executable``
     - ``PyConfig.executable``. Can be set independently of the executable
       used to *probe* the environment -- see "Two-step configuration"
       below.
   * - ``python.config.isolated``
     - Selects ``PyConfig_InitIsolatedConfig`` over
       ``PyConfig_InitPythonConfig``.
   * - ``python.config.fault_handler``, ``quiet``, ``verbose``,
       ``site_import``, ``user_site_directory``, ``write_bytecode``
     - The matching ``PyConfig`` boolean, 1:1.

Plus ``python.module.path`` (read once, at construction, not a
``python.config.*`` property), ``python.executable``, ``python.lib``, and
the ``jpype.*`` cache/probe/install family (``jpype.lib``, ``jpype.arch``,
``jpype.nocache``, ``jpype.install``, ``jpype.version``,
``jpype.properties``).

``python.config.prefix``/``exec_prefix`` don't exist as properties --
Python calculates both from ``home`` on its own; there's nothing to set.

Two environment variables matter: ``PATH`` (searched for a
``python``/``python.exe`` binary) and ``PYTHONHOME`` (used to derive an
executable path, but only if ``python.executable`` is unset).

Executable resolution, in order:

1. ``System.getProperty("python.executable")``
2. ``System.getenv("PYTHONHOME")`` + the platform-specific suffix
3. The first ``python``/``python.exe`` found on ``PATH``
4. Otherwise, ``start()`` throws ``RuntimeException``.

Two-step configuration
~~~~~~~~~~~~~~~~~~~~~~~~

``prepare()`` runs the probe/cache lookup (below) and applies its results
as system-property defaults, without launching the interpreter yet;
``start()`` calls it automatically if it hasn't already run. Calling
``prepare()`` explicitly, inspecting/overriding a ``python.config.*``
property, and *then* calling ``start()`` is how you override one probed
value (say, ``python.config.executable``) without having to supply every
other value yourself:

.. code-block:: java

    MainInterpreter interpreter = MainInterpreter.getInstance();
    interpreter.prepare();
    System.setProperty("python.config.executable", "/opt/venv/bin/python3");
    interpreter.start(new String[0]);

The probe/cache subsystem
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The first time a given Python executable is used, ``prepare()`` runs it
once (``<executable> -c <probe source>``) to discover its ``home``,
``sys.path``, and the location of the ``_jpype`` native library, then
caches the result in ``~/.jpype/jpype.properties`` (``%AppData%\Roaming\JPype``
on Windows), keyed by a hash of the executable's path string. Later
launches against the same executable path hit the cache and skip the
subprocess entirely.

This cache is **not** validated against the Python installation itself --
a cache hit only checks that the cached ``python.lib`` and ``jpype.lib``
paths still exist on disk, not that they're still correct for the current
environment. Deleting and recreating a venv at the same path with a
different Python version can still leave a stale cache entry that isn't
detected until ``System.load()`` fails with ``UnsatisfiedLinkError`` later
in startup. If you hit an unexplained native-library load failure after
changing a Python environment, set ``jpype.nocache=true`` (the only
cache-bypass switch) and retry before digging further.

If the probe fails and ``jpype.install=true``, ``start()`` attempts a
self-heal via ``pip install`` (preferring a local wheel under
``~/.jpype/dep`` if present) and re-probes -- useful for a from-scratch
environment that doesn't have JPype's Python-side package installed yet.

Module search paths
~~~~~~~~~~~~~~~~~~~~~

Module paths come from two sources, concatenated: ``python.module.path``
(set once, when the ``MainInterpreter`` is constructed -- typically used to
restrict an embedded deployment to a known set of modules) and
``python.config.path`` (typically the probe's dumped ``sys.path``, or your
own override). Both lists are passed to the native layer and appended to
``sys.path`` before the interpreter starts running any Python code.

Subinterpreters don't use any of this
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

None of the ``python.config.*`` machinery above applies to
``org.jpype.SubInterpreter``/``SubInterpreterBuilder``. A subinterpreter's
entire configuration surface is the seven ``PyInterpreterConfig`` booleans
(``useMainObmalloc``, ``allowFork``, ``allowExec``, ``allowThreads``,
``allowDaemonThreads``, ``checkMultiInterpExtensions``, ``ownGil``) plus
stdio redirection -- see :doc:`threading_java`. It always inherits the
process-wide module search path the main interpreter already established;
there's no per-subinterpreter ``home``/``executable``/``sys.path``
override, by design (PEP 684 doesn't expose most ``PyConfig`` fields
per-subinterpreter the way ``Py_InitializeFromConfig`` does for the main
interpreter).


PyBuiltIn and Script
------------------------

``getBuiltIn()`` returns the interpreter's top-level ``PyBuiltIn`` -- the
handle for Python's builtins (``str()``, ``int()``, ``dict()``, ``eval()``,
...). ``org.jpype.Script`` wraps a ``PyBuiltIn`` together with its own
private globals/locals dict, which is usually more convenient than sharing
the interpreter's top-level namespace across unrelated pieces of code (each
``Script`` gets an isolated namespace, the same way a fresh Python module
would):

.. code-block:: java

    import python.lang.PyBuiltIn;

    PyBuiltIn builtin = interpreter.getBuiltIn();
    Script context = new Script(interpreter);   // its own globals/locals


Running code the way ``python`` does
----------------------------------------

``org.jpype.Runner`` runs Python code the way the real ``python`` command
line would -- as a module (``-m``), a script file, or an inline command
(``-c``) -- built on ``runpy``, the same machinery real Python itself uses
for ``-m``/script execution, so ``sys.path`` handling, ``__main__``
naming, and package/zipapp support all match real behavior for free.
Construct it from any ``Interpreter`` (``MainInterpreter`` or a
``SubInterpreter``):

.. code-block:: java

    import org.jpype.Runner;

    Runner runner = new Runner(interpreter);

    // python -m pip install requests
    runner.pipInstall("install", "requests");

    // python -c "print(6 * 7)"
    runner.runCommand("print(6 * 7)");

    // python script.py arg1 arg2
    runner.runFile(Paths.get("script.py"), "arg1", "arg2");

Each call sets ``sys.argv`` for the duration of the run (``argv[0]`` is
the module name, script path, or literal ``"-c"``, matching real Python)
and restores whatever ``sys.argv`` held before, success or exception --
unlike a real ``python`` process, an ``Interpreter`` doesn't exit after
one run, so leaving ``sys.argv`` mutated would leak into whatever code
uses the interpreter next. All three methods return the executed
namespace as a ``PyDict``, so a Java caller can pull out whatever the run
defined:

.. code-block:: java

    PyDict ns = runner.runCommand("answer = 6 * 7");
    ns.get("answer");   // 42

A run that raises surfaces exactly like any other ``Script.exec``/``eval``
failure -- the matching ``python.exceptions.*`` type. ``SystemExit`` is
not special-cased: it surfaces as ``python.exceptions.PySystemExit``, same
as any other exception crossing the bridge. This matters in practice for
``pipInstall`` -- real ``pip`` calls ``sys.exit()`` on completion *even
when it succeeds*, so a clean ``pipInstall("list")`` raises
``PySystemExit`` with exit code 0, not a normal return:

.. code-block:: java

    import python.exceptions.PySystemExit;

    try {
        runner.pipInstall("install", "requests");
    } catch (PySystemExit exit) {
        // exit.get() is the PyExc; its "code" attribute is pip's exit code
    }

``MainInterpreter.dispatch(String[] args)`` builds ``-c``/``-m``/bare-file/
``-i`` argv parsing on top of ``Runner``, matching the real ``python``
binary's own command-line semantics -- a leading run of ``-i`` flags
requests the interactive shell after the action runs; no action and no
``-i`` falls through directly to ``interactive()``:

.. code-block:: java

    // java org.jpype.MainInterpreter -m pip install requests
    MainInterpreter.main(new String[]{"-m", "pip", "install", "requests"});

``main(String[])`` calls ``dispatch()`` after ``start()``, so running the
``MainInterpreter`` class directly from the command line behaves like
running ``python`` itself.


The GIL model, briefly
--------------------------

Every call across the Java/Python boundary -- ``eval()``, ``exec()``, a
``PyCallable`` invocation, even a trivial ``toString()`` on a wrapped
object -- acquires and releases CPython's Global Interpreter Lock around
the call. This makes single-threaded use transparent, but it means **any**
Java thread that touches a ``PyObject`` is a potential GIL contention
point, and a call that never returns control to Java (blocked native code,
an uncaught leak in a custom SPI provider) can starve every other thread
trying to call into Python. :doc:`threading_java` covers this in full,
including the bounded async thread pool behind ``PyCallable.callAsync``. If
you're only calling Python from a single Java thread, this model is
invisible to you; the moment a second thread is involved, read
:doc:`threading_java` before writing that code.


Where to next
---------------

- :doc:`threading_java` -- the GIL-per-call model in full, and what's safe
  to do from a background thread.
- :doc:`quickguide_java` -- a shorter, example-first tour covering the same
  startup/``Script``/``eval`` ground.
