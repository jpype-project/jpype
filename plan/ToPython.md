# toPython(): give the java.time/java.sql/java.math/java.nio.file customizers the same public reverse-conversion story java.io already has

## Status (2026-07-11): scoped, not started

## The problem

`jpype/_jio.py` established a real, public, documented convention: a Java
value hands back a genuine independent Python object via a `.toPython()`
method, added through a `@_jcustomizer.JImplementationFor(...)`
customizer (`_JWriter`/`_JReader`/`_JOutputStream`/`_JInputStream`, lines
161-192). It's documented in `doc/userguide.rst` (~lines 5229-5298) and
tested in `test/jpypetest/test_streamredirect.py`. This is the *reverse*
direction of the existing `JConversion` customizers in `jpype/protocol.py`
(Python value → Java method argument): Java value → genuine Python value.

`jpype/protocol.py` already has forward `JConversion`s for `pathlib`/
`os.PathLike` → `java.nio.file.Path`/`java.io.File` (lines 64-72),
`datetime.datetime` → `java.time.Instant` (line 124), `datetime.time`/
`date`/`datetime` → `java.sql.Time`/`Date`/`Timestamp` (lines 160-172),
and `decimal.Decimal` → `java.math.BigDecimal` (line 175). But the reverse
direction is inconsistent:

- `java.sql.Date`/`Time`/`Timestamp` and `java.math.BigDecimal` already
  have a reverse conversion — but it's called `_py()`
  (`jpype/protocol.py:135-157`), a private-looking name that is genuinely
  private in practice: used internally by `jpype/dbapi2.py:228` and
  exercised only in `test/jpypetest/test_hints.py` (`testDate`,
  `testTimestamp`, `testTime`, `testBigDecimal`, lines ~412-440). It is
  **not** documented anywhere in `doc/userguide.rst` and no `CHANGELOG.rst`
  entry advertises it as public API, unlike `toPython()`.
- `java.time.Instant` and `java.nio.file.Path`/`java.io.File` have **no**
  reverse conversion at all — only the forward `JConversion`. A Java
  method that returns an `Instant` or a `Path` currently hands back a
  bare Java-backed proxy object with no way to get a real
  `datetime.datetime`/`pathlib.Path` out of it.

User's ask: bring all of these up to the same public, documented
`toPython()` convention `java.io` already has, rather than leaving four
types on an undocumented private `_py()` and three types with no reverse
conversion at all.

## This is a different piece of work than plan/Datetime.md, plan/Decimal.md, plan/Pathlib.md

Those three plans scope the *reverse-bridge SPI* direction: a `WrapperService`-registered
Java front-end type (`PyDecimal`, `PyDate`, `PyPath`) for a Python value
that stays conceptually Python-side and dispatches back into the
interpreter — the case where Java received a value *originating in
Python* and wants ergonomic typed access without leaving Python's control.

This plan is about the opposite direction and a different kind of value:
Java already has a real `java.math.BigDecimal`/`java.time.Instant`/
`java.nio.file.Path`, constructed and owned on the Java side (e.g. handed
back from a Java library call, no Python involvement in its creation), and
Python code wants a genuine, independent, pure-Python
`decimal.Decimal`/`datetime.datetime`/`pathlib.Path` — no residual Java
reference, unlike `_JavaTextIO`'s composed-wrapper shape (which is correct
for streams specifically because they're live, stateful resources; a
`BigDecimal`/`Instant`/`Path` is an immutable value, so its `toPython()`
should produce a real value-copy, matching what `_py()` already does for
`BigDecimal`/`Date`/`Time`/`Timestamp`).

Do not conflate the two — they use the same `JImplementationFor`
mechanism but solve different problems, and both can land independently
in either order.

## Scope

1. **Rename `_py()` → `toPython()`** on the four existing customizers
   (`_JSQLDate`, `_JSQLTime`, `_JDate` (→ `java.sql.Timestamp`),
   `_JBigDecimal` in `jpype/protocol.py:135-157`). Update the two real
   call sites: `jpype/dbapi2.py:228` and the four `test_hints.py` tests
   (`d._py()` → `d.toPython()`). Check whether `_py()` needs to survive as
   a deprecated alias for any external caller before removing it outright
   — grep the wild for `_py()` usage isn't possible, but check
   `CHANGELOG.rst`/`doc/` for any prior public mention first; if none
   exists (expected, given the private-looking name), a clean rename is
   fine, no alias needed.
2. **Add new `toPython()` customizers**:
   - `java.time.Instant` → `datetime.datetime` (UTC-aware; mirror
     `_JInstantConversion`'s epoch-seconds/nanos math in reverse).
   - `java.nio.file.Path` → `pathlib.Path` (via `str(self)` +
     `pathlib.Path(...)`, or `Paths`-side `toString()` — check platform
     separator behavior on a real interpreter before assuming `str()` is
     sufficient, same caveat `plan/Pathlib.md` already flagged for the
     forward direction).
   - `java.io.File` → `pathlib.Path` (via `getPath()`/`getAbsolutePath()`,
     same target type as `Path` for symmetry — a `File` and a `Path` play
     the same role in Python, no need for two different Python-side
     representations).
3. **Document** each in `doc/userguide.rst`, same shape as the existing
   `java.io.Writer.toPython(...)` etc. entries (~lines 5247-5267) — method
   signature, one line on what it returns.
4. **`CHANGELOG.rst`** entry, matching the existing "`toPython()`
   customizer added to `java.io.Writer`/`Reader`/..." entry's phrasing.
5. **Tests**: extend `test_hints.py`'s existing `testDate`/`testTimestamp`/
   `testTime`/`testBigDecimal` to call `.toPython()` instead of `._py()`;
   add `testInstant`/`testPath`/`testFile` alongside them, same
   round-trip shape (`Java value → toPython() → JObject(pythonValue, cls)
   → equals original`).

## Verification

- Full suite green on python3.10 and python3.12.
- `grep -rn "\._py()" jpype/ test/` → zero hits when done.
- Confirm round-trip correctness for at least one non-trivial case per
  type (an `Instant` with sub-second precision, a `Path` with multiple
  segments, a `BigDecimal` with more digits than fit in a `double`) so the
  conversion is verified on real values, not just the trivial case.
</content>
