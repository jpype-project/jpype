# python.decimal: SPI provider for Python's decimal module

## Status (2026-07-11): scoped, not started

## The problem

No Java front-end exists for `decimal.Decimal`. Any Python API returning
a `Decimal` currently crosses into Java as a bare `PyObject` with no typed
methods and no conversion to `java.math.BigDecimal`. This matters
specifically for finance/numeric-precision use cases, a plausible reason
someone reaches for the reverse bridge in the first place (Python's
`decimal` exists precisely because `float` isn't safe for money).

Scoped alongside `plan/archive/Collections.md` and `plan/Datetime.md` as one of
the "prove the SPI mechanism generalizes past `python.io`" candidates. See
`plan/archive/SPI.md` / [[jpype_spi_installer_status]] for the mechanism
itself.

## Existing forward-direction integration (do not confuse with this plan)

JPype already has a forward-bridge (Python value satisfying a Java method
contract) `JConversion` customizer for exactly this type:
`decimal.Decimal` → `java.math.BigDecimal` in `jpype/protocol.py` (~line
175). That path is structural/per-call — it fires when a `Decimal` is
passed *into* a Java method expecting `BigDecimal`, constructing a fresh
`BigDecimal` each time, no persistent identity — and is unrelated to this
plan's actual goal, which is giving Java code a typed front-end object for
a `Decimal` value it received *from* Python (the reverse direction). No
conflict, but it does mean the string-round-trip behavior between
`Decimal` and `BigDecimal` has likely already been worked out once on the
forward path — check that conversion function's implementation before
re-deriving the same round-trip logic for `PyDecimal.toBigDecimal()`.

## Why SPI, not core

Same reasoning as the other two: `WrapperService` provider (model:
`python.io.PyIOWrapperService`), registered via `module-info.java`, not
hand-wired into `_jbridge.py` or core `python.lang`.

## Scope: one type, but a real one

- `decimal.Decimal` → `PyDecimal` — this is the entire scope; `decimal`'s
  other public surface (`Context`, `localcontext`, rounding-mode
  constants) is process-global configuration state, not a value type, and
  is almost certainly out of scope for a first cut. Don't expand beyond
  `Decimal` itself without discussing first.

## Design questions to resolve before coding

- Promotion to `java.math.BigDecimal` — likely the single most valuable
  method on this type (`toBigDecimal()`), since `BigDecimal` is the
  natural Java-side representation for the same problem `Decimal` solves.
  Round-trip both directions: confirm `BigDecimal.toString()`'s output is
  always accepted by `Decimal(str)` (it should be, but verify against a
  real interpreter, don't assume).
- Whether to also support constructing a `PyDecimal` from a Java
  `BigDecimal` without a Python round-trip for the string conversion, or
  always go through `Decimal(str(bigDecimalValue))` via the interpreter —
  check `python.io`/`python.lang` precedent for how similar promotions
  are built before inventing a new pattern.
- Immutability: `Decimal` is immutable in Python; `PyDecimal` should be
  too, same spirit as `PyBytes`/`PyTuple`.
- Arithmetic: Python's `Decimal` overloads `+`, `-`, `*`, `/` etc. Confirm
  whether this needs the same `eval`-escape-hatch treatment
  `PyAbstractSet`'s Javadoc points operator-based set algebra at, or
  whether `Decimal`'s arithmetic is common enough to warrant real Java
  methods (`add`, `subtract`, ...) — this is a real design call, not
  mechanical, make it deliberately rather than defaulting to whichever is
  less work.

## Naming-mangling interaction

Same caveat as the other two plans: `PyObject`-rooted proxy interface, so
`.pyspi` method keys need whatever the current `$`/`.` mangling convention
is at execution time — verify against a live `.pyspi` file.

## Steps (mirror `plan/archive/Collections.md` / `plan/archive/IO.md`)

1. Design `PyDecimal`'s method surface, resolving the design questions
   above against a real interpreter first.
2. One `.pyspi` resource under
   `native/jpype_module/src/main/resources/python/decimal/spi/`.
3. `PyDecimalWrapperService` implementing `WrapperService`, registered in
   `module-info.java`.
4. End-user Javadoc at the `plan/archive/Javadoc.md` "Audience 1" bar, plus
   a real `python/decimal/package-info.java`.
5. Tests: construction, `toBigDecimal()`/`BigDecimal` round-trip, whatever
   arithmetic surface got decided above, comparison.

## Verification

- Full suite green on python3.10 and python3.12.
- Confirm coexistence with `python.io` and whichever other SPI providers
  have landed by the time this runs — no registration-order or
  module-name collisions.
</content>
