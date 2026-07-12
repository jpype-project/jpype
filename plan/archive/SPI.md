# SPI: Java-registered Python dispatch hooks

## Status (2026-07-10, still later): lazy per-module resolution implemented and live

The lazy `_cache.__missing__` hook sketched below is no longer a design —
implemented and exercised by `python.io` (475/475 tests passing on branch
`spi`, 473 prior + `PyLazySpiNGTest`'s 2 new tests):

- `WrapperService.getResources()` replaces the old
  `getEagerResources()`/`getLazyResources()` split — one method returning
  every `.pyspi` resource path a provider has; each resource now declares
  its own eager-vs-lazy behavior via a `lazy: true` header field
  (`SpiResource`), rather than which method's return value it appeared in.
  `Installer` gained `registerLazyClass(pyModule, pyClass, javaInterface,
  methodsSource)` alongside `registerClass`/`registerBackend` — it only
  stashes the ingredients (no import, no exec); mini-backends (`kind:
  backend`) must stay eager (`SpiResource.parse` throws if `lazy: true` is
  combined with `kind: backend`).
- All the actual cross-language work happens once, at boot:
  `SpiLoader.load()` reads every resource's header (cheap - no `exec`, no
  Python import) and, for lazy ones, calls
  `_jbridge._installer_register_lazy_class` (hoisted to module level so
  it's reachable outside `initialize()`'s closure), which just stashes
  `(javaInterface, methodsSource)` into `_jpype._lazy_pending[pyModule][pyClass]`.
  The actual `_cache.__missing__`-equivalent hook (`_core.py`'s
  `_LazyCache`, replacing `_jpype._cache`'s plain
  `weakref.WeakKeyDictionary`) does **zero** cross-language round trips on
  a miss — it's pure Python, draining `_lazy_pending` for the missed
  type's `__mro__` module set and calling the same
  `_jbridge._installer_register_class` the eager path already uses. This
  is a deliberate simplification over the `Backend.spiResolveModule`
  per-module-round-trip sketch below: the round trip already happened at
  boot (reading headers), so the hot miss path pays nothing but a Python
  dict/set operation.
- Confirmed empirically (not just by reading `pyjp_probe.cpp`): `_cache`
  is read via `PyObject_GetItem`, a genuine miss `PyErr_Clear()`s and
  falls through unchanged to the existing `concreteDict` MRO scan and
  tuple-building — so `_LazyCache.__getitem__` only has to get
  `_jpype._concrete`/`_jpype._methods` populated before re-raising
  `KeyError`; it never builds the probe's result tuple itself. One
  correction from the sketch below: `WeakKeyDictionary.__getitem__` does
  its own dict lookup and raises `KeyError` directly - it does **not**
  consult a subclass's `__missing__` - so the hook overrides
  `__getitem__`, not `__missing__`.
- `SpiLoader.listPyspiResources(Class<?> anchor, String dir)` — new
  classpath-directory scanner (handles both an exploded `target/classes`
  dir, as under `mvn test`, and a packaged jar, as the real `ant`-built
  `org.jpype.jar`; both verified this pass) so a provider's
  `getResources()` can just point at its resource directory once instead
  of hardcoding every file name. `python.io.PyIoWrapperService` now does
  exactly that — `_io.StringIO.pyspi` picked up `lazy: true` as the
  worked lazy example; everything else in `python.io` stays eager.
- Cleanup: `WrapperProvider.java` and `WrapperService.getInterfaces(String)`
  (confirmed dead — zero references anywhere in the tree, an abandoned
  earlier sketch this design supersedes) were deleted.
- Tests: `PyLazySpiNGTest` (Java-triggered direction — constructs a raw
  `_io.StringIO` via `context.eval(...)`, never touching `IO.using(...)`,
  and casts it straight to `PyStringIO`) and `test_lazy_spi.py` at the
  repo root (Python-launched-script direction — asserts
  `_jpype._lazy_pending` is populated before first use and drained after,
  via `jpype.JObject(io.StringIO(...), "python.io.PyStringIO")`).

