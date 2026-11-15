[DONE 2026-07-10] Complete the subinterpret debugging which closes the largest risk item
  - Fixed real crash: JPPyCallAcquire destructor cleared thread state after
    swapping it away instead of before (commit 83e62a2d)
  - Fixed real leak: orphaned root thread state broke Py_EndInterpreter on
    close (same commit)
  - Fixed isGilHeld() false positive on the subinterpreter path (a808d4d2)
  - Proved no cross-talk/hangs with main + 2 subinterpreters running
    interleaved, and proved the GIL is genuinely released via a concurrent
    second-thread test (a42e1d21)
  - Still legacy-style (shared GIL/allocator with main), not full PEP 684
    own-GIL isolation - that would need a multi-phase-init rewrite of _jpype

[DONE 2026-07-11] Multi-phase init rewrite of _jpype for true PEP 684
own-GIL subinterpreter support - see plan/archive/MultiPhaseInit.md
  - Java-core global removal (plan/archive/Globals.md) already done - this is the
    remaining Python-C-extension-side gap
  - Steps 1-3 all DONE, committed 3644e70e on `reverse`:
    1. Dead-code cleanup: deleted dead PyJPArrayPrimitive_Type ptr; fixed
       (not deleted) the fault-injection instrumentation - the two broken
       blocks in pyjp_misc.cpp referenced undeclared identifiers and
       didn't compile under -DENABLE_COVERAGE=ON, breaking the coverage CI
       build. User confirmed fault_code should stay a single process-wide
       global (test-only, never in production, single-threaded) rather
       than per-interpreter PyJPModuleState.
    2. jarTmpPath moved off a file-scope static into PyJPModuleState - but
       as a heap `std::string*` (new/delete, same pattern as `context`),
       not embedded by value, since PyJPModuleState is memset-constructed
       with no destructors ever run on free - embedding a std::string
       object would have been UB. The plan's original text calling for a
       plain `std::string jarTmpPath;` member was wrong; caught before
       committing.
    3. The actual PyModuleDef_Slot/PyJPModule_exec split - done per plan,
       plus resolved the open A.3 question: Py_mod_multiple_interpreters
       (3.12+) needed its own PY_VERSION_HEX guard, Py_mod_gil (3.13+)
       didn't need one since it's already inside the pre-existing
       Py_GIL_DISABLED ifdef which implies 3.13+ headers.
  - Verified (python3.10 only): normal + coverage builds compile clean;
    import works; full native/jpype_module reverse-bridge suite (mvn -o
    test -Dpython.executable=python3.10, no filter) - 514 run, 0 failures,
    0 errors, 6 skipped, no hangs.
  - Item C.2 (_PyJPModule_trace policy) DONE, no code change needed -
    already documented in the code as intentionally shared/debug-only,
    same reasoning as fault_code.
  - Proof-of-goal test DONE 2026-07-11 under python3.12-dbg: built clean;
    `_xxsubinterpreters.create(isolated=True)` (-> check_multi_interp_
    extensions=True) then `import _jpype` inside it succeeded. Confirmed
    meaningful (not a no-op) by showing the same isolated subinterpreter
    genuinely rejects CPython's own single-phase-init test fixture
    (_testsinglephase) with the exact ImportError _jpype itself would have
    hit pre-rewrite. This is the real evidence the rewrite achieved its
    goal.
  - Lower-priority follow-ups not done this session (see plan doc): full
    mvn suite specifically under python3.12-dbg; a native/Java-side test
    using Py_NewInterpreterFromConfig with .gil=PyInterpreterConfig_OWN_GIL
    directly (to actually run JPype inside a true own-GIL subinterpreter,
    not just prove eligibility).

