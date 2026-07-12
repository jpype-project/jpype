# Java-to-Python stream channeling: explicit stdin/stdout/stderr redirect

## Status (2026-07-11): DONE, implemented and tested same day

Implemented as designed below, with two real bugs found and fixed along
the way (neither changes the design, both were implementation mistakes):

1. `_JavaTextIO.flush()`/`close()` unconditionally called
   `self._stream.flush()`, but `io.TextIOBase.close()` calls `self.flush()`
   internally, and `java.io.Reader` has no `flush()` method at all. Fixed
   by guarding with `hasattr(self._stream, "flush")`.
2. `Interpreter.resetOutput()`/`resetError()`/`resetInput()` originally
   called `getBuiltIn().exec(statement, null, null)`, assuming Java `null`
   would cross the reverse bridge as Python's `None` the way it does in
   plain Python (`exec(source, None, None)` uses the caller's own
   namespace). It doesn't — the reverse bridge's `exec`/`eval` require a
   real dict object for globals, and a bare Java `null` surfaces as
   `exec() globals must be a dict, not python.lang.PyDict` instead. Fixed
   by running these through the same `PyDict` scratch-scope helper as
   `installStdio`, just with no bound variables.
3. The Java-side `_JOutputStream.toPython()`/`_JInputStream.toPython()`
   customizers originally did `import java.io` from Python to reach
   `OutputStreamWriter`/`InputStreamReader` — this only works if
   `jpype.imports` has been enabled in that interpreter, which isn't
   guaranteed for an embedded/Java-driven interpreter. Switched to
   `jpype.JClass("java.io.OutputStreamWriter")`/`InputStreamReader`
   (cached module-level), which works unconditionally.

All four "open questions" below were resolved during implementation (see
inline notes); tests: `native/jpype_module/src/test/java/python/lang/
InterpreterStdioNGTest.java` (Java side, `Interpreter.setOutput`/`setError`/
`setInput`/resets, two independently-redirected `SubInterpreter`s) and
`test/jpypetest/test_streamredirect.py` (Python side, `toPython()` on
`StringWriter`/`StringReader`/`ByteArrayOutputStream`/`ByteArrayInputStream`/
`PrintStream`, plus the free `sys.stdout = x.toPython()` trigger). Both
suites pass; full existing Java and Python test suites re-run clean (one
pre-existing, unrelated failure in `test_sql_h2.py`/`test_sql_sqlite.py`
caused by `[DEBUG]` log noise from a debug native build breaking a
`java_version()` subprocess parse — confirmed present on `HEAD~` before
this work, not a regression).

Buffering (the courtesy `BufferedWriter`/`BufferedReader` wrap discussed
below) was explicitly deprioritized by the user as a later enhancement and
was not implemented in this pass.