## Status (2026-07-10, later same day): BackendRegistry moved off a static, InputStream/OutputStream added

Two follow-ups after the eager path below went live:

- `org.jpype.BackendRegistry` (a bare static `Map`) was replaced —
  mini-backend registration now lives on `NativeContext` (the true
  per-interpreter object) as `registerBackend`/`getBackend`, reached from
  Python via `_jpype.context().registerBackend(...)` and from Java via
  `PyBuiltIn.getBackend(Class<T>)` (added to `python.lang.PyBuiltIn`,
  reachable from any live `PyObject` via the existing `builtin()` backdoor —
  no statics needed once you have any `PyObject` in hand).
  `python.io.IO.instance()` still has one `MainInterpreter.getInstance()`
  hop as a bootstrap convenience for when no `PyObject` exists yet, matching
  the existing `getBackend()`/`getInstaller()` precedent — this only works
  when called from **Java**; see `plan/IO.md`'s new status section for why
  calling it from a plain launched Python script doesn't work the same way
  (an unrelated jpype forward-marshalling quirk, not an SPI bug).
- `native/build.xml` (the real `ant`-built jar `pip install -e .` ships) was
  silently dropping `.pyspi` resources — fixed, see `plan/IO.md`.

## Status (2026-07-10): eager path implemented and live

The eager registration path described below is no longer a sketch — it's
implemented and exercised by `python.io` (466/466 tests passing on branch
`spi`):

- `org.jpype.Installer` — `registerClass(pyModule, pyClass, javaInterface,
  methodsSource)` / `registerBackend(javaInterface, methodsSource)`.
  Implemented by `_jbridge.py` (`_installer_register_class` /
  `_installer_register_backend`, a `JProxy` same as `Backend`) and installed
  via `MainInterpreter.setInstaller(Installer)`, which immediately calls
  `org.jpype.SpiLoader.load(installer)`.
- `org.jpype.SpiLoader` — `ServiceLoader.load(WrapperService.class)`, then
  for each provider reads every path in the new
  `WrapperService.getEagerResources()` (default empty array) and replays it
  into the installer.
- **`.pyspi` resource format** (`org.jpype.SpiResource`), one file per
  Python class or per mini-backend: a `key: value` header (`kind: class` +
  `module:`/`class:`/`interface:`, or `kind: backend` + `interface:`), a
  line containing only `---`, then a blob of Python source that must bind a
  top-level `METHODS` dict when `exec`'d. `_installer_register_class` execs
  it and writes into `_jpype._concrete[pyType]` / `_jpype._methods[iface]`;
  `_installer_register_backend` execs it, builds a `JProxy`, and registers
  it in `org.jpype.BackendRegistry` (see "Mini-backends" below).
- Worked example: `native/jpype_module/src/main/resources/python/io/spi/*.pyspi`
  (6 files) + `python.io.PyIoWrapperService.getEagerResources()`. This fully
  replaced the hand-written `_PyIOBaseMethods`-style dicts and the manual
  `_jpype._concrete[io.IOBase] = _PyIOBase`-style lines that used to live
  directly in `_jbridge.py` — core `_jbridge.py` now has zero
  `python.io`-specific code left in it.
- Required a `module-info.java` fix along the way: `uses
  org.jpype.WrapperService;` + `provides org.jpype.WrapperService with
  python.io.PyIoWrapperService;` — `ServiceLoader` inside a named JPMS
  module ignores `META-INF/services` for providers in that same module;
  only the module descriptor's `provides...with` is consulted.

**Update:** the lazy per-module `_cache.__missing__` hook below is now
implemented and live — see the "lazy per-module resolution" status section
at the top of this file. `WrapperService.getInterfaces(String)` /
`PyIoWrapperService.MANIFEST` (the drafted-but-unused shape this paragraph
used to point at) were dead code and have been deleted; the real
implementation took a different, simpler shape (see top status section).

