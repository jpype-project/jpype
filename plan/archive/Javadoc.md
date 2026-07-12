# Javadoc rewrite: strictly user-facing, no plan/ references

## Status (2026-07-11): DONE

Verified 2026-07-11: every item below was already implemented in the tree
(landed inside commit e5a93a46, "Multi-phase init, python.io completion,
cross-interpreter safety, and dispatch mangling" — TODO.md just hadn't been
updated to reflect it). Re-checked file by file against this plan's
checklist:
- `python/io/package-info.java`, `python/io/IO.java` — Audience 1 rewrite
  present, no SPI/internals language.
- `python/lang` doc-coverage gaps (`PyDictItemsIterator`, `PyFunction`,
  `PyIndex`, `PyLambda`, `PyMutableSet`) — all have real one-line docs.
- `"attributes fromMap"` typo — zero hits repo-wide.
- `PyCallable.java`, `PyIterator.java`, `PyAbstractSet.java` — internal-
  mechanics/FIXME framing already reworded to user-facing behavior.
- `WrapperService.java`, `Installer.java`, `SpiLoader.java`,
  `SpiResource.java`, `PyIOWrapperService.java` — Audience 2 rewrite
  present: package name fixed, dangling sentences fixed, `.pyspi` format
  inlined on `getResources()`, `plan/` references stripped.
- `PyBufferedIOBase.java`, `PyIOInputStream.java` — describe actual
  buffered behavior, no `plan/IO.md` pointer.
- `grep -rn "plan/SPI.md\|plan/IO.md" native/jpype_module/src/main/java/`
  → zero hits.
- `mvn -q -f native/jpype_module/pom.xml compile -Dpython.executable=python3.10`
  → clean compile, no syntax breakage from doc edits.

Original scoping below, kept for reference.

## Original status (2026-07-10): scoped, not started

User's rule: Javadoc is user facing, not private developer notes — "we
can't refer to plan/", "it must be strictly how does the user operate on
it." Grounded in a repo-wide Explore survey this session (15 confirmed
`plan/SPI.md`/`plan/IO.md` Javadoc references, a full `python.lang`/
`python.io` doc-coverage audit, and the three `package-info.java` files
read in full). Should land **after** `plan/Naming.md`'s rename pass, so
this pass documents final names, not names about to change (in particular
`PyIoWrapperService` → `PyIOWrapperService`, `PyKeyArgs` → `PyKwArgs`).

Organized by audience, since "user facing" means something different
depending on who reads the doc:

- **Audience 1 — `python.io`/`python.lang` end users**: someone calling
  `IO.using(context).bytesIO()`. Never writes a `WrapperService`, never
  sees a `.pyspi` file.
- **Audience 2 — SPI provider authors**: someone implementing
  `WrapperService` to expose their own package's types (the numpy-shaped
  motivating example). Their real contract is `WrapperService`'s methods
  and the `.pyspi` file format — genuinely needs to be documented
  somewhere public, just not conflated with Audience 1's docs.

## Audience 1 — end-user docs

- **`python/io/package-info.java`** — full rewrite. Currently an
  SPI-internals explainer (mentions `_jpype._cache.__missing__`, links
  `plan/SPI.md`/`plan/IO.md`, describes eager/lazy registration
  mechanics). Replace with a "how to use python.io" guide mirroring
  `python/lang/package-info.java`'s existing tone (already good — no
  `plan/` refs, describes collection-interface mapping and the `eval`
  escape hatch from a caller's perspective): show `IO.using(context)`,
  the concrete classes (`PyBytesIO`, `PyStringIO`, ...), and
  `asInputStream()`/`asReader()` promotion. No mention of `.pyspi`,
  `WrapperService`, or lazy/eager internals — that content moves to
  Audience 2's docs below, not deleted.
- **`python/io/IO.java`** — rewrite the class Javadoc (currently narrates
  `NativeContext`/per-interpreter registration internals) to a plain
  "construct `python.io` objects via `IO.using(context)`" usage doc with
  the actual factory methods listed.
- **`python/lang` doc-coverage gaps** — add real one-line class docs
  (currently `@author`-only or empty) to the public classes: `PyDictItemsIterator`,
  `PyFunction`, `PyIndex`, `PyLambda`, `PyMutableSet`. Pattern to match:
  `PyBytesIO.java`/`PyStringIO.java`'s existing concise style ("Create
  instances via ...", one sentence on what the type represents).
  (`PyExceptionFactory`/`Utility` are package-private — skip, not part of
  the public surface.)
- **Repeated typo**: `"attributes fromMap a Python object"` across
  `PyAttributes.java`, `PyDictItems.java`, `PyDictKeySet.java`,
  `PyDictValues.java`, `PyMappingEntrySetIterator.java` — evidently a bad
  find/replace artifact, fix in all five.
- **`PyCallable.java`** — reword "To allow for method overloading, the
  entry point for calls must remain private" (internal Java-mechanics
  rationale) to describe the user-visible calling convention instead
  (what a caller actually does to invoke with positional/keyword args).
- **`PyIterator.java`** — reword "This is a private class used under the
  hood" (on a `public class`) and the Python/Java-iterator-philosophy
  rationale paragraph to a plain usage description of what iterating over
  this type does from the caller's side.
