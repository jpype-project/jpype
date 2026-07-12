# python.datetime: SPI provider for Python's datetime module

## Status (2026-07-11): DONE — PyDate/PyDateTime/PyTimeDelta shipped

Implemented as scoped below: `PyDate` (`year`/`month`/`day`/`weekday`/
`isoWeekday`/`toOrdinal`, plus a pure-Java `toLocalDate()` promotion),
`PyDateTime extends PyDate` (adds `hour`/`minute`/`second`/`microsecond`/
`isAware`/`utcOffsetSeconds`/`timestamp`, plus `toLocalDateTime()` and an
aware-only `toInstant()` promotion that throws `IllegalStateException` on
a naive instance), `PyTimeDelta` (`days`/`seconds`/`microseconds`/
`totalSeconds`, plus a `toDuration()` promotion) — all three
`Comparable` against their own type via a `.compareTo` `.pyspi` entry
computing Python rich-comparison as `(x > other) - (x < other)`.
`PyDatetimeWrapperService` mirrors `PyCollectionsWrapperService`; the
factory interface is named `DateTime` (not `PyDateTime`/`PyDatetime`),
following `python.io.IO`'s plain-name convention rather than
`PyCollections`'s collision-avoidance one, since there's no
`java.time.DateTime` to collide with (user caught this naming
inconsistency mid-implementation). `datetime.date`/`datetime.datetime`/
`datetime.timedelta` all report plain `"datetime"` as `__module__` — no
`io`/`_io`-style split needed here — checked empirically, not assumed.
`datetime.time`/`datetime.tzinfo` were not implemented (lower priority per
this plan's own scoping, skipped as non-blocking).

A second real, non-obvious bug class was found and fixed while building
this (same *kind* of bug as `plan/SPI_tutorial.md`'s `moveToEnd` lesson,
but from the opposite direction — see that file's updated "Two real bugs"
section, now three): the `java.time`-accepting factory convenience
methods (`dateFromLocalDate`, `dateTimeFromLocalDateTime`,
`dateTimeFromInstant`) had to be given distinct names rather than made
overloads of `date`/`dateTime` sharing those names, because JPype's proxy
dispatch for `WrapperService`-backed interfaces routes purely by method
name — a `default` method sharing a registered method's name is
intercepted and handed the *actual* arguments passed (here, a raw
`LocalDate`/`LocalDateTime`/`Instant` object) directly to that name's
`.pyspi` callable, without ever running the Java default body that would
otherwise have decomposed it into primitives first. Confirmed by a
research subagent reading `ProxyInstance.invoke`/`jp_proxy.cpp`'s
`JPProxyIndirectDict::getCallable`, which also found this exact bug
still latent and unfixed in already-shipped code:
`PyDeque.rotate()` (0-arg default calling `rotate(1)`) collided with
`collections.deque.pyspi`'s `".rotate": lambda x, n: x.rotate(n)` (no
default for `n`) — fixed as a drive-by (`lambda x, n=1: x.rotate(n)`),
same fix shape as the original `moveToEnd` bug, confirmed via full suite
still green afterward.

Full suite green on both python3.10 and python3.12 (621/621, up from
596/596 before this plan — 25 new tests across
`PyDateNGTest`/`PyDateTimeNGTest`/`PyTimeDeltaNGTest`).

## Original scoping (superseded by the above, kept for context)

## The problem

There is currently no Java front-end for Python's `datetime` module.
Any Python API that hands back a `datetime.date`/`datetime.datetime`/
`datetime.timedelta` currently crosses into Java as a bare `PyObject`,
with no typed methods and no conversion to the Java standard library's
equivalent types. `datetime` is one of the most commonly used stdlib
modules in real Python code, so this is a real gap for the reverse
bridge's usefulness, not a hypothetical one.

Scoped alongside [[jpype_collections_spi_plan]] (`plan/archive/Collections.md`) as
part of proving the `WrapperService` SPI mechanism generalizes past
`python.io` to more than one third-party-shaped provider without
conflicts. See that plan and `plan/archive/SPI.md` /
[[jpype_spi_installer_status]] for the mechanism itself — this plan only
scopes the `datetime`-specific parts.

## Existing forward-direction integration (do not confuse with this plan)

