"""Manifest of test/jpypetest/*.py files run on-device via plain unittest
(see testapp/main.py). This package is a thin loader only - it holds no
copies of the test files themselves. project/android/sync_tests.py reads
TEST_MODULES below and stages the real, unmodified files from
test/jpypetest/ into this directory (gitignored, regenerated on every
sync) before each `buildozer android debug`; run that script first if
tests/test_*.py look missing here.

Not discovered by filename pattern: p4a bundles the app as compiled .pyc
only (no .py sources on-device), and unittest.discover() scans the
filesystem for literal `test_*.py` filenames, which never match a
.pyc-only bundle - it silently finds nothing rather than erroring, which
is what happened the first time this was tried (logged as
"PORTED SUITE: ran=0"). Importing modules by name and loading tests from
the module object works regardless, since that goes through Python's
normal import machinery instead of a directory listing.

Genuinely Android-specific behavior differences within a shared file are
handled in place with test/jpypetest/common.py's
`@common.skipOnAndroid(reason)` (a no-op on desktop) - see
test_attr.py/test_classloader.py/test_fault.py/test_forname.py/
test_jclass.py/test_jvmfinder.py for examples. Add a new file's module
name below only once it's been verified clean on-device; a file needing
no Android-specific changes at all can be added exactly as it stands
upstream.

Deliberately NOT ported (see doc/android.rst's "Removed JPype Services"
and "Unsupported Java libraries" for why):

- test_thread.py: uses jpype.attachThreadToJVM()/detachThreadFromJVM(),
  removed on Android - Android's JVM is always already attached.
- test_coverage.py: calls jpype.getDefaultJVMPath()/jpype.startJVM(),
  meaningless on Android's single already-running JVM.
- test_sql_h2.py, test_sql_hsqldb.py, test_sql_sqlite.py: need real JDBC
  driver jars this harness doesn't bundle.
- test_keywords.py: uses @pytest.mark.parametrize, a real pytest feature -
  would need rewriting as a loop over explicit unittest test methods, not
  just a straight copy.
- Anything using subrun (test/jpypetest/subrun.py): spawns a fresh
  subprocess with its own startJVM() call per test - Android has neither
  subprocesses-with-their-own-JVM nor startJVM().
"""
TEST_MODULES = [
    'test_annotation',
    'test_array',
    'test_arrayFromBuffer',
    'test_attr',
    'test_boxed',
    'test_boxing',
    'test_boxing_comprehensive',
    'test_boxing_edge_cases',
    'test_buffer',
    'test_bytebuffer',
    'test_caller_sensitive',
    'test_charSequence',
    'test_classhints',
    'test_classloader',
    'test_closeable',
    'test_closed',
    'test_collection',
    'test_comparable',
    'test_conversion',
    'test_conversionInt',
    'test_conversionLong',
    'test_conversionShort',
    'test_core',
    'test_customizer',
    'test_directbuffer',
    'test_docstring',
    'test_exc',
    'test_fault',
    'test_fields',
    'test_forname',
    'test_functional',
    'test_gcfree_leaves',
    'test_generic',
    'test_hash',
    'test_hints',
    'test_inherit',
    'test_javacoverage',
    'test_javadoc',
    'test_jboolean',
    'test_jbyte',
    'test_jchar',
    'test_jclass',
    'test_jdouble',
    'test_jedi',
    'test_jfloat',
    'test_jint',
    'test_jlong',
    'test_jmethod',
    'test_jobject',
    'test_jpackage',
    'test_jshort',
    'test_jstring',
    'test_jvmfinder',
    'test_lambdas',
    'test_leak2',
    'test_list',
    'test_map',
    'test_module2',
    'test_mro',
    'test_number',
    'test_numeric',
    'test_objectwrapper',
    'test_opts',
    'test_overloads',
    'test_pickle',
    'test_proxy_multithreaded',
    'test_ref',
    'test_reflect',
    'test_repr',
    'test_serial',
    'test_sql_generic',
    'test_synchronized',
    'test_utf8',
    'test_varargs',
    'test_virtual',
]
