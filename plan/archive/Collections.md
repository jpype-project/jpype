# python.collections: implement as an SPI provider, like python.io

## Status (2026-07-11): DONE — all four types shipped, plus a fifth (ChainMap)

Implemented exactly as scoped below: `PyDeque` (stands alone, not a
`PyList`/`PySequence` — `deque` isn't slice-able), `PyOrderedDict`/
`PyDefaultDict`/`PyCounter` (all extend `PyDict`, reusing its entire `Map`
surface), `PyCollectionsWrapperService` (mirrors `PyIOWrapperService`,
scans its own `spi/` resource dir), `PyCollections` factory (mirrors
`IO.using(context)`), one `.pyspi` per class, real Javadoc, and one
NGTest class per type. `collections.deque().__module__` etc. all report
plain `"collections"` (no `io`/`_io`-style split needed here — checked,
not assumed, per this plan's own instruction).

Two real design bugs found and fixed during verification (not just
wiring gaps — see `plan/SPI_tutorial.md` for the full writeup, now the
canonical reference for the next SPI plan):
1. `OrderedDict.moveToEnd(key)`'s one-arg Java default couldn't reach a
   Java-side default value through proxy dispatch (which routes by
   Python attribute name, not Java signature) — the default has to live
   in the `.pyspi` Python function itself.
2. `Counter`'s Javadoc wrongly assumed inherited `PyDict.get()` already
   returned `0` for a missing key; it doesn't (`dict.get()` never
   consults `__missing__`) — added a dedicated `getCount()` backed by
   `x[key]`.

Extended past the original four-type scope with a fifth type,
**`PyChainMap`** (`collections.ChainMap`), added on user request as a
follow-up in the same session: correctly extends `PyMapping` rather than
`PyDict` (ChainMap is a `MutableMapping`, not a real `dict` subclass —
the plan's own "pick the right Java shape for the real Python ABC"
principle, applied a second time), plus `PyCombinable` for `|`. Building
it surfaced a third bug, this time in core `python.lang` rather than
anything `collections`-specific: `PyMapping.get()`'s default
implementation never caught `KeyError`, contradicting both its own
Javadoc and real Python's `collections.abc.Mapping.get()` mixin
semantics — nothing had exercised that default path before, since every
prior `PyMapping`-family type in the tree was also a real `dict` and
always went through `PyDict.get()`'s separate, correct override instead.
Fixed in `PyMapping.java`; full suite re-verified green on both Python
versions afterward since it's a shared core fix.

Full suite green throughout: python3.10 and python3.12, 0 failures, 0
new skips beyond the 14 pre-existing ones. See `plan/SPI_tutorial.md`
for the process writeup aimed at whoever builds the next SPI provider
(Datetime/Decimal/Pathlib/etc., or `ChainMap`'s stdlib siblings like
`UserDict` if ever justified).

## The problem

`python.collections` (`PyCounter`, `PyDefaultDict`, `PyDeque`,
`PyOrderedDict`) are empty stub classes — `public class` with no body, no
methods, `@author`-only Javadoc. Nothing in the tree constructs one, wires
it to the backend, or registers a `.pyspi` resource for it.
`module-info.java` already `exports python.collections;` with a comment
("Lets users use PyDeque, PyCounter, etc.") that is aspirational, not
current — a user who reaches for `PyDeque` today gets an unusable stub.
Discovered 2026-07-11 while auditing `python.exceptions`/`python.collections`
doc coverage for the alpha review branch; user's call was **don't hand-wire
these into `_jbridge.py` like the early `python.io` cut was** — build them
the same way `python.io` ended up: as a `WrapperService` SPI provider. See
[[jpype_spi_installer_status]] / `plan/archive/SPI.md` for how that
mechanism works and why it replaced hand-written `_jbridge.py` dispatch
dicts.

## Why SPI, not hand-wired

`python.io`'s worked example is `PyIOWrapperService`
(`native/jpype_module/src/main/java/python/io/PyIOWrapperService.java`) plus
one `.pyspi` file per class under
`native/jpype_module/src/main/resources/python/io/spi/`. That's the whole
integration surface — no `_jbridge.py` edits, no core changes.
`WrapperService`'s Javadoc (`native/jpype_module/src/main/java/org/jpype/WrapperService.java`)
documents the full `.pyspi` format and is the reference for provider
authors; this plan should read as "do the `python.io` recipe again for
`collections`," not invent a new mechanism.

## Scope: four Python `collections` types

- `collections.deque` → `PyDeque` — sequence-shaped, model on `PyList`'s
  Java-collection-interface pattern (implements `java.util.Deque<T>` or a
  reasonable subset) rather than `PyCounter`/`PyOrderedDict`'s map shape.
- `collections.OrderedDict` → `PyOrderedDict` — should extend/reuse
  `PyDict` (`native/jpype_module/src/main/java/python/lang/PyDict.java`)
  rather than duplicate its `Map` plumbing; `OrderedDict` is a `dict`
  subclass in Python with the same protocol plus insertion-order guarantees
  Java's `Map` doesn't express directly (may not need extra methods beyond
  `PyDict`'s).
- `collections.defaultdict` → `PyDefaultDict` — also `PyDict`-shaped, plus
  a constructor/factory path for the `default_factory` callable argument
  (no equivalent on plain `PyDict`).
- `collections.Counter` → `PyCounter` — `dict[T, int]`-shaped with Python's
  extra methods (`most_common`, `elements`, `subtract`, `update`-merge
  semantics different from plain dict update) layered on top.

Check whether `.pyspi`'s existing `kind: class` header (module name +
class name) works unmodified for `collections.deque` etc., or whether the
module name reported by `__module__` needs the same "friendly vs. real"
handling `python.io` has for `io`/`_io` — verify against a real Python
`collections.deque().__module__` etc. before assuming.

## Naming-mangling interaction

These four classes are `PyObject`-rooted, so if the `python.lang`
[[jpype_name_mangling_plan]] `$`/`.` dispatch convention applies to
`WrapperService`-registered proxy interfaces the same way it applies to
user-defined `@JImplements` proxies, method keys in the new `.pyspi` files
need the same `.`-prefixed mangling the `python.io` ones already use (see
`_io.BytesIO.pyspi` above: `".getvalue"` not `"getvalue"`). Confirm this
against a current `.pyspi` file before writing the `collections` ones,
don't assume — the convention may have evolved since [[jpype_dispatch_fallback_status]].

## Steps (mirror `plan/archive/IO.md`'s shape)

1. Design each interface's Java method surface (see "Scope" above for the
   shape per type) in `native/jpype_module/src/main/java/python/collections/`,
   replacing the four empty stub bodies.