## TL;DR

The cost of this feature is much lower than it looks. There is no new
mechanism to build — both extension points already exist and are idle:

1. `st->cacheDict[type] = (interfaces, methods)` in `pyjp_probe.cpp` — "this
   Python type satisfies these Java interfaces."
2. The dispatch dict behind `JPProxyIndirectDict` in `jp_proxy.cpp` — "this
   method name resolves to this callable."

The SPI is just: **a file format for declaring injects (class name,
interface list, method-name-to-binding pairs) plus a loader in
`_jbridge.py` that reads it at init and populates those two maps.** No new
C++ dispatch code, no new probe logic — just data plus a populate step.

## Goal

Allow a Java module to register additional handlers into the reverse-proxy
dispatch tables so that Python-side objects gain new callable "special
methods" without hand-writing a full `@JImplements` class per case. Motivating
use case: build classes that behave like numpy arrays (or other
Python-protocol-heavy types) directly from Java, by injecting operator/dunder
implementations at bind time.

Target call shapes, all resolving to a single registered handler named `foo`:

```
$foo(arg)
$foo(arg, arg)
$foo(arg, PyKeyArgs)
$foo(...arg)
```

The `$`-prefix is the marker that says "this call should be resolved through
the SPI dispatch table by stripping the prefix," as opposed to an ordinary
interface method name.

## What already exists (confirmed 2026-07-09)

Dispatch fallback already exists and does most of the needed work:

- `jpype/_jbridge.py:233` — `_PyJPBackendMethods`, a hardcoded Python
  `dict[str, Callable]` built once in `initialize()`.
- `jpype/_jbridge.py:911` — `Backend@JProxy(Backend, dict=_PyJPBackendMethods)`
  constructs the proxy with `dict=`, selecting the indirect-dict resolution
  path.
- `native/python/pyjp_proxy.cpp:87` — `dict=` kwarg selects
  `JPProxyIndirectDict` (vs `JPProxyDirect` / `JPProxyIndirectAttr`).
- `native/common/jp_proxy.cpp:371-379` —
  `JPProxyIndirectDict::getCallable()`: `PyDict_GetItem(m_Instance->m_Dispatch, name)`,
  falling back to `PyObject_GetAttr` on the proxy instance if the name isn't
  in the dict. This is the "fall back to lookup" behavior — but it's an
  **exact-key** lookup only; no prefix stripping.
- Java side: `MethodDescriptor` (`org/jpype/proxy/MethodDescriptor.java:25-40`)
  already carries `name`, `defaultHandler` (MethodHandle), and a `bypass`
  flag per interface method. `ProxyType.buildDescriptor`
  (`org/jpype/proxy/ProxyType.java:160-186`) builds one per interface method;
  `ProxyInstance.invoke()` (`org/jpype/proxy/ProxyInstance.java:37-103`)
  checks `md.bypass` before falling through to `hostInvoke`, which lands in
  `Java_org_jpype_proxy_ProxyInstance_hostInvoke`
  (`native/common/jp_proxy.cpp:128`) and calls `proxy->getCallable(...)`
  (~line 197) — i.e. the dict/attr lookup above.
- Keyword-argument plumbing for this path is done (see
  `PyKeyArgs`/`ProxyInstance.invoke` — commit `14ec2596`): a trailing
  `PyKeyArgs` vararg is already flattened into `numPositional`/`numKeyword`
  before crossing into native code, so `$foo(arg, PyKeyArgs)` requires no new
  work beyond naming/resolution.

**Not implemented yet:**

1. `$`-prefix stripping / name-mangling — grepped, zero hits. Purely
   conceptual today.
2. Runtime registration — `_PyJPBackendMethods` is a static dict literal, not
   an API a Java module can add entries to.
