"""Minimal headless verification app for the project/android/ build harness.

Everything is written to stdout, which p4a's webview bootstrap forwards to
logcat, so `buildozer android debug deploy run logcat` is the whole
interaction loop - see project/android/README.md.

Three checks, run in this specific order:

1. Golden path: `import jpype` (which triggers `_jpype.bootstrap()`
   automatically, see jpype/__init__.py) followed by a trivial JClass call.
   Confirms the whole chain - NDK cross-compile, Android_JNI_GetEnv() glue,
   org.jpype Java classes reaching the APK's dex, PyJPModule_bootstrap() -
   actually works end to end.

2. The actual test/jpypetest/*.py suite, ported one file at a time under
   tests/ (see tests/common.py's docstring for what changes between the
   desktop and Android versions - not much). Run via plain unittest, not
   pytest - avoids the open question of whether pytest itself runs on
   Android at all, and there's no jvm_session-equivalent fixture needed
   (the JVM is already attached by the time `import jpype` above returns).
   Only subrun-free tests are portable this way at all: subrun.py spawns a
   fresh subprocess with its own startJVM() call per test, and Android has
   neither subprocesses-with-their-own-JVM nor startJVM() (see
   doc/android.rst) - see doc/android_build.rst for the porting approach.

3. Regression check for #1257, run LAST and deliberately. PyJPModule_bootstrap()
   previously let a C++ exception escape uncaught across the C-linkage boundary
   into CPython, aborting the whole process (SIGABRT) instead of raising a
   catchable Python exception. Reproducing the exact resource-loading failure
   from the original report isn't practical from here (it depended on the
   reporter's own modified bindings), but calling `_jpype.bootstrap()` a
   second time exercises the same JP_PY_TRY/JP_PY_CATCH-wrapped function
   again. What matters is only whether the process survives to print a
   result at all - whether the second call raises or succeeds cleanly are
   both fine outcomes now that bootstrap genuinely works; only a SIGABRT /
   "terminating due to uncaught exception" in logcat (this script never
   reaching its next print) would indicate the original #1257 crash
   signature came back.

   This check is deliberately last: calling bootstrap() a second time - not
   a supported operation in normal use, real code calls it exactly once via
   `import jpype` - re-creates the JVM's internal type context, leaving any
   Python-level Java wrapper types/objects created before this point (e.g.
   jpype.types.JChar, or anything the ported suite above touched) holding a
   stale JPClass pointer from the first context. That was root-caused via a
   confusing android-only-looking failure: fixture.callChar(JChar('B'))
   failing overload resolution with the char argument's JPClass no longer
   matching the (post-second-bootstrap) context's own char type, even though
   the standalone same-object check reported an exact match moments earlier.
   Nothing was wrong with the char/overload matching itself; running this
   check before the ported suite was corrupting the single-bootstrap
   invariant the rest of the script (and normal JPype usage) relies on.
"""
print("=== jpype android testapp starting ===")

try:
    import jpype
    cls = jpype.JClass('java.lang.String')
    print("GOLDEN PATH: PASS (%s)" % cls)
except Exception as ex:
    print("GOLDEN PATH: FAIL: %r" % (ex,))

print("=== package marker asset diagnostic ===")
try:
    ctx = jpype.JClass('org.jpype.JPypeContext').getInstance()
    for probe in ("java", "java.lang", "jpype"):
        print("DIAG isPackage(%r)=%r" % (probe, ctx.isPackage(probe)))
except Exception as ex:
    print("DIAG marker probe FAILED: %r" % (ex,))

print("=== running ported test/jpypetest suite ===")
try:
    import sys
    import os
    import unittest
    import importlib

    sys.path.insert(0, os.path.join(os.path.dirname(__file__), "tests"))
    import tests

    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    load_failures = []
    for name in tests.TEST_MODULES:
        # A single module that fails to import (e.g. a leftover, unused
        # `import pytest` in an otherwise-plain-unittest file - real once
        # already, see tests/__init__.py's test_keywords.py note for a
        # case that needed excluding outright rather than fixing) must not
        # take down the whole run: report it and keep going, the same way
        # a real desktop test run isn't voided by one broken file.
        try:
            module = importlib.import_module("tests." + name)
        except Exception as ex:
            load_failures.append((name, ex))
            continue
        suite.addTests(loader.loadTestsFromModule(module))

    for name, ex in load_failures:
        print("PORTED SUITE: FAILED TO LOAD tests.%s: %r" % (name, ex))

    result = unittest.TextTestRunner(stream=sys.stdout, verbosity=2).run(suite)
    print("PORTED SUITE: ran=%d failures=%d errors=%d skipped=%d" % (
        result.testsRun, len(result.failures), len(result.errors),
        len(result.skipped)))
except Exception as ex:
    print("PORTED SUITE: FAIL to run at all: %r" % (ex,))

# Run last and unconditionally - see module docstring for why this must not
# run before the ported suite above.
try:
    import _jpype
    _jpype.bootstrap()
    print("REGRESSION CHECK #1257: PASS - second bootstrap() completed, no crash")
except Exception as ex:
    print("REGRESSION CHECK #1257: PASS - caught %r instead of crashing" % (ex,))

print("=== jpype android testapp done ===")