[DONE 2026-07-10] Resync with main - see plan/archive/Resync.md
  - Corrected mid-flight: `review` was never the mainline (frozen LLNL
    snapshot); real target was upstream/master
  - spi folded into reverse (clean merge, 482/482), squashed to a single
    commit, rebased onto upstream/master (2 real conflicts resolved:
    CHANGELOG.rst content merge, pyjp_array.cpp JPArray(frame,...)
    signature vs upstream's #845 slice-clone fix - combined both, plus a
    follow-up fix for a stale PyJPArray_Type symbol caught by a real
    native rebuild), squashed again to one commit on upstream/master
    (47f97604), force-pushed to origin/reverse
  - origin/master fast-forwarded to upstream/master (9b0a4a39) and pushed
    cleanly (was a strict ancestor, no rewrite needed)
  - Side quest: reviewed pr/pr1385 (the "overloads" PR) - CodeQL's
    "useless comparison" flag on MethodResolution's Case 4 was real but
    benign (proven exhaustive-partition redundancy, verified via an
    instrumented exception through a new torture test suite, then
    removed); added TestVarArgsHierarchy.java + 12 test_overloads.py
    cases addressing the reviewer's "needs real tests" pushback; pushed
    to origin/overloads for review (commit 6fc84547)


[DONE 2026-07-10] SPI installer (eager + lazy per-module resolution) -
see plan/archive/SPI.md - implemented and live, python.io worked example both paths

[DONE 2026-07-10] Naming/typo cleanup across SPI + python.io - see
plan/archive/Naming.md - committed 444587f4, folded into reverse's squash,
verified live (no PyIoWrapperService/PyKeyArgs remnants)

New work from plans:
- [DONE 2026-07-11] Investigate FIXME in GlobalPool as there was a
  divergence from J2NI - see plan/archive/GlobalPool.md. Per-interpreter pool
  instances + central prefix router were already correct (J2NI has one
  process-wide pool because it's single-JVM-connection; jpype needs many
  independently-closeable pools for concurrent subinterpreters) - the
  actual bug was the internal allocator: fully-synchronized ArrayList
  instead of J2NI's lock-free block/bitmask VarHandle design. Benchmarked
  both head-to-head (16 cores): sync get() collapsed under contention
  (39M ops/s at 1 thread -> 13M at 4 threads) while block/bitmask scaled
  to 575M ops/s at 16 threads. Ported the allocator in, kept the
  prefix/registry contract unchanged.
  - Also caught and removed a pre-existing per-slot generation counter
    that was dead-weight safety - a handle is only ever resolved through
    its live owning C++ wrapper, which is also the only thing that ever
    releases it (exactly once, from its own destructor), so slot reuse
    after release is safe by construction, same guarantee level as a
    plain JNI global ref slot.
  - Then corrected again: the user's actual intent for "generation bits"
    was a per-*pool* identity stamp guarding the 16-bit prefix wrapping
    around after 65536 pool constructions and colliding with a later
    pool - a real routing-error risk, not covered by the lifetime
    argument above. Added: each pool hashes its owning interpreter's
    native context pointer once at construction into the bits the
    per-slot generation vacated; tryRelease/get check it, log+no-op on
    mismatch. Zero extra runtime cost.
  - New test: GlobalPoolNGTest (11 tests) incl. a concurrent stress test
    and a real 70k-pool prefix-wraparound test. Full suite 555/555, 0
    failures/errors.
- [DONE 2026-07-11] Finish IO (section C, the last open piece) - see
  plan/IO.md. Buffered rewrite of all four adapters (PyIOInputStream/
  PyIOOutputStream/PyIOReader/PyIOWriter): internal 8KB/8K-char buffer,
  refilled/flushed via a single bulk Python call; requests >= the buffer
  bypass it. Existing tests that checked state right after an unflushed
  write were updated to flush first (correct java.io.Buffered* semantics).
  - Benchmarked on the real bridge (not simulated), same code path both
    sides, differing only in buffer size: byte write 1164x, char read 778x,
    char write 33x, but byte read only 1.8x - PyBytes->byte[] extraction
    has no bulk helper (unlike PyString->String), so it still costs one
    bridge call per byte inside fill(); buffering only removed the
    read(n)-call overhead, not that. If byte-read throughput ever matters
    more, the real lever is a bulk PyBytes->byte[] helper, not more
    buffering.
  - New tests: PyIOBufferingNGTest (6, call-count assertions via a
    dynamic-proxy spy, part of the normal suite) + PyIOBufferingBenchmark
    (wall-clock, excluded from the normal suite by surefire's *Test.java
    default pattern, run explicitly with -Dtest=PyIOBufferingBenchmark).
  - Full suite: 535/535, 0 failures/errors, 14 pre-existing skips.
  - All of plan/IO.md's A/B/C/D/F are now done; only the explicitly-deferred
    $foo dispatch / third-party-provider items in section E remain, and
    they don't block anything.
