# Plan: audit and fix remaining `NewGlobalRef` call sites

## Status (2026-07-10): DONE - all 5 items complete, 475/475

Execution order items 1-4 are complete and verified (475/475 after each
step, incremental commits recommended per step):

1. Category B immortals - `jp_classloader.cpp`/`.h` (`s_ClassClass`/
   `s_SystemClassLoader`/`s_ForNameID`, `m_BootLoader` now always takes its
   own owned ref and is released via `JPClassLoader::release()`), `jp_gc.cpp`
   (`s_SystemClass`/`s_GcMethodID`), `jp_context.cpp` (`s_RuntimeExceptionClass`/
   `s_ArrayClass`/`s_ArrayNewInstanceID` via `bindCoreImmortals()`). Also added
   `delete m_ClassLoader` to `~JPContext()` (was leaked, never deleted).
2. Category C item 1 - `JPProxy::m_ProxyType` converted to `jref`, routed
   through `frame.storeGlobal()`/`tryRelease()`. This also fixed a real leak:
   the old destructor only released `m_ProxyType` when `m_Ref != nullptr`
   (i.e. only if `getProxy()` had been called at least once) - `tryRelease()`
   is now called unconditionally. `m_Ref` (weak global) left untouched as
   planned.
3. Category A release wiring - `detachJVM()` now calls
   `context->ReleaseGlobalRef(...)` for `m_ContextClass`, `m_JavaContext`,
   `m_TypeManager->m_JavaTypeManager` (confirmed `~JPTypeManager() = default`,
   no existing release, safe to add), `m_Reflector`, `m_SupportClass`,
   `m_ProxyFactoryClass`, `m_JavaProxyFactory`, `m_ProxyTypeClass`,
   `m_PyJavaObjectClass`.
4. Category C item 2 - `pyjp_package.cpp`'s per-module package cache now
   encodes a `jref`'s `long` value directly as the `PyCapsule` pointer
   payload (`(void*)(intptr_t) ref.value`), resolved via
   `frame.retrieveGlobal()`/released via `tryRelease()` in `dtor()` - removed
   the old hand-rolled attach-if-detached-then-`DeleteGlobalRef` dance
   entirely (same bug class `PyJPValue_finalize` had before this session's
   earlier fix).

5. Category C item 3 - `jp_reference_queue.cpp`'s `s_ReferenceQueue` was a
   file-scope static silently clobbered on every `NativeReferenceQueue`
   init (i.e. every context/subinterpreter start), root-caused to
   `NativeReference.init(Object self, Method m)` never having taken a
   context handle in the first place - it had no way to know which
   `JPContext` it belonged to. Fixed at the source: added a `long ctx`
   parameter to `NativeReference.init` (matching the existing
   `removeHostReference`/`wake` signatures), passed as
   `NativeReferenceQueue`'s existing `address` field
   (`NativeReferenceQueue.java:98`). `Java_org_jpype_ref_NativeReference_init`
   now resolves `(JPContext*) ctx` and stores into new per-context fields
   `JPContext::m_ReferenceQueue`/`m_ReferenceQueueRegisterMethod` instead of
   file-scope statics. `JPReferenceQueue::registerRef()` reads
   `frame.getContext()->m_ReferenceQueue` instead of the old statics.
   Released via `ReleaseGlobalRef()` in `detachJVM()` alongside the other
   Category A per-context globals. No `RegisterNatives` table to update -
   standard JNI name-based native resolution picks up the new signature
   automatically.

## Original scoping (context for the plan, now executed above)

## Context

This is the direct follow-on to the `jref`/`GlobalPool` work done earlier this
session (see git log: "Fix leaked-GIL startup bug, enable async calls, and
finish testbench coverage" and the uncommitted working-tree changes on top of
it - `JPClass`/`JPMethod`/`JPField`/`JPArray`/`JPMonitor`/`JPBuffer`/`JPValue`
converted from raw JNI globals to pool-routed `jref` handles, 475/475 tests
passing).

That work only converted the specific set of C++ objects that call
`tryRelease()` in their destructors. There are many more `frame.NewGlobalRef`/
`env->NewGlobalRef` call sites in the tree that were never audited. The user's
framing for this pass, verbatim:

> now you will want to remove Frame.NewGlobal and decide if that is a real
> global that can use JNIEnv*, an immortal that doesn't belong on the context
> but in a run once when JVM starts, or a stray like m_ProxyType that we
> failed to commit.

