# DOCS: the reverse-direction manual (Java calling Python)

## TL;DR

`doc/userguide.rst` (6482 lines) documents Python code using Java
(`import java.lang.String`, JPype types, JVM control, proxies for Python
callbacks into Java). There is no equivalent manual for the new direction:
Java code holding and calling methods directly on Python objects via
`python.lang`/`python.exceptions`/`python.io`/`python.collections`/
`python.datetime` (updated 2026-07-11: all three of the latter have now
landed, see `plan/archive/IO.md`, `plan/archive/Collections.md`, and
`plan/archive/Datetime.md`). `doc/bridge.md`
is the closest existing thing, and it's an internal design note about type
probing, not user-facing documentation — it predates most of the concrete
classes actually being implemented and is now stale as an API reference
(no `PyString`/`PyDict`/async/etc. content).

This plan is a chapter-by-chapter mirror of `userguide.rst`'s structure,
retargeted at the reverse direction, plus a concrete "how do we know the
docs are true" mechanism: use the `NGTest` suite as ground truth, not
memory or guesswork, since that's the only place the current API surface
is verified to actually work (451/451 passing per
[[jpype_reverse_bridge_testing]]).

## Existing manual structure (source of the mirror)

`doc/userguide.rst` top-level chapters, in order:

1. Introduction
2. JPype Types
3. Controlling the JVM
4. Customization
5. Serialization with JPickler
6. Working with NumPy
7. **Calling Python Code from Java** (existing — but this is proxies:
   Python objects implementing Java interfaces so Java can call back into
   Python-defined callbacks. Different feature from `python.lang`.)
8. AWT/Swing
9. Miscellaneous topics (threading, multiprocessing, GUI shells, javadoc,
   autopep8, garbage collection, diagnostics, known limitations, glossary)

Plus separate documents: `quickguide.rst` (tutorial-style walkthrough),
`api.rst` (API reference index), `install.rst`, `develguide.rst`.

## Mirror mapping