- Squash merge again as we are preparing for alpha review branch on jpype-project
- Push at this point 

Post follow up:
- We need to set up source formatting spotify and clang as per the j2ni model, but we need to modify the C++ formatter to try to minimize whitespace damage to the existing code base
- It likely would be better if we reorganize to be both a maven and pip build at root. That means src/main/java src/main/cpp/(common,python) etc

[ALPHA-BLOCKING] The next two items are needed before the alpha review
branch goes out - users can't try the reverse bridge without docs for it:
- [DONE, discovered already complete 2026-07-11] JavaDoc for python.lang/
  python.io - see plan/archive/Javadoc.md. Every item in the plan
  (Audience 1 python.io/python.lang end-user docs, Audience 2
  WrapperService/Installer/SpiLoader/SpiResource provider-facing docs, the
  "attributes fromMap" typo, all plan/SPI.md and plan/IO.md Javadoc
  references) was already implemented in the tree (landed inside
  e5a93a46, TODO.md just hadn't been updated). Re-verified file by file
  against the plan's checklist plus a clean `mvn compile`; see
  plan/archive/Javadoc.md for the full verification list. Moved to
  archive.
- [DONE 2026-07-11] JavaDoc for python.exceptions - not covered by the
  original plan/Javadoc.md scope (that plan only covered python.lang/
  python.io). Found all 50 concrete exception classes plus
  PyException/PyBaseException had @author-only or empty Javadoc, and 36 of
  50 files were missing the license header entirely (PyWarning.java had
  neither). Added a one-line "Java front-end for Python's {@code X}." doc
  to all 52 classes (mechanical: Python's exception name is always the
  Java class name minus its "Py" prefix here) and the missing license
  headers; also trimmed a stray non-Javadoc "why exceptions get their own
  package" comment off the bottom of package-info.java (that file's real
  Javadoc was already good). Full suite green: 547/547, 0 failures, 14
  skipped (python3.10).