So every remaining `NewGlobalRef` site needs to be bucketed into one of three
categories, and each category has a different fix:

- **(A) Real per-context global.** Legitimately different per `JPContext`
  (i.e. per interpreter/subinterpreter) - e.g. the `NativeContext` instance
  itself. Fine to keep as a plain `frame.NewGlobalRef`/raw `jobject` field,
  *but* it must be released at context teardown via
  `JPContext::ReleaseGlobalRef()` (the existing helper, already used for
  `m_Interpreter` in `detachJVM()` - see `jp_context.cpp:359`). Today, only
  `m_Interpreter` gets this treatment; every other per-context global listed
  below is currently **leaked** on context teardown (never released) - not a
  correctness bug for a single main interpreter that lives for the process
  lifetime, but a real leak for subinterpreters that start/stop repeatedly.
- **(B) True process-wide immortal.** The exact same Java object/class
  regardless of which `JPContext` resolves it (there is one JVM process; only
  Python has multiple interpreters). Should be hoisted out of the
  per-context struct entirely into a file-scope `static` resolved once via
  `std::call_once`, mirroring the `s_GlobalPoolClass`/`bindGlobalPoolRelease`
  pattern already built this session in `jp_context.cpp` (~line 728-755).
  No release needed - it lives for the process, same as any interned JNI
  global for a core JDK class.
- **(C) Stray.** A per-*instance* (not per-context) binding that was
  overlooked when the `jref`/`GlobalPool` conversion was done - i.e. it
  should have gotten the same treatment as `JPClass::m_Class` etc. but didn't.
  Fix: convert to `jref`, route through `frame.storeGlobal()`/
  `frame.retrieveGlobal()`/`tryRelease()` like everything else this session.

## Site-by-site classification (from `grep -rn NewGlobalRef native/common
native/python`, minus the `jp_javaframe.*` wrapper itself)

### Category B - immortals to hoist (do these first, they're independent/low-risk)

1. **`jp_classloader.cpp` `JPClassLoader::JPClassLoader`** - `m_ClassClass`
   (`java.lang.Class`) and `m_SystemClassLoader`
   (`ClassLoader.getSystemClassLoader()`) are resolved fresh on *every*
   `JPClassLoader` construction (i.e. every `JPContext`/subinterpreter start),
   but both are the same object every time - there's one JVM. Hoist to
   file-scope `static jclass s_ClassClass` / `static jobject
   s_SystemClassLoader` / `static jmethodID s_ForNameID`, resolved once via
   `std::call_once` in a `bindImmortals(JPJavaFrame&)` helper called at the
   top of the constructor (mirrors `bindGlobalPoolRelease` exactly).
   `findClass()` then calls `s_ClassClass`/`s_ForNameID` instead of the
   member fields.

   **`m_BootLoader` stays per-context** (Category A, not B) - in the
   "already installed" branch it currently just aliases
   `m_BootLoader = m_SystemClassLoader` (no new ref taken) which is a latent
   double-free risk once `m_SystemClassLoader` becomes a shared immortal -
   fix by always taking an owned ref: `m_BootLoader =
   frame.NewGlobalRef(s_SystemClassLoader)` in that branch too, so
   `m_BootLoader` can always be released independently. Needs a
   `JPClassLoader::release(JPContext*)` method (calling
   `context->ReleaseGlobalRef(m_BootLoader)`) invoked from
   `JPContext::detachJVM()` - `JPContext::m_ClassLoader` is a raw pointer,
   never deleted today either (`~JPContext()` only deletes `m_TypeManager`
   and `m_GC` - `native/common/jp_context.cpp:161-165`); add `delete
   m_ClassLoader` there too while touching this.

   *(An edit implementing the `s_ClassClass`/`s_SystemClassLoader` half of
   this was started and then reverted this session to keep the tree
   buildable before compacting - the header (`jp_classloader.h`) was never
   updated to match. Redo from scratch, updating both files together this
   time.)*

2. **`jp_gc.cpp` `JPGarbageCollection::init`** - `_SystemClass`
   (`java.lang.System`, for `System.gc()`) is a per-instance field of
   `JPGarbageCollection` (`native/common/include/jp_gc.h:58`), and JPContext
   owns one `JPGarbageCollection` per context (`m_GC = new
   JPGarbageCollection(this)`) - so this gets re-resolved and re-globalized
   per context, identically to the classloader case. Hoist to a file-scope
   `static jclass s_SystemClass` resolved once (same `std::call_once`
   pattern). `_gcMethodID` can be hoisted alongside it (method IDs from an
   immortal class are themselves stable/immortal).

3. **`jp_context.cpp` `initializeResources`** - two JDK core classes are
   resolved via plain `frame.FindClass` (i.e. boot/system classloader, not
   `m_ClassLoader->findClass`, which is what makes them different from the
   per-context-classloader classes below) and stored as long-lived context
   fields:
   - `m_RuntimeException` (`java.lang.RuntimeException`, line ~425)
   - `m_Array` (`java.lang.reflect.Array`, line ~584)

   Both are immortal candidates - same object regardless of context. Hoist
   both to file-scope statics resolved once (can go in the same
   `bindGlobalPoolRelease`-style helper, or a new one - there's already a
   `#include <mutex>` and `std::once_flag` precedent in this file to copy).
   `m_Array_NewInstanceID` (a method ID off `m_Array`) hoists alongside it
   for the same reason as `_gcMethodID` above.

   Everything else resolved via plain `frame.FindClass` in this function
   (`throwableClass`, `objectClass`, `stringClass`, `classClass`,
   `bufferClass`, `bytebufferClass`, `comparableClass`) is **not** stored as
   a global at all - only used locally to resolve a method ID and then
   dropped - so those are already fine as-is, nothing to do.

