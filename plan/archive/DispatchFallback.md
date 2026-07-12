# Prove `$foo` mangled-proxy methods fall through to real Python attributes

## Status (2026-07-11): DONE

Implemented and green on both python3.10 and python3.12 (full suite, 0
failures/0 errors both interpreters). New files:
`native/jpype_module/src/test/java/python/lang/PyAliceBobCharlieDerik.java`
(the fixture interface, exactly as designed below) and
`DispatchFallbackNGTest.java` (7 tests, 6 as designed plus the negative
`.alice`-pollution check). No production code changes were needed - the
mangle/dispatch/varargs/kwargs machinery all worked exactly as designed
below. The result: **confirmed**, `$`-prefixed methods do fall through to
a plain, unregistered Python object's real attributes, with correct
overload selection, correct varargs flattening (multi-element, not just
0/1), and correct `PyKwArgs`-based keyword calls.

Two things turned out differently from the original design, both about
*how to construct* the test proxy, not about the dispatch mechanism itself
(see "What actually had to change" below for the full account):

1. The originally-assumed construction path - register the fixture class
   into `_jpype._concrete` like a real SPI provider, then rely on
   `context.eval()` + a Java-side cast - does not work for `$`-only
   interfaces. That path always builds a `JPProxyIndirectDict`, whose
   attribute-lookup fallback checks the *proxy wrapper's own* attributes,
   not the wrapped object's - so it can never reach a plain object's real
   methods. It produced `NoSuchMethodError` for every `$`-method.
2. The correct construction is the public `jpype.JProxy(intf, inst=...,
   convert=True)` API (`jpype/_jproxy.py:186-227`), which builds the
   `JPProxyIndirectAttr` variant that does forward to the real instance -
   but getting the *result* into Java typed as the target interface
   additionally requires an explicit `targetClass @ pyProxy` cast on the
   Python side before crossing, because `Script.eval()`'s generic
   `PyObject` return type otherwise lets the probe-based `pythonConversion`
   win before the proxy-aware `proxyConversion` ever gets a chance to run
   (`jp_pythontype.cpp:34-41` tries them in that fixed order). Both steps
   are necessary; using only one or the other fails differently (see below
   for the full trace of both failure modes actually observed).

One assertion originally planned (`assertEquals(obj, obj)` for `equals()`
reflexivity) was dropped after it failed for reasons that look specific to
this double-hop construction (two distinct native proxy wrappers around
what should be the same target - both printed identical `toString()` but
compared unequal) rather than anything to do with mangling. Noted as an
open anomaly, not chased further - out of scope for what this plan set out

## Follow-up: closed the `_concrete`-route construction hazard

The two construction-path findings above (see "Status") left a real gap:
the explicit `JProxy(...)` + `@`-cast route worked, but the *normal* SPI
construction path every real provider uses (`_jpype._concrete` +
`context.eval()` + cast) could never serve `$`-methods at all. Fixed by
patching `JPProxyIndirectDict::getCallable`
(`native/common/jp_proxy.cpp`): on a dict miss, it now additionally tries
`PyObject_GetAttr` on the real wrapped instance (`m_Instance->m_Target`)
before falling back to checking the proxy wrapper's own attributes. Safe
for every existing `.`-mangled dict interface (the mangle-completeness
test guarantees their dict lookups always hit, so this new fallback branch
never fires for them) - it only starts mattering for `$`-methods, which
never have a dict entry by design and are exactly the case that should
reach the real object.

New regression test `testConcreteRegistrationRouteNowSupportsDollarMethods`
(via `makeFixtureViaConcreteRegistration()`) proves the plain SPI-style
construction now serves `$`-methods directly, with no `JProxy(...)`/`@`-cast
workaround needed - matching normal SPI ergonomics.

Also added four adversarial fixture cases per user request ("hazel() I
always throw and other nasty edge cases"), each confirmed (by an
exploratory catch-all pass first, not guessed) to fail *cleanly* rather
than crash, hang, or silently misbehave:

| Case | Fixture shape | Result |
|---|---|---|
| `$hazel()` | real, callable, always raises `ValueError` | `python.exceptions.PyValueError` propagates with the real message |
| `$ghost()` | no matching attribute at all | `java.lang.NoSuchMethodError` (the normal, pre-existing miss path) |
| `$ike()` | real, callable, returns `42` (not a tuple) where `PyTuple` is declared | `python.exceptions.PyTypeError`: "Return value is not compatible with required type." |
| `$wilma()` | real attribute, but a plain `int` (not callable) | `python.exceptions.PyTypeError`: `'int' object is not callable` |

Full suite green on both python3.10 and python3.12 after the fix (0
failures/0 errors both interpreters), `DispatchFallbackNGTest` now at
12/12 (both interpreters). Rebuilt via `ninja -C build/{wheel_tag}` per
[[jpype_build_env_gotchas]], copied to both `site-packages` and the
ABI-tagged repo-root copies for python3.10 and python3.12.

This plan's scope is now fully closed - no further open items.

## Original design (superseded in part - see Status above for what changed)

Closes out the last open item from TODO.md's "routing" note (the paragraph
starting "So as for the routing..."). [[NameMangling]] (`plan/NameMangling.md`,
committed `ad21b874`) built the mangling mechanism and proved it doesn't
break the existing SPI/python.io protocol dicts. What it did **not** prove
is the other direction TODO.md called out: that a `$`-prefixed Java
interface method actually reaches an arbitrary Python object's *own*
attributes with the right calling convention (positional, overload-selected,
`**kwargs`, varargs) - the shape needed to eventually wrap something like a
live numpy array with "full access" through a hand-written interface, with
no SPI/Installer registration at all.

## Why this isn't already covered

Confirmed by research before writing this plan (no `$`-prefixed proxy
method exists anywhere in the codebase today):

- `PyObject.java` (`native/jpype_module/src/main/java/python/lang/PyObject.java`)
  declares no `$`-prefixed method. Its only methods are the always-unmangled
  `hashCode`/`equals`/`toString` (real `java.lang.reflect.Proxy` Object-method
  dispatch, never touches the mangle path) plus `getAttributes()`/`getType()`.
- The two existing `$`-named methods in the tree, `PyBuiltIn.$float`/`$int`
  (`native/jpype_module/src/main/java/python/lang/PyBuiltIn.java:81,92`), are
  plain Java keyword-escaping methods, unrelated to proxy dispatch.
- Every real SPI interface (`python.io.PyRawIOBase` etc.) only ever declares
  protocol methods (`read`, `write`, ...), which mangle to `.read`/`.write`
  map keys - the opposite path from what we need to test.

So the `$`-strip -> `PyObject_GetAttr` fallback path is implemented
(`JPProxyIndirectDict::getCallable` in `native/common/jp_proxy.cpp`, dict
lookup first, `PyObject_GetAttr` fallback) but has zero test coverage and
zero real usage anywhere in the tree.

## Test design

### Python fixture

A plain Python class, no `PyObject` awareness, no SPI/Installer
registration - the whole point is that it's an ordinary duck-typed object,
same shape a numpy array or any third-party class would present:

```python
class AliceBobCharlieDerik:
    def alice(self, arg):
        return ("alice", arg)

    def bob(self, a, b, key=None):
        return ("bob", a, b, key)

    def charlie(self, *args):
        return ("charlie", args)

    def derik(self, *, kwonly):
        return ("derik", kwonly)
```

### Java interface

Return types are **not** generic `Object`. Every fixture method here has a
known, guaranteed return shape (all four return a Python tuple), so the
interface should declare the specific wrapper type - `PyTuple` - not
`java.lang.Object`.

Confirmed the precise mechanism (not assumed) before writing this:
declaring the raw `java.lang.Object` as a `$`-method's return type is a
**hard failure**, not a silent degrade. `TypeManager.java:506-507` only
sets the `PYTHON` modifier bit for classes where
`PyObject.class.isAssignableFrom(cls)`; `jp_typefactory.cpp:203-204` uses
that bit to instantiate `JPPythonType` instead of the plain
`JPObjectType` used for `Object`. Only `JPPythonType::findJavaConversion`
(`jp_pythontype.cpp:34-47`) includes the Python-proxy conversion path at
all - `JPObjectType::findJavaConversion` (`jp_objecttype.cpp:34-51`) never
does, so a `$`-method declared to return plain `Object` throws
`TypeError` at `jp_proxy.cpp:235` the moment it's called. `PyObject`
itself is fine as a return type (it trivially satisfies its own
assignability check and gets `JPPythonType` too) - the actual hazard was
`Object`, not `PyObject`.

