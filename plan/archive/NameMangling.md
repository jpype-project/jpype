# Name mangling for PyObject-rooted proxy dispatch

## Status (2026-07-11): DONE - committed `ad21b874` on `reverse`. Root-based
`PyObject` trigger, `$foo`/`.foo` transform, and the 47-map rename in
`_jbridge.py` + `.pyspi` resources all landed atomically with the
completeness test, as planned below. `equals`/`hashCode`/`toString` carved
out of mangling entirely (verified they'd otherwise break identity
silently). The dispatch path itself - a `$foo` method actually reaching a
real Python object's attribute at runtime - had no test coverage at the
time this was marked done; that gap is closed separately by
`plan/DispatchFallback.md` (DONE), which also found and fixed a related
construction-path bug in `JPProxyIndirectDict::getCallable`.

## Problem

`@JImplements`/`JProxy` proxies dispatch a Java interface method call down to
Python by sending the plain Java method name (`method.getName()`, interned
once as a Python string by `StringManager`, cached per-`Method` in
`ProxyType.methodCache` - see `ProxyType.buildDescriptor`) across the bridge.
On the Python side, `JPProxyIndirectDict`/`JPProxyIndirectAttr::getCallable`
(`native/common/jp_proxy.cpp`) look that name up in a dispatch dict/object
first, falling back to `getattr` on the target instance.

