"""Turn parsed.json (from parse_logs.py) into RESULTS.md's markdown body.

Usage:
    python3 gen_results.py <parsed.json> [graalpy_section.md] > RESULTS.md

<parsed.json> is parse_logs.py's output. [graalpy_section.md] is the
static, carried-forward GraalPy section text (Section 9); defaults to
graalpy_section.md next to this script -- GraalPy is not part of the
jpype/jpy/jep/pyjnius re-run this script drives and is expected to stay
a periodically-refreshed, separately-maintained snapshot, not something
parse_logs.py's log format feeds automatically.

The row-matching logic below (`direction_table`, `multidim_direction`,
`shape_rows`, etc.) depends on each script's printed `label` text having
a consistent, parseable shape (e.g. `"list->array int[100000], fresh"`,
`"buffer->array int[][][](10^3), transposed, manual per-row"`) --
matching project/benchmark/{jpype,jpy,jep,pyjnius}/*.py's actual label
strings as of when this was written. If a script's label wording
changes, update the matching logic here to match, not the other way
around.
"""
import json
import os
import sys

LIBS = ['jpype', 'jpy', 'jep', 'pyjnius']

if len(sys.argv) < 2:
    print(__doc__, file=sys.stderr)
    sys.exit(1)

PARSED_JSON = sys.argv[1]
GRAALPY_SECTION_PATH = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
    os.path.dirname(os.path.abspath(__file__)), 'graalpy_section.md')

with open(PARSED_JSON) as f:
    d = json.load(f)

def get(lib, script):
    return {r['label']: r for r in d.get(lib, {}).get(script, [])}

def fmt(v):
    if v is None:
        return '--'
    return f"{v:,.0f}"

def table(headers, rows):
    lines = []
    lines.append('| ' + ' | '.join(headers) + ' |')
    lines.append('|' + '|'.join(['---'] + [':---:'] * (len(headers) - 1)) + '|')
    for r in rows:
        lines.append('| ' + ' | '.join(r) + ' |')
    return '\n'.join(lines)

def cross_table(script, labels_in_order, field='best'):
    """labels_in_order: list of exact label strings (must match across libs)."""
    data = {lib: get(lib, script) for lib in LIBS}
    rows = []
    for label in labels_in_order:
        row = [label]
        for lib in LIBS:
            rec = data[lib].get(label)
            row.append(fmt(rec[field]) if rec else '--')
        rows.append(row)
    return table(['operation'] + LIBS, rows)

out = []

def h(level, text):
    out.append('#' * level + ' ' + text)
    out.append('')

def p(text):
    out.append(text)
    out.append('')

# ---------------------------------------------------------------
h(1, 'Cross-library benchmark results')
p("""jpype vs. jpy, jep, and pyjnius on the JVM-embedding side of a Python/Java
bridge (Python drives Java in all four); GraalPy (Python-in-the-JVM,
opposite architecture) is tracked separately in Section 9 and was not
re-run for this edition. All numbers below are from a single sequential
re-run of the full suite, in disposable/isolated environments per this
repo's CLAUDE.md, with increased statistics (`trials=7`, higher
iteration floors) versus prior editions of this report to reduce noise.
Every section follows the same shape: **Methodology** (what is measured
and how), **Table** (raw numbers, `best` of the trials, nanoseconds per
call unless noted), **Result** (the factual takeaway only -- no
running commentary, no revision history).""")

h(2, 'Global methodology')
p("""- **Timing.** `timeit()` (`project/benchmark/_common.py`, and an
  inlined equivalent in every `jep/*.py` script since jep's embedded
  interpreter can't import a sibling file): warm up, then run `trials=7`
  timed batches of `n` calls each; report `best` (minimum batch mean) and
  `median` across the 7 batches. Tables below show `best`; raw `best`/
  `median` pairs and CSVs are preserved alongside each script's own
  output.
- **Iteration counts.** Fixed-cost benchmarks (scalars, dispatch, proxy,
  strings, object identity) use `n=200,000`. Array benchmarks scale `n`
  down as element count grows (`calls_for()` in each script) so a
  100,000-element case doesn't take minutes; floors were raised this
  edition (`n >= 30`, `warmup >= 6` per batch, up from `n >= 20`,
  `warmup >= 5`) for better statistics without making the deepest
  multi-dimensional cases impractically slow.
- **Environments.** jpype: fresh venv, `pip install --no-build-isolation
  -e .` with `BUILD_TEST_HARNESS=ON`. jpy: fresh venv, prebuilt wheel
  from `~/devel/jpy/dist`. jep: `~/devel/jep/target/jep-4.3.1.jar` +
  the Python-3.12-matched native build
  (`~/devel/jep/build/lib.linux-x86_64-cpython-312`), launched as a Java
  process per this repo's README. pyjnius: fresh venv, built from source
  (`~/devel/pyjnius`, Cython 3.1.2) against this machine's JDK. Full
  per-library setup in `project/benchmark/README.md`.
- **Machine.** Single 16-core/7.7GB-RAM machine, one library's suite run
  at a time except where noted; no concurrent unrelated load.
- **Scope.** `int`/`long`/`float`/`double` element types throughout
  unless noted. `Z`/`B`/`C`/`S` (boolean/byte/char/short) arrays are not
  separately benchmarked in this report.
""")

