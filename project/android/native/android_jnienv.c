/*
 * Reference implementation of Android_JNI_GetEnv(), which
 * native/python/pyjp_module.cpp declares (extern) and calls from
 * PyJPModule_bootstrap() but never defines - by design, that symbol is
 * meant to be supplied by whatever Android app host embeds JPype.
 *
 * This file supplies it for the specific host this project/android/
 * harness builds against: python-for-android's "genericndkbuild"/webview
 * bootstrap, which exposes the current thread's JNIEnv via
 * WebView_AndroidGetJNIEnv() (see pythonforandroid/bootstraps/webview).
 * A different host application embedding JPype (e.g. a from-scratch
 * Android Studio project, or a different p4a bootstrap) would supply its
 * own definition of Android_JNI_GetEnv() instead of this one - this file
 * is deliberately not part of the core native/ sources for that reason;
 * see native/CMakeLists.txt's ANDROID branch, which only compiles it in
 * when project/android/ is present (i.e. only for this harness's own
 * build, never for a normal host wheel build).
 */
#include <jni.h>

extern JNIEnv *WebView_AndroidGetJNIEnv(void);

JNIEnv *Android_JNI_GetEnv(void)
{
	return WebView_AndroidGetJNIEnv();
}
