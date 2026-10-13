# RESULTS.md generation tooling

Regenerates `project/benchmark/RESULTS.md` from a fresh, full run of the
jpype/jpy/jep/pyjnius benchmark suites, instead of hand-transcribing
numbers edition to edition. GraalPy is not part of this pipeline (see
`graalpy_section.md` below).

## Workflow

1. Run every script in `project/benchmark/{jpype,jpy,jep,pyjnius}/*.py`
   per that library's setup in `project/benchmark/README.md`, redirecting
   each script's output into `<logdir>/<library>/<script-name>.log`
   (`.txt` for jep, since it writes to an explicit `out_path` rather than
   stdout).
2. `python3 parse_logs.py <logdir> [parsed.json]` -- parses every
   `label  best=... ns/call  median=... ns/call` line out of those log
   files into one `parsed.json`.
3. `python3 gen_results.py <parsed.json> [graalpy_section.md] > RESULTS.md`
   -- turns `parsed.json` into the full markdown report (methodology +
   table + result per section), inserting `graalpy_section.md`'s static
   text as Section 9.

`parsed.json` in this directory is the data behind the current
`RESULTS.md` (a re-run of the full suite with raised statistics --
`trials=7`, higher `calls_for()` floors -- see the scripts' own diffs
and `RESULTS.md`'s Global Methodology section). Regenerating from a new
`parsed.json` reproduces `RESULTS.md` byte-for-byte given unchanged
label text in the scripts; if a script's printed label wording changes,
update the matching logic in `gen_results.py`'s `direction_table`/
`multidim_direction`/`shape_rows` helpers to match.

## `graalpy_section.md`

Static, hand-maintained snapshot of the GraalPy comparison (Section 9).
GraalPy needs a separate GraalVM CE + Maven/Truffle setup (see
`project/benchmark/README.md`) and was out of scope for the automated
jpype/jpy/jep/pyjnius pipeline above. Refresh it manually by re-running
`project/benchmark/graalpy/run_all.sh` and hand-editing this file when
GraalPy itself is due for a re-run -- there is no parser for it.

## Known limitation this tooling does not paper over

jep's `array_multidim.py` can hit `java.lang.OutOfMemoryError` partway
through at raised statistics, even at a raised `-Xmx`, because it runs
every (type x direction) combination back-to-back in one JVM process --
see `RESULTS.md`'s "Known limitations of this run" section. `gen_results.py`
handles this gracefully (missing cells render `--`), but a future re-run
should expect to hit the same wall unless jep's own GC/allocation
behavior at this workload shape changes.