- [DONE 2026-07-11] python.collections was a stub, not a doc gap - see
  plan/Collections.md. Implemented PyDeque/PyOrderedDict/PyDefaultDict/
  PyCounter as real interfaces, wired via a WrapperService SPI provider
  (PyCollectionsWrapperService, mirroring PyIOWrapperService) plus one
  .pyspi resource per class, a PyCollections factory (mirrors IO.using()),
  Audience-1 Javadoc, and per-type NGTest classes. Caught and fixed two
  design bugs during verification (not just wiring bugs): (1)
  OrderedDict.moveToEnd(key)'s 1-arg default didn't reach a Python-side
  default for `last` - JPype's proxy dispatch routes by Python attribute
  name, so a Java default method's hardcoded second argument never reaches
  the SPI lambda; the default has to live in the .pyspi Python function
  itself (same pattern io.IOBase.pyspi's seek already uses). (2)
  PyCounter's Javadoc wrongly assumed inherited PyDict.get() already
  returned 0 for a missing key - it doesn't, dict.get() keeps plain-dict
  semantics (null) regardless of subclass; only __getitem__/__missing__
  gives Counter's zero-default, so added a dedicated getCount() method
  backed by `x[key]`. Full suite green on both python3.10 (582/582, 14
  pre-existing skips) and python3.12 (582/582, 0 skips).
- [SCOPED, not started] Broader stdlib SPI survey 2026-07-11: with
  python.io done and python.collections in flight, user asked what other
  stdlib packages deserve the same WrapperService treatment, to prove the
  SPI mechanism generalizes past a single provider without conflicts.
  Ranked by value/effort, each scoped as its own plan/ doc (same shape as
  plan/Collections.md - method surface, .pyspi resources,
  WrapperService/module-info registration, Audience-1 Javadoc, tests):
  - [DONE 2026-07-11] plan/archive/Datetime.md - date/datetime/timedelta,
    promoting to java.time.*. Shipped PyDate/PyDateTime/PyTimeDelta +
    DateTime factory; found + fixed a general name-only-proxy-dispatch
    overload hazard (also live in PyDeque.rotate()) - see
    plan/SPI_tutorial.md bug #4. Full suite 621/621 on python3.10/3.12.
  - [SCOPED, not started] plan/Decimal.md - decimal.Decimal, promoting to
    java.math.BigDecimal. High value for finance/numeric-precision use
    cases.
  - [SCOPED, not started] plan/Pathlib.md - pathlib.Path, promoting to
    java.nio.file.Path. Clean mapping, possible reuse with python.io's
    existing stream types for Path.open().
  - [SCOPED, not started, opportunistic/lower priority] plan/Re.md -
    re.Pattern/re.Match. Narrower audience (most regex work would just use
    java.util.regex directly) but small, well-defined protocol.
  - [SCOPED, not started, opportunistic/lower priority] plan/Queue.md -
    queue.Queue/LifoQueue/PriorityQueue. Narrower audience but plays to
    this repo's existing subinterpreter/GIL/async work; threading safety
    of blocking get()/put() is the real design risk here, not the method
    surface - flagged as a prerequisite in the plan, not a detail to
    fill in later.
- [SCOPED, not started] plan/ToPython.md - discovered while researching the
  above: java.io already has a real, public, documented reverse-conversion
  convention (Java value -> genuine Python value via a toPython()
  JImplementationFor customizer, jpype/_jio.py, documented in
  doc/userguide.rst). java.sql.Date/Time/Timestamp and java.math.BigDecimal
  already have the equivalent conversion but under an undocumented private
  name (_py(), jpype/protocol.py:135-157, only used internally by
  dbapi2.py and test_hints.py); java.time.Instant and java.nio.file.Path/
  java.io.File have no reverse conversion at all despite having the
  forward Python->Java JConversion already. User's ask: bring all of these
  up to the same public toPython() convention. Distinct from the
  Datetime/Decimal/Pathlib SPI plans above (this is Java-owned-value ->
  pure Python value; those are Python-owned-value -> Java front-end
  object) - can land independently in either order.
- [SCOPED, not started] Finish documentation for reverse bridge - see
  plan/DOCS.md - chapter-by-chapter mirror of userguide.rst for the
  Java-calling-Python direction; suggested order: Known limitations ->
  reverse quickguide -> python.lang Types -> Controlling the interpreter/
  async chapters -> everything else. Updated 2026-07-11: SPI has now
  landed, so the Customization chapter is unblocked; also added a new row
  for the just-completed $-mangled direct dispatch system
  (plan/archive/NameMangling.md + plan/archive/DispatchFallback.md) - a
  PyObject-rooted Java interface's $foo(...) method routes straight
  through to the real Python object's foo attribute, bypassing the
  hand-authored dispatch map, while plain foo(...) becomes a collision-
  free .foo map-only key. This has zero prior documentation anywhere
  user-facing and needs its own sub-section; ground truth is
  DispatchFallbackNGTest (12/12) and the PyAliceBobCharlieDerik fixture.
  Also refined the Introduction chapter's pitch per user's framing: the
  real differentiator isn't "also works in reverse" - it's that this
  mirrors JPype's own Python-side customizer system back onto Java via
  SPI, so the other language directly implements your Java interfaces
  instead of going through some fixed set of pre-generated static interop
  behaviors. Near-unprecedented framing for a language bridge; the intro
  should lead with this, not bury it as one supported operation among
  many.
- [SCOPED, not started] plan/SPIConstructionHazards.md - Audience 2 (SPI
  provider authors) doc task, distinct from the $-mangled-dispatch
  mechanism row above (that's "what $foo does", this is "the real
  hazards hit while constructing/registering a $-method-bearing
  interface"). Two real gotchas from plan/archive/DispatchFallback.md:
  (1) historical trap, now fixed - the normal _jpype._concrete +
  context.eval() + cast route used to silently fail for $-only interfaces
  (JPProxyIndirectDict::getCallable only checked the proxy wrapper, not
  the wrapped instance), now works, document as "why it's safe today" not
  a live warning; (2) still-live gotcha - Script.eval()'s generic
  PyObject return type needs an explicit `targetClass @ pyProxy` cast or
  probe-based structural matching wins before the proxy-aware conversion
  ever runs. Plus a failure-mode catalog (real callable that throws ->
  matching Py* exception; missing attribute -> NoSuchMethodError; wrong
  return type -> PyTypeError "not compatible with required type"; not
  callable -> PyTypeError "not callable") and the Object-vs-PyObject/
  PyTuple return-type hazard (plain Object return type is a hard
  TypeError at call time, not a silent degrade). Scoped for both
  WrapperService.java Javadoc (short) and doc/userguide.rst's
  Customization chapter (full failure-mode table) - same split
  plan/archive/SPI.md used for the .pyspi format between
  WrapperService.getResources() and SpiResource.java.
- [SCOPED, not started] plan/ToPythonDocs.md - forward-bridge doc task
  (Python calling Java), NOT part of plan/DOCS.md's reverse-bridge mirror.
  Once plan/ToPython.md lands, generalize doc/userguide.rst's existing
  "Customizing java.io Streams" section (currently written as if
  toPython() were java.io-specific) into documentation of the general
  toPython() convention, and add entries for the newly-public/newly-added
  toPython() methods (java.sql.Date/Time/Timestamp, java.math.BigDecimal,
  java.time.Instant, java.nio.file.Path, java.io.File). Depends on
  plan/ToPython.md landing first - docs-only, don't start early.

Everything else remaining in plan/ (ArrayRegionCopy, JSR223, JVMOptions,
ReflectMethod, Smuggler's string-round-trip piece, IO's byte-read bulk
helper + real third-party provider) is post-alpha backlog, not blocking.
Completed plans have been moved to plan/archive/ (2026-07-11) to keep
plan/ showing only active work.
- [STEP 1 DONE 2026-07-11] Smuggler - see plan/Smuggler.md - a Java object
  wrapping one interpreter's Python value, pulled back into Python from a
  *different* interpreter (e.g. out of a plain Java container), used to
  hand back a foreign PyObject* with no check - memory corruption under
  real own-GIL isolation (plan/archive/MultiPhaseInit.md), not just a wrong
  answer. Fixed the minimum: JPClass::convertToPythonObject
  (native/common/jp_class.cpp) now compares the proxy's owning context
  against the current one and raises RuntimeError on mismatch instead of
  corrupting memory. Verified against a real Python 3.12 own-GIL
  subinterpreter, not just compiled: SubInterpreterNGTest.
  testSmuggledProxyAcrossInterpretersThrows. Full regression green on
  python3.10 (483/483, 7 skipped) and python3.12 (483/483, 0 skipped).
  Committed c6e243a7 on `reverse`.:
  - Side effect: getting a real 3.12 build to test against surfaced and
    fixed a build-system bug - editable.mode=inplace pinned CMake's build
    dir to the repo root, so switching Python versions reused stale
    CMakeFiles/ object caches and could silently link the wrong Python.h.
    Now editable.mode=redirect + build-dir=build/{wheel_tag} (per-version
    build dirs), plus dev.mk drops an ABI-tagged copy at the repo root
    (_jpype.cpython-310-...so etc, CPython's own suffix convention) so
    multiple versions coexist there without clobbering, same as this
    repo's pre-scikit-build-core layout.
  - Not done: the "advanced" string-round-trip conversion path for
    convertible types (jump locks to the owning interpreter, convert,
    return) - still just documented in plan/Smuggler.md, not started.


[DONE 2026-07-11] Edge Case need for smuggler: There is a chance that somehow
an exception gets passed down to the wrong interpreter. When it does then the
convertToPythonObject in the exception path can unpack and throw inside of
an exception handling routine. I know that situation that creates that is
near impossible to exercise, but we must wear grade 10 asbestos on wnything
that touches exception paths including smuggled goods. Best to add a
defensive try on that path.
  - Fixed in JPypeException::convertJavaToPython (native/common/jp_exception.cpp):
    the cls->convertToPythonObject(frame, v, false) unpack call - which hits
    the same smuggler guard as any other proxy if the Java exception object
    wraps a Python proxy created by a different interpreter - had no local
    guard. A pre-existing sanity check right below it assumed failure meant a
    null return, but conversion never returns null on failure, it throws, so
    that check was dead code. Wrapped the unpack in try/catch(JPypeException&),
    reusing that same intended fallback (PyErr_SetString(RuntimeError,
    frame.toString(th))) so a corrupted/smuggled exception degrades to a
    clean, contextual error right there instead of unwinding through the
    whole exception-conversion routine to the generic outer "Fatal error
    occurred" catch in toPython() (which already prevented a crash, just
    with a coarser, less informative net).
  - No dedicated repro test added - the trigger condition (an exception
    object itself crossing interpreters) was flagged above as near
    impossible to exercise on purpose.
  - Full suite green both interpreters: 565/565, 0 failures/errors
    (python3.10: 14 skipped; python3.12: 0 skipped).

As part of our longer strategy for allowing direct dispatch we need to consider conflicts between method names.  Java has full priority and thus it gets clean names "foo", and thus Python names get "$foo".  But when the downcall goes it has to look up in a map and that is where contension lives.  JPype has has this issue for a while on teh user side, but it is minor.  When we use this at scale edges will happen.  I recommend that we add an annotation on the Java side to our ccalls that adds a dot to the name when the type is generated in the proxy so we send ".foo" to the _jbridge making it conflict free.  We can then strip the "$" from the name as well so "$foo" Java routes to "foo" Python and "foo" Java with annotation routes to ".foo" in the Python map.  We can substitute . for a different symbol if needed.  For example I am fine with $ swapping places but people may get confused.


So as for the routing.  We will make a class in Python with some methods and an Interface to wrap it like it was an SPI.  We then place methods like alice(arg) bob(arg, arg, key) charlie(arg,...) derik(kwonly) and so forth.  On the Java side we make these into front ends $alice(arg), $bob(arg,arg) $bob(arg,arg, PyKwArgs) $charlie(Object...) $derik(PyKeyArgs) .  If the proxy code is doing its job and we have stripped the dot they will fall through the map entirely and then land on the real calls in Python.

[DONE 2026-07-11] The above routing proof - see plan/archive/DispatchFallback.md.
PyAliceBobCharlieDerik (native/jpype_module/src/test/java/python/lang/) is
exactly the alice/bob/charlie/derik interface described above, backed by a
plain unregistered Python fixture class, with DispatchFallbackNGTest (7
tests) proving every $-method genuinely falls through to the real Python
attribute - correct overload selection between the two $bob signatures,
correct multi-element varargs flattening for $charlie (not just 0/1, which
would have hidden a re-nesting regression), correct PyKwArgs->**kwargs
keyword delivery for $derik, and a negative check proving a real ".alice"
attribute (the map-only key form) is never consulted. No production code
changes were needed - the mangle/dispatch/varargs/kwargs machinery already
worked exactly as designed. Full suite green on python3.10 and python3.12.
  - One real construction wrinkle surfaced along the way, documented in
    plan/archive/DispatchFallback.md: the obvious-looking approach (register the
    fixture class into _jpype._concrete like a real SPI provider, then
    cast after context.eval()) does NOT work for $-only interfaces - that
    route always builds a dict-backed proxy whose attribute fallback
    checks the proxy wrapper itself, never the wrapped object. The
    working construction is the public jpype.JProxy(intf, inst=...,
    convert=True) API plus an explicit `targetClass @ proxy` cast on the
    Python side (needed because Script.eval()'s generic PyObject return
    type otherwise lets probe-based structural matching win before the
    already-a-proxy check ever runs).
  - [DONE 2026-07-11] Follow-up: closed that construction hazard rather
    than live with it. Fixed JPProxyIndirectDict::getCallable
    (native/common/jp_proxy.cpp) so a dict miss now also tries
    PyObject_GetAttr on the real wrapped instance before falling back to
    the proxy wrapper's own attributes - safe for every existing
    "."-mangled interface (their dict lookups always hit per the
    completeness test, so this new branch never fires for them), and it
    makes the *normal* SPI construction path (_jpype._concrete +
    context.eval() + cast, no JProxy()/`@`-cast workaround needed) finally
    serve $-methods too. New regression test
    testConcreteRegistrationRouteNowSupportsDollarMethods proves it.
  - [DONE 2026-07-11] Also added four adversarial fixture cases per user
    request ("hazel() I always throw and other nasty edge cases"), each
    verified (via an exploratory catch-all pass first, not guessed) to
    fail cleanly rather than crash/hang/misbehave: $hazel() (real,
    callable, always raises ValueError) -> python.exceptions.PyValueError
    propagates with the real message; $ghost() (no matching attribute at
    all) -> java.lang.NoSuchMethodError, the normal pre-existing miss
    path; $ike() (real, callable, returns a plain int where PyTuple is
    declared) -> python.exceptions.PyTypeError "Return value is not
    compatible with required type."; $wilma() (real attribute, but a
    plain int, not callable) -> python.exceptions.PyTypeError "'int'
    object is not callable". DispatchFallbackNGTest now 12/12, full suite
    green on python3.10 and python3.12 after the native fix. This item is
    now fully closed.

[PLANNED 2026-07-11, not started - out of tokens this session] Name mangling
for PyObject-rooted proxy dispatch - full design written up in
plan/archive/NameMangling.md, ready to execute next session:
  - Trigger: root-based, in ProxyType's constructor - check once whether any
    interface in the proxy's set is assignable to python.lang.PyObject; if
    so mangle=true for the whole proxy type. No per-interface annotation to
    forget. Plain @JImplements proxies (Runnable, etc.) never have PyObject
    in their hierarchy so they're structurally unaffected.
  - Transform: "$foo" -> "foo" (strip, real direct dispatch); "foo" ->
    ".foo" (map-only key, collision-free with any real Java or Python name).
    Applied only in ProxyType.buildDescriptor's two stringManager.get(...)
    call sites, gated on mangle. Verified this does NOT need to touch
    ProxyFactory's shared global Object-method cache (hashCode/equals/
    toString for plain proxies) - PyObject's own redeclared versions resolve
    through the interface-specific mangled path automatically via the
    existing bestBySignature/isBetter machinery.
  - Python-side blast radius (confirmed by grep, this is the real work):
    35 _Py*Methods dicts in jpype/_jbridge.py + 12 .pyspi resource files
    under native/jpype_module/src/main/resources/python/io/spi/ - every key
    needs the "." prefix added. _PyJPBackendMethods (the Backend interface)
    is correctly OUT of scope - Backend does not extend PyObject.
  - Critical risk: ProxyInstance.hostInvoke treats a missing callable as a
    silent no-op, not an error. A partially-completed rename doesn't crash,
    it just makes methods quietly do nothing. Must land the Java trigger +
    all 47 map-key renames as ONE atomic change, plus a new completeness
    test (enumerate every PyObject-rooted interface registered in
    _jpype._methods, mangle its method names the same way ProxyType does,
    assert the key exists in the dict) so a future forgotten SPI map update
    fails loudly in CI instead of silently at runtime.
  - Open questions before/while executing: confirm "." as the mangle symbol
    (leading dot can never collide with a real getattr identifier, so the
    getCallable fallback path fails safely too); decide whether to also make
    hostInvoke's missing-callable case loud for mangle==true proxies now, or
    defer that hardening as a separate follow-up.