# ---------------------------------------------------------------
h(2, '1. Scalars, strings, object identity')
p("""**Methodology.** `Math.max(int,int)` / `new Integer(int)` (int.py),
`Math.sqrt(double)` / `new Double(double)` (double.py), a round-trip
Java-String-from-Python-then-`str()`-back (strings.py), and reference
identity of a returned Java object compared across two calls
(object.py). All four are fixed-cost, `n=200,000` per batch.""")

rows = []
for label in ['Math.max(int,int)', 'new Integer(int)', 'Math.sqrt(double)',
              'new Double(double)', 'new String + toString', 'Object identity']:
    row = [label]
    for lib in LIBS:
        rec = None
        for script in ['int', 'double', 'strings', 'object']:
            rec = get(lib, script).get(label)
            if rec:
                break
        row.append(fmt(rec['best']) if rec else '--')
    rows.append(row)
out.append(table(['operation'] + LIBS, rows))
out.append('')
p("""**Result.** jpy is fastest on every scalar/string/identity op; jpype
trails jpy by roughly 1.3-1.9x; jep and pyjnius trail jpype by a further
1.5-4x depending on the op, with pyjnius's `new Integer`/`new Double`/
string-roundtrip costs the widest outliers (7-23x jpy).""")

# ---------------------------------------------------------------
h(2, '2. Method dispatch and proxy callbacks')
p("""**Methodology.** `dispatch.py`: a 16-overload method called
monomorphically (same arg type every call) vs. polymorphically
(argument type varies call-to-call, forcing overload resolution to
redo work jpype/jep can otherwise cache). `proxy.py`: steady-state cost
of invoking an already-constructed Python-implements-Java-interface
callback, `int` argument and (where supported) `Object` argument.""")

rows = []
for label in ['overload x16, monomorphic', 'overload x16, polymorphic',
              'proxy callback (established), int arg',
              'proxy callback (established), Object arg']:
    row = [label]
    for lib in LIBS:
        rec = get(lib, 'dispatch').get(label) or get(lib, 'proxy').get(label)
        row.append(fmt(rec['best']) if rec else '--')
    rows.append(row)
out.append(table(['operation'] + LIBS, rows))
out.append('')
p("""**Result.** jpy has no separate proxy script (not benchmarked here).

pyjnius's proxy Object-arg case is not benchmarked: it reliably
segfaults this pyjnius checkout (`GetObjectClass`/`IsSameObject` called
without a null check on a genuinely-null `Object` argument), reproduced
independently against a fresh build before being treated as a real
finding rather than stale-build noise, per this repo's CLAUDE.md. jpype
leads jep and pyjnius on dispatch by roughly 4-7x; jpy leads jpype on
raw dispatch cost the same way it does on scalars.""")


# ---------------------------------------------------------------
DTYPES = ['int', 'long', 'float', 'double']
SIZES = ['100', '1000', '10000', '100000']

def direction_table(script, direction_label, extra_suffix=''):
    """Rows = type[size], columns = libraries, for one direction prefix
    within a script (e.g. 'list->array' in array_flat.py)."""
    rows = []
    any_data = False
    for t in DTYPES:
        for sz in SIZES:
            label = f"{direction_label} {t}[{sz}]{extra_suffix}"
            row = [f"{t}[{sz}]"]
            found = False
            for lib in LIBS:
                rec = get(lib, script).get(label)
                row.append(fmt(rec['best']) if rec else '--')
                if rec:
                    found = True
                    any_data = True
            if found:
                rows.append(row)
    if not any_data:
        return None
    return table(['size'] + LIBS, rows)

