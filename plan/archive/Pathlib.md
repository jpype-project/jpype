# python.pathlib: SPI provider for Python's pathlib module

## Status (2026-07-11): DONE — PyPath shipped

Implemented as scoped below: `PyPath` (name/stem/suffix/suffixes/parts/
parent/anchor/drive/root/isAbsolute/asPosix/join/withName/withSuffix/
matches/isRelativeTo/relativeTo/compareTo, plus the filesystem predicates
exists/isFile/isDirectory/isSymlink), factory `Pathlib.using(context).path(...)`
/`pathFromFile(...)`/`pathFromNioPath(...)`, promotion defaults
`toNioPath()`/`toFile()`. Registered against both `pathlib.PosixPath` and
`pathlib.WindowsPath` (`WindowsPath` lazy, since it's never actually
instantiated off Windows) — both report `__module__ == "pathlib"`, no
`io`-style module split needed. Scope deliberately excludes the rest of
`Path`'s I/O surface (`open`/`read_text`/`mkdir`/`unlink`/`resolve`/...);
left as a follow-up rather than folded in here — see the pure-path-vs-I/O
discussion below, resolved in favor of pure-path plus the four cheap
predicates.

Found and fixed two proxy-dispatch argument-marshaling gotchas while
building this, worth remembering for the next SPI plan:

1. **Varargs are flattened, not array-wrapped, when crossing the proxy
   boundary.** A Java `String... more` parameter does not arrive in the
   `.pyspi` lambda as a single Python list/tuple argument — it arrives
   spread across positional arguments (and if the caller passed zero
   varargs elements, that trailing argument is dropped entirely, not
   passed as an empty array). The `.pyspi` function must itself declare
   `*more` to collect them (`def _path(first, *more): ...`), not
   `def _path(first, more): ...`. Confirmed empirically: the fixed-arity
   form either raised `missing 1 required positional argument` (zero
   varargs) or silently iterated the single varargs string
   character-by-character (one varargs element, since Python happily
   iterates a bare `str`).
2. **A bare Java `String` argument crossing into a `.pyspi` lambda is not
   automatically a Python `str`** — passing it straight into a stdlib call
   expecting a real `str` (e.g. `Path.match(pattern)`, which calls
   `sys.intern()` internally) raises `TypeError: intern() argument must be
   str, not java.lang.String`. Every existing `.pyspi` in the tree that
   takes a string-shaped argument already wraps it in `str(...)` before
   using it (see `_date`/`_date_time` in `datetime.date.pyspi`) — this
   plan initially missed that convention for two single-string-argument
   methods (`withName`/`matches`) and had to add the `str(...)` wrap
   after a real test failure surfaced it. Multi-value varargs args were
   already being wrapped element-by-element (`str(s) for s in segments`)
   and didn't hit this.

Full suite green on python3.10 and python3.12 (`mvn -o test
-Dpython.executable=python3.1{0,2}`), 14/14 new `PyPathNGTest` tests
passing on both.

## The problem

No Java front-end exists for `pathlib.Path`. Any Python API that returns
a `Path` (increasingly common in modern Python code, which prefers
`pathlib` over raw string paths) currently crosses into Java as a bare
`PyObject`. `Path` maps unusually cleanly onto `java.nio.file.Path`, so
this is a comparatively low-effort, high-clarity addition.

Scoped alongside `plan/archive/Collections.md`, `plan/Datetime.md`, and
`plan/Decimal.md` as one of the "prove the SPI mechanism generalizes past
`python.io`" candidates. See `plan/archive/SPI.md` /
[[jpype_spi_installer_status]] for the mechanism itself.

## Existing forward-direction integration (do not confuse with this plan)