- **`PyAbstractSet.java`** — reword "Python uses operators for many set
  operations, which are not yet included in this protocol. This is marked
  as a FIXME in the implementation" to a plain factual capability
  statement (what operations ARE supported), no FIXME/TODO/"not yet"
  project-status framing.

## Audience 2 — SPI provider-facing contract

- **`WrapperService.java`** —
  - Fix the stale package name in its own class-doc usage example
    (`org.jpype.bridge.WrapperService` → `org.jpype.WrapperService`; the
    `bridge` package doesn't exist — same root cause as `plan/Naming.md`
    item 3, fixed here since it's inside a Javadoc block).
  - Fix the two broken/trailing-off sentences: `getModuleNames()`'s doc
    currently reads "A list of fully qualified Python module names this"
    (dangling) / "One Wrapper service can" (trails off, no completion);
    `getVersion()`'s doc says "Get the Python module this binding was
    targeting" but the method returns a version string, not a module —
    description doesn't match the return value.
  - Inline the full `.pyspi` header/body format directly into
    `getResources()`'s doc. Currently the best version of this content
    lives on the package-private `SpiResource.java` (undiscoverable via
    public Javadoc since the class isn't public) — move/duplicate the
    format spec here, keep the same 3 worked examples `SpiResource.java`
    already has (eager class registration, lazy class registration,
    mini-backend registration), and reference `python.io.PyIOWrapperService`
    (post-rename) as the worked real-world example, same as today.
  - Strip the `plan/SPI.md` reference from the `getResources()` doc
    (currently `"see {@code SpiResource} and {@code plan/SPI.md}"`) now
    that the format is inlined rather than pointed-at.
- **`Installer.java`** — reclassify as internal in tone (a provider never
  constructs or calls this type directly; it's jpype's own SPI plumbing,
  invoked automatically once `WrapperService.getResources()` runs) — keep
  it technical but strip the `plan/SPI.md` references (3 hits: class doc,
  `registerClass`'s param doc, `registerBackend`'s doc) and the "Every hook
  the `python.io` first cut hand-wired directly in `_jbridge.py`... becomes
  one of these two calls instead" implementation-history narrative. State
  the contract plainly: what `registerClass`/`registerLazyClass`/
  `registerBackend` each do and when they're invoked, without prose about
  why the design evolved to this shape.
- **`SpiLoader.java`** —
  - `load(Installer)` currently has **zero** Javadoc (it's `public
    static`, technically callable, though in practice only invoked by
    jpype at startup via `MainInterpreter.setInstaller`) — add a short
    factual doc describing what it does and when it's called.
  - `listPyspiResources(Class<?>, String)` keeps its existing
    provider-facing doc (this is the real public helper a
    `WrapperService.getResources()` implementation is meant to call) —
    strip the `plan/SPI.md` parenthetical, keep the jar-vs-exploded-
    directory behavior description since that's genuinely useful to a
    caller.
  - Trim the class-level doc (currently documents `load()`'s internals
    even though it decorates the whole class) down to a short summary,
    strip its `plan/SPI.md` reference.
- **`SpiResource.java`** — since its `.pyspi` format documentation moves
  to `WrapperService.getResources()` (above), trim this class's own doc to
  a short internal note (it's package-private, genuinely not part of any
  public contract) rather than duplicating the full format spec in two
  places. Strip its `plan/SPI.md` reference.
- **`python/io/PyIOWrapperService.java`** (post-rename) — strip "this is
  the first real, non-hardcoded WrapperService" narrative and "deferred to
  the next implementation step in plan/IO.md" roadmap language (2 `plan/`
  hits here); state plainly which `io`/`_io` classes are currently
  exposed and point to `WrapperService`'s doc for the general mechanism
  rather than re-explaining it.
- **`PyBufferedIOBase.java`**'s `asInputStream()`/`asOutputStream()` doc
  — currently says "not tuned for throughput; see plan/IO.md" (2 hits) —
  replace with a plain factual note about the current adapters' behavior
  (buffered or not, per whatever `plan/IO.md`'s IO-completeness pass has
  landed by the time this runs) without pointing at the plan doc.
- **`PyIOInputStream.java`** — one `plan/IO.md` reference in its class doc
  — same treatment, describe actual behavior instead of pointing at the
  plan.

## Mechanical replacement, both audiences

Every literal `plan/SPI.md`/`plan/IO.md` Javadoc reference (15 confirmed
hits from the survey, file:line list already in hand) gets replaced with
either (a) the actual content inlined if it's a provider-facing contract
detail (the `.pyspi` format move above is the main instance of this), or
(b) deleted if it was internal implementation rationale not needed by any
real caller (most of the `Installer.java`/`PyIOWrapperService.java`
history-narrative hits).

## Verification

- `mvn -q test -Dpython.executable=python3.10` — full suite, expect
  unchanged pass count (doc-only changes, but compile the whole tree to
  catch any accidental syntax breakage in edited `.java` files).
- `grep -rn "plan/SPI.md\|plan/IO.md" native/jpype_module/src/main/java/`
  — expect zero hits when done.
- Spot-check rendered Javadoc: `mvn javadoc:javadoc` (or equivalent) on
  `WrapperService`, `Installer`, `python.io` package, `python.lang`
  package — confirm no broken `{@link}`/`{@code}` references from the
  edits (renamed classes, moved content) and that the package-level docs
  read coherently start-to-finish as a "how do I use this" guide.
