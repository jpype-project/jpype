# Plan: convert `_jpype` to multi-phase init (`Py_MOD_PER_INTERPRETER_GIL_SUPPORTED`)

## Status (2026-07-11): COMPLETE. Steps 1-4 all done (committed 3644e70e on
`reverse`), verified against the real reverse-bridge test suite (514/514,
python3.10), and the actual proof-of-goal check passed under
`python3.12-dbg`: `_jpype` now imports successfully inside a real isolated
(`check_multi_interp_extensions=True`) subinterpreter, where it — and any
single-phase-init extension — would previously have been refused. See
"Verification" at the bottom for the remaining lower-priority follow-ups
(Java-side `python3.12-dbg` mvn run, a native own-GIL test using
`Py_NewInterpreterFromConfig` directly) that would round this out further
but aren't required to call the stated goal met.

## Context

Subinterpreters currently work in **legacy-style** mode only: shared process
GIL, shared object allocator (`use_main_obmalloc=1`,
`check_multi_interp_extensions=0`), per [[jpype_subinterpreter_difficulty]]
(resolved for lifecycle correctness — start/register/eval/close verified
with two subinterpreters + main running concurrently, no cross-talk, no
hangs). The reason it's legacy-style rather than true PEP 684 own-GIL
isolation is that `_jpype` is a **single-phase-init** extension
(`PyModule_Create(&moduledef)` called directly in `PyInit__jpype`, no
`PyModuleDef_Slot` array) — CPython requires multi-phase init
(`Py_mod_exec` + declaring `Py_mod_multiple_interpreters` support) before an
extension is even eligible to opt into `Py_MOD_PER_INTERPRETER_GIL_SUPPORTED`.

The Java-core side of global-state removal is **already done** (see
[[jpype_final_mission_status]] / `plan/Globals.md`, DONE 2026-07-10,
475/475) — that work converted `native/common/`'s JNI global refs into
per-`JPContext` `jref`/`GlobalPool`-routed handles or true process-wide
immortals, and threads `JPContext` through explicitly rather than via a
singleton. This plan is the follow-on: the **Python C-extension side**
(`native/python/`) — the module-init mechanism itself, plus a handful of
smaller `native/python/`-local globals never audited by the Globals.md pass
because they're Python-C-API state, not JNI global refs.

An Explore survey (this session, file:line-grounded, no code changes made)
found the codebase already unusually well-prepared: **no static
`PyTypeObject`s to convert** (every module type is already a heap type
built via `PyType_FromSpec`/`PyType_FromSpecWithBases` and stored on
`PyJPModuleState`), and **no cached global state pointer** (every call site
fetches state via `PyModule_GetState()`). The actual remaining work is
narrow — one mechanical restructuring of the init function, plus cleanup of
a few small process-global variables.

## A. Module init structure — the actual single→multi-phase conversion

**DONE 2026-07-11.** Implemented as described below, with one refinement to
item 3 (the `Py_mod_gil` version-gating question is now resolved, see after
the numbered list). Verified: both normal and `-DENABLE_COVERAGE=ON` builds
compile clean against python3.10 (`Py_mod_multiple_interpreters`/
`Py_mod_gil` correctly compile out via the version guards); `import _jpype`/
`import jpype` work and `PyJPModule_exec` runs correctly end-to-end
(module state, all twelve `*_initType` calls, `JPContext` construction);
full `native/jpype_module` reverse-bridge suite (`mvn -o test
-Dpython.executable=python3.10`, no `-Dtest=` filter) — **514 run, 0
failures, 0 errors, 6 skipped, no hangs** — confirming JVM startup, the
full Python-calling-Java and Java-calling-Python paths, and JVM shutdown
all still work correctly through the new `Py_mod_exec`-based init.

`native/python/pyjp_module.cpp:1202-1212` (original) — current `PyModuleDef`:
```cpp
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "_jpype", "jpype module",
    sizeof(PyJPModuleState), moduleMethods,
    nullptr,  // m_slots - THIS is what needs to become a real slot array
    PyJPModule_traverse, PyJPModule_clear, PyJPModule_free
};

PyMODINIT_FUNC PyInit__jpype()
{
    PyObject* module = PyModule_Create(&moduledef);   // single-phase call
    ...                                                 // all init work inline
    return module;
}
```

**Change**:
1. Add a `static PyModuleDef_Slot jpype_slots[] = { {Py_mod_exec,
   PyJPModule_exec}, {Py_mod_multiple_interpreters,
   Py_MOD_PER_INTERPRETER_GIL_SUPPORTED}, {0, nullptr} };` and set
   `moduledef.m_slots = jpype_slots` (replacing the `nullptr`).