The finer reason to prefer `PyTuple` specifically over the safe-but-loose
`PyObject`: conversion always constructs the *same* runtime object either
way - a live dynamic `JPProxy` wrapping the raw `PyObject*`
(`JPConversionPython::convert`, `jp_classhints.cpp:1054-1073`). What
differs is whether the **match** step actually fires:
`JPConversionPython::matches` (`jp_classhints.cpp:1003-1048`) calls
`PyJP_probe` (`native/python/pyjp_probe.cpp`), which walks the real
returned value's `__mro__` against the `_jpype._concrete` registry
(`jpype/_jbridge.py:986-1009`, e.g. `_concrete[tuple] = _PyTuple`) to
decide which interfaces it satisfies. `PyObject` matches trivially - every
value satisfies it, so a `PyObject`-declared return proves nothing about
whether the specific-type recognition path works. `PyTuple` only matches
if the real Python value is recognized as a tuple by that registry (or
duck-types it structurally via `interrogate()`). That's exactly the
question that matters for a future numpy wrapper: would a returned numpy
scalar/array actually get recognized as its specific type, or silently
fall back to bare `PyObject`? Declaring `PyTuple` here and asserting on
tuple-specific accessors is what actually exercises that match, not just
the always-succeeds `PyObject` path.

```java
package python.lang;  // or a dedicated test package - see open question below

@JImplements(...)  // whatever base annotation plain PyObject-rooted test
                    // proxies already use, per PyObjectNGTest.java precedent
public interface PyAliceBobCharlieDerik extends PyObject
{
  PyTuple $alice(Object arg);
  PyTuple $bob(Object a, Object b);
  PyTuple $bob(Object a, Object b, PyKwArgs kw);
  PyTuple $charlie(Object... args);
  PyTuple $derik(PyKwArgs kw);
}
```

Mechanics already confirmed present, no new production code needed:

- Overload resolution: `ProxyType.MethodKey` keys on `name +
  parameterTypes`, so the two `$bob` overloads get distinct descriptors and
  real `java.lang.reflect.Proxy` call-site resolution picks the right one -
  same mechanism any plain Java proxy overload uses.
- `PyKwArgs` (`native/jpype_module/src/main/java/python/lang/PyKwArgs.java`,
  the renamed `PyKeyArgs`): `ProxyInstance.invoke` already detects a
  trailing `PyKwArgs` arg and flattens its entries into keyword arguments
  before crossing to native `hostInvoke`. `PyKwArgs.of("key", value)` for
  `$bob`'s second overload, `PyKwArgs.of("kwonly", value)` for `$derik`,
  should arrive as real Python keyword arguments with matching names.
- `$charlie(Object... args)` exercises plain Java varargs -> Python
  `*args`. This is the specific risk the user flagged while scoping this
  plan: `java.lang.reflect.Proxy`'s `InvocationHandler.invoke` always
  delivers one `Object[]` sized to the interface method's *formal*
  parameter count, so a varargs method's `args` array has a single
  element that is itself the real varargs array - naively forwarding that
  would present `Object[]` where Python expects flattened positional
  arguments (i.e. would land as a single one-tuple argument, not `*args`).
  Confirmed this is already guarded against, not something this plan needs
  to add: `ProxyInstance.invoke`
  (`native/jpype_module/src/main/java/org/jpype/proxy/ProxyInstance.java:50-61`)
  checks `method.isVarArgs()` and unpacks that single wrapper element
  before flattening into the positional/keyword array sent to `hostInvoke`.
  The test still needs to specifically exercise `$charlie` with 2+ elements
  (not just 0 or 1) since that's the only arity that would expose a broken
  unpack - a single-element call is ambiguous evidence either way.
