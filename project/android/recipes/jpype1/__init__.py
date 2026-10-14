"""python-for-android recipe for JPype's own `_jpype` C extension.

Modeled on the upstream pyjnius recipe (pythonforandroid/recipes/pyjnius),
which solves the same class of problem (a JNI-based Python<->Java bridge
built as a native extension). Unlike pyjnius, this recipe does not fetch a
release tarball: it builds directly from this repo's own working tree via
`IncludedFilesBehaviour`, since the whole point of this harness is to build
and test whatever is currently checked out, not a released version.

See project/android/README.md for how this recipe fits into the overall
build/deploy/verify loop, and for the current known rough edges.
"""
import glob
import zipfile
from os import walk, sep
from os.path import join, dirname, isfile, realpath, relpath
import sh

from pythonforandroid.recipe import PyProjectRecipe, IncludedFilesBehaviour, Recipe
from pythonforandroid.logger import shprint, info, warning
from pythonforandroid.util import current_directory, ensure_dir, rmdir


# Repo root: project/android/recipes/jpype1/__init__.py -> up 4 levels.
_REPO_ROOT = join(dirname(__file__), '..', '..', '..', '..')


class JPype1Recipe(IncludedFilesBehaviour, PyProjectRecipe):
    name = 'jpype1'
    version = 'local'
    site_packages_name = 'jpype'
    depends = [('genericndkbuild', 'sdl2', 'sdl3')]

    # IncludedFilesBehaviour.prepare_build_dir() does a plain `cp -a` of
    # src_filename into the build dir. The jpype repo root also carries
    # .git/, any host-arch build/ directory (CLAUDE.md's build-isolation
    # note applies here too - a stale host build/ must never leak into an
    # Android cross-build), test artifacts, and this project/android/ tree
    # itself. None of that belongs in the Android build, so this recipe
    # overrides prepare_build_dir to rsync with excludes instead of relying
    # on the mixin's unconditional copy.
    src_filename = _REPO_ROOT

    def prepare_build_dir(self, arch):
        if self.src_filename is None:
            raise ValueError('src_filename must be set')
        build_dir = self.get_build_dir(arch)
        rmdir(build_dir)
        shprint(sh.rsync, '-a',
                '--exclude=.git',
                '--exclude=build',
                '--exclude=project/android/recipes',
                '--exclude=project/android/testapp',
                '--exclude=test/classes',
                '--exclude=__pycache__',
                self.src_filename + '/', build_dir)

    def get_recipe_env(self, arch, **kwargs):
        env = super().get_recipe_env(arch, **kwargs)

        # jpype's own build (native/CMakeLists.txt) needs to see ANDROID
        # so it takes the libc++_shared linking branch instead of the
        # generic Linux/glibc one. p4a's NDK toolchain file already sets
        # CMAKE_SYSTEM_NAME=Android; this recipe passes the toolchain file
        # itself and the per-arch ABI/platform through scikit-build-core's
        # generic `cmake.define.<VAR>=<value>` config-settings mechanism
        # (see CLAUDE.md - the same mechanism used for BUILD_TEST_HARNESS
        # in the normal host build), so no pyproject.toml changes are
        # needed on jpype's side for the toolchain plumbing itself.
        toolchain_file = join(self.ctx.ndk_dir, 'build', 'cmake',
                               'android.toolchain.cmake')

        # CMake's FindPython3 (native/CMakeLists.txt's
        # `find_package(Python3 COMPONENTS Interpreter Development.Module
        # REQUIRED)`) can't introspect the target Android Python the way it
        # does a normal host build - there is no sysconfig to query and the
        # target's own python binary can't run on the host CPU anyway. Point
        # it at what p4a already built for exactly this purpose: a
        # host-runnable "hostpython" (self.real_hostpython_location, the
        # same one build_arch() below invokes `python -m build` with) for
        # the Interpreter component, and the target Android sysroot's real
        # headers/libpython for Development.Module.
        python_recipe = Recipe.get_recipe('python3', self.ctx)
        python_include_dir = python_recipe.include_root(arch.arch)
        python_library = join(python_recipe.link_root(arch.arch),
                               python_recipe._libpython)

        # Android_JNI_GetEnv() (project/android/native/android_jnienv.c)
        # calls WebView_AndroidGetJNIEnv(), exported by the webview
        # bootstrap's libmain.so - confirmed present and GLOBAL DEFAULT in
        # its dynamic symbol table. Leaving it as a plain unresolved
        # import (the --unresolved-symbols=ignore-all path in
        # native/CMakeLists.txt) isn't enough on-device: Android's linker
        # namespace isolation blocks a same-process-but-different-namespace
        # symbol lookup for a merely-undefined reference (_jpype.so, loaded
        # via Python's import machinery, ends up in a different linker
        # namespace than libmain.so, loaded by the app's native launcher).
        # An explicit DT_NEEDED dependency on libmain.so, however, *does*
        # get resolved across that boundary - this mirrors exactly what
        # the upstream pyjnius recipe's get_recipe_env() does with its own
        # -L.../libmain*.so linking.
        # Passed as a cmake.define (native/CMakeLists.txt's
        # JPYPE_ANDROID_LIBMAIN_DIR), not via the LDFLAGS environment
        # variable: scikit-build-core does not read ambient LDFLAGS the
        # way legacy setuptools/distutils builds did, so an env-var-only
        # attempt at this silently has no effect on the actual link line.
        libmain_dir = join(self.ctx.bootstrap.build_dir, 'libs', arch.arch)
        self.extra_build_args = self.extra_build_args + [
            '--config-setting=cmake.define.CMAKE_TOOLCHAIN_FILE=' + toolchain_file,
            '--config-setting=cmake.define.ANDROID_ABI=' + arch.arch,
            '--config-setting=cmake.define.ANDROID_PLATFORM=android-' + str(self.ctx.ndk_api),
            '--config-setting=cmake.define.ENABLE_BUILD_JAR=OFF',
            '--config-setting=cmake.define.Python3_EXECUTABLE=' + self.real_hostpython_location,
            '--config-setting=cmake.define.Python3_INCLUDE_DIR=' + python_include_dir,
            '--config-setting=cmake.define.Python3_LIBRARY=' + python_library,
            '--config-setting=cmake.define.JPYPE_ANDROID_LIBMAIN_DIR=' + libmain_dir,
        ]
        return env

    def check_prebuilt(self, arch, msg=""):
        # PyProjectRecipe.lookup_prebuilt() builds a pip requirement string
        # "name==version" to dry-run-check for a prebuilt wheel; version =
        # 'local' isn't a valid PEP440 specifier, so that dry-run raises
        # (harmlessly - p4a's own thread wrapper swallows it and falls
        # through to a real build) but it's noisy. This recipe always
        # builds from this repo's own working tree, so skip the prebuilt
        # lookup outright instead.
        return False

    def build_arch(self, arch):
        # PyProjectRecipe.build_arch() (pythonforandroid/recipe.py) always
        # invokes `python -m build --wheel --config-setting builddir=...`,
        # hardcoding a config-setting name that's specific to meson-python
        # (see the sibling MesonRecipe class, which shares this same base
        # build_arch). scikit-build-core - what jpype's own pyproject.toml
        # actually uses - calls the equivalent option `build-dir`, and
        # rejects an unrecognized `builddir` outright rather than ignoring
        # it. There's no override hook for just this one flag, so this is
        # a full copy of the upstream method with that one line corrected;
        # keep it in sync with upstream's build_arch if it changes.
        if self.check_prebuilt(arch, "skipping build_arch"):
            result = self.install_prebuilt_wheel(arch)
            if result:
                return
            warning("Failed to install prebuilt wheel, falling back to build_arch")

        build_dir = self.get_build_dir(arch.arch)
        if not (isfile(join(build_dir, "pyproject.toml")) or isfile(join(build_dir, "setup.py"))):
            warning("Skipping build because it does not appear to be a Python project.")
            return
        self.install_hostpython_prerequisites(
            packages=["build[virtualenv]", "pip", "setuptools", "patchelf"] + self.hostpython_prerequisites
        )

        env = self.get_recipe_env(arch, with_flags_in_cc=True)
        sub_build_dir = join(build_dir, "p4a_android_build")
        ensure_dir(sub_build_dir)

        build_args = [
            "-m",
            "build",
            "--wheel",
            "--config-setting",
            "build-dir={}".format(sub_build_dir),
        ] + self.extra_build_args

        built_wheels = []
        with current_directory(build_dir):
            shprint(
                sh.Command(self.real_hostpython_location), *build_args, _env=env
            )
            built_wheels = [realpath(whl) for whl in glob.glob("dist/*.whl")]
        self.install_wheel(arch, built_wheels)

    # p4a arch name -> NDK sysroot triple, for locating libc++_shared.so.
    _NDK_TRIPLE = {
        'arm64-v8a': 'aarch64-linux-android',
        'armeabi-v7a': 'arm-linux-androideabi',
        'x86': 'i686-linux-android',
        'x86_64': 'x86_64-linux-android',
    }

    def postbuild_arch(self, arch):
        super().postbuild_arch(arch)

        # _jpype.so is linked against libc++_shared.so (native/CMakeLists.txt's
        # ANDROID branch), but that's an NDK runtime library, not something
        # Android's system image provides - it has to be bundled into the
        # APK's own native library dir explicitly, or dlopen(_jpype.so)
        # fails at runtime with "library libc++_shared.so not found" (it
        # doesn't matter that _jpype.so itself built and installed fine).
        libcxx = join(self.ctx.ndk_dir, 'toolchains', 'llvm', 'prebuilt',
                      'linux-x86_64', 'sysroot', 'usr', 'lib',
                      self._NDK_TRIPLE[arch.arch], 'libc++_shared.so')
        self.install_libs(arch, libcxx)

        # See jp_classloader.cpp: FindClass("org/jpype/JPypeClassLoader")
        # must resolve on-device. That class comes from the same
        # native/jpype_module Java sources the host build packages into
        # org.jpype.jar via Ant - on Android they need to reach the APK's
        # classes.dex instead, which p4a/buildozer handles automatically
        # for anything placed under javaclass_dir before the Java/dex
        # build step runs (the same mechanism pyjnius's postbuild_arch
        # uses for its own org/ sources).
        info('Copying org.jpype Java sources to classes build dir')
        with current_directory(self.get_build_dir(arch.arch)):
            shprint(sh.cp, '-a',
                    join('native', 'jpype_module', 'src', 'main', 'java', 'org'),
                    self.ctx.javaclass_dir)

            # Reflector0.java lives outside that org/ tree on purpose -
            # native/build.xml excludes it from the normal javac pass and
            # compiles it separately (into META-INF/versions/0/, see that
            # file), so its source sits at .../java/exclude/org/jpype/
            # rather than .../java/org/jpype/ and the copy above misses it.
            # JPypeContext.createContext() does
            # `Class.forName("org.jpype.Reflector0", ...)` unconditionally
            # (not Android-specific) to get a dedicated stack frame for
            # invoking caller-sensitive Java methods correctly - without
            # this file compiled in, that lookup fails with "Unable to
            # create reflector", which is a missing-source bug in this
            # recipe, not an Android/ART bytecode-generation limitation.
            shprint(sh.cp,
                    join('native', 'jpype_module', 'src', 'main', 'java',
                         'exclude', 'org', 'jpype', 'Reflector0.java'),
                    join(self.ctx.javaclass_dir, 'org', 'jpype', 'Reflector0.java'))

            # test/harness/jpype/* - the Java-side fixtures the ported
            # test/jpypetest/*.py tests need (e.g. jpype.common.Fixture,
            # jpype.array.TestArray). See project/android/testapp/tests/.
            # Excludes attr/ClassWithBuffer.java, which imports
            # java.awt.image.BufferStrategy - AWT isn't part of Android's
            # platform API (see doc/android.rst) and that one file would
            # fail to compile against android.jar; everything else in the
            # harness tree was checked and has no such dependency.
            #
            # annotation/ and reflect/ (custom @Retention(RUNTIME)
            # annotation types) were excluded for a while after an
            # earlier build hit an ART/CheckJNI startup abort with them
            # present. Root-caused (not a stale build, though one of
            # those - see doc/android_build.rst's "Stale rebuilds"
            # section - did mask the fix while investigating): a raw JNI
            # call using a methodID cached from an interface declaration
            # (or a java.lang.reflect.Proxy class's own getMethods()
            # result) isn't reliably usable against that specific Proxy
            # instance on ART's CheckJNI - see native/common/jp_method.cpp's
            # JPMethod::invoke, the m_ReflectProxyClass check. Fixed
            # there; the full suite, including test_annotation.py/
            # test_reflect.py, now runs clean.
            info('Copying test/harness Java fixtures to classes build dir')
            shprint(sh.rsync, '-a',
                    '--exclude=attr/ClassWithBuffer.java',
                    join('test', 'harness', 'jpype') + '/',
                    join(self.ctx.javaclass_dir, 'jpype'))

            # test/harness/org/jpype/fail/* - fixtures for test_exc.py's
            # testExcCauseChained1/2 (classes whose static initializers
            # deliberately throw, to exercise ExceptionInInitializerError
            # chaining). This is a sibling tree to test/harness/jpype
            # above (rooted at org/, not jpype/) that the original rsync
            # above never picked up - checked for AWT/annotation issues
            # the same way the jpype/ tree was, found none.
            info('Copying test/harness/org Java fixtures to classes build dir')
            shprint(sh.rsync, '-a',
                    join('test', 'harness', 'org') + '/',
                    join(self.ctx.javaclass_dir, 'org'))

            self.generate_package_markers(arch)

    def generate_package_markers(self, arch):
        """Emit a list of every Java package Android's build can reach, so
        JPypePackageManager.isPackage() (see
        native/jpype_module/src/main/java/org/jpype/pkg/
        JPypePackageManager.java) can answer "is this a valid package"
        cheaply instead of needing the jar/jrt filesystem enumeration ART
        doesn't have. Without this, jpype.imports and jpype.JPackage(...)
        - both of which resolve a dotted name one package component at a
        time - fail at the very first component (see doc/android.rst's
        "Removed JPype Services").

        Scans everything that ends up on this build's classpath: the
        Android platform stub jar (java.*, javax.*, android.*, ...) and
        the org.jpype / test-harness sources just copied into
        javaclass_dir above (compiled/dexed by p4a's own subsequent
        build step, not by this recipe).

        Two earlier attempts at bundling this list both looked right at
        the point they ran, and both turned out not to survive into the
        actual APK:

        1. Writing one empty marker file per package under javaclass_dir
           (src/main/java), on the theory that non-.java files sitting in
           a Java source directory ride along as generic resources -
           false: Android Gradle's default java source set silently drops
           anything that isn't a .java file.
        2. Writing the same marker tree under the bootstrap's own
           src/main/assets/ (self.ctx.bootstrap.build_dir) instead, since
           assets/ is where the webview bootstrap's own
           _load.html/private.tar demonstrably do survive into the APK.
           Still didn't work, and for a subtler reason: those two files
           don't actually originate from that source directory either -
           bootstraps/common/build/build.py's make_package() (the step
           that runs *after* p4a's own dist assembly, actually invoking
           gradle) does `rmdir(assets_dir); ensure_dir(assets_dir)` and
           repopulates it from scratch, from only two sources: the
           bootstrap's separate webview_includes/ directory (a flat,
           non-recursive copy - that's where _load.html really lives) and
           whatever was passed via repeatable `--asset SRC:DEST` CLI
           args. Anything already sitting in assets_dir from the earlier
           `cp -r` gets wiped, unconditionally, every build.

        So this now goes through that same `--asset` mechanism instead of
        writing into any p4a-internal build directory directly: a single
        flat package-list file at a path fixed at recipe-authoring time
        (not per-arch, not under ctx.build_dir), which
        project/android/testapp/buildozer.spec's `android.add_assets`
        setting points at explicitly. That setting is what actually
        produces the `--asset` argument build.py's asset-copy step reads
        - see targets/android.py's `android.add_assets` handling in
        buildozer and toolchain.py's `--add-asset` argument in p4a. A
        single list file (one dotted package name per line) is also
        simpler and cheaper for JPypePackageManager to read back than
        hundreds of individual marker files/dirs would be.
        """
        info('Generating Android package markers for jpype.imports/JPackage')
        packages = set()

        def add_with_ancestors(dotted):
            parts = dotted.split('.')
            for i in range(1, len(parts) + 1):
                packages.add('.'.join(parts[:i]))

        android_jar = join(self.ctx.sdk_dir, 'platforms',
                            'android-{}'.format(self.ctx.android_api), 'android.jar')
        with zipfile.ZipFile(android_jar) as zf:
            for entry in zf.namelist():
                if not entry.endswith('.class') or '$' in entry:
                    continue
                pkg_path = dirname(entry)
                if not pkg_path:
                    continue
                add_with_ancestors(pkg_path.replace('/', '.'))

        for root_name in ('org', 'jpype'):
            root_dir = join(self.ctx.javaclass_dir, root_name)
            for dirpath, _dirnames, filenames in walk(root_dir):
                if not any(f.endswith('.java') for f in filenames):
                    continue
                rel = relpath(dirpath, self.ctx.javaclass_dir)
                add_with_ancestors(rel.replace(sep, '.'))

        # Fixed path, known at buildozer.spec-authoring time (NOT under
        # self.ctx.build_dir/bootstrap.build_dir - see the docstring above
        # for why writing into any p4a-internal build directory doesn't
        # survive into the final APK). buildozer.spec's `android.add_assets`
        # references this exact path; it just needs to exist and be
        # populated by the time build.py's asset-copy step runs, which is
        # after this recipe's postbuild_arch (still within the same
        # buildozer invocation).
        ensure_dir(join(dirname(__file__), 'generated'))
        package_list_path = join(dirname(__file__), 'generated', 'android-packages.txt')
        with open(package_list_path, 'w') as fileh:
            for pkg in sorted(packages):
                fileh.write(pkg + '\n')
        info('Wrote {} Android package names to {}'.format(len(packages), package_list_path))

        # org.jpype.html.Html's entities.txt hits the exact same
        # non-.java-files-get-dropped problem as the package list above
        # (it sits right next to Html.java under src/main/java, and never
        # reached the built APK before this fix - see that class's static
        # initializer for the AssetManager fallback that reads this copy
        # back). It's a small, static, already-in-the-repo file, so this
        # just stages a copy here for the same buildozer.spec
        # android.add_assets mechanism to pick up - no scan needed.
        entities_src = join(self.get_build_dir(arch.arch), 'native', 'jpype_module',
                             'src', 'main', 'java', 'org', 'jpype', 'html', 'entities.txt')
        entities_dst = join(dirname(__file__), 'generated', 'entities.txt')
        shprint(sh.cp, entities_src, entities_dst)
        info('Staged Android asset copy of entities.txt at {}'.format(entities_dst))

        self.generate_javadoc_assets(arch)

    def generate_javadoc_assets(self, arch):
        """Generate and stage test_javadoc.py's fixture docs (jpype.doc.Test)
        as Android assets, same reasoning and mechanism as entities.txt
        above: JavadocExtractor.getDocumentationAsStream() looks up
        "<class/as/a/path>.html" via a classloader resource lookup, which
        works on desktop only because `test/build.xml`'s `javadoc` Ant
        target has already generated and placed that file on the
        classpath (test/classes/jpype/doc/Test.html) as part of the
        normal desktop test setup - nothing analogous runs for this
        Android build, so the file never existed here at all (not even a
        bundling gap this time, a generation gap).

        Runs the exact same Ant target here, inside this recipe's own
        build dir (a full rsynced copy of the repo, so test/build.xml and
        test/harness/jpype/doc/Test.java are both present - see
        prepare_build_dir above), then stages the one output file that
        matters under generated/javadoc/, preserving jpype.doc.Test's
        class-name-derived relative path so JavadocExtractor's Android
        fallback (see that class) can look it up the same way for
        whatever class, not just this one.
        """
        build_dir = self.get_build_dir(arch.arch)
        with current_directory(build_dir):
            shprint(sh.ant, '-f', join('test', 'build.xml'), 'javadoc')
        doc_src = join(build_dir, 'test', 'classes', 'jpype', 'doc', 'Test.html')
        doc_dst = join(dirname(__file__), 'generated', 'javadoc', 'jpype', 'doc', 'Test.html')
        ensure_dir(dirname(doc_dst))
        shprint(sh.cp, doc_src, doc_dst)
        info('Staged Android asset copy of jpype.doc.Test javadoc at {}'.format(doc_dst))


recipe = JPype1Recipe()