JPype already has forward-bridge (Python object satisfying a Java method
contract) duck-typing for paths, registered in `jpype/protocol.py`: a
structural `SupportsPath` protocol (anything with `__fspath__`, so not
`pathlib.Path`-specific — any `os.PathLike`) converts to
`java.nio.file.Path` via `Paths.get(...)` and to `java.io.File` (~lines
59-72). This is what makes `Files.newBufferedReader(somePythonPath)` work
today. It's a different code path — structural, per-call, constructs a
fresh Java object each time via `JPClassHints`, no persistent identity —
and fires when a Python path is passed *into* a Java method. It does
**not** give Java code a typed front-end object for a `Path` value
received *from* Python (this plan's actual goal), and doesn't conflict
with this plan's SPI work. Worth reusing as a reference for the
`__fspath__`/separator-handling details already resolved there rather than
re-deriving them.

## Why SPI, not core

Same reasoning as the others: `WrapperService` provider (model:
`python.io.PyIOWrapperService`), registered via `module-info.java`, not
hand-wired into `_jbridge.py` or core `python.lang`.

## Scope

- `pathlib.Path` (which on every real platform is actually
  `PosixPath`/`WindowsPath`, the concrete subclasses `pathlib.Path()`
  dispatches to) → `PyPath`. Check `__module__`/`__class__.__name__` on a
  real interpreter on at least one platform before assuming `Path` itself
  is what needs registering versus its concrete platform subclasses — this
  is exactly the "friendly vs. real module name" trap `python.io` already
  had to handle for `io`/`_io` (see `PyIOWrapperService`'s Javadoc), likely
  recurring here for `pathlib`/`PosixPath`/`WindowsPath`.

## Design questions to resolve before coding

- Promotion to `java.nio.file.Path` — the obvious high-value method
  (`toPath()` or similar), given `java.nio.file.Path`/`Files` is the
  natural Java-side equivalent for the same problem `pathlib` solves.
  Confirm string round-trip behavior between the two on a real
  interpreter, particularly around platform separator differences if this
  ever needs to work cross-platform (Windows `WindowsPath` vs Java's
  platform-`FileSystem`-dependent `Path`).
- Method surface: `pathlib.Path` supports both pure-path operations
  (`name`, `parent`, `suffix`, `/` joining) and I/O operations (`exists`,
  `is_file`, `read_text`, `open`) that call into the OS. Decide whether
  this first cut covers both or starts with the pure-path subset only —
  the I/O half has real interop value with `python.io`'s existing
  `PyFileIO`/stream types (a `Path.open()` result could plausibly return
  through the `python.io` interfaces already built), which is worth
  checking for reuse before designing a separate path.
- Immutability: `Path` is immutable in Python; `PyPath` should be too.
- The `/` join operator: likely needs a named Java method
  (`resolve(...)`/`join(...)`) rather than operator overloading, same
  spirit as `PyAbstractSet`'s treatment of Python's set-algebra operators.

## Naming-mangling interaction

Same caveat as the other plans: `PyObject`-rooted proxy interface, so
`.pyspi` method keys need whatever the current `$`/`.` mangling convention
is at execution time — verify against a live `.pyspi` file.

## Steps (mirror `plan/archive/Collections.md` / `plan/archive/IO.md`)

1. Resolve the `Path`/`PosixPath`/`WindowsPath` module-name question
   against a real interpreter first — this determines how many `.pyspi`
   files are actually needed.
2. Design `PyPath`'s method surface, deciding the pure-path-vs-I/O scope
   question above.
3. `.pyspi` resource(s) under
   `native/jpype_module/src/main/resources/python/pathlib/spi/`.
4. `PyPathlibWrapperService` implementing `WrapperService`, registered in
   `module-info.java`.
5. End-user Javadoc at the `plan/archive/Javadoc.md` "Audience 1" bar, plus
   a real `python/pathlib/package-info.java`.
6. Tests: construction, `java.nio.file.Path` round-trip, whichever
   pure-path/I/O methods got scoped in.

## Verification

- Full suite green on python3.10 and python3.12.
- Confirm coexistence with `python.io` and whichever other SPI providers
  have landed by the time this runs — no registration-order or
  module-name collisions.
</content>
