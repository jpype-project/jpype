[app]
title = JPype Android Test
package.name = jpypetest
package.domain = org.jpype.test
source.dir = .
source.include_exts = py
version = 0.1

# jpype1 is resolved via p4a.local_recipes below, against
# project/android/recipes/jpype1, which builds from this repo's own
# working tree (see that recipe's docstring).
#
# numpy: p4a ships its own recipe (meson-based, needs android.minapi >= 24,
# already the case below) - lets test/jpypetest's numpy-dependent tests
# (gated behind common.requireNumpy, currently skipped on Android for lack
# of numpy) actually run on-device instead.
requirements = python3,jpype1,numpy

# webview was evaluated against service_only (headless, no-UI) to chase
# down a trailing SIGABRT - root-caused to a Chromium WebView GPU-thread
# EGL teardown crash in the Android emulator's own EGL layer, confirmed
# via full tombstone to be unrelated to jpype and to happen strictly
# *after* this app's own script already completes (see doc/android_build.rst).
# service_only avoids it but has its own cost: it hit three real upstream
# python-for-android bugs, two of which live in *generated* per-dist files
# that must be hand-patched after every fresh dist creation (not durable,
# not something a recipe change can fix). webview needs none of that and
# is the more standard, better-trodden path, so it's used here again -
# the trailing SIGABRT is real but harmless for testing purposes, since it
# happens after this app's own script has already finished and reported
# its results.
p4a.bootstrap = webview

p4a.local_recipes = ../recipes

# jpype1's recipe (postbuild_arch's generate_package_markers) writes the
# full list of Java packages reachable on this build's classpath here, as
# one dotted name per line - see that recipe for why it has to go through
# add_assets rather than being written directly into any p4a-internal
# build directory (didn't survive into the APK either way it was tried).
# JPypePackageManager.java (native/jpype_module) reads this back via
# AssetManager under the jpype-android-packages.txt name on the right of
# the colon.
#
# entities.txt is org.jpype.html.Html's static resource file - same
# non-.java-files-get-dropped problem, same fix. See Html.java's static
# initializer and JPypePackageManager.openAndroidAsset().
#
# generated/javadoc/ is a directory (not a single file, unlike the two
# above) - test/jpypetest/test_javadoc.py's testClass/testMethod need
# jpype.doc.Test's generated javadoc HTML, which desktop gets from
# test/build.xml's javadoc Ant target running as part of normal desktop
# test setup. Nothing analogous runs for Android otherwise, so the
# recipe runs that same Ant target itself and stages the one file that
# matters (jpype/doc/Test.html, preserving that relative path) here.
# build.py's asset-copy step copytree()s a directory source wholesale,
# so the bundled asset preserves the same jpype/doc/Test.html layout -
# see JavadocExtractor's Android fallback, which looks up
# "jpype-android-javadoc/" + <the same relative path> for any class.
android.add_assets = ../recipes/jpype1/generated/android-packages.txt:jpype-android-packages.txt,../recipes/jpype1/generated/entities.txt:jpype-android-html-entities.txt,../recipes/jpype1/generated/javadoc:jpype-android-javadoc

android.api = 34
android.minapi = 24
android.ndk = 25.1.8937393
android.archs = x86_64

# Point at the SDK/NDK already installed locally (see doc/android_build.rst
# setup step) instead of buildozer's default behavior of downloading its
# own separate copies into ~/.buildozer/android/platform/ - besides being
# wasteful, that default download path has also been observed pulling in a
# stale commandlinetools URL that 404s.
#
# ADJUST THESE TWO PATHS to wherever you installed the SDK/NDK locally -
# these are absolute paths, not something buildozer.spec can make portable.
android.sdk_path = /home/kenelson/android-sdk
android.ndk_path = /home/kenelson/android-sdk/ndk/25.1.8937393

log_level = 2

[buildozer]
warn_on_root = 1
