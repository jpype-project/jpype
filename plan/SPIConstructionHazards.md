# Document the SPI/$-mangled construction hazards for provider authors

## Status (2026-07-11): scoped, not started

## Where this fits

This is **Audience 2** content per `plan/archive/Javadoc.md`'s split (SPI
provider authors implementing `WrapperService`, not end users calling
`IO.using(context)`). It belongs alongside `WrapperService.java`'s
existing Javadoc (`native/jpype_module/src/main/java/org/jpype/
WrapperService.java`) and, for the reverse-bridge manual, in `plan/DOCS.md`'s
Customization chapter next to the `$`-mangled-dispatch row already added
there — but it is a **distinct** piece of content from that row: that row
documents the dispatch *mechanism* (what `$foo` does); this plan documents
the *construction hazards* a provider author hits while actually building
and registering a `$`-method-bearing interface, which is a separate,
sharper-edged topic that deserves its own section so it isn't buried.

Ground truth: `plan/archive/DispatchFallback.md` in full (already read in
full while scoping this plan — don't re-derive from memory, that file has
the complete, verified account) plus `DispatchFallbackNGTest.java` /
`PyAliceBobCharlieDerik.java` (`native/jpype_module/src/test/java/
python/lang/`), 12/12 passing on both python3.10 and python3.12.

## Why this needs dedicated documentation, not just a passing mention

Two real, non-obvious hazards were hit and fixed while building the
`$`-mangled dispatch proof, both about *construction*, not the dispatch
mechanism itself:

1. **The historical trap (now closed, but worth documenting as "why the
   normal route works")**: the obvious-looking construction path —
   register a fixture class into `_jpype._concrete` like a real SPI
   provider, then rely on `context.eval()` plus a Java-side cast — used to
   silently fail for `$`-only interfaces. It always built a
   `JPProxyIndirectDict`, whose attribute-lookup fallback checked the
   *proxy wrapper's own* attributes, never the wrapped Python instance —
   so every `$`-method threw `NoSuchMethodError`. This was fixed by
   patching `JPProxyIndirectDict::getCallable`
   (`native/common/jp_proxy.cpp`) to also try `PyObject_GetAttr` on the
   real wrapped instance on a dict miss. **Current state: the normal SPI
   construction route (`_jpype._concrete` + `context.eval()` + cast) now
   works for `$`-methods with no workaround needed** — this is what
   provider authors should be told to use; the historical trap is context
   for *why* it's safe now, not a warning they need to route around today.
2. **The `@`-cast requirement (still real, still needed today)**: getting
   a `context.eval()` result typed as the target interface requires an
   explicit `targetClass @ pyProxy` cast on the Python side, because
   `Script.eval()`'s generic `PyObject` return type otherwise lets
   probe-based structural matching (`pythonConversion`) win before the
   proxy-aware conversion path (`proxyConversion`) ever runs
   (`jp_pythontype.cpp:34-41` tries them in that fixed order). This is a
   **currently-live** gotcha a provider author will hit, not historical —
   must be documented as an actual requirement, with a worked example, not
   a footnote.

Neither of these is guessable from the method signatures alone — both were
discovered by hitting real failures, and a provider author without this
document will rediscover them the hard way.

## Scope

1. **`WrapperService.java`** (or wherever the SPI construction contract is
   documented — check current state, `plan/archive/Javadoc.md` already
   rewrote this file once) — add a "Constructing instances that use
   `$`-mangled methods" subsection: state plainly that the normal
   `_jpype._concrete` + `context.eval()` + `@`-cast route now works, show
   the cast requirement with a real worked snippet (lift verbatim from
   `DispatchFallbackNGTest`'s `makeFixtureViaConcreteRegistration()`, don't
   hand-write a fresh example that might drift from what's actually
   tested).
2. **Failure-mode catalog** — document the four adversarial cases verified
   in `plan/archive/DispatchFallback.md`, as a table (same shape as that
   plan's own table): a `$`-method whose backing Python callable throws
   propagates as the matching `python.exceptions.Py*` type (verified:
   `ValueError` → `PyValueError`, with the real message intact); a
   `$`-method with no matching Python attribute at all throws
   `java.lang.NoSuchMethodError` (the ordinary miss path, not something
   `$`-specific); a `$`-method whose real attribute returns a value that
   doesn't satisfy the declared return type throws
   `python.exceptions.PyTypeError` ("Return value is not compatible with
   required type."); a `$`-method whose real attribute exists but isn't
   callable throws `python.exceptions.PyTypeError` (`'int' object is not
   callable`, i.e. the real Python `TypeError` message, not a JPype-authored
   one). This table is the single most useful thing a provider author
   troubleshooting a broken `$`-method needs — prioritize writing this
   over prose.
3. **The `Object`-vs-`PyObject`/`PyTuple` return-type hazard** — a
   `$`-method declared to return plain `java.lang.Object` is a hard
   `TypeError` at call time (`jp_proxy.cpp:235`), not a silent degrade,
   because `JPObjectType::findJavaConversion` never includes the
   Python-proxy conversion path the way `JPPythonType::findJavaConversion`
   does (`jp_pythontype.cpp:34-47` vs `jp_objecttype.cpp:34-51`). Document
   this as a hard rule: declare the tightest real `PyObject`-hierarchy
   return type you know the method will produce (`PyTuple`, `PyString`,
   ...), never plain `Object`; `PyObject` itself is safe but loose (always
   matches, proves nothing about the actual returned shape — explain why
   that matters for e.g. a numpy wrapper returning a scalar and getting
   silently typed as bare `PyObject` instead of the specific numeric type
   if the interface author wasn't careful).
4. **Varargs flattening note** — `$charlie(Object... args)`-style varargs
   methods are already handled correctly (`ProxyInstance.invoke` unpacks
   `Proxy.invoke`'s single wrapper array before flattening to Python
   `*args`), but this is exactly the kind of thing that looks broken if a
   provider author doesn't know it's handled — one line confirming varargs
   "just work," pointing at the real regression risk (a 0- or 1-element
   call wouldn't have exposed a broken unpack, so this was specifically
   verified with 2+ elements) so readers trust the claim rather than
   independently re-verifying it.

## Where this content lives (needs a decision, not an assumption)

Two candidate homes, not mutually exclusive:
- **Javadoc** (`WrapperService.java`) — reachable by anyone who ends up on
  that class's generated docs, the natural place for "how do I construct
  this."
- **`doc/userguide.rst`'s Customization chapter**, cross-referenced from
  the `$`-mangled-dispatch row already added to `plan/DOCS.md`'s mirror
  table — reachable by anyone reading the manual top-to-bottom, doesn't
  require already knowing to look at `WrapperService`'s Javadoc.

Recommend both, with the Javadoc version short (pointer + the `@`-cast
requirement + a one-line "see the manual for the full failure-mode
catalog") and the manual version carrying the full failure-mode table —
same split `plan/archive/SPI.md`'s `.pyspi` format documentation used
between `WrapperService.getResources()` (full spec) and `SpiResource.java`
(short internal pointer). Don't duplicate the full table in both places.

## Verification

- Every claim in the failure-mode table must be re-confirmed against
  `DispatchFallbackNGTest` at time of writing (12/12 passing is the
  current state per `plan/archive/DispatchFallback.md`, but re-run it, don't
  trust the historical count if meaningful time has passed) — same "ground
  truth from tests, not memory" discipline as the rest of `plan/DOCS.md`.
- If documented in Javadoc: `mvn javadoc:javadoc` (or equivalent) builds
  clean, no broken `{@link}`s.
- If documented in `doc/userguide.rst`: Sphinx build succeeds with no new
  warnings.
</content>
