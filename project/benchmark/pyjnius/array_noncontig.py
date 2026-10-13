"""Non-contiguous (strided) buffer-protocol source -- pyjnius side.

This file intentionally contains NO benchmarks. It exists purely so the
directory listing stays consistent with jpype/array_noncontig.py,
jpy/array_noncontig.py, jep/array_noncontig.py (one file per category
per library) -- while being honest that pyjnius has nothing to measure
here, rather than silently omitting the file and leaving a gap that
looks like an oversight.

Why there is nothing to benchmark: jpype/array_noncontig.py measures
whether pushing a non-contiguous numpy source (a column slice, a
transposed array -- still a fully valid buffer-protocol object, just
without a C-contiguous memory layout) takes a bulk buffer-read path or
falls back to a per-element/per-row walk. That question only makes sense
for a library that has a buffer->array push path *at all*. pyjnius does
not, at any size or depth, contiguous or not -- confirmed empirically
(not assumed), the same finding documented in
project/benchmark/pyjnius/array_flat.py's docstring: passing a numpy
array as an argument where a Java array is expected raises
`JavaException('Expecting a python list/tuple, got array(...)')`
unconditionally, regardless of the array's strides or contiguity. A
non-contiguous buffer source and a contiguous one both hit that same
unconditional rejection before any conversion machinery gets a chance to
care whether the memory layout is friendly -- so "does pyjnius's
buffer->array path handle non-contiguous input well" isn't a slower-path
question here, it's a no-path question: there is no buffer->array push
for a non-contiguous source to be slower *than*.

See project/benchmark/README.md for the same limitation as it applies to
array_flat.py/array_multidim.py's missing buffer->array rows.
"""

if __name__ == '__main__':
    print(
        "pyjnius/array_noncontig.py: no benchmarks in this file. "
        "pyjnius has no buffer->array push at all, at any size or depth "
        "(JavaException('Expecting a python list/tuple, got array(...)') "
        "unconditionally, confirmed empirically -- see this file's "
        "docstring and array_flat.py's), so there is no non-contiguous-"
        "buffer-source case to measure: pyjnius never takes a buffer "
        "source in the first place, contiguous or not."
    )
