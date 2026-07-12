# Naming/typo cleanup across the SPI + python.io + adjacent code

## Status (2026-07-10): DONE - committed 444587f4, folded into reverse's
squashed history (47f97604), verified live in the tree (no `PyIoWrapperService`/
`PyKeyArgs` remnants remain)

User-flagged: "PyIoWrapperService using Io instead of IO, PyKeyArgs, etc
... a lot of random typos, lazy naming and other things we commit to once
released." Grounded in a repo-wide Explore survey this session. Split into
"fix now" (mechanical, low blast radius, executed this pass) vs. "flag,
don't touch" (coordinated Java/native/Python wire-name risk, needs its own
dedicated look).

## Fix now

1. **`PyIoWrapperService` → `PyIOWrapperService`** (file + class rename).
   The sole `Io`-cased identifier in a package where every sibling
   (`PyIOBase`, `PyBytesIO`, `PyIOInputStream`, `PyIOOutputStream`, ...)
   uses all-caps `IO`. References to update: `WrapperService.java`'s
   javadoc, `python/io/package-info.java`, `module-info.java`'s
   `provides...with`, `META-INF/services/org.jpype.WrapperService`,
   `PyLazySpiNGTest.java`.

2. **`PyKeyArgs` → `PyKwArgs`** (file + class rename), `.kw()` method kept
   as-is. Unifies with the dominant `kwargs`/`Kwargs`/`KwCall` vocabulary
   used everywhere else for the identical concept (`PyCallable
   .callWithKwargs`, `jpype/_jbridge.py`'s `kwargs` params throughout,
   `KwCallRegistry`/`KwCall` test helpers) rather than the one-off
   "KeyArgs" spelling that appeared nowhere else. References to update:
   `org/jpype/proxy/ProxyInstance.java`, test file `PyKeyArgsNGTest.java`
   → `PyKwArgsNGTest.java`, `native/common/jp_proxy.cpp`'s comment.

