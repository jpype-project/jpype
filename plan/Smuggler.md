# Smuggler: detect and safely reject/convert cross-interpreter Proxy calls

## Status (2026-07-11): step 1 DONE (committed `c6e243a7`), remainder not
started. `JPClass::convertToPythonObject` (`native/common/jp_class.cpp`)
now detects a proxy's owning-context mismatch and raises `RuntimeError`
instead of corrupting memory, verified against a real Python 3.12 own-GIL
subinterpreter (`SubInterpreterNGTest.testSmuggledProxyAcrossInterpretersThrows`).
The exception-unpack edge case (a smuggled proxy inside a Java exception
object being converted inside exception-handling itself) is also fixed,
wrapped in a local try/catch (`JPypeException::convertJavaToPython`,
`native/common/jp_exception.cpp`). **Not started:** the "advanced"
string-round-trip conversion path for convertible types (jump locks to the
owning interpreter, convert, return) described below - still just a
design, no code. Follow-on to [[jpype_multiphase_init_status]]/
`plan/MultiPhaseInit.md` (COMPLETE) - own-GIL subinterpreter isolation is
now real, which is precisely what makes this bug reachable/dangerous
rather than theoretical.

## The bug

A `PyJPProxy` (a Java-side dynamic proxy backing a Python callable/object,
`native/common/include/jp_proxy.h`) is created inside one interpreter and
carries a `JPContext* m_Context` plus `PyJPProxy* m_Instance` (the actual
Python target object) that both belong to the interpreter that registered
it. If that Java proxy reference is then invoked from a *different*
interpreter (a second subinterpreter, or main, running under its own GIL
now that per-interpreter-GIL isolation is real per
`plan/MultiPhaseInit.md`), `hostInvoke`/`getArgs`
(`native/common/jp_proxy.cpp`) will touch interpreter A's `PyObject`s and
allocator arena while holding interpreter B's GIL. This is memory
corruption, not just a wrong-answer bug — cross-interpreter object access
without holding the owning interpreter's GIL is exactly the class of
unsafety `Py_MOD_PER_INTERPRETER_GIL_SUPPORTED` isolation exists to
prevent, and JPype's Java-side proxy layer currently has no guard against
it at all.

**Reproduction shape** ("smuggler" name comes from this): register a
`JProxy`-backed Java object in interpreter A, hand the *Java* reference
(not the Python object - the whole point is the Java side has no
interpreter tag on it once it's just a `jobject`) across to interpreter B
(e.g. via a shared static, a JNI global, or just Java code running with
access to both), then call a proxy method on it while executing inside
interpreter B. Today: silent corruption or crash. Desired: detected and
handled.

## Desired behavior

1. **Detect**: at the point `hostInvoke` is about to touch `m_Instance`,
   compare the proxy's owning context/interpreter (`m_Context`, or
   whatever interpreter-identity field ends up being the right check -
   `PyJPModuleState::interp_state` is the existing per-interpreter
   identity field, see `pyjp.h`) against the interpreter currently holding
   the GIL on the calling thread. Mismatch = smuggled proxy.
2. **If the call is convertible** (the plan's own phrase: arguments/return
   value are of a small set of context-free-safe types - the stated
   example is strings, likely extends to other primitives/immutables that
   have no interpreter-bound identity): "jump the locks" - i.e.
   temporarily acquire the *owning* interpreter's GIL (release the
   calling interpreter's), perform the real call and any needed
   string-round-trip conversion there, then return to the calling
   interpreter's GIL to hand back the (interpreter-agnostic, since it's a
   string) result. This is deliberately narrow: only for types that are
   safe to reconstruct fresh on the other side, not for handing raw
   `PyObject*`s across the boundary.
3. **If not convertible** (arbitrary Python objects, anything with
   interpreter-bound identity): throw a real exception (Java-side,
   presumably something under `python.exceptions` or a new
   `JPypeException` subtype) instead of corrupting memory or crashing.

## Open questions / not yet designed

- Exact detection point and identity check - `hostInvoke` in
  `native/common/jp_proxy.cpp` is the obvious call site, but the
  cheapest-correct check (context pointer compare? interpreter-state
  pointer compare? something already computed by the multi-phase-init
  work?) hasn't been picked.
- Which types count as "convertible" beyond strings - needs an explicit
  list, not an open-ended "if it looks safe" heuristic.
- What "jump the locks" means precisely in terms of `PyGILState_Ensure`/
  `PyThreadState_Swap`/per-interpreter thread state APIs, and whether it's
  even safe to hold two interpreters' GILs sequentially on one OS thread
  without tripping over the same lifecycle hazards already fought through
  in [[jpype_subinterpreter_difficulty]] (Clear/Swap ordering, orphaned
  root tstate).
- Performance/complexity cost of the mismatch check on *every* proxy
  invocation (this is a hot path) - needs to be cheap in the common
  (non-smuggled) case.
- Whether this is purely a defense (throw on mismatch, no conversion
  path) as a first cut, with the string-round-trip smuggling as a
  stretch goal - user explicitly called the conversion path "advanced
  use case," suggesting phased scope: exception-on-mismatch first,
  conversion later.

## Non-goals

- Not attempting to make arbitrary Python objects transparently shareable
  across interpreters - that's what `plan/MultiPhaseInit.md`'s "Known
  non-goal" section already draws the line at (single shared JVM, not
  full multi-JVM-per-process), and this plan doesn't relitigate that.
