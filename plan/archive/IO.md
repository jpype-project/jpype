# python.io: porting Python's `io` module with InputStream/OutputStream promotion

## Status (2026-07-11, even later): C done - buffered rewrite implemented and benchmarked; A-F all complete

`PyIOInputStream`/`PyIOOutputStream`/`PyIOReader`/`PyIOWriter` (all four
adapters) now buffer internally (default 8KB/8K-chars, package-private
constructor-overridable for tests): reads are served out of an internal
buffer refilled by a single bulk `read(size)` call; writes accumulate
locally and flush via a single bulk `write(...)` call on buffer-full,
`flush()`, or `close()`. A request at least as large as the buffer bypasses
buffering entirely (no pointless extra copy). This is a pure Java-side
change - the four adapters' `stream`/constructor contracts are unchanged, so
`asInputStream()`/`asOutputStream()`/`asReader()`/`asWriter()` callers see no
API difference, only different visibility timing: a pending write is not
observable on the underlying Python object until flushed (existing tests in
`PyIOStreamAdapterNGTest`/`PyWriterAdapterNGTest` that checked state
immediately after unflushed single-byte/char writes were updated to flush
first - correct per ordinary `java.io.Buffered*` semantics, not a
compatibility break of anything documented).

**Benchmark - and a real, asymmetric finding, not just a confirmation.**
Ran a real-bridge (not simulated) A/B on `PyTestHarness` context: same
adapter code path both sides, differing only in buffer size (`1`, i.e. a
fill/flush on every single-element call - exactly the old unbuffered
behavior - vs the real default), 20,000 single-element calls each,
`python3.10`:

| Operation | speedup |
|---|---|
| single-byte `InputStream.read()` | **1.8x** |
| single-byte `OutputStream.write(int)` | **1164x** |
| single-char `Reader.read()` | **778x** |
| single-char `Writer.write(int)` | **33x** |

