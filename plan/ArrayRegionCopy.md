# Bulk Java-array -> caller-provided Python buffer copy (ArrayRegion-style)

## Status (2026-07-11): scoped, not started

Raised as a question during the numpy buffer-protocol discussion
(`plan/NumpyBufferBench.md`): JNI has a "you provide the destination,
JVM copies into it" family (`Get<Type>ArrayRegion`/`Set<Type>ArrayRegion`,
and `GetPrimitiveArrayCritical` used as a pinned source/destination) that
is a fundamentally different shape from Python's buffer protocol, which is
"I (the exporter) provide a live pointer, you accept it." jpype currently
only implements the *accept-a-live-pointer* shape for exposing Java arrays
as Python buffers — there is no operation for bulk-copying a Java array's
contents into a Python buffer the *caller* already owns (e.g. a
preallocated numpy array), which is the natural fit for the
"provide a destination" JNI family.

## Existing precedent (proves the pattern already works, other direction)

`convertMultiArray` (`native/common/include/jp_primitive_accessor.h:88-160`)
already does exactly this shape, but Python-buffer-as-source /
Java-array-as-destination: given a Python `Py_buffer` (arbitrary
shape/strides), it walks it element-by-element via `buffer.getBufferPtr(indices)`
and copies into a JVM-provided `GetPrimitiveArrayCritical` pointer,
respecting the source buffer's strides (`view.strides[u]`) and committing/
advancing per innermost-dimension run (`ReleasePrimitiveArrayCritical(...,
JNI_COMMIT)` then re-acquire for the next array in a multi-array structure).
This is the template to mirror, not a new algorithm to invent.

## What exists today for the Java-array side

- `JPByteType::getView`/`releaseView` (`native/common/jp_bytetype.cpp:247-262`,
  and the sibling `jp_<type>type.cpp` files) — buffer-protocol *export*:
  `Get<Type>ArrayElements`/`Release<Type>ArrayElements` hands back a
  JVM-owned, possibly-copied live pointer that Python holds until
  `releasebuffer`. This is the "accept a pointer" shape — already
  implemented, not what's missing.
- `getArrayItem`/`setArrayItem` (same files) — single-element
  `Get/SetByteArrayRegion` calls. Proves the "provide a destination" JNI
  family is already in use, but only ever one element at a time, never
  bulk.
- Nothing currently does a bulk multi-element (let alone
  multi-dimensional/strided) copy from a Java array into an
  already-existing Python buffer.

## Goal

A bulk copy operation: given a Java array and a target Python object that
already exports the buffer protocol (e.g. a preallocated numpy array, a
`bytearray`, an existing `PyMemoryView`'s backing buffer), copy the Java
array's contents directly into that buffer's memory, honoring the
*destination* buffer's shape/strides — no intermediate `PyMemoryView`
export/pin of the Java array itself, no intermediate full-array Python
object materialization.

Two windows into the JNI side to choose between (need to weigh at
implementation time, not decided here):
- `Get/Set<Type>ArrayRegion` — simple, well-defined bulk memcpy-style
  calls with no pinning/lifetime concerns, but each call is a fixed
  contiguous run (`start, len, dest`) — multi-dimensional/non-contiguous
  destination strides would need multiple calls (one per contiguous run),
  same shape as how `convertMultiArray` already breaks a multi-dim copy
  into per-innermost-dimension runs.
  Care needed on JNI Critical Section API restrictions — no GC-triggering
  or blocking calls while a Critical section is held.
- `GetPrimitiveArrayCritical`/`ReleasePrimitiveArrayCritical` — matches
  `convertMultiArray`'s existing pattern exactly (just reversed roles:
  Java array becomes the *source* pointer instead of the destination),
  reusing the same strided-walk logic already written and tested there.

Leaning toward mirroring `convertMultiArray` directly (critical-section
source, `Py_buffer`-described destination with its own strides), since
that reuses proven stride-walking code and keeps both directions
symmetric, rather than introducing a second, differently-shaped
implementation using `ArrayRegion` calls.

## Design sketch

- New function, sibling to `convertMultiArray` in
  `jp_primitive_accessor.h` or a new home — something like
  `copyArrayToBuffer(JPJavaFrame&, JPPrimitiveType*, jarray source,
  JPPyBuffer& destBuffer)` — walks `destBuffer`'s shape/strides (same
  `Py_buffer` traversal logic as `convertMultiArray`), pulling from a
  `GetPrimitiveArrayCritical`-pinned view of `source` instead of writing
  into one.
- Java-facing entry point: likely on the reverse-bridge side
  (`python.lang`) as something like a `copyInto(PyObject buffer)` method
  on the Java array wrapper, or on the classic side as
  `JArray.copyToBuffer(pyBufferObj)` — placement TBD, check which
  direction actually needs this first (came up in the numpy
  stride/copy-ops test bench discussion, so likely driven by whichever
  direction that bench needs first).
- Handle dtype/format mismatches the same way existing array conversion
  does (`getConverter(view.format, itemsize, code)` in
  `convertMultiArray`) — reuse, don't reinvent.

## Testing

- Extend `plan/NumpyBufferBench.md`'s test bench: allocate a numpy array
  up front (`np.empty(shape, dtype=...)`), copy a same-shaped Java array
  into it via this new operation, confirm values match — including a
  non-contiguous destination case (copy into a transposed/sliced numpy
  array view) to prove destination-stride handling actually works, not
  just the contiguous case.
- Confirm no intermediate full-array Python object or `PyMemoryView`
  export of the Java array is created during the copy (this is the actual
  point of the feature — avoid the pin-and-hold-a-live-pointer path
  entirely for a one-shot bulk copy).

## Open questions

- Should this also support the reverse (Python buffer -> existing Java
  array, i.e. "SetArrayRegion-style" bulk write into a Java array the
  caller already has, as opposed to `convertMultiArray`'s
  construct-a-new-array path)? Not yet scoped; flag as a likely follow-on
  once this lands, not part of this doc's initial goal.
