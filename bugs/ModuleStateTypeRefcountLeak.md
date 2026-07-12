# Bug: module-state type caches double-decref'd at teardown (missing INCREF before PyModule_AddObject steal)

## Status (2026-07-10): FIXED, verified against python3.12-dbg. Needs upstream write-up/PR.

## Symptom

Under a debug CPython build (`python3.12-dbg`), plain `import _jpype` (no
subinterpreters, no unusual code paths) crashed at interpreter shutdown
(or immediately on an explicit `gc.collect()`) with:

```
../Modules/gcmodule.c:113: gc_decref: Assertion "gc_get_refs(g) > 0" failed: refcount is too small
object type name: _jpype._JClass
object repr     : <java class '_jpype._JBuffer'>
Fatal Python error: _PyObject_AssertFailed: _PyObject_AssertFailed
```

A regular (non-debug) build never asserts this - the refcount underflow
is silently tolerated by release-mode CPython, which is why this had
gone unnoticed. It was originally mistaken (in an earlier session) for a
subinterpreter-specific SIGSEGV inside `libpython3.12.so`; it is not
subinterpreter-specific, it reproduces in the plain single-interpreter
case as soon as anything triggers a GC pass while `_jpype` is loaded.

## Root cause

Every `PyJPXxx_initType()` function in `native/python/*.cpp` creates a
heap type, stores the sole owned reference in `PyJPModuleState` (e.g.
`st->PyJPBuffer_Type`), then calls `PyModule_AddObject(module, "_JXxx",
(PyObject*) st->PyJPXxx_Type)`. **`PyModule_AddObject` steals a
reference on success.** After that call, the module's `__dict__` is the
sole real owner (refcount contribution: 1), but `st->PyJPXxx_Type` is
still treated everywhere else as an independently-owned strong
reference:

- `PyJPModule_traverse` (`native/python/pyjp_module.cpp:1060`) does
  `Py_VISIT(st->PyJPXxx_Type)` for every type, claiming an edge from the
  module object itself (separate from the module `__dict__`'s own edge).
- `PyJPModule_clear` (`native/python/pyjp_module.cpp:249`) does
  `Py_CLEAR(st->PyJPXxx_Type)`, issuing an *extra* decref beyond the one
  the module `__dict__`'s own teardown already performs.

Both of these assume the module state holds its own incref, which most
`_initType` functions never took. The result is a real double-decref /
potential use-after-free at module teardown in every build (debug and
release); the debug build's GC just happens to assert on the refcount
bookkeeping mismatch before anything gets freed twice.

Confirmed via direct C-level inspection (`gc.get_referrers`, wrapper
descriptor `__objclass__` checks) that `_JBuffer`'s type object had
exactly this shape: real edges from the module `__dict__` entry, from
`PyJPModule_traverse`'s direct visit, and from its own three built-in
wrapper descriptors (`__repr__`, `__buffer__`, `__release_buffer__`,
each holding a real `d_type` ref) - four-plus GC-tracked incoming edges
against a total `ob_refcnt` that only actually accounted for one
independent module-state ownership, not two.

Two existing call sites in the codebase already do this correctly
(`Py_INCREF` placed either just before or just after the
`PyModule_AddObject` call, to compensate for the steal) -
`PyJPPackage_initType`'s `_JPackage`/`_packages`
(`native/python/pyjp_package.cpp:408-416`) and `_strings`
(`native/python/pyjp_module.cpp:1259-1261`) - confirming this is the
intended, just inconsistently-applied, pattern.

## Sites fixed (were missing the compensating `Py_INCREF`)

- `native/python/pyjp_class.cpp:1099` - `st->PyJPClass_Type` (`_JClass`,
  the custom metaclass used for every Java-backed type - the highest-
  impact of the five, since every other type is an instance of it)
- `native/python/pyjp_buffer.cpp:148` - `st->PyJPBuffer_Type` (`_JBuffer`
  - the type that happened to surface the assertion first)
- `native/python/pyjp_field.cpp:141` - `st->PyJPField_Type` (`_JField`)
- `native/python/pyjp_char.cpp:631` - `st->PyJPChar_Type` (`_JChar`)
- `native/python/pyjp_classhints.cpp:170` - `st->PyJPClassHints_Type`
  (`_JClassHints`)

All five now follow the same pattern already used by
`PyJPPackage_initType`/`_strings`: `Py_INCREF((PyObject*) st->X_Type)`
immediately adjacent to the `PyModule_AddObject` call.

## Sites already correct (no change needed)

`pyjp_method.cpp`, `pyjp_monitor.cpp`, `pyjp_number.cpp` (x3),
`pyjp_object.cpp` (x3), `pyjp_proxy.cpp`, `pyjp_array.cpp` (x2) already
had the compensating `Py_INCREF` (placed before their
`PyModule_AddObject` call) - audited and confirmed correct, not touched.

## Verification

- `PYTHONPATH=. python3.12-dbg -c "import _jpype"` - clean exit (was:
  `Fatal Python error: _PyObject_AssertFailed`, abort).
- `PYTHONPATH=. python3.12-dbg -c "import _jpype, gc; gc.collect()"` -
  clean exit (was: same assertion, triggered immediately instead of
  waiting for shutdown).
- Full `mvn -o test` suite still needs a re-run against the primary
  python3.10 build to confirm zero regression (release-mode CPython
  never asserted on this, so no prior passing/failing test result
  changes are expected either way - this was invisible to the existing
  suite).

## Why this matters beyond the debug-build assertion

Because this is a real double-decref, not just a debug-only false
positive: in a release build the same sequence of events can free the
type object once when the module `__dict__` is cleared, then any code
that runs during `PyJPModule_clear`'s `Py_CLEAR(st->PyJPXxx_Type)` (or
anything that reads `st->PyJPXxx_Type` after the first free but before
the second) is a use-after-free. This is a plausible contributor to
other previously-unexplained crashes during interpreter/subinterpreter
teardown (see `plan/Globals.md` and the subinterpreter work referenced
in memory `jpype-subinterpreter-difficulty`), not just this specific
debug-build assertion. Worth flagging upstream (or fixing more broadly)
independent of the subinterpreter effort.

## Next step

Continue the original task #25 investigation (subinterpreter class
registration) now that this masking bug is fixed - a *new*, distinct
GIL-state assertion has surfaced in its place:

```
GIL still held on calling thread after TypeManager.init() - leaked GIL acquire in class definition path.
Fatal Python error: _PyInterpreterState_GET: the function must be called with the GIL held, ... but the GIL is released (the current Python thread state is NULL)
```

happening in `jpype/_jclass.py:99 __new__` during
`_core.py:428 initializeResources` inside the subinterpreter. This is a
separate bug from the one documented here and is under active
investigation.