h(2, '3. Array push, flat (1D)')
p("""**Methodology.** `array_flat.py`. A Python `list`/numpy array of
length 100/1,000/10,000/100,000 pushed into a Java method parameter
(`int[]`/`long[]`/`float[]`/`double[]`), and the reverse (`array->list`,
`array->buffer`): reading a Java array back into a Python `list` /
numpy buffer. `list->array, widening from int` additionally covers a
plain Python `int` list pushed against a `float[]`/`double[]`
parameter (int has no widening case against itself).""")

h(3, '`list->array` push (method argument)')
out.append(direction_table('array_flat', 'list->array', ', fresh'))
out.append('')

h(3, '`list->array`, widening from int (float/double only)')
rows = []
for t in ['float', 'double']:
    for sz in SIZES:
        label = f"list->array {t}[{sz}], widening from int"
        row = [f"{t}[{sz}]"]
        for lib in LIBS:
            rec = get(lib, 'array_flat').get(label)
            row.append(fmt(rec['best']) if rec else '--')
        rows.append(row)
out.append(table(['size'] + LIBS, rows))
out.append('')

h(3, '`buffer->array` push (method argument, numpy source)')
t_ = direction_table('array_flat', 'buffer->array', ', fresh')
out.append(t_ if t_ else '_pyjnius has no buffer->array push at all -- see Section 5._')
out.append('')

h(3, '`array->list` pull (Java array -> Python list)')
out.append(direction_table('array_flat', 'array->list'))
out.append('')

h(3, '`array->buffer` pull (Java array -> Python/numpy buffer)')
t_ = direction_table('array_flat', 'array->buffer')
out.append(t_ if t_ else '_no data._')
out.append('')

p("""**Result.** `list->array`: jpype leads jpy/jep/pyjnius at every
size 1,000 and up; all four show a 1.5-2x int-widening penalty on
float/double vs. their own matched-type number, jpype's the narrowest.
`buffer->array`: pyjnius has no buffer-protocol push at all (falls back
to `sequenceConversion`, i.e. it isn't in this table -- see Section 5
for the isolated cost of that fallback). `array->list`: pyjnius is the
fastest of all four despite losing most other benchmarks in this
report, because its Cython bridge boxes one plain `PyLong`/`PyFloat`
per element while jpype/jep/jpy build heavier tagged wrapper objects.
`array->buffer`: jep/pyjnius have no real buffer-protocol return path --
their columns above are `array->list`'s cost plus a redundant
`np.asarray()`, not a genuine bulk read, which is why they land *worse*
than their own `array->list` number instead of better. jpype and jpy
have a genuine buffer-protocol return path and are close to each other,
with jpy consistently faster.""")

# ---------------------------------------------------------------
def lookup_variants(lib, script, variants):
    for label in variants:
        rec = get(lib, script).get(label)
        if rec:
            return rec
    return None

h(2, '4. Array push, non-contiguous sources')
p("""**Methodology.** `array_noncontig.py`. A numpy source that cannot
provide a C-contiguous buffer view -- a non-unit-stride column slice
(flat, 1D) or a transposed array (2D-5D, `np.transpose` with reversed
axis order) -- pushed as a method argument. Tests whether a bulk
buffer-read path is still reached, or whether the implementation falls
back to a fully general per-element/per-row walk.""")

h(3, 'Flat (1D), non-contiguous column slice')
rows = []
for t in DTYPES:
    for sz in SIZES:
        label = f"buffer->array {t}[{sz}], column slice"
        row = [f"{t}[{sz}]"]
        any_ = False
        for lib in LIBS:
            rec = get(lib, 'array_noncontig').get(label)
            row.append(fmt(rec['best']) if rec else '--')
            if rec:
                any_ = True
        if any_:
            rows.append(row)
out.append(table(['size'] + LIBS, rows))
out.append('')
p("_jpy and pyjnius have no entry: jpy's buffer matcher requires "
  "`PyBUF_SIMPLE` (fails outright on a non-contiguous 1D source); "
  "pyjnius has no buffer->array push at all, contiguous or not._")
out.append('')

h(3, 'Multi-dimensional (2D-5D), transposed')
DEPTHS = [('2', '10^2'), ('3', '10^3'), ('4', '10^4'), ('5', '10^5')]
BRACKETS = {'2': '[][]', '3': '[][][]', '4': '[][][][]', '5': '[][][][][]'}
rows = []
for t in DTYPES:
    for dep, pow_ in DEPTHS:
        br = BRACKETS[dep]
        row_label = f"{t}{br}({pow_})"
        row = [row_label]
        any_ = False
        for lib in LIBS:
            variants = [
                f"buffer->array {t}{br}({pow_}), transposed",
                f"buffer->array {t}{br}({pow_}), transposed, manual per-row",
            ]
            rec = lookup_variants(lib, 'array_noncontig', variants)
            row.append(fmt(rec['best']) if rec else '--')
            if rec:
                any_ = True
        if any_:
            rows.append(row)
