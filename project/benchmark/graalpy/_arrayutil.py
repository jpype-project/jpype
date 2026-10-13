"""Shared helper for GraalPy's array_flat.py/array_multidim.py/
array_noncontig.py/array_shape.py: manual numpy-array -> Java-array push.

GraalPy's polyglot argument-conversion layer has no buffer->array push
path at all, at any depth or element type -- confirmed empirically:
passing a numpy array anywhere a Java array argument is expected raises
`TypeError('invalid instantiation of foreign object')` unconditionally,
even for a flat int[] (stricter than jep, which at least has a real
numpy fast path for a flat 1D target -- see ../jep/array_flat.py).

Converting a numpy array into a Java array (and back) is basic
orchestration, not an edge case, so rather than treat that as a
documented "no path" gap the way pyjnius's equivalent limitation is
treated (see ../README.md), this builds the missing push path by hand:
allocate a genuine Java primitive array via java.type('<prim>[]...') and
fill it element by element (recursing one level per dimension) from the
numpy source. It's a plain, uniform Python-level loop -- no bulk buffer
read, no shortcut -- which is exactly the kind of hot loop a real JIT
(Truffle/Graal) is supposed to be good at optimizing, so it's fair game
to measure on its own merits even though it takes the long way around.
"""
import java

_JAVA_PRIM = {'int': 'int', 'long': 'long', 'float': 'float', 'double': 'double'}
_array_type_cache = {}


def array_type(label, dims):
    """java.type() for a `label`, `dims`-deep Java primitive array class
    (e.g. array_type('int', 2) -> the int[][] class), cached since
    java.type() itself does a lookup each call."""
    key = (label, dims)
    t = _array_type_cache.get(key)
    if t is None:
        t = java.type(_JAVA_PRIM[label] + '[]' * dims)
        _array_type_cache[key] = t
    return t


def calls_for_manual(total_elements):
    """Iteration-count budget for the manual build_manual() push path,
    separate from the other categories' calls_for() (which assumes a
    single bulk-copy call per element count and scales for ~5,000,000
    elements/trial). build_manual() does one polyglot host-call per
    *element*, not per array, and that per-element crossing cost turned
    out to be on the order of microseconds even once warmed up (measured:
    ~1.2ms for a 100-int array, i.e. ~12,000ns/element) -- several orders
    of magnitude past the bulk-copy cost the other categories are tuned
    around. Budgeting for ~5,000,000 elements/trial here would take
    minutes per row; this instead targets ~5,000 elements/trial, which is
    still enough calls to warm up and get a stable steady-state reading."""
    n = max(3, 5_000 // total_elements)
    warmup = max(2, n // 4)
    return n, warmup


def build_manual(np_sub, dims, label):
    """Recursively build a Java `label`[]...[] (`dims` deep) array from a
    numpy array of the same shape, one element (dims==1) or one row
    (dims>1) at a time. Works the same whether np_sub is contiguous or
    not -- numpy's own indexing handles the stride math, so this is a
    fair "does the manual path degrade for non-contiguous input" probe
    too (see array_noncontig.py)."""
    n = np_sub.shape[0]
    ja = array_type(label, dims)(n)
    if dims == 1:
        for i in range(n):
            ja[i] = np_sub[i].item()
        return ja
    for i in range(n):
        ja[i] = build_manual(np_sub[i], dims - 1, label)
    return ja