### Category A - real per-context globals, currently leaked (wire up release)

All of these are resolved via `m_ClassLoader->findClass(...)` (this
context's own classloader) or are per-context *instances* (not classes), so
they are genuinely different per context/subinterpreter and must stay
per-context fields. None of them are released today. Add
`context->ReleaseGlobalRef(...)` calls for each in `JPContext::detachJVM()`
(`native/common/jp_context.cpp:357-370`), right alongside the existing
`ReleaseGlobalRef(m_Interpreter);`:

- `m_ContextClass` (`org.jpype.internal.NativeContext`, line ~448)
- `m_JavaContext` (the `NativeContext` instance itself, line ~479)
- `m_TypeManager->m_JavaTypeManager` (line ~503) - check whether
  `JPTypeManager`'s own destructor already handles this before adding a
  second release (would double-free) - it didn't appear to when last
  checked, but verify.
- `m_Reflector` (line ~509)
- `m_SupportClass` (`org.jpype.internal.Support`, line ~524)
- `m_ProxyFactoryClass` (`org.jpype.proxy.ProxyFactory`, line ~556)
- `m_JavaProxyFactory` (line ~561)
- `m_ProxyTypeClass` (`org.jpype.proxy.ProxyType`, line ~569)
- `m_PyJavaObjectClass` (`python.lang.PyJavaObject`, line ~601)
- `JPClassLoader::m_BootLoader` (see Category B item 1 above - same fix,
  different mechanism since it's not a direct `JPContext` field)

`m_Interpreter` (`jp_bridge.cpp:295`, `jp_context.cpp:335`) is **already**
handled correctly - leave as reference for the pattern to copy.

`jp_exception.cpp`'s `JPypeException::m_Throwable` (lines 66, 110, 117) is
**already correct as-is** - it's a proper RAII pair (global taken in the
ctor/copy-ctor, released in `~JPypeException()` via
`m_Context->getEnv()->DeleteGlobalRef`), scoped to the lifetime of a C++
exception object that's always stack-unwound. Not a leak, nothing to fix,
just confirming it doesn't need touching.

### Category C - strays (need the full `jref`/pool treatment)

1. **`jp_proxy.cpp` `JPProxy::m_ProxyType`** (line ~287) - the user's named
   example. `JPProxy` is a C++ object bound 1:1 to a Python callable
   (`m_Instance`), exactly the shape of object this session already
   converted (`JPClass`, `JPMethod`, ...). Currently `m_ProxyType` is a raw
   `jobject` field, taken via `frame.NewGlobalRef` in the constructor and
   released by hand in `~JPProxy()` via `m_Context->getEnv()->
   DeleteGlobalRef(m_ProxyType)` (plus a matching raw `DeleteWeakGlobalRef`
   for `m_Ref` right next to it, same file, same treatment needed). Convert
   `m_ProxyType` to `jref`, route creation through `frame.storeGlobal(...)`
   and destruction through `tryRelease(...)`, matching the established
   pattern. `m_Ref` (a *weak* global) doesn't fit the pool model (pool
   membership itself is what keeps an object alive - a weak ref is the
   opposite intent) - leave `m_Ref` as a raw JNI weak global, but confirm
   `JPProxy::~JPProxy()`'s existing `m_Context->isRunning()` guard is still
   correct after the `m_ProxyType` change.

