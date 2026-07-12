# Document toPython() as a general forward-facing conversion convention

## Status (2026-07-11): scoped, not started; depends on plan/ToPython.md landing first

## Where this fits

This is a **forward-bridge** documentation task (Python calling Java),
not part of `plan/DOCS.md`'s reverse-bridge manual (Java calling Python) -
don't fold it into that mirror-mapping table, it's a different audience
and a different existing manual (`doc/userguide.rst` directly, which
already documents the forward direction end-to-end).

`doc/userguide.rst` already has a real section for this, titled
"Customizing java.io Streams" (`====`-level heading, ~line 5219), with the
sub-structure Overview (~5225) / Customized Methods (~5245) / Examples
(~5271). It documents `toPython()` **only** for the `java.io` stream
hierarchy (`Writer`/`Reader`/`OutputStream`/`InputStream`), written as if
`toPython()` were a one-off feature specific to streams rather than a
general pattern. It sits structurally next to "Examples with Java Thread"/
"Best Practices for Java Thread" (~line 5157-5217), i.e. under the same
`java.lang.Thread`-customization part of the manual (the "Customization"
chapter per `plan/DOCS.md`'s existing chapter list).

## The problem

Once `plan/ToPython.md` lands (renaming `_py()` → `toPython()` on
`java.sql.Date`/`Time`/`Timestamp`/`java.math.BigDecimal`, and adding new
`toPython()` customizers for `java.time.Instant` and
`java.nio.file.Path`/`java.io.File`), there will be seven types across
four unrelated Java package families (`java.io`, `java.sql`, `java.time`,
`java.math`, `java.nio.file`) all implementing the same convention, but
the manual will only document one family (`java.io`) and readers have no
way to discover that the same method name means the same thing elsewhere,
or that it's a general pattern they could plausibly expect on other
customized types.

## Scope

1. **Generalize the existing "Customizing java.io Streams" section's
   framing** — add a short preamble (either as part of that section, or
   as a new small section immediately before/after it, whichever reads
   better once drafted) stating the general convention plainly: any
   JPype-customized Java type may implement `toPython()` to hand back a
   genuine, independent Python-native object; this is the reverse of
   `JConversion`-based automatic argument coercion; not every customized
   type needs one (streams need it because there's no automatic
   coercion target; simple value types get it as a convenience). Base
   this framing on `plan/ToPython.md`'s own "the problem" section, don't
   redrive the rationale from scratch.
2. **Document each new/renamed `toPython()`** landed by `plan/ToPython.md`,
   matching the existing `java.io.Writer.toPython(...)` entries' format
   (`.. method::` signature block + one-line description of what's
   returned) — `java.time.Instant.toPython()`, `java.sql.Date.toPython()`,
   `java.sql.Time.toPython()`, `java.sql.Timestamp.toPython()`,
   `java.math.BigDecimal.toPython()`, `java.nio.file.Path.toPython()`,
   `java.io.File.toPython()`.
3. **Cross-reference the matching forward `JConversion`** next to each
   entry (e.g. `Instant.toPython()`'s doc should mention "see also: passing
   a `datetime.datetime` to a Java method expecting `Instant`" and point at
   wherever the existing forward-conversion behavior is documented
   elsewhere in the manual, or add it if it isn't documented yet — check
   first, don't assume the forward side is already written up anywhere
   just because the code exists in `jpype/protocol.py`).
4. **`CHANGELOG.rst`** — covered by `plan/ToPython.md` itself (already in
   scope there), no duplicate work needed here.

## Explicitly out of scope

- Don't touch `plan/DOCS.md`'s reverse-bridge manual — this is unrelated
  to that effort even though both are "documentation."
- Don't invent new `toPython()` customizers here — that's
  `plan/ToPython.md`'s job. This plan is docs-only, and should not start
  until that code lands (the manual would otherwise document methods that
  don't exist yet).

## Verification

- Sphinx build (`doc/` build target, whatever the project's normal doc
  build command is - check `doc/Makefile`/`tox.ini`/CI config rather than
  guessing) succeeds with no new warnings.
- Every `.. method::` entry added actually matches the real method
  signature in the shipped code at time of writing (same "ground truth
  from tests, not memory" discipline `plan/DOCS.md` already established -
  don't hand-write signatures from intent).
</content>