For the SPI/`python.lang.PyObject` family this is dangerous: those proxies
duck-type arbitrary third-party Python objects (a user's own `io.RawIOBase`
subclass, say). Every protocol method name we use as a map key - `read`,
`write`, `hashCode`, `equals`, ... - is also a name a real Java interface
elsewhere could plausibly declare, and more importantly there is no
namespace separation between "look this up in our internal dispatch map"
and "this is the user's actual attribute name." `PyObject`'s own Javadoc
already flags the intended convention (`native/jpype_module/src/main/java/
python/lang/PyObject.java` lines 30-32): user-defined wrapper methods use a
leading `$` so they can never collide - but nothing currently mangles
anything; the `_jproxy.py` FIXME at the top of the file says as much
("the java.lang.method we are overriding should be passed to the lookup
function so we can properly handle name mangling on the override").

Plain `@JImplements` proxies (`Runnable`, `Comparator`, arbitrary user
interfaces) must NOT be touched - they rely on exact-name direct/dict
dispatch today and changing that would break every existing user.

## Trigger: root-based, not per-interface annotation

Anchor the switch on `python.lang.PyObject` itself rather than an annotation
authors could forget to add to a new SPI interface. In `ProxyType`'s
constructor (`native/jpype_module/src/main/java/org/jpype/proxy/
ProxyType.java`), once per proxy type:

```java
boolean mangle = false;
for (Class<?> iface : interfaces)
  if (PyObject.class.isAssignableFrom(iface))
  {
    mangle = true;
    break;
  }
this.mangle = mangle;
```

Any interface that ever extends `PyObject` - directly or transitively
(`PyRawIOBase`, `PyBufferedIOBase`, `PyMapping`, `PySequence`, the whole
protocol family) - gets mangling for free, with no per-interface opt-in
step to remember. Plain user interfaces with no `PyObject` in their
hierarchy see `mangle == false` and behave exactly as today.

## Transform

```java
private static String mangle(String name)
{
  if (name.startsWith("$"))
    return name.substring(1);   // user wrapper method -> real direct dispatch
  return "." + name;            // protocol method -> map-only, collision-free
}
```

Applied in `buildDescriptor` at both call sites that currently do
`this.stringManager.get(method.getName())`, gated on `this.mangle`:

```java
String wireName = this.mangle ? mangle(method.getName()) : method.getName();
...this.stringManager.get(wireName)...
```

`ProxyFactory.objectMethods` (the *shared, global* cache of `Object`'s own
`hashCode`/`equals`/`toString`/... built once in `ProxyFactory.init()` and
reused by every proxy type via `tempMap.putAll(factory.objectMethods)`) is
**not** touched. That cache is keyed by `Method` objects from `Object.class`
itself, a different key than the `Method` objects `iface.getMethods()`
returns for an interface that explicitly re-declares `hashCode`/`equals`/
`toString` (as `PyObject` does, `@Override`, abstract). `java.lang.reflect
.Proxy` hands `InvocationHandler.invoke` the most-derived redeclaration when
one exists, so a `PyObject`-rooted proxy's runtime dispatch resolves through
the *interface-specific* `Method` (built by `buildDescriptor`, mangled) while
a plain proxy's dispatch resolves through the *shared* `Object`-cache
`Method` (untouched, unmangled). No extra logic needed - this already falls
out of the existing `bestBySignature`/`isBetter`/`putIfAbsent` machinery.

## Python side: the maps that must move in lockstep

Every dict key currently equal to a plain (non-`$`) protocol method name on a
`PyObject`-descended interface must gain the same `.` prefix, everywhere that
name is registered as a lookup key:

- **`jpype/_jbridge.py`**: 35 `_Py*Methods: MutableMapping[str, Callable]`
  dict literals feeding `_jpype._methods[iface] = ...`, plus any of
  `_PyObjectMethods` that get inherited/reused. (`_PyJPBackendMethods` for
  the internal `Backend` interface is explicitly **out of scope** - `Backend`
  does not extend `PyObject`, so `mangle` is false for it and its keys stay
  as-is.)
- **12 `.pyspi` resource files** under `native/jpype_module/src/main/
  resources/python/io/spi/*.pyspi` - each has a `METHODS = { ... }` dict
  executed by `_installer_register_class`/`_installer_register_lazy_class`,
  same plain-name-key pattern (e.g. `io.RawIOBase.pyspi`: `"read"`,
  `"readall"`, `"readinto"`, `"write"`).

No existing key in either place currently starts with `$`, so in practice
every key across all 47 map sites gets the `.` prefix added - a mechanical,
scriptable rename, not a redesign.

## The real risk: silent, not loud, failure on a missed key

`ProxyInstance.hostInvoke` (`native/common/jp_proxy.cpp`) treats a missing
callable as a no-op success, not an error:

```cpp
if (callable.isNull() || callable.get() == Py_None)
    return parameterTypePtrs;   // silently "succeeds", does nothing
```

If the Java-side trigger flips before every map key is renamed (or a new SPI
interface is added later without updating its `.pyspi`/dict), the failure
mode is not a crash or an exception - it's every call on that method quietly
doing nothing. This is why the user flagged the blast radius as critical.
Mitigation, in order of priority:

1. **Land the Java trigger and the Python key renames as one atomic change**,
   never partially - there is no safe intermediate state.
2. **Add a completeness test**: for every interface reachable from
   `python.lang.PyObject` that is registered in `_jpype._methods`, enumerate
   its abstract methods, mangle each name the same way `ProxyType` does, and
   assert the mangled key exists in the registered dict. This turns "someone
   added a new SPI interface/provider and forgot to mangle its map" into an
   immediate loud test failure instead of a silent runtime no-op, and is the
   regression guard that lets this scale past the current interface set.
3. *(Optional, out of scope for the initial switch)*: consider making
   `hostInvoke`'s missing-callable path loud (raise) instead of a silent
   no-op *specifically* for `mangle == true` proxies, where "the map doesn't
   have this key" should never legitimately happen. Not required to land the
   trigger, but worth a follow-up TODO.

## Sequencing

1. `ProxyType.java`: add the root-based `mangle` field + `mangle(String)`
   helper, gate the two `stringManager.get(...)` call sites in
   `buildDescriptor`.
2. Mechanical rename pass: add `.` prefix to every key in the 35
   `_jbridge.py` dicts and the 12 `.pyspi` files.
3. Add the completeness test (#2 above) before/alongside step 2 so it can
   catch any keys the mechanical pass misses.
4. Full regression suite, both python3.10 and python3.12 - expect broad,
   loud breakage if anything was missed, which is the point of doing this as
   one atomic change rather than incrementally.

## Mangle symbol: decided - `.`

Considered `.` vs `$`. `$` is confusing to use as the actual mangle symbol
here because it would make the scheme a three-tier shift rather than a
symmetric swap: `$foo` (Java) -> bare `foo` (Python) is already one
direction of the transform, so reusing `$` again as the *map*-key prefix
would read as `$foo -> foo -> $foo`-ish and invite mixing up which `$`
means which thing. `.` keeps the two transforms visually distinct (strip a
`$`, or add a `.`).

Checked the one place `.` already means something in JPype: `JClass`
attribute access uses a leading `.` to mean "bypass the static/restricted
view and monkey-patch the bare underlying Python object directly" (e.g.
`setattr(JClass('java.lang.String'), ".field", value)`). Confirmed this is
not a real conflict - that mechanism is about setting attributes on a
`JClass` object itself and never proxies through the Java-calling-Python
dispatch path this plan touches, so there is zero overlap in practice.

Decision: use `.` for the map-only mangled key. If it ever turns out to be
a problem after all, the fallback candidate is `!` (keeps the same
strip-vs-add shift shape, just a different add-symbol).

## Open questions for discussion

- Do we also want optional hardening item #3 above (loud failure on missing
  mangled callable) as part of this change, or strictly deferred?

