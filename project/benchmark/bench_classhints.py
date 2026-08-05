"""JPype-specific benchmark for the per-type conversion cache added to
JPClass::findJavaConversion (see native/common/include/jp_conversioncache.h).

No equivalent exists in jpy/jep (neither has a @JConversion-style
extensible hint mechanism), so this one isn't cross-library comparable --
see bench_jpype.py/bench_jpy.py/bench_jep.py for that.

Requires the test harness classes (jpype.classhints.Custom/ClassHintsTest,
built via BUILD_TEST_HARNESS=ON) on the classpath.

Usage:
    /path/to/venv/bin/python project/benchmark/bench_classhints.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from _common import timeit, format_row

import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

Custom = jpype.JClass('jpype.classhints.Custom')
ClassHintsTest = jpype.JClass('jpype.classhints.ClassHintsTest')


@jpype.JImplements(Custom)
class CustomImpl:
    pass


_singleton = CustomImpl()

TOTAL_HINTS = 400
all_classes = [type(f'Impl{i}', (object,), {}) for i in range(TOTAL_HINTS)]
for c in all_classes:
    @jpype.JConversion(Custom, instanceof=c)
    def _conv(jcls, obj):
        return _singleton

print("=== JPype: hint-conversion cache (N=400 registered hints) ===")
for pos in (1, 5, 20, 100, 200, 400):
    inst = all_classes[pos - 1]()
    best, median = timeit(lambda: ClassHintsTest.call(inst))
    print(format_row(f"match@{pos}/{TOTAL_HINTS}", best, median))

jpype.shutdownJVM()