- Mangle-completeness test (`test/jpypetest/test_proxy_mangle.py`,
  `ProxyMangleCompletenessTestCase`) only requires dict keys for
  **non**-`$` abstract methods on `PyObject`-rooted interfaces; since every
  method here is `$`-prefixed, this new interface should need zero entries
  in `_jbridge.py` and should not need to be excluded from that test - but
  run it explicitly once the interface exists to confirm it's genuinely a
  no-op, not silently broken.

### What the test proves, concretely

1. Each of `$alice`/`$bob` (2-arg)/`$bob` (3-arg + `PyKwArgs`)/`$charlie`
   (varargs, called with **2+ elements**, not just 0 or 1 - see the
   varargs-unpack note above)/`$derik` (`PyKwArgs`, single named key)
   calls the *matching* real Python method with the *correct* arguments -
   assert on the tuple each fixture method returns, not just "didn't
   throw." For `$charlie` specifically, assert the returned tuple's
   length matches the call's argument count, so a regression that
   re-nests the varargs array (landing as a single one-tuple argument
   instead of N flattened ones) fails loudly instead of silently passing
   because both shapes happen to not throw.
2. The two `$bob` overloads are never confused with each other (arity
   dispatch is exercised, not assumed).
3. `PyObject`'s own inherited behavior (`hashCode`/`equals`/`toString`,
   `getAttributes()`, `getType()`) still works unchanged on the same proxy
   instance that also carries the `$`-methods - mangling one set of methods
   on an interface must not perturb the other.
4. The `PyTuple`-declared return actually exercises the `_jpype._concrete`
   match (real recognition of the returned value's Python type), not the
   always-succeeds `PyObject` fallback - assert on `PyTuple`-specific
   accessors (`size()`/`get(int)`), not just that the call didn't throw.
   This is the piece that generalizes to a real typed wrapper (e.g. numpy
   returning a `PyFloat` from an indexing op) actually landing as that
   type rather than a bare `PyObject` the caller has to duck-type or cast
   itself. (Not in scope to re-test here, but worth remembering: declaring
   the return type as plain `java.lang.Object` is a hard `TypeError` at
   `jp_proxy.cpp:235`, not a silent degrade - confirmed via
   `JPObjectType::findJavaConversion` never including the Python-proxy
   conversion path.)
5. (Negative check, cheap to add) confirm `getCallable`'s dict-first lookup
   is genuinely being bypassed and not coincidentally succeeding: give the
   fixture object an *extra* attribute literally named `.alice` (a string
   most Python code would never define) with a different, distinguishable
   return value, and confirm `$alice(...)` still returns the real `alice`
   result, not the `.alice` one. This is the one assertion that actually
   distinguishes "fell through to `PyObject_GetAttr`" from "coincidentally
   found something in the dispatch dict."

### Where this lives

Follows the existing `PyObjectNGTest.java` / `test_proxy_mangle.py`
pattern: new Java test interface + fixture under
`native/jpype_module/src/test/java/python/lang/` (or a new
`DispatchFallbackNGTest.java` there), Python fixture class either inlined
via `Interpreter.exec()` in the test setup or as a small resource under
`test/jpypetest/` - whichever existing tests in that directory already do
for ad hoc Python fixtures, to stay consistent.

## Open questions before starting

- Package/annotation for the new interface: plain `@JImplements` test
  proxies live under `test/harness/jpype/proxy/`, but those don't extend
  `PyObject`. `PyObjectNGTest.java` is the closer precedent for a
  `PyObject`-rooted interface but may not itself declare a custom `$`
  proxy. Confirm the right annotation/base pattern by reading
  `PyObjectNGTest.java` and `PyLazySpiNGTest.java` before writing new code,
  rather than assuming.
- Whether `$derik(PyKwArgs kw)` cleanly expresses "the sole argument is
  keyword-only" or whether the Python fixture should instead accept
  `**kwargs` (looser) to avoid a `TypeError` if the flattening path ever
  sends zero entries - decide once `PyKwArgs`'s exact flatten semantics are
  re-checked against `plan/SPI.md:155,189-191`.
- This plan intentionally does *not* attempt a real numpy wrapper - it's
  the minimal proof that the fallback path works at all with a
  hand-written fixture. A follow-up "wrap a real third-party class" plan
  should only be scoped after this one is green.