Supersedes/expands `plan/IO.md` section F ("stdout/stderr capture"). That
section only covered output and left the trigger point vague ("could also
be a `SubInterpreterConfig`-style builder option ... or just a plain
`sys.stdout = ...` assignment"). This plan commits to a concrete shape:
**explicit, Java-triggered**, via `Interpreter.setOutput()`/`setInput()`/
`setError()`, covering both directions (stdin included), with a secondary
Python-side trigger for symmetry. Motivating case: channeling a
subinterpreter's stdio fully through Java-owned streams so it can be
embedded/sandboxed without touching process-level `stdout`/`stderr`.

## Key finding: no new bridge mechanism needed

Surveyed the existing Java<->Python object-crossing mechanisms before
designing this (see [[jpype_reverse_bridge_testing]] and the `python.io`
work for the mechanisms already in the codebase):

- `org.jpype.proxy.ProxyInstance`/`ProxyFactory`/`ProxyType` (backing
  `jpype.JProxy`/`@JImplements`) is the **opposite** direction: a Python
  object wrapped so *Java* can call it through a JDK dynamic `Proxy`. Not
  usable here.
- `python.lang.PyJavaObject` is a Java-side label for "this is some Java
  object," but explicitly throws `UnsupportedOperationException` from
  `getAttributes()`/`builtin()` — not attribute-accessible from Python.
- There is, however, no need for either: **every plain Java object handed
  into Python already gets ordinary Python attribute access to its public
  methods and fields** — this is core `_jpype`/`pyjp_object.cpp` behavior,
  the same mechanism that lets any Java method's return value be used from
  Python today. Confirmed via `PyBuiltIn.setattr(PyObject obj, CharSequence
  key, Object value)` (`python/lang/PyBuiltIn.java:604`) and
  `Backend.setattr` taking a plain `Object` — no `PyObject` conversion step
  is required before an arbitrary Java object can become the value of a
  Python attribute.

So the entire feature is: expose the raw Java stream to Python (already
automatic), and get *something* assigned to `sys.stdout`. The only real
design decision is where the duck-typed surface (`write`/`flush`/
`encoding`/`isatty`/`closed`/...) gets implemented.

**Revised direction (per user feedback, twice now):**

1. Put the duck-typed surface in Python, not Java — subclass `io.TextIOBase`
   to get context-manager support, `writelines`/`readlines`/line-iteration,
   and the standard `encoding`/`errors`/`closed` contract for free from the
   stdlib, instead of hand-approximating each one.
2. **Give it a public face via JPype's existing `JCustomizer` mechanism,
   not a new class users construct by hand.** `jpype/_jio.py` *already
   exists* — it's the file backing the `java.lang.AutoCloseable`
   customizer that gives every closeable Java object Python's `with`
   support today:

   ```python
   @_jcustomizer.JImplementationFor("java.lang.AutoCloseable")
   class _JCloseable(object):
       def __enter__(self): return self
       def __exit__(self, exception_type, exception_value, traceback): ...
   ```

   `JImplementationFor` mixes the decorated class's methods directly onto
   the generated Python wrapper class for that Java type — every Java
   object that `implements AutoCloseable`, seen from Python, already has
   `__enter__`/`__exit__` "like it always belonged there." The same
   mechanism, applied to `java.io.Writer`/`Reader`/`OutputStream`/
   `InputStream`, gives every Java stream object a `toPython()` method
   directly, with no separate class name for a user to ever import. This
   also directly answers this plan's earlier open question ("should the
   wrapper class be public?") — it doesn't need to be; the method *on the
   Java object* is the public API surface.

   This is not a one-off trick invented for this plan — it is one of
   JPype's core, established idioms for "making Java native look
   Pythonic," per the user. `jpype/_jthread.py` does exactly the same
   thing for `java.lang.Thread`, bolting on `isAttached()`/`attach()`/
   `attachAsDaemon()` via the identical `@_jcustomizer.JImplementationFor
   ('java.lang.Thread')` pattern, so a Java `Thread` object looks like it
   always had JVM-attachment methods. `toPython()` on the `java.io.*`
   hierarchy is the same move applied to streams: extending an existing,
   proven idiom, not introducing a new one.

Java's job shrinks accordingly: normalize the caller's stream to a
`Writer`/`Reader` (a one-line wrap if given a byte stream), hand it to
Python, and call the `toPython()` method that's now already sitting on it
— no `import` needed in the generated snippet, no separate wrapper-class
reference. No Java adapter classes, no new `org.jpype.io` package.

## Design

### Python side: extend the existing `jpype/_jio.py`

```python
import io
from . import _jcustomizer

class _JavaTextIO(io.TextIOBase):
    """Wraps a java.io.Writer or java.io.Reader as a Python text stream."""

    def __init__(self, stream, encoding=None, errors="strict"):
        self._stream = stream
        self._encoding = encoding
        self._errors = errors

    @property
    def encoding(self):
        return self._encoding

    @property
    def errors(self):
        return self._errors

    def writable(self):
        return hasattr(self._stream, "write")

    def readable(self):
        return hasattr(self._stream, "read")

    def write(self, s):
        self._stream.write(s)          # java.io.Writer.write(String) — JPype
        return len(s)                  # converts the Python str directly

    def read(self, size=-1):
        # size < 0: drain via repeated char[]-buffer reads until EOF
        ...

    def readline(self, size=-1):
        ...

    def flush(self):
        self._stream.flush()

    def isatty(self):
        return False

    def close(self):
        self._stream.close()
        super().close()


@_jcustomizer.JImplementationFor("java.io.Writer")
class _JWriter(object):
    def toPython(self, encoding=None, errors="strict"):
        return _JavaTextIO(self, encoding, errors)


@_jcustomizer.JImplementationFor("java.io.Reader")
class _JReader(object):
    def toPython(self, encoding=None, errors="strict"):
        return _JavaTextIO(self, encoding, errors)


@_jcustomizer.JImplementationFor("java.io.OutputStream")
class _JOutputStream(object):
    def toPython(self, encoding="utf-8", errors="strict"):
        import java.io  # java.io.OutputStreamWriter
        return java.io.OutputStreamWriter(self, encoding).toPython(encoding, errors)


@_jcustomizer.JImplementationFor("java.io.InputStream")
class _JInputStream(object):
    def toPython(self, encoding="utf-8", errors="strict"):
        import java.io  # java.io.InputStreamReader
        return java.io.InputStreamReader(self, encoding).toPython(encoding, errors)
```

`write`/`read` delegate straight to `java.io.Writer.write(String)` /
`Reader.read(char[])` — JPype's ordinary Python-str <-> Java-`String`/
`char[]` conversion handles the encoding, so `_JavaTextIO` never touches
raw bytes itself. `OutputStream.toPython()`/`InputStream.toPython()` just
wrap themselves in an `OutputStreamWriter`/`InputStreamReader` for the
requested charset and delegate to the `Writer`/`Reader` version — one
real implementation, four entry points. Since `PrintStream` and every
other JDK stream class `extends OutputStream` (etc.), and `JClass` Python
wrapper classes carry ordinary Python inheritance from their Java
superclass's wrapper, `toPython()` becomes available on every concrete
stream class automatically, not just the four base types listed above —
worth an explicit test (e.g. against `System.out`, a `PrintStream`) to
confirm rather than assume.

### Java side: thin `Interpreter` default methods

```java
public interface Interpreter extends AutoCloseable
{
  PyBuiltIn getBuiltIn();

  default void setOutput(OutputStream out) { installStdio("stdout", out); }
  default void setOutput(Writer out) { installStdio("stdout", out); }
  default void setError(OutputStream err) { installStdio("stderr", err); }
  default void setError(Writer err) { installStdio("stderr", err); }
  default void setInput(InputStream in) { installStdio("stdin", in); }
  default void setInput(Reader in) { installStdio("stdin", in); }

  default void resetOutput() { getBuiltIn().exec("import sys; sys.stdout = sys.__stdout__", null, null); }
  default void resetError()  { getBuiltIn().exec("import sys; sys.stderr = sys.__stderr__", null, null); }
  default void resetInput()  { getBuiltIn().exec("import sys; sys.stdin = sys.__stdin__", null, null); }

  private default void installStdio(String name, Object stream)
  {
    PyBuiltIn b = getBuiltIn();
    PyDict locals = b.dictFromMap(Map.of("_stream", stream));
    b.exec("import sys; sys." + name + " = _stream.toPython()", locals, locals);
  }

  @Override
  void close();
}
```

No `mode` parameter anywhere on the Java side any more — `toPython()`
already knows whether it's producing a readable or writable wrapper from
which customizer (`Writer`/`OutputStream` vs. `Reader`/`InputStream`) it
was called on, so `installStdio` collapses to one overload-free helper.
Bind the (already-Python-visible) Java stream object into a locals dict
via `PyBuiltIn.dictFromMap`, then one `exec()` call — that is the entire
Java contribution beyond the two trivial `OutputStream`/`InputStream`
delegating overloads. `resetOutput`/`resetError`/`resetInput` lean on
Python's own preserved `sys.__stdout__`/`__stderr__`/`__stdin__` rather
than caching originals on the Java side.

Because each `SubInterpreter` has its own `PyInterpreterState` and its own
`sys` module object, calling `setOutput`/etc. on one subinterpreter
redirects only that subinterpreter's streams — this "just works" from the
existing per-subinterpreter isolation ([[jpype_subinterpreter_difficulty]]),
no additional per-subinterpreter bookkeeping needed in this plan.

### Python-side trigger

This is now free: `toPython()` sits directly on every Java stream object
a Python script already holds, so a user who wants to trigger this from
Python — no Java call at all — just does:

```python
import sys
sys.stdout = my_java_writer.toPython()
```

No import of any `jpype` internals, no separate class reference — this
*is* the "hook for a user to trigger it from Python," and it falls out of
the customizer design for free rather than needing its own API.

### Buffering / GIL — simpler than section F assumed

`plan/IO.md`'s section F assumed this direction would need the same
chunked-buffering treatment as the `python.io` adapters. It doesn't: a
`sys.stdout.write(...)` call is **Python calling into Java**, the ordinary
forward JPype call path, not the reverse-bridge round trip that made
`PyIOInputStream`/`PyIOOutputStream` slow one-byte-at-a-time. Python
already holds the GIL when making the call, the call lands directly in
plain Java code, and there is no callback into Python required — so none
of [[jpype_gil_reacquire_bug]]'s reacquire logic applies. The only
buffering worth doing is wrapping a caller-supplied unbuffered
`OutputStream`/`InputStream` in a `BufferedWriter`/`BufferedReader`
internally as a courtesy default in the `OutputStream`/`InputStream`
overloads above — a Java-side convenience, not a correctness requirement.

## Open questions (resolved during implementation)

1. **Placement: shared `Interpreter`, not `SubInterpreter`-only.** Went
   with the shared interface as planned; `MainInterpreter` and
   `SubInterpreter` both get `setOutput`/`setError`/`setInput` for free.
   Verified against both in `InterpreterStdioNGTest`.
2. **`read(size=-1)`/`readline(size=-1)` against a `java.io.Reader`.**
   Implemented as a small internal char-buffer loop (`_read_chunk`) in
   `_JavaTextIO`, converting each `char[]` result to a Python `str` via
   `java.lang.String(buf, 0, n)`. `readline` reads one char at a time,
   which is fine for the stdin/interactive use case this plan targets, not
   for bulk throughput (that's the buffering enhancement, still deferred).
3. **`.encoding` asymmetry.** Confirmed acceptable and left as designed:
   `OutputStream`/`InputStream` report the caller's requested charset
   name (default `"utf-8"`); `Writer`/`Reader` report `None` unless the
   caller passes `encoding=` explicitly.
4. **`toPython()` reachable on concrete JDK subclasses.** Confirmed
   empirically: `java.io.PrintStream` (via `OutputStream`) gets
   `toPython()` through ordinary Python inheritance, no per-subclass
   registration needed. See `testPrintStreamInheritsToPython` in
   `test/jpypetest/test_streamredirect.py`.

## Suggested implementation order

1. `jpype/_jio.py`'s `_JavaTextIO` alone, tested directly from a plain
   Python script against a real `java.io.StringWriter`/`StringReader` via
   `jpype.startJVM()` — no `Interpreter` wiring yet, prove the class
   against real Java `Writer`/`Reader` objects before touching Java code.
2. `Interpreter.setOutput`/`setError`/`setInput`/`resetOutput`/
   `resetError`/`resetInput` default methods (the `installStdio` helper
   above).
3. Tests: capture `print()`/`sys.stderr.write(...)` into a
   `ByteArrayOutputStream`, assert exact bytes; `resetOutput()` restores
   the original; two `SubInterpreter`s redirected to two different
   `ByteArrayOutputStream`s with no cross-talk; a stdin round trip via
   `sys.stdin.readline()`/`input()` fed from a canned `ByteArrayInputStream`.
4. Defensive-probing hardening (`buffer`/`newlines`/etc. on `_JavaTextIO`)
   only if a real library trips on the minimal surface from step 1-3.