2. **`pyjp_package.cpp` `getPackage()`/`dtor()`** (lines ~104-152) - a
   per-Python-module cache of a Java package object, stored as a raw
   `jobject` inside a `PyCapsule` (`dict["_jpackage"]`), released via a
   hand-rolled `dtor()` that does the exact manual
   attach-if-detached-then-`DeleteGlobalRef` dance that
   `PyJPValue_finalize` used to do before this session's fix (see
   `pyjp_value.cpp`, now routes through `tryRelease()` instead). This is
   the same bug class, just in a different (capsule-based, not
   `JPValue`-slot-based) home. Fix: store a `jref`-derived handle in the
   capsule instead of a raw `jobject` pointer (a `jref` is just a `long`,
   encodable as a `void*` via `(void*)(intptr_t) ref.value`, no new struct
   needed), obtained via `frame.storeGlobal(jo)`; `dtor()` decodes it back
   to a `jref` and calls `tryRelease()` instead of the manual JNI dance.

3. **`jp_reference_queue.cpp` `s_ReferenceQueue`** (line 25, set at line 44
   in `Java_org_jpype_ref_NativeReference_init`) - **flagged, not fixed
   this pass.** This is a file-scope `static jobject` that gets silently
   overwritten every time a *new* `NativeReferenceQueue` is initialized
   (i.e. every context/subinterpreter start) - the same singleton-clobber
   shape of bug as `m_ProxyType`, except worse because it's process-wide
   static state standing in for what should be per-context state, and each
   overwrite leaks the previous global outright. This is the same
   reference-queue subsystem that had a larger rework (`JPReferencePool`,
   `NativeReferenceQueue`/`ReferenceSet` Java-side changes) attempted and
   then explicitly reverted earlier this session (see prior conversation
   summary: "You are likely goign to have to revert as you are so far off")
   - fixing it properly means either (a) making it a real per-context field
   reached through `JPContext` instead of a file-scope static, or (b)
   routing it through the same `jref`/`GlobalPool` mechanism. Given the
   size of the earlier revert, this deserves its own dedicated session
   rather than folding it into this NewGlobalRef cleanup pass - scoping it
   here so it isn't lost, but the recommendation is: **do it separately,
   after this plan's A/B/C-minus-this-item work lands and is verified.**

## Suggested execution order

1. Category B immortals (3 independent, low-risk file-local changes:
   `jp_classloader.cpp`+`.h`, `jp_gc.cpp`+`.h`, `jp_context.cpp`). Each is
   self-contained - do one, rebuild (`ninja -C /home/kenelson/jcef/jpype .`),
   run the full suite, move to the next. Do **not** batch all three before
   testing - this session already burned time once from batching too many
   `JPValue` call-site edits before running the suite and having to
   backtrack through several files to find two misses.
2. Category C item 1 (`JPProxy::m_ProxyType`) - the user's named example,
   moderate size, self-contained to `jp_proxy.cpp`/`jp_proxy.h`.
3. Category A release wiring (`detachJVM()` additions) - low-risk
   mechanically, but double-check the `m_TypeManager->m_JavaTypeManager`
   double-free question before adding its release call.
4. Category C item 2 (`pyjp_package.cpp`) - smaller, self-contained.
5. Category C item 3 (`jp_reference_queue.cpp`) - explicitly deferred to a
   separate session per above.

## Verification

After each step: `ninja -C /home/kenelson/jcef/jpype .` (incremental native
rebuild) then `cd native/jpype_module && mvn -q clean test
-Dpython.executable=python3.10` (full Java+embedded-Python suite, expect
475/475, 0 failures/errors - this is the baseline this plan starts from).
For the Category A/C work specifically, the real test is repeated
subinterpreter start/stop (once `startSubInterpreter`/`finishSub` are wired
up per task #25) without unbounded global-ref growth - that's the actual
motivating scenario for all of this, not just passing the existing suite
(which never exercises more than one interpreter today).