3. There is an **unwired** ServiceLoader precedent already scaffolded:
   `org/jpype/WrapperService.java` + `WrapperProvider.java:12-65`
   (`ServiceLoader.load(WrapperService.class)`, intended to map Python module
   names to Java interfaces). Nothing else in the tree references it, and no
   `META-INF/services/org.jpype.WrapperService` file exists. This looks like
   the closest existing shape for "a Java module injects hooks" and is the
   leading candidate to revive rather than build a parallel mechanism.

## Proposed design (revised 2026-07-10, grounded in `WrapperService`)

### Key finding: the cache lookup is `PyObject_GetItem`, not `PyDict_GetItem`

`PyJP_probe(st, type)` (`native/python/pyjp_probe.cpp:228`) is entirely gated
by a single cache check at line 231:

```cpp
JPPyObject cached = JPPyObject::use(PyObject_GetItem(st->cacheDict, (PyObject*) type));
if (cached.isValid())
        return cached.keep();
PyErr_Clear();
```

`st->cacheDict` is `_jpype._cache`, a plain Python `dict` loaded once via
`loadAttr(module, "_cache")` (`pyjp_module.cpp:163`). Critically, the probe
reads it with `PyObject_GetItem` — the generic subscript operator — not the
C-level `PyDict_GetItem` (which is used a few lines later for
`st->concreteDict`, at line 258, and does *not* respect Python-level
overrides). `PyObject_GetItem` on a `dict` subclass honors `__missing__`
exactly like `collections.defaultdict`.

**This means lazy SPI resolution needs zero new C++ code.** Replace
`_jpype._cache` with a `dict` subclass implementing `__missing__(self, type)`:
on a genuine miss it does the SPI resolution (below), either populates
`self[type]` and returns the tuple (probe never runs), or raises `KeyError`
(the existing `PyErr_Clear()` at line 234 already handles that and falls
through to the normal MRO-scan/`interrogate()` path, completely unchanged).
No pre-seeding, no write-once-cache risk, no new probe logic — the hook lives
entirely in `_jbridge.py`.

The tuple shape a resolved entry must produce is still whatever
`finalizeInterfaces` (`pyjp_probe.cpp:134-157`) / `finalizeMethods`
(`pyjp_probe.cpp:159-198`) produce: `(interfaces_tuple, methods_dict)`. The
`__missing__` hook should call those two conceptually (or replicate their
contract) rather than inventing a second shape.

### Lazy granularity: by module, not by class

A naive design resolves one Python *class* per SPI round trip. But the
motivating use case (`numpy.ndarray`-style third-party types) means the
target class often doesn't exist as a `PyTypeObject*` until its package is
imported — eager, boot-time registration cannot work for it, so lazy is not
optional, it's the primary mechanism. Given that, batch by **module** instead
of by class: one Java round trip resolves everything a provider knows about
for that Python module, and every class in it gets cached in one shot.

Checked empirically against the `io` module (the intended first provider,
see `plan/IO.md`):

```
IOBase             module=io    bases=['_IOBase']
BytesIO            module=_io   bases=['_BufferedIOBase']
StringIO           module=_io   bases=['_TextIOBase']
FileIO             module=_io   bases=['_RawIOBase']
BufferedReader      module=_io   ...
TextIOWrapper       module=_io   ...
```

Gotcha to design around: the *public* abstract classes (`io.IOBase` etc.)
report `__module__ == "io"`, but every concrete class report
`__module__ == "_io"` (the C accelerator module backing the `io` facade).
A hook keyed on the leaf class's own `__module__` alone will never resolve
`BytesIO`. Resolve against the set of `__module__` values across
`type.__mro__`, not just `type.__module__`, and expect real third-party
providers (numpy has the same C-extension/Python-facade split) to need the
same treatment.

```python
_triedModules = set()

class _CacheDict(dict):
    def __missing__(self, type_):
        for mod in {c.__module__ for c in type_.__mro__} - _triedModules:
            _triedModules.add(mod)
            manifest = _jpype.Backend.spiResolveModule(mod)  # one call per unseen module
            if manifest:
                for cls, entry in manifest.items():
                    dict.__setitem__(self, cls, entry)
        if type_ in self:
            return self[type_]
        raise KeyError(type_)  # falls through to normal probe, unchanged
```