out.append(table(['shape'] + LIBS, rows))
out.append('')
p("""**Result.** jpype and jep are the only libraries with a real bulk
path for a non-contiguous 1D source; jep is faster at this size. jpy
has no buffer->array push at all for a non-contiguous source in any
dimensionality (fails outright, 1D; not benchmarked, ND, since the
underlying push has no bulk path to exercise); pyjnius has no
buffer->array push at any size, depth, or contiguity. jep's
"transposed, manual per-row" ND numbers are a per-row Python-level
walk, not a bulk buffer read -- included for completeness, not a
like-for-like comparison to jpype's single-JNI-call path.""")

# ---------------------------------------------------------------
h(2, '5. Array push/pull, multi-dimensional (depth 2-5, rectangular)')
p("""**Methodology.** `array_multidim.py`. A nested Python list (or
nested numpy-backed structure) of depth 2-5 with a fixed total element
count (~10^depth), pushed (`list->array`, `buffer->array`) or pulled
(`array->list`, `array->buffer`). `buffer->array` is a genuine bulk
buffer read where the library has one (jpype, jpy); jep's is a manual
per-row Python-level walk (no bulk ND push path exists in jep).
pyjnius has no buffer->array push at any depth.""")

def multidim_direction(direction, key):
    rows = []
    for t in DTYPES:
        for dep, pow_ in DEPTHS:
            br = BRACKETS[dep]
            row = [f"{t}{br}({pow_})"]
            any_ = False
            for lib in LIBS:
                variants = [f"{direction} {t}{br}({pow_}){key}",
                            f"{direction} {t}{br}({pow_}){key}, manual per-row"]
                rec = lookup_variants(lib, 'array_multidim', variants)
                row.append(fmt(rec['best']) if rec else '--')
                if rec:
                    any_ = True
            if any_:
                rows.append(row)
    return table(['shape'] + LIBS, rows)

h(3, '`list->array` push, fresh nested list')
out.append(multidim_direction('list->array', ', fresh'))
out.append('')

h(3, '`buffer->array` push, numpy source (jep: manual per-row fallback)')
out.append(multidim_direction('buffer->array', ', fresh'))
out.append('')
p("_pyjnius: no entry -- no buffer->array push at any depth._")
out.append('')

h(3, '`array->list` pull')
out.append(multidim_direction('array->list', ''))
out.append('')

h(3, '`array->buffer` pull')
out.append(multidim_direction('array->buffer', ''))
out.append('')

p("""_jpype numbers reflect the list/tuple-specialized ragged-native readout
(`matchRaggedNode`/`encodeRaggedNode`, `native/common/jp_classhints.cpp`)
-- see Section 11._""")
out.append('')

p("""**Result.** `list->array`: jpype now leads at every depth, having
closed and reversed a 2.2-2.5x deficit against jpy (see Section 11).
`buffer->array`:
jpy and jpype both reach a genuine bulk path and are within a few
percent of each other by depth 4-5; jep's manual per-row fallback is
1-2 orders of magnitude slower at depth 4-5; pyjnius has none.
`array->list`/`array->buffer`: pyjnius is fastest at shallow depth the
same way it is in Section 3, but jpype and jpy's `array->buffer` bulk
path pulls further ahead as depth grows, since it scales with leaf-array
count rather than total element count.""")

# ---------------------------------------------------------------
h(2, '6. Array push, ragged (jagged, non-rectangular)')
p("""**Methodology.** `array_ragged.py`. A nested Python list whose
sub-lists have varying lengths (a genuinely jagged/ragged structure, not
a rectangular array-of-arrays), pushed fresh into a Java array-of-arrays
parameter, depth 2-5, ~10^depth total elements.""")

rows = []
for t in DTYPES:
    for dep, pow_ in DEPTHS:
        br = BRACKETS[dep]
        label_core = f"{t}{br}(~{pow_})"
        row = [label_core]
        any_ = False
        for lib in LIBS:
            rec = get(lib, 'array_ragged').get(f"ragged list->array {label_core}, fresh")
            row.append(fmt(rec['best']) if rec else '--')
            if rec:
                any_ = True
        if any_:
            rows.append(row)