2. Split `PyInit__jpype` into two functions:
   - `PyInit__jpype()` becomes a thin `return PyModuleDef_Init(&moduledef);`
   - A new `PyJPModule_exec(PyObject *module)` (signature: `int(PyObject*)`,
     returns 0 on success / -1 on failure per `Py_mod_exec` contract) takes
     everything currently in the body of `PyInit__jpype`
     (`pyjp_module.cpp:1227-1284`: state zeroing, `module_dict`/
     `interp_state`/`is_main_interpreter` setup, `__version__`,
     `__builtins__`, `strings_dict`/`Py_JP_CALL`/`class_magic` setup, the
     twelve `*_initType` calls, `st->context = new JPContext(st)`) — same
     logic, just relocated and adjusted to return `int` instead of
     `PyObject*` (error paths become `return -1;` instead of `return
     nullptr;`, no `Py_DECREF(module)` needed since `Py_mod_exec` failure is
     handled by the interpreter, not the callee).
3. **`Py_GIL_DISABLED`/`PyUnstable_Module_SetGIL` call**
   (`pyjp_module.cpp:1222-1224`, original) — **resolved, done as a slot.**
   Checked both `/usr/include/python3.12/moduleobject.h` and a bleeding-edge
   CPython checkout's `moduleobject.h`: `Py_mod_multiple_interpreters`/
   `Py_MOD_PER_INTERPRETER_GIL_SUPPORTED` are gated
   `!defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030c0000` (3.12+);
   `Py_mod_gil`/`Py_MOD_GIL_NOT_USED` are gated at `0x030d0000` (3.13+) —
   **different thresholds, confirming the plan's suspicion they're
   independently versioned.** But no extra `PY_VERSION_HEX` guard was
   needed for the `Py_mod_gil` slot specifically: it's already wrapped in
   the pre-existing `#ifdef Py_GIL_DISABLED`, and `Py_GIL_DISABLED` is only
   ever defined by a compiler building against free-threaded CPython
   headers, which are always >=3.13 — so that ifdef transitively implies
   the needed version floor. `Py_mod_multiple_interpreters` (3.12, not
   free-threading-specific) does need its own explicit
   `#if PY_VERSION_HEX>=0x030c0000` guard since it can be reached while
   building against plain (non-free-threaded) 3.10/3.11 headers where the
   macro doesn't exist — confirmed no such macro in
   `/usr/include/python3.10/`. Both slots now declared statically in
   `jpype_slots[]`; the old post-hoc `PyUnstable_Module_SetGIL()` call
   after `PyModule_Create()` was removed entirely.
4. `moduledef.m_size = sizeof(PyJPModuleState)` is **already correct** for
   multi-phase init (per-module-instance allocation, effectively
   per-interpreter) — no change needed.

## B. Static `PyTypeObject` → heap type conversions

**None needed.** Confirmed via grep — no `static PyTypeObject`/file-scope
`PyTypeObject` struct literals exist anywhere in `native/python/`. Every
module-level type (`PyJPClass_Type`, `PyJPObject_Type`,
`PyJPException_Type`, `PyJPComparable_Type`, `PyJPArray_Type`,
`PyJPArrayPrimitive_Type`, `PyJPBuffer_Type`, `PyJPChar_Type`,
`PyJPField_Type`, `PyJPMethod_Type`, `PyJPMonitor_Type`, `PyJPProxy_Type`,
`PyJPNumberLong_Type`/`Float`/`Bool`, `PyJPClassHints_Type`,
`PyJPPackage_Type`) is built via `PyType_FromSpec`/
`PyType_FromSpecWithBases` inside a `*_initType(module, st)` call and stored
exclusively on `PyJPModuleState` (`native/python/include/pyjp.h:146-162`) —
already correct for multi-phase init, since `Py_mod_exec` re-runs these
`*_initType` calls once per interpreter, producing a fresh heap type per
interpreter as required.

The `PyType_Spec`/`PyType_Slot`/`PyMethodDef`/`PyGetSetDef` tables
themselves correctly remain `static` at file scope — immutable read-only
metadata, exactly CPython's own recommended pattern. No change.

