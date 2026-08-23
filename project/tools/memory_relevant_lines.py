#!/usr/bin/env python
"""Classify which lines of a native/ C++ source file are "memory-relevant":
within CONTEXT lines of something this leak-checker effort actually cares
about -- acquiring/releasing a Python reference, a JNI local/global/weak
ref, or heap-allocating/freeing a C++ object.

This exists because raw gcovr line coverage treats a giant mechanical
dispatch table (jp_convert.cpp's dtype-conversion switch, jp_javaframe.cpp's
per-primitive-type JNI wrapper functions) exactly the same as a function
that actually manages a reference's lifetime -- so "60% of native/ covered"
and "60% of the code that can actually leak is covered" are very different
numbers, and only the second is the real goal. This script computes the
line set needed to report the second number instead of the first.

Approach (deliberately simple, a grep with a context window rather than a
real parser): grep each file for the keyword patterns below, then mark
every line within CONTEXT lines of a match as memory-relevant. A generous
context window is the point -- a match near the top of a function should
pull in the rest of that function's error paths a few lines down/up,
without needing to actually find the function's boundaries.
"""
import re
import sys

CONTEXT = 8

MEMORY_KEYWORDS = re.compile(
    r'\bPy_(?:X)?(?:INCREF|DECREF)\b'
    r'|\bPy_(?:CLEAR|VISIT)\b'
    r'|\bJPPyObject\b'
    r'|\btp_alloc\b|\btp_free\b'
    r'|\bNew(?:Global|Local|WeakGlobal)Ref\b'
    r'|\bDelete(?:Global|Local|WeakGlobal)Ref\b'
    r'|\bPushLocalFrame\b|\bPopLocalFrame\b'
    r'|\bJPJavaFrame\b(?!::)'  # local JPJavaFrame construction (RAII
                               # guard) -- not "JPJavaFrame::method(...)",
                               # which is just this class's own method
                               # qualifier and would match every line in
                               # jp_javaframe.cpp itself otherwise.
    r'|\bnew\s+[A-Za-z_]'
    r'|\bdelete\s'
    r'|\bmalloc\s*\(|\bfree\s*\(|\bcalloc\s*\('
    r'|\bregisterRef\b|\breleaseProxyPython\b|\breleasePython\b'
    r'|\bPyObject_GC_(?:Track|UnTrack)\b'
)


def memory_relevant_lines(path, context=CONTEXT):
    with open(path, encoding='utf-8', errors='replace') as f:
        lines = f.readlines()
    n = len(lines)
    relevant = set()
    for i, line in enumerate(lines):
        if MEMORY_KEYWORDS.search(line):
            lo = max(0, i - context)
            hi = min(n, i + context + 1)
            relevant.update(range(lo + 1, hi + 1))  # 1-indexed
    return relevant


if __name__ == '__main__':
    for path in sys.argv[1:]:
        rel = memory_relevant_lines(path)
        print(f"{path}: {len(rel)} memory-relevant lines")