3. **Stale `// --- file: .../bridge/... ---` header-comment typo.** First
   line of ~19 files points at a nonexistent `org.jpype.bridge` package or
   an old/renamed class name (confirmed via `find . -type d -name bridge`
   — no such package exists anywhere in the tree). Pure one-line-per-file
   mechanical fix — replace with the file's actual path. Full file list
   from the survey:
   - `org/jpype/Script.java` → claims `org/jpype/bridge/Context.java`
   - `org/jpype/MainInterpreter.java` → claims `org/jpype/bridge/Interpreter.java`
     (also fix the same stale package name inside a logging-config javadoc
     example at line 62: `org.jpype.bridge.Interpreter.level=INFO`)
   - `org/jpype/WrapperService.java` → claims `org/jpype/bridge/WrapperService.java`
     (also its own class-doc usage example at line 9 says
     `org.jpype.bridge.WrapperService` — fixed alongside the Javadoc pass,
     noted here since it's the same stale-name root cause)
   - `org/jpype/Backend.java` → claims `org/jpype/bridge/Backend.java`
   - `org/jpype/internal/NativeLauncherControl.java` → claims `org/jpype/bridge/Natives.java`
   - `org/jpype/BootstrapLoader.java` → claims `org/jpype/bridge/BootstrapLoader.java`
   - `python/lang/package-info.java` → javadoc references
     `{@link org.jpype.bridge.BuiltIn#eval eval}` (nonexistent package)
   - `org/jpype/Reflector.java` → claims `org/jpype/JPypeReflector.java`
   - `org/jpype/proxy/ProxyFactory.java` → claims `org/jpype/proxy/JPypeProxyFactory.java`
   - `org/jpype/proxy/ProxyInstance.java` → claims `org/jpype/proxy/JPypeProxyInstance.java`
   - `org/jpype/proxy/ProxyType.java` → claims `org/jpype/proxy/JPypeProxyType.java`
   - `org/jpype/proxy/MethodDescriptor.java` → claims `org/jpype/proxy/JPypeMethodDescriptor.java`
   - `org/jpype/internal/Support.java` → claims `org/jpype/JPypeUtilities.java`
   - `org/jpype/internal/Keywords.java` → claims `org/jpype/JPypeKeywords.java`
   - `org/jpype/internal/FunctionalAdapters.java` → claims `org/jpype/internal/Utility.java`
   - `org/jpype/internal/Signal.java` → claims `org/jpype/JPypeSignal.java`
   - `org/jpype/pkg/Package.java` → claims `org/jpype/pkg/JPypePackage.java`
   - `org/jpype/pkg/PackageManager.java` → claims `org/jpype/pkg/JPypePackageManager.java`
   - `org/jpype/manager/StringManager.java` → claims `org/jpype/internal/JPypeStringManager.java`
   - `python/exceptions/PyIsADirectoryError.java` → claims
     `python/exception/PyADirectionError.java` (wrong package, singular
     `exception`; also a garbled class name `PyADirectionError`)

4. **`getBuildIn()` → `getBuiltIn()` typo.** Two call sites:
   `org/jpype/internal/NativeContext.java:159` (the method definition) and
   `org/jpype/proxy/ProxyType.java:53` (its one caller). Note the
   correctly-spelled `getBuiltIn()` already exists on `MainInterpreter`
   (`org/jpype/MainInterpreter.java:220`) and the `Interpreter` interface
   (`org/jpype/Interpreter.java:10`) — two different types expose the same
   concept, one misspelled, one not; this fix makes them consistent.

5. **`interpeter` → `interpreter` param-name typo**, `org/jpype/Script.java`
   (lines 41, 43, 48, 50 — 4 occurrences). Local parameter name only, zero
   external API impact.

6. **`"Sanning jar"` → `"Scanning jar"` log message**,
   `org/jpype/internal/NativeContext.java:455`.

7. **`MainInterpreter.setInstaller(Installer entry)`** — rename parameter
   `entry` → `installer`, matching every other reference to the same value
   in the same file/class (field name, `getInstaller()`, `SpiLoader.load(installer)`
   all already say "installer"; only this one parameter says "entry").

## Flag, don't touch this pass

Coordinated Java/native/Python wire-name risk — needs its own dedicated
look before touching, not part of this mechanical pass:

- **`Backend.java`'s singular/plural drift**:
  `mappingRemoveAllValue`/`mappingRetainAllValue` (singular) vs.
  `mappingRemoveAllKeys`/`mappingRetainAllKeys`/`mappingContainsAllValues`
  (plural). The Java method name doubles as a literal Python
  dispatch-dict string key in `jpype/_jbridge.py` (lines 317, 320), so a
  rename must touch both sides atomically, and the blast radius on any
  other consumer of that exact string isn't yet confirmed. Also
  `newXFromIterator` methods that actually take `Iterable`, not
  `Iterator` (`newByteArrayFromIterator`, `newBytesFromIterator`,
  `newTupleFromIterator`) — same Java/Python-dispatch-key coupling risk.
- **`WrapperService`'s generic "Wrapper" noun.** No clearly better
  alternative identified during the survey; renaming a public SPI
  entry-point interface is high blast radius for low, unclear benefit.
  Left as-is.
- **`NativeContext.registerBackend` vs. `Installer.registerBackend`**
  sharing one name for two different concepts (one stores an SPI
  mini-backend instance per-interpreter, the other replays a `.pyspi`
  "backend" registration into the Python dispatch layer). Both names are
  locally sensible on their own; better resolved via clarifying prose in
  the Javadoc pass (`plan/Javadoc.md`) than by forcing a rename.

## Verification

- `mvn -q test -Dpython.executable=python3.10` — full suite, expect
  475/475 unchanged (pure rename/typo fix, no behavior change).
- Grep-confirm zero remaining references to old names: `PyIoWrapperService`,
  `PyKeyArgs`, `getBuildIn`, `interpeter`, `org.jpype.bridge`,
  `org/jpype/bridge`.
- Rebuild the real `ant`-built jar (`ant clean && ant`, copy to repo root)
  and rerun `test_io.py`/`test_lazy_spi.py` standalone via
  `python3.10 <script>.py` — these exercise the renamed `IO.java`/
  `PyIOWrapperService` code paths from a plain launched script, not just
  the NGTest harness.