One thing to retest under multi-phase init specifically: the pre-3.12
fallback path `PyJPClass_FromSpecWithBases`
(`native/python/pyjp_class.cpp:83-140`), which hand-builds a heap type off
`st->PyJPClass_Type->tp_alloc` to smuggle extra Java-slot space onto
arbitrary objects. Already per-`st`/heap-allocated, so structurally fine,
but it pokes at `tp_*` slots directly and should get explicit test coverage
once `Py_mod_exec` starts running this per-interpreter instead of once at
process startup.

## C. Other global/process-mutable state to fix

1. **DONE 2026-07-11.** `native/python/pyjp_module.cpp:386` —
   `static string jarTmpPath;`, written in `PyJPModule_startup`, read/deleted
   in `PyJPModule_shutdown`. Genuine cross-interpreter race: two
   subinterpreters each starting a JVM via the jar-path launch option would
   clobber each other's temp path. Moved to `PyJPModuleState` — but **not**
   as a plain `std::string` member as originally planned. `PyJPModuleState`
   is `memset`-zeroed into existence (`PyInit__jpype`,
   `pyjp_module.cpp:1227`) and freed with no destructors ever running
   (`PyJPModule_free`) — every non-POD member already in the struct
   (`context`) is a heap pointer, manually `new`/`delete`d, precisely to
   avoid embedding an object that needs real construction/destruction
   into memory that gets neither. Embedding `std::string` by value would
   have been undefined behavior (`operator=` on a zeroed-but-never-
   constructed object). Followed the existing `JPContext*` convention
   instead: `std::string* jarTmpPath;`, `new`'d in `PyJPModule_startup`,
   `delete`'d (and read) in `PyJPModule_shutdown`, with a defensive
   `delete`+null in `PyJPModule_free` too in case shutdown was never
   called. Verified: compiles clean in both normal and
   `-DENABLE_COVERAGE=ON` builds.

2. **DONE, no code change needed — resolved as option (a).**
   `native/python/pyjp_module.cpp:986` — `int _PyJPModule_trace;`
   (`extern "C"` in `native/common/include/jp_tracer.h:88`, consumed by
   `native/common/jp_tracer.cpp:87-193`), toggled by the debug-only
   `trace()` Python method. The existing code comment already documents
   the decision: *"used only in debugging and does not need to be
   subinterpreter safe."* Same reasoning as
   [[jpype_fault_injection_global_by_design]] — a shared, documented,
   debug-only knob is fine; CPython's multi-phase-init contract is about
   the module's user-facing correctness across interpreters, not about
   eliminating every debug hook. Nothing further to do here.

3. **DONE 2026-07-11.** `native/python/pyjp_array.cpp:478` — dead
   file-scope `PyTypeObject *PyJPArrayPrimitive_Type = nullptr;`, never
   read (the real value lives at `st->PyJPArrayPrimitive_Type`). Deleted.