Per-class lazy remains available as a fallback for a provider that can't
manifest a whole module up front, but module-batch should be the default
contract.

### The `Installer` surface: eager for interfaces, lazy for class membership

Split the registration surface along a line that falls out of the above:

- **Java interfaces are compiled classes, known at JVM boot.** Their method
  dicts (what currently ships hardcoded as
  `_jpype._methods[_PyBytes] = _PyBytesMethods` etc., `_jbridge.py:1006+`)
  can and should be registered **eagerly**, during `_jbridge.initialize()`,
  regardless of whether any Python type using them has been imported yet.
  There's no reason to defer this — the interface exists whether or not a
  satisfying Python class has shown up.
- **Which Python type satisfies which interface is the part that has to be
  lazy**, per the module-batch hook above, since the Python type may not
  exist yet.

One `Installer` interface, used both ways, unifies this with the existing
`WrapperService`/`WrapperProvider` scaffold
(`org/jpype/WrapperService.java`, `WrapperProvider.java:12-65` — currently
unwired, zero references elsewhere in the tree, no
`META-INF/services/org.jpype.WrapperService` file):

- `installer.registerInterface(Class<?> javaInterface, methodBindings)` —
  called once per provider at init; writes into `_methods` the same way the
  hardcoded lines do today.
- Extend `WrapperService` (currently only `getInterfaces(String clsName)`,
  one class at a time — `WrapperProvider.java:36-39`) with a batch method:
  ```java
  default Map<String, Class<?>[]> getModuleManifest(String moduleName) {
      return null; // provider opts in; null means "fall back to per-class"
  }
  ```
  `WrapperProvider` already shards services by module name
  (`moduleToServiceMap`, keyed by the string before the last `.` in the
  Python qualified name) — the batch method just needs to be called from the
  `__missing__` hook via a dedicated SPI-resolution entry point (see "Mini-
  backends" below — **not** a new method on `Backend` itself, same
  restriction as everything else here).

This also folds in cleanly as the loader for `io` becoming an actual SPI
provider itself (see `plan/IO.md`) rather than a hardcoded factory — `io`
proves the `Installer`/`WrapperService` contract works for a real,
non-trivial class hierarchy before any third party depends on it.

### Mini-backends: providers can't extend `Backend`, so they get their own

Resolved 2026-07-10, first implemented in `python.io`: `Backend` is one
fixed, compiled interface, constructed once as a single `JProxy` bound to
`_PyJPBackendMethods` (`_jbridge.py`). An SPI provider cannot add methods to
it — there's no per-provider extension point on a single shared interface
and single shared dict. The original `python.io` first cut got this wrong,
bolting `newBytesIO()`/`newStringIO()`-style construction hooks directly
onto `Backend` (and a matching `PyBuiltIn.bytesIO()` convenience method) —
that would require editing core `org.jpype.Backend` and core
`python.lang.PyBuiltIn` for every new SPI provider, exactly the coupling the
SPI is supposed to avoid.