out.append(table(['shape'] + LIBS, rows))
out.append('')
p("""_jpype numbers reflect the list/tuple-specialized ragged-native readout
-- see Section 11._""")
out.append('')
p("""**Result.** jpype leads at every depth/type (previously trailed jpy
2.2-2.5x here -- see Section 11 for the fix); the gap to jpy widens
with depth (both walk the ragged structure recursively, jpype's
ragged-native encode path stays closer to linear in total elements).""")

# ---------------------------------------------------------------
h(2, '7. Array push/pull, shape at fixed depth and total element count')
p("""**Methodology.** `array_shape.py`. Fixed total element count
(~10,000 or ~100,000), depth held at 3, but the *shape* varies (e.g.
`[1000][10][10]` vs. `[10][10][1000]`) to isolate whether cost tracks
total elements or leaf-array count / row-heaviness. `list->array` and
`buffer->array` (jep: manual per-row) directions only.""")

def shape_rows(script, direction, key):
    data = {lib: get(lib, script) for lib in LIBS}
    all_labels = []
    seen = set()
    for lib in LIBS:
        for label, rec in data[lib].items():
            if label.startswith(direction) and key in label and label not in seen:
                all_labels.append(label)
                seen.add(label)
    rows = []
    for label in all_labels:
        core = label[len(direction) + 1:]
        row = [core]
        any_ = False
        for lib in LIBS:
            variants = [label, label.replace(key, key + ', manual per-row')]
            rec = lookup_variants(lib, script, variants) or data[lib].get(label)
            row.append(fmt(rec['best']) if rec else '--')
            if rec:
                any_ = True
        if any_:
            rows.append(row)
    return rows

h(3, '`list->array` push')
rows = shape_rows('array_shape', 'list->array', '[')
out.append(table(['shape'] + LIBS, rows))
out.append('')

h(3, '`buffer->array` push (jep: manual per-row)')
rows = [r for r in shape_rows('array_shape', 'buffer->array', '[') if 'manual per-row' not in r[0]]
out.append(table(['shape'] + LIBS, rows))
out.append('')
p("_pyjnius: no entry -- no buffer->array push at any shape._")
out.append('')

p("""**Result.** At fixed total element count, a row-heavy shape (many
short rows, e.g. `[100000][3]`) costs more than a column-heavy one
(few long rows, e.g. `[3][100000]`) in every library that has a bulk
path -- more leaf arrays means more per-leaf JNI/reflection overhead
even though total elements is unchanged. The penalty is much larger for
`list->array` (recursive per-row Python-level walk regardless of
library) than for `buffer->array` (jpype/jpy's bulk path pays only
per-leaf-array overhead, not per-element).""")

# ---------------------------------------------------------------
h(2, '8. jpype-only microbenchmarks')
p("""These three scripts (`array_to_list_dtype.py`, `arraytransfer.py`,
`classhints.py`) have no equivalent in the jpy/jep/pyjnius suites --
they exercise jpype-internal API surface (`toList()` dtype variants,
`pullTo`/`pushFrom` bulk in-place transfer, `JPConversionList`/
`JPConversionTuple`'s cached-class-hint lookup) with nothing to compare
against. jpype-only, `best` ns/call.""")

h(3, '`list()` vs. `toList()` dtype variants')
rows = []
for r in d['jpype']['array_to_list_dtype']:
    rows.append([r['label'], fmt(r['best'])])
out.append(table(['operation', 'jpype'], rows))
out.append('')

h(3, 'Bulk in-place transfer (`pullTo`/`pushFrom`) vs. naive per-element')
rows = []
for r in d['jpype']['arraytransfer']:
    rows.append([r['label'], fmt(r['best'])])
out.append(table(['operation', 'jpype'], rows))
out.append('')

h(3, 'Class-hint cache lookup cost vs. registered-class-count')
rows = []
for r in d['jpype']['classhints']:
    rows.append([r['label'], fmt(r['best'])])
out.append(table(['operation', 'jpype'], rows))
out.append('')

p("""**Result.** `pullTo`/`pushFrom` beat their naive per-element
counterparts by roughly one to two orders of magnitude at 100,000+
elements. `direct-buffer-shared` (steady-state cost once a direct
buffer is already set up) is the cheapest transfer path at every size.
`classhints` cache lookup cost is flat from 1 to 400 registered classes
-- confirms the cache is a real O(1) lookup, not a linear scan that
happens to be fast at small N.""")

