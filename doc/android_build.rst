Android Build/Test Harness
===========================

This page documents ``project/android/``, a project-local harness for actually
cross-compiling JPype's native ``_jpype`` extension for Android, packaging it
into a minimal test app, and running it on an emulator. It exists because the
:doc:`android` feature/behavior differences page describes what the
``#ifdef ANDROID`` code in ``native/`` is *supposed* to do, but until this
harness there was no way to actually build or run that code from this repo -
issue `#1257 <https://github.com/jpype-project/jpype/issues/1257>`_ (an
Android crash) had to be root-caused and fixed from a stack trace and static
analysis alone.

This is project-local developer tooling, not something JPype ships or
installs for end users - it is not referenced from ``pyproject.toml`` and has
no effect on a normal ``pip install jpype1``.

Background: why buildozer/python-for-android
----------------------------------------------

Two options were evaluated: `Chaquopy <https://chaquo.com/chaquopy/>`_ and
`buildozer/python-for-android (p4a) <https://python-for-android.readthedocs.io/>`_.
Chaquopy was ruled out: its ``pip install`` mechanism explicitly refuses to
compile native C extensions (there is a public bug report of someone hitting
exactly this wall trying to install a package that depends on ``jpype1``), and
there is no documented way to bundle a prebuilt ``.so`` around that
restriction either.

p4a fits: its sibling project **pyjnius** (also a Python<->Java bridge over
JNI, architecturally the same problem JPype has) already ships a working
recipe (``pythonforandroid/recipes/pyjnius``) that this harness's own recipe
(``project/android/recipes/jpype1``) is modeled on, including reusing the same
``WebView_AndroidGetJNIEnv()`` hook that several p4a bootstraps expose for
getting a ``JNIEnv*`` from the already-running Dalvik/ART VM (despite the
name, it isn't specific to the ``webview`` bootstrap - ``service_only``,
the one this harness actually uses, exports the identical symbol from its
own bootstrap C sources; see the SIGABRT section below for why
``service_only`` and not ``webview``).

What's in ``project/android/``
-------------------------------

``native/android_jnienv.c``
  A reference implementation of ``Android_JNI_GetEnv()`` - declared
  ``extern`` in ``native/python/pyjp_module.cpp`` but deliberately never
  defined there, since it must come from whatever app embeds JPype. This
  file supplies it for p4a's ``service_only`` bootstrap (this harness's
  current choice - see the SIGABRT section below); it also works unchanged
  against the ``webview`` bootstrap, since both export the same
  ``WebView_AndroidGetJNIEnv()`` symbol. A different host application (a
  from-scratch Android Studio project, a different p4a bootstrap, ...)
  would supply its own definition instead - that's why this file lives
  here and not in ``native/``.