| userguide.rst chapter | Java-calling-Python equivalent | Status |
|---|---|---|
| Introduction | Introduction: "JPype the Java-to-Python bridge" — same pitch, reversed; explain this is the newer, not-yet-shipped surface, and how it relates to the existing Python-to-Java direction (same library, opposite call direction, can be used together). **The real differentiator, and the thing the intro should actually lead with** (user's framing, 2026-07-11 — use this, don't water it down to generic "also supports the reverse direction" language): plenty of language bridges expose *some* fixed set of static interop behaviors. What `python.lang`'s SPI + `$`-mangled dispatch does is different in kind, not degree — it mirrors JPype's own long-standing Python-side customizer system (the mechanism that lets a Python class transparently take on Java behaviors) back onto the *Java* side, so a Python object can satisfy an arbitrary hand-written Java interface by directly implementing it, dispatched through the interpreter, not through a fixed pre-generated binding. Framed as "the other language actually implementing your Java interfaces" is close to unprecedented as a language-bridge design — worth stating plainly as the reason this chapter exists, not just documenting it as one more supported operation among many. | not started |
| JPype Types | **python.lang Types** — the `PyObject` hierarchy: concrete types (`PyString`, `PyDict`, `PyList`, `PyInt`, `PyFloat`, `PyBytes`, `PySlice`, `PyRange`, `PySet`/`PyFrozenSet`, `PyTuple`, ...) and protocol interfaces (`PyIterable`, `PyCallable`, `PyMapping`, `PySequence`, `PyBuffer`, ...) — mirrors `doc/bridge.md`'s design intent but as verified, shipped API | not started; `bridge.md` is the design precursor, now stale as reference |
| *(new, no userguide.rst precedent)* | **python.collections Types** — `PyDeque`/`PyOrderedDict`/`PyDefaultDict`/`PyCounter`/`PyChainMap`, the second `WrapperService` SPI provider after `python.io` (`plan/archive/Collections.md`, DONE). Content needed: the `PyCollections.using(context)` factory entry point (mirrors `IO.using(context)`); each type's Python-specific extras (`PyDeque.rotate`/`maxlen`, `PyCounter.mostCommon`/`elements`/`subtract`/`getCount`, `PyOrderedDict.moveToEnd`/`popItem(boolean)`, `PyDefaultDict`'s `default_factory`, `PyChainMap.maps`/`newChild`/`parents`); and — genuinely worth a callout box, not a footnote — the `PyDict` vs. `PyMapping` type-shape decision (`OrderedDict`/`DefaultDict`/`Counter` are real `dict` subclasses and extend `PyDict`; `ChainMap` is a `MutableMapping` but *not* a `dict` subclass and extends the weaker `PyMapping` instead), since a reader reaching for a sixth stdlib type later needs to make the same judgment call correctly. Ground truth: `native/jpype_module/src/test/java/python/collections/*NGTest.java` (all five types, 43 tests). Cross-reference `plan/SPI_tutorial.md` for the underlying mechanism explanation (this chapter should show the *usage*, `SPI_tutorial.md` remains the *provider-authoring* reference, don't merge the two — different audiences, same split as `WrapperService.getResources()`/`SpiResource.java` already established for Audience 1 vs. Audience 2). | not started, ground-truth source is the `python.collections` NGTest suite |
| *(new, no userguide.rst precedent)* | **python.datetime Types** — `PyDate`/`PyDateTime`/`PyTimeDelta`, the third `WrapperService` SPI provider (`plan/archive/Datetime.md`, DONE). Content needed: the `DateTime.using(context)` factory entry point (named `DateTime`, not `PyDateTime`/`PyDatetime`, mirroring `python.io.IO`'s plain-name convention rather than `PyCollections`'s collision-avoidance one, since there's no `java.time.DateTime` to collide with); the `PyDateTime extends PyDate` mirroring of Python's own `datetime` being a `date` subclass; the four `java.time` promotion default methods (`PyDate.toLocalDate()`, `PyDateTime.toLocalDateTime()`/`toInstant()` (aware-only, throws `IllegalStateException` on a naive instance), `PyTimeDelta.toDuration()`) computed entirely Java-side from already-fetched accessor values, no extra Python round trip; and a callout on the naive-vs-aware distinction (`PyDateTime.isAware()`/`utcOffsetSeconds()`, the latter `null` for naive). Worth a callout box on a second concrete instance of the "name-based proxy dispatch, not overload-based" hazard from `plan/SPI_tutorial.md`: the `java.time`-accepting factory convenience methods (`dateFromLocalDate`, `dateTimeFromLocalDateTime`, `dateTimeFromInstant`) are deliberately *not* overloads of `date`/`dateTime` sharing those names, since a shared-name default would get intercepted and handed the raw Java object instead of running its body. Ground truth: `native/jpype_module/src/test/java/python/datetime/*NGTest.java` (all three types, 25 tests). | DONE 2026-07-11, ground-truth source is the `python.datetime` NGTest suite |
| Controlling the JVM | **Controlling the Python interpreter from Java** — starting the embedded interpreter, `PyBuiltIn`/`context`, shutdown, GIL model (this is where [[jpype_gil_reacquire_bug]]'s "every call into Java is a potential GIL release boundary" lesson belongs, translated to user-facing guidance) | not started |
| Customization | **Customization** — how to extend/adapt. SPI (`plan/archive/SPI.md`) has now landed, and so has a second piece that belongs in this same chapter: **`$`-mangled direct dispatch** for `PyObject`-rooted proxies (`plan/archive/NameMangling.md`, `plan/archive/DispatchFallback.md`, both DONE 2026-07-11). This needs real user-facing documentation, not just the design notes those plans left behind — see the new row below. | SPI landed, needs writing; mangled-dispatch sub-section also needs writing |
| *(new, no userguide.rst precedent)* | **`$`-mangled direct dispatch** — a Java interface that extends `python.lang.PyObject` (directly or transitively) may declare a method `$foo(...)` whose call is routed straight through to the real Python object's `foo` attribute, bypassing the interface's own hand-authored dispatch map entirely. Any *non*-`$` method `foo(...)` on such an interface instead becomes a map-only key mangled to `.foo` internally, so it can never collide with a real Python attribute of the same name. This is how `WrapperService` providers (`python.io.PyIOWrapperService` etc.) and any custom `PyObject`-rooted interface get structural, low-effort access to a wrapped Python instance's real attributes without writing out a full `METHODS = {...}` entry for every single one. Content needed: when to reach for `$foo` vs. a normal mapped method; the collision-avoidance rationale (why plain `foo`/`foo` would silently shadow real Python attributes without the `.` mangling); worked example lifted from `PyAliceBobCharlieDerik`/`DispatchFallbackNGTest` (`native/jpype_module/src/test/java/python/lang/`) — alice/bob overload resolution, charlie varargs, derik `PyKwArgs`, and at least one of the adversarial hazard cases (hazel/ghost/ike/wilma) so readers see the real failure modes (`NoSuchMethodError` for a true miss, `PyTypeError` for a return-type/callability mismatch) rather than assuming silent fallback always succeeds. Cross-reference `python.lang.PyCallable`'s existing "calling convention" doc (`plan/archive/Javadoc.md`'s Audience 1 pass) since this is the same calling-convention family. | not started, ground-truth source is `DispatchFallbackNGTest` (12/12 passing) per the same "tests not memory" discipline as the rest of this plan |
| Serialization with JPickler | Not directly applicable yet — no reverse-direction pickling story exists. Flag as an open question, don't invent content. | open question |
| Working with NumPy | Not directly applicable yet — no reverse-direction numpy story exists (buffer promotion in `plan/archive/IO.md`, now DONE, is adjacent but not this). Flag as open question, don't invent content. | open question |
| Calling Python Code from Java (proxies) | Already exists and already documents the *other* Java→Python mechanism (callbacks). Add a short cross-reference note distinguishing "proxies: Python object playing a Java role" from "python.lang: Java code directly manipulating a live Python object" so readers don't confuse the two. | small addition to existing chapter |
| AWT/Swing | Not applicable | n/a |
| Threading / Synchronization | **Async calls and the thread pool** — `PyCallable.callAsync`/`callAsyncWithTimeout`, the 32-thread pool, GIL-per-call model, what's safe to call from a background thread vs. what isn't | not started; this is the one chapter with the most subtle correctness content (GIL) and should be reviewed carefully against [[jpype_gil_reacquire_bug]] before publishing |
| Garbage collection | **Object lifetime** — how `PyObject` proxies relate to CPython refcounting, whether closing/releasing is needed anywhere (e.g. `PyMemoryView.release()`, streams' `close()` — `python.io` landed, see `plan/archive/IO.md`) | not started |
| Known limitations | **Known limitations** — mirror the existing chapter's honesty; enumerate what's NOT implemented yet (updated 2026-07-11: `python.io`, `python.collections`, and `python.datetime` have all now landed — see `plan/archive/IO.md`/`plan/archive/Collections.md`/`plan/archive/Datetime.md` — so drop them from this list; any remaining protocol gaps, plus whatever's still unscoped from `plan/Pathlib.md`/`plan/Decimal.md`/`plan/Re.md`/`plan/Queue.md`, belong here instead) rather than implying full parity with Python. Subinterpreters are now supported (start/register-classes/eval/close, verified with two subinterpreters + main running concurrently with no cross-talk or hangs) but should be documented as *legacy-style* — shared process GIL and object allocator with the main interpreter, not full PEP 684 own-GIL isolation, since `_jpype` is a single-phase-init extension | not started, but cheap and high-value — do this early since the list already exists implicitly across memory files (see [[jpype_subinterpreter_difficulty]], now resolved) |
| Glossary | Extend existing glossary with reverse-direction terms (proxy vs. bridge object, reverse-bridge, `_jbridge.py`, GIL leak terminology) or keep a single shared glossary | not started |

`quickguide.rst`-equivalent: a **reverse quickguide** — "5-minute tour of
calling Python from Java" — is probably higher value early than the full
reference chapters, since it's what a new reader actually wants first. This
should come before the deep chapters, not after.

`api.rst`-equivalent: mostly free — this is Javadoc's job
(`python.lang`/`python.io`/`python.exceptions` package Javadoc, generated
from the doc comments already present on every interface method, e.g.
`PyBytes.decode`'s existing encoding/errors doc block). The Sphinx `api.rst`
mirror should mostly be "link to the generated Javadoc," not hand-written
prose duplicating it.

## Ground truth: source content from tests, not memory

The single biggest risk for this doc effort is writing prose that describes
an API that used to be true, or was never quite true, or drifted during
implementation (this session alone fixed several real bugs — boxed-null
slices, `PyDictItems.add()`, `removePrefix`/`removeSuffix` — that a
doc-writer working from intent rather than the actual test suite would have
gotten wrong). Every code example in the manual should be lifted from or
directly validated against an existing passing `NGTest` file, not
freehand-written:

- `PyStringNGTest.java`, `PyDictItemsNGTest.java`, `PySliceNGTest.java`,
  `PyCallableAsyncNGTest.java`, etc. under
  `native/jpype_module/src/test/java/python/lang/` are the closest thing to
  a spec this project has (451/451 passing, per
  [[jpype_reverse_bridge_testing]]).
- Where `quickguide.rst` already does this for the forward direction, check
  whether `doc/quickguide.py` (a runnable companion script, given the `.py`
  file sitting next to `quickguide.rst`) is executed as part of doc build or
  CI — if so, the reverse quickguide should get the same treatment (a
  runnable Java companion, or at minimum examples copy-pasted verbatim from
  passing test methods) so the manual can't silently drift from working code
  again.

## Open questions

1. Format: match `userguide.rst`'s Sphinx/RST + `.. code-block::` style, or
   is Markdown acceptable (per `doc/bridge.md` precedent)? Given this ships
   as one Sphinx-built manual (`doc/conf.py`, `index.rst` toctree), RST is
   almost certainly required for anything meant to appear in the built
   manual; Markdown is fine for internal design notes like `bridge.md` but
   that's a different audience.
2. Does this get its own top-level chapter in `doc/index.rst`'s toctree, or
   folded into `userguide.rst` as new top-level sections? A new file (e.g.
   `doc/reverseguide.rst`) is probably cleaner given `userguide.rst` is
   already 6482 lines, but the "Calling Python Code from Java" chapter
   already sits inside `userguide.rst`, so there's precedent either way —
   needs a decision, not an assumption.
3. Should the manual document `python.lang` as "shipped/stable" or caveat it
   as pre-release? Per [[jpype_naming_convention]], a naming-check pass is
   still planned before shipping — publishing a manual with method names
   that then get renamed would immediately go stale. Likely answer: wait
   for the naming pass, or explicitly mark the chapter as provisional/
   pre-1.0 API subject to change.
4. `doc/bridge.md` disposition — supersede it with the new "python.lang
   Types" chapter once written (its content is now partially wrong: e.g. it
   proposes probing mechanisms and a class list that doesn't fully match
   what got implemented), or keep it as-is as an internal design record and
   layer the new user-facing chapter on top without touching it. Leaning
   toward: leave `bridge.md` alone as a historical design note (like
   `plan/SPI.md` will become once SPI ships), don't retrofit it into a
   manual chapter.

## Suggested order

1. Cheap, high-value, low-risk-of-going-stale-fast: **Known limitations**
   chapter — mostly transcribing what's already in memory files.
2. **Reverse quickguide** — the 5-minute tour, examples pulled from
   `PyStringNGTest`/`PyDictNGTest`/etc.
3. **python.lang Types** chapter — the meaty reference chapter, supersedes
   `bridge.md`'s user-facing role. **python.collections Types** (above)
   is a natural companion piece written right after this one — smaller,
   and a good worked example of the `PyDict`/`PyMapping` type-shape
   decision that a reader will need again for any future stdlib chapter.
4. **Controlling the Python interpreter from Java** + **Async calls and the
   thread pool** — the two chapters with real correctness content (GIL),
   write these carefully and cross-check against
   [[jpype_gil_reacquire_bug]] line by line.
5. Everything else (Object lifetime / Customization / Glossary) once the
   above exist. SPI, `python.io`, and `python.collections` have all now
   landed (two independent worked examples of the same SPI mechanism,
   see `plan/SPI_tutorial.md`), so the Customization chapter (including
   the new `$`-mangled direct dispatch sub-section) has real content to
   write against and no longer needs to wait. The new
   **python.collections Types** row (above, in the mapping table) should
   land alongside **python.lang Types** in step 3, not deferred to this
   step — it's a reference chapter like `python.lang Types`, not a
   Customization sub-topic.
6. Revisit "Serialization" and "NumPy" chapters only if/when a reverse-
   direction story for either actually gets built — don't write placeholder
   chapters for features that don't exist.
