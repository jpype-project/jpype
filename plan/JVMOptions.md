# Composable pre-launch JVM options (addJVMOption)

## Status (2026-07-11): scoped, not started

Follow-on from the pyjnius comparison (see `jpype_gold_standard_mission`
memory / prior-art survey series). pyjnius's `jnius_config` module
(`~/jcef/pyjnius/jnius_config/__init__.py`) exposes `set_options()`/
`add_options()`/`get_options()` alongside `set_classpath()`/
`add_classpath()` — any independent piece of code can accumulate JVM
options (memory flags, GC settings, `-D` properties, etc.) at any point
before the JVM actually starts, without needing to own the call site that
triggers the launch. This is the one real ergonomic edge pyjnius showed in
the whole prior-art survey series (jep, jpy, GraalVM, pyjnius) — everything
else jpype already matches or beats, including having no equivalent to
jpype's `@` cast operator or signature-free `@JImplements` proxies.

## Current state

jpype already has the classpath half of this: `jpype._classpath.addClassPath()`
(`jpype/_classpath.py:31`) is composable — callable from anywhere, any
number of times, before *or* after `startJVM()` (it pushes straight into
the running classloader if the JVM is already up). Not a gap.

JVM options/flags have no equivalent. `startJVM(*jvmargs, ...)`
(`jpype/_core.py:236`) is a single call that takes all extra JVM args as
positional arguments; there is no module-level accumulator a library could
call into during its own import/init time to contribute flags before some
other, unrelated part of the app decides it's time to actually call
`startJVM()`. In a plugin-style app where several independent libraries
each want to request their own JVM flags, jpype currently forces all of
that to funnel through whoever owns the single `startJVM()` call site.

## Goal

Mirror the existing `addClassPath`/`getClassPath` pattern with an options
equivalent:

```python
import jpype
jpype.addJVMOption("-Xmx2g")
jpype.addJVMOption("-Djava.awt.headless=true")
...
jpype.startJVM()  # picks up accumulated options automatically
```

## Design sketch

- New functions in `jpype/_core.py` (or a small new module, matching
  `_classpath.py`'s shape): `addJVMOption(opt: str) -> None`,
  `getJVMOptions() -> list[str]`, backed by a module-level
  `_JVM_OPTIONS: list[str]` list, mirroring `_classpath._CLASSPATHS`.
- `startJVM()` merges `_JVM_OPTIONS` into `extra_jvm_args`
  (`jpype/_core.py:339`-ish, alongside the existing `--add-modules`/
  `--add-opens` merge logic) — accumulated options come first, explicit
  `*jvmargs`/keyword args passed directly to `startJVM()` still take
  precedence/are appended after, so a caller can always override.
- Unlike `addClassPath`, there's no "JVM already started" live-injection
  path to support — JVM options are fundamentally launch-time-only (no
  `-Xmx` after the fact). `addJVMOption()` after `isJVMStarted()` is `True`
  should raise the same `OSError` style used elsewhere for
  already-started-JVM misuse, not silently no-op.
- Consider whether accumulated options should be de-duplicated (e.g. two
  libraries both calling `addJVMOption("--enable-native-access=org.jpype")`)
  or left as-is and let the JVM itself complain about conflicting flags —
  lean toward leaving it as-is, matching pyjnius's own `add_options()`
  (append-only, no dedup) and jpype's existing `_merge_options` helper
  already dedups the specific mandatory-modules/opens sets where it matters.

## Testing

- Two independent "modules" (simulated by two functions/files) each call
  `addJVMOption()` before a third piece of code calls `startJVM()` with no
  args; assert both options landed in the actual JVM invocation (e.g. a
  `-D` property readable via `System.getProperty()` after start).
- Assert `addJVMOption()` raises after the JVM is already started.
- Assert explicit `startJVM(*jvmargs)` args and accumulated
  `addJVMOption()` calls compose (both present) rather than one silently
  overriding the other.

## Note for context (not an action item)

Not directly related to this plan, but worth recording as market-survey
context: jpype has been ported to and fully tested on Android — every test
in the suite runs there. pyjnius, despite being the Android/Kivy-oriented
bridge, segfaulted on most tests in that same environment and jpype's
Android port work found nothing usable to build on from pyjnius's side.