# ---------------------------------------------------------------
h(2, '9. GraalPy: a true-JIT comparison point (not re-run this edition)')
p("""GraalPy (Python-in-the-JVM, opposite architecture from
jpype/jpy/pyjnius, same direction as jep) was not part of this edition's
re-run -- it needs a separate GraalVM CE + Maven/Truffle setup (see
`project/benchmark/README.md`) and was out of scope for this pass. The
subsections below are carried forward **unchanged** from the previous
edition of this report; their internal `Section N` cross-references
point to *that* edition's section numbers, not this document's current
numbering.""")
with open(GRAALPY_SECTION_PATH) as f:
    out.append(f.read())
out.append('')

# ---------------------------------------------------------------
h(2, '10. Known limitations of this run')
p("""- **jep, `array_multidim.py`**: hit `java.lang.OutOfMemoryError`
  partway through the `long` type sweep even at `-Xmx3g` (this
  machine's stock ergonomic default was ~2GB; raising to 4GB and then
  6GB each let it get further into the sweep before still OOMing --
  6GB pushed total system memory to the edge of exhaustion on this
  7.7GB machine and was not pursued further). This happens only in
  `array_multidim.py`, which runs all four (type x direction) sweeps
  back-to-back in one JVM process; the other jep array scripts
  (`array_ragged.py`, `array_noncontig.py`, `array_shape.py`), each a
  narrower slice of the same workload, complete cleanly at the same
  depths/sizes under the stock default heap. That a larger heap
  measurably postpones but does not prevent the failure, combined with
  it being specific to the single long-running combined-sweep process,
  points at accumulated garbage outliving each `timeit()` batch (an
  allocation-rate-vs-GC-throughput problem) rather than a fixed
  working-set size that a given heap either does or doesn't fit --
  flagged here as a real, reproducible finding, not investigated
  further. Section 5's `array_multidim` tables show `int` in full and
  `long`/`float`/`double` only as far as this run reached before
  failing (missing cells read `--`).
- **pyjnius, `array_noncontig.py`**: intentionally a no-op -- pyjnius
  has no buffer->array push at all (confirmed empirically: any buffer
  source raises `JavaException('Expecting a python list/tuple, got
  array(...)')` unconditionally), so there is no non-contiguous-source
  case to measure separately from Section 3's finding.
- **GraalPy**: not re-run this edition; see Section 9 for the last
  captured numbers and their own methodology/caveats.""")

# ---------------------------------------------------------------
h(2, '11. Where to focus next')
p("""- **Resolved since the numbers above were first captured: `list->array`
  push, depth >= 2 (both rectangular and ragged) used to lose to jpy's
  naive per-element recursion by a consistent 2.2-2.5x, despite jpype
  having a dedicated ragged-native fast path
  (`isRaggedLeafElement`/`matchRaggedNode`/`encodeRaggedNode`,
  `jp_classhints.cpp`) that jpy has no equivalent of at all -- every jpy
  push there is a generic `PySequence_GetItem` recursion. Root cause:
  `matchRaggedNode`/`encodeRaggedNode` read every node's contents via
  the generic `JPPySequence` wrapper (`PySequence_Size`/
  `PySequence_GetItem` -- protocol dispatch, owned reference per
  element), at every node, in both the validation and encode passes.
  This is exactly the cost `JPClass::sequenceCheckList`/
  `sequenceCheckTuple` (`jp_class.h`) already exist to eliminate for the
  flat (1D) push path via `PyList_GET_ITEM`/`PyTuple_GET_ITEM` (direct
  index, borrowed reference, no dispatch) -- the ragged-native path had
  never gotten the equivalent treatment. Fix: classify each node once
  (list/tuple/generic) and use type-specific loops instead of the
  one-size-fits-all `seq[i]` path, in both passes. Measured via isolated
  `git worktree` + fresh venv before/after: `list->array` push at depth
  2-5 is 2-4.4x faster for both rectangular and ragged shapes across all
  four leaf types, closing and reversing the deficit -- jpype now leads
  jpy at every depth/shape in Sections 5 and 6 above (e.g. rectangular
  `int[][][][][](10^5)`: was 4,447,629 vs. jpy 1,757,121 (jpy 2.53x
  faster), now 1,088,620 vs. the same jpy figure (jpype 1.6x faster);
  ragged `int[][][][][](~10^5)`: was 5,108,332 vs. jpy 2,139,732 (jpy
  2.39x faster), now 1,389,415 vs. the same jpy figure (jpype 1.5x
  faster)).""")

print('\n'.join(out))