JPype already has forward-bridge (Python object satisfying a Java method
contract) duck-typing for `datetime`, registered as `JConversion`
customizers in `jpype/protocol.py`: `datetime.datetime` → `java.time.Instant`
(`exact=datetime.datetime`, ~line 124) and `datetime.time`/`date`/`datetime`
→ `java.sql.Time`/`Date`/`Timestamp` (~lines 160-172, for `dbapi2`). This is
a *different* code path — structural, per-call, constructs a fresh Java
object each time via `JPClassHints`/`JPClassHints::matches`, no persistent
identity, and used when a Python value is passed *into* a Java method that
expects a `java.time`/`java.sql` type. It does **not** give Java code a
typed front-end object for a `datetime` value it received *from* Python
(this plan's actual goal, the reverse direction), and this plan's SPI work
doesn't touch or conflict with it. Worth reusing as reference for how
Python↔Java datetime semantics were already resolved (naive vs. aware, unit
precision) rather than re-deriving them from scratch.

## Why SPI, not core

Same reasoning as `Collections.md`: build this as a `WrapperService`
provider (model: `python.io.PyIOWrapperService`,
`native/jpype_module/src/main/java/python/io/PyIOWrapperService.java`),
registered via `module-info.java`'s `provides
org.jpype.WrapperService with ...`, not hand-wired into `_jbridge.py` or
core `python.lang`.

## Scope: three Python `datetime` types

- `datetime.date` → `PyDate` — map read accessors (`year`, `month`,
  `day`) and consider a `toLocalDate()` promotion to `java.time.LocalDate`,
  mirroring how `python.io`'s `PyBufferedIOBase.asInputStream()` promotes
  to a standard Java type rather than only exposing the Python-native
  shape.
- `datetime.datetime` → `PyDateTime` — extends/relates to `PyDate`
  (Python's own `datetime` subclasses `date`); promotion to
  `java.time.LocalDateTime`/`java.time.Instant` (need to decide which,
  given `datetime.datetime` can be naive or timezone-aware — aware
  instances should probably promote to `Instant`/`ZonedDateTime`, naive to
  `LocalDateTime`; check both cases against a real interpreter rather than
  assuming).
- `datetime.timedelta` → `PyTimeDelta` — promotion to `java.time.Duration`
  (straightforward: Python's `timedelta` is already a fixed span, unlike
  the calendar-aware `date`/`datetime` split).

`datetime.time` and `datetime.tzinfo` are lower priority — include only if
trivial once the other three are working, not blocking.

## Design questions to resolve before coding (not decided by this plan)

- Whether `PyDate`/`PyDateTime` should be Java `Comparable`, matching
  Python's own rich comparison support.
- Whether construction should go through the interpreter (Python
  `datetime.date(...)` call) or whether a pure-Java-side construction path
  makes sense for values that never need to touch Python again — check
  what `python.io`/`python.lang` precedent exists for this before
  inventing a new pattern.
- Immutability: Python's `datetime` types are immutable value types: the
  Java front-end should be too (no setters), same spirit as
  `PyBytes`/`PyTuple` in `python.lang`.

## Naming-mangling interaction

Same caveat as `Collections.md`: these are `PyObject`-rooted proxy
interfaces, so `.pyspi` method keys need whatever the current `$`/`.`
mangling convention is at execution time
([[jpype_dispatch_fallback_status]], [[jpype_name_mangling_plan]]) —
verify against a live `.pyspi` file, don't assume it matches this plan's
text.

## Steps (mirror `plan/archive/Collections.md` / `plan/archive/IO.md`)

1. Design the three interfaces in a new `python.datetime` package.
2. Resolve the design questions above against a real Python 3.10+
   interpreter (module/class names, aware-vs-naive `__module__` behavior,
   comparison/immutability shape) before locking the Java API.
3. One `.pyspi` resource per class under
   `native/jpype_module/src/main/resources/python/datetime/spi/`.
4. `PyDatetimeWrapperService` implementing `WrapperService`, registered in
   `module-info.java`.
5. End-user Javadoc at the `plan/archive/Javadoc.md` "Audience 1" bar, plus
   a real `python/datetime/package-info.java`.
6. Tests: one NGTest class (or one per type) constructing through the real
   SPI path, exercising both the Python-native accessors and the
   `java.time` promotion methods.

## Verification

- Full suite green on python3.10 and python3.12.
- Confirm a second SPI provider (`python.io` already live) coexisting with
  this one doesn't produce any registration-order or module-name
  collision — this is part of the "prove more than one provider works
  cleanly" motivation for doing `collections` and `datetime` alongside
  `io`.
</content>
