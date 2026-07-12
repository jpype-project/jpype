# python.re: SPI provider for Python's re module

## Status (2026-07-11): scoped, not started, opportunistic (lower priority than Datetime/Decimal/Pathlib)

## The problem

No Java front-end exists for `re.Pattern`/`re.Match`. Narrower audience
than `datetime`/`decimal`/`pathlib` (most regex work happening on the Java
side would just use `java.util.regex` directly), but worth doing once the
SPI pattern is well-proven since the protocol surface is small.

See `plan/archive/Collections.md` for the general "why SPI, not core" rationale
and `plan/archive/SPI.md` / [[jpype_spi_installer_status]] for the
mechanism.

## Scope

- `re.Pattern` → `PyPattern` — `match`, `search`, `fullmatch`, `findall`,
  `sub`, `split`; construction via `re.compile(pattern, flags)`.
- `re.Match` → `PyMatch` — `group`/`groups`/`groupdict`, `start`/`end`,
  `span`.

Both types' `__module__` is `re` uniformly (no `io`/`_io`-style split
expected, but confirm on a real interpreter rather than assuming, same as
every other plan in this batch).

## Design questions to resolve before coding

- Whether flag constants (`re.IGNORECASE` etc.) need Java-side mirrors or
  can just be passed through as ints via `PyBuiltIn.eval`/direct attribute
  access to the Python `re` module — likely the latter, avoid inventing a
  parallel Java enum unless there's a concrete reason to.
- Whether `PyMatch`'s group accessors should return `PyString`/`String` or
  support the `None`-for-unmatched-group case cleanly — check the
  [[jpype_boxed_null_reverse_bridge_gotcha]] memory before assuming a
  fixed-arity boxed return type behaves the way Java `null` normally
  would.

## Naming-mangling interaction

Same caveat as the other plans in this batch: verify the current `$`/`.`
mangling convention against a live `.pyspi` file before writing new ones.

## Steps (mirror `plan/archive/Collections.md`)

1. Design `PyPattern`/`PyMatch`'s method surfaces.
2. `.pyspi` resources under
   `native/jpype_module/src/main/resources/python/re/spi/`.
3. `PyReWrapperService` implementing `WrapperService`, registered in
   `module-info.java`.
4. End-user Javadoc, `python/re/package-info.java`.
5. Tests covering match/no-match/group access, including the `None`-group
   edge case above.

## Verification

- Full suite green on python3.10 and python3.12, no collision with
  whichever other SPI providers have landed by the time this runs.
</content>