4. **DONE 2026-07-11, but not a simple delete — see item 5.** The plan's
   original read (delete the header-scope global, `st->fault_code` is "the
   one working mechanism") was wrong: `st->fault_code` was itself
   unreachable dead state. `PyJPModuleFault_check`/`_throw` are called from
   deep inside `native/common/` call frames via the `JP_TRACE_IN`/
   `JP_FAULT_RETURN`/`JP_BLOCK` macros (`native/common/include/jpype.h:87-91`),
   which have no `PyObject* module`/`st` in scope — so routing the fault
   trigger through per-interpreter module state was never actually
   wireable, which is exactly why the two broken blocks in item 5 existed
   half-finished. **User confirmed this is intentional and correct as a
   single process-wide global**: fault injection is test-only (used to
   force otherwise-unreachable error paths, e.g. `_jpype.fault("Name")`
   from `test_numeric.py`/`test_jchar.py`), never runs in production
   (`JP_INSTRUMENTATION` is coverage-build only), and is single-threaded
   test infra — no per-interpreter isolation is needed or wanted here. Net
   change: deleted the ODR-violating header-scope `int fault_code = 0;` in
   `pyjp.h`, deleted the `uint32_t fault_code;` member from
   `PyJPModuleState`, and repointed `PyJPModule_fault`
   (`pyjp_module.cpp:1000-1015`, the Python-facing `_jpype.fault()` setter)
   at the same global instead of `st->fault_code`.

5. **DONE 2026-07-11.** `native/python/pyjp_misc.cpp` had two consecutive
   `#ifdef JP_INSTRUMENTATION` blocks both (re)defining
   `PyJPModuleFault_check`/`_throw`, one referencing an undeclared `st`, the
   other an undeclared `_PyJPModule_fault_code` — neither compiled. Since
   `JP_INSTRUMENTATION` is defined whenever `-DENABLE_COVERAGE=ON` is set
   (`CMakeLists.txt:87`), **the coverage build (`.azure/scripts/coverage.yml`)
   was broken** — confirmed by reproducing the failure locally before this
   fix. Replaced both with one working definition backed by the process-wide
   `_PyJPModule_fault_code` global (see item 4), moved to file scope *outside*
   the file's `extern "C" { ... }` JNI-export block (`pyjp_misc.cpp:29-249`) —
   the original dead blocks were physically inside that block, which would
   have been a second compile error (C linkage vs. the C++-linkage
   `extern void PyJPModuleFault_throw(uint32_t)` declaration in
   `native/common/include/jpype.h:87-88`) once the undeclared-identifier
   errors were fixed. Verified: coverage build
   (`pip install -e . --config-setting cmake.args="-DENABLE_COVERAGE=ON"`)
   now compiles and links cleanly; `_jpype.fault("Name")`/`_jpype.fault(None)`
   exercised directly against the built module and confirmed working.

6. **`native/python/pyjp_module.cpp:1025-1035`** — `#ifdef ANDROID`
   `PyJPModule_bootstrap` references an undefined `JPContext_global`
   (a would-be process-global `JPContext*` singleton — grep confirms it's
   never declared/defined anywhere) and an undeclared `st`. Dead/
   non-compiling Android-only code path, evidently unbuilt/untested today.
   **Not part of this rewrite's scope** — flag only: if this path is ever
   resurrected, `JPContext_global` is precisely the anti-pattern
   per-interpreter isolation requires eliminating, and it should be
   rewritten to thread state through like the rest of the file (mirroring
   `st->context`) rather than fixed as a singleton.

## D. Already fine / no change needed

- `PyJPModuleState` fetched exclusively via `PyModule_GetState()` at every
  call site (`pyjp_module.cpp:67-75`) — no cached global state pointer
  exists anywhere.
- `JPContext` is allocated once per module-init call
  (`st->context = new JPContext(st)`) and threaded through explicitly on
  the `native/common/` side via `context->modulestate` at every relevant
  call site (`jp_bridge.cpp`, `jp_typefactory.cpp`, `jp_exception.cpp`,
  `jp_reference_queue.cpp`, `jp_class.cpp`, `jp_classhints.cpp`,
  `jp_functional.cpp`, `jp_chartype.cpp`, `jp_booleantype.cpp`) — no
  singleton reach-through.
- GIL/thread-state handling (`jp_pythontypes.cpp:406,418,438,475,496,501`)
  uses `PyGILState_Ensure/Release` and `PyEval_SaveThread/RestoreThread`,
  both thread-state-scoped APIs that already operate correctly against
  whichever interpreter's thread state is current — no change needed for
  multi-phase init itself (this is the exact code already fixed for
  lifecycle correctness this session, see [[jpype_gil_reacquire_bug]]).
- No C-side module-global Python exception singletons exist.
  `PyJPException_Type` is a heap type (see B) built with `PyExc_Exception`
  (a CPython static *builtin* — explicitly permitted as a base; builtin
  static types are shared read-only singletons by design) plus
  `st->PyJPObject_Type`.
- `allocSpec`/`local_alloc_type` (`pyjp_value.cpp:328-398`) is deliberately
  thread-local (`PyThreadState_GetDict()`), not a global cache — already
  correctly scoped.
- `PyJPModule_loadResources`/`_clearResources`/`_ready`/`_traverse`/
  `_clear`/`_free` all operate exclusively on their `st`/`module`
  parameters — already structurally ready to run once per interpreter under
  `Py_mod_exec` with no modification.

## Known non-goal / hard external constraint

The JNI specification only supports **one JVM per process** in practice —
`JNI_CreateJavaVM`/`JNI_GetCreatedJavaVMs` are resolved per-`JPContext` on
the `native/common/` side, with no `JavaVM*` cache in `native/python/`, but
this is a constraint of the JNI layer itself, not something fixable by this
rewrite. Even after `_jpype` is proven per-interpreter-GIL-safe at the
Python-C-API level, running JPype in more than one subinterpreter
*simultaneously, each independently starting its own embedded JVM*, remains
bounded by this. **Scope of this plan is limited to making `_jpype` a
correct, safe multi-phase-init extension** (i.e., what CPython requires to
even offer `Py_MOD_PER_INTERPRETER_GIL_SUPPORTED`); it does not promise
multi-JVM-per-process, which is a separate, likely much harder problem (or
possibly a non-problem if the existing single-JVM-shared-across-
subinterpreters model, already working today per
[[jpype_subinterpreter_difficulty]], remains the intended usage pattern —
own-GIL isolation is valuable even with one shared JVM, since it removes
Python-level cross-talk risk between subinterpreters' non-Java state).

## Suggested execution order

1. **DONE 2026-07-11.** Item C.3/C.4/C.5 dead-code cleanup — zero-risk,
   immediately makes the subsequent grep/audit trail cleaner (no noise from
   dead globals that look real but aren't). Not committed yet — sitting as
   a working-tree diff across `pyjp_array.cpp`/`pyjp.h`/`pyjp_misc.cpp`/
   `pyjp_module.cpp`; commit when ready.
2. **DONE 2026-07-11.** Item C.1 (`jarTmpPath` → `PyJPModuleState`) — small,
   mechanical, independently testable/committable.
3. **DONE 2026-07-11.** Item A (the actual `PyModuleDef_Slot`/
   `PyJPModule_exec` split) — the core rewrite. Not yet committed —
   everything from steps 1-3 is sitting together as one working-tree diff;
   commit when ready (could also be split into 3 commits matching the
   plan's numbered items, at the user's preference).
4. **DONE, no code change needed.** Item C.2 (`_PyJPModule_trace` policy
   decision) — resolved as option (a), already documented by the existing
   code comment.
5. **DONE, folded into step 3.** The `Py_GIL_DISABLED` slot question (A.3)
   is resolved — see item A above.

## Verification

**Done so far (2026-07-11, this session, python3.10 only):** normal and
`-DENABLE_COVERAGE=ON` builds both compile clean; `import _jpype`/
`import jpype` and `PyJPModule_exec` confirmed working directly; full
`native/jpype_module` reverse-bridge suite (no `-Dtest=` filter) — 514 run,
0 failures, 0 errors, 6 skipped, no hangs. This exercises JVM start/stop
(`PyJPModule_startup`/`_shutdown`, including the moved `jarTmpPath`) and a
wide swath of both bridge directions through the new init path.

**Proof-of-goal test DONE 2026-07-11 (`python3.12-dbg`, plain `pip install
-e .`, no coverage flag needed for this check):** built cleanly against
`python3.12-dbg`. Used the stdlib's `_xxsubinterpreters.create(isolated=True)`
(maps to `check_multi_interp_extensions=True` in 3.12's C implementation —
the exact flag that gates whether a single-phase-init extension is refused)
to create a real isolated subinterpreter, then imported `_jpype` inside it
via `_xxsubinterpreters.run_string()`: **succeeded**
(`IMPORT OK in isolated subinterpreter: 1.7.2.dev0`). Confirmed this is a
meaningful test, not a no-op: same isolated subinterpreter genuinely
rejects a real single-phase-init extension
(`_testsinglephase`, CPython's own test fixture for this exact scenario,
found pre-built from an earlier session's `python3.12` build directory)
with `ImportError: module _testsinglephase does not support loading in
subinterpreters` — precisely the failure `_jpype` itself would have hit
before this rewrite. This is the actual evidence the rewrite achieved its
goal: CPython now accepts `_jpype` under `check_multi_interp_extensions=True`,
which was refused pre-rewrite.

Not done this session (would need more setup/time, lower priority now that
the core proof above is in hand):
- Full `mvn -q test -Dpython.executable=python3.12-dbg` (Java-side
  reverse-bridge suite under 3.12-dbg specifically, not just python3.10).
- Rerun the full `SubInterpreterNGTest` suite (7 tests, including the two
  stress tests added in an earlier session —
  `testMixAndMatchNoCrossTalkNoHangs`/`testConcurrentThreadDoesNotBlockAfterSubEval`)
  under `python3.12-dbg` — these are existing ground-truth Java-side
  subinterpreter lifecycle tests and should stay green, but they exercise
  the legacy-config path (`use_main_obmalloc=1`,
  `check_multi_interp_extensions=0`), not the new own-GIL-eligible path
  proven above via the stdlib test — a native/Java-side test using
  `Py_NewInterpreterFromConfig` with `.gil=PyInterpreterConfig_OWN_GIL`
  directly would be the more complete follow-up if this project wants to
  actually run JPype itself (not just prove eligibility) inside a true
  own-GIL subinterpreter.
- `python3.12-dbg`'s extra runtime assertions should stay in the loop for
  any future work here — own-GIL isolation bugs are exactly the class of
  memory-safety issue that build catches that a release build silently
  tolerates.