Fixed shape: each provider that needs its own module-level hooks (mostly
construction — a provider's factory functions have nowhere else to live)
declares its own small backend-shaped interface, builds its own `JProxy`
bound to its own dict, and registers the instance in a new
`org.jpype.BackendRegistry` (`Map<Class<?>, Object>`, `register`/`get`) —
independent of `Backend` and independent of every other provider's
mini-backend. The provider's interface exposes a `static instance()`
accessor that looks itself up in the registry, and **is** the user-facing
entry point (there is deliberately no `PyBuiltIn`-style wrapper layer on
top — core `python.lang` should not gain per-provider knowledge).

`python.io.IO` is the worked example:

```java
public interface IO {
  static IO instance() { return BackendRegistry.get(IO.class); }
  PyBytesIO bytesIO();
  PyBytesIO bytesIO(PyBuffer initial);
  PyStringIO stringIO();
  PyStringIO stringIO(CharSequence initial);
}
```
```python
_io_backend = _IO@JProxy(_IO, dict=_PyIOBackendMethods)
_BackendRegistry.register(_IO, _io_backend)
```

Called as `IO.instance().bytesIO()`, not `context.bytesIO()`. The same
mini-backend/registry pattern is the answer for the `__missing__` hook's
module-resolution call above: it should go through a provider-agnostic SPI
entry point (e.g. a small fixed interface for "resolve a module manifest",
itself registered the same way) rather than a method hung directly on
`Backend`.

`@Bypass` note: none of `IO`'s own methods need it — they're plain,
dict-resolved abstract methods, same as `Backend`'s (which also carries no
`@Bypass` methods). `@Bypass` only matters on `python.lang`/`python.io`
*object*-family interfaces (`PyObject` and friends) where a method's real
implementation must never go through the per-instance Python dict lookup —
e.g. it constructs a Java-side helper directly (`PyObject.getAttributes()`)
or collides in signature with an unrelated JDK interface method the proxy
also implements (`PyBytes.get(int)` vs. `List.get(int)`). Keep watching for
this as `python.io` grows — e.g. if `PyIOBase` ever implements
`java.lang.AutoCloseable` for try-with-resources support (part of the
`InputStream`/`OutputStream` promotion work in `plan/IO.md`), `close()`
would collide with `AutoCloseable.close()` the same way and need `@Bypass`
then. No current `python.io` method needs it yet.

### The Java-method-name dispatch side (`$foo`)

Two small pieces, not a new dispatch engine:

1. **Registration**: make the dispatch dict extendable per proxy/type rather
   than one hardcoded global. Likely shape: a Java-side resource (map
   entries, similar to how `WrapperProvider` already maps names to
   interfaces) read at bind time and injected into the dict backing
   `JPProxyIndirectDict`, so `foo` becomes resolvable without editing
   `_jbridge.py`.
2. **Dispatch**: when a Java interface method is named `$foo`, strip the `$`
   before doing the existing dict/attr lookup, so it resolves to whatever was
   registered under `foo`. This is a small change in
   `JPProxyIndirectDict::getCallable()` (`jp_proxy.cpp:371`) or in
   `MethodDescriptor`/`ProxyType.buildDescriptor` when the name is interned —
   TBD which layer should own the stripping.

Open questions for next session:
- Should `$foo` registration be per-`ProxyType` (interface) or global (one
  shared dispatch table across all reverse proxies)?
- Where does the actual Python-side callable come from when a Java module
  "injects hooks" — is the resource a class name to instantiate, a static
  method reference, or something else?
- Does the class-membership resolution (`_cache.__missing__`, above) and the
  `$foo` method-dispatch mechanism end up sharing the same `Installer`
  registration call, or stay two entry points on one interface (one for
  "what interfaces does this type satisfy," one for "what does this method
  name resolve to")? Current lean: two methods on one `Installer`, since
  they populate different tables (`_cache`/`_concrete`/`_methods` vs. the
  `JPProxyIndirectDict` dispatch dict) even if a single provider often wants
  to call both.
- Resolved this pass: cache injection is a `__missing__` hook, not a
  pre-seed, so the "is `cacheDict` write-once / can a bad entry be
  permanent" risk from the previous draft no longer applies — a hook can be
  fixed and will simply be consulted again on the next miss for a type not
  yet cached.

## Other work queued behind this (not started)

- **Test bench**: more coverage in general, lower risk, should probably be
  done before/alongside SPI work since it doesn't touch the same code paths.
- **Subinterpreter testing**: flagged as the highest-risk remaining item
  (silent/racy failures, interacts with proxy lifetime and PEP 684 state) —
  should get its own dedicated session, not be interleaved with SPI work.