Byte writes and char reads/writes all get a huge win because, once buffered,
the bulk conversion at the buffer boundary (`PyString.toString()` for reads,
`context.bytes(byte[])`/`String` construction for writes) is itself a real
bulk operation with no per-element bridge cost. **Byte reads are the
outlier**: `PyBytes` -> `byte[]` extraction (`chunk.get(i)` +
`builtin().asLong(...)`) has no bulk helper anywhere in the codebase (this
was already true, and already flagged, before this pass - see "Open
follow-up" in the prior status section below), so it still costs one bridge
call *per byte* inside `fill()`, same as the old code did inside its
per-request loop. Buffering only removes the `stream.read(n)` round trips,
not this per-element cost - which dominates, capping the byte-read win far
below every other direction. If byte-read throughput ever matters enough to
chase further, the lever is a bulk `PyBytes` -> `byte[]` conversion helper
(mirroring `Backend.newBytesFromBuffer`'s existing reverse direction), not
more buffering - buffering alone has already captured everything it can for
this specific path.

New test coverage: `PyIOBufferingNGTest` (6 tests, part of the normal suite)
- asserts call counts collapse to `ceil(N/bufferSize)` for small requests
  and to exactly 1 for a request at least as large as the buffer, using a
  dynamic-proxy call counter wrapping a real `PyBytesIO`/`PyStringIO`
  (forwards every call, counts `read`/`write` invocations) rather than a
  hand-rolled fake implementing the full `PyBufferedIOBase`/`PyTextIOBase`
  surface. `PyIOBufferingBenchmark` (excluded from the normal suite by
  surefire's default `*Test.java`-only pattern, unlike `*NGTest.java`; run
  explicitly with `-Dtest=PyIOBufferingBenchmark`) is the wall-clock
  benchmark behind the table above.

Full suite: 535/535, 0 failures/errors, 14 pre-existing skips
(`mvn -o test -Dpython.executable=python3.10`).

With this, all of sections A/B/C/D/F are done - `plan/IO.md` is complete
except for the explicitly-deferred items in section E (`$foo` dispatch, a
real third-party numpy-shaped provider) and the byte-read extraction-helper
follow-up noted above, neither of which block anything.

## Status (2026-07-11, later): sections A, B, D, F done; only C (buffering rewrite) still pending

Implemented this pass, on top of the existing `PyBytesIO`/`PyStringIO`/
`asInputStream()`/`asOutputStream()` foundation:

- **A.** `PyRawIOBase` (+ `io.RawIOBase.pyspi`, eager), `PyFileIO` (+
  `_io.FileIO.pyspi`, lazy), `PyBufferedReader`/`PyBufferedWriter`/
  `PyBufferedRandom` (+ matching lazy `.pyspi` files, empty `METHODS` —
  nothing beyond `PyBufferedIOBase`'s surface), `PyTextIOWrapper` (+
  `_io.TextIOWrapper.pyspi`, lazy, `line_buffering()`/`write_through()`).
  `IO.fileIO(CharSequence path, CharSequence mode)` added to the
  `python.io.IO` mini-backend, delegating to `io.open(path, mode,
  buffering=0)`.
- **B.** `PyIOReader`/`PyIOWriter` (package-private, mirroring
  `PyIOInputStream`/`PyIOOutputStream`'s shape exactly) + `default
  asReader()`/`asWriter()` on `PyTextIOBase`.
- **D.** `PyFileIONGTest` (real temp files via `Files.createTempFile`,
  covers `write`/`readall`/`readinto`/`name`/`mode`/`close`),
  `PyReaderAdapterNGTest`/`PyWriterAdapterNGTest` (including a non-ASCII
  round trip), `PyBufferedAndTextWrapperNGTest` (smoke tests for
  `PyBufferedReader`/`PyBufferedWriter`/`PyBufferedRandom`/
  `PyTextIOWrapper` via real `open()` calls, cast from `context.eval(...)`).
  All new + pre-existing tests pass (1537 PASS / 0 FAIL / 7 pre-existing
  SKIP, `mvn -q test -Dpython.executable=python3.10`).

Not done this pass: **C** (the buffered rewrite of
`PyIOInputStream`/`PyIOOutputStream`/`PyIOReader`/`PyIOWriter` — all four
adapters are still the unbuffered MVP, one bridge round-trip per element for
single-byte/char calls) and **F** (stdout/stderr redirect). Both remain
exactly as scoped below.

## Status (2026-07-10, later): remaining-work punch list, scoped and not started

Scope confirmed with user: **`$foo` method-dispatch (`plan/SPI.md`) is
explicitly out of scope** — no concrete consumer yet. Everything below is
additive `python.io` work on top of the lazy SPI mechanism (`plan/SPI.md`,
implemented and live). Grounded in an Explore survey of the current
`native/jpype_module/src/main/java/python/io/` tree this session.

**A. Remaining concrete classes** (pattern: `PyBytesIO.java`/`PyStringIO.java`
are the template — a bare interface extending the right abstract base,
zero Java method bodies, all behavior in a matching `.pyspi` file; see
`native/jpype_module/src/main/resources/python/io/spi/_io.BytesIO.pyspi`
for the resource-file template):
- New `PyRawIOBase.java` interface (`extends PyIOBase`) — `read(size)`,
  `readall()`, `readinto(buffer)`, `write(buffer)` — needed as the base for
  `PyFileIO`. Plus `io.RawIOBase.pyspi` (abstract, eager, mirrors
  `io.BufferedIOBase.pyspi`'s shape).
- `PyFileIO.java` (`extends PyRawIOBase`, adds `name()`, `mode()`) +
  `_io.FileIO.pyspi` (`lazy: true`, matching `_io.StringIO.pyspi`'s
  header style).
- `PyBufferedReader.java`, `PyBufferedWriter.java`, `PyBufferedRandom.java`
  (all `extends PyBufferedIOBase`, no extra methods needed beyond the base
  surface) + matching `_io.BufferedReader.pyspi` /
  `_io.BufferedWriter.pyspi` / `_io.BufferedRandom.pyspi` (`lazy: true`).
- `PyTextIOWrapper.java` (`extends PyTextIOBase`, adds `line_buffering()`,
  `write_through()`) + `_io.TextIOWrapper.pyspi` (`lazy: true`).
- `IO` mini-backend (`python/io/IO.java`) gains `fileIO(CharSequence path,
  CharSequence mode)` delegating to Python `open(path, mode)`, following
  the existing `bytesIO()`/`stringIO()` pattern; add the matching entry to
  `python.io.IO.pyspi`'s `METHODS` dict.
- All new resources default to `lazy: true` (consistent with `StringIO`,
  the established precedent, and cheaper at boot).

**B. `asReader()`/`asWriter()` promotion**:
- New package-private `PyIOReader.java` (`extends java.io.Reader`) /
  `PyIOWriter.java` (`extends java.io.Writer`), mirroring
  `PyIOInputStream.java`/`PyIOOutputStream.java`'s exact shape, built on
  `PyTextIOBase.read(size)`/`write(CharSequence)`, using
  `PyBuiltIn.str(Object)` for the Java→Python direction (no bulk
  `PyString`→`char[]` helper exists — confirmed none in `python/lang` —
  so extraction is via `PyString.toString()` once per internal-buffer
  refill).
- Add `default Reader asReader() { ... }` / `default Writer asWriter() { ... }`
  to `PyTextIOBase.java`, matching `PyBufferedIOBase.java`'s existing
  `asInputStream()`/`asOutputStream()` pattern.

**C. Buffered rewrite of the binary adapters** — current
`PyIOInputStream`/`PyIOOutputStream` are confirmed unbuffered (every
`read()`/`write(int)` single-byte call is its own Python round trip). Give
them (and the new Reader/Writer) an internal buffer (default 8KB),
refilled/flushed via a single bulk Python call; large requests bypass the
buffer. No new native/`Backend` helper needed or in scope — no bulk
`PyBytes`→`byte[]` primitive exists anywhere in `Backend`/`python.lang`,
and the buffering change alone (reducing round trips, not per-element
extraction cost) delivers the win the original design asked for.

**D. Tests** — new `PyFileIONGTest`, `PyReaderAdapterNGTest`/
`PyWriterAdapterNGTest` (real temp files via `Files.createTempFile`),
extend `PyIOStreamAdapterNGTest` with a round-trip-count assertion, a
smoke test per new concrete class — all following the existing
`PyTestHarness`/`IO.using(context)` style.

**E. Still deferred, not part of this pass**: `$foo` dispatch; the
GIL/threading stress test for the adapters; a real third-party
(numpy-shaped) lazy provider.

**F. stdout/stderr capture (Java stream duck-typed as a Python file object)**
— superseded by `plan/StreamRedirect.md` (2026-07-11), **DONE** same day:
explicit `Interpreter.setOutput()`/`setInput()`/`setError()` (+ resets) on
the shared `Interpreter` interface, backed by a `toPython()` JCustomizer on
`java.io.Writer`/`Reader`/`OutputStream`/`InputStream` (`jpype/_jio.py`).
Extends scope to stdin as well. Kept here for history; see that plan for
the design and `doc/userguide.rst`'s "Customizing java.io Streams" section
for user-facing docs.
— scoped 2026-07-11 from `plan/JepParity.md` (jep parity survey), sequenced
*after* A-D above since it's the mirror-image direction of the
`asReader()`/`asWriter()` promotion (a Java `OutputStream`/`Writer` made to
look like a Python file object, rather than a Python file object promoted to
a Java stream) and reuses the same buffering/encoding groundwork. jep has
this via `JepConfig.redirectStdout/redirectStdErr`
(`src/main/java/jep/JepConfig.java`) + `redirect_streams.py`; jpype has no
equivalent.
- A small Java-defined Python-facing object implementing the minimal
  duck-typed surface Python's `sys.stdout`/`sys.stderr` actually need at
  runtime — `write(str)`, `flush()`, and ideally `isatty()`/`encoding`/
  `errors` for library code that probes them defensively (e.g. `logging`,
  `print`'s own internals) — backed by a Java `OutputStream`/`Writer`
  supplied by the caller.
  * This is *not* the `PyTextIOWrapper`/`PyBufferedWriter` concrete-class
    port from section A — those wrap a real Python-side `io` object; this
    is the reverse direction: a Java-implemented object handed *into*
    Python and assigned to `sys.stdout`. Likely implemented via the
    existing `JPProxy` mechanism (Java object implementing a small
    Python-callable-attribute protocol) rather than anything from the
    `python.io` interface hierarchy.
- Entry point: something like `Interpreter.redirectStdout(OutputStream)` /
  `redirectStderr(OutputStream)` (exact placement TBD — could also be a
  `SubInterpreterConfig`-style builder option, see
  `plan/SubInterpreterFactory.md`, or just a plain `sys.stdout = ...`
  assignment made available post-`start()`) that installs the proxy.
- Buffering: reuse the chunked-buffering design from the "Chunked
  buffering" section below rather than a naive per-`write()`-call round
  trip.
- Tests: assert Python `print()`/`sys.stderr.write(...)` calls actually
  land in the backing Java stream, including from a background Python
  thread and from inside a subinterpreter, matching the existing
  GIL-stress-test pattern used elsewhere in this plan.

**Verification**: `mvn -q test -Dpython.executable=python3.10` (expect
475 + new tests, zero regressions); rebuild the real `ant` jar and rerun
`test_io.py` standalone; add a `test_io_reader_writer.py`-style standalone
script exercising `asReader()`/`asWriter()` and the new concrete classes
from a plain launched script, matching `test_io.py`'s existing pattern.

## Goal

Add a `python.io` package (parallel to `python.lang`/`python.exceptions`) that
wraps Python's `io` module — `IOBase`, `RawIOBase`, `BufferedIOBase`,
`TextIOBase`, and the concrete classes (`BytesIO`, `StringIO`, `FileIO`,
`BufferedReader`, `BufferedWriter`, `BufferedRandom`, `TextIOWrapper`) —
using the same reverse-bridge pattern as `python.lang`. On top of the
straight port, every `PyIOBase`-family object must be promotable to a real
`java.io.InputStream` / `OutputStream` (and `Reader`/`Writer` for the text
variants) so Java code that only knows standard Java I/O can consume a
Python file-like object without learning the Python-flavored API.

Deferred per user (2026-07-10): scope only, no implementation this session.
See [[jpype_python_io_port_todo]] memory for the original ask.

## Status (2026-07-10, later same day): asInputStream()/asOutputStream() implemented, unbuffered MVP

`PyBufferedIOBase.asInputStream()`/`asOutputStream()` are implemented and
tested (`native/jpype_module/src/main/java/python/io/PyIOInputStream.java`,
`PyIOOutputStream.java`; `PyIOStreamAdapterNGTest`), plus a
`java.io.InputStream`/`OutputStream` `JConversion` in `jpype/protocol.py`
so a plain `io.BytesIO` can be passed straight into a Java API expecting a
stream. Verified end-to-end from a plain launched script (`jpype.startJVM()`,
not the NGTest harness) via `test_io.py` at the repo root.

This is intentionally the simple/MVP version of the "Chunked buffering
(required, not optional)" design below, not the full thing:

- **No internal Java-side buffering.** Every `InputStream.read(byte[])`
  issues exactly one bridge round-trip (`stream.read(len)`), and every byte
  within that chunk is extracted via a separate `PyBytes.get(i)` +
  `asLong(...)` round-trip — i.e. still O(N) bridge calls per N bytes read,
  not O(N / bufferSize). Single-byte `read()`/`write(int)` are equally
  unbuffered. Explicitly not tuned for throughput (see the classes'
  javadoc) — this exists to prove the SPI wiring end-to-end, not for
  production use.
- **No `Reader`/`Writer` promotion for `PyTextIOBase`** — only the binary
  `PyBufferedIOBase` got `asInputStream()`/`asOutputStream()`. Text
  promotion (`asReader()`/`asWriter()`) is still just this plan's design,
  unimplemented.
- **No GIL/multi-thread stress test** — the "GIL / threading concerns"
  section's dedicated background-thread test wasn't written.

If real throughput matters later, revisit "Chunked buffering" below and
replace `PyIOInputStream`/`PyIOOutputStream`'s per-element extraction with
proper bulk byte-array marshalling (there's no existing bulk `PyBytes` ->
`byte[]` conversion helper anywhere in the codebase yet — worth checking
whether the buffer-protocol path used for `byte[]` -> `PyBytes`, see
`PyBuiltIn.bytes(Object)`, has a symmetric reverse that's just not exposed
yet, before building one from scratch).

Also fixed in passing: `native/build.xml` (the `ant`-based jar the actual
`project/dev.mk`/`pip install -e .` build ships) was silently dropping
`.pyspi` resources and `META-INF/services/*` from `org.jpype.jar` — its
resource-copy step only whitelisted `*.class`/`*.properties`/`*.txt`. This
meant the SPI's eager registration would never have worked outside the
Maven test build. Broadened to copy everything under `src/main/resources`
except `.java`. This is how `test_io.py` was actually able to run against
a real launched-script jar in the first place.

## What already exists to build on

The codebase has two precedents that cover almost everything this needs —
this is assembly of existing patterns, not new mechanism, much like the SPI
plan (`plan/SPI.md`) turned out to be mostly wiring.

### 1. Concrete-class-name → Java-interface dispatch (`PyExceptionFactory`) — precedent, but NOT the mechanism `io` uses

`native/jpype_module/src/main/java/python/lang/PyExceptionFactory.java` is a
static `HashMap<String, Class>` keyed by Python exception class name
(`"KeyError"` → `PyKeyError.class`, etc.), used together with the `__mro__`
walk in `_pyexc_convert` (`jpype/_jbridge.py:1059-1066`).

**Correction from the original draft of this plan (2026-07-10):** this
mechanism exists for exceptions specifically because a Python exception must
become a real, throwable `java.lang.Throwable` instance to propagate through
the JVM's exception machinery — it's not how general Python objects get
wrapped for the reverse bridge. Confirmed by `_jbridge.py:961` —
`_jpype._concrete[BaseException] = _PyExc` — exceptions *also* go through
the same generic `_concrete`/`PyJP_probe` path every other type uses for its
proxy-view (`PyBytes`, `PyDict`, etc, `_jbridge.py:954-978`).
`PyExceptionFactory` is the *second*, exception-only mechanism layered on
top for the throw-a-real-Throwable case.

`io` objects are never thrown — they're consumed exactly like any other
`python.lang` object, through the ordinary proxy-wrap path. So `io` does not
need a `PyIOFactory`/`_pyio_convert` analog of `PyExceptionFactory` at all,
and open question 1 from the original draft ("find the native call site that
special-cases exception-typed returns") is moot — there is no such site to
hook for `io`. What `io` needs is exactly what `PyBytes`/`PyDict`/etc.
already have: entries in `_concrete`/`_methods` (or their lazy equivalents,
below) keyed by the real Python type object. That's now routed through the
SPI (`plan/SPI.md`) instead of another hardcoded dict — see "Design:
python.io package" below.

### 2. Protocol dict for abstract capabilities (`_jpype._protocol`)

`_jbridge.py:983-1000` registers abstract "protocol" interfaces
(`PyBuffer`, `PyIterable`, `PyCollection`, …) the same way concrete types are
registered, each backed by a `_PyXxxMethods` dict bound via
`_jpype._methods[_PyXxx] = _PyXxxMethods`. `PyIOBase`/`PyRawIOBase`/
`PyBufferedIOBase`/`PyTextIOBase` should be registered the same way as
abstract protocol interfaces (not instantiable directly, just method-carrying
supertypes), giving concrete classes their shared `read`/`write`/`close`/
`seek`/`tell`/`flush`/`closed`/`__enter__`/`__exit__` surface via normal Java
interface inheritance, exactly how `PyBytes extends PyBuffer, PySequence<PyInt>`
composes today.

### 3. Buffer access for bulk transfer (`PyBuffer`, `PyMemoryView`, `PyBytes`)

`PyBuffer` (`python/lang/PyBuffer.java`) is currently a marker interface.
`PyMemoryView`/`PyBytes`/`PyByteArray` already round-trip through
`Backend.newBytesFromBuffer(PyBuffer)` /
`Backend.newByteArrayFromBuffer(PyBuffer)` (`org/jpype/Backend.java:204,215`).
The `InputStream`/`OutputStream` adapters (below) should reuse this same
byte-transfer path rather than inventing a new one — `RawIOBase.read(size)`
in Python returns a `bytes` object, which is already a `PyBytes` on the Java
side with an established route to a Java `byte[]`.

## Design: python.io package, as the first real SPI provider

`io` is now the dogfood case for `plan/SPI.md`'s `Installer`/`WrapperService`
mechanism, not a hand-wired fifth copy of the `PyExceptionFactory` pattern.
Concretely, this means:

- The Java **interfaces** (`PyIOBase`, `PyRawIOBase`, ...) are still
  hand-written `.java` files, same file tree as originally sketched (below)
  — interfaces are compiled Java, nothing about them is SPI-discovered.
  Per `plan/SPI.md`'s eager/lazy split, their method dicts get registered
  **eagerly** at `_jbridge.initialize()` time via
  `installer.registerInterface(...)`, exactly like the hardcoded
  `_jpype._methods[_PyBytes] = _PyBytesMethods` lines — there's no reason to
  defer this, the interfaces exist regardless of which Python `io` classes
  get touched.
- What's genuinely SPI-driven is the **class-membership mapping** — "this
  Python type (`_io.BytesIO`, `_io.TextIOWrapper`, ...) satisfies these
  interfaces" — which used to be the `PyIOFactory` `HashMap<String, Class>`.
  Instead, a `PyIoWrapperService implements WrapperService` (in
  `org.jpype.io` or alongside the interfaces — TBD, same question as the
  adapter package) provides `getModuleManifest("_io")` (concrete classes)
  and `getModuleManifest("io")` (the public `IOBase`/`RawIOBase`/
  `BufferedIOBase`/`TextIOBase` abstract names — both module names are real,
  see `plan/SPI.md`'s `_io`-vs-`io` gotcha), returning the interface list per
  class name. This is consulted lazily, once per module, from the
  `_cache.__missing__` hook — not scanned at boot.
- Note `io`/`_io` are stdlib and normally already imported by the time
  JPype starts, so eager registration would technically also work for this
  particular module. Routing it through the lazy SPI path anyway is
  deliberate: `io`'s hierarchy (4 abstract bases, 7+ concrete classes, a
  module-name split) is a large enough real case to prove the
  `Installer`/`WrapperService` contract holds before any third party
  (numpy-shaped or otherwise) depends on it.

File layout, unchanged from the original sketch except `PyIOFactory.java` is
replaced by `PyIoWrapperService.java`:

```
package-info.java          -- doc, same tone as python.lang's
PyIOBase.java               (abstract protocol interface: close, closed,
                              fileno, flush, isatty, readable, seek, seekable,
                              tell, truncate, writable, __enter__/__exit__)
PyRawIOBase.java             extends PyIOBase (read, readall, readinto, write)
PyBufferedIOBase.java        extends PyIOBase (read, read1, readinto, write)
PyTextIOBase.java             extends PyIOBase (read, readline, write, encoding,
                              errors, newlines)
PyBytesIO.java                extends PyBufferedIOBase (getvalue, getbuffer)
PyStringIO.java                extends PyTextIOBase (getvalue)
PyFileIO.java                  extends PyRawIOBase (name, mode)
PyBufferedReader.java          extends PyBufferedIOBase
PyBufferedWriter.java          extends PyBufferedIOBase
PyBufferedRandom.java          extends PyBufferedIOBase
PyTextIOWrapper.java           extends PyTextIOBase (line_buffering, write_through)
PyIoWrapperService.java        (implements org.jpype.WrapperService; module
                              manifest for "_io" and "io" -> interface lists;
                              registered via
                              META-INF/services/org.jpype.WrapperService)
```

Method naming follows [[jpype_naming_convention]]: match Python's `io` names
(`read`, `readline`, `readlines`, `write`, `writelines`, `seek`, `tell`,
`flush`, `close`, `closed`, `readable`, `writable`, `seekable`, `truncate`,
`isatty`, `fileno`) rather than inventing Java-style synonyms — this is
already mostly Java-conventional vocabulary since `java.io` and Python `io`
independently converged on similar names, so few renames are expected. Flag
any real conflicts for the deferred naming-check pass rather than deciding
ad hoc.

Bridge wiring in `_jbridge.py`:
- Interface method dicts: same per-interface pattern as
  `_PyBytesMethods`/`_PyMemoryViewMethods`, a `_PyIOBaseMethods`-style dict
  per interface, registered eagerly via the `Installer` at init.
- Class membership: no hardcoded `_concrete[...]` entries for the concrete
  `io` classes — those get populated lazily by the `_cache.__missing__`
  hook consulting `PyIoWrapperService`'s module manifest the first time any
  `_io`/`io` class is probed, per `plan/SPI.md`.
- Factory functions do **not** go on `Backend`/`PyBuiltIn` — resolved
  2026-07-10 (see `plan/SPI.md`'s "Mini-backends" section): an SPI provider
  can't add methods to the shared `Backend`, and core `python.lang` shouldn't
  gain per-provider knowledge either. Instead `python.io.IO` is its own
  mini-backend interface, its own `JProxy` bound to its own `_jbridge.py`
  dict (`_PyIOBackendMethods`), registered via `org.jpype.BackendRegistry`.
  Called as `IO.instance().bytesIO()` / `.bytesIO(PyBuffer initial)` /
  `.stringIO()` / `.stringIO(CharSequence initial)` — implemented for
  `BytesIO`/`StringIO`; `fileIO`/etc. follow the same pattern once added.

## Extraction script: drafting the manifest instead of hand-writing it

Rather than hand-enumerate the `io` class/interface graph the way
`PyExceptionFactory.LOOKUP` was hand-written (108 lines of `HashMap.put`), a
small dev-time Python script introspects the real `io` module and drafts the
`PyIoWrapperService` manifest data (class name → desired interface list,
inheritance graph, `_io`-vs-`io` module split flagged automatically). This
is also the reusable tool a third-party provider (numpy-shaped) would run
against their own package to draft their `WrapperService`, so `io` doubles
as the worked example for that workflow. See implementation order below —
this script is the first concrete artifact, before any Java is written.

## Design: InputStream/OutputStream promotion

This is the genuinely new piece — no existing adapter-to-`java.io.*` pattern
exists in the codebase to copy.

### Shape

Two small adapter classes, not part of the `python.io` interface hierarchy
itself (they wrap it):

```java
// org.jpype.io.PyInputStreamAdapter (package TBD - likely org.jpype.io or python.io)
public final class PyInputStreamAdapter extends java.io.InputStream {
    private final PyRawIOBase /* or PyBufferedIOBase */ source;
    // read() / read(byte[]) / read(byte[], off, len) -> source.read(n),
    // converting the returned PyBytes to Java bytes via the existing
    // PyBuffer round-trip (see #3 above). Empty bytes result => EOF (-1).
    // close() -> source.close().
}

public final class PyOutputStreamAdapter extends java.io.OutputStream {
    private final PyRawIOBase /* or PyBufferedIOBase */ sink;
    // write(int) / write(byte[]) / write(byte[], off, len) -> wrap into a
    // PyBytes/bytes-like via context.bytes(...) and call sink.write(...).
    // flush() -> sink.flush(); close() -> sink.close().
}
```

For text variants, `PyReaderAdapter extends java.io.Reader` /
`PyWriterAdapter extends java.io.Writer` wrapping `PyTextIOBase`, translating
`char[]` against Python `str` via the existing `PyString` conversions
(no new mechanism — `context.str(...)`/`PyString.toString()` already exist).

### Chunked buffering (required, not optional)

Every call an adapter makes into Python crosses the GIL/reverse bridge, which
has real per-call overhead (see [[jpype_gil_reacquire_bug]]). A naive
adapter that forwards each Java call 1:1 — in particular `read()` /
`write(int)`, the single-byte forms `InputStream`/`OutputStream` callers
commonly use — would issue one bridge round-trip per byte. That is
acceptable for a smoke test but not for real usage (e.g. wrapping the
adapter in `BufferedInputStream` doesn't help if the *inner* stream is
already the slow part).

The adapters must therefore buffer internally rather than pass every call
through:

- `PyInputStreamAdapter` keeps an internal `byte[]` buffer (reasonable
  default size, e.g. 8KB, constructor-overridable). `read()` (no args) and
  small `read(byte[], off, len)` calls are served out of the buffer;
  the buffer is refilled with a single bulk `source.read(bufferSize)` call
  into Python only when it's empty. Large `read(byte[], off, len)` requests
  that exceed the remaining buffer should bypass buffering and read directly
  into the caller's array to avoid a pointless extra copy.
- `PyOutputStreamAdapter` keeps an internal `byte[]` buffer and accumulates
  `write(int)`/small `write(byte[], off, len)` calls locally, flushing to a
  single bulk `sink.write(...)` call into Python when the buffer fills, on
  `flush()`, or on `close()`. Large writes that exceed the buffer should
  flush the pending buffer then write the large chunk directly, rather than
  copying it through the buffer first.
- Same principle for `PyReaderAdapter`/`PyWriterAdapter` (buffer `char`s
  instead of `byte`s).
- This buffering is purely a Java-side concern — it must not be confused
  with Python's own internal buffering (`BufferedReader` etc. already do
  their own buffering on the Python side). The two are independent: Java-side
  buffering amortizes the cost of *crossing the bridge*, Python-side
  buffering amortizes the cost of *syscalls*. Wrapping a `PyFileIO` (raw,
  unbuffered on the Python side) is the case where Java-side buffering
  matters most.
- Test coverage for this (see Testing plan below) should include an
  assertion on bridge-call count, not just correctness — e.g. instrument or
  mock the underlying `read`/`write` call to verify that N single-byte
  `InputStream.read()` calls result in O(N / bufferSize) bridge calls, not
  O(N). Correctness alone won't catch a regression back to 1:1 forwarding.

### Promotion entry point

Add default methods directly on the `PyIOBase`/`PyRawIOBase`/`PyBufferedIOBase`
interfaces (Java default methods, consistent with how `@Bypass default`
methods are used throughout `python.lang`, e.g. `PyBytes.get`/`size`):

```java
default InputStream asInputStream() { return new PyInputStreamAdapter(this); }
default OutputStream asOutputStream() { return new PyOutputStreamAdapter(this); }
```

on `PyRawIOBase`/`PyBufferedIOBase`, and `asReader()`/`asWriter()` on
`PyTextIOBase`. `asInputStream()` on a write-only stream (or vice versa)
should raise `UnsupportedOperationException` at call time by checking
`readable()`/`writable()` first, not fail silently on first read/write.

### GIL / threading concerns

Every adapter method that touches Python (`read`, `write`, `flush`, `close`)
crosses the reverse bridge and must go through the same per-call GIL
acquire/release path used elsewhere (`JPPyCallAcquire`), per
[[jpype_gil_reacquire_bug]]. No new GIL-handling code should be needed — the
adapters just call ordinary interface methods on `this`, which already go
through `builtin().backend.*` — but this must be verified empirically once
implemented, especially if a Java caller wraps the adapter in something like
`BufferedInputStream` that may call `read()` rapidly from a single thread vs.
handing the stream to another thread (the latter is the async pattern that
originally surfaced the GIL leak — worth a dedicated multi-thread stress
test, not just a single-thread smoke test).

### Nullable / EOF argument gotchas

`RawIOBase.read(size=-1)` and friends use `-1`/`None` sentinels in Python for
"read everything." Any Java method taking a boxed `Integer size` with a
fixed-arity call into the bridge is a candidate for
[[jpype_boxed_null_reverse_bridge_gotcha]] — use the established
`_unwrap_optional_int`-style guard in the `_jbridge.py` dispatch functions,
don't rely on `is None` checks against values crossing from Java.

## Testing plan

Follow the `PyTestHarness` pattern (`python/lang/PyStringNGTest.java` etc.):

- `PyBytesIONGTest` / `PyStringIONGTest` — read/write/seek/tell/getvalue round
  trips, closed-stream error behavior.
- `PyFileIONGTest` — real temp-file read/write (use `java.nio.file.Files.
  createTempFile`, clean up in `@AfterMethod`).
- `PyInputStreamAdapterNGTest` / `PyOutputStreamAdapterNGTest` — feed a
  `PyBytesIO` through `asInputStream()`/`asOutputStream()` and verify byte-
  for-byte fidelity against reading the same data directly through the
  `PyRawIOBase` API; test EOF handling (`read()` returns `-1` correctly),
  partial reads/writes with explicit `off`/`len`, and behavior wrapped in
  standard JDK decorators (`BufferedInputStream`, `DataInputStream`) to prove
  the adapters are indistinguishable from a real Java stream to library code.
- `PyReaderAdapterNGTest` / `PyWriterAdapterNGTest` — same shape for text,
  including a non-ASCII (multi-byte UTF-8) round trip to catch any
  encoding-boundary bugs in the `char[]`↔`str` translation.
- A GIL-regression check: run adapter reads from a background Java thread
  (reusing the `PyCallableAsyncNGTest` pattern/pool) to confirm no repeat of
  the startup GIL-leak class of bug.

## Open questions / not yet resolved by this plan

Archiving note (2026-07-11): question 2's package-name choice shipped as
`python.io` (not `org.jpype.io`) — see the real tree,
`native/jpype_module/src/main/java/python/io/`. Questions 3-4 were
resolved implicitly by the shipped implementation rather than by a
written decision; not revisited before archiving since the code has been
green (582+/582+, both Python versions) across every subsequent session
that touched this area, including the `python.collections` work that
used `python.io` as its reference pattern (`plan/archive/Collections.md`,
`plan/SPI_tutorial.md`) without needing to change anything here.

1. Resolved this pass (see "precedent, but NOT the mechanism" above): `io`
   doesn't need an exception-style native conversion hook at all, it rides
   the generic `_concrete`/`PyJP_probe` path via the SPI. No native call
   site to trace.
2. Package name for the adapter classes (`org.jpype.io` vs. `python.io`) —
   leaning `org.jpype.io` since `InputStream`/`OutputStream` are Java-facing
   glue rather than a Python concept, but unconfirmed against existing
   package conventions for similar "glue" code (compare where `ProxyFactory`/
   `ProxyInstance` live under `org.jpype.proxy`, a precedent for putting
   adaptation machinery outside the `python.*` packages).
3. Whether `io.RawIOBase` vs `io.BufferedIOBase` should be the more common
   promotion source in practice (buffered is what `open()` returns by
   default) — adapters should probably prefer wrapping whichever concrete
   type is actually handed in rather than forcing one path, but the two base
   interfaces have different method surfaces (`read1` only on buffered,
   `readall`/`readinto` differ slightly) so the adapter may need two small
   variants or a shared internal helper keyed on which interface the source
   implements.
4. Scope check on `codecs`/`locale`-adjacent encoding edge cases for
   `TextIOWrapper` — likely fine to punt to Python's own handling via
   `context.eval`/direct passthrough rather than reimplementing encoding
   logic in Java, consistent with the package-info.java guidance that type
   restrictions can be worked around via `eval` when a wrapper would be
   burdensome.

## Suggested implementation order

1. Extraction script against the real `io`/`_io` modules — draft the class/
   interface manifest and confirm the `_io`-vs-`io` split empirically before
   writing any Java.
2. `PyIOBase`/`PyRawIOBase`/`PyBufferedIOBase`/`PyTextIOBase` interfaces +
   `_jbridge.py` eager method-dict wiring via the `Installer` (no concrete
   classes yet) — get the protocol layer registered and passing a trivial
   "can call `.closed` on an arbitrary io object" test.
3. `PyBytesIO`/`PyStringIO` concrete classes + `PyIoWrapperService`'s module
   manifest + the `_cache.__missing__` lazy-by-module hook (`plan/SPI.md`)
   — these are in-memory, no filesystem/OS interaction, easiest to get
   fully test-covered first, and the first real exercise of the SPI's lazy
   path end to end.
4. `PyFileIO`/`PyBufferedReader`/`PyBufferedWriter`/`PyBufferedRandom`/
   `PyTextIOWrapper` — real file I/O, `open()` builtin wiring.
5. `asInputStream()`/`asOutputStream()`/`asReader()`/`asWriter()` promotion
   adapters + their dedicated test suite, including the JDK-decorator and
   background-thread GIL checks.