``recipes/jpype1/__init__.py``
  The p4a recipe. Builds directly from this repo's own working tree (not a
  release tarball) via a custom ``prepare_build_dir`` that ``rsync``\s the
  repo into the build dir (excluding ``.git``, any host ``build/`` directory,
  and this harness's own ``recipes``/``testapp`` subdirectories - a stale
  *host*-arch build must never leak into an Android cross-build, matching
  ``CLAUDE.md``'s isolation rule for normal builds too). Passes the NDK
  toolchain file and per-arch ABI/platform through to jpype's own CMake build
  using scikit-build-core's generic ``cmake.define.<VAR>=<value>``
  config-settings mechanism (the same mechanism ``CLAUDE.md`` documents for
  ``BUILD_TEST_HARNESS`` in the normal host build) - no changes needed on
  jpype's ``pyproject.toml`` side for that part. Its ``postbuild_arch`` copies
  the ``org.jpype.*`` Java sources (``native/jpype_module/src/main/java/org``)
  into p4a's Java class dir so they get compiled to dex and bundled into the
  APK, since ``JPClassLoader`` (``native/common/jp_classloader.cpp``) needs
  ``org/jpype/JPypeClassLoader`` to already be loadable on-device - Android
  can't do jar-based ``addClassPath`` (see :doc:`android`).

``testapp/``
  A minimal headless app (``buildozer.spec`` + ``main.py``) that imports
  jpype, calls a trivial ``JClass``, and runs a regression check for #1257
  (see comments in ``main.py`` for exactly what it checks and why calling
  ``_jpype.bootstrap()`` a second time is used as a stand-in for the original
  crash trigger, which depended on the original reporter's own modified
  bindings and isn't practical to reproduce exactly).

Changes to core ``native/`` and ``jpype/``
-------------------------------------------

Building for a real NDK target - not just adding the harness - surfaced
seven genuine bugs, all fixed in core, not worked around in this harness.
Each was root-caused by reading the actual source (not guessed), and each
was verified against the full host ``test/jpypetest`` suite (1756 passed,
173 skipped, zero regressions) before and after.

1. ``native/python/pyjp_module.cpp``: ``PyJPModule_bootstrap()`` (the
   Android entry point) was missing the ``JP_PY_TRY``/``JP_PY_CATCH``
   wrapper every sibling entry point has. A failure inside it (see #4/#6
   below - there were real failures to hit) threw a C++ exception straight
   across the C-linkage boundary into CPython instead of becoming a Python
   exception - this is #1257 itself.

2. ``native/common/jp_context.cpp``: ``AttachCurrentThread``/
   ``AttachCurrentThreadAsDaemon`` took a ``(void**) &env`` cast that's
   correct for desktop JNI headers, but Android's NDK ``jni.h`` declares
   the *same* functions with a ``JNIEnv**`` parameter instead - a real,
   incompatible header divergence between platforms, not a style
   difference. Fixed with an ``#ifdef ANDROID`` branch per call site (3
   total). Without this, ``native/`` doesn't even compile against the NDK.

3. ``native/jpype_module/.../org/jpype/JPypeContext.java``:
   ``getHeapMemory()`` used ``java.lang.management.MemoryMXBean``,
   unavailable on Android's platform API. Replaced with
   ``Runtime.getRuntime().totalMemory() - freeMemory()``, portable to every
   JVM - no platform branching needed since Java has no preprocessor.
   ``native/jpype_module/.../org/jpype/JPypeUtilities.java``'s sealed-class
   detection used ``MethodHandleProxies.asInterfaceInstance``, also
   unavailable on Android; replaced with a plain ``Method.invoke()``-based
   lambda, equally correct on desktop.

   Bugs 2 and 3 were also present, independently, in the original #1257
   reporter's own local patch - confirming they're real, load-bearing fixes
   any Android build needs, not artifacts of this particular harness.

4. **Reflector0 was missing from the Android build entirely.**
   ``JPypeContext.createContext()`` does
   ``Class.forName("org.jpype.Reflector0", true, loader)`` unconditionally
   (not Android-specific) to get a dedicated Java stack frame for invoking
   caller-sensitive methods correctly - JNI never gained an equivalent to
   ``@CallerSensitive``'s stack-walking caller check, so without a real Java
   frame sitting between JNI and the target call, a native caller invoking
   reflectively has no legitimate immediate caller for that check to see.
   ``native/build.xml`` deliberately excludes ``Reflector0.java`` from the
   main ``org.jpype.jar`` compile and compiles it separately into
   ``META-INF/versions/0/`` (its source lives at
   ``native/jpype_module/src/main/java/exclude/org/jpype/Reflector0.java``,
   a sibling of the ``org/`` tree, not inside it) - a JDK 8-era classloader
   "doesn't trust down" constraint requires Reflector0 to be *defined by*
   the DynamicLoader (or an ancestor), not just visible to it, which is why
   it's loaded reflectively with an explicit loader argument rather than
   referenced directly. This recipe's ``postbuild_arch`` was only copying
   ``.../java/org``, missing that sibling ``exclude/`` directory entirely -
   fixed by also copying ``Reflector0.java`` in as an ordinary source file.
   This surfaced as ``java.lang.RuntimeException: Unable to create reflector
   org.jpype.Reflector0`` - **not** a per-class bytecode-generation
   limitation on ART (there is no such generation; Reflector0 is one plain,
   static class that just calls ``method.invoke(obj, args)`` - an earlier
   version of this doc claimed otherwise, based on an unverified guess
   rather than reading the source; that was wrong).

5. **``ClassLoader.getSystemClassLoader()`` doesn't return a dex-visible
   loader on Android.** Even with Reflector0 correctly compiled in, the
   same ``Class.forName(..., loader)`` call still failed, because
   ``JPClassLoader``'s constructor (``native/common/jp_classloader.cpp``)
   built its wrapping ``JPypeClassLoader``'s parent from
   ``ClassLoader.getSystemClassLoader()``. That call exists to find
   whatever classloader JPype's own ``-Djava.system.class.loader`` JVM
   launch flag installed - a late-loading mechanism to work around JPMS
   module-boundary restrictions on a desktop JVM launch. Android has no
   such flag (``startJVM()`` never runs there) and no JPMS, so the call
   answers a question that doesn't apply on Android: it returns a minimal
   boot loader stub that can't see the app's own dex classes (including
   ``org.jpype.*`` itself) at all - never the real
   ``dalvik.system.PathClassLoader`` those classes actually load through.
   Fixed with an ``#ifdef ANDROID`` branch that instead asks for the
   classloader that defined ``org.jpype.JPypeClassLoader`` itself
   (``dynamicLoaderClass.getClassLoader()``), which is guaranteed to have
   that visibility on any platform. This *must* stay ``#ifdef ANDROID``:
   trying the same substitution unconditionally was tried first and broke
   two real desktop tests (``testNonASCIIPath``/``testOldStyleNonASCIIPath``
   in ``test_startup.py``) that depend on the genuine desktop
   ``getSystemClassLoader()`` late-load behavior.

6. **``attachJVM()`` never marked the context as running.**
   ``native/common/jp_context.cpp``'s ``attachJVM()`` - called exclusively
   from ``PyJPModule_bootstrap()`` on Android, no other caller - never set
   ``m_Running = true`` the way ``startJVM()`` does. ``isRunning()`` checks
   that flag unconditionally, so ``assertJVMRunning()`` threw
   ``JVMNotRunning`` on the very first ``JPJavaFrame`` created after a
   successful bootstrap - i.e. the first real use of JPype after import,
   such as ``JClass(...)``. One-line fix. This bug was masked by #4/#5
   above firing first; it only became visible once those were fixed.

7. ``jpype/_core.py``: the ``_JTerminate()`` ``atexit`` handler called
   ``_jpype.shutdown(...)`` unconditionally inside a bare
   ``except RuntimeError: pass``. Android never registers a ``shutdown``
   function (only ``bootstrap``, see ``jpype/__init__.py``'s
   ``hasattr(_jpype, 'bootstrap')`` check), so this raised an uncaught
   ``AttributeError`` at interpreter shutdown on every run, logged as
   "Exception ignored in atexit callback". Fixed by only registering the
   hook when ``hasattr(_jpype, 'shutdown')``, mirroring the existing
   ``bootstrap`` check.

``native/CMakeLists.txt`` also gained an ``ANDROID`` branch (checked before
the generic ``LINUX OR UNIX`` branch, which Android would otherwise fall
into since it's Linux-derived): links against NDK's ``libc++_shared.so``
instead of glibc's ``libstdc++.so.6``; adds ``--allow-shlib-undefined``/
``--unresolved-symbols=ignore-all`` so ``Android_JNI_GetEnv()`` can stay a
deliberately-unresolved-at-link-time reference; compiles in
``project/android/native/android_jnienv.c`` when present; and links against
an optional ``JPYPE_ANDROID_LIBMAIN_DIR`` (see the linker-namespace note
below). This is build-system plumbing rather than a "bug" in the same
sense as 1-7 above, but it's the other core-file change this harness
required.

Setup
-----

Install the Android SDK/NDK/emulator pieces (adjust versions/paths as
needed; this was verified against SDK cmdline-tools already present at
``~/android-sdk``)::

    ~/android-sdk/cmdline-tools/latest/bin/sdkmanager \
        "emulator" \
        "system-images;android-34;google_apis;x86_64" \
        "ndk;25.1.8937393"

    ~/android-sdk/cmdline-tools/latest/bin/avdmanager create avd \
        -n jpype-test -k "system-images;android-34;google_apis;x86_64"

p4a's toolchain (as of the version in use when this was written) still looks
for ``sdkmanager``/``avdmanager`` at the legacy ``tools/bin/`` location
rather than the current ``cmdline-tools/latest/bin/`` one - without this,
buildozer decides the SDK's sdkmanager "is not installed" and tries to
download a second copy of the whole SDK. Symlink the legacy path in::

    mkdir -p ~/android-sdk/tools/bin
    ln -sf ~/android-sdk/cmdline-tools/latest/bin/sdkmanager ~/android-sdk/tools/bin/sdkmanager
    ln -sf ~/android-sdk/cmdline-tools/latest/bin/avdmanager ~/android-sdk/tools/bin/avdmanager

Hardware acceleration needs the running user in the ``kvm`` group
(``sudo usermod -aG kvm $USER``, then a new login session) - otherwise the
emulator still works, just much slower (software rendering).

Use a JDK the bundled Gradle wrapper actually supports for ``JAVA_HOME``
during the build - a too-new JDK fails with
``BUG! exception in phase 'semantic analysis' ... Unsupported class file
major version NN`` (NN-44 is the Java version Gradle can't parse; e.g. 69
means Java 25). JDK 21 was verified working; a JDK newer than what your
Gradle wrapper version supports will not.

Per ``CLAUDE.md``'s build-isolation rule, use a disposable venv for the
build tooling too, kept separate from JPype's own dev venvs (this one never
touches JPype's C extension build directly - p4a drives its own
sub-builds). It must be created with ``--system-site-packages``: p4a's own
bootstrap does a plain ``pip install --user ...`` internally, and pip
unconditionally refuses ``--user`` inside any virtualenv unless
``site.ENABLE_USER_SITE`` is true, which only happens with
``--system-site-packages``::

    python3.12 -m venv --system-site-packages /tmp/venv-android
    /tmp/venv-android/bin/pip install --upgrade pip
    /tmp/venv-android/bin/pip install buildozer cython
    export PATH=/tmp/venv-android/bin:$PATH   # buildozer resolves `cython` etc.
                                               # via plain PATH lookup, not
                                               # sys.executable's directory

Build, deploy, run, verify
----------------------------

First, stage the ported ``test/jpypetest/*.py`` files into
``testapp/tests/`` - that directory holds no checked-in copies of its own
(other than ``__init__.py``, the manifest of which files to stage); see
``project/android/sync_tests.py``'s docstring::

    python3 project/android/sync_tests.py

Then, from ``project/android/testapp/``, with a JDK 21 (see above) on
``JAVA_HOME``::

    export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64   # adjust to your JDK 21
    buildozer android debug

Re-run ``sync_tests.py`` any time ``test/jpypetest/*.py`` changes, or
after adding a module name to ``testapp/tests/__init__.py``'s
``TEST_MODULES``, before the next ``buildozer android debug``.

**Whenever jpype1's source changes** (anything under ``native/``,
``native/jpype_module/``, or ``project/android/recipes/jpype1/``), p4a's own
build caching will otherwise silently reuse a stale build - it does not
hash sources, only checks "does expected output already exist". Before
rebuilding, always clear *all five* of these together (partially clearing
them has caused real failures - corrupting ``build/venv``'s pip install, or
silently reusing a stale ``_jpype.so`` - more than once)::

    D=.buildozer/android/platform/build-x86_64/build
    rm -rf "$D/other_builds/jpype1-genericndkbuild" "$D/javaclasses" \
           "$D/../dists" "$D/python-installs" "$D/venv"

Expect the *first* build to take real iteration - toolchain-flag wrangling
between p4a's NDK cross-compile environment and jpype's CMake build is the
main source of friction - and to be slow (p4a's own dependency chain,
30-60+ minutes uncached). Subsequent rebuilds are much faster, and an
app-only change (just ``testapp/main.py``, no jpype1 source change) rebuilds
in seconds.

Deploy to the emulator (create it once, matching the setup step above)::

    ~/android-sdk/platform-tools/adb install -r bin/jpypetest-0.1-x86_64-debug.apk
    ~/android-sdk/platform-tools/adb logcat -c
    ~/android-sdk/platform-tools/adb shell am start -n org.jpype.test.jpypetest/org.kivy.android.PythonActivity
    ~/android-sdk/platform-tools/adb logcat -d | grep -i 'GOLDEN\|REGRESSION'

If the emulator isn't already running, launch it with KVM acceleration via
``sg kvm`` (works without a full new login session, unlike a bare
``usermod -aG kvm`` which only takes effect on next login)::

    sg kvm -c "~/android-sdk/emulator/emulator -avd jpype-test -no-window -no-audio -no-boot-anim -gpu swiftshader_indirect > /tmp/emulator.log 2>&1" &
    ~/android-sdk/platform-tools/adb wait-for-device shell 'while [[ -z $(getprop sys.boot_completed) ]]; do sleep 2; done'

A working build/run prints (see ``testapp/main.py``; this exact output was
confirmed on-device with all seven fixes above applied)::

    === jpype android testapp starting ===
    GOLDEN PATH: PASS (<java class 'java.lang.String'>)
    REGRESSION CHECK #1257: PASS - second bootstrap() completed, no crash
    === jpype android testapp done ===

If the golden path fails, ``main.py`` prints ``GOLDEN PATH: FAIL`` and the
exception, then still runs the regression check (it does not exit early -
these are two independent checks). The regression check passes whether the
second ``_jpype.bootstrap()`` call raises or completes cleanly - either
outcome proves the process survived it. If the process aborts instead of
printing ``REGRESSION CHECK #1257: ...`` at all, look for a SIGABRT /
``libc++abi: terminating due to uncaught exception`` in logcat - that is the
original #1257 crash signature, meaning something reintroduced the missing
exception handling in ``PyJPModule_bootstrap()``.

Understood, not chased further: a trailing SIGABRT after every run
------------------------------------------------------------------------

Every run through this harness - including the very first ones, long
before any of the fixes above existed - printed
``=== jpype android testapp done ===`` and then crashed anyway with
``Fatal signal 6 (SIGABRT)`` / ``FORTIFY: pthread_mutex_lock called on a
destroyed mutex``. The full tombstone (not just the one-line summary)
made the cause unambiguous::

    pid: ..., tid: ..., name: Chrome_InProcGp
    Abort message: 'FORTIFY: pthread_mutex_lock called on a destroyed mutex'
    backtrace:
      abort -> __fortify_fatal -> pthread_mutex_lock
      -> std::__1::mutex::lock() (libc++.so)
      -> EGLContext_t::deleteOnce -> eglDestroyContext -> eglMakeCurrent
         (libEGL_emulation.so - the emulator's own GL passthrough)
      -> android::egl_display_t::makeCurrent (libEGL.so)
      -> ... libmonochrome_64.so (Chromium/TrichromeLibrary)

This is entirely inside Android's WebView/Chromium GPU process
(``Chrome_InProcGp``) and the emulator's own EGL emulation layer - nothing
in this backtrace touches JPype, Python, or this project's code. It only
happened because the ``webview`` p4a bootstrap (used initially) pulls in a
real Chromium WebView instance, which this headless test app never
actually needed - it was only chosen for the ``WebView_AndroidGetJNIEnv()``
JNI hook.

**Tried, and reverted**: switching ``p4a.bootstrap`` from ``webview`` to
``service_only`` - p4a's headless, no-UI bootstrap - does eliminate it.
It still exports ``WebView_AndroidGetJNIEnv()`` from its own
``pyjniusjni.c`` (confirmed by reading the source), so
``project/android/native/android_jnienv.c`` needed no changes, and with no
WebView/Chromium instance ever created there's no GPU thread left to
crash. A full run was confirmed completely clean under it - zero
occurrences of ``Fatal signal``, ``SIGABRT``, ``AndroidRuntime``,
``FORTIFY``, or ``tombstone`` anywhere in logcat.

But ``service_only`` traded one problem for three, all in
python-for-android's own bootstrap/build machinery (none in JPype or this
recipe) - detailed below. Two of them live in files p4a *generates fresh
per dist*, not in anything tracked by python-for-android's own source or
this repo, so they can't be fixed by a recipe change: they need to be
hand-patched again after every fresh ``buildozer android debug`` that
(re)creates the dist. That ongoing cost was judged worse than the SIGABRT
itself, which - despite looking alarming in logcat - is harmless for
actual testing purposes: it happens on Chromium's own GPU thread, strictly
*after* this app's own script has already finished and printed its
results (confirmed: ``=== jpype android testapp done ===`` always prints
first). **This harness uses ``webview`` again** (see ``buildozer.spec``);
the ``service_only`` findings below are kept for reference, in case the
tradeoff is ever worth revisiting, or the two upstream bugs get fixed:

- **NDK platform cap**: ``service_only``'s own native launcher builds via
  the legacy ``ndk-build``/``Android.mk`` path (the ``genericndkbuild``
  recipe it depends on), not CMake. This specific NDK version
  (25.1.8937393) caps that path at platform 33
  (``android-34 is above the maximum supported version android-33``),
  even though the same NDK's CMake-based toolchain (used for ``jpype1``
  itself) handles 34 fine. Fixed by lowering ``android.api`` from 34 to 33
  in ``buildozer.spec``.

- **A real p4a bug**: buildozer deliberately skips passing
  ``--orientation``/``--manifest-orientation`` when
  ``p4a.bootstrap = service_only`` (an orientation makes no sense for a
  headless service) - but the generated ``dists/<app>/build.py``'s
  ``parse_args_and_make_package()`` unconditionally reads
  ``args.orientation`` right after, raising
  ``AttributeError: 'Namespace' object has no attribute 'orientation'``.
  The same file *does* correctly guard the analogous
  ``args.sdl_orientation_hint`` line with
  ``if is_sdl_bootstrap():`` a few lines below - this looks like a simple
  oversight upstream, not a design choice.

- **Another real p4a bug**: ``service_only``'s generated
  ``PythonActivity.java`` (``org.kivy.android.PythonActivity.onCreate()``)
  passes the static ``mActivity`` field to ``PythonUtil.unpackAsset()``
  *before* ever assigning it (``this.mActivity = this;`` runs several
  lines later in the same method) - so it's still ``null`` at that call,
  crashing every launch with
  ``NullPointerException: ... getResources() on a null object reference``
  before any Python code runs at all.

Both of the last two are genuine bugs in the *generated* per-dist files
(``dists/<app>/build.py`` and
``dists/<app>/src/main/java/org/kivy/android/PythonActivity.java``) - not
in anything under ``project/android/`` or in python-for-android's checked-in
source, and not something a JPype-side recipe change can fix once and for
all. They need to be patched by hand after each fresh ``buildozer android
debug`` run that (re)creates the dist (i.e. whenever the five directories
listed above get cleared, or on a first build). Until/unless these are
fixed upstream in python-for-android, apply both patches manually:

1. In ``.buildozer/android/platform/build-x86_64/dists/<app>/build.py``,
   inside ``parse_args_and_make_package()``, wrap the
   ``args.manifest_orientation = get_manifest_orientation(...)`` call
   (search for it) in ``if get_bootstrap_name() != "service_only":``,
   matching the guard already used for ``args.sdl_orientation_hint`` right
   below it.

2. In
   ``.buildozer/android/platform/build-x86_64/dists/<app>/src/main/java/org/kivy/android/PythonActivity.java``,
   in ``onCreate()``, move (or add) ``this.mActivity = this;`` to the very
   first line of the method body, before ``resourceManager = new
   ResourceManager(this)`` and before any use of ``mActivity``/``getAppRoot()``.

Confirmed clean after both patches: a full run from launch to
``=== jpype android testapp done ===`` with zero occurrences of
``Fatal signal``, ``SIGABRT``, ``AndroidRuntime``, ``FORTIFY``, or
``tombstone`` anywhere in logcat.

Two runtime linking issues, beyond what compiles/links on the host, only
show up once the APK actually runs on-device - both fixed in the recipe,
documented here since they generalize to any similar Android C-extension
recipe:

- **libc++_shared.so must be bundled explicitly.** ``_jpype.so`` links
  against it, but it's an NDK runtime library, not part of Android's system
  image - without copying it into the APK's native lib dir (this recipe's
  ``postbuild_arch`` does so via p4a's ``install_libs()``), you get
  ``dlopen failed: library "libc++_shared.so" not found`` at import time.
- **Android's linker namespace isolation blocks implicit cross-namespace
  symbol resolution**, even for symbols that are genuinely exported.
  ``Android_JNI_GetEnv()`` calls ``WebView_AndroidGetJNIEnv()``, exported
  (confirmed via ``readelf --dyn-syms``) by the bootstrap's own
  ``libmain.so`` (true of ``webview``, where this was first found, and
  ``service_only`` alike) - but ``_jpype.so``, loaded via Python's import machinery,
  ends up in a different linker namespace than ``libmain.so``, loaded by the
  app's native launcher, so an unresolved/deferred-to-runtime reference to
  it fails with ``dlopen failed: cannot locate symbol
  "WebView_AndroidGetJNIEnv"`` even though ``--allow-shlib-undefined``/
  ``--unresolved-symbols=ignore-all`` let it link cleanly. An *explicit*
  ``DT_NEEDED`` dependency, in contrast, does get resolved across that
  boundary - this is exactly what the upstream pyjnius recipe does with its
  own ``-L.../libmain*.so`` linking, and what this recipe now does too, via
  ``native/CMakeLists.txt``'s ``JPYPE_ANDROID_LIBMAIN_DIR`` cmake define
  (**not** the ``LDFLAGS`` environment variable - scikit-build-core doesn't
  read ambient ``LDFLAGS`` the way legacy setuptools/distutils builds did;
  an env-var-only attempt at this silently has no effect on the actual link
  line, which cost real time to notice).

numpy
-----

``requirements = python3,jpype1,numpy`` in ``buildozer.spec`` now works -
p4a ships a ``numpy`` recipe, but its exact vendored source
(``numpy/_core/src/multiarray/unique.cpp``) fails to compile against this
NDK's libc++::

    ../numpy/_core/src/multiarray/unique.cpp:123:6: error: no template named
    'unordered_map' in namespace 'std'; did you mean 'unordered_set'?

A bug in numpy's own source (that file uses ``std::unordered_map`` while
only ``#include``-ing ``<unordered_set>``, relying on it transitively
pulling in ``unordered_map`` - true for libstdc++, not for the NDK's
stricter libc++), not a JPype issue, so it isn't patched in this repo's
own ``native/`` - instead, ``project/android/recipes/numpy/__init__.py``
subclasses p4a's bundled ``NumpyRecipe`` and adds one patch
(``unordered_map_include.patch``) that adds the missing include. Local
recipes take precedence over p4a's bundled ones by directory-name match
(``Recipe.recipe_dirs()`` puts ``p4a.local_recipes`` first), so this is a
small, self-contained override, not a fork of the whole recipe. Check
whether a numpy release newer than the pinned ``v2.3.0`` has already fixed
this upstream before carrying the patch forward indefinitely.

One on-device test needed gating even with numpy working:
``test_attr.py``'s ``testPassedObjectGetsCleanedUp`` sizes its stress-test
iteration count off ``Runtime.freeMemory()``, tuned for a desktop JVM's
heap; ART's heap-growth model reports a much larger figure for the same
physical memory, blowing the loop count up to where a single run measured
well past two minutes without completing even once. Not a JPype bug - the
thing under test isn't in question, only the stress loop's sizing
assumption - so it's gated with ``common.skipOnAndroid()`` like the
platform-limitation tests already are, rather than fixed (there is nothing
to fix: it is not a fixed number of iterations that could be capped
without changing what the test is stress-testing).

Stale rebuilds: when editing the jpype1 recipe or ``native/`` itself
------------------------------------------------------------------------

Running ``buildozer android debug`` again after editing something does
**not** guarantee those edits actually get rebuilt. Two independent
caches can each silently keep serving old output:

- p4a's own dist-matching reuses an existing
  ``.buildozer/android/platform/build-<arch>/dists/<dist_name>/`` by
  recipe-*name* match only, with no content hash - "jpypetest has
  compatible recipes, using this one" in the log means the entire
  ``create`` phase (``prepare_build_dir``, and *both*
  ``build_arch``/``postbuild_arch`` for every recipe) is skipped
  outright, purely because the recipe list didn't change, even if a
  recipe's own code did - including plain Python edits to ``recipe.py``
  itself, not just ``native/``. This is the one that actually bit
  harder in practice: it's easy to misread "the build succeeded and
  even printed my recipe's own log lines" as proof a change took effect
  when those log lines were really just cached ``buildozer`` output
  scrollback from an earlier run, not this run's actual execution -
  check for "jpypetest has compatible recipes, using this one" in the
  *current* run's own log before trusting anything else in it.
- Even when the ``create`` phase does run, ``Recipe.should_build()``
  (for ``jpype1``, inherited from ``PyProjectRecipe``) checks
  ``self.ctx.has_package(name, arch)`` - i.e. whether a wheel for
  ``jpype1`` is already installed under
  ``.buildozer/android/platform/build-<arch>/build/python-installs/<dist>/<arch>/``.
  If one is, ``build_arch()`` (the actual CMake/scikit-build-core
  compile of ``_jpype.so``) is skipped - silently, but ``postbuild_arch``
  still runs regardless in that case, so this layer alone only masks
  ``native/``/``pyproject.toml`` changes, not recipe.py/Java-copying
  ones.

Symptom: the build succeeds, the APK installs and runs, but a change -
native, Java, or even a plain recipe.py edit - never actually takes
effect. This cost real debugging time more than once while working on
this harness (a fixed C++ bug kept reproducing as if unfixed; a recipe
rsync exclude-list edit kept appearing to have no effect). The fix is
to force a truly fresh recipe build before re-running
``buildozer android debug``::

    rm -rf .buildozer/android/platform/build-<arch>/dists/<dist_name>
    rm -rf .buildozer/android/platform/build-<arch>/build/other_builds/jpype1-genericndkbuild
    rm -rf .buildozer/android/platform/build-<arch>/build/python-installs/<dist_name>/<arch>/jpype
    rm -rf .buildozer/android/platform/build-<arch>/build/python-installs/<dist_name>/<arch>/jpype1-*.dist-info
    rm -f  .buildozer/android/platform/build-<arch>/build/python-installs/<dist_name>/<arch>/_jpype.so

When in doubt after any ``native/`` or recipe change, do this rather than
trusting a plain rebuild - and specifically, don't treat an unchanged
symptom after an edit as proof the edit didn't fix the bug without first
confirming the rebuild actually recompiled the changed file (present in
the build log's compiler-invocation lines).

Reproducing a *new* Android bug report
-----------------------------------------

Before this harness, an Android-specific bug report could only be
root-caused by reading the stack trace and reasoning about the C++ code
(as #1257 was). With this harness in place, the preferred approach for a
new report is: extend ``testapp/main.py`` to reproduce the reported failure
mode (or as close an approximation as practical, as the #1257 regression
check above does), confirm it actually fails the same way against the
*current* code first, then fix and re-run the same build/deploy/run/logcat
loop to confirm the fix.

Follow-up: porting ``test/jpypetest`` onto this harness
-------------------------------------------------------------

``testapp/main.py`` is a minimal smoke test, not the real suite. Running
the actual ``test/jpypetest`` tests on-device is a real next step, but a
substantial one - not a quick extension of this harness:

- ``test/jpypetest/common.py`` and ``subrun.py`` assume ``jpype.startJVM()``
  and per-test subprocess isolation (a fresh JVM per test, or per test
  class). Neither exists on Android: there is exactly one already-running
  ART VM for the whole app process, started before Python even begins, and
  ``startJVM()``/``shutdownJVM()`` are both removed entirely (see
  :doc:`android`). Porting the suite means either adapting that
  infrastructure to run everything in a single shared VM/process (losing
  test isolation the desktop suite currently relies on), or building a
  different on-device test runner altogether.
- The suite's Java-side test fixtures (``test/harness/``) would need to be
  compiled and dexed into the test app the same way ``org.jpype.*`` is now
  - a much larger set of classes than the two-file fix needed for
  Reflector0.
- Some tests need numpy (see above - not currently buildable against this
  NDK without a numpy-side patch) or other host-only tooling.

None of this is blocked by anything found in this session; it's simply
unstarted, larger work that should be scoped as its own effort.