2. Write one `.pyspi` resource per class under
   `native/jpype_module/src/main/resources/python/collections/spi/`,
   modeled on the `python/io/spi/*.pyspi` files.
3. Add `PyCollectionsWrapperService` (or similar name — check final naming
   conventions in place at execution time) implementing `WrapperService`,
   scanning its own resource directory via
   `SpiLoader.listPyspiResources`, same pattern as `PyIOWrapperService`.
4. Register it: `provides org.jpype.WrapperService with
   python.collections.PyCollectionsWrapperService;` in `module-info.java`
   (alongside the existing `python.io.PyIOWrapperService` line).
5. Real end-user Javadoc on the four classes and a real
   `python/collections/package-info.java` (current one is a bare `//`
   comment, not even a Javadoc block) — same "Audience 1" bar
   `plan/archive/Javadoc.md` set for `python.lang`/`python.io`.
6. Tests: one NGTest class per type (or one combined
   `PyCollectionsNGTest`), same shape as `python.io`'s per-adapter tests —
   construct via the real SPI path (not a hand-built proxy), exercise the
   Java-collection-interface methods plus each type's Python-specific
   extras (`most_common`, `default_factory`, etc.).

## Verification

- Full suite green on python3.10 and python3.12 (match the bar every other
  closed plan/ item in this repo has hit).
- `PyDeque`/`PyCounter`/`PyDefaultDict`/`PyOrderedDict` genuinely
  constructible and usable end-to-end through the public API (`IO.using`
  pattern equivalent — whatever factory shape this ends up needing), not
  just compiling.
</content>