## Implementation note: equals/hashCode/toString are never mangled

Discovered empirically while landing this (via `PyObjectNGTest.testIdentity`
and `PyExcNGTest.testUnwrapPyException` regressing): `java.lang.reflect.Proxy`
special-cases `equals`/`hashCode`/`toString` at the JVM level. No matter how a
proxy interface redeclares them (as `PyObject` does, `@Override abstract`),
`InvocationHandler.invoke` is *always* called with `Object.class`'s own
`Method` for these three - confirmed by instrumenting `ProxyInstance.invoke`
and observing `method.getDeclaringClass() == java.lang.Object` at the actual
call site, never the `PyObject`-declared redeclaration. That routes through
`ProxyFactory.objectMethods` (built once in `ProxyFactory.init()` straight
from `Object.class.getMethods()`), which is a separate code path from
`ProxyType.buildDescriptor` and was never mangle-aware.

Consequence: mangling `_PyObjectMethods`' `hashCode`/`equals`/`toString` keys
broke every mangle-eligible proxy's identity/equality/string dispatch
silently returning wrong values (not a loud exception - the missing-callable
path happened to still produce a valid-looking `boolean`/`String` result
downstream, which is its own worthwhile finding about the "silent, not
loud" section above). Fixed by:
- `ProxyType.isObjectMethodSignature(Method)`: excludes exactly
  `equals(Object)`/`hashCode()`/`toString()` from mangling regardless of
  `this.mangle`, so `buildDescriptor` never mints a mangled descriptor for
  them (that descriptor would just be dead - never looked up - but leaving
  it mangled invites confusion and broke the completeness test's mirror
  logic).
- `_PyObjectMethods` in `_jbridge.py` keeps its original unmangled
  `"hashCode"`/`"equals"`/`"toString"` keys.
- The completeness test's `_mangle()` mirrors this exact carve-out.

This means the earlier "the shared `Object`-cache dispatches unmangled,
while `PyObject`-rooted dispatch resolves through the interface-specific
mangled `Method`" reasoning above (in the "not touched" paragraph on
`ProxyFactory.objectMethods`) was wrong for these three specific method
names - it holds for every *other* method PyObject or a descendant
redeclares, just not these three, which `java.lang.reflect.Proxy` never lets
an interface override the dispatch `Method` for.
